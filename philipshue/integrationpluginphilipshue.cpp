/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*
* Copyright 2013 - 2020, nymea GmbH
* Contact: contact@nymea.io
*
* This file is part of nymea.
* This project including source code and documentation is protected by
* copyright law, and remains the property of nymea GmbH. All rights, including
* reproduction, publication, editing and translation, are reserved. The use of
* this project is subject to the terms of a license agreement to be concluded
* with nymea GmbH in accordance with the terms of use of nymea GmbH, available
* under https://nymea.io/license
*
* GNU Lesser General Public License Usage
* Alternatively, this project may be redistributed and/or modified under the
* terms of the GNU Lesser General Public License as published by the Free
* Software Foundation; version 3. This project is distributed in the hope that
* it will be useful, but WITHOUT ANY WARRANTY; without even the implied
* warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
* Lesser General Public License for more details.
*
* You should have received a copy of the GNU Lesser General Public License
* along with this project. If not, see <https://www.gnu.org/licenses/>.
*
* For any further details and any questions please contact us under
* contact@nymea.io or see our FAQ/Licensing Information on
* https://nymea.io/license/faq
*
* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

#include "integrationpluginphilipshue.h"

#include "integrations/thing.h"
#include "types/param.h"
#include "plugininfo.h"
#include "network/upnp/upnpdiscovery.h"
#include "network/upnp/upnpdiscoveryreply.h"

#include "platform/platformzeroconfcontroller.h"
#include "network/zeroconf/zeroconfservicebrowser.h"

#include <QDebug>
#include <QColor>
#include <QDateTime>
#include <QStringList>
#include <QJsonDocument>

IntegrationPluginPhilipsHue::IntegrationPluginPhilipsHue()
{

}

IntegrationPluginPhilipsHue::~IntegrationPluginPhilipsHue()
{
    hardwareManager()->pluginTimerManager()->unregisterTimer(m_pluginTimer1Sec);
    hardwareManager()->pluginTimerManager()->unregisterTimer(m_pluginTimer5Sec);
    hardwareManager()->pluginTimerManager()->unregisterTimer(m_pluginTimer15Sec);
}

void IntegrationPluginPhilipsHue::init()
{
    m_pluginTimer1Sec = hardwareManager()->pluginTimerManager()->registerTimer(1);
    connect(m_pluginTimer1Sec, &PluginTimer::timeout, this, [this]() {
        // refresh sensors every second
        foreach (HueBridge *bridge, m_bridges.keys()) {
            refreshSensors(bridge);
        }
    });
    m_pluginTimer5Sec = hardwareManager()->pluginTimerManager()->registerTimer(5);
    connect(m_pluginTimer5Sec, &PluginTimer::timeout, this, [this]() {
        // refresh lights every 5 seconds
        foreach (HueBridge *bridge, m_bridges.keys()) {
            refreshLights(bridge);
        }
    });
    m_pluginTimer15Sec = hardwareManager()->pluginTimerManager()->registerTimer(15);
    connect(m_pluginTimer15Sec, &PluginTimer::timeout, this, [this]() {
        // refresh bridges every 15 seconds
        foreach (Thing *thing, m_bridges.values()) {
            refreshBridge(thing);
        }
    });

    m_zeroConfBrowser = hardwareManager()->zeroConfController()->createServiceBrowser("_hue._tcp");
    connect(m_zeroConfBrowser, &ZeroConfServiceBrowser::serviceEntryAdded, this, [=](const ZeroConfServiceEntry &entry){
        qCDebug(dcPhilipsHue()) << "service entry added!" << entry;
        if (entry.protocol() != QAbstractSocket::IPv4Protocol) {
            return;
        }
        QString bridgeId = normalizeBridgeId(entry.txt("bridgeid"));
        Thing *thing = bridgeForBridgeId(bridgeId);
        if (!thing) {
            qCDebug(dcPhilipsHue()) << "We don't know this bridge yet...";
            return;
        }
        thing->setParamValue(bridgeThingHostParamTypeId, entry.hostAddress().toString());
        pluginStorage()->beginGroup(thing->id().toString());
        pluginStorage()->setValue("hostCache", entry.hostAddress().toString());
        pluginStorage()->endGroup();
        HueBridge *bridge = m_bridges.key(thing);
        bridge->setHostAddress(entry.hostAddress());
    });
}

void IntegrationPluginPhilipsHue::discoverThings(ThingDiscoveryInfo *info)
{
    // We're starting a mDNS-, Upnp- and a NUpnpDiscovery.
    // For that, we create a tracking object holding pointers to both of those discoveries.
    // Both discoveries add their results to a temporary list.
    // Once a discovery is finished, it will remove itself from the tracking object.
    // When both discoveries are gone from the tracking object, the results are processed
    // deduped (a bridge can be found on both discovieries) and handed over to the ThingDiscoveryInfo.

    // Tracking object
    DiscoveryJob *discovery = new DiscoveryJob();
    connect(info, &ThingDiscoveryInfo::destroyed, this, [=](){ delete discovery; });

    foreach (const ZeroConfServiceEntry &entry, m_zeroConfBrowser->serviceEntries()) {

        /*
         * Philips Hue - xxxxxx._hue._tcp._local
         * protocol: tcp
         * service: hue
         * name: Philips Hue - xxxxxx where xxxxxx are the last 6 digits of the bridge ID.
        */
        if (!entry.serviceType().contains("_hue._tcp") || entry.protocol() != QAbstractSocket::IPv4Protocol) {
            continue;
        }

        QString id = normalizeBridgeId(entry.txt("bridgeid"));
        QHostAddress address = entry.hostAddress();

        ParamList params;
        params.append(Param(bridgeThingHostParamTypeId, address.toString()));
        params.append(Param(bridgeThingIdParamTypeId, id));

        ThingDescriptor descriptor(bridgeThingClassId, "Philips Hue Bridge", address.toString());
        descriptor.setParams(params);

        foreach (Thing *thing, myThings()) {
            if (thing->paramValue(bridgeThingIdParamTypeId).toString() == id) {
                descriptor.setThingId(thing->id());
                break;
            }
        }
        qCDebug(dcPhilipsHue()) << "mDNS: Found Hue bridge:" << id << address.toString();
        discovery->results.append(descriptor);
    }

    finishDiscovery(info, discovery);
}

void IntegrationPluginPhilipsHue::startUpnPDiscovery(ThingDiscoveryInfo *info, DiscoveryJob *discovery)
{
    qCDebug(dcPhilipsHue()) << "Starting UPnP discovery...";
    UpnpDiscoveryReply *upnpReply = hardwareManager()->upnpDiscovery()->discoverDevices("libhue:idl");
    discovery->upnpReply = upnpReply;
    // Always clean up the upnp discovery
    connect(upnpReply, &UpnpDiscoveryReply::finished, upnpReply, &UpnpDiscoveryReply::deleteLater);

    // Process results if info is still around
    connect(upnpReply, &UpnpDiscoveryReply::finished, info, [=](){

        // Indicate we're done...
        discovery->upnpDone = true;
        discovery->upnpReply = nullptr;

        if (upnpReply->error() != UpnpDiscoveryReply::UpnpDiscoveryReplyErrorNoError) {
            qCWarning(dcPhilipsHue()) << "Upnp discovery error" << upnpReply->error();
        }

        foreach (const UpnpDeviceDescriptor &upnpDevice, upnpReply->deviceDescriptors()) {
            if (upnpDevice.modelDescription().contains("Philips")) {
                ThingDescriptor descriptor(bridgeThingClassId, "Philips Hue Bridge", upnpDevice.hostAddress().toString());
                ParamList params;
                QString bridgeId = upnpDevice.serialNumber().toLower();

                // We need to add it, check if this bridge is already set up
                foreach (Thing *existingThing, myThings()) {
                    if (existingThing->paramValue(bridgeThingIdParamTypeId).toString() == bridgeId) {
                        descriptor.setThingId(existingThing->id());
                        break;
                    }
                }
                params.append(Param(bridgeThingHostParamTypeId, upnpDevice.hostAddress().toString()));
                params.append(Param(bridgeThingIdParamTypeId, bridgeId));
                descriptor.setParams(params);
                qCDebug(dcPhilipsHue()) << "UPnP: Found Hue bridge:" << bridgeId << upnpDevice.hostAddress().toString();
                discovery->results.append(descriptor);
            }
        }

        finishDiscovery(info, discovery);
    });
}

void IntegrationPluginPhilipsHue::startNUpnpDiscovery(ThingDiscoveryInfo *info, DiscoveryJob *discovery)
{
    qCDebug(dcPhilipsHue) << "Starting N-UPNP discovery...";
    QNetworkRequest request(QUrl("https://discovery.meethue.com"));
    QNetworkReply *nUpnpReply = hardwareManager()->networkManager()->get(request);
    discovery->nUpnpReply = nUpnpReply;

    // Always clean up the network reply
    connect(nUpnpReply, &QNetworkReply::finished, nUpnpReply, &QNetworkReply::deleteLater);

    // Process results if info is still around
    connect(nUpnpReply, &QNetworkReply::finished, info, [=](){

        discovery->nUpnpReply = nullptr;
        discovery->nUpnpDone = true;

        if (nUpnpReply->error() != QNetworkReply::NoError) {
            qCWarning(dcPhilipsHue()) << "N-UPnP discovery failed:" << nUpnpReply->error() << nUpnpReply->errorString();
            finishDiscovery(info, discovery);
            return;
        }
        QJsonParseError error;
        QJsonDocument jsonDoc = QJsonDocument::fromJson(nUpnpReply->readAll(), &error);
        if (error.error != QJsonParseError::NoError) {
            qCWarning(dcPhilipsHue) << "N-UPNP discovery JSON error in response" << error.errorString();
            finishDiscovery(info, discovery);
            return;
        }

        foreach (const QVariant &bridgeVariant, jsonDoc.toVariant().toList()) {
            QVariantMap bridgeMap = bridgeVariant.toMap();
            ThingDescriptor descriptor(bridgeThingClassId, "Philips Hue Bridge", bridgeMap.value("internalipaddress").toString());
            ParamList params;
            QString bridgeId = normalizeBridgeId(bridgeMap.value("id").toString());
            QString address = bridgeMap.value("internalipaddress").toString();
            params.append(Param(bridgeThingIdParamTypeId, bridgeId));
            params.append(Param(bridgeThingHostParamTypeId, address));
            descriptor.setParams(params);
            qCDebug(dcPhilipsHue()) << "N-UPnP: Found Hue bridge:" << bridgeId << address;
            discovery->results.append(descriptor);
        }

        finishDiscovery(info, discovery);
    });
}

void IntegrationPluginPhilipsHue::finishDiscovery(ThingDiscoveryInfo *info, IntegrationPluginPhilipsHue::DiscoveryJob *job)
{
    if (job->upnpReply || job->nUpnpReply) {
        // still pending...
        return;
    }

    if (job->results.isEmpty() && !job->upnpDone) {
        qCDebug(dcPhilipsHue()) << "No results on mDNS. Trying UPnP...";
        startUpnPDiscovery(info, job);
        return;
    }
    if (job->results.isEmpty() && !job->nUpnpDone) {
        qCDebug(dcPhilipsHue()) << "No results on UPnP. Trying NUPnP...";
        startNUpnpDiscovery(info, job);
        return;
    }

    QHash<QString, ThingDescriptor> results;
    foreach (ThingDescriptor result, job->results) {
        // dedupe
        QString bridgeId = result.params().paramValue(bridgeThingIdParamTypeId).toString();
        if (results.contains(bridgeId)) {
            qCDebug(dcPhilipsHue()) << "Discarding duplicate search result" << bridgeId;
            continue;
        }
        Thing *dev = bridgeForBridgeId(bridgeId);
        if (dev) {
            qCDebug(dcPhilipsHue()) << "Bridge already added in system:" << bridgeId;
            result.setThingId(dev->id());
        }
        results.insert(bridgeId, result);

    }

    info->addThingDescriptors(results.values());
    info->finish(Thing::ThingErrorNoError);
}

void IntegrationPluginPhilipsHue::startPairing(ThingPairingInfo *info)
{
    Q_ASSERT_X(info->thingClassId() == bridgeThingClassId, "DevicePluginPhilipsHue::startPairing", "Unexpected thing class.");

    info->finish(Thing::ThingErrorNoError, QT_TR_NOOP("Please press the button on the Hue Bridge within 30 seconds before you continue."));
}

void IntegrationPluginPhilipsHue::setupThing(ThingSetupInfo *info)
{
    Thing *thing = info->thing();

    // Update the name on the bridge if the user changes the thing name
    connect(thing, &Thing::nameChanged, this, &IntegrationPluginPhilipsHue::onDeviceNameChanged);

    // hue bridge
    if (thing->thingClassId() == bridgeThingClassId) {
        // Loaded bridge
        qCDebug(dcPhilipsHue) << "Setup Hue Bridge" << thing->params();

        pluginStorage()->beginGroup(thing->id().toString());
        QString apiKey = pluginStorage()->value("apiKey").toString();
        QString hostCache = pluginStorage()->value("hostCache").toString();
        pluginStorage()->endGroup();

        // For legacy reasons we might not have the api key in the pluginstorage yet. Check if there is a key in the thing params.
        if (apiKey.isEmpty()) {
            qCWarning(dcPhilipsHue()) << "Loading api key from thing params!";
            // Used to be in json, not any more.
            ParamTypeId bridgeThingApiKeyParamTypeId = ParamTypeId("{8bf5776a-d5a6-4600-8b27-481f0d803a8f}");
            apiKey = thing->paramValue(bridgeThingApiKeyParamTypeId).toString();
        }

        if (apiKey.isEmpty()) {
            qCWarning(dcPhilipsHue()) << "Failed to load api key";
            info->finish(Thing::ThingErrorAuthenticationFailure, QT_TR_NOOP("Not authenticated to bridge. Please reconfigure the bridge."));
            return;
        }
        HueBridge *bridge = m_bridges.key(thing);
        if (!bridge) {
            bridge = new HueBridge(this);
            m_bridges.insert(bridge, thing);
        }
        bridge->setApiKey(apiKey);

        QHostAddress zeroconfAddress;
        foreach (const ZeroConfServiceEntry &entry, m_zeroConfBrowser->serviceEntries()) {
            if (entry.protocol() == QAbstractSocket::IPv4Protocol && normalizeBridgeId(entry.txt("bridgeid")) == thing->paramValue(bridgeThingIdParamTypeId).toString()) {
                zeroconfAddress = entry.hostAddress();
            }
        }
        if (!zeroconfAddress.isNull()) {
            qCDebug(dcPhilipsHue()) << "Using IP address from zeroconf:" << zeroconfAddress.toString();
            bridge->setHostAddress(zeroconfAddress);
            pluginStorage()->beginGroup(thing->id().toString());
            pluginStorage()->setValue("hostCache", zeroconfAddress.toString());
            pluginStorage()->endGroup();
        } else if (!hostCache.isEmpty()) {
            qCDebug(dcPhilipsHue()) << "Using last known IP:" << hostCache;
            bridge->setHostAddress(QHostAddress(hostCache));
        } else {
            // Let's keep this for now for backward compatibility... But probably can go away at some point.
            // Bridge v1 didn't have zeroconf...
            QString host = thing->paramValue(bridgeThingHostParamTypeId).toString();
            qCDebug(dcPhilipsHue()) << "Using IP from params:" << host;
            bridge->setHostAddress(QHostAddress(host));
        }
        discoverBridgeDevices(bridge);
        return info->finish(Thing::ThingErrorNoError);
    }

    // At this point we need to have a bridge or we can't continue anyways
    HueBridge *bridge = m_bridges.key(myThings().findById(thing->parentId()));
    if (!bridge) {
        qCWarning(dcPhilipsHue()) << "No hue bridge set up. Cannot continue.";
        info->finish(Thing::ThingErrorHardwareNotAvailable);
        return;
    }

    // Hue color light
    if (thing->thingClassId() == colorLightThingClassId) {
        qCDebug(dcPhilipsHue) << "Setup Hue color light" << thing->params();

        HueLight *hueLight = new HueLight(bridge, this);
        hueLight->setId(thing->paramValue(colorLightThingLightIdParamTypeId).toInt());
        hueLight->setModelId(thing->paramValue(colorLightThingModelIdParamTypeId).toString());
        hueLight->setUuid(thing->paramValue(colorLightThingUuidParamTypeId).toString());
        hueLight->setType(thing->paramValue(colorLightThingTypeParamTypeId).toString());

        connect(hueLight, &HueLight::stateChanged, this, &IntegrationPluginPhilipsHue::lightStateChanged);
        m_lights.insert(hueLight, thing);

        refreshLight(thing);

        return info->finish(Thing::ThingErrorNoError);
    }

    // Hue color temperature light
    if (thing->thingClassId() == colorTemperatureLightThingClassId) {
        qCDebug(dcPhilipsHue) << "Setup Hue color temperature light" << thing->params();

        HueLight *hueLight = new HueLight(bridge, this);
        hueLight->setId(thing->paramValue(colorTemperatureLightThingLightIdParamTypeId).toInt());
        hueLight->setModelId(thing->paramValue(colorTemperatureLightThingModelIdParamTypeId).toString());
        hueLight->setUuid(thing->paramValue(colorTemperatureLightThingUuidParamTypeId).toString());
        hueLight->setType(thing->paramValue(colorTemperatureLightThingTypeParamTypeId).toString());

        connect(hueLight, &HueLight::stateChanged, this, &IntegrationPluginPhilipsHue::lightStateChanged);
        m_lights.insert(hueLight, thing);

        refreshLight(thing);

        return info->finish(Thing::ThingErrorNoError);
    }

    // Hue white light
    if (thing->thingClassId() == dimmableLightThingClassId) {
        qCDebug(dcPhilipsHue) << "Setup Hue white light" << thing->params();

        HueLight *hueLight = new HueLight(bridge, this);

        // Migrate thing parameters after changing param type UUIDs in 0.14.
        QMap<QString, ParamTypeId> migrationMap;
        migrationMap.insert("095a463b-f59e-46b1-989a-a71f9cbe3e30", dimmableLightThingModelIdParamTypeId);
        migrationMap.insert("3f3467ef-4483-4eb9-bcae-84e628322f84", dimmableLightThingTypeParamTypeId);
        migrationMap.insert("1a5129ca-006c-446c-9f2e-79b065de715f", dimmableLightThingUuidParamTypeId);
        migrationMap.insert("491dc012-ccf2-4d3a-9f18-add98f7374af", dimmableLightThingLightIdParamTypeId);

        ParamList migratedParams;
        foreach (const Param &oldParam, thing->params()) {
            QString oldId = oldParam.paramTypeId().toString();
            oldId.remove(QRegExp("[{}]"));
            if (migrationMap.contains(oldId)) {
                ParamTypeId newId = migrationMap.value(oldId);
                QVariant oldValue = oldParam.value();
                qCDebug(dcPhilipsHue()) << "Migrating hue white light param:" << oldId << "->" << newId << ":" << oldValue;
                Param newParam(newId, oldValue);
                migratedParams << newParam;
            } else {
                migratedParams << oldParam;
            }
        }
        thing->setParams(migratedParams);
        // Migration done

        hueLight->setModelId(thing->paramValue(dimmableLightThingModelIdParamTypeId).toString());
        hueLight->setType(thing->paramValue(dimmableLightThingTypeParamTypeId).toString());
        hueLight->setUuid(thing->paramValue(dimmableLightThingUuidParamTypeId).toString());
        hueLight->setId(thing->paramValue(dimmableLightThingLightIdParamTypeId).toInt());

        connect(hueLight, &HueLight::stateChanged, this, &IntegrationPluginPhilipsHue::lightStateChanged);

        m_lights.insert(hueLight, thing);
        refreshLight(thing);

        return info->finish(Thing::ThingErrorNoError);
    }

    // Hue on/off light
    if (thing->thingClassId() == onOffLightThingClassId) {
        qCDebug(dcPhilipsHue) << "Setup Hue white light" << thing->params();

        HueLight *hueLight = new HueLight(bridge, this);

        hueLight->setModelId(thing->paramValue(onOffLightThingModelIdParamTypeId).toString());
        hueLight->setType(thing->paramValue(onOffLightThingTypeParamTypeId).toString());
        hueLight->setUuid(thing->paramValue(onOffLightThingUuidParamTypeId).toString());
        hueLight->setId(thing->paramValue(onOffLightThingLightIdParamTypeId).toInt());

        connect(hueLight, &HueLight::stateChanged, this, &IntegrationPluginPhilipsHue::lightStateChanged);

        m_lights.insert(hueLight, thing);
        refreshLight(thing);

        return info->finish(Thing::ThingErrorNoError);
    }

    // Hue remote
    if (thing->thingClassId() == remoteThingClassId) {
        qCDebug(dcPhilipsHue) << "Setup Hue remote" << thing->params() << thing->thingClassId();

        HueRemote *hueRemote = new HueRemote(bridge, this);

        // Migrate thing parameters after changing param type UUIDs in 0.14.
        QMap<QString, ParamTypeId> migrationMap;
        migrationMap.insert("095a463b-f59e-46b1-989a-a71f9cbe3e30", remoteThingModelIdParamTypeId);
        migrationMap.insert("3f3467ef-4483-4eb9-bcae-84e628322f84", remoteThingTypeParamTypeId);
        migrationMap.insert("1a5129ca-006c-446c-9f2e-79b065de715f", remoteThingUuidParamTypeId);

        ParamList migratedParams;
        foreach (const Param &oldParam, thing->params()) {
            QString oldId = oldParam.paramTypeId().toString();
            oldId.remove(QRegExp("[{}]"));
            if (migrationMap.contains(oldId)) {
                ParamTypeId newId = migrationMap.value(oldId);
                QVariant oldValue = oldParam.value();
                qCDebug(dcPhilipsHue()) << "Migrating hue remote param:" << oldId << "->" << newId << ":" << oldValue;
                Param newParam(newId, oldValue);
                migratedParams << newParam;
            } else {
                migratedParams << oldParam;
            }
        }
        thing->setParams(migratedParams);
        // Migration done


        hueRemote->setId(thing->paramValue(remoteThingSensorIdParamTypeId).toInt());
        hueRemote->setModelId(thing->paramValue(remoteThingModelIdParamTypeId).toString());
        hueRemote->setType(thing->paramValue(remoteThingTypeParamTypeId).toString());
        hueRemote->setUuid(thing->paramValue(remoteThingUuidParamTypeId).toString());

        connect(hueRemote, &HueRemote::stateChanged, this, &IntegrationPluginPhilipsHue::remoteStateChanged);
        connect(hueRemote, &HueRemote::buttonPressed, this, &IntegrationPluginPhilipsHue::onRemoteButtonEvent);

        m_remotes.insert(hueRemote, thing);
        return info->finish(Thing::ThingErrorNoError);
    }

    // Hue Dimmer Switch V2
    if (thing->thingClassId() == dimmerSwitch2ThingClassId) {
        qCDebug(dcPhilipsHue) << "Setup Hue Dimmer Switch V2" << thing->params() << thing->thingClassId();

        HueRemote *hueDimmerSwitch2 = new HueRemote(bridge, this);

        hueDimmerSwitch2->setId(thing->paramValue(dimmerSwitch2ThingSensorIdParamTypeId).toInt());
        hueDimmerSwitch2->setModelId(thing->paramValue(dimmerSwitch2ThingModelIdParamTypeId).toString());
        hueDimmerSwitch2->setType(thing->paramValue(dimmerSwitch2ThingTypeParamTypeId).toString());
        hueDimmerSwitch2->setUuid(thing->paramValue(dimmerSwitch2ThingUuidParamTypeId).toString());

        connect(hueDimmerSwitch2, &HueRemote::stateChanged, this, &IntegrationPluginPhilipsHue::remoteStateChanged);
        connect(hueDimmerSwitch2, &HueRemote::buttonPressed, this, &IntegrationPluginPhilipsHue::onRemoteButtonEvent);

        m_remotes.insert(hueDimmerSwitch2, thing);
        return info->finish(Thing::ThingErrorNoError);
    }

    // Hue Tap Dial
    if (thing->thingClassId() == tapDialThingClassId) {
        qCDebug(dcPhilipsHue) << "Setup Hue Tap Dial" << thing->params() << thing->thingClassId();

        HueTapDial *hueTapDial = new HueTapDial(bridge, this);

        hueTapDial->setModelId(thing->paramValue(tapDialThingModelIdParamTypeId).toString());
        hueTapDial->setUuid(thing->paramValue(tapDialThingUuidParamTypeId).toString());
        hueTapDial->setRotaryId(thing->paramValue(tapDialThingIdRotaryParamTypeId).toInt());
        hueTapDial->setRotaryUuid(thing->paramValue(tapDialThingUuidRotaryParamTypeId).toString());
        hueTapDial->setSwitchId(thing->paramValue(tapDialThingIdSwitchParamTypeId).toInt());
        hueTapDial->setSwitchUuid(thing->paramValue(tapDialThingUuidSwitchParamTypeId).toString());

        connect(hueTapDial, &HueTapDial::reachableChanged, this, &IntegrationPluginPhilipsHue::onTapDialReachableChanged);
        connect(hueTapDial, &HueTapDial::batteryLevelChanged, this, &IntegrationPluginPhilipsHue::onTapDialBatteryLevelChanged);
        connect(hueTapDial, &HueTapDial::buttonPressed, this, &IntegrationPluginPhilipsHue::onTapDialButtonEvent);
        connect(hueTapDial, &HueTapDial::rotated, this, &IntegrationPluginPhilipsHue::onTapDialRotaryEvent);

        m_tapDials.insert(hueTapDial, thing);
        return info->finish(Thing::ThingErrorNoError);
    }

    // Hue tap
    if (thing->thingClassId() == tapThingClassId) {
        HueRemote *hueTap = new HueRemote(bridge, this);
        hueTap->setName(thing->name());
        hueTap->setId(thing->paramValue(tapThingSensorIdParamTypeId).toInt());
        hueTap->setModelId(thing->paramValue(tapThingModelIdParamTypeId).toString());
        hueTap->setUuid(thing->paramValue(tapThingUuidParamTypeId).toString());

        connect(hueTap, &HueRemote::stateChanged, this, &IntegrationPluginPhilipsHue::remoteStateChanged);
        connect(hueTap, &HueRemote::buttonPressed, this, &IntegrationPluginPhilipsHue::onRemoteButtonEvent);

        m_remotes.insert(hueTap, thing);
        return info->finish(Thing::ThingErrorNoError);
    }

    // Friends of Hue switch
    if (thing->thingClassId() == fohThingClassId) {
        HueRemote *hueFoh = new HueRemote(bridge, this);
        hueFoh->setName(thing->name());
        hueFoh->setId(thing->paramValue(fohThingSensorIdParamTypeId).toInt());
        hueFoh->setModelId(thing->paramValue(fohThingModelIdParamTypeId).toString());
        hueFoh->setUuid(thing->paramValue(fohThingUuidParamTypeId).toString());

        connect(hueFoh, &HueRemote::stateChanged, this, &IntegrationPluginPhilipsHue::remoteStateChanged);
        connect(hueFoh, &HueRemote::buttonPressed, this, &IntegrationPluginPhilipsHue::onRemoteButtonEvent);

        m_remotes.insert(hueFoh, thing);
        return info->finish(Thing::ThingErrorNoError);
    }

    // Hue smart button
    if (thing->thingClassId() == smartButtonThingClassId) {
        HueRemote *smartButton = new HueRemote(bridge, this);
        smartButton->setName(thing->name());
        smartButton->setId(thing->paramValue(smartButtonThingSensorIdParamTypeId).toInt());
        smartButton->setModelId(thing->paramValue(smartButtonThingModelIdParamTypeId).toString());
        smartButton->setUuid(thing->paramValue(smartButtonThingUuidParamTypeId).toString());

        connect(smartButton, &HueRemote::stateChanged, this, &IntegrationPluginPhilipsHue::remoteStateChanged);
        connect(smartButton, &HueRemote::buttonPressed, this, &IntegrationPluginPhilipsHue::onRemoteButtonEvent);

        m_remotes.insert(smartButton, thing);
        return info->finish(Thing::ThingErrorNoError);
    }

    // Hue Wall Switch module
    if (thing->thingClassId() == wallSwitchThingClassId) {
        HueRemote *wallSwitch = new HueRemote(bridge, this);
        wallSwitch->setName(thing->name());
        wallSwitch->setId(thing->paramValue(wallSwitchThingSensorIdParamTypeId).toInt());
        wallSwitch->setModelId(thing->paramValue(wallSwitchThingModelIdParamTypeId).toString());
        wallSwitch->setUuid(thing->paramValue(wallSwitchThingUuidParamTypeId).toString());

        connect(wallSwitch, &HueRemote::stateChanged, this, &IntegrationPluginPhilipsHue::remoteStateChanged);
        connect(wallSwitch, &HueRemote::buttonPressed, this, &IntegrationPluginPhilipsHue::onRemoteButtonEvent);

        m_remotes.insert(wallSwitch, thing);
        return info->finish(Thing::ThingErrorNoError);
    }

    // Hue Motion sensor
    if (thing->thingClassId() == motionSensorThingClassId) {
        qCDebug(dcPhilipsHue) << "Setup Hue motion sensor" << thing->params();

        HueIndoorSensor *motionSensor = new HueIndoorSensor(bridge, this);
        motionSensor->setTimeout(thing->setting(motionSensorSettingsTimeoutParamTypeId).toUInt());
        motionSensor->setUuid(thing->paramValue(motionSensorThingUuidParamTypeId).toString());
        motionSensor->setModelId(thing->paramValue(motionSensorThingModelIdParamTypeId).toString());
        motionSensor->setTemperatureSensorId(thing->paramValue(motionSensorThingSensorIdTemperatureParamTypeId).toInt());
        motionSensor->setTemperatureSensorUuid(thing->paramValue(motionSensorThingSensorUuidTemperatureParamTypeId).toString());
        motionSensor->setPresenceSensorId(thing->paramValue(motionSensorThingSensorIdPresenceParamTypeId).toInt());
        motionSensor->setPresenceSensorUuid(thing->paramValue(motionSensorThingSensorUuidPresenceParamTypeId).toString());
        motionSensor->setLightSensorId(thing->paramValue(motionSensorThingSensorIdLightParamTypeId).toInt());
        motionSensor->setLightSensorUuid(thing->paramValue(motionSensorThingSensorUuidLightParamTypeId).toString());

        connect(motionSensor, &HueMotionSensor::reachableChanged, this, &IntegrationPluginPhilipsHue::onMotionSensorReachableChanged);
        connect(motionSensor, &HueMotionSensor::batteryLevelChanged, this, &IntegrationPluginPhilipsHue::onMotionSensorBatteryLevelChanged);
        connect(motionSensor, &HueMotionSensor::temperatureChanged, this, &IntegrationPluginPhilipsHue::onMotionSensorTemperatureChanged);
        connect(motionSensor, &HueMotionSensor::presenceChanged, this, &IntegrationPluginPhilipsHue::onMotionSensorPresenceChanged);
        connect(motionSensor, &HueMotionSensor::lightIntensityChanged, this, &IntegrationPluginPhilipsHue::onMotionSensorLightIntensityChanged);

        connect(thing, &Thing::settingChanged, motionSensor, [motionSensor](const ParamTypeId &paramTypeId, const QVariant &value){
            if (paramTypeId == motionSensorSettingsTimeoutParamTypeId) {
                motionSensor->setTimeout(value.toUInt());
            }
        });

        m_motionSensors.insert(motionSensor, thing);

        return info->finish(Thing::ThingErrorNoError);
    }

    // Hue Outdoor sensor
    if (thing->thingClassId() == outdoorSensorThingClassId) {
        qCDebug(dcPhilipsHue) << "Setup Hue Outdoor sensor" << thing->params();

        HueMotionSensor *outdoorSensor = new HueOutdoorSensor(bridge, this);
        outdoorSensor->setTimeout(thing->setting(outdoorSensorSettingsTimeoutParamTypeId).toUInt());
        outdoorSensor->setUuid(thing->paramValue(outdoorSensorThingUuidParamTypeId).toString());
        outdoorSensor->setModelId(thing->paramValue(outdoorSensorThingModelIdParamTypeId).toString());
        outdoorSensor->setTemperatureSensorId(thing->paramValue(outdoorSensorThingSensorIdTemperatureParamTypeId).toInt());
        outdoorSensor->setTemperatureSensorUuid(thing->paramValue(outdoorSensorThingSensorUuidTemperatureParamTypeId).toString());
        outdoorSensor->setPresenceSensorId(thing->paramValue(outdoorSensorThingSensorIdPresenceParamTypeId).toInt());
        outdoorSensor->setPresenceSensorUuid(thing->paramValue(outdoorSensorThingSensorUuidPresenceParamTypeId).toString());
        outdoorSensor->setLightSensorId(thing->paramValue(outdoorSensorThingSensorIdLightParamTypeId).toInt());
        outdoorSensor->setLightSensorUuid(thing->paramValue(outdoorSensorThingSensorUuidLightParamTypeId).toString());

        connect(outdoorSensor, &HueMotionSensor::reachableChanged, this, &IntegrationPluginPhilipsHue::onMotionSensorReachableChanged);
        connect(outdoorSensor, &HueMotionSensor::batteryLevelChanged, this, &IntegrationPluginPhilipsHue::onMotionSensorBatteryLevelChanged);
        connect(outdoorSensor, &HueMotionSensor::temperatureChanged, this, &IntegrationPluginPhilipsHue::onMotionSensorTemperatureChanged);
        connect(outdoorSensor, &HueMotionSensor::presenceChanged, this, &IntegrationPluginPhilipsHue::onMotionSensorPresenceChanged);
        connect(outdoorSensor, &HueMotionSensor::lightIntensityChanged, this, &IntegrationPluginPhilipsHue::onMotionSensorLightIntensityChanged);

        connect(thing, &Thing::settingChanged, outdoorSensor, [outdoorSensor](const ParamTypeId &paramTypeId, const QVariant &value){
            if (paramTypeId == outdoorSensorSettingsTimeoutParamTypeId) {
                outdoorSensor->setTimeout(value.toUInt());
            }
        });

        m_motionSensors.insert(outdoorSensor, thing);

        return info->finish(Thing::ThingErrorNoError);
    }

    // Hue smart plug
    if (thing->thingClassId() == smartPlugThingClassId) {
        qCDebug(dcPhilipsHue) << "Setting up Hue Smart plug" << thing->params();
        HueLight *smartPlug = new HueLight(bridge, this);
        smartPlug->setUuid(thing->paramValue(smartPlugThingUuidParamTypeId).toString());
        smartPlug->setId(thing->paramValue(smartPlugThingLightIdParamTypeId).toInt());
        smartPlug->setModelId(thing->paramValue(smartPlugThingModelIdParamTypeId).toString());
        smartPlug->setType(thing->paramValue(smartPlugThingTypeParamTypeId).toString());

        connect(smartPlug, &HueLight::reachableChanged, thing, [thing](bool reachable){
            thing->setStateValue(smartPlugConnectedStateTypeId, reachable);
        });
        connect(smartPlug, &HueLight::stateChanged, this, &IntegrationPluginPhilipsHue::lightStateChanged);
        m_lights.insert(smartPlug, thing);
        info->finish(Thing::ThingErrorNoError);
        return;
    }

    qCWarning(dcPhilipsHue()) << "Unhandled setupDevice call" << thing->thingClassId();
}

void IntegrationPluginPhilipsHue::thingRemoved(Thing *thing)
{
    abortRequests(m_lightRefreshRequests, thing);
    abortRequests(m_setNameRequests, thing);
    abortRequests(m_bridgeRefreshRequests, thing);
    abortRequests(m_lightsRefreshRequests, thing);
    abortRequests(m_sensorsRefreshRequests, thing);
    abortRequests(m_bridgeLightsDiscoveryRequests, thing);
    abortRequests(m_bridgeSensorsDiscoveryRequests, thing);
    abortRequests(m_bridgeSearchDevicesRequests, thing);

    if (thing->thingClassId() == bridgeThingClassId) {
        qCDebug(dcPhilipsHue()) << "Bridge removed" << thing->name();
        HueBridge *bridge = m_bridges.key(thing);
        m_bridges.remove(bridge);
        bridge->deleteLater();
    }

    if (thing->thingClassId() == colorLightThingClassId
            || thing->thingClassId() == colorTemperatureLightThingClassId
            || thing->thingClassId() == dimmableLightThingClassId
            || thing->thingClassId() == onOffLightThingClassId
            || thing->thingClassId() == smartPlugThingClassId) {
        HueLight *light = m_lights.key(thing);
        m_lights.remove(light);
        light->deleteLater();
    }

    if (thing->thingClassId() == remoteThingClassId || thing->thingClassId() == dimmerSwitch2ThingClassId || thing->thingClassId() == tapThingClassId || thing->thingClassId() == fohThingClassId || thing->thingClassId() == smartButtonThingClassId || thing->thingClassId() == wallSwitchThingClassId) {
        HueRemote *remote = m_remotes.key(thing);
        m_remotes.remove(remote);
        remote->deleteLater();
    }

    if (thing->thingClassId() == tapDialThingClassId) {
        HueTapDial *tapDial = m_tapDials.key(thing);
        m_tapDials.remove(tapDial);
        tapDial->deleteLater();
    }

    if (thing->thingClassId() == outdoorSensorThingClassId || thing->thingClassId() == motionSensorThingClassId) {
        HueMotionSensor *motionSensor = m_motionSensors.key(thing);
        m_motionSensors.remove(motionSensor);
        motionSensor->deleteLater();
    }
}

void IntegrationPluginPhilipsHue::confirmPairing(ThingPairingInfo *info, const QString &username, const QString &secret)
{
    Q_UNUSED(username)
    Q_UNUSED(secret)

    QVariantMap deviceTypeParam;
    deviceTypeParam.insert("devicetype", "nymea");

    QJsonDocument jsonDoc = QJsonDocument::fromVariant(deviceTypeParam);

    QString host = info->params().paramValue(bridgeThingHostParamTypeId).toString();
    QNetworkRequest request(QUrl("http://" + host + "/api"));
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    QNetworkReply *reply = hardwareManager()->networkManager()->post(request, jsonDoc.toJson());
    connect(reply, &QNetworkReply::finished, reply, &QNetworkReply::deleteLater);

    connect(reply, &QNetworkReply::finished, info, [this, info, reply](){
        if (reply->error() != QNetworkReply::NoError) {
            info->finish(Thing::ThingErrorHardwareFailure, QT_TR_NOOP("Connecting to the Hue Bridge failed. Please make sure that your Hue Bridge is working and connected to the same network."));
            return;
        }
        QByteArray data = reply->readAll();
        QJsonParseError error;
        QJsonDocument jsonDoc = QJsonDocument::fromJson(data, &error);

        // check JSON error
        if (error.error != QJsonParseError::NoError) {
            qCWarning(dcPhilipsHue) << "Hue Bridge json error in response" << error.errorString();
            info->finish(Thing::ThingErrorHardwareFailure, QT_TR_NOOP("The pairing process failed. Please make sure that your Hue Bridge is working."));
            return;
        }
        if (jsonDoc.toVariant().toList().isEmpty()) {
            qCWarning(dcPhilipsHue) << "Hue Bridge empty json!";
            info->finish(Thing::ThingErrorHardwareFailure, QT_TR_NOOP("The pairing process failed. Please make sure that your Hue Bridge is working."));
            return;
        }

        QVariantMap pairingReply = jsonDoc.toVariant().toList().first().toMap();

        // check response error
        if (pairingReply.contains("error")) {
            qCWarning(dcPhilipsHue) << "Failed to pair Hue Bridge:" << pairingReply;
            if (pairingReply.value("error").toMap().value("type").toInt() == 101) {
                info->finish(Thing::ThingErrorHardwareFailure, QT_TR_NOOP("The pairing process failed. The link button has not been pressed. Please follow the on-screen instructions again."));
            } else {
                info->finish(Thing::ThingErrorHardwareFailure, QT_TR_NOOP("Please make sure that your Hue bridge is working and follow the on-screen instructions again."));
            }
            return;
        }

        QString apiKey = pairingReply.value("success").toMap().value("username").toString();

        if (apiKey.isEmpty()) {
            qCWarning(dcPhilipsHue) << "Failed to pair Hue Bridge: did not get any key from the bridge";
            return info->finish(Thing::ThingErrorAuthenticationFailure, QT_TR_NOOP("The hue bridge has rejected the connection request."));
        }

        qCDebug(dcPhilipsHue) << "Got api key from bridge:" << apiKey;

        // All good. Store the API key
        pluginStorage()->beginGroup(info->thingId().toString());
        pluginStorage()->setValue("apiKey", apiKey);
        pluginStorage()->endGroup();

        info->finish(Thing::ThingErrorNoError);
    });
}

void IntegrationPluginPhilipsHue::networkManagerReplyReady()
{
    QNetworkReply *reply = static_cast<QNetworkReply *>(sender());
    reply->deleteLater();

    int status = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();

    //    qCDebug(dcPhilipsHue()) << "Hue reply:" << status << reply->error() << reply->errorString();

    // create user finished
    if (m_bridgeLightsDiscoveryRequests.contains(reply)) {
        Thing *thing = m_bridgeLightsDiscoveryRequests.take(reply);

        // check HTTP status code
        if (status != 200 || reply->error() != QNetworkReply::NoError) {
            qCWarning(dcPhilipsHue) << "Bridge light discovery error:" << status << reply->errorString();
            bridgeReachableChanged(thing, false);
            return;
        }
        processBridgeLightDiscoveryResponse(thing, reply->readAll());

    } else if (m_bridgeSensorsDiscoveryRequests.contains(reply)) {
        Thing *thing = m_bridgeSensorsDiscoveryRequests.take(reply);

        // check HTTP status code
        if (status != 200 || reply->error() != QNetworkReply::NoError) {
            qCWarning(dcPhilipsHue) << "Bridge sensor discovery error:" << status << reply->errorString();
            bridgeReachableChanged(thing, false);
            return;
        }
        processBridgeSensorDiscoveryResponse(thing, reply->readAll());

    } else if (m_bridgeSearchDevicesRequests.contains(reply)) {
        Thing *thing = m_bridgeSearchDevicesRequests.take(reply);

        // check HTTP status code
        if (status != 200 || reply->error() != QNetworkReply::NoError) {
            qCWarning(dcPhilipsHue) << "Bridge search new devices error:" << status << reply->errorString();
            bridgeReachableChanged(thing, false);
            return;
        }
        discoverBridgeDevices(m_bridges.key(thing));

    } else if (m_bridgeRefreshRequests.contains(reply)) {
        Thing *thing = m_bridgeRefreshRequests.take(reply);

        // check HTTP status code
        if (status != 200 || reply->error() != QNetworkReply::NoError) {
            if (thing->stateValue(bridgeConnectedStateTypeId).toBool()) {
                qCWarning(dcPhilipsHue) << "Refresh Hue Bridge request error:" << status << reply->errorString();
                bridgeReachableChanged(thing, false);
            }
            return;
        }
        processBridgeRefreshResponse(thing, reply->readAll());

    } else if (m_lightRefreshRequests.contains(reply)) {
        Thing *thing = m_lightRefreshRequests.take(reply);

        // check HTTP status code
        if (status != 200 || reply->error() != QNetworkReply::NoError) {
            qCWarning(dcPhilipsHue) << "Refresh Hue Light request error:" << status << reply->errorString();
            bridgeReachableChanged(thing, false);
            return;
        }
        processLightRefreshResponse(thing, reply->readAll());

    } else if (m_lightsRefreshRequests.contains(reply)) {
        Thing *thing = m_lightsRefreshRequests.take(reply);

        // check HTTP status code
        if (status != 200 || reply->error() != QNetworkReply::NoError) {
            if (thing->stateValue(bridgeConnectedStateTypeId).toBool()) {
                qCWarning(dcPhilipsHue) << "Refresh Hue lights request error:" << status << reply->errorString();
                bridgeReachableChanged(thing, false);
            }
            return;
        }
        processLightsRefreshResponse(thing, reply->readAll());

    } else if (m_sensorsRefreshRequests.contains(reply)) {
        Thing *thing = m_sensorsRefreshRequests.take(reply);

        // check HTTP status code
        if (status != 200 || reply->error() != QNetworkReply::NoError) {
            if (thing->stateValue(bridgeConnectedStateTypeId).toBool()) {
                qCWarning(dcPhilipsHue) << "Refresh Hue sensors request error:" << status << reply->errorString();
                bridgeReachableChanged(thing, false);
            }
            return;
        }
        processSensorsRefreshResponse(thing, reply->readAll());

    } else if (m_setNameRequests.contains(reply)) {
        Thing *thing = m_setNameRequests.take(reply);

        // check HTTP status code
        if (status != 200 || reply->error() != QNetworkReply::NoError) {
            qCWarning(dcPhilipsHue) << "Set name of Hue Light request error:" << status << reply->errorString();
            bridgeReachableChanged(thing, false);
            return;
        }
        processSetNameResponse(thing, reply->readAll());
    } else {
        qCWarning(dcPhilipsHue()) << "Unhandled bridge reply" << reply->error() << reply->readAll();
    }
}

void IntegrationPluginPhilipsHue::onDeviceNameChanged()
{
    Thing *thing = static_cast<Thing*>(sender());
    if (m_lights.values().contains(thing)) {
        setLightName(thing);
    }

    if (m_remotes.values().contains(thing)) {
        setRemoteName(thing);
    }
}

QString IntegrationPluginPhilipsHue::normalizeBridgeId(const QString &bridgeId)
{
    // For some reason the hue apis return slightly different bridge ids:
    // * UPnP api returns serial numbers withouot a "fffe" in the middle
    // * N-UPnP and ZeroConf do have such a "fffe" in them.
    // Let's normalize it to something that's save to compare.

    QString ret = bridgeId.toLower();
    if (bridgeId.indexOf("fffe") == 6) {
        ret.remove(6, 4);
    }
    return ret;
}

void IntegrationPluginPhilipsHue::executeAction(ThingActionInfo *info)
{
    Thing *thing = info->thing();
    Action action = info->action();

    qCDebug(dcPhilipsHue) << "Execute action" << action.actionTypeId() << action.params();

    QNetworkReply *reply = nullptr;

    // lights
    if (thing->thingClassId() == colorLightThingClassId ||
            thing->thingClassId() == colorTemperatureLightThingClassId ||
            thing->thingClassId() == dimmableLightThingClassId ||
            thing->thingClassId() == onOffLightThingClassId ||
            thing->thingClassId() == smartPlugThingClassId) {

        HueLight *light = m_lights.key(thing);

        if (!light->reachable()) {
            qCWarning(dcPhilipsHue) << "Light" << light->name() << "not reachable";
            return info->finish(Thing::ThingErrorHardwareNotAvailable);
        }

        if (action.actionTypeId() == colorLightPowerActionTypeId) {
            QPair<QNetworkRequest, QByteArray> request = light->createSetPowerRequest(action.param(colorLightPowerActionPowerParamTypeId).value().toBool());
            reply = hardwareManager()->networkManager()->put(request.first, request.second);
        } else if (action.actionTypeId() == colorLightColorActionTypeId) {
            QPair<QNetworkRequest, QByteArray> request = light->createSetColorRequest(action.param(colorLightColorActionColorParamTypeId).value().value<QColor>());
            reply = hardwareManager()->networkManager()->put(request.first, request.second);
        } else if (action.actionTypeId() == colorLightBrightnessActionTypeId) {
            QPair<QNetworkRequest, QByteArray> request = light->createSetBrightnessRequest(percentageToBrightness(action.param(colorLightBrightnessActionBrightnessParamTypeId).value().toInt()));
            reply = hardwareManager()->networkManager()->put(request.first, request.second);
        } else if (action.actionTypeId() == colorLightEffectActionTypeId) {
            QPair<QNetworkRequest, QByteArray> request = light->createSetEffectRequest(action.param(colorLightEffectActionEffectParamTypeId).value().toString());
            reply = hardwareManager()->networkManager()->put(request.first, request.second);
        } else if (action.actionTypeId() == colorLightAlertActionTypeId) {
            QPair<QNetworkRequest, QByteArray> request = light->createFlashRequest(action.param(colorLightAlertActionAlertParamTypeId).value().toString());
            reply = hardwareManager()->networkManager()->put(request.first, request.second);
        } else if (action.actionTypeId() == colorLightColorTemperatureActionTypeId) {
            QPair<QNetworkRequest, QByteArray> request = light->createSetTemperatureRequest(action.param(colorLightColorTemperatureActionColorTemperatureParamTypeId).value().toInt());
            reply = hardwareManager()->networkManager()->put(request.first, request.second);
        }
        // Color temperature light
        else if (action.actionTypeId() == colorTemperatureLightPowerActionTypeId) {
            QPair<QNetworkRequest, QByteArray> request = light->createSetPowerRequest(action.param(colorTemperatureLightPowerActionPowerParamTypeId).value().toBool());
            reply = hardwareManager()->networkManager()->put(request.first, request.second);
        } else if (action.actionTypeId() == colorTemperatureLightBrightnessActionTypeId) {
            QPair<QNetworkRequest, QByteArray> request = light->createSetBrightnessRequest(percentageToBrightness(action.param(colorTemperatureLightBrightnessActionBrightnessParamTypeId).value().toInt()));
            reply = hardwareManager()->networkManager()->put(request.first, request.second);
        } else if (action.actionTypeId() == colorTemperatureLightAlertActionTypeId) {
            QPair<QNetworkRequest, QByteArray> request = light->createFlashRequest(action.param(colorTemperatureLightAlertActionAlertParamTypeId).value().toString());
            reply = hardwareManager()->networkManager()->put(request.first, request.second);
        } else if (action.actionTypeId() == colorTemperatureLightColorTemperatureActionTypeId) {
            QPair<QNetworkRequest, QByteArray> request = light->createSetTemperatureRequest(action.param(colorTemperatureLightColorTemperatureActionColorTemperatureParamTypeId).value().toInt());
            reply = hardwareManager()->networkManager()->put(request.first, request.second);
        }
        // Dimmable light
        else if (action.actionTypeId() == dimmableLightPowerActionTypeId) {
            QPair<QNetworkRequest, QByteArray> request = light->createSetPowerRequest(action.param(dimmableLightPowerActionPowerParamTypeId).value().toBool());
            reply = hardwareManager()->networkManager()->put(request.first, request.second);
        } else if (action.actionTypeId() == dimmableLightBrightnessActionTypeId) {
            QPair<QNetworkRequest, QByteArray> request = light->createSetBrightnessRequest(percentageToBrightness(action.param(dimmableLightBrightnessActionBrightnessParamTypeId).value().toInt()));
            reply = hardwareManager()->networkManager()->put(request.first, request.second);
        } else if (action.actionTypeId() == dimmableLightAlertActionTypeId) {
            QPair<QNetworkRequest, QByteArray> request = light->createFlashRequest(action.param(dimmableLightAlertActionAlertParamTypeId).value().toString());
            reply = hardwareManager()->networkManager()->put(request.first, request.second);
        }
        // On/Off light
        else if (action.actionTypeId() == onOffLightPowerActionTypeId) {
            QPair<QNetworkRequest, QByteArray> request = light->createSetPowerRequest(action.param(onOffLightPowerActionPowerParamTypeId).value().toBool());
            reply = hardwareManager()->networkManager()->put(request.first, request.second);
        }

        // Hue smart plug
        else if (action.actionTypeId() == smartPlugPowerActionTypeId) {
            QPair<QNetworkRequest, QByteArray> request = light->createSetPowerRequest(action.param(smartPlugPowerActionPowerParamTypeId).value().toBool());
            reply = hardwareManager()->networkManager()->put(request.first, request.second);
        }
    }

    // Hue bridge
    if (thing->thingClassId() == bridgeThingClassId) {
        HueBridge *bridge = m_bridges.key(thing);
        if (!thing->stateValue(bridgeConnectedStateTypeId).toBool()) {
            qCWarning(dcPhilipsHue) << "Bridge" << bridge->hostAddress().toString() << "not reachable";
            return info->finish(Thing::ThingErrorHardwareNotAvailable);
        }

        if (action.actionTypeId() == bridgeSearchNewDevicesActionTypeId) {
            searchNewDevices(bridge, action.param(bridgeSearchNewDevicesActionSerialParamTypeId).value().toString());
            return info->finish(Thing::ThingErrorNoError);
        } else if (action.actionTypeId() == bridgeCheckForUpdatesActionTypeId) {
            QPair<QNetworkRequest, QByteArray> request = bridge->createCheckUpdatesRequest();
            reply = hardwareManager()->networkManager()->put(request.first, request.second);
        } else if (action.actionTypeId() == bridgePerformUpdateActionTypeId) {
            QPair<QNetworkRequest, QByteArray> request = bridge->createUpgradeRequest();
            reply = hardwareManager()->networkManager()->put(request.first, request.second);
        }
    }

    if (!reply) {
        qCWarning(dcPhilipsHue()) << "Unhandled Hue action! Plugin bug!";
        Q_ASSERT_X(false, "HuePlugin", "Unhandled action");
        info->finish(Thing::ThingErrorUnsupportedFeature);
        return;
    }

    // Always clean up the reply when it finishes
    connect(reply, &QNetworkReply::finished, reply, &QNetworkReply::deleteLater);

    // Handle response if info is still around
    connect(reply, &QNetworkReply::finished, info, [this, info, reply](){
        if (reply->error() != QNetworkReply::NoError) {
            info->finish(Thing::ThingErrorHardwareFailure, QT_TR_NOOP("Error sending command to hue bridge."));
            return;
        }

        QByteArray data = reply->readAll();
        QJsonParseError error;
        QJsonDocument jsonDoc = QJsonDocument::fromJson(data, &error);

        if (error.error != QJsonParseError::NoError) {
            qCWarning(dcPhilipsHue) << "Hue Bridge json error in response" << error.errorString();
            info->finish(Thing::ThingErrorHardwareFailure, QT_TR_NOOP("Received unexpected data from hue bridge."));
            return;
        }

        if (data.contains("error")) {
            if (!jsonDoc.toVariant().toList().isEmpty()) {
                qCWarning(dcPhilipsHue) << "Failed to execute Hue action:" << jsonDoc.toJson(); //jsonDoc.toVariant().toList().first().toMap().value("error").toMap().value("description").toString();
            } else {
                qCWarning(dcPhilipsHue) << "Failed to execute Hue action: Invalid error message format";
            }
            info->finish(Thing::ThingErrorHardwareFailure, QT_TR_NOOP("An unexpected error happened when sending the command to the hue bridge."));
            return;
        }

        if (info->thing()->thingClassId() != bridgeThingClassId) {
            m_lights.key(info->thing())->processActionResponse(jsonDoc.toVariant().toList());
        }

        info->finish(Thing::ThingErrorNoError);
    });
}

void IntegrationPluginPhilipsHue::browseThing(BrowseResult *result)
{
    Thing *bridgeThing = result->thing();
    HueBridge* bridge = m_bridges.key(bridgeThing);

    QNetworkRequest request(QUrl("http://" + bridge->hostAddress().toString() + "/api/" + bridge->apiKey() + "/scenes"));
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    QNetworkReply* reply = hardwareManager()->networkManager()->get(request);
    connect(reply, &QNetworkReply::finished, reply, &QNetworkReply::deleteLater);
    connect(reply, &QNetworkReply::finished, result, [result, reply]() {
        if (reply->error() != QNetworkReply::NoError) {
            qCWarning(dcPhilipsHue()) << "Error fetching scenes";
            result->finish(Thing::ThingErrorHardwareNotAvailable);
            return;
        }
        QByteArray data = reply->readAll();
        QJsonParseError error;
        QJsonDocument jsonDoc = QJsonDocument::fromJson(data, &error);
        if (error.error != QJsonParseError::NoError) {
            qCWarning(dcPhilipsHue()) << "Error parsing json from hue bridge" << data;
            result->finish(Thing::ThingErrorHardwareFailure);
            return;
        }

        qCDebug(dcPhilipsHue()) << "Scenes reply:" << qUtf8Printable(jsonDoc.toJson());
        QVariantMap scenesMap = jsonDoc.toVariant().toMap();
        foreach (const QString &sceneId, scenesMap.keys()) {
            QVariantMap scene = scenesMap.value(sceneId).toMap();
            BrowserItem item(sceneId, scene.value("name").toString(), false, true);
            item.setIcon(BrowserItem::BrowserIconFavorites);
            result->addItem(item);
        }
        result->finish(Thing::ThingErrorNoError);
    });
}

void IntegrationPluginPhilipsHue::browserItem(BrowserItemResult *result)
{
    Thing *bridgeThing = result->thing();
    HueBridge* bridge = m_bridges.key(bridgeThing);

    QNetworkRequest request(QUrl("http://" + bridge->hostAddress().toString() + "/api/" + bridge->apiKey() + "/scenes"));
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    QNetworkReply* reply = hardwareManager()->networkManager()->get(request);
    connect(reply, &QNetworkReply::finished, reply, &QNetworkReply::deleteLater);
    connect(reply, &QNetworkReply::finished, result, [result, reply]() {
        if (reply->error() != QNetworkReply::NoError) {
            qCWarning(dcPhilipsHue()) << "Error fetching scenes";
            result->finish(Thing::ThingErrorHardwareNotAvailable);
            return;
        }
        QByteArray data = reply->readAll();
        QJsonParseError error;
        QJsonDocument jsonDoc = QJsonDocument::fromJson(data, &error);
        if (error.error != QJsonParseError::NoError) {
            qCWarning(dcPhilipsHue()) << "Error parsing json from hue bridge" << data;
            result->finish(Thing::ThingErrorHardwareFailure);
            return;
        }

        qCDebug(dcPhilipsHue()) << "Scenes reply:" << qUtf8Printable(jsonDoc.toJson());
        QVariantMap scenesMap = jsonDoc.toVariant().toMap();
        QVariantMap scene = scenesMap.value(result->itemId()).toMap();
        BrowserItem item(result->itemId(), scene.value("name").toString(), false, true);
        item.setIcon(BrowserItem::BrowserIconFavorites);
        result->finish(item);
    });
}

void IntegrationPluginPhilipsHue::executeBrowserItem(BrowserActionInfo *info)
{
    Thing *bridgeThing = info->thing()->parentId().isNull() ? info->thing() : myThings().findById(info->thing()->parentId());
    HueBridge* bridge = m_bridges.key(bridgeThing);

    QUrl url = QUrl(QString("http://%1/api/%2/groups/%3/action")
                    .arg(bridge->hostAddress().toString())
                    .arg(bridge->apiKey())
                    .arg("0")
                    );
    QNetworkRequest request(url);

    QVariantMap payload;
    payload.insert("scene", info->browserAction().itemId());

    qCDebug(dcPhilipsHue()) << "Recalling scene" << url.toString();

    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    QNetworkReply* reply = hardwareManager()->networkManager()->put(request, QJsonDocument::fromVariant(payload).toJson());
    connect(reply, &QNetworkReply::finished, reply, &QNetworkReply::deleteLater);
    connect(reply, &QNetworkReply::finished, info, [info, reply]() {
        if (reply->error() != QNetworkReply::NoError) {
            qCWarning(dcPhilipsHue()) << "Error fetching scenes";
            info->finish(Thing::ThingErrorHardwareNotAvailable);
            return;
        }
        QByteArray data = reply->readAll();
        QJsonParseError error;
        QJsonDocument jsonDoc = QJsonDocument::fromJson(data, &error);
        if (error.error != QJsonParseError::NoError) {
            qCWarning(dcPhilipsHue()) << "Error parsing json from hue bridge" << data;
            info->finish(Thing::ThingErrorHardwareFailure);
            return;
        }

        qCDebug(dcPhilipsHue()) << "Set scene reply:" << qUtf8Printable(jsonDoc.toJson());
        info->finish(Thing::ThingErrorNoError);
    });
}

void IntegrationPluginPhilipsHue::lightStateChanged()
{
    HueLight *light = static_cast<HueLight *>(sender());

    Thing *thing = m_lights.value(light);
    if (!thing) {
        qCWarning(dcPhilipsHue) << "Could not find thing for light" << light->name();
        return;
    }

    if (thing->thingClassId() == colorLightThingClassId) {
        thing->setStateValue(colorLightConnectedStateTypeId, light->reachable());
        thing->setStateValue(colorLightColorStateTypeId, QVariant::fromValue(light->color()));
        thing->setStateValue(colorLightPowerStateTypeId, light->power());
        thing->setStateValue(colorLightBrightnessStateTypeId, brightnessToPercentage(light->brightness()));
        thing->setStateValue(colorLightColorTemperatureStateTypeId, light->ct());
        thing->setStateValue(colorLightEffectStateTypeId, light->effect());
    } else if (thing->thingClassId() == colorTemperatureLightThingClassId) {
        thing->setStateValue(colorTemperatureLightConnectedStateTypeId, light->reachable());
        thing->setStateValue(colorTemperatureLightPowerStateTypeId, light->power());
        thing->setStateValue(colorTemperatureLightBrightnessStateTypeId, brightnessToPercentage(light->brightness()));
        thing->setStateValue(colorTemperatureLightColorTemperatureStateTypeId, light->ct());
    } else if (thing->thingClassId() == dimmableLightThingClassId) {
        thing->setStateValue(dimmableLightConnectedStateTypeId, light->reachable());
        thing->setStateValue(dimmableLightPowerStateTypeId, light->power());
        thing->setStateValue(dimmableLightBrightnessStateTypeId, brightnessToPercentage(light->brightness()));
    } else if (thing->thingClassId() == onOffLightThingClassId) {
        thing->setStateValue(onOffLightConnectedStateTypeId, light->reachable());
        thing->setStateValue(onOffLightPowerStateTypeId, light->power());
    } else if (thing->thingClassId() == smartPlugThingClassId) {
        thing->setStateValue(smartPlugConnectedStateTypeId, light->reachable());
        thing->setStateValue(smartPlugPowerStateTypeId, light->power());
    }
}

void IntegrationPluginPhilipsHue::remoteStateChanged()
{
    HueRemote *remote = static_cast<HueRemote *>(sender());

    Thing *thing = m_remotes.value(remote);
    if (!thing) {
        qCWarning(dcPhilipsHue) << "Could not find thing for remote" << remote->name();
        return;
    }
    if (thing->thingClassId() == remoteThingClassId) {
        thing->setStateValue(remoteConnectedStateTypeId, remote->reachable());
        thing->setStateValue(remoteBatteryLevelStateTypeId, remote->battery());
        thing->setStateValue(remoteBatteryCriticalStateTypeId, remote->battery() < 5);
    } else if (thing->thingClassId() == dimmerSwitch2ThingClassId) {
        thing->setStateValue(dimmerSwitch2ConnectedStateTypeId, remote->reachable());
        thing->setStateValue(dimmerSwitch2BatteryLevelStateTypeId, remote->battery());
        thing->setStateValue(dimmerSwitch2BatteryCriticalStateTypeId, remote->battery() < 5);
    } else if (thing->thingClassId() == tapDialThingClassId) {
        thing->setStateValue(tapDialConnectedStateTypeId, remote->reachable());
        thing->setStateValue(tapDialBatteryLevelStateTypeId, remote->battery());
        thing->setStateValue(tapDialBatteryCriticalStateTypeId, remote->battery() < 5);
    } else if (thing->thingClassId() == tapThingClassId) {
        thing->setStateValue(tapConnectedStateTypeId, remote->reachable());
    } else if (thing->thingClassId() == fohThingClassId) {
        thing->setStateValue(fohConnectedStateTypeId, remote->reachable());
    } else if (thing->thingClassId() == smartButtonThingClassId) {
        thing->setStateValue(smartButtonConnectedStateTypeId, remote->reachable());
        thing->setStateValue(smartButtonBatteryLevelStateTypeId, remote->battery());
        thing->setStateValue(smartButtonBatteryCriticalStateTypeId, remote->battery() < 5);
    } else if (thing->thingClassId() == wallSwitchThingClassId) {
        thing->setStateValue(wallSwitchConnectedStateTypeId, remote->reachable());
        thing->setStateValue(wallSwitchBatteryLevelStateTypeId, remote->battery());
        thing->setStateValue(wallSwitchBatteryCriticalStateTypeId, remote->battery() < 5);
    }
}

void IntegrationPluginPhilipsHue::onRemoteButtonEvent(int buttonCode)
{
    HueRemote *remote = static_cast<HueRemote *>(sender());
    Thing *thing = m_remotes.value(remote);
    if (!thing) {
        qCWarning(dcPhilipsHue()) << "Received a button press event for a thing we don't know!";
        return;
    }

    EventTypeId id;
    Param param;

    if (thing->thingClassId() == remoteThingClassId) {
        switch (buttonCode) {
        case 1002:
            param = Param(remotePressedEventButtonNameParamTypeId, "ON");
            id = remotePressedEventTypeId;
            break;
        case 1001:
            param = Param(remoteLongPressedEventButtonNameParamTypeId, "ON");
            id = remoteLongPressedEventTypeId;
            break;
        case 2002:
            param = Param(remotePressedEventButtonNameParamTypeId, "DIM UP");
            id = remotePressedEventTypeId;
            break;
        case 2001:
            param = Param(remoteLongPressedEventButtonNameParamTypeId, "DIM UP");
            id = remoteLongPressedEventTypeId;
            break;
        case 3002:
            param = Param(remotePressedEventButtonNameParamTypeId, "DIM DOWN");
            id = remotePressedEventTypeId;
            break;
        case 3001:
            param = Param(remoteLongPressedEventButtonNameParamTypeId, "DIM DOWN");
            id = remoteLongPressedEventTypeId;
            break;
        case 4002:
            param = Param(remotePressedEventButtonNameParamTypeId, "OFF");
            id = remotePressedEventTypeId;
            break;
        case 4001:
            param = Param(remoteLongPressedEventButtonNameParamTypeId, "OFF");
            id = remoteLongPressedEventTypeId;
            break;
        default:
            qCDebug(dcPhilipsHue()) << "Unhandled button code received from Hue Remote:" << buttonCode;
            return;
        }
    } else if (thing->thingClassId() == dimmerSwitch2ThingClassId) {
        switch (buttonCode) {
        case 1002:
            param = Param(dimmerSwitch2PressedEventButtonNameParamTypeId, "POWER");
            id = dimmerSwitch2PressedEventTypeId;
            break;
        case 1001:
            param = Param(dimmerSwitch2LongPressedEventButtonNameParamTypeId, "POWER");
            id = dimmerSwitch2LongPressedEventTypeId;
            break;
        case 2002:
            param = Param(dimmerSwitch2PressedEventButtonNameParamTypeId, "DIM UP");
            id = dimmerSwitch2PressedEventTypeId;
            break;
        case 2001:
            param = Param(dimmerSwitch2LongPressedEventButtonNameParamTypeId, "DIM UP");
            id = dimmerSwitch2LongPressedEventTypeId;
            break;
        case 3002:
            param = Param(dimmerSwitch2PressedEventButtonNameParamTypeId, "DIM DOWN");
            id = dimmerSwitch2PressedEventTypeId;
            break;
        case 3001:
            param = Param(dimmerSwitch2LongPressedEventButtonNameParamTypeId, "DIM DOWN");
            id = dimmerSwitch2LongPressedEventTypeId;
            break;
        case 4002:
            param = Param(dimmerSwitch2PressedEventButtonNameParamTypeId, "HUE");
            id = dimmerSwitch2PressedEventTypeId;
            break;
        case 4001:
            param = Param(dimmerSwitch2LongPressedEventButtonNameParamTypeId, "HUE");
            id = dimmerSwitch2LongPressedEventTypeId;
            break;
        default:
            qCDebug(dcPhilipsHue()) << "Unhandled button code received from Hue Dimmer Switch V2:" << buttonCode;
            return;
        }
        // codes ending in 2 (e.g. 1002) are short presses;
        // for long presses the Dimmer Switch V2 sends 3 codes (same behaviour as hue remote and smart button): 
        // * codes ending in 0 (e.g. 1000) indicate start of long press
        // * codes ending in 3 (e.g. 1003) indicate end of long press
        // * codes ending in 1 (e.g. 1001) are sent during the long press
    } else if (thing->thingClassId() == tapThingClassId) {
        switch (buttonCode) {
        case 34:
            param = Param(tapPressedEventButtonNameParamTypeId, "•");
            id = tapPressedEventTypeId;
            break;
        case 16:
            param = Param(tapPressedEventButtonNameParamTypeId, "••");
            id = tapPressedEventTypeId;
            break;
        case 17:
            param = Param(tapPressedEventButtonNameParamTypeId, "•••");
            id = tapPressedEventTypeId;
            break;
        case 18:
            param = Param(tapPressedEventButtonNameParamTypeId, "••••");
            id = tapPressedEventTypeId;
            break;
        default:
            qCDebug(dcPhilipsHue()) << "Received unhandled button code from Hue Tap:" << buttonCode;
            return;
        }
    } else if (thing->thingClassId() == fohThingClassId) {
        switch (buttonCode) {
        case 20:
            param = Param(fohPressedEventButtonNameParamTypeId, "UPPER LEFT");
            id = fohPressedEventTypeId;
            break;
        case 21:
            param = Param(fohPressedEventButtonNameParamTypeId, "LOWER LEFT");
            id = fohPressedEventTypeId;
            break;
        case 23:
            param = Param(fohPressedEventButtonNameParamTypeId, "UPPER RIGHT");
            id = fohPressedEventTypeId;
            break;
        case 22:
            param = Param(fohPressedEventButtonNameParamTypeId, "LOWER RIGHT");
            id = fohPressedEventTypeId;
            break;
        default:
            qCDebug(dcPhilipsHue()) << "Received unhandled button code from Friends of Hue switch:" << buttonCode;
            return;
        }
    } else if (thing->thingClassId() == smartButtonThingClassId) {
        switch (buttonCode) {
        case 1002:
            id = smartButtonPressedEventTypeId;
            break;
        case 1001:
            id = smartButtonLongPressedEventTypeId;
            break;
        default:
            qCDebug(dcPhilipsHue()) << "Received unhandled button code from Hue Smart Button:" << buttonCode;
            return;
        }
    } else if (thing->thingClassId() == wallSwitchThingClassId) {
        switch (buttonCode) {
        case 1002: // temporary number, replace with code (codes for on and off?)
            param = Param(wallSwitchPressedEventButtonNameParamTypeId, "ONE");
            id = wallSwitchPressedEventTypeId;
            break;
        case 2002: // temporary number, replace with code (codes for on and off?)
            param = Param(wallSwitchPressedEventButtonNameParamTypeId, "TWO");
            id = wallSwitchPressedEventTypeId;
            break;
        default:
            qCDebug(dcPhilipsHue()) << "Received unhandled button code from Hue Wall Switch Module:" << buttonCode;
            return;
        }
    }
    emitEvent(Event(id, m_remotes.value(remote)->id(), ParamList() << param));
}

void IntegrationPluginPhilipsHue::onTapDialButtonEvent(int buttonCode)
{
    HueTapDial *tapDial = static_cast<HueTapDial *>(sender());
    Thing *thing = m_tapDials.value(tapDial);
    if (!thing) {
        qCWarning(dcPhilipsHue()) << "Received a button press event for a thing we don't know!";
        return;
    }

    EventTypeId id;
    Param param;

    if (thing->thingClassId() == tapDialThingClassId) {
        switch (buttonCode) {
        case 1002:
            param = Param(tapDialPressedEventButtonNameParamTypeId, "•");
            id = tapDialPressedEventTypeId;
            break;
        case 1001:
            param = Param(tapDialLongPressedEventButtonNameParamTypeId, "•");
            id = tapDialLongPressedEventTypeId;
            break;
        case 2002:
            param = Param(tapDialPressedEventButtonNameParamTypeId, "••");
            id = tapDialPressedEventTypeId;
            break;
        case 2001:
            param = Param(tapDialLongPressedEventButtonNameParamTypeId, "••");
            id = tapDialLongPressedEventTypeId;
            break;
        case 3002:
            param = Param(tapDialPressedEventButtonNameParamTypeId, "•••");
            id = tapDialPressedEventTypeId;
            break;
        case 3001:
            param = Param(tapDialLongPressedEventButtonNameParamTypeId, "•••");
            id = tapDialLongPressedEventTypeId;
            break;
        case 4002:
            param = Param(tapDialPressedEventButtonNameParamTypeId, "••••");
            id = tapDialPressedEventTypeId;
            break;
        case 4001:
            param = Param(tapDialLongPressedEventButtonNameParamTypeId, "••••");
            id = tapDialLongPressedEventTypeId;
            break;
        default:
            qCDebug(dcPhilipsHue()) << "Unhandled button code received from Hue Tap Dial:" << buttonCode << "Thing name:" << thing->name();
            return;
        }
    }
    emitEvent(Event(id, m_tapDials.value(tapDial)->id(), ParamList() << param));
}


void IntegrationPluginPhilipsHue::onTapDialRotaryEvent(int rotationCode)
{
    HueTapDial *tapDial = static_cast<HueTapDial *>(sender());
    Thing *thing = m_tapDials.value(tapDial);
    if (!thing) {
        qCWarning(dcPhilipsHue()) << "Received a rotary event for a thing we don't know!";
        return;
    }

    EventTypeId id;
    Param param;
    int currentLevel = thing->stateValue(tapDialLevelStateTypeId).toUInt();
    int stepSize = thing->setting(tapDialSettingsStepSizeParamTypeId).toUInt();
    int largeStepSize = thing->setting(tapDialSettingsLargeStepSizeParamTypeId).toUInt();

    if (thing->thingClassId() == tapDialThingClassId) {
        qCDebug(dcPhilipsHue()) << "Rotation code received from Hue Tap Dial:" << rotationCode << "Thing name:" << thing->name();
        if (rotationCode == 15) {
            id = tapDialIncreaseEventTypeId;
            thing->setStateValue(tapDialLevelStateTypeId, qMin(100, currentLevel + stepSize));
        } else if (rotationCode == -15) {   
            id = tapDialDecreaseEventTypeId;
            thing->setStateValue(tapDialLevelStateTypeId, qMax(0, currentLevel - stepSize));
        } else if (rotationCode > 15) {   
            id = tapDialLargeIncreaseEventTypeId;
            thing->setStateValue(tapDialLevelStateTypeId, qMin(100, currentLevel + largeStepSize));
        } else if (rotationCode < -15) {  
            id = tapDialLargeDecreaseEventTypeId;
            thing->setStateValue(tapDialLevelStateTypeId, qMax(0, currentLevel - largeStepSize));
        } else {
            qCDebug(dcPhilipsHue()) << "Unhandled rotation code received from Hue Tap Dial:" << rotationCode << "Thing name:" << thing->name();
            return;
        }
    }
    emitEvent(Event(id, m_tapDials.value(tapDial)->id()));
}

void IntegrationPluginPhilipsHue::onTapDialReachableChanged(bool reachable)
{
    HueTapDial *tapDial = static_cast<HueTapDial *>(sender());
    Thing *tapDialDevice = m_tapDials.value(tapDial);
    tapDialDevice->setStateValue(tapDialConnectedStateTypeId, reachable);
}

void IntegrationPluginPhilipsHue::onTapDialBatteryLevelChanged(int batteryLevel)
{
    HueTapDial *tapDial = static_cast<HueTapDial *>(sender());
    Thing *tapDialDevice = m_tapDials.value(tapDial);
    tapDialDevice->setStateValue(tapDialBatteryLevelStateTypeId, batteryLevel);
    tapDialDevice->setStateValue(tapDialBatteryCriticalStateTypeId, (batteryLevel < 5));
}

void IntegrationPluginPhilipsHue::onMotionSensorReachableChanged(bool reachable)
{
    HueMotionSensor *sensor = static_cast<HueMotionSensor *>(sender());
    Thing *sensorDevice = m_motionSensors.value(sensor);
    sensorDevice->setStateValue(sensor->connectedStateTypeId(), reachable);
}

void IntegrationPluginPhilipsHue::onMotionSensorBatteryLevelChanged(int batteryLevel)
{
    HueMotionSensor *sensor = static_cast<HueMotionSensor *>(sender());
    Thing *sensorDevice = m_motionSensors.value(sensor);
    sensorDevice->setStateValue(sensor->batteryLevelStateTypeId(), batteryLevel);
    sensorDevice->setStateValue(sensor->batteryCriticalStateTypeId(), (batteryLevel < 5));
}

void IntegrationPluginPhilipsHue::onMotionSensorTemperatureChanged(double temperature)
{
    HueMotionSensor *sensor = static_cast<HueMotionSensor *>(sender());
    Thing *sensorDevice = m_motionSensors.value(sensor);
    sensorDevice->setStateValue(sensor->temperatureStateTypeId(), temperature);
}

void IntegrationPluginPhilipsHue::onMotionSensorPresenceChanged(bool presence)
{
    HueMotionSensor *sensor = static_cast<HueMotionSensor *>(sender());
    Thing *sensorDevice = m_motionSensors.value(sensor);
    sensorDevice->setStateValue(sensor->isPresentStateTypeId(), presence);
    if (presence) sensorDevice->setStateValue(sensor->lastSeenTimeStateTypeId(), QDateTime::currentDateTime().toTime_t());
}

void IntegrationPluginPhilipsHue::onMotionSensorLightIntensityChanged(double lightIntensity)
{
    HueMotionSensor *sensor = static_cast<HueMotionSensor *>(sender());
    Thing *sensorDevice = m_motionSensors.value(sensor);
    sensorDevice->setStateValue(sensor->lightIntensityStateTypeId(), lightIntensity);
}

void IntegrationPluginPhilipsHue::refreshLight(Thing *thing)
{
    HueLight *light = m_lights.key(thing);

    QNetworkRequest request(QUrl("http://" + light->hostAddress().toString() + "/api/" + light->apiKey() + "/lights/" + QString::number(light->id())));
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    QNetworkReply *reply = hardwareManager()->networkManager()->get(request);
    connect(reply, &QNetworkReply::finished, this, &IntegrationPluginPhilipsHue::networkManagerReplyReady);
    m_lightRefreshRequests.insert(reply, thing);
}

void IntegrationPluginPhilipsHue::refreshBridge(Thing *thing)
{
    HueBridge *bridge = m_bridges.key(thing);
    //    qCDebug(dcPhilipsHue()) << "refreshing bridge";

    QNetworkRequest request(QUrl("http://" + bridge->hostAddress().toString() + "/api/" + bridge->apiKey() + "/config"));
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    QNetworkReply *reply = hardwareManager()->networkManager()->get(request);
    connect(reply, &QNetworkReply::finished, this, &IntegrationPluginPhilipsHue::networkManagerReplyReady);
    m_bridgeRefreshRequests.insert(reply, thing);
}

void IntegrationPluginPhilipsHue::refreshLights(HueBridge *bridge)
{
    Thing *thing = m_bridges.value(bridge);
    //    qCDebug(dcPhilipsHue()) << "refreshing lights";

    QNetworkRequest request(QUrl("http://" + bridge->hostAddress().toString() + "/api/" + bridge->apiKey() + "/lights"));
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    QNetworkReply *reply = hardwareManager()->networkManager()->get(request);
    connect(reply, &QNetworkReply::finished, this, &IntegrationPluginPhilipsHue::networkManagerReplyReady);
    m_lightsRefreshRequests.insert(reply, thing);
}

void IntegrationPluginPhilipsHue::refreshSensors(HueBridge *bridge)
{
    Thing *thing = m_bridges.value(bridge);
    //    qCDebug(dcPhilipsHue()) << "refreshing sensors";

    QNetworkRequest request(QUrl("http://" + bridge->hostAddress().toString() + "/api/" + bridge->apiKey() + "/sensors"));
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    QNetworkReply *reply = hardwareManager()->networkManager()->get(request);
    connect(reply, &QNetworkReply::finished, this, &IntegrationPluginPhilipsHue::networkManagerReplyReady);
    m_sensorsRefreshRequests.insert(reply, thing);
}

void IntegrationPluginPhilipsHue::discoverBridgeDevices(HueBridge *bridge)
{
    Thing *thing = m_bridges.value(bridge);
    qCDebug(dcPhilipsHue) << "Asking bridge for new devices" << bridge->hostAddress();

    QPair<QNetworkRequest, QByteArray> lightsRequest = bridge->createDiscoverLightsRequest();
    QNetworkReply *lightsReply = hardwareManager()->networkManager()->get(lightsRequest.first);
    connect(lightsReply, &QNetworkReply::finished, this, &IntegrationPluginPhilipsHue::networkManagerReplyReady);
    m_bridgeLightsDiscoveryRequests.insert(lightsReply, thing);

    QPair<QNetworkRequest, QByteArray> sensorsRequest = bridge->createSearchSensorsRequest();
    QNetworkReply *reply = hardwareManager()->networkManager()->get(sensorsRequest.first);
    connect(reply, &QNetworkReply::finished, this, &IntegrationPluginPhilipsHue::networkManagerReplyReady);
    m_bridgeSensorsDiscoveryRequests.insert(reply, thing);
}

void IntegrationPluginPhilipsHue::searchNewDevices(HueBridge *bridge, const QString &serialNumber)
{
    Thing *thing = m_bridges.value(bridge);
    qCDebug(dcPhilipsHue) << "Triggering ZigBee scan on bridge" << bridge->hostAddress();

    QPair<QNetworkRequest, QByteArray> request = bridge->createSearchLightsRequest(serialNumber);
    QNetworkReply *reply = hardwareManager()->networkManager()->post(request.first, request.second);
    connect(reply, &QNetworkReply::finished, this, &IntegrationPluginPhilipsHue::networkManagerReplyReady);
    m_bridgeSearchDevicesRequests.insert(reply, thing);
}

void IntegrationPluginPhilipsHue::setLightName(Thing *thing)
{
    HueLight *light = m_lights.key(thing);

    QVariantMap requestMap;
    requestMap.insert("name", thing->name());
    QJsonDocument jsonDoc = QJsonDocument::fromVariant(requestMap);

    QNetworkRequest request(QUrl("http://" + light->hostAddress().toString() + "/api/" + light->apiKey() +
                                 "/lights/" + QString::number(light->id())));
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

    QNetworkReply *reply = hardwareManager()->networkManager()->put(request,jsonDoc.toJson());
    connect(reply, &QNetworkReply::finished, this, &IntegrationPluginPhilipsHue::networkManagerReplyReady);
    m_setNameRequests.insert(reply, thing);
}

void IntegrationPluginPhilipsHue::setRemoteName(Thing *thing)
{
    HueRemote *remote = m_remotes.key(thing);

    QVariantMap requestMap;
    requestMap.insert("name", thing->name());
    QJsonDocument jsonDoc = QJsonDocument::fromVariant(requestMap);

    QNetworkRequest request(QUrl("http://" + remote->hostAddress().toString() + "/api/" + remote->apiKey() +
                                 "/sensors/" + QString::number(remote->id())));
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

    QNetworkReply *reply = hardwareManager()->networkManager()->put(request,jsonDoc.toJson());
    connect(reply, &QNetworkReply::finished, this, &IntegrationPluginPhilipsHue::networkManagerReplyReady);
    m_setNameRequests.insert(reply, thing);
}

void IntegrationPluginPhilipsHue::processBridgeLightDiscoveryResponse(Thing *thing, const QByteArray &data)
{
    QJsonParseError error;
    QJsonDocument jsonDoc = QJsonDocument::fromJson(data, &error);

    // Check JSON error
    if (error.error != QJsonParseError::NoError) {
        qCWarning(dcPhilipsHue) << "Bridge light discovery json error in response" << error.errorString();
        return;
    }

    // Check response error
    if (data.contains("error")) {
        if (!jsonDoc.toVariant().toList().isEmpty()) {
            qCWarning(dcPhilipsHue) << "Failed to discover Hue Bridge lights:" << jsonDoc.toVariant().toList().first().toMap().value("error").toMap().value("description").toString();
        } else {
            qCWarning(dcPhilipsHue) << "Failed to discover Hue Bridge lights: Invalid error message format";
        }
        return;
    }

    // qCDebug(dcPhilipsHue()) << "Lights on bridge:" << qUtf8Printable(jsonDoc.toJson());

    // Create Lights if not already added
    ThingDescriptors descriptors;

    QVariantMap lightsMap = jsonDoc.toVariant().toMap();
    QList<HueLight*> lightsToRemove = m_lights.keys();
    foreach (QString lightId, lightsMap.keys()) {
        QVariantMap lightMap = lightsMap.value(lightId).toMap();

        QString uuid = lightMap.value("uniqueid").toString();
        QString model = lightMap.value("modelid").toString();
        QString type = lightMap.value("type").toString();

        foreach (HueLight *light, lightsToRemove) {
            if (light->uuid() == uuid) {
                lightsToRemove.removeAll(light);
                break;
            }
        }

        if (lightAlreadyAdded(uuid))
            continue;

        if (type == "Dimmable light") {
            ThingDescriptor descriptor(dimmableLightThingClassId, lightMap.value("name").toString(), "Philips Hue White Light", thing->id());
            ParamList params;
            params.append(Param(dimmableLightThingModelIdParamTypeId, model));
            params.append(Param(dimmableLightThingTypeParamTypeId, lightMap.value("type").toString()));
            params.append(Param(dimmableLightThingUuidParamTypeId, uuid));
            params.append(Param(dimmableLightThingLightIdParamTypeId, lightId));
            descriptor.setParams(params);
            descriptors.append(descriptor);

            qCDebug(dcPhilipsHue) << "Found new dimmable light" << lightMap.value("name").toString() << model;
        } else if (type == "On/Off light") {
            ThingDescriptor descriptor(onOffLightThingClassId, lightMap.value("name").toString(), "Philips Hue On/Off Light", thing->id());
            ParamList params;
            params.append(Param(onOffLightThingModelIdParamTypeId, model));
            params.append(Param(onOffLightThingTypeParamTypeId, lightMap.value("type").toString()));
            params.append(Param(onOffLightThingUuidParamTypeId, uuid));
            params.append(Param(onOffLightThingLightIdParamTypeId, lightId));
            descriptor.setParams(params);
            descriptors.append(descriptor);

            qCDebug(dcPhilipsHue) << "Found new on/off light" << lightMap.value("name").toString() << model;
        } else if (type == "Color temperature light") {
            ThingDescriptor descriptor(colorTemperatureLightThingClassId, lightMap.value("name").toString(), "Philips Hue Color Temperature Light", thing->id());
            ParamList params;
            params.append(Param(colorTemperatureLightThingModelIdParamTypeId, model));
            params.append(Param(colorTemperatureLightThingTypeParamTypeId, lightMap.value("type").toString()));
            params.append(Param(colorTemperatureLightThingUuidParamTypeId, uuid));
            params.append(Param(colorTemperatureLightThingLightIdParamTypeId, lightId));
            descriptor.setParams(params);
            descriptors.append(descriptor);

            qCDebug(dcPhilipsHue) << "Found new color temperature light" << lightMap.value("name").toString() << model;
        } else if (type == "Extended color light") {
            ThingDescriptor descriptor(colorLightThingClassId, lightMap.value("name").toString(), "Philips Hue Color Light", thing->id());
            ParamList params;
            params.append(Param(colorLightThingModelIdParamTypeId, model));
            params.append(Param(colorLightThingTypeParamTypeId, lightMap.value("type").toString()));
            params.append(Param(colorLightThingUuidParamTypeId, uuid));
            params.append(Param(colorLightThingLightIdParamTypeId, lightId));
            descriptor.setParams(params);
            descriptors.append(descriptor);
            qCDebug(dcPhilipsHue) << "Found new color light" << lightMap.value("name").toString() << model;
        } else if (type == "On/Off plug-in unit") {
            ThingDescriptor descriptor(smartPlugThingClassId, lightMap.value("name").toString(), "Philips Hue Smart plug", thing->id());
            ParamList params;
            params.append(Param(smartPlugThingModelIdParamTypeId, model));
            params.append(Param(smartPlugThingTypeParamTypeId, lightMap.value("type").toString()));
            params.append(Param(smartPlugThingUuidParamTypeId, uuid));
            params.append(Param(smartPlugThingLightIdParamTypeId, lightId));
            descriptor.setParams(params);
            descriptors.append(descriptor);
            qCDebug(dcPhilipsHue) << "Found smart plug" << lightMap.value("name").toString() << model;
        }
    }

    if (!descriptors.isEmpty()) {
        emit autoThingsAppeared(descriptors);
    }

    foreach (HueLight *light, lightsToRemove) {
        Thing *lightThing = m_lights.value(light);
        if (lightThing->parentId() == thing->id()) {
            emit autoThingDisappeared(lightThing->id());
        }
    }
}

void IntegrationPluginPhilipsHue::processBridgeSensorDiscoveryResponse(Thing *thing, const QByteArray &data)
{
    QJsonParseError error;
    QJsonDocument jsonDoc = QJsonDocument::fromJson(data, &error);

    // Check JSON error
    if (error.error != QJsonParseError::NoError) {
        qCWarning(dcPhilipsHue) << "Bridge sensor discovery json error in response" << error.errorString();
        return;
    }

    // Check response error
    if (data.contains("error")) {
        if (!jsonDoc.toVariant().toList().isEmpty()) {
            qCWarning(dcPhilipsHue) << "Failed to discover Hue Bridge sensors:" << jsonDoc.toVariant().toList().first().toMap().value("error").toMap().value("description").toString();
        } else {
            qCWarning(dcPhilipsHue) << "Failed to discover Hue Bridge sensors: Invalid error message format";
        }
        return;
    }

    HueBridge *bridge = m_bridges.key(thing);
    if (!bridge) {
        qCWarning(dcPhilipsHue()) << "Received a reply for a bridge we don't have any more.";
        return;
    }

    // qCDebug(dcPhilipsHue()) << "Sensors on bridge:" << qUtf8Printable(jsonDoc.toJson());

    // Create sensors if not already added
    QVariantMap sensorsMap = jsonDoc.toVariant().toMap();
    QHash<QString, HueMotionSensor *> motionSensors;
    QHash<QString, HueTapDial *> tapDials;
    QList<HueRemote*> remotesToRemove = m_remotes.keys();
    QList<HueMotionSensor*> sensorsToRemove = m_motionSensors.keys();
    QList<HueTapDial*> tapDialsToRemove = m_tapDials.keys();
    foreach (const QString &sensorId, sensorsMap.keys()) {

        QVariantMap sensorMap = sensorsMap.value(sensorId).toMap();
        QString uuid = sensorMap.value("uniqueid").toString();
        QString model = sensorMap.value("modelid").toString();

        //        qCDebug(dcPhilipsHue()) << "Found sensor on bridge:" << model << uuid;
        foreach (HueRemote* remote, remotesToRemove) {
            //            qCDebug(dcPhilipsHue()) << "  - Checking remote to remove" << remote->modelId() << remote->uuid();
            if (remote->uuid() == uuid) {
                remotesToRemove.removeAll(remote);
                break;
            }
        }
        foreach (HueTapDial* tapDial, tapDialsToRemove) {
            if (tapDial->uuid() == uuid.split("-").first()) {
                tapDialsToRemove.removeAll(tapDial);
                break;
            }
        }
        foreach (HueMotionSensor* sensor, sensorsToRemove) {
            if (sensor->uuid() == uuid.split("-").first()) {
                sensorsToRemove.removeAll(sensor);
                break;
            }
        }

        if (sensorAlreadyAdded(uuid))
            continue;

        // Dimmer Switch: RWL020 == US, RWL021 == EU
        if (model == "RWL020" || model == "RWL021") {
            ThingDescriptor descriptor(remoteThingClassId, sensorMap.value("name").toString(), "Philips Hue Remote", thing->id());
            ParamList params;
            params.append(Param(remoteThingModelIdParamTypeId, model));
            params.append(Param(remoteThingTypeParamTypeId, sensorMap.value("type").toString()));
            params.append(Param(remoteThingUuidParamTypeId, uuid));
            params.append(Param(remoteThingSensorIdParamTypeId, sensorId));
            descriptor.setParams(params);
            emit autoThingsAppeared({descriptor});
            qCDebug(dcPhilipsHue) << "Found new remote" << sensorMap.value("name").toString() << model;

            // Dimmer Switch V2
        } else if (model == "RWL022") {
            ThingDescriptor descriptor(dimmerSwitch2ThingClassId, sensorMap.value("name").toString(), "Philips Hue Dimmer Switch V2", thing->id());
            ParamList params;
            params.append(Param(dimmerSwitch2ThingModelIdParamTypeId, model));
            params.append(Param(dimmerSwitch2ThingTypeParamTypeId, sensorMap.value("type").toString()));
            params.append(Param(dimmerSwitch2ThingUuidParamTypeId, uuid));
            params.append(Param(dimmerSwitch2ThingSensorIdParamTypeId, sensorId));
            descriptor.setParams(params);
            emit autoThingsAppeared({descriptor});
            qCDebug(dcPhilipsHue) << "Found new dimmer switch v2" << sensorMap.value("name").toString() << model;

            // Tap Dial
        } else if (model == "RDM002") {
            // Get the base uuid from this sensor
            QString baseUuid = HueDevice::getBaseUuid(uuid);
            qCDebug(dcPhilipsHue) << "Base uuid:" << baseUuid;

            // Rotary dial
            if (sensorMap.value("type").toString() == "ZLLRelativeRotary") {
                qCDebug(dcPhilipsHue()) << "Found rotary dial from tap dial:" << baseUuid << sensorMap;
                // Check if we have tap dial for this rotary dial
                if (tapDials.contains(baseUuid)) {
                    HueTapDial *tapDial = tapDials.value(baseUuid);
                    tapDial->setRotaryUuid(uuid);
                    tapDial->setRotaryId(sensorId.toInt());
                } else {
                    // Create a tap dial
                    HueTapDial *tapDial = nullptr;
                    tapDial = new HueTapDial(bridge, this);

                    tapDial->setModelId(model);
                    tapDial->setUuid(baseUuid);
                    tapDial->setRotaryUuid(uuid);
                    tapDial->setRotaryId(sensorId.toInt());
                    tapDials.insert(baseUuid, tapDial);
                }
            }
            // Buttons
            if (sensorMap.value("type").toString() == "ZLLSwitch") {
                qCDebug(dcPhilipsHue()) << "Found switch from tap dial:" << baseUuid << sensorMap;
                // Check if we have tap dial for this switch
                if (tapDials.contains(baseUuid)) {
                    HueTapDial *tapDial = tapDials.value(baseUuid);
                    tapDial->setSwitchUuid(uuid);
                    tapDial->setSwitchId(sensorId.toInt());
                } else {
                    // Create a tap dial
                    HueTapDial *tapDial = nullptr;
                    tapDial = new HueTapDial(bridge, this);

                    tapDial->setModelId(model);
                    tapDial->setUuid(baseUuid);
                    tapDial->setSwitchUuid(uuid);
                    tapDial->setSwitchId(sensorId.toInt());
                    tapDials.insert(baseUuid, tapDial);
                }
            }

            // Smart Button
        } else if (model == "ROM001") {
            ThingDescriptor descriptor(smartButtonThingClassId, sensorMap.value("name").toString(), "Philips Hue Smart Button", thing->id());
            ParamList params;
            params.append(Param(smartButtonThingModelIdParamTypeId, model));
            params.append(Param(smartButtonThingTypeParamTypeId, sensorMap.value("type").toString()));
            params.append(Param(smartButtonThingUuidParamTypeId, uuid));
            params.append(Param(smartButtonThingSensorIdParamTypeId, sensorId));
            descriptor.setParams(params);
            emit autoThingsAppeared({descriptor});
            qCDebug(dcPhilipsHue) << "Found new smart button" << sensorMap.value("name").toString() << model;

            // Wall Switch Module
        } else if (model == "RDM001") {
            ThingDescriptor descriptor(wallSwitchThingClassId, sensorMap.value("name").toString(), "Philips Hue Wall Switch Module", thing->id());
            ParamList params;
            params.append(Param(wallSwitchThingModelIdParamTypeId, model));
            params.append(Param(wallSwitchThingTypeParamTypeId, sensorMap.value("type").toString()));
            params.append(Param(wallSwitchThingUuidParamTypeId, uuid));
            params.append(Param(wallSwitchThingSensorIdParamTypeId, sensorId));
            descriptor.setParams(params);
            emit autoThingsAppeared({descriptor});
            qCDebug(dcPhilipsHue) << "Found new wall switch module" << sensorMap.value("name").toString() << model;

            // Friends of Hue switch
        } else if (model == "FOHSWITCH") {
            ThingDescriptor descriptor(fohThingClassId, sensorMap.value("name").toString(), "Friends of Hue Switch", thing->id());
            ParamList params;
            params.append(Param(fohThingModelIdParamTypeId, model));
            params.append(Param(fohThingTypeParamTypeId, sensorMap.value("type").toString()));
            params.append(Param(fohThingUuidParamTypeId, uuid));
            params.append(Param(fohThingSensorIdParamTypeId, sensorId));
            descriptor.setParams(params);
            emit autoThingsAppeared({descriptor});
            qCDebug(dcPhilipsHue) << "Found new friends of hue switch" << sensorMap.value("name").toString() << model;

            // Hue Tap
        } else if (sensorMap.value("type").toString() == "ZGPSwitch") {
            ThingDescriptor descriptor(tapThingClassId, sensorMap.value("name").toString(), "Philips Hue Tap", thing->id());
            ParamList params;
            params.append(Param(tapThingUuidParamTypeId, uuid));
            params.append(Param(tapThingModelIdParamTypeId, model));
            params.append(Param(tapThingSensorIdParamTypeId, sensorId));
            descriptor.setParams(params);
            emit autoThingsAppeared({descriptor});
            qCDebug(dcPhilipsHue()) << "Found hue tap:" << sensorMap << tapThingClassId;

        } else if (model == "SML001" || model == "SML002") {
            // Get the base uuid from this sensor
            QString baseUuid = HueDevice::getBaseUuid(uuid);

            // Temperature sensor
            if (sensorMap.value("type").toString() == "ZLLTemperature") {
                qCDebug(dcPhilipsHue()) << "Found temperature sensor from OurdoorSensor:" << baseUuid << sensorMap;
                // Check if we haven outdoor sensor for this temperature sensor
                if (motionSensors.contains(baseUuid)) {
                    HueMotionSensor *motionSensor = motionSensors.value(baseUuid);
                    motionSensor->setTemperatureSensorUuid(uuid);
                    motionSensor->setTemperatureSensorId(sensorId.toInt());
                } else {
                    // Create an outdoor sensor
                    HueMotionSensor *motionSensor = nullptr;
                    if (model == "SML001") {
                        motionSensor = new HueIndoorSensor(bridge, this);
                    } else {
                        motionSensor = new HueOutdoorSensor(bridge, this);
                    }
                    motionSensor->setModelId(model);
                    motionSensor->setUuid(baseUuid);
                    motionSensor->setTemperatureSensorUuid(uuid);
                    motionSensor->setTemperatureSensorId(sensorId.toInt());
                    motionSensors.insert(baseUuid, motionSensor);
                }
            }

            if (sensorMap.value("type").toString() == "ZLLPresence") {
                qCDebug(dcPhilipsHue()) << "Found presence sensor from OurdoorSensor:" << baseUuid << sensorMap;
                // Check if we haven outdoor sensor for this presence sensor
                if (motionSensors.contains(baseUuid)) {
                    HueMotionSensor *motionSensor = motionSensors.value(baseUuid);
                    motionSensor->setPresenceSensorUuid(uuid);
                    motionSensor->setPresenceSensorId(sensorId.toInt());
                } else {
                    // Create an outdoor sensor
                    HueMotionSensor *motionSensor = nullptr;
                    if (model == "SML001") {
                        motionSensor = new HueIndoorSensor(bridge, this);
                    } else {
                        motionSensor = new HueOutdoorSensor(bridge, this);
                    }
                    motionSensor->setModelId(model);
                    motionSensor->setUuid(baseUuid);
                    motionSensor->setPresenceSensorUuid(uuid);
                    motionSensor->setPresenceSensorId(sensorId.toInt());
                    motionSensors.insert(baseUuid, motionSensor);
                }
            }

            if (sensorMap.value("type").toString() == "ZLLLightLevel") {
                qCDebug(dcPhilipsHue()) << "Found light sensor from OurdoorSensor:" << sensorMap;
                // Check if we haven outdoor sensor for this light sensor
                if (motionSensors.contains(baseUuid)) {
                    HueMotionSensor *motionSensor = motionSensors.value(baseUuid);
                    motionSensor->setLightSensorUuid(uuid);
                    motionSensor->setLightSensorId(sensorId.toInt());
                } else {
                    // Create an outdoor sensor
                    HueMotionSensor *motionSensor = nullptr;
                    if (model == "SML001") {
                        motionSensor = new HueIndoorSensor(bridge, this);
                    } else {
                        motionSensor = new HueOutdoorSensor(bridge, this);
                    }
                    motionSensor->setModelId(model);
                    motionSensor->setUuid(baseUuid);
                    motionSensor->setLightSensorUuid(uuid);
                    motionSensor->setLightSensorId(sensorId.toInt());
                    motionSensors.insert(baseUuid, motionSensor);
                }
            }
        } else {
            qCDebug(dcPhilipsHue()) << "Found unknown sensor:" << model;
        }
    }

    // Create outdoor sensors if there are any new sensors found
    foreach (HueMotionSensor *motionSensor, motionSensors.values()) {
        QString baseUuid = motionSensors.key(motionSensor);
        if (motionSensor->isValid()) {
            if (motionSensor->modelId() == "SML001") {
                ThingDescriptor descriptor(motionSensorThingClassId, tr("Philips Hue Motion sensor"), baseUuid, thing->id());
                ParamList params;
                params.append(Param(motionSensorThingUuidParamTypeId, motionSensor->uuid()));
                params.append(Param(motionSensorThingModelIdParamTypeId, motionSensor->modelId()));
                params.append(Param(motionSensorThingSensorUuidTemperatureParamTypeId, motionSensor->temperatureSensorUuid()));
                params.append(Param(motionSensorThingSensorIdTemperatureParamTypeId, motionSensor->temperatureSensorId()));
                params.append(Param(motionSensorThingSensorUuidPresenceParamTypeId, motionSensor->presenceSensorUuid()));
                params.append(Param(motionSensorThingSensorIdPresenceParamTypeId, motionSensor->presenceSensorId()));
                params.append(Param(motionSensorThingSensorUuidLightParamTypeId, motionSensor->lightSensorUuid()));
                params.append(Param(motionSensorThingSensorIdLightParamTypeId, motionSensor->lightSensorId()));
                descriptor.setParams(params);
                qCDebug(dcPhilipsHue()) << "Found new motion sensor" << baseUuid << motionSensorThingClassId;
                emit autoThingsAppeared({descriptor});
            } else if (motionSensor->modelId() == "SML002") {
                ThingDescriptor descriptor(outdoorSensorThingClassId, tr("Philips Hue Outdoor sensor"), baseUuid, thing->id());
                ParamList params;
                params.append(Param(outdoorSensorThingUuidParamTypeId, motionSensor->uuid()));
                params.append(Param(outdoorSensorThingModelIdParamTypeId, motionSensor->modelId()));
                params.append(Param(outdoorSensorThingSensorUuidTemperatureParamTypeId, motionSensor->temperatureSensorUuid()));
                params.append(Param(outdoorSensorThingSensorIdTemperatureParamTypeId, motionSensor->temperatureSensorId()));
                params.append(Param(outdoorSensorThingSensorUuidPresenceParamTypeId, motionSensor->presenceSensorUuid()));
                params.append(Param(outdoorSensorThingSensorIdPresenceParamTypeId, motionSensor->presenceSensorId()));
                params.append(Param(outdoorSensorThingSensorUuidLightParamTypeId, motionSensor->lightSensorUuid()));
                params.append(Param(outdoorSensorThingSensorIdLightParamTypeId, motionSensor->lightSensorId()));
                descriptor.setParams(params);
                qCDebug(dcPhilipsHue()) << "Found new outdoor sensor" << baseUuid << outdoorSensorThingClassId;
                emit autoThingsAppeared({descriptor});
            }
        }

        // Clean up
        motionSensors.remove(baseUuid);
        motionSensor->deleteLater();
    }

    // Create tap dials if there are any new devices found
    foreach (HueTapDial *tapDial, tapDials.values()) {
        QString baseUuid = tapDials.key(tapDial);
        if (tapDial->isValid()) {
            ThingDescriptor descriptor(tapDialThingClassId, tr("Philips Hue Tap Dial"), baseUuid, thing->id());
            ParamList params;
            params.append(Param(tapDialThingModelIdParamTypeId, tapDial->modelId()));
            params.append(Param(tapDialThingUuidParamTypeId, tapDial->uuid()));
            params.append(Param(tapDialThingIdRotaryParamTypeId, tapDial->rotaryId()));
            params.append(Param(tapDialThingUuidRotaryParamTypeId, tapDial->rotaryUuid()));
            params.append(Param(tapDialThingIdSwitchParamTypeId, tapDial->switchId()));
            params.append(Param(tapDialThingUuidSwitchParamTypeId, tapDial->switchUuid()));
            descriptor.setParams(params);
            qCDebug(dcPhilipsHue()) << "Found new tap dial" << baseUuid << tapDialThingClassId;
            emit autoThingsAppeared({descriptor});
        }

        // Clean up
        tapDials.remove(baseUuid);
        tapDial->deleteLater();
    }

    foreach (HueRemote* remote, remotesToRemove) {
        Thing *remoteThing = m_remotes.value(remote);
        if (remoteThing->parentId() == thing->id()) {
            qCDebug(dcPhilipsHue()) << "Hue remote" << remote->uuid() << "disappeared from bridge";
            emit autoThingDisappeared(remoteThing->id());
        }
    }

    foreach (HueTapDial* tapDial, tapDialsToRemove) {
        Thing *tapDialThing = m_tapDials.value(tapDial);
        if (tapDialThing->parentId() == thing->id()) {
            qCDebug(dcPhilipsHue()) << "Hue tap dial disappeared from bridge";
            emit autoThingDisappeared(tapDialThing->id());
        }
    }

    foreach (HueMotionSensor* sensor, sensorsToRemove) {
        Thing *sensorThing = m_motionSensors.value(sensor);
        if (sensorThing->parentId() == thing->id()) {
            qCDebug(dcPhilipsHue()) << "Hue motion sensor disappeared from bridge";
            emit autoThingDisappeared(sensorThing->id());
        }
    }
}

void IntegrationPluginPhilipsHue::processLightRefreshResponse(Thing *thing, const QByteArray &data)
{
    QJsonParseError error;
    QJsonDocument jsonDoc = QJsonDocument::fromJson(data, &error);

    // check JSON error
    if (error.error != QJsonParseError::NoError) {
        qCWarning(dcPhilipsHue) << "Hue Bridge json error in response" << error.errorString();
        return;
    }

    // check response error
    if (data.contains("error")) {
        if (!jsonDoc.toVariant().toList().isEmpty()) {
            qCWarning(dcPhilipsHue) << "Failed to refresh Hue Light:" << jsonDoc.toVariant().toList().first().toMap().value("error").toMap().value("description").toString();
        } else {
            qCWarning(dcPhilipsHue) << "Failed to refresh Hue Light: Invalid error message format";
        }
        return;
    }

    HueLight *light = m_lights.key(thing);
    light->updateStates(jsonDoc.toVariant().toMap().value("state").toMap());
}

void IntegrationPluginPhilipsHue::processBridgeRefreshResponse(Thing *thing, const QByteArray &data)
{
    QJsonParseError error;
    QJsonDocument jsonDoc = QJsonDocument::fromJson(data, &error);

    // check JSON error
    if (error.error != QJsonParseError::NoError) {
        qCWarning(dcPhilipsHue) << "Hue Bridge json error in response" << error.errorString();
        return;
    }

    if (!jsonDoc.toVariant().toList().isEmpty()) {
        qCWarning(dcPhilipsHue) << "Failed to refresh Hue Bridge:" << jsonDoc.toVariant().toList().first().toMap().value("error").toMap().value("description").toString();
        bridgeReachableChanged(thing, false);
        return;
    }

    //qCDebug(dcPhilipsHue()) << "Bridge refresh response" << qUtf8Printable(jsonDoc.toJson(QJsonDocument::Indented));

    QVariantMap configMap = jsonDoc.toVariant().toMap();

    // mark bridge as reachable
    bridgeReachableChanged(thing, true);
    QString bridgeApiVersion = configMap.value("apiversion").toString();
    thing->setStateValue(bridgeApiVersionStateTypeId, bridgeApiVersion);
    thing->setStateValue(bridgeCurrentVersionStateTypeId, configMap.value("swversion").toString());

    HueBridge *bridge = m_bridges.key(thing);
    bridge->setApiVersion(bridgeApiVersion);
    if (bridgeApiVersion < "1.20") {
        int updateStatus = configMap.value("swupdate").toMap().value("updatestate").toInt();
        switch (updateStatus) {
        case 0:
            thing->setStateValue(bridgeUpdateStatusStateTypeId, "idle");
            break;
        case 1:
            thing->setStateValue(bridgeUpdateStatusStateTypeId, "idle");
            break;
        case 2:
            thing->setStateValue(bridgeUpdateStatusStateTypeId, "available");
            break;
        case 3:
            thing->setStateValue(bridgeUpdateStatusStateTypeId, "updating");
            break;
        default:
            break;
        }
    } else {
        QString updateStatus = configMap.value("swupdate2").toMap().value("state").toString();
        QHash<QString, QString> mapping = {
            {"unknown", "idle"},
            {"noupdates", "idle"},
            {"transferring", "idle"},
            {"anyreadytoinstall", "available"},
            {"allreadytoinstall", "available"},
            {"installing", "updating"}
        };
        thing->setStateValue(bridgeUpdateStatusStateTypeId, mapping.value(updateStatus));
    }

    discoverBridgeDevices(m_bridges.key(thing));
}

void IntegrationPluginPhilipsHue::processLightsRefreshResponse(Thing *thing, const QByteArray &data)
{
    QJsonParseError error;
    QJsonDocument jsonDoc = QJsonDocument::fromJson(data, &error);

    // check JSON error
    if (error.error != QJsonParseError::NoError) {
        qCWarning(dcPhilipsHue) << "Hue Bridge json error in response" << error.errorString();
        return;
    }

    // check response error
    if (data.contains("error")) {
        if (!jsonDoc.toVariant().toList().isEmpty()) {
            qCWarning(dcPhilipsHue) << "Failed to refresh Hue Lights:" << jsonDoc.toVariant().toList().first().toMap().value("error").toMap().value("description").toString();
        } else {
            qCWarning(dcPhilipsHue) << "Failed to refresh Hue Lights: Invalid error message format";
        }
        return;
    }

    // Update light states
    QVariantMap lightsMap = jsonDoc.toVariant().toMap();
    foreach (const QString &lightId, lightsMap.keys()) {
        QVariantMap lightMap = lightsMap.value(lightId).toMap();
        // get the light of this bridge
        foreach (HueLight *light, m_lights.keys()) {
            if (light->id() == lightId.toInt() && m_lights.value(light)->parentId() == thing->id()) {
                light->updateStates(lightMap.value("state").toMap());
            }
        }
    }
}

void IntegrationPluginPhilipsHue::processSensorsRefreshResponse(Thing *thing, const QByteArray &data)
{
    QJsonParseError error;
    QJsonDocument jsonDoc = QJsonDocument::fromJson(data, &error);

    // check JSON error
    if (error.error != QJsonParseError::NoError) {
        qCWarning(dcPhilipsHue) << "Hue Bridge json error in response" << error.errorString();
        return;
    }

    // check response error
    if (!jsonDoc.toVariant().toList().isEmpty()) {
        qCWarning(dcPhilipsHue) << "Failed to refresh Hue Sensors:" << jsonDoc.toVariant().toList().first().toMap().value("error").toMap().value("description").toString();
        return;
    }

    // Update sensor states
    QVariantMap sensorsMap = jsonDoc.toVariant().toMap();
    foreach (const QString &sensorId, sensorsMap.keys()) {
        QVariantMap sensorMap = sensorsMap.value(sensorId).toMap();

        // Remotes
        foreach (HueRemote *remote, m_remotes.keys()) {
            if (remote->id() == sensorId.toInt() && m_remotes.value(remote)->parentId() == thing->id()) {
                remote->updateStates(sensorMap.value("state").toMap(), sensorMap.value("config").toMap());
            }
        }

        // Tap dials
        foreach (HueTapDial *tapDial, m_tapDials.keys()) {
            if (tapDial->hasSensor(sensorId.toInt()) && m_tapDials.value(tapDial)->parentId() == thing->id()) {
                tapDial->updateStates(sensorMap);
            }
        }

        // Motion sensors
        foreach (HueMotionSensor *motionSensor, m_motionSensors.keys()) {
            if (motionSensor->hasSensor(sensorId.toInt()) && m_motionSensors.value(motionSensor)->parentId() == thing->id()) {
                motionSensor->updateStates(sensorMap);
            }
        }
    }
}

void IntegrationPluginPhilipsHue::processSetNameResponse(Thing *thing, const QByteArray &data)
{
    QJsonParseError error;
    QJsonDocument jsonDoc = QJsonDocument::fromJson(data, &error);

    // check JSON error
    if (error.error != QJsonParseError::NoError) {
        qCWarning(dcPhilipsHue) << "Hue Bridge json error in response" << error.errorString();
        return;
    }

    // check response error
    if (data.contains("error")) {
        if (!jsonDoc.toVariant().toList().isEmpty()) {
            qCWarning(dcPhilipsHue) << "Failed to set name of Hue:" << jsonDoc.toVariant().toList().first().toMap().value("error").toMap().value("description").toString();
        } else {
            qCWarning(dcPhilipsHue) << "Failed to set name of Hue: Invalid error message format";
        }
        return;
    }

    if (thing->thingClassId() == colorLightThingClassId || thing->thingClassId() == dimmableLightThingClassId || thing->thingClassId() == onOffLightThingClassId)
        refreshLight(thing);

}

void IntegrationPluginPhilipsHue::bridgeReachableChanged(Thing *thing, bool reachable)
{
    if (reachable) {
        thing->setStateValue(bridgeConnectedStateTypeId, true);
    } else {
        // mark bridge and corresponding hue devices unreachable
        if (thing->thingClassId() == bridgeThingClassId) {
            thing->setStateValue(bridgeConnectedStateTypeId, false);

            foreach (HueLight *light, m_lights.keys()) {
                if (m_lights.value(light)->parentId() == thing->id()) {
                    light->setReachable(false);
                    if (m_lights.value(light)->thingClassId() == colorLightThingClassId) {
                        m_lights.value(light)->setStateValue(colorLightConnectedStateTypeId, false);
                    } else if (m_lights.value(light)->thingClassId() == colorTemperatureLightThingClassId) {
                        m_lights.value(light)->setStateValue(colorTemperatureLightConnectedStateTypeId, false);
                    } else if (m_lights.value(light)->thingClassId() == dimmableLightThingClassId) {
                        m_lights.value(light)->setStateValue(dimmableLightConnectedStateTypeId, false);
                    } else if (m_lights.value(light)->thingClassId() == onOffLightThingClassId) {
                        m_lights.value(light)->setStateValue(onOffLightConnectedStateTypeId, false);
                    } else if (m_lights.value(light)->thingClassId() == smartPlugThingClassId) {
                        m_lights.value(light)->setStateValue(smartPlugConnectedStateTypeId, false);
                    }
                }
            }

            foreach (HueRemote *remote, m_remotes.keys()) {
                if (m_remotes.value(remote)->parentId() == thing->id()) {
                    remote->setReachable(false);
                    if (m_remotes.value(remote)->thingClassId() == remoteThingClassId) {
                        m_remotes.value(remote)->setStateValue(remoteConnectedStateTypeId, false);
                    } else if (m_remotes.value(remote)->thingClassId() == dimmerSwitch2ThingClassId) {
                        m_remotes.value(remote)->setStateValue(dimmerSwitch2ConnectedStateTypeId, false);
                    } else if (m_remotes.value(remote)->thingClassId() == tapDialThingClassId) {
                        m_remotes.value(remote)->setStateValue(tapDialConnectedStateTypeId, false);
                    } else if (m_remotes.value(remote)->thingClassId() == tapThingClassId) {
                        m_remotes.value(remote)->setStateValue(tapConnectedStateTypeId, false);
                    } else if (m_remotes.value(remote)->thingClassId() == fohThingClassId) {
                        m_remotes.value(remote)->setStateValue(fohConnectedStateTypeId, false);
                    } else if (m_remotes.value(remote)->thingClassId() == smartButtonThingClassId) {
                        m_remotes.value(remote)->setStateValue(smartButtonConnectedStateTypeId, false);
                    } else if (m_remotes.value(remote)->thingClassId() == wallSwitchThingClassId) {
                        m_remotes.value(remote)->setStateValue(wallSwitchConnectedStateTypeId, false);
                    }
                }
            }

            foreach (HueTapDial *tapDial, m_tapDials.keys()) {
                if (m_tapDials.value(tapDial)->parentId() == thing->id()) {
                    tapDial->setReachable(false);
                    m_tapDials.value(tapDial)->setStateValue(tapDialConnectedStateTypeId, false);
                }
            }

            foreach (HueMotionSensor *motionSensor, m_motionSensors.keys()) {
                if (m_motionSensors.value(motionSensor)->parentId() == thing->id()) {
                    motionSensor->setReachable(false);
                    m_motionSensors.value(motionSensor)->setStateValue(motionSensor->connectedStateTypeId(), false);
                }
            }
        }
    }
}

Thing* IntegrationPluginPhilipsHue::bridgeForBridgeId(const QString &id)
{
    foreach (Thing *thing, myThings()) {
        if (thing->thingClassId() == bridgeThingClassId) {
            if (thing->paramValue(bridgeThingIdParamTypeId).toString().toLower() == id) {
                return thing;
            }
        }
    }
    return nullptr;
}

bool IntegrationPluginPhilipsHue::lightAlreadyAdded(const QString &uuid)
{
    foreach (Thing *thing, myThings()) {
        if (thing->thingClassId() == colorLightThingClassId) {
            if (thing->paramValue(colorLightThingUuidParamTypeId).toString() == uuid) {
                return true;
            }
        } else if (thing->thingClassId() == dimmableLightThingClassId) {
            if (thing->paramValue(dimmableLightThingUuidParamTypeId).toString() == uuid) {
                return true;
            }
        } else if (thing->thingClassId() == onOffLightThingClassId) {
            if (thing->paramValue(onOffLightThingUuidParamTypeId).toString() == uuid) {
                return true;
            }
        }
        if (thing->thingClassId() == colorTemperatureLightThingClassId) {
            if (thing->paramValue(colorTemperatureLightThingUuidParamTypeId).toString() == uuid) {
                return true;
            }
        }
        if (thing->thingClassId() == smartPlugThingClassId) {
            if (thing->paramValue(smartPlugThingUuidParamTypeId).toString() == uuid) {
                return true;
            }
        }
    }
    return false;
}

bool IntegrationPluginPhilipsHue::sensorAlreadyAdded(const QString &uuid)
{
    foreach (Thing *thing, myThings()) {
        // Hue remote
        if (thing->thingClassId() == remoteThingClassId) {
            if (thing->paramValue(remoteThingUuidParamTypeId).toString() == uuid) {
                return true;
            }
        }

        // Hue dimmer switch V2
        if (thing->thingClassId() == dimmerSwitch2ThingClassId) {
            if (thing->paramValue(dimmerSwitch2ThingUuidParamTypeId).toString() == uuid) {
                return true;
            }
        }

        // Hue tap dial
        if (thing->thingClassId() == tapDialThingClassId) {
            if (thing->paramValue(tapDialThingUuidParamTypeId).toString() == uuid) {
                return true;
            }
        }

        // Hue tap
        if (thing->thingClassId() == tapThingClassId) {
            if (thing->paramValue(tapThingUuidParamTypeId).toString() == uuid) {
                return true;
            }
        }

        // Friends of Hue
        if (thing->thingClassId() == fohThingClassId) {
            if (thing->paramValue(fohThingUuidParamTypeId).toString() == uuid) {
                return true;
            }
        }

        // Hue smart button
        if (thing->thingClassId() == smartButtonThingClassId) {
            if (thing->paramValue(smartButtonThingUuidParamTypeId).toString() == uuid) {
                return true;
            }
        }

        // Hue wall switch module
        if (thing->thingClassId() == wallSwitchThingClassId) {
            if (thing->paramValue(wallSwitchThingUuidParamTypeId).toString() == uuid) {
                return true;
            }
        }

        // Tap Dial consists out of 2 devices
        if (thing->thingClassId() == tapDialThingClassId) {
            if (thing->paramValue(tapDialThingUuidRotaryParamTypeId).toString() == uuid) {
                return true;
            } else if (thing->paramValue(tapDialThingUuidSwitchParamTypeId).toString() == uuid) {
                return true;
            }
        }

        // Outdoor sensor consits out of 3 sensors
        if (thing->thingClassId() == outdoorSensorThingClassId) {
            if (thing->paramValue(outdoorSensorThingSensorUuidLightParamTypeId).toString() == uuid) {
                return true;
            } else if (thing->paramValue(outdoorSensorThingSensorUuidPresenceParamTypeId).toString() == uuid) {
                return true;
            } else if (thing->paramValue(outdoorSensorThingSensorUuidTemperatureParamTypeId).toString() == uuid) {
                return true;
            }
        }
        // Motion sensor consits out of 3 sensors
        if (thing->thingClassId() == motionSensorThingClassId) {
            if (thing->paramValue(motionSensorThingSensorUuidLightParamTypeId).toString() == uuid) {
                return true;
            } else if (thing->paramValue(motionSensorThingSensorUuidPresenceParamTypeId).toString() == uuid) {
                return true;
            } else if (thing->paramValue(motionSensorThingSensorUuidTemperatureParamTypeId).toString() == uuid) {
                return true;
            }
        }
    }

    return false;
}

int IntegrationPluginPhilipsHue::brightnessToPercentage(int brightness)
{
    return qRound((100.0 * brightness) / 255.0);
}

int IntegrationPluginPhilipsHue::percentageToBrightness(int percentage)
{
    return qRound((255.0 * percentage) / 100.0);
}

void IntegrationPluginPhilipsHue::abortRequests(QHash<QNetworkReply *, Thing *> requestList, Thing *thing)
{
    foreach (QNetworkReply* reply, requestList.keys()) {
        if (requestList.value(reply) == thing) {
            reply->abort();
        }
    }
}