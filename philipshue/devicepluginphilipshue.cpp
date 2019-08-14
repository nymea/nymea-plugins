/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *                                                                         *
 *  Copyright (C) 2014 Michael Zanetti <michael_zanetti@gmx.net>           *
 *  Copyright (C) 2015 - 2019 Simon Stürz <simon.stuerz@nymea.io>          *
 *  Copyright (C) 2018 Michael Zanetti <michael.zanetti@nymea.io>          *
 *  Copyright (C) 2019 Bernhard Trinnes <bernhard.trinnes@nymea.io>        *
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

Device::DeviceError DevicePluginPhilipsHue::discoverDevices(const DeviceClassId &deviceClassId, const ParamList &params)
{
    Q_UNUSED(params)
    Q_UNUSED(deviceClassId)

    DiscoveryJob *discovery = new DiscoveryJob();

    qCDebug(dcPhilipsHue()) << "Starting UPnP discovery...";
    UpnpDiscoveryReply *upnpReply = hardwareManager()->upnpDiscovery()->discoverDevices("libhue:idl");
    discovery->upnpReply = upnpReply;
    connect(upnpReply, &UpnpDiscoveryReply::finished, this, [this, upnpReply, discovery](){
        upnpReply->deleteLater();
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
                // Not known yet...
                params.append(Param(bridgeDeviceApiKeyParamTypeId, QString()));
                params.append(Param(bridgeDeviceMacParamTypeId, QString()));
                params.append(Param(bridgeDeviceZigbeeChannelParamTypeId, -1));
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
    connect(nUpnpReply, &QNetworkReply::finished, this, [this, nUpnpReply, discovery](){
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
            params.append(Param(bridgeDeviceApiKeyParamTypeId, QString()));
            params.append(Param(bridgeDeviceMacParamTypeId, QString()));
            params.append(Param(bridgeDeviceZigbeeChannelParamTypeId, -1));
            descriptor.setParams(params);
            qCDebug(dcPhilipsHue()) << "N-UPnP: Found Hue bridge:" << bridgeId;
            discovery->results.append(descriptor);
        }
        finishDiscovery(discovery);
    });
    return Device::DeviceErrorAsync;
}

Device::DeviceSetupStatus DevicePluginPhilipsHue::setupDevice(Device *device)
{
    // Update the name on the bridge if the user changes the device name
    connect(device, &Device::nameChanged, this, &DevicePluginPhilipsHue::onDeviceNameChanged);

    // hue bridge
    if (device->deviceClassId() == bridgeDeviceClassId) {
        // unconfigured bridges (from pairing)
        foreach (HueBridge *b, m_unconfiguredBridges) {
            if (b->hostAddress().toString() == device->paramValue(bridgeDeviceHostParamTypeId).toString()) {
                m_unconfiguredBridges.removeAll(b);
                qCDebug(dcPhilipsHue) << "Setup unconfigured Hue Bridge" << b->name();
                // Set data which was not known during discovery
                device->setParamValue(bridgeDeviceApiKeyParamTypeId, b->apiKey());
                device->setParamValue(bridgeDeviceZigbeeChannelParamTypeId, b->zigbeeChannel());
                device->setParamValue(bridgeDeviceMacParamTypeId, b->macAddress());
                m_bridgeConnections.insert(device->id(), );
                device->setStateValue(bridgeConnectedStateTypeId, true);
                discoverBridgeDevices(b);
                return Device::DeviceSetupStatusSuccess;
            }
        }

        // Loaded bridge
        qCDebug(dcPhilipsHue) << "Setup Hue Bridge" << device->params();


        HueBridge *bridge = new HueBridge(this);
        bridge->setId(device->paramValue(bridgeDeviceIdParamTypeId).toString());
        bridge->setApiKey(device->paramValue(bridgeDeviceApiKeyParamTypeId).toString());
        bridge->setHostAddress(QHostAddress(device->paramValue(bridgeDeviceHostParamTypeId).toString()));
        bridge->setMacAddress(device->paramValue(bridgeDeviceMacParamTypeId).toString());
        bridge->setZigbeeChannel(device->paramValue(bridgeDeviceZigbeeChannelParamTypeId).toInt());

        BridgeConnection *bridgeConnection = new BridgeConnection(bridge, hardwareManager(), this);
        m_bridgeConnections.insert(device->id(), bridgeConnection);

        bridgeConnection->discoverBridgeDevices();
        return Device::DeviceSetupStatusSuccess;
    }

    // Hue color light
    if (device->deviceClassId() == colorLightDeviceClassId) {
        qCDebug(dcPhilipsHue) << "Setup Hue color light" << device->params();

        BridgeConnection *bridge = m_bridgeConnections;
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

        return Device::DeviceSetupStatusSuccess;
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

        return Device::DeviceSetupStatusSuccess;
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

        return Device::DeviceSetupStatusSuccess;
    }

    // Hue remote
    if (device->deviceClassId() == remoteDeviceClassId) {
        qCDebug(dcPhilipsHue) << "Setup Hue remote" << device->params() << device->deviceClassId();

        BridgeConnection *bridge = m_bridgeConnections.value(device->parentId());
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
        return Device::DeviceSetupStatusSuccess;
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
        return Device::DeviceSetupStatusSuccess;
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

        return Device::DeviceSetupStatusSuccess;
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

        return Device::DeviceSetupStatusSuccess;
    }

    qCWarning(dcPhilipsHue()) << "Unhandled setupDevice call" << device->deviceClassId();

    return Device::DeviceSetupStatusFailure;
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

    if (device->deviceClassId() == outdoorSensorDeviceClassId) {
        HueMotionSensor *outdoorSensor = m_motionSensors.key(device);
        m_motionSensors.remove(outdoorSensor);
        outdoorSensor->deleteLater();
    }
}

Device::DeviceSetupStatus DevicePluginPhilipsHue::confirmPairing(const PairingTransactionId &pairingTransactionId, const DeviceClassId &deviceClassId, const ParamList &params, const QString &secret)
{
    Q_UNUSED(secret)

    qCDebug(dcPhilipsHue()) << "Confirming pairing for transactionId" << pairingTransactionId;
    if (deviceClassId != bridgeDeviceClassId)
        return Device::DeviceSetupStatusFailure;

    PairingInfo *pairingInfo = new PairingInfo(this);
    pairingInfo->setPairingTransactionId(pairingTransactionId);
    pairingInfo->setHost(QHostAddress(params.paramValue(bridgeDeviceHostParamTypeId).toString()));

    QVariantMap deviceTypeParam;
    deviceTypeParam.insert("devicetype", "nymea");

    QJsonDocument jsonDoc = QJsonDocument::fromVariant(deviceTypeParam);

    QNetworkRequest request(QUrl("http://" + pairingInfo->host().toString() + "/api"));
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    QNetworkReply *reply = hardwareManager()->networkManager()->post(request, jsonDoc.toJson());
    connect(reply, &QNetworkReply::finished, this, &DevicePluginPhilipsHue::networkManagerReplyReady);

    m_pairingRequests.insert(reply, pairingInfo);

    return Device::DeviceSetupStatusAsync;
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
    emit devicesDiscovered(bridgeDeviceClassId, results.values());
    delete job;
}

Device::DeviceError DevicePluginPhilipsHue::executeAction(Device *device, const Action &action)
{
    qCDebug(dcPhilipsHue) << "Execute action" << action.actionTypeId() << action.params();

    // Color light
    if (device->deviceClassId() == colorLightDeviceClassId) {

        BridgeConnection* bridge = m_bridgeConnections.value(device->parentId());
        QString uuid = device->paramValue(colorLightDeviceUuidParamTypeId).toString();

        /*if (!light->reachable()) {
            qCWarning(dcPhilipsHue) << "Light" << light->name() << "not reachable";
            return Device::DeviceErrorHardwareNotAvailable;
        }*/

        if (action.actionTypeId() == colorLightPowerActionTypeId) {
            //get bridge
            bridge->setPower(uuid, action.param(colorLightPowerActionPowerParamTypeId).value().toBool())
                    //m_asyncActions.insert(device, action.id());
                    return Device::DeviceErrorAsync;
        } else if (action.actionTypeId() == colorLightColorActionTypeId) {
            bridge->setColor(uuid, action.param(colorLightColorActionColorParamTypeId).value().value<QColor>());
            //m_asyncActions.insert(reply,QPair<Device *, ActionId>(device, action.id()));
            return Device::DeviceErrorAsync;
        } else if (action.actionTypeId() == colorLightBrightnessActionTypeId) {
            bridge->setBrightness(uuid, action.param(colorLightBrightnessActionBrightnessParamTypeId).value().toInt());
            //m_asyncActions.insert(reply, QPair<Device *, ActionId>(device, action.id()));
            return Device::DeviceErrorAsync;
        } else if (action.actionTypeId() == colorLightEffectActionTypeId) {
            bridge->setEffect(uuid, action.param(colorLightEffectActionEffectParamTypeId).value().toString());
            //m_asyncActions.insert(reply, QPair<Device *, ActionId>(device, action.id()));
            return Device::DeviceErrorAsync;
        } else if (action.actionTypeId() == colorLightAlertActionTypeId) {
            bridge->setFlash(uuid, action.param(colorLightAlertActionAlertParamTypeId).value().toString());
            //m_asyncActions.insert(reply, QPair<Device *, ActionId>(device, action.id()));
            return Device::DeviceErrorAsync;
        } else if (action.actionTypeId() == colorLightColorTemperatureActionTypeId) {
            bridge->setTemperature(uuid, action.param(colorLightColorTemperatureActionColorTemperatureParamTypeId).value().toInt());
            //m_asyncActions.insert(reply, QPair<Device *, ActionId>(device, action.id()));
            return Device::DeviceErrorAsync;
        }
        return Device::DeviceErrorActionTypeNotFound;
    }

    // Color temperature light
    if (device->deviceClassId() == colorTemperatureLightDeviceClassId) {
        BridgeConnection* bridge = m_bridgeConnections.value(device->parentId());
        QString uuid = device->paramValue(colorTemperatureLightDeviceUuidParamTypeId).toString();

        /*if (!light->reachable()) {
            qCWarning(dcPhilipsHue) << "Light" << light->name() << "not reachable";
            return Device::DeviceErrorHardwareNotAvailable;
        }*/ //TODO

        if (action.actionTypeId() == colorTemperatureLightPowerActionTypeId) {
            bridge->setTemperature(uuid, action.param(colorTemperatureLightPowerActionPowerParamTypeId).value().toBool());
            //m_asyncActions.insert(reply, QPair<Device *, ActionId>(device, action.id()));
            return Device::DeviceErrorAsync;
        } else if (action.actionTypeId() == colorTemperatureLightBrightnessActionTypeId) {
            bridge->setBrightness(uuid, action.param(colorTemperatureLightBrightnessActionBrightnessParamTypeId).value().toInt());
            //m_asyncActions.insert(reply, QPair<Device *, ActionId>(device, action.id()));
            return Device::DeviceErrorAsync;
        } else if (action.actionTypeId() == colorTemperatureLightAlertActionTypeId) {
            bridge->setFlash(uuid, action.param(colorTemperatureLightAlertActionAlertParamTypeId).value().toString());
            //m_asyncActions.insert(reply, QPair<Device *, ActionId>(device, action.id()));
            return Device::DeviceErrorAsync;
        } else if (action.actionTypeId() == colorTemperatureLightColorTemperatureActionTypeId) {
            bridge->setTemperature(uuid, action.param(colorTemperatureLightColorTemperatureActionColorTemperatureParamTypeId).value().toInt());
            //m_asyncActions.insert(reply, QPair<Device *, ActionId>(device, action.id()));
            return Device::DeviceErrorAsync;
        }
        return Device::DeviceErrorActionTypeNotFound;
    }

    // Dimmable light
    if (device->deviceClassId() == dimmableLightDeviceClassId) {

        BridgeConnection* bridge = m_bridgeConnections.value(device->parentId());
        QString uuid = device->paramValue(dimmableLightDeviceUuidParamTypeId).toString();

        /*if (!light->reachable()) {
            qCWarning(dcPhilipsHue) << "Light" << light->name() << "not reachable";
            return Device::DeviceErrorHardwareNotAvailable;
        }*/ //TODO

        if (action.actionTypeId() == dimmableLightPowerActionTypeId) {
            bridge->setPower(uuid, action.param(dimmableLightPowerActionPowerParamTypeId).value().toBool());
            QNetworkReply *reply = hardwareManager()->networkManager()->put(request.first, request.second);
            connect(reply, &QNetworkReply::finished, this, &DevicePluginPhilipsHue::networkManagerReplyReady);
            //m_asyncActions.insert(reply, QPair<Device *, ActionId>(device, action.id()));
            return Device::DeviceErrorAsync;
        } else if (action.actionTypeId() == dimmableLightBrightnessActionTypeId) {
            bridge->setBrightness(uuid, action.param(dimmableLightBrightnessActionBrightnessParamTypeId).value().toInt());
            //m_asyncActions.insert(reply, QPair<Device *, ActionId>(device, action.id()));
            return Device::DeviceErrorAsync;
        } else if (action.actionTypeId() == dimmableLightAlertActionTypeId) {
            bridge->setFlash(uuid, action.param(dimmableLightAlertActionAlertParamTypeId).value().toString());
            //m_asyncActions.insert(reply, QPair<Device *, ActionId>(device, action.id()));
            return Device::DeviceErrorAsync;
        }
        return Device::DeviceErrorActionTypeNotFound;
    }

    // Hue bridge
    if (device->deviceClassId() == bridgeDeviceClassId) {
        BridgeConnection* bridge = m_bridgeConnections.value(device->id());

        if (!device->stateValue(bridgeConnectedStateTypeId).toBool()) {
            qCWarning(dcPhilipsHue) << "Bridge" << bridge->hostAddress().toString() << "not reachable";
            return Device::DeviceErrorHardwareNotAvailable;
        }

        if (action.actionTypeId() == bridgeSearchNewDevicesActionTypeId) {
            bridge->searchNewDevices(action.param(bridgeSearchNewDevicesActionSerialParamTypeId).value().toString());
            return Device::DeviceErrorNoError;
        } else if (action.actionTypeId() == bridgeCheckForUpdatesActionTypeId) {
            QPair<QNetworkRequest, QByteArray> request = bridge->createCheckUpdatesRequest();
            QNetworkReply *reply = hardwareManager()->networkManager()->put(request.first, request.second);
            connect(reply, &QNetworkReply::finished, this, &DevicePluginPhilipsHue::networkManagerReplyReady);
            m_asyncActions.insert(reply, QPair<Device *, ActionId>(device, action.id()));
            return Device::DeviceErrorAsync;
        } else if (action.actionTypeId() == bridgeUpgradeActionTypeId) {
            QPair<QNetworkRequest, QByteArray> request = bridge->createUpgradeRequest();
            QNetworkReply *reply = hardwareManager()->networkManager()->put(request.first, request.second);
            connect(reply, &QNetworkReply::finished, this, &DevicePluginPhilipsHue::networkManagerReplyReady);
            m_asyncActions.insert(reply, QPair<Device *, ActionId>(device, action.id()));
            return Device::DeviceErrorAsync;
        }
        return Device::DeviceErrorActionTypeNotFound;
    }
    return Device::DeviceErrorDeviceClassNotFound;
}

void DevicePluginPhilipsHue::onLightsDiscovered(QHash<QString, HueLight *> lights)
{
    BridgeConnection *bridge = static_cast<BridgeConnection *>(sender());
    DeviceId parentDeviceId = m_bridgeConnections.key(bridge);

    // Create Lights if not already added
    QList<DeviceDescriptor> colorLightDescriptors;
    QList<DeviceDescriptor> colorTemperatureLightDescriptors;
    QList<DeviceDescriptor> dimmableLightDescriptors;


    foreach (HueLight *light, lights.keys()) {

        if (lightAlreadyAdded(uuid))
            continue;

        if (light->type() == "Dimmable light") {
            DeviceDescriptor descriptor(dimmableLightDeviceClassId, light->name(), "Philips Hue White Light", parentDeviceId);
            ParamList params;
            params.append(Param(dimmableLightDeviceModelIdParamTypeId, model));
            params.append(Param(dimmableLightDeviceTypeParamTypeId, light->type()));
            params.append(Param(dimmableLightDeviceUuidParamTypeId, uuid));
            params.append(Param(dimmableLightDeviceLightIdParamTypeId, lightId));
            descriptor.setParams(params);
            dimmableLightDescriptors.append(descriptor);
            qCDebug(dcPhilipsHue) << "Found new dimmable light" << light->name() << light->modelId();

        } else if (light->type() == "Color temperature light") {
            DeviceDescriptor descriptor(colorTemperatureLightDeviceClassId, light->name(), "Philips Hue Color Temperature Light", parentDeviceId);
            ParamList params;
            params.append(Param(colorTemperatureLightDeviceModelIdParamTypeId, light->modelId()));
            params.append(Param(colorTemperatureLightDeviceTypeParamTypeId, light->type()));
            params.append(Param(colorTemperatureLightDeviceUuidParamTypeId, light->uuid()));
            params.append(Param(colorTemperatureLightDeviceLightIdParamTypeId, light->id()));
            descriptor.setParams(params);
            colorTemperatureLightDescriptors.append(descriptor);

            qCDebug(dcPhilipsHue) << "Found new color temperature light" << light->name() << model;
        } else {
            DeviceDescriptor descriptor(colorLightDeviceClassId, light->name(), "Philips Hue Color Light", parentDeviceId);
            ParamList params;
            params.append(Param(colorLightDeviceModelIdParamTypeId, light->modelId()));
            params.append(Param(colorLightDeviceTypeParamTypeId, light->type()));
            params.append(Param(colorLightDeviceUuidParamTypeId, light->uuid()));
            params.append(Param(colorLightDeviceLightIdParamTypeId, light->id()));
            descriptor.setParams(params);
            colorLightDescriptors.append(descriptor);
            qCDebug(dcPhilipsHue) << "Found new color light" << light->name() << model;
        }
    }

    if (!colorLightDescriptors.isEmpty())
        emit autoDevicesAppeared(colorLightDeviceClassId, colorLightDescriptors);
    if (!colorTemperatureLightDescriptors.isEmpty())
        emit autoDevicesAppeared(colorTemperatureLightDeviceClassId, colorTemperatureLightDescriptors);
    if (!dimmableLightDescriptors.isEmpty())
        emit autoDevicesAppeared(dimmableLightDeviceClassId, dimmableLightDescriptors);
}

void DevicePluginPhilipsHue::onRemotesDiscovered(QHash<QString, HueRemote *> remotes)
{
    BridgeConnection *bridge = static_cast<BridgeConnection *>(sender());
    DeviceId parentDeviceId = m_bridgeConnections.key(bridge);

    foreach (HueRemote *remote, remotes.values()) {
        QString baseUuid = motionSensors.key(motionSensor);
        if (sensorAlreadyAdded(uuid))
            continue;

        if (remote->type() == "ZLLSwitch") {
            DeviceDescriptor descriptor(remoteDeviceClassId, remote->name(), "Philips Hue Remote", parentDeviceId);
            ParamList params;
            params.append(Param(remoteDeviceModelIdParamTypeId, remote->modelId()));
            params.append(Param(remoteDeviceTypeParamTypeId, remote->type()));
            params.append(Param(remoteDeviceUuidParamTypeId, remote->uuid()));
            params.append(Param(remoteDeviceSensorIdParamTypeId, remote->sensorId()));
            descriptor.setParams(params);
            emit autoDevicesAppeared(remoteDeviceClassId, {descriptor});
        }

        if (remote->type() == "ZGPSwitch") {
            DeviceDescriptor descriptor(tapDeviceClassId, remote->name(), "Philips Hue Tap", device->id());
            ParamList params;
            params.append(Param(tapDeviceUuidParamTypeId, remote->uuid()));
            params.append(Param(tapDeviceModelIdParamTypeId, remote->modelId()));
            params.append(Param(tapDeviceSensorIdParamTypeId, remote->sensorId()));
            descriptor.setParams(params);
            emit autoDevicesAppeared(tapDeviceClassId, {descriptor});
        }
    }
}

void DevicePluginPhilipsHue::onMotionSensorDiscovered(QHash<QString, HueMotionSensor *> motionSensors)
{
    // Create outdoor sensors if there are any new sensors found
    foreach (HueMotionSensor *motionSensor, motionSensors.values()) {
        QString baseUuid = motionSensors.key(motionSensor);
        if (sensorAlreadyAdded(uuid))
            continue;

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
                qCDebug(dcPhilipsHue()) << "Found new motion sensor" << baseUuid << outdoorSensorDeviceClassId;
                emit autoDevicesAppeared(motionSensorDeviceClassId, {descriptor});
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
                emit autoDevicesAppeared(outdoorSensorDeviceClassId, {descriptor});
            }
        }
    }
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
