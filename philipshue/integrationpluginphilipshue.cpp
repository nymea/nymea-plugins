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

}

void IntegrationPluginPhilipsHue::discoverThings(ThingDiscoveryInfo *info)
{
    // We're starting a UpnpDiscovery and a NUpnpDiscovery.
    // For that, we create a tracking object holding pointers to both of those discoveries.
    // Both discoveries add their results to a temporary list.
    // Once a discovery is finished, it will remove itself from the tracking object.
    // When both discoveries are gone from the tracking object, the results are processed
    // deduped (a bridge can be found on both discovieries) and handed over to the ThingDiscoveryInfo.

    // Tracking object
    DiscoveryJob *discovery = new DiscoveryJob();
    m_discoveries.insert(info, discovery);

    // clean up the discovery job when the ThingDiscoveryInfo is destroyed (either finished or cancelled)
    connect(info, &ThingDiscoveryInfo::destroyed, this, [this, info](){
        delete m_discoveries.take(info);
    });


    qCDebug(dcPhilipsHue()) << "Starting UPnP discovery...";
    UpnpDiscoveryReply *upnpReply = hardwareManager()->upnpDiscovery()->discoverDevices("libhue:idl");
    discovery->upnpReply = upnpReply;
    // Always clean up the upnp discovery
    connect(upnpReply, &UpnpDiscoveryReply::finished, upnpReply, &UpnpDiscoveryReply::deleteLater);

    // Process results if info is still around
    connect(upnpReply, &UpnpDiscoveryReply::finished, info, [this, upnpReply, discovery](){

        // Indicate we're done...
        discovery->upnpReply = nullptr;

        if (upnpReply->error() != UpnpDiscoveryReply::UpnpDiscoveryReplyErrorNoError) {
            qCWarning(dcPhilipsHue()) << "Upnp discovery error" << upnpReply->error();
        }

        foreach (const UpnpDeviceDescriptor &upnpDevice, upnpReply->deviceDescriptors()) {
            if (upnpDevice.modelDescription().contains("Philips")) {
                ThingDescriptor descriptor(bridgeThingClassId, "Philips Hue Bridge", upnpDevice.hostAddress().toString());
                ParamList params;
                QString bridgeId = upnpDevice.serialNumber().toLower();
                foreach (Thing *existingThing, myThings()) {
                    if (existingThing->paramValue(bridgeThingIdParamTypeId).toString() == bridgeId) {
                        descriptor.setThingId(existingThing->id());
                        break;
                    }
                }
                params.append(Param(bridgeThingHostParamTypeId, upnpDevice.hostAddress().toString()));
                params.append(Param(bridgeThingIdParamTypeId, bridgeId));
                descriptor.setParams(params);
                qCDebug(dcPhilipsHue()) << "UPnP: Found Hue bridge:" << bridgeId;
                discovery->results.append(descriptor);
            }
        }

        finishDiscovery(discovery);
    });


    qCDebug(dcPhilipsHue) << "Starting N-UPNP discovery...";
    QNetworkRequest request(QUrl("https://www.meethue.com/api/nupnp"));
    QNetworkReply *nUpnpReply = hardwareManager()->networkManager()->get(request);
    discovery->nUpnpReply = nUpnpReply;

    // Always clean up the network reply
    connect(nUpnpReply, &QNetworkReply::finished, nUpnpReply, &QNetworkReply::deleteLater);

    // Process results if info is still around
    connect(nUpnpReply, &QNetworkReply::finished, info, [this, nUpnpReply, discovery](){
        nUpnpReply->deleteLater();
        discovery->nUpnpReply = nullptr;

        if (nUpnpReply->error() != QNetworkReply::NoError) {
            qCWarning(dcPhilipsHue()) << "N-UPnP discovery failed:" << nUpnpReply->error() << nUpnpReply->errorString();
            finishDiscovery(discovery);
            return;
        }
        QJsonParseError error;
        QJsonDocument jsonDoc = QJsonDocument::fromJson(nUpnpReply->readAll(), &error);
        if (error.error != QJsonParseError::NoError) {
            qCWarning(dcPhilipsHue) << "N-UPNP discovery JSON error in response" << error.errorString();
            finishDiscovery(discovery);
            return;
        }

        foreach (const QVariant &bridgeVariant, jsonDoc.toVariant().toList()) {
            QVariantMap bridgeMap = bridgeVariant.toMap();
            ThingDescriptor descriptor(bridgeThingClassId, "Philips Hue Bridge", bridgeMap.value("internalipaddress").toString());
            ParamList params;
            QString bridgeId = bridgeMap.value("id").toString().toLower();
            // For some reason the N-UPnP api returns serial numbers with a "fffe" in the middle...
            if (bridgeId.indexOf("fffe") == 6) {
                bridgeId.remove(6, 4);
            }
            params.append(Param(bridgeThingHostParamTypeId, bridgeMap.value("internalipaddress").toString()));
            params.append(Param(bridgeThingIdParamTypeId, bridgeId));
            descriptor.setParams(params);
            qCDebug(dcPhilipsHue()) << "N-UPnP: Found Hue bridge:" << bridgeId;
            discovery->results.append(descriptor);
        }

        finishDiscovery(discovery);
    });
}

void IntegrationPluginPhilipsHue::finishDiscovery(IntegrationPluginPhilipsHue::DiscoveryJob *job)
{
    if (job->upnpReply || job->nUpnpReply) {
        // still pending...
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

    ThingDiscoveryInfo *info = m_discoveries.key(job);

    info->addThingDescriptors(results.values());
    info->finish(Thing::ThingErrorNoError);
}

void IntegrationPluginPhilipsHue::startPairing(ThingPairingInfo *info)
{
    Q_ASSERT_X(info->thingClassId() == bridgeThingClassId, "DevicePluginPhilipsHue::startPairing", "Unexpected thing class.");

    info->finish(Thing::ThingErrorNoError, QT_TR_NOOP("Please press the button on the Hue Bridge within 30 seconds before you continue"));
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

        HueBridge *bridge = new HueBridge(this);
        bridge->setId(thing->paramValue(bridgeThingIdParamTypeId).toString());
        bridge->setApiKey(apiKey);
        bridge->setHostAddress(QHostAddress(thing->paramValue(bridgeThingHostParamTypeId).toString()));
        m_bridges.insert(bridge, thing);

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

        HueBridge *bridge = m_bridges.key(myThings().findById(thing->parentId()));
        HueLight *hueLight = new HueLight(this);
        hueLight->setHostAddress(bridge->hostAddress());
        hueLight->setApiKey(bridge->apiKey());
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

        HueBridge *bridge = m_bridges.key(myThings().findById(thing->parentId()));
        HueLight *hueLight = new HueLight(this);
        hueLight->setHostAddress(bridge->hostAddress());
        hueLight->setApiKey(bridge->apiKey());
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

        HueBridge *bridge = m_bridges.key(myThings().findById(thing->parentId()));
        HueLight *hueLight = new HueLight(this);
        hueLight->setHostAddress(bridge->hostAddress());
        hueLight->setApiKey(bridge->apiKey());

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

    // Hue remote
    if (thing->thingClassId() == remoteThingClassId) {
        qCDebug(dcPhilipsHue) << "Setup Hue remote" << thing->params() << thing->thingClassId();

        HueBridge *bridge = m_bridges.key(myThings().findById(thing->parentId()));
        HueRemote *hueRemote = new HueRemote(this);
        hueRemote->setHostAddress(bridge->hostAddress());
        hueRemote->setApiKey(bridge->apiKey());

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

    // Hue tap
    if (thing->thingClassId() == tapThingClassId) {
        HueRemote *hueTap = new HueRemote(this);
        hueTap->setName(thing->name());
        hueTap->setId(thing->paramValue(tapThingSensorIdParamTypeId).toInt());
        hueTap->setModelId(thing->paramValue(tapThingModelIdParamTypeId).toString());

        connect(hueTap, &HueRemote::stateChanged, this, &IntegrationPluginPhilipsHue::remoteStateChanged);
        connect(hueTap, &HueRemote::buttonPressed, this, &IntegrationPluginPhilipsHue::onRemoteButtonEvent);

        m_remotes.insert(hueTap, thing);
        return info->finish(Thing::ThingErrorNoError);
    }

    // Hue Motion sensor
    if (thing->thingClassId() == motionSensorThingClassId) {
        qCDebug(dcPhilipsHue) << "Setup Hue motion sensor" << thing->params();

        HueIndoorSensor *motionSensor = new HueIndoorSensor(this);
        motionSensor->setTimeout(thing->setting(motionSensorSettingsTimeoutParamTypeId).toUInt() * 1000);
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
                motionSensor->setTimeout(value.toUInt() * 1000);
            }
        });

        m_motionSensors.insert(motionSensor, thing);

        return info->finish(Thing::ThingErrorNoError);
    }

    // Hue Outdoor sensor
    if (thing->thingClassId() == outdoorSensorThingClassId) {
        qCDebug(dcPhilipsHue) << "Setup Hue Outdoor sensor" << thing->params();

        HueMotionSensor *outdoorSensor = new HueOutdoorSensor(this);
        outdoorSensor->setTimeout(thing->setting(outdoorSensorSettingsTimeoutParamTypeId).toUInt() * 1000);
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
                outdoorSensor->setTimeout(value.toUInt() * 1000);
            }
        });

        m_motionSensors.insert(outdoorSensor, thing);

        return info->finish(Thing::ThingErrorNoError);
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
        HueBridge *bridge = m_bridges.key(thing);
        m_bridges.remove(bridge);
        bridge->deleteLater();
    }

    if (thing->thingClassId() == colorLightThingClassId
            || thing->thingClassId() == colorTemperatureLightThingClassId
            || thing->thingClassId() == dimmableLightThingClassId) {
        HueLight *light = m_lights.key(thing);
        m_lights.remove(light);
        light->deleteLater();
    }

    if (thing->thingClassId() == remoteThingClassId || thing->thingClassId() == tapThingClassId) {
        HueRemote *remote = m_remotes.key(thing);
        m_remotes.remove(remote);
        remote->deleteLater();
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
            info->finish(Thing::ThingErrorHardwareFailure, QT_TR_NOOP("Error connecting to hue bridge."));
            return;
        }
        QByteArray data = reply->readAll();
        QJsonParseError error;
        QJsonDocument jsonDoc = QJsonDocument::fromJson(data, &error);

        // check JSON error
        if (error.error != QJsonParseError::NoError) {
            qCWarning(dcPhilipsHue) << "Hue Bridge json error in response" << error.errorString();
            info->finish(Thing::ThingErrorHardwareFailure, QT_TR_NOOP("Received unexpected data from hue bridge."));
            return;
        }

        // check response error
        if (data.contains("error")) {
            if (!jsonDoc.toVariant().toList().isEmpty()) {
                qCWarning(dcPhilipsHue) << "Failed to pair Hue Bridge:" << jsonDoc.toVariant().toList().first().toMap().value("error").toMap().value("description").toString();
            } else {
                qCWarning(dcPhilipsHue) << "Failed to pair Hue Bridge: Invalid error message format";
            }
            info->finish(Thing::ThingErrorHardwareFailure, QT_TR_NOOP("An error happened pairing the hue bridge."));
            return;
        }

        QString apiKey = jsonDoc.toVariant().toList().first().toMap().value("success").toMap().value("username").toString();

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

void IntegrationPluginPhilipsHue::executeAction(ThingActionInfo *info)
{
    Thing *thing = info->thing();
    Action action = info->action();

    qCDebug(dcPhilipsHue) << "Execute action" << action.actionTypeId() << action.params();

    QNetworkReply *reply = nullptr;

    // lights
    if (thing->thingClassId() == colorLightThingClassId ||
            thing->thingClassId() == colorTemperatureLightThingClassId ||
            thing->thingClassId() == dimmableLightThingClassId) {

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
        } else if (action.actionTypeId() == bridgeUpgradeActionTypeId) {
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
    if (thing->thingClassId() == tapThingClassId) {
        thing->setStateValue(tapConnectedStateTypeId, remote->reachable());
    } else {
        thing->setStateValue(remoteConnectedStateTypeId, remote->reachable());
        thing->setStateValue(remoteBatteryLevelStateTypeId, remote->battery());
        thing->setStateValue(remoteBatteryCriticalStateTypeId, remote->battery() < 5);
    }
}

void IntegrationPluginPhilipsHue::onRemoteButtonEvent(int buttonCode)
{
    HueRemote *remote = static_cast<HueRemote *>(sender());

    EventTypeId id;
    Param param;

    switch (buttonCode) {
    case HueRemote::OnPressed:
        param = Param(remotePressedEventButtonNameParamTypeId, "ON");
        id = remotePressedEventTypeId;
        break;
    case HueRemote::OnLongPressed:
        param = Param(remoteLongPressedEventButtonNameParamTypeId, "ON");
        id = remoteLongPressedEventTypeId;
        break;
    case HueRemote::DimUpPressed:
        param = Param(remotePressedEventButtonNameParamTypeId, "DIM UP");
        id = remotePressedEventTypeId;
        break;
    case HueRemote::DimUpLongPressed:
        param = Param(remoteLongPressedEventButtonNameParamTypeId, "DIM UP");
        id = remoteLongPressedEventTypeId;
        break;
    case HueRemote::DimDownPressed:
        param = Param(remotePressedEventButtonNameParamTypeId, "DIM DOWN");
        id = remotePressedEventTypeId;
        break;
    case HueRemote::DimDownLongPressed:
        param = Param(remoteLongPressedEventButtonNameParamTypeId, "DIM DOWN");
        id = remoteLongPressedEventTypeId;
        break;
    case HueRemote::OffPressed:
        param = Param(remotePressedEventButtonNameParamTypeId, "OFF");
        id = remotePressedEventTypeId;
        break;
    case HueRemote::OffLongPressed:
        param = Param(remoteLongPressedEventButtonNameParamTypeId, "OFF");
        id = remoteLongPressedEventTypeId;
        break;
    case HueRemote::TapButton1Pressed:
        param = Param(tapPressedEventButtonNameParamTypeId, "•");
        id = tapPressedEventTypeId;
        break;
    case HueRemote::TapButton2Pressed:
        param = Param(tapPressedEventButtonNameParamTypeId, "••");
        id = tapPressedEventTypeId;
        break;
    case HueRemote::TapButton3Pressed:
        param = Param(tapPressedEventButtonNameParamTypeId, "•••");
        id = tapPressedEventTypeId;
        break;
    case HueRemote::TapButton4Pressed:
        param = Param(tapPressedEventButtonNameParamTypeId, "••••");
        id = tapPressedEventTypeId;
        break;
    default:
        break;
    }
    emitEvent(Event(id, m_remotes.value(remote)->id(), ParamList() << param));
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
    qCDebug(dcPhilipsHue) << "Discover bridge devices" << bridge->hostAddress();

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
    qCDebug(dcPhilipsHue) << "Discover bridge devices" << bridge->hostAddress();

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

    // Create Lights if not already added
    ThingDescriptors descriptors;

    QVariantMap lightsMap = jsonDoc.toVariant().toMap();
    QList<HueLight*> lightsToRemove = m_lights.keys();
    foreach (QString lightId, lightsMap.keys()) {
        QVariantMap lightMap = lightsMap.value(lightId).toMap();

        QString uuid = lightMap.value("uniqueid").toString();
        QString model = lightMap.value("modelid").toString();

        foreach (HueLight *light, lightsToRemove) {
            if (light->uuid() == uuid) {
                lightsToRemove.removeAll(light);
                break;
            }
        }

        if (lightAlreadyAdded(uuid))
            continue;

        if (lightMap.value("type").toString() == "Dimmable light") {
            ThingDescriptor descriptor(dimmableLightThingClassId, lightMap.value("name").toString(), "Philips Hue White Light", thing->id());
            ParamList params;
            params.append(Param(dimmableLightThingModelIdParamTypeId, model));
            params.append(Param(dimmableLightThingTypeParamTypeId, lightMap.value("type").toString()));
            params.append(Param(dimmableLightThingUuidParamTypeId, uuid));
            params.append(Param(dimmableLightThingLightIdParamTypeId, lightId));
            descriptor.setParams(params);
            descriptors.append(descriptor);

            qCDebug(dcPhilipsHue) << "Found new dimmable light" << lightMap.value("name").toString() << model;
        } else if (lightMap.value("type").toString() == "Color temperature light") {
            ThingDescriptor descriptor(colorTemperatureLightThingClassId, lightMap.value("name").toString(), "Philips Hue Color Temperature Light", thing->id());
            ParamList params;
            params.append(Param(colorTemperatureLightThingModelIdParamTypeId, model));
            params.append(Param(colorTemperatureLightThingTypeParamTypeId, lightMap.value("type").toString()));
            params.append(Param(colorTemperatureLightThingUuidParamTypeId, uuid));
            params.append(Param(colorTemperatureLightThingLightIdParamTypeId, lightId));
            descriptor.setParams(params);
            descriptors.append(descriptor);

            qCDebug(dcPhilipsHue) << "Found new color temperature light" << lightMap.value("name").toString() << model;
        } else {
            ThingDescriptor descriptor(colorLightThingClassId, lightMap.value("name").toString(), "Philips Hue Color Light", thing->id());
            ParamList params;
            params.append(Param(colorLightThingModelIdParamTypeId, model));
            params.append(Param(colorLightThingTypeParamTypeId, lightMap.value("type").toString()));
            params.append(Param(colorLightThingUuidParamTypeId, uuid));
            params.append(Param(colorLightThingLightIdParamTypeId, lightId));
            descriptor.setParams(params);
            descriptors.append(descriptor);
            qCDebug(dcPhilipsHue) << "Found new color light" << lightMap.value("name").toString() << model;
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

    // Create sensors if not already added
    QVariantMap sensorsMap = jsonDoc.toVariant().toMap();
    QHash<QString, HueMotionSensor *> motionSensors;
    QList<HueRemote*> remotesToRemove = m_remotes.keys();
    QList<HueMotionSensor*> sensorsToRemove = m_motionSensors.keys();
    foreach (const QString &sensorId, sensorsMap.keys()) {

        QVariantMap sensorMap = sensorsMap.value(sensorId).toMap();
        QString uuid = sensorMap.value("uniqueid").toString();
        QString model = sensorMap.value("modelid").toString();

        foreach (HueRemote* remote, remotesToRemove) {
            if (remote->uuid() == uuid) {
                remotesToRemove.removeAll(remote);
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

        if (sensorMap.value("type").toString() == "ZLLSwitch") {
            ThingDescriptor descriptor(remoteThingClassId, sensorMap.value("name").toString(), "Philips Hue Remote", thing->id());
            ParamList params;
            params.append(Param(remoteThingModelIdParamTypeId, model));
            params.append(Param(remoteThingTypeParamTypeId, sensorMap.value("type").toString()));
            params.append(Param(remoteThingUuidParamTypeId, uuid));
            params.append(Param(remoteThingSensorIdParamTypeId, sensorId));
            descriptor.setParams(params);
            emit autoThingsAppeared({descriptor});
            qCDebug(dcPhilipsHue) << "Found new remote" << sensorMap.value("name").toString() << model;
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
                        motionSensor = new HueIndoorSensor(this);
                    } else {
                        motionSensor = new HueOutdoorSensor(this);
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
                        motionSensor = new HueIndoorSensor(this);
                    } else {
                        motionSensor = new HueOutdoorSensor(this);
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
                        motionSensor = new HueIndoorSensor(this);
                    } else {
                        motionSensor = new HueOutdoorSensor(this);
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

    foreach (HueRemote* remote, remotesToRemove) {
        Thing *remoteThing = m_remotes.value(remote);
        if (remoteThing->parentId() == thing->id()) {
            qCDebug(dcPhilipsHue()) << "Hue remote disappeared from bridge";
            emit autoThingDisappeared(remoteThing->id());
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
    thing->setStateValue(bridgeApiVersionStateTypeId, configMap.value("apiversion").toString());
    thing->setStateValue(bridgeSoftwareVersionStateTypeId, configMap.value("swversion").toString());

    int updateStatus = configMap.value("swupdate").toMap().value("updatestate").toInt();
    switch (updateStatus) {
    case 0:
        thing->setStateValue(bridgeUpdateStatusStateTypeId, "Up to date");
        break;
    case 1:
        thing->setStateValue(bridgeUpdateStatusStateTypeId, "Downloading updates");
        break;
    case 2:
        thing->setStateValue(bridgeUpdateStatusStateTypeId, "Updates ready to install");
        break;
    case 3:
        thing->setStateValue(bridgeUpdateStatusStateTypeId, "Installing updates");
        break;
    default:
        break;
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

    if (thing->thingClassId() == colorLightThingClassId || thing->thingClassId() == dimmableLightThingClassId)
        refreshLight(thing);

}

void IntegrationPluginPhilipsHue::bridgeReachableChanged(Thing *thing, const bool &reachable)
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
                    }
                }
            }

            foreach (HueRemote *remote, m_remotes.keys()) {
                if (m_remotes.value(remote)->parentId() == thing->id()) {
                    remote->setReachable(false);
                    if (m_remotes.value(remote)->thingClassId() == remoteThingClassId) {
                        m_remotes.value(remote)->setStateValue(remoteConnectedStateTypeId, false);
                    } else if (m_remotes.value(remote)->thingClassId() == tapThingClassId) {
                        m_remotes.value(remote)->setStateValue(tapConnectedStateTypeId, false);
                    }
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
        }
        if (thing->thingClassId() == colorTemperatureLightThingClassId) {
            if (thing->paramValue(colorTemperatureLightThingUuidParamTypeId).toString() == uuid) {
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

        // Hue tap
        if (thing->thingClassId() == tapThingClassId) {
            if (thing->paramValue(tapThingUuidParamTypeId).toString() == uuid) {
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
