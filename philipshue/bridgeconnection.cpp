/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *                                                                         *
 *  Copyright (C) 2014 Michael Zanetti <michael_zanetti@gmx.net>           *
 *  Copyright (C) 2015 - 2019 Simon Stürz <simon.stuerz@nymea.io>          *
 *  Copyright (C) 2019 Bernhard Trinns <bernhard.trinnes@nymea.io>         *
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


#include "bridgeconnection.h"
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


BridgeConnection::BridgeConnection(HueBridge *bridge, HardwareManager *hardwareManager, QObject *parent) :
    QObject(parent),
    m_hardwareManager(hardwareManager),
    m_bridge(bridge)
{
    m_bridge = new HueBridge(this);

}

BridgeConnection::~BridgeConnection()
{
    m_hardwareManager->pluginTimerManager()->unregisterTimer(m_pluginTimer1Sec);
    m_hardwareManager->pluginTimerManager()->unregisterTimer(m_pluginTimer5Sec);
    m_hardwareManager->pluginTimerManager()->unregisterTimer(m_pluginTimer15Sec);
}

void BridgeConnection::init()
{
    m_pluginTimer1Sec = m_hardwareManager->pluginTimerManager()->registerTimer(1);
    connect(m_pluginTimer1Sec, &PluginTimer::timeout, this, [this]() {
        // refresh sensors every second
        if (m_remotes.isEmpty()) {
            return;
        }
        refreshSensors();
    });
    m_pluginTimer5Sec = m_hardwareManager->pluginTimerManager()->registerTimer(5);
    connect(m_pluginTimer5Sec, &PluginTimer::timeout, this, [this]() {
        // refresh lights every 5 seconds
        refreshLights();

    });
    m_pluginTimer15Sec = m_hardwareManager->pluginTimerManager()->registerTimer(15);
    connect(m_pluginTimer15Sec, &PluginTimer::timeout, this, [this]() {
        // refresh bridges every 15 seconds
        refreshBridge();
        // search for new devices connected to the bridge
        discoverBridgeDevices();
    });
}

void BridgeConnection::refreshLight(const QString &uuid)
{
    HueLight *light = m_lights.value(uuid);

    QNetworkRequest request(QUrl("http://" + light->hostAddress().toString() + "/api/" + light->apiKey() + "/lights/" + QString::number(light->id())));
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    QNetworkReply *reply = m_hardwareManager->networkManager()->get(request);
    connect(reply, &QNetworkReply::finished, this, &BridgeConnection::networkManagerReplyReady);

    //m_lightRefreshRequests.insert(reply);
}

void BridgeConnection::refreshBridge()
{
    //    qCDebug(dcPhilipsHue()) << "refreshing bridge";

    QNetworkRequest request(QUrl("http://" + m_bridge->hostAddress().toString() + "/api/" + m_bridge->apiKey() + "/config"));
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    QNetworkReply *reply = m_hardwareManager->networkManager()->get(request);
    connect(reply, &QNetworkReply::finished, this, &BridgeConnection::networkManagerReplyReady);
    //TODO abort old request and start new one
    //m_bridgeRefreshRequests.insert(reply);
}

void BridgeConnection::refreshLights()
{
    //    qCDebug(dcPhilipsHue()) << "refreshing lights";
    QNetworkRequest request(QUrl("http://" + m_bridge->hostAddress().toString() + "/api/" + m_bridge->apiKey() + "/lights"));
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    QNetworkReply *reply = m_hardwareManager->networkManager()->get(request);
    connect(reply, &QNetworkReply::finished, this, &BridgeConnection::networkManagerReplyReady);
    //m_lightsRefreshRequests.insert(reply, device);
}

void BridgeConnection::refreshSensors()
{
    //    qCDebug(dcPhilipsHue()) << "refreshing sensors";

    QNetworkRequest request(QUrl("http://" + m_bridge->hostAddress().toString() + "/api/" + m_bridge->apiKey() + "/sensors"));
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    QNetworkReply *reply = m_hardwareManager->networkManager()->get(request);
    connect(reply, &QNetworkReply::finished, this, &BridgeConnection::networkManagerReplyReady);
    //m_sensorsRefreshRequests.insert(reply, device);
}

void BridgeConnection::discoverBridgeDevices()
{
    qCDebug(dcPhilipsHue) << "Discover bridge devices" << m_bridge->hostAddress();

    QPair<QNetworkRequest, QByteArray> lightsRequest = m_bridge->createDiscoverLightsRequest();
    QNetworkReply *lightsReply = m_hardwareManager->networkManager()->get(lightsRequest.first);
    connect(lightsReply, &QNetworkReply::finished, this, &BridgeConnection::networkManagerReplyReady);
    //m_bridgeLightsDiscoveryRequests.insert(lightsReply, device);

    QPair<QNetworkRequest, QByteArray> sensorsRequest = m_bridge->createSearchSensorsRequest();
    QNetworkReply *reply = m_hardwareManager->networkManager()->get(sensorsRequest.first);
    connect(reply, &QNetworkReply::finished, this, &BridgeConnection::networkManagerReplyReady);
    //m_bridgeSensorsDiscoveryRequests.insert(reply, device);
}

void BridgeConnection::searchNewDevices(const QString &serialNumber)
{
    qCDebug(dcPhilipsHue) << "Discover bridge devices" << m_bridge->hostAddress();

    QPair<QNetworkRequest, QByteArray> request = m_bridge->createSearchLightsRequest(serialNumber);
    QNetworkReply *reply = m_hardwareManager->networkManager()->post(request.first, request.second);
    connect(reply, &QNetworkReply::finished, this, &BridgeConnection::networkManagerReplyReady);
    //m_bridgeSearchDevicesRequests.insert(reply, device);
}

void BridgeConnection::setPower(const QString &uuid, bool power)
{
    HueLight *light = m_lights.value(uuid);
    QPair<QNetworkRequest, QByteArray> request = light->createSetPowerRequest(power);
    QNetworkReply *reply = m_hardwareManager->networkManager()->put(request.first, request.second);
    connect(reply, &QNetworkReply::finished, this, &BridgeConnection::networkManagerReplyReady);
}

void BridgeConnection::setBrightness(const QString &uuid, quint8 percent)
{
    HueLight *light = m_lights.value(uuid);
    QPair<QNetworkRequest, QByteArray> request = light->createSetBrightnessRequest(percent);
    QNetworkReply *reply = m_hardwareManager->networkManager()->put(request.first, request.second);
    connect(reply, &QNetworkReply::finished, this, &BridgeConnection::networkManagerReplyReady);
}

void BridgeConnection::setColor(const QString &uuid, QColor color)
{
    HueLight *light = m_lights.value(uuid);
    QPair<QNetworkRequest, QByteArray> request = light->createSetColorRequest(color);
    QNetworkReply *reply = m_hardwareManager->networkManager()->put(request.first, request.second);
    connect(reply, &QNetworkReply::finished, this, &BridgeConnection::networkManagerReplyReady);
}

void BridgeConnection::setEffect(const QString &uuid, const QString &effect)
{
    HueLight *light = m_lights.value(uuid);
    QPair<QNetworkRequest, QByteArray> request = light->createSetEffectRequest(effect);
    QNetworkReply *reply = m_hardwareManager->networkManager()->put(request.first, request.second);
    connect(reply, &QNetworkReply::finished, this, &BridgeConnection::networkManagerReplyReady);
}

void BridgeConnection::setTemperature(const QString &uuid, quint16 temperature)
{
    HueLight *light = m_lights.value(uuid);
    QPair<QNetworkRequest, QByteArray> request = light->createSetTemperatureRequest(temperature);
    QNetworkReply *reply = m_hardwareManager->networkManager()->put(request.first, request.second);
    connect(reply, &QNetworkReply::finished, this, &BridgeConnection::networkManagerReplyReady);
}

void BridgeConnection::setFlash(const QString &uuid, const QString &mode)
{
    HueLight *light = m_lights.value(uuid);
    QPair<QNetworkRequest, QByteArray> request = light->createFlashRequest(mode);
    QNetworkReply *reply = m_hardwareManager->networkManager()->put(request.first, request.second);
    connect(reply, &QNetworkReply::finished, this, &BridgeConnection::networkManagerReplyReady);
}



void BridgeConnection::setLightName(const QString &uuid, const QString &name)
{
    HueLight *light = m_lights.value(uuid);

    QVariantMap requestMap;
    requestMap.insert("name", name);
    QJsonDocument jsonDoc = QJsonDocument::fromVariant(requestMap);

    QNetworkRequest request(QUrl("http://" + light->hostAddress().toString() + "/api/" + light->apiKey() +
                                 "/lights/" + QString::number(light->id())));
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

    QNetworkReply *reply = m_hardwareManager->networkManager()->put(request,jsonDoc.toJson());
    connect(reply, &QNetworkReply::finished, this, &BridgeConnection::networkManagerReplyReady);
    //m_setNameRequests.insert(reply, device);
}

void BridgeConnection::setRemoteName(const QString &uuid, const QString &name)
{
    HueRemote *remote = m_remotes.balue(uuid);

    QVariantMap requestMap;
    requestMap.insert("name", device->name());
    QJsonDocument jsonDoc = QJsonDocument::fromVariant(requestMap);

    QNetworkRequest request(QUrl("http://" + remote->hostAddress().toString() + "/api/" + remote->apiKey() +
                                 "/sensors/" + QString::number(remote->id())));
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

    QNetworkReply *reply = m_hardwareManager->networkManager()->put(request,jsonDoc.toJson());
    connect(reply, &QNetworkReply::finished, this, &BridgeConnection::networkManagerReplyReady);
    //m_setNameRequests.insert(reply, device);
}

void BridgeConnection::processLightRefreshResponse(QString uuid, const QByteArray &data)
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

    HueLight *light = m_lights.value(uuid);
    light->updateStates(jsonDoc.toVariant().toMap().value("state").toMap());
}

void BridgeConnection::processBridgeRefreshResponse(const QByteArray &data)
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
        bridgeReachableChanged(false);
        return;
    }

    //qCDebug(dcPhilipsHue()) << "Bridge refresh response" << qUtf8Printable(jsonDoc.toJson(QJsonDocument::Indented));

    QVariantMap configMap = jsonDoc.toVariant().toMap();

    // mark bridge as reachable
    bridgeReachableChanged(true);
    //device->setStateValue(bridgeApiVersionStateTypeId, configMap.value("apiversion").toString());
    //device->setStateValue(bridgeSoftwareVersionStateTypeId, configMap.value("swversion").toString());

    int updateStatus = configMap.value("swupdate").toMap().value("updatestate").toInt();
    switch (updateStatus) {
    case 0:
        //device->setStateValue(bridgeUpdateStatusStateTypeId, "Up to date");
        break;
    case 1:
        //device->setStateValue(bridgeUpdateStatusStateTypeId, "Downloading updates");
        break;
    case 2:
        //device->setStateValue(bridgeUpdateStatusStateTypeId, "Updates ready to install");
        break;
    case 3:
        //device->setStateValue(bridgeUpdateStatusStateTypeId, "Installing updates");
        break;
    default:
        break;
    }
}

void BridgeConnection::processLightsRefreshResponse(const QByteArray &data)
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
        foreach (HueLight *light, m_lights.values()) {
            if (light->id() == lightId.toInt()) {
                light->updateStates(lightMap.value("state").toMap());
            }
        }
    }
}

void BridgeConnection::processSensorsRefreshResponse(const QByteArray &data)
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
        foreach (HueRemote *remote, m_remotes.values()) {
            if (remote->id() == sensorId.toInt()) {
                remote->updateStates(sensorMap.value("state").toMap(), sensorMap.value("config").toMap());
            }
        }

        // Outdoor sensors
        foreach (HueMotionSensor *motionSensor, m_motionSensors.values()) {
            if (motionSensor->hasSensor(sensorId.toInt())) {
                motionSensor->updateStates(sensorMap);
            }
        }
    }
}

void BridgeConnection::processSetNameResponse(const QByteArray &data)
{
    QJsonParseError error;
    QJsonDocument jsonDoc = QJsonDocument::fromJson(data, &error);

    // check JSON error
    if (error.error != QJsonParseError::NoError) {
        qCWarning(dcPhilipsHue) << "Hue Bridge json error in response" << error.errorString();
        //emit deviceSetupFinished(device, Device::DeviceSetupStatusFailure);
        return;
    }

    // check response error
    if (data.contains("error")) {
        if (!jsonDoc.toVariant().toList().isEmpty()) {
            qCWarning(dcPhilipsHue) << "Failed to set name of Hue:" << jsonDoc.toVariant().toList().first().toMap().value("error").toMap().value("description").toString();
        } else {
            qCWarning(dcPhilipsHue) << "Failed to set name of Hue: Invalid error message format";
        }
        //emit setNameFinished(true);
        return;
    }
}

void BridgeConnection::processBridgeLightDiscoveryResponse(const QByteArray &data)
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


    QHash<QString, HueLight *> lights;

    QVariantMap lightsMap = jsonDoc.toVariant().toMap();
    foreach (QString lightId, lightsMap.keys()) {
        QVariantMap lightMap = lightsMap.value(lightId).toMap();

        QString uuid = lightMap.value("uniqueid").toString();
        QString model = lightMap.value("modelid").toString();

        if (lightAlreadyAdded(uuid))
            continue;

        HueLight *light = new HueLight();
        light->modelId(model)


        if (lightMap.value("type").toString() == "Dimmable light") {
            DeviceDescriptor descriptor(dimmableLightDeviceClassId, lightMap.value("name").toString(), "Philips Hue White Light", device->id());
            ParamList params;
            params.append(Param(dimmableLightDeviceModelIdParamTypeId, model));
            params.append(Param(dimmableLightDeviceTypeParamTypeId, lightMap.value("type").toString()));
            params.append(Param(dimmableLightDeviceUuidParamTypeId, uuid));
            params.append(Param(dimmableLightDeviceLightIdParamTypeId, lightId));
            descriptor.setParams(params);
            dimmableLightDescriptors.append(descriptor);

            qCDebug(dcPhilipsHue) << "Found new dimmable light" << lightMap.value("name").toString() << model;
        } else if (lightMap.value("type").toString() == "Color temperature light") {
            DeviceDescriptor descriptor(colorTemperatureLightDeviceClassId, lightMap.value("name").toString(), "Philips Hue Color Temperature Light", device->id());
            ParamList params;
            params.append(Param(colorTemperatureLightDeviceModelIdParamTypeId, model));
            params.append(Param(colorTemperatureLightDeviceTypeParamTypeId, lightMap.value("type").toString()));
            params.append(Param(colorTemperatureLightDeviceUuidParamTypeId, uuid));
            params.append(Param(colorTemperatureLightDeviceLightIdParamTypeId, lightId));
            descriptor.setParams(params);
            colorTemperatureLightDescriptors.append(descriptor);

            qCDebug(dcPhilipsHue) << "Found new color temperature light" << lightMap.value("name").toString() << model;
        } else {
            DeviceDescriptor descriptor(colorLightDeviceClassId, lightMap.value("name").toString(), "Philips Hue Color Light", device->id());
            ParamList params;
            params.append(Param(colorLightDeviceModelIdParamTypeId, model));
            params.append(Param(colorLightDeviceTypeParamTypeId, lightMap.value("type").toString()));
            params.append(Param(colorLightDeviceUuidParamTypeId, uuid));
            params.append(Param(colorLightDeviceLightIdParamTypeId, lightId));
            descriptor.setParams(params);
            colorLightDescriptors.append(descriptor);
            qCDebug(dcPhilipsHue) << "Found new color light" << lightMap.value("name").toString() << model;
        }
    }

    if (!lights.isEmpty())
        emit lightDiscovered(lights);
}

void BridgeConnection::processBridgeSensorDiscoveryResponse(const QByteArray &data)
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
    QHash<QString, HueRemote *> remotes;
    foreach (const QString &sensorId, sensorsMap.keys()) {

        QVariantMap sensorMap = sensorsMap.value(sensorId).toMap();
        QString uuid = sensorMap.value("uniqueid").toString();
        QString model = sensorMap.value("modelid").toString();

        if (sensorMap.value("type").toString() == "ZLLSwitch" || sensorMap.value("type").toString() == "ZGPSwitch") {

            HueRemote *remote = new HueRemote(this);
            remote->setModelId(model);
            remote->setType(sensorMap.value("type").toString());
            remote->setUuid(uuid);
            remote->setSensorId(sensorId);
            qCDebug(dcPhilipsHue) << "Found new remote" << sensorMap.value("name").toString() << model;

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
                    //TODO create a request specific device list and compare it to the existing one
                    //If a UUID is not preset it was removed fomr the bridge and the auto device should be removed too
                    //maybe deleting the device list and adding the new devices is the way to go
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

    if (!remotes.isEmpty()) {
        emit remotesDiscovered(remotes);
    }

    if (!motionSensors.isEmpty()) {
        emit motionSensorsDiscovered(motionSensors);
    }
}



void BridgeConnection::processPairingResponse(PairingInfo *pairingInfo, const QByteArray &data)
{
    QJsonParseError error;
    QJsonDocument jsonDoc = QJsonDocument::fromJson(data, &error);

    // check JSON error
    if (error.error != QJsonParseError::NoError) {
        qCWarning(dcPhilipsHue) << "Hue Bridge json error in response" << error.errorString();
        emit pairingFinished(pairingInfo->pairingTransactionId(), Device::DeviceSetupStatusFailure);
        pairingInfo->deleteLater();
        return;
    }

    // check response error
    if (data.contains("error")) {
        if (!jsonDoc.toVariant().toList().isEmpty()) {
            qCWarning(dcPhilipsHue) << "Failed to pair Hue Bridge:" << jsonDoc.toVariant().toList().first().toMap().value("error").toMap().value("description").toString();
        } else {
            qCWarning(dcPhilipsHue) << "Failed to pair Hue Bridge: Invalid error message format";
        }
        emit pairingFinished(pairingInfo->pairingTransactionId(), Device::DeviceSetupStatusFailure);
        pairingInfo->deleteLater();
        return;
    }

    pairingInfo->setApiKey(jsonDoc.toVariant().toList().first().toMap().value("success").toMap().value("username").toString());

    qCDebug(dcPhilipsHue) << "Got api key from bridge:" << pairingInfo->apiKey();

    if (pairingInfo->apiKey().isEmpty()) {
        qCWarning(dcPhilipsHue) << "Failed to pair Hue Bridge: did not get any key from the bridge";
        emit pairingFinished(pairingInfo->pairingTransactionId(), Device::DeviceSetupStatusFailure);
        pairingInfo->deleteLater();
        return;
    }

    // Paired successfully, check bridge information
    QNetworkRequest request(QUrl("http://" + pairingInfo->host().toString() + "/api/" + pairingInfo->apiKey() + "/config"));
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    QNetworkReply *reply = hardwareManager()->networkManager()->get(request);
    connect(reply, &QNetworkReply::finished, this, &DevicePluginPhilipsHue::networkManagerReplyReady);
    m_informationRequests.insert(reply, pairingInfo);
}

void BridgeConnection::processInformationResponse(PairingInfo *pairingInfo, const QByteArray &data)
{
    QJsonParseError error;
    QJsonDocument jsonDoc = QJsonDocument::fromJson(data, &error);

    // check JSON error
    if (error.error != QJsonParseError::NoError) {
        qCWarning(dcPhilipsHue) << "Hue Bridge json error in response" << error.errorString();
        emit pairingFinished(pairingInfo->pairingTransactionId(), Device::DeviceSetupStatusFailure);
        pairingInfo->deleteLater();
        return;
    }

    QVariantMap response = jsonDoc.toVariant().toMap();

    // check response error
    if (response.contains("error")) {
        qCWarning(dcPhilipsHue) << "Failed to get information from Hue Bridge:" << response.value("error").toMap().value("description").toString();
        emit pairingFinished(pairingInfo->pairingTransactionId(), Device::DeviceSetupStatusFailure);
        pairingInfo->deleteLater();
        return;
    }

    // create Bridge
    HueBridge *bridge = new HueBridge(this);
    bridge->setId(response.value("bridgeid").toString());
    bridge->setApiKey(pairingInfo->apiKey());
    bridge->setHostAddress(pairingInfo->host());
    bridge->setApiVersion(response.value("apiversion").toString());
    bridge->setSoftwareVersion(response.value("swversion").toString());
    bridge->setMacAddress(response.value("mac").toString());
    bridge->setName(response.value("name").toString());
    bridge->setZigbeeChannel(response.value("zigbeechannel").toInt());

    m_unconfiguredBridges.append(bridge);

    emit pairingFinished(pairingInfo->pairingTransactionId(), Device::DeviceSetupStatusSuccess);
    pairingInfo->deleteLater();
}

void BridgeConnection::processActionResponse(Device *device, const ActionId actionId, const QByteArray &data)
{
    QJsonParseError error;
    QJsonDocument jsonDoc = QJsonDocument::fromJson(data, &error);

    // check JSON error
    if (error.error != QJsonParseError::NoError) {
        qCWarning(dcPhilipsHue) << "Hue Bridge json error in response" << error.errorString();
        emit actionExecutionFinished(actionId, Device::DeviceErrorHardwareFailure);
        return;
    }

    // check response error
    if (data.contains("error")) {
        if (!jsonDoc.toVariant().toList().isEmpty()) {
            qCWarning(dcPhilipsHue) << "Failed to execute Hue action:" << jsonDoc.toJson(); //jsonDoc.toVariant().toList().first().toMap().value("error").toMap().value("description").toString();
        } else {
            qCWarning(dcPhilipsHue) << "Failed to execute Hue action: Invalid error message format";
        }
        emit actionExecutionFinished(actionId, Device::DeviceErrorHardwareFailure);
        return;
    }

    if (device->deviceClassId() != bridgeDeviceClassId)
        m_lights.key(device)->processActionResponse(jsonDoc.toVariant().toList());

    emit actionExecutionFinished(actionId, Device::DeviceErrorNoError);
}

void BridgeConnection::networkManagerReplyReady()
{
    QNetworkReply *reply = static_cast<QNetworkReply *>(sender());
    reply->deleteLater();

    int status = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();

    //    qCDebug(dcPhilipsHue()) << "Hue reply:" << status << reply->error() << reply->errorString();

    // create user finished
    if (m_pairingRequests.contains(reply)) {
        PairingInfo *pairingInfo = m_pairingRequests.take(reply);

        // check HTTP status code
        if (status != 200 || reply->error() != QNetworkReply::NoError) {
            qCWarning(dcPhilipsHue) << "Request error:" << status << reply->errorString();
            pairingInfo->deleteLater();
            return;
        }
        processPairingResponse(pairingInfo, reply->readAll());

    } else if (m_informationRequests.contains(reply)) {
        PairingInfo *pairingInfo = m_informationRequests.take(reply);

        // check HTTP status code
        if (status != 200 || reply->error() != QNetworkReply::NoError) {
            qCWarning(dcPhilipsHue) << "Request error:" << status << reply->errorString();
            pairingInfo->deleteLater();
            return;
        }
        processInformationResponse(pairingInfo, reply->readAll());

    } else if (reply == m_bridgeLightsDiscoveryRequests) {

        // check HTTP status code
        if (status != 200 || reply->error() != QNetworkReply::NoError) {
            qCWarning(dcPhilipsHue) << "Bridge light discovery error:" << status << reply->errorString();
            bridgeReachableChanged(false);
            return;
        }
        processBridgeLightDiscoveryResponse(reply->readAll());

    } else if (reply == m_bridgeSensorsDiscoveryRequests) {

        // check HTTP status code
        if (status != 200 || reply->error() != QNetworkReply::NoError) {
            qCWarning(dcPhilipsHue) << "Bridge sensor discovery error:" << status << reply->errorString();
            bridgeReachableChanged(false);
            return;
        }
        processBridgeSensorDiscoveryResponse(reply->readAll());

    } else if (reply == m_bridgeSearchDevicesRequests) {

        // check HTTP status code
        if (status != 200 || reply->error() != QNetworkReply::NoError) {
            qCWarning(dcPhilipsHue) << "Bridge search new devices error:" << status << reply->errorString();
            bridgeReachableChanged(false);
            return;
        }
        discoverBridgeDevices();

    } else if (m_bridgeRefreshRequests == reply) {;

        // check HTTP status code
        if (status != 200 || reply->error() != QNetworkReply::NoError) {
            qCWarning(dcPhilipsHue) << "Refresh Hue Bridge request error:" << status << reply->errorString();
            bridgeReachableChanged(false);
            return;
        }
        processBridgeRefreshResponse(reply->readAll());

    } else if (reply == m_lightRefreshRequests) {

        // check HTTP status code
        if (status != 200 || reply->error() != QNetworkReply::NoError) {
            qCWarning(dcPhilipsHue) << "Refresh Hue Light request error:" << status << reply->errorString();
            bridgeReachableChanged(false);
            return;
        }
        processLightRefreshResponse(reply->readAll());

    } else if (reply == m_lightsRefreshRequests) {

        // check HTTP status code
        if (status != 200 || reply->error() != QNetworkReply::NoError) {
            qCWarning(dcPhilipsHue) << "Refresh Hue lights request error:" << status << reply->errorString();
            bridgeReachableChanged(false);
            return;
        }
        processLightsRefreshResponse(reply->readAll());

    } else if (reply == m_sensorsRefreshRequests) {

        // check HTTP status code
        if (status != 200 || reply->error() != QNetworkReply::NoError) {
            qCWarning(dcPhilipsHue) << "Refresh Hue sensors request error:" << status << reply->errorString();
            bridgeReachableChanged(false);
            return;
        }
        processSensorsRefreshResponse(reply->readAll());

    } else if (m_asyncActions.contains(reply)) {
        QPair<Device *, ActionId> actionInfo = m_asyncActions.take(reply);

        // check HTTP status code
        if (status != 200 || reply->error() != QNetworkReply::NoError) {
            qCWarning(dcPhilipsHue) << "Execute Hue Light action request error:" << status << reply->errorString();
            bridgeReachableChanged(false);
            //emit actionExecutionFinished(actionInfo.second, Device::DeviceErrorHardwareNotAvailable);
            return;
        }
        processActionResponse(actionInfo.second, reply->readAll());

    } else if (reply == m_setNameRequests) {

        // check HTTP status code
        if (status != 200 || reply->error() != QNetworkReply::NoError) {
            qCWarning(dcPhilipsHue) << "Set name of Hue Light request error:" << status << reply->errorString();
            bridgeReachableChanged(false);
            return;
        }
        processSetNameResponse(reply->readAll());
    } else {
        qCWarning(dcPhilipsHue()) << "Unhandled bridge reply" << reply->error() << reply->readAll();
    }
}


int BridgeConnection::brightnessToPercentage(int brightness)
{
    return qRound((100.0 * brightness) / 255.0);
}

int BridgeConnection::percentageToBrightness(int percentage)
{
    return qRound((255.0 * percentage) / 100.0);
}

void BridgeConnection::abortRequests(QHash<QNetworkReply *, Device *> requestList, Device *device)
{
    foreach (QNetworkReply* reply, requestList.keys()) {
        if (requestList.value(reply) == device) {
            reply->abort();
        }
    }
}
