/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *                                                                         *
 *  Copyright (C) 2014 Michael Zanetti <michael_zanetti@gmx.net>           *
 *  Copyright (C) 2015 - 2019 Simon Stürz <simon.stuerz@guh.io>            *
 *  Copyright (C) 2018 Michael Zanetti <michael.zanetti@guh.io>            *
 *                                                                         *
 *  This file is part of nymea.                                            *
 *                                                                         *
 *  This library is free software; you can redistribute it and/or          *
 *  modify it under the terms of the GNU Lesser General Public             *
 *  License as published by the Free Software Foundation; either           *
 *  version 2.1 of the License, or (at your option) any later version.     *
 *                                                                         *
 *  This library is distributed in the hope that it will be useful,        *
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of         *
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU      *
 *  Lesser General Public License for more details.                        *
 *                                                                         *
 *  You should have received a copy of the GNU Lesser General Public       *
 *  License along with this library; If not, see                           *
 *  <http://www.gnu.org/licenses/>.                                        *
 *                                                                         *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

#include "devicepluginphilipshue.h"

#include "devices/device.h"
#include "types/param.h"
#include "plugininfo.h"
#include "network/upnp/upnpdiscovery.h"
#include "network/upnp/upnpdiscoveryreply.h"

#include <QDebug>
#include <QColor>
#include <QDateTime>
#include <QStringList>
#include <QJsonDocument>

DevicePluginPhilipsHue::DevicePluginPhilipsHue()
{

}

DevicePluginPhilipsHue::~DevicePluginPhilipsHue()
{
    hardwareManager()->pluginTimerManager()->unregisterTimer(m_pluginTimer1Sec);
    hardwareManager()->pluginTimerManager()->unregisterTimer(m_pluginTimer5Sec);
    hardwareManager()->pluginTimerManager()->unregisterTimer(m_pluginTimer15Sec);
}

void DevicePluginPhilipsHue::init()
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
        foreach (Device *device, m_bridges.values()) {
            refreshBridge(device);
        }
    });

}

void DevicePluginPhilipsHue::discoverDevices(DeviceDiscoveryInfo *info)
{
    // We're starting a UpnpDiscovery and a NUpnpDiscovery.
    // For that, we create a tracking object holding pointers to both of those discoveries.
    // Both discoveries add their results to a temporary list.
    // Once a discovery is finished, it will remove itself from the tracking object.
    // When both discoveries are gone from the tracking object, the results are processed
    // deduped (a bridge can be found on both discovieries) and handed over to the DeviceDiscoveryInfo.

    // Tracking object
    DiscoveryJob *discovery = new DiscoveryJob();
    m_discoveries.insert(info, discovery);

    // clean up the discovery job when the DeviceDiscoveryInfo is destroyed (either finished or cancelled)
    connect(info, &DeviceDiscoveryInfo::destroyed, this, [this, info](){
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
                DeviceDescriptor descriptor(bridgeDeviceClassId, "Philips Hue Bridge", upnpDevice.hostAddress().toString());
                ParamList params;
                QString bridgeId = upnpDevice.serialNumber().toLower();
                foreach (Device *existingDevice, myDevices()) {
                    if (existingDevice->paramValue(bridgeDeviceIdParamTypeId).toString() == bridgeId) {
                        descriptor.setDeviceId(existingDevice->id());
                        break;
                    }
                }
                params.append(Param(bridgeDeviceHostParamTypeId, upnpDevice.hostAddress().toString()));
                params.append(Param(bridgeDeviceIdParamTypeId, bridgeId));
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
            DeviceDescriptor descriptor(bridgeDeviceClassId, "Philips Hue Bridge", bridgeMap.value("internalipaddress").toString());
            ParamList params;
            QString bridgeId = bridgeMap.value("id").toString().toLower();
            // For some reason the N-UPnP api returns serial numbers with a "fffe" in the middle...
            if (bridgeId.indexOf("fffe") == 6) {
                bridgeId.remove(6, 4);
            }
            params.append(Param(bridgeDeviceHostParamTypeId, bridgeMap.value("internalipaddress").toString()));
            params.append(Param(bridgeDeviceIdParamTypeId, bridgeId));
            descriptor.setParams(params);
            qCDebug(dcPhilipsHue()) << "N-UPnP: Found Hue bridge:" << bridgeId;
            discovery->results.append(descriptor);
        }

        finishDiscovery(discovery);
    });
}

void DevicePluginPhilipsHue::finishDiscovery(DevicePluginPhilipsHue::DiscoveryJob *job)
{
    if (job->upnpReply || job->nUpnpReply) {
        // still pending...
        return;
    }
    QHash<QString, DeviceDescriptor> results;
    foreach (DeviceDescriptor result, job->results) {
        // dedupe
        QString bridgeId = result.params().paramValue(bridgeDeviceIdParamTypeId).toString();
        if (results.contains(bridgeId)) {
            qCDebug(dcPhilipsHue()) << "Discarding duplicate search result" << bridgeId;
            continue;
        }
        Device *dev = bridgeForBridgeId(bridgeId);
        if (dev) {
            qCDebug(dcPhilipsHue()) << "Bridge already added in system:" << bridgeId;
            result.setDeviceId(dev->id());
        }
        results.insert(bridgeId, result);

    }

    DeviceDiscoveryInfo *info = m_discoveries.key(job);

    info->addDeviceDescriptors(results.values());
    info->finish(Device::DeviceErrorNoError);
}

void DevicePluginPhilipsHue::startPairing(DevicePairingInfo *info)
{
    Q_ASSERT_X(info->deviceClassId() == bridgeDeviceClassId, "DevicePluginPhilipsHue::startPairing", "Unexpected device class.");

    info->finish(Device::DeviceErrorNoError, QT_TR_NOOP("Please press the button on the Hue Bridge within 30 seconds before you continue"));
}

void DevicePluginPhilipsHue::setupDevice(DeviceSetupInfo *info)
{
    Device *device = info->device();

    // Update the name on the bridge if the user changes the device name
    connect(device, &Device::nameChanged, this, &DevicePluginPhilipsHue::onDeviceNameChanged);

    // hue bridge
    if (device->deviceClassId() == bridgeDeviceClassId) {
        // Loaded bridge
        qCDebug(dcPhilipsHue) << "Setup Hue Bridge" << device->params();

        pluginStorage()->beginGroup(device->id().toString());
        QString apiKey = pluginStorage()->value("apiKey").toString();
        pluginStorage()->endGroup();

        // For legacy reasons we might not have the api key in the pluginstorage yet. Check if there is a key in the device params.
        if (apiKey.isEmpty()) {
            qCWarning(dcPhilipsHue()) << "Loading api key from device params!";
            // Used to be in json, not any more.
            ParamTypeId bridgeDeviceApiKeyParamTypeId = ParamTypeId("{8bf5776a-d5a6-4600-8b27-481f0d803a8f}");
            apiKey = device->paramValue(bridgeDeviceApiKeyParamTypeId).toString();
        }

        if (apiKey.isEmpty()) {
            info->finish(Device::DeviceErrorAuthenticationFailure, QT_TR_NOOP("Not authenticated to bridge. Please reconfigure the device."));
            return;
        }

        HueBridge *bridge = new HueBridge(this);
        bridge->setId(device->paramValue(bridgeDeviceIdParamTypeId).toString());
        bridge->setApiKey(apiKey);
        bridge->setHostAddress(QHostAddress(device->paramValue(bridgeDeviceHostParamTypeId).toString()));
        m_bridges.insert(bridge, device);

        discoverBridgeDevices(bridge);
        return info->finish(Device::DeviceErrorNoError);
    }

    // Hue color light
    if (device->deviceClassId() == colorLightDeviceClassId) {
        qCDebug(dcPhilipsHue) << "Setup Hue color light" << device->params();

        HueBridge *bridge = m_bridges.key(myDevices().findById(device->parentId()));
        HueLight *hueLight = new HueLight(this);
        hueLight->setHostAddress(bridge->hostAddress());
        hueLight->setApiKey(bridge->apiKey());
        hueLight->setId(device->paramValue(colorLightDeviceLightIdParamTypeId).toInt());
        hueLight->setModelId(device->paramValue(colorLightDeviceModelIdParamTypeId).toString());
        hueLight->setUuid(device->paramValue(colorLightDeviceUuidParamTypeId).toString());
        hueLight->setType(device->paramValue(colorLightDeviceTypeParamTypeId).toString());

        connect(hueLight, &HueLight::stateChanged, this, &DevicePluginPhilipsHue::lightStateChanged);
        m_lights.insert(hueLight, device);

        refreshLight(device);

        return info->finish(Device::DeviceErrorNoError);
    }

    // Hue color temperature light
    if (device->deviceClassId() == colorTemperatureLightDeviceClassId) {
        qCDebug(dcPhilipsHue) << "Setup Hue color temperature light" << device->params();

        HueBridge *bridge = m_bridges.key(myDevices().findById(device->parentId()));
        HueLight *hueLight = new HueLight(this);
        hueLight->setHostAddress(bridge->hostAddress());
        hueLight->setApiKey(bridge->apiKey());
        hueLight->setId(device->paramValue(colorTemperatureLightDeviceLightIdParamTypeId).toInt());
        hueLight->setModelId(device->paramValue(colorTemperatureLightDeviceModelIdParamTypeId).toString());
        hueLight->setUuid(device->paramValue(colorTemperatureLightDeviceUuidParamTypeId).toString());
        hueLight->setType(device->paramValue(colorTemperatureLightDeviceTypeParamTypeId).toString());

        connect(hueLight, &HueLight::stateChanged, this, &DevicePluginPhilipsHue::lightStateChanged);
        m_lights.insert(hueLight, device);

        refreshLight(device);

        return info->finish(Device::DeviceErrorNoError);
    }

    // Hue white light
    if (device->deviceClassId() == dimmableLightDeviceClassId) {
        qCDebug(dcPhilipsHue) << "Setup Hue white light" << device->params();

        HueBridge *bridge = m_bridges.key(myDevices().findById(device->parentId()));
        HueLight *hueLight = new HueLight(this);
        hueLight->setHostAddress(bridge->hostAddress());
        hueLight->setApiKey(bridge->apiKey());

        // Migrate device parameters after changing param type UUIDs in 0.14.
        QMap<QString, ParamTypeId> migrationMap;
        migrationMap.insert("095a463b-f59e-46b1-989a-a71f9cbe3e30", dimmableLightDeviceModelIdParamTypeId);
        migrationMap.insert("3f3467ef-4483-4eb9-bcae-84e628322f84", dimmableLightDeviceTypeParamTypeId);
        migrationMap.insert("1a5129ca-006c-446c-9f2e-79b065de715f", dimmableLightDeviceUuidParamTypeId);
        migrationMap.insert("491dc012-ccf2-4d3a-9f18-add98f7374af", dimmableLightDeviceLightIdParamTypeId);

        ParamList migratedParams;
        foreach (const Param &oldParam, device->params()) {
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
        device->setParams(migratedParams);
        // Migration done

        hueLight->setModelId(device->paramValue(dimmableLightDeviceModelIdParamTypeId).toString());
        hueLight->setType(device->paramValue(dimmableLightDeviceTypeParamTypeId).toString());
        hueLight->setUuid(device->paramValue(dimmableLightDeviceUuidParamTypeId).toString());
        hueLight->setId(device->paramValue(dimmableLightDeviceLightIdParamTypeId).toInt());

        connect(hueLight, &HueLight::stateChanged, this, &DevicePluginPhilipsHue::lightStateChanged);

        m_lights.insert(hueLight, device);
        refreshLight(device);

        return info->finish(Device::DeviceErrorNoError);
    }

    // Hue remote
    if (device->deviceClassId() == remoteDeviceClassId) {
        qCDebug(dcPhilipsHue) << "Setup Hue remote" << device->params() << device->deviceClassId();

        HueBridge *bridge = m_bridges.key(myDevices().findById(device->parentId()));
        HueRemote *hueRemote = new HueRemote(this);
        hueRemote->setHostAddress(bridge->hostAddress());
        hueRemote->setApiKey(bridge->apiKey());

        // Migrate device parameters after changing param type UUIDs in 0.14.
        QMap<QString, ParamTypeId> migrationMap;
        migrationMap.insert("095a463b-f59e-46b1-989a-a71f9cbe3e30", remoteDeviceModelIdParamTypeId);
        migrationMap.insert("3f3467ef-4483-4eb9-bcae-84e628322f84", remoteDeviceTypeParamTypeId);
        migrationMap.insert("1a5129ca-006c-446c-9f2e-79b065de715f", remoteDeviceUuidParamTypeId);

        ParamList migratedParams;
        foreach (const Param &oldParam, device->params()) {
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
        device->setParams(migratedParams);
        // Migration done


        hueRemote->setId(device->paramValue(remoteDeviceSensorIdParamTypeId).toInt());
        hueRemote->setModelId(device->paramValue(remoteDeviceModelIdParamTypeId).toString());
        hueRemote->setType(device->paramValue(remoteDeviceTypeParamTypeId).toString());
        hueRemote->setUuid(device->paramValue(remoteDeviceUuidParamTypeId).toString());

        connect(hueRemote, &HueRemote::stateChanged, this, &DevicePluginPhilipsHue::remoteStateChanged);
        connect(hueRemote, &HueRemote::buttonPressed, this, &DevicePluginPhilipsHue::onRemoteButtonEvent);

        m_remotes.insert(hueRemote, device);
        return info->finish(Device::DeviceErrorNoError);
    }

    // Hue tap
    if (device->deviceClassId() == tapDeviceClassId) {
        HueRemote *hueTap = new HueRemote(this);
        hueTap->setName(device->name());
        hueTap->setId(device->paramValue(tapDeviceSensorIdParamTypeId).toInt());
        hueTap->setModelId(device->paramValue(tapDeviceModelIdParamTypeId).toString());

        connect(hueTap, &HueRemote::stateChanged, this, &DevicePluginPhilipsHue::remoteStateChanged);
        connect(hueTap, &HueRemote::buttonPressed, this, &DevicePluginPhilipsHue::onRemoteButtonEvent);

        m_remotes.insert(hueTap, device);
        return info->finish(Device::DeviceErrorNoError);
    }

    // Hue Motion sensor
    if (device->deviceClassId() == motionSensorDeviceClassId) {
        qCDebug(dcPhilipsHue) << "Setup Hue motion sensor" << device->params();

        HueIndoorSensor *motionSensor = new HueIndoorSensor(this);
        motionSensor->setTimeout(device->setting(motionSensorSettingsTimeoutParamTypeId).toUInt());
        motionSensor->setUuid(device->paramValue(motionSensorDeviceUuidParamTypeId).toString());
        motionSensor->setModelId(device->paramValue(motionSensorDeviceModelIdParamTypeId).toString());
        motionSensor->setTemperatureSensorId(device->paramValue(motionSensorDeviceSensorIdTemperatureParamTypeId).toInt());
        motionSensor->setTemperatureSensorUuid(device->paramValue(motionSensorDeviceSensorUuidTemperatureParamTypeId).toString());
        motionSensor->setPresenceSensorId(device->paramValue(motionSensorDeviceSensorIdPresenceParamTypeId).toInt());
        motionSensor->setPresenceSensorUuid(device->paramValue(motionSensorDeviceSensorUuidPresenceParamTypeId).toString());
        motionSensor->setLightSensorId(device->paramValue(motionSensorDeviceSensorIdLightParamTypeId).toInt());
        motionSensor->setLightSensorUuid(device->paramValue(motionSensorDeviceSensorUuidLightParamTypeId).toString());

        connect(motionSensor, &HueMotionSensor::reachableChanged, this, &DevicePluginPhilipsHue::onMotionSensorReachableChanged);
        connect(motionSensor, &HueMotionSensor::batteryLevelChanged, this, &DevicePluginPhilipsHue::onMotionSensorBatteryLevelChanged);
        connect(motionSensor, &HueMotionSensor::temperatureChanged, this, &DevicePluginPhilipsHue::onMotionSensorTemperatureChanged);
        connect(motionSensor, &HueMotionSensor::presenceChanged, this, &DevicePluginPhilipsHue::onMotionSensorPresenceChanged);
        connect(motionSensor, &HueMotionSensor::lightIntensityChanged, this, &DevicePluginPhilipsHue::onMotionSensorLightIntensityChanged);

        connect(device, &Device::settingChanged, motionSensor, [motionSensor](const ParamTypeId &paramTypeId, const QVariant &value){
            if (paramTypeId == motionSensorSettingsTimeoutParamTypeId) {
                motionSensor->setTimeout(value.toUInt());
            }
        });

        m_motionSensors.insert(motionSensor, device);

        return info->finish(Device::DeviceErrorNoError);
    }

    // Hue Outdoor sensor
    if (device->deviceClassId() == outdoorSensorDeviceClassId) {
        qCDebug(dcPhilipsHue) << "Setup Hue Outdoor sensor" << device->params();

        HueMotionSensor *outdoorSensor = new HueOutdoorSensor(this);
        outdoorSensor->setTimeout(device->setting(outdoorSensorSettingsTimeoutParamTypeId).toUInt());
        outdoorSensor->setUuid(device->paramValue(outdoorSensorDeviceUuidParamTypeId).toString());
        outdoorSensor->setModelId(device->paramValue(outdoorSensorDeviceModelIdParamTypeId).toString());
        outdoorSensor->setTemperatureSensorId(device->paramValue(outdoorSensorDeviceSensorIdTemperatureParamTypeId).toInt());
        outdoorSensor->setTemperatureSensorUuid(device->paramValue(outdoorSensorDeviceSensorUuidTemperatureParamTypeId).toString());
        outdoorSensor->setPresenceSensorId(device->paramValue(outdoorSensorDeviceSensorIdPresenceParamTypeId).toInt());
        outdoorSensor->setPresenceSensorUuid(device->paramValue(outdoorSensorDeviceSensorUuidPresenceParamTypeId).toString());
        outdoorSensor->setLightSensorId(device->paramValue(outdoorSensorDeviceSensorIdLightParamTypeId).toInt());
        outdoorSensor->setLightSensorUuid(device->paramValue(outdoorSensorDeviceSensorUuidLightParamTypeId).toString());

        connect(outdoorSensor, &HueMotionSensor::reachableChanged, this, &DevicePluginPhilipsHue::onMotionSensorReachableChanged);
        connect(outdoorSensor, &HueMotionSensor::batteryLevelChanged, this, &DevicePluginPhilipsHue::onMotionSensorBatteryLevelChanged);
        connect(outdoorSensor, &HueMotionSensor::temperatureChanged, this, &DevicePluginPhilipsHue::onMotionSensorTemperatureChanged);
        connect(outdoorSensor, &HueMotionSensor::presenceChanged, this, &DevicePluginPhilipsHue::onMotionSensorPresenceChanged);
        connect(outdoorSensor, &HueMotionSensor::lightIntensityChanged, this, &DevicePluginPhilipsHue::onMotionSensorLightIntensityChanged);

        connect(device, &Device::settingChanged, outdoorSensor, [outdoorSensor](const ParamTypeId &paramTypeId, const QVariant &value){
            if (paramTypeId == outdoorSensorSettingsTimeoutParamTypeId) {
                outdoorSensor->setTimeout(value.toUInt());
            }
        });

        m_motionSensors.insert(outdoorSensor, device);

        return info->finish(Device::DeviceErrorNoError);
    }

    qCWarning(dcPhilipsHue()) << "Unhandled setupDevice call" << device->deviceClassId();
}

void DevicePluginPhilipsHue::deviceRemoved(Device *device)
{
    abortRequests(m_lightRefreshRequests, device);
    abortRequests(m_setNameRequests, device);
    abortRequests(m_bridgeRefreshRequests, device);
    abortRequests(m_lightsRefreshRequests, device);
    abortRequests(m_sensorsRefreshRequests, device);
    abortRequests(m_bridgeLightsDiscoveryRequests, device);
    abortRequests(m_bridgeSensorsDiscoveryRequests, device);
    abortRequests(m_bridgeSearchDevicesRequests, device);

    if (device->deviceClassId() == bridgeDeviceClassId) {
        HueBridge *bridge = m_bridges.key(device);
        m_bridges.remove(bridge);
        bridge->deleteLater();
    }

    if (device->deviceClassId() == colorLightDeviceClassId
            || device->deviceClassId() == colorTemperatureLightDeviceClassId
            || device->deviceClassId() == dimmableLightDeviceClassId) {
        HueLight *light = m_lights.key(device);
        m_lights.remove(light);
        light->deleteLater();
    }

    if (device->deviceClassId() == remoteDeviceClassId || device->deviceClassId() == tapDeviceClassId) {
        HueRemote *remote = m_remotes.key(device);
        m_remotes.remove(remote);
        remote->deleteLater();
    }

    if (device->deviceClassId() == outdoorSensorDeviceClassId || device->deviceClassId() == motionSensorDeviceClassId) {
        HueMotionSensor *motionSensor = m_motionSensors.key(device);
        m_motionSensors.remove(motionSensor);
        motionSensor->deleteLater();
    }
}

void DevicePluginPhilipsHue::confirmPairing(DevicePairingInfo *info, const QString &username, const QString &secret)
{
    Q_UNUSED(username)
    Q_UNUSED(secret)

    QVariantMap deviceTypeParam;
    deviceTypeParam.insert("devicetype", "nymea");

    QJsonDocument jsonDoc = QJsonDocument::fromVariant(deviceTypeParam);

    QString host = info->params().paramValue(bridgeDeviceHostParamTypeId).toString();
    QNetworkRequest request(QUrl("http://" + host + "/api"));
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    QNetworkReply *reply = hardwareManager()->networkManager()->post(request, jsonDoc.toJson());
    connect(reply, &QNetworkReply::finished, reply, &QNetworkReply::deleteLater);

    connect(reply, &QNetworkReply::finished, info, [this, info, reply](){
        if (reply->error() != QNetworkReply::NoError) {
            info->finish(Device::DeviceErrorHardwareFailure, QT_TR_NOOP("Error connecting to hue bridge."));
            return;
        }
        QByteArray data = reply->readAll();
        QJsonParseError error;
        QJsonDocument jsonDoc = QJsonDocument::fromJson(data, &error);

        // check JSON error
        if (error.error != QJsonParseError::NoError) {
            qCWarning(dcPhilipsHue) << "Hue Bridge json error in response" << error.errorString();
            info->finish(Device::DeviceErrorHardwareFailure, QT_TR_NOOP("Received unexpected data from hue bridge."));
            return;
        }

        // check response error
        if (data.contains("error")) {
            if (!jsonDoc.toVariant().toList().isEmpty()) {
                qCWarning(dcPhilipsHue) << "Failed to pair Hue Bridge:" << jsonDoc.toVariant().toList().first().toMap().value("error").toMap().value("description").toString();
            } else {
                qCWarning(dcPhilipsHue) << "Failed to pair Hue Bridge: Invalid error message format";
            }
            info->finish(Device::DeviceErrorHardwareFailure, QT_TR_NOOP("An error happened pairing the hue bridge."));
            return;
        }

        QString apiKey = jsonDoc.toVariant().toList().first().toMap().value("success").toMap().value("username").toString();

        if (apiKey.isEmpty()) {
            qCWarning(dcPhilipsHue) << "Failed to pair Hue Bridge: did not get any key from the bridge";
            return info->finish(Device::DeviceErrorAuthenticationFailure, QT_TR_NOOP("The hue bridge has rejected the connection request."));
        }

        qCDebug(dcPhilipsHue) << "Got api key from bridge:" << apiKey;

        // All good. Store the API key
        pluginStorage()->beginGroup(info->deviceId().toString());
        pluginStorage()->setValue("apiKey", apiKey);
        pluginStorage()->endGroup();

        info->finish(Device::DeviceErrorNoError);
    });
}

void DevicePluginPhilipsHue::networkManagerReplyReady()
{
    QNetworkReply *reply = static_cast<QNetworkReply *>(sender());
    reply->deleteLater();

    int status = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();

    //    qCDebug(dcPhilipsHue()) << "Hue reply:" << status << reply->error() << reply->errorString();

    // create user finished
    if (m_bridgeLightsDiscoveryRequests.contains(reply)) {
        Device *device = m_bridgeLightsDiscoveryRequests.take(reply);

        // check HTTP status code
        if (status != 200 || reply->error() != QNetworkReply::NoError) {
            qCWarning(dcPhilipsHue) << "Bridge light discovery error:" << status << reply->errorString();
            bridgeReachableChanged(device, false);
            return;
        }
        processBridgeLightDiscoveryResponse(device, reply->readAll());

    } else if (m_bridgeSensorsDiscoveryRequests.contains(reply)) {
        Device *device = m_bridgeSensorsDiscoveryRequests.take(reply);

        // check HTTP status code
        if (status != 200 || reply->error() != QNetworkReply::NoError) {
            qCWarning(dcPhilipsHue) << "Bridge sensor discovery error:" << status << reply->errorString();
            bridgeReachableChanged(device, false);
            return;
        }
        processBridgeSensorDiscoveryResponse(device, reply->readAll());

    } else if (m_bridgeSearchDevicesRequests.contains(reply)) {
        Device *device = m_bridgeSearchDevicesRequests.take(reply);

        // check HTTP status code
        if (status != 200 || reply->error() != QNetworkReply::NoError) {
            qCWarning(dcPhilipsHue) << "Bridge search new devices error:" << status << reply->errorString();
            bridgeReachableChanged(device, false);
            return;
        }
        discoverBridgeDevices(m_bridges.key(device));

    } else if (m_bridgeRefreshRequests.contains(reply)) {
        Device *device = m_bridgeRefreshRequests.take(reply);

        // check HTTP status code
        if (status != 200 || reply->error() != QNetworkReply::NoError) {
            if (device->stateValue(bridgeConnectedStateTypeId).toBool()) {
                qCWarning(dcPhilipsHue) << "Refresh Hue Bridge request error:" << status << reply->errorString();
                bridgeReachableChanged(device, false);
            }
            return;
        }
        processBridgeRefreshResponse(device, reply->readAll());

    } else if (m_lightRefreshRequests.contains(reply)) {
        Device *device = m_lightRefreshRequests.take(reply);

        // check HTTP status code
        if (status != 200 || reply->error() != QNetworkReply::NoError) {
            qCWarning(dcPhilipsHue) << "Refresh Hue Light request error:" << status << reply->errorString();
            bridgeReachableChanged(device, false);
            return;
        }
        processLightRefreshResponse(device, reply->readAll());

    } else if (m_lightsRefreshRequests.contains(reply)) {
        Device *device = m_lightsRefreshRequests.take(reply);

        // check HTTP status code
        if (status != 200 || reply->error() != QNetworkReply::NoError) {
            if (device->stateValue(bridgeConnectedStateTypeId).toBool()) {
                qCWarning(dcPhilipsHue) << "Refresh Hue lights request error:" << status << reply->errorString();
                bridgeReachableChanged(device, false);
            }
            return;
        }
        processLightsRefreshResponse(device, reply->readAll());

    } else if (m_sensorsRefreshRequests.contains(reply)) {
        Device *device = m_sensorsRefreshRequests.take(reply);

        // check HTTP status code
        if (status != 200 || reply->error() != QNetworkReply::NoError) {
            if (device->stateValue(bridgeConnectedStateTypeId).toBool()) {
                qCWarning(dcPhilipsHue) << "Refresh Hue sensors request error:" << status << reply->errorString();
                bridgeReachableChanged(device, false);
            }
            return;
        }
        processSensorsRefreshResponse(device, reply->readAll());

    } else if (m_setNameRequests.contains(reply)) {
        Device *device = m_setNameRequests.take(reply);

        // check HTTP status code
        if (status != 200 || reply->error() != QNetworkReply::NoError) {
            qCWarning(dcPhilipsHue) << "Set name of Hue Light request error:" << status << reply->errorString();
            bridgeReachableChanged(device, false);
            return;
        }
        processSetNameResponse(device, reply->readAll());
    } else {
        qCWarning(dcPhilipsHue()) << "Unhandled bridge reply" << reply->error() << reply->readAll();
    }
}

void DevicePluginPhilipsHue::onDeviceNameChanged()
{
    Device *device = static_cast<Device*>(sender());
    if (m_lights.values().contains(device)) {
        setLightName(device);
    }

    if (m_remotes.values().contains(device)) {
        setRemoteName(device);
    }
}

void DevicePluginPhilipsHue::executeAction(DeviceActionInfo *info)
{
    Device *device = info->device();
    Action action = info->action();

    qCDebug(dcPhilipsHue) << "Execute action" << action.actionTypeId() << action.params();

    QNetworkReply *reply = nullptr;

    // lights
    if (device->deviceClassId() == colorLightDeviceClassId ||
            device->deviceClassId() == colorTemperatureLightDeviceClassId ||
            device->deviceClassId() == dimmableLightDeviceClassId) {

        HueLight *light = m_lights.key(device);

        if (!light->reachable()) {
            qCWarning(dcPhilipsHue) << "Light" << light->name() << "not reachable";
            return info->finish(Device::DeviceErrorHardwareNotAvailable);
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
    if (device->deviceClassId() == bridgeDeviceClassId) {
        HueBridge *bridge = m_bridges.key(device);
        if (!device->stateValue(bridgeConnectedStateTypeId).toBool()) {
            qCWarning(dcPhilipsHue) << "Bridge" << bridge->hostAddress().toString() << "not reachable";
            return info->finish(Device::DeviceErrorHardwareNotAvailable);
        }

        if (action.actionTypeId() == bridgeSearchNewDevicesActionTypeId) {
            searchNewDevices(bridge, action.param(bridgeSearchNewDevicesActionSerialParamTypeId).value().toString());
            return info->finish(Device::DeviceErrorNoError);
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
        info->finish(Device::DeviceErrorUnsupportedFeature);
        return;
    }

    // Always clean up the reply when it finishes
    connect(reply, &QNetworkReply::finished, reply, &QNetworkReply::deleteLater);

    // Handle response if info is still around
    connect(reply, &QNetworkReply::finished, info, [this, info, reply](){
        if (reply->error() != QNetworkReply::NoError) {
            info->finish(Device::DeviceErrorHardwareFailure, QT_TR_NOOP("Error sending command to hue bridge."));
            return;
        }

        QByteArray data = reply->readAll();
        QJsonParseError error;
        QJsonDocument jsonDoc = QJsonDocument::fromJson(data, &error);

        if (error.error != QJsonParseError::NoError) {
            qCWarning(dcPhilipsHue) << "Hue Bridge json error in response" << error.errorString();
            info->finish(Device::DeviceErrorHardwareFailure, QT_TR_NOOP("Received unexpected data from hue bridge."));
            return;
        }

        if (data.contains("error")) {
            if (!jsonDoc.toVariant().toList().isEmpty()) {
                qCWarning(dcPhilipsHue) << "Failed to execute Hue action:" << jsonDoc.toJson(); //jsonDoc.toVariant().toList().first().toMap().value("error").toMap().value("description").toString();
            } else {
                qCWarning(dcPhilipsHue) << "Failed to execute Hue action: Invalid error message format";
            }
            info->finish(Device::DeviceErrorHardwareFailure, QT_TR_NOOP("An unexpected error happened when sending the command to the hue bridge."));
            return;
        }

        if (info->device()->deviceClassId() != bridgeDeviceClassId) {
            m_lights.key(info->device())->processActionResponse(jsonDoc.toVariant().toList());
        }

        info->finish(Device::DeviceErrorNoError);
    });
}

void DevicePluginPhilipsHue::lightStateChanged()
{
    HueLight *light = static_cast<HueLight *>(sender());

    Device *device = m_lights.value(light);
    if (!device) {
        qCWarning(dcPhilipsHue) << "Could not find device for light" << light->name();
        return;
    }

    if (device->deviceClassId() == colorLightDeviceClassId) {
        device->setStateValue(colorLightConnectedStateTypeId, light->reachable());
        device->setStateValue(colorLightColorStateTypeId, QVariant::fromValue(light->color()));
        device->setStateValue(colorLightPowerStateTypeId, light->power());
        device->setStateValue(colorLightBrightnessStateTypeId, brightnessToPercentage(light->brightness()));
        device->setStateValue(colorLightColorTemperatureStateTypeId, light->ct());
        device->setStateValue(colorLightEffectStateTypeId, light->effect());
    } else if (device->deviceClassId() == colorTemperatureLightDeviceClassId) {
        device->setStateValue(colorTemperatureLightConnectedStateTypeId, light->reachable());
        device->setStateValue(colorTemperatureLightPowerStateTypeId, light->power());
        device->setStateValue(colorTemperatureLightBrightnessStateTypeId, brightnessToPercentage(light->brightness()));
        device->setStateValue(colorTemperatureLightColorTemperatureStateTypeId, light->ct());
    } else if (device->deviceClassId() == dimmableLightDeviceClassId) {
        device->setStateValue(dimmableLightConnectedStateTypeId, light->reachable());
        device->setStateValue(dimmableLightPowerStateTypeId, light->power());
        device->setStateValue(dimmableLightBrightnessStateTypeId, brightnessToPercentage(light->brightness()));
    }
}

void DevicePluginPhilipsHue::remoteStateChanged()
{
    HueRemote *remote = static_cast<HueRemote *>(sender());

    Device *device = m_remotes.value(remote);
    if (!device) {
        qCWarning(dcPhilipsHue) << "Could not find device for remote" << remote->name();
        return;
    }
    if (device->deviceClassId() == tapDeviceClassId) {
        device->setStateValue(tapConnectedStateTypeId, remote->reachable());
    } else {
        device->setStateValue(remoteConnectedStateTypeId, remote->reachable());
        device->setStateValue(remoteBatteryLevelStateTypeId, remote->battery());
        device->setStateValue(remoteBatteryCriticalStateTypeId, remote->battery() < 5);
    }
}

void DevicePluginPhilipsHue::onRemoteButtonEvent(int buttonCode)
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

void DevicePluginPhilipsHue::onMotionSensorReachableChanged(bool reachable)
{
    HueMotionSensor *sensor = static_cast<HueMotionSensor *>(sender());
    Device *sensorDevice = m_motionSensors.value(sensor);
    sensorDevice->setStateValue(sensor->connectedStateTypeId(), reachable);
}

void DevicePluginPhilipsHue::onMotionSensorBatteryLevelChanged(int batteryLevel)
{
    HueMotionSensor *sensor = static_cast<HueMotionSensor *>(sender());
    Device *sensorDevice = m_motionSensors.value(sensor);
    sensorDevice->setStateValue(sensor->batteryLevelStateTypeId(), batteryLevel);
    sensorDevice->setStateValue(sensor->batteryCriticalStateTypeId(), (batteryLevel < 5));
}

void DevicePluginPhilipsHue::onMotionSensorTemperatureChanged(double temperature)
{
    HueMotionSensor *sensor = static_cast<HueMotionSensor *>(sender());
    Device *sensorDevice = m_motionSensors.value(sensor);
    sensorDevice->setStateValue(sensor->temperatureStateTypeId(), temperature);
}

void DevicePluginPhilipsHue::onMotionSensorPresenceChanged(bool presence)
{
    HueMotionSensor *sensor = static_cast<HueMotionSensor *>(sender());
    Device *sensorDevice = m_motionSensors.value(sensor);
    sensorDevice->setStateValue(sensor->isPresentStateTypeId(), presence);
    if (presence) sensorDevice->setStateValue(sensor->lastSeenTimeStateTypeId(), QDateTime::currentDateTime().toTime_t());
}

void DevicePluginPhilipsHue::onMotionSensorLightIntensityChanged(double lightIntensity)
{
    HueMotionSensor *sensor = static_cast<HueMotionSensor *>(sender());
    Device *sensorDevice = m_motionSensors.value(sensor);
    sensorDevice->setStateValue(sensor->lightIntensityStateTypeId(), lightIntensity);
}

void DevicePluginPhilipsHue::refreshLight(Device *device)
{
    HueLight *light = m_lights.key(device);

    QNetworkRequest request(QUrl("http://" + light->hostAddress().toString() + "/api/" + light->apiKey() + "/lights/" + QString::number(light->id())));
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    QNetworkReply *reply = hardwareManager()->networkManager()->get(request);
    connect(reply, &QNetworkReply::finished, this, &DevicePluginPhilipsHue::networkManagerReplyReady);
    m_lightRefreshRequests.insert(reply, device);
}

void DevicePluginPhilipsHue::refreshBridge(Device *device)
{
    HueBridge *bridge = m_bridges.key(device);
    //    qCDebug(dcPhilipsHue()) << "refreshing bridge";

    QNetworkRequest request(QUrl("http://" + bridge->hostAddress().toString() + "/api/" + bridge->apiKey() + "/config"));
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    QNetworkReply *reply = hardwareManager()->networkManager()->get(request);
    connect(reply, &QNetworkReply::finished, this, &DevicePluginPhilipsHue::networkManagerReplyReady);
    m_bridgeRefreshRequests.insert(reply, device);
}

void DevicePluginPhilipsHue::refreshLights(HueBridge *bridge)
{
    Device *device = m_bridges.value(bridge);
    //    qCDebug(dcPhilipsHue()) << "refreshing lights";

    QNetworkRequest request(QUrl("http://" + bridge->hostAddress().toString() + "/api/" + bridge->apiKey() + "/lights"));
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    QNetworkReply *reply = hardwareManager()->networkManager()->get(request);
    connect(reply, &QNetworkReply::finished, this, &DevicePluginPhilipsHue::networkManagerReplyReady);
    m_lightsRefreshRequests.insert(reply, device);
}

void DevicePluginPhilipsHue::refreshSensors(HueBridge *bridge)
{
    Device *device = m_bridges.value(bridge);
    //    qCDebug(dcPhilipsHue()) << "refreshing sensors";

    QNetworkRequest request(QUrl("http://" + bridge->hostAddress().toString() + "/api/" + bridge->apiKey() + "/sensors"));
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    QNetworkReply *reply = hardwareManager()->networkManager()->get(request);
    connect(reply, &QNetworkReply::finished, this, &DevicePluginPhilipsHue::networkManagerReplyReady);
    m_sensorsRefreshRequests.insert(reply, device);
}

void DevicePluginPhilipsHue::discoverBridgeDevices(HueBridge *bridge)
{
    Device *device = m_bridges.value(bridge);
    qCDebug(dcPhilipsHue) << "Discover bridge devices" << bridge->hostAddress();

    QPair<QNetworkRequest, QByteArray> lightsRequest = bridge->createDiscoverLightsRequest();
    QNetworkReply *lightsReply = hardwareManager()->networkManager()->get(lightsRequest.first);
    connect(lightsReply, &QNetworkReply::finished, this, &DevicePluginPhilipsHue::networkManagerReplyReady);
    m_bridgeLightsDiscoveryRequests.insert(lightsReply, device);

    QPair<QNetworkRequest, QByteArray> sensorsRequest = bridge->createSearchSensorsRequest();
    QNetworkReply *reply = hardwareManager()->networkManager()->get(sensorsRequest.first);
    connect(reply, &QNetworkReply::finished, this, &DevicePluginPhilipsHue::networkManagerReplyReady);
    m_bridgeSensorsDiscoveryRequests.insert(reply, device);
}

void DevicePluginPhilipsHue::searchNewDevices(HueBridge *bridge, const QString &serialNumber)
{
    Device *device = m_bridges.value(bridge);
    qCDebug(dcPhilipsHue) << "Discover bridge devices" << bridge->hostAddress();

    QPair<QNetworkRequest, QByteArray> request = bridge->createSearchLightsRequest(serialNumber);
    QNetworkReply *reply = hardwareManager()->networkManager()->post(request.first, request.second);
    connect(reply, &QNetworkReply::finished, this, &DevicePluginPhilipsHue::networkManagerReplyReady);
    m_bridgeSearchDevicesRequests.insert(reply, device);
}

void DevicePluginPhilipsHue::setLightName(Device *device)
{
    HueLight *light = m_lights.key(device);

    QVariantMap requestMap;
    requestMap.insert("name", device->name());
    QJsonDocument jsonDoc = QJsonDocument::fromVariant(requestMap);

    QNetworkRequest request(QUrl("http://" + light->hostAddress().toString() + "/api/" + light->apiKey() +
                                 "/lights/" + QString::number(light->id())));
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

    QNetworkReply *reply = hardwareManager()->networkManager()->put(request,jsonDoc.toJson());
    connect(reply, &QNetworkReply::finished, this, &DevicePluginPhilipsHue::networkManagerReplyReady);
    m_setNameRequests.insert(reply, device);
}

void DevicePluginPhilipsHue::setRemoteName(Device *device)
{
    HueRemote *remote = m_remotes.key(device);

    QVariantMap requestMap;
    requestMap.insert("name", device->name());
    QJsonDocument jsonDoc = QJsonDocument::fromVariant(requestMap);

    QNetworkRequest request(QUrl("http://" + remote->hostAddress().toString() + "/api/" + remote->apiKey() +
                                 "/sensors/" + QString::number(remote->id())));
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

    QNetworkReply *reply = hardwareManager()->networkManager()->put(request,jsonDoc.toJson());
    connect(reply, &QNetworkReply::finished, this, &DevicePluginPhilipsHue::networkManagerReplyReady);
    m_setNameRequests.insert(reply, device);
}

void DevicePluginPhilipsHue::processBridgeLightDiscoveryResponse(Device *device, const QByteArray &data)
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
    DeviceDescriptors descriptors;

    QVariantMap lightsMap = jsonDoc.toVariant().toMap();
    foreach (QString lightId, lightsMap.keys()) {
        QVariantMap lightMap = lightsMap.value(lightId).toMap();

        QString uuid = lightMap.value("uniqueid").toString();
        QString model = lightMap.value("modelid").toString();

        if (lightAlreadyAdded(uuid))
            continue;

        if (lightMap.value("type").toString() == "Dimmable light") {
            DeviceDescriptor descriptor(dimmableLightDeviceClassId, lightMap.value("name").toString(), "Philips Hue White Light", device->id());
            ParamList params;
            params.append(Param(dimmableLightDeviceModelIdParamTypeId, model));
            params.append(Param(dimmableLightDeviceTypeParamTypeId, lightMap.value("type").toString()));
            params.append(Param(dimmableLightDeviceUuidParamTypeId, uuid));
            params.append(Param(dimmableLightDeviceLightIdParamTypeId, lightId));
            descriptor.setParams(params);
            descriptors.append(descriptor);

            qCDebug(dcPhilipsHue) << "Found new dimmable light" << lightMap.value("name").toString() << model;
        } else if (lightMap.value("type").toString() == "Color temperature light") {
            DeviceDescriptor descriptor(colorTemperatureLightDeviceClassId, lightMap.value("name").toString(), "Philips Hue Color Temperature Light", device->id());
            ParamList params;
            params.append(Param(colorTemperatureLightDeviceModelIdParamTypeId, model));
            params.append(Param(colorTemperatureLightDeviceTypeParamTypeId, lightMap.value("type").toString()));
            params.append(Param(colorTemperatureLightDeviceUuidParamTypeId, uuid));
            params.append(Param(colorTemperatureLightDeviceLightIdParamTypeId, lightId));
            descriptor.setParams(params);
            descriptors.append(descriptor);

            qCDebug(dcPhilipsHue) << "Found new color temperature light" << lightMap.value("name").toString() << model;
        } else {
            DeviceDescriptor descriptor(colorLightDeviceClassId, lightMap.value("name").toString(), "Philips Hue Color Light", device->id());
            ParamList params;
            params.append(Param(colorLightDeviceModelIdParamTypeId, model));
            params.append(Param(colorLightDeviceTypeParamTypeId, lightMap.value("type").toString()));
            params.append(Param(colorLightDeviceUuidParamTypeId, uuid));
            params.append(Param(colorLightDeviceLightIdParamTypeId, lightId));
            descriptor.setParams(params);
            descriptors.append(descriptor);
            qCDebug(dcPhilipsHue) << "Found new color light" << lightMap.value("name").toString() << model;
        }
    }

    if (!descriptors.isEmpty()) {
        emit autoDevicesAppeared(descriptors);
    }
}

void DevicePluginPhilipsHue::processBridgeSensorDiscoveryResponse(Device *device, const QByteArray &data)
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
    foreach (const QString &sensorId, sensorsMap.keys()) {

        QVariantMap sensorMap = sensorsMap.value(sensorId).toMap();
        QString uuid = sensorMap.value("uniqueid").toString();
        QString model = sensorMap.value("modelid").toString();

        if (sensorAlreadyAdded(uuid))
            continue;

        if (sensorMap.value("type").toString() == "ZLLSwitch") {
            DeviceDescriptor descriptor(remoteDeviceClassId, sensorMap.value("name").toString(), "Philips Hue Remote", device->id());
            ParamList params;
            params.append(Param(remoteDeviceModelIdParamTypeId, model));
            params.append(Param(remoteDeviceTypeParamTypeId, sensorMap.value("type").toString()));
            params.append(Param(remoteDeviceUuidParamTypeId, uuid));
            params.append(Param(remoteDeviceSensorIdParamTypeId, sensorId));
            descriptor.setParams(params);
            emit autoDevicesAppeared({descriptor});
            qCDebug(dcPhilipsHue) << "Found new remote" << sensorMap.value("name").toString() << model;
        } else if (sensorMap.value("type").toString() == "ZGPSwitch") {
            DeviceDescriptor descriptor(tapDeviceClassId, sensorMap.value("name").toString(), "Philips Hue Tap", device->id());
            ParamList params;
            params.append(Param(tapDeviceUuidParamTypeId, uuid));
            params.append(Param(tapDeviceModelIdParamTypeId, model));
            params.append(Param(tapDeviceSensorIdParamTypeId, sensorId));
            descriptor.setParams(params);
            emit autoDevicesAppeared({descriptor});
            qCDebug(dcPhilipsHue()) << "Found hue tap:" << sensorMap << tapDeviceClassId;

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
                DeviceDescriptor descriptor(motionSensorDeviceClassId, tr("Philips Hue Motion sensor"), baseUuid, device->id());
                ParamList params;
                params.append(Param(motionSensorDeviceUuidParamTypeId, motionSensor->uuid()));
                params.append(Param(motionSensorDeviceModelIdParamTypeId, motionSensor->modelId()));
                params.append(Param(motionSensorDeviceSensorUuidTemperatureParamTypeId, motionSensor->temperatureSensorUuid()));
                params.append(Param(motionSensorDeviceSensorIdTemperatureParamTypeId, motionSensor->temperatureSensorId()));
                params.append(Param(motionSensorDeviceSensorUuidPresenceParamTypeId, motionSensor->presenceSensorUuid()));
                params.append(Param(motionSensorDeviceSensorIdPresenceParamTypeId, motionSensor->presenceSensorId()));
                params.append(Param(motionSensorDeviceSensorUuidLightParamTypeId, motionSensor->lightSensorUuid()));
                params.append(Param(motionSensorDeviceSensorIdLightParamTypeId, motionSensor->lightSensorId()));
                descriptor.setParams(params);
                qCDebug(dcPhilipsHue()) << "Found new motion sensor" << baseUuid << motionSensorDeviceClassId;
                emit autoDevicesAppeared({descriptor});
            } else if (motionSensor->modelId() == "SML002") {
                DeviceDescriptor descriptor(outdoorSensorDeviceClassId, tr("Philips Hue Outdoor sensor"), baseUuid, device->id());
                ParamList params;
                params.append(Param(outdoorSensorDeviceUuidParamTypeId, motionSensor->uuid()));
                params.append(Param(outdoorSensorDeviceModelIdParamTypeId, motionSensor->modelId()));
                params.append(Param(outdoorSensorDeviceSensorUuidTemperatureParamTypeId, motionSensor->temperatureSensorUuid()));
                params.append(Param(outdoorSensorDeviceSensorIdTemperatureParamTypeId, motionSensor->temperatureSensorId()));
                params.append(Param(outdoorSensorDeviceSensorUuidPresenceParamTypeId, motionSensor->presenceSensorUuid()));
                params.append(Param(outdoorSensorDeviceSensorIdPresenceParamTypeId, motionSensor->presenceSensorId()));
                params.append(Param(outdoorSensorDeviceSensorUuidLightParamTypeId, motionSensor->lightSensorUuid()));
                params.append(Param(outdoorSensorDeviceSensorIdLightParamTypeId, motionSensor->lightSensorId()));
                descriptor.setParams(params);
                qCDebug(dcPhilipsHue()) << "Found new outdoor sensor" << baseUuid << outdoorSensorDeviceClassId;
                emit autoDevicesAppeared({descriptor});
            }
        }

        // Clean up
        motionSensors.remove(baseUuid);
        motionSensor->deleteLater();
    }
}

void DevicePluginPhilipsHue::processLightRefreshResponse(Device *device, const QByteArray &data)
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

    HueLight *light = m_lights.key(device);
    light->updateStates(jsonDoc.toVariant().toMap().value("state").toMap());
}

void DevicePluginPhilipsHue::processBridgeRefreshResponse(Device *device, const QByteArray &data)
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
        bridgeReachableChanged(device, false);
        return;
    }

    //qCDebug(dcPhilipsHue()) << "Bridge refresh response" << qUtf8Printable(jsonDoc.toJson(QJsonDocument::Indented));

    QVariantMap configMap = jsonDoc.toVariant().toMap();

    // mark bridge as reachable
    bridgeReachableChanged(device, true);
    device->setStateValue(bridgeApiVersionStateTypeId, configMap.value("apiversion").toString());
    device->setStateValue(bridgeSoftwareVersionStateTypeId, configMap.value("swversion").toString());

    int updateStatus = configMap.value("swupdate").toMap().value("updatestate").toInt();
    switch (updateStatus) {
    case 0:
        device->setStateValue(bridgeUpdateStatusStateTypeId, "Up to date");
        break;
    case 1:
        device->setStateValue(bridgeUpdateStatusStateTypeId, "Downloading updates");
        break;
    case 2:
        device->setStateValue(bridgeUpdateStatusStateTypeId, "Updates ready to install");
        break;
    case 3:
        device->setStateValue(bridgeUpdateStatusStateTypeId, "Installing updates");
        break;
    default:
        break;
    }
}

void DevicePluginPhilipsHue::processLightsRefreshResponse(Device *device, const QByteArray &data)
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
            if (light->id() == lightId.toInt() && m_lights.value(light)->parentId() == device->id()) {
                light->updateStates(lightMap.value("state").toMap());
            }
        }
    }
}

void DevicePluginPhilipsHue::processSensorsRefreshResponse(Device *device, const QByteArray &data)
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
            if (remote->id() == sensorId.toInt() && m_remotes.value(remote)->parentId() == device->id()) {
                remote->updateStates(sensorMap.value("state").toMap(), sensorMap.value("config").toMap());
            }
        }

        // Motion sensors
        foreach (HueMotionSensor *motionSensor, m_motionSensors.keys()) {
            if (motionSensor->hasSensor(sensorId.toInt()) && m_motionSensors.value(motionSensor)->parentId() == device->id()) {
                motionSensor->updateStates(sensorMap);
            }
        }
    }
}

void DevicePluginPhilipsHue::processSetNameResponse(Device *device, const QByteArray &data)
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

    if (device->deviceClassId() == colorLightDeviceClassId || device->deviceClassId() == dimmableLightDeviceClassId)
        refreshLight(device);

}

void DevicePluginPhilipsHue::bridgeReachableChanged(Device *device, const bool &reachable)
{
    if (reachable) {
        device->setStateValue(bridgeConnectedStateTypeId, true);
    } else {
        // mark bridge and corresponding hue devices unreachable
        if (device->deviceClassId() == bridgeDeviceClassId) {
            device->setStateValue(bridgeConnectedStateTypeId, false);

            foreach (HueLight *light, m_lights.keys()) {
                if (m_lights.value(light)->parentId() == device->id()) {
                    light->setReachable(false);
                    if (m_lights.value(light)->deviceClassId() == colorLightDeviceClassId) {
                        m_lights.value(light)->setStateValue(colorLightConnectedStateTypeId, false);
                    } else if (m_lights.value(light)->deviceClassId() == colorTemperatureLightDeviceClassId) {
                        m_lights.value(light)->setStateValue(colorTemperatureLightConnectedStateTypeId, false);
                    } else if (m_lights.value(light)->deviceClassId() == dimmableLightDeviceClassId) {
                        m_lights.value(light)->setStateValue(dimmableLightConnectedStateTypeId, false);
                    }
                }
            }

            foreach (HueRemote *remote, m_remotes.keys()) {
                if (m_remotes.value(remote)->parentId() == device->id()) {
                    remote->setReachable(false);
                    if (m_remotes.value(remote)->deviceClassId() == remoteDeviceClassId) {
                        m_remotes.value(remote)->setStateValue(remoteConnectedStateTypeId, false);
                    } else if (m_remotes.value(remote)->deviceClassId() == tapDeviceClassId) {
                        m_remotes.value(remote)->setStateValue(tapConnectedStateTypeId, false);
                    }
                }
            }

            foreach (HueMotionSensor *motionSensor, m_motionSensors.keys()) {
                if (m_motionSensors.value(motionSensor)->parentId() == device->id()) {
                    motionSensor->setReachable(false);
                    m_motionSensors.value(motionSensor)->setStateValue(motionSensor->connectedStateTypeId(), false);
                }
            }
        }
    }
}

Device* DevicePluginPhilipsHue::bridgeForBridgeId(const QString &id)
{
    foreach (Device *device, myDevices()) {
        if (device->deviceClassId() == bridgeDeviceClassId) {
            if (device->paramValue(bridgeDeviceIdParamTypeId).toString().toLower() == id) {
                return device;
            }
        }
    }
    return nullptr;
}

bool DevicePluginPhilipsHue::lightAlreadyAdded(const QString &uuid)
{
    foreach (Device *device, myDevices()) {
        if (device->deviceClassId() == colorLightDeviceClassId) {
            if (device->paramValue(colorLightDeviceUuidParamTypeId).toString() == uuid) {
                return true;
            }
        } else if (device->deviceClassId() == dimmableLightDeviceClassId) {
            if (device->paramValue(dimmableLightDeviceUuidParamTypeId).toString() == uuid) {
                return true;
            }
        }
        if (device->deviceClassId() == colorTemperatureLightDeviceClassId) {
            if (device->paramValue(colorTemperatureLightDeviceUuidParamTypeId).toString() == uuid) {
                return true;
            }
        }
    }
    return false;
}

bool DevicePluginPhilipsHue::sensorAlreadyAdded(const QString &uuid)
{
    foreach (Device *device, myDevices()) {
        // Hue remote
        if (device->deviceClassId() == remoteDeviceClassId) {
            if (device->paramValue(remoteDeviceUuidParamTypeId).toString() == uuid) {
                return true;
            }
        }

        // Hue tap
        if (device->deviceClassId() == tapDeviceClassId) {
            if (device->paramValue(tapDeviceUuidParamTypeId).toString() == uuid) {
                return true;
            }
        }

        // Outdoor sensor consits out of 3 sensors
        if (device->deviceClassId() == outdoorSensorDeviceClassId) {
            if (device->paramValue(outdoorSensorDeviceSensorUuidLightParamTypeId).toString() == uuid) {
                return true;
            } else if (device->paramValue(outdoorSensorDeviceSensorUuidPresenceParamTypeId).toString() == uuid) {
                return true;
            } else if (device->paramValue(outdoorSensorDeviceSensorUuidTemperatureParamTypeId).toString() == uuid) {
                return true;
            }
        }
        // Motion sensor consits out of 3 sensors
        if (device->deviceClassId() == motionSensorDeviceClassId) {
            if (device->paramValue(motionSensorDeviceSensorUuidLightParamTypeId).toString() == uuid) {
                return true;
            } else if (device->paramValue(motionSensorDeviceSensorUuidPresenceParamTypeId).toString() == uuid) {
                return true;
            } else if (device->paramValue(motionSensorDeviceSensorUuidTemperatureParamTypeId).toString() == uuid) {
                return true;
            }
        }
    }

    return false;
}

int DevicePluginPhilipsHue::brightnessToPercentage(int brightness)
{
    return qRound((100.0 * brightness) / 255.0);
}

int DevicePluginPhilipsHue::percentageToBrightness(int percentage)
{
    return qRound((255.0 * percentage) / 100.0);
}

void DevicePluginPhilipsHue::abortRequests(QHash<QNetworkReply *, Device *> requestList, Device *device)
{
    foreach (QNetworkReply* reply, requestList.keys()) {
        if (requestList.value(reply) == device) {
            reply->abort();
        }
    }
}
