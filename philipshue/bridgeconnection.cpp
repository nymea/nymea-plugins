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
