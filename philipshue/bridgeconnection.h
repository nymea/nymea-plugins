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


#ifndef BRIDGECONNECTION_H
#define BRIDGECONNECTION_H

#include "devices/deviceplugin.h"
#include "huebridge.h"
#include "huelight.h"
#include "hueremote.h"
#include "pairinginfo.h"
#include "huemotionsensor.h"

#include "plugintimer.h"
#include "network/networkaccessmanager.h"
#include "network/upnp/upnpdiscovery.h"
#include "devices/devicemanager.h"

#include <QObject>

class BridgeConnection : public QObject
{
    Q_OBJECT
public:
    explicit BridgeConnection(HueBridge *bridge, HardwareManager *hardwaremanager, QObject *parent = nullptr);
    ~BridgeConnection();
    void init();

    //bridge actions
    void discoverBridgeDevices();
    void searchNewDevices(const QString &serialNumber);

    //light action - the uuid is to track if the actions has been executed
    QUuid setPower(const QString &uuid, bool power);
    QUuid setBrightness(const QString &uuid, quint8 percent);
    QUuid setColor(const QString &uuid, QColor percent);
    QUuid setEffect(const QString &uuid, const QString &effect);
    QUuid setFlash(const QString &uuid, const QString &flashMode);
    QUuid setTemperature(const QString &uuid, quint16 temperature);

    QUuid setLightName(const QString &uuid, const QString &name);
    QUuid setRemoteName(const QString &uuid, const QString &name);

    void refreshLight(const QString &uuid);    //
    void refreshBridge();
    void refreshLights();
    void refreshSensors();

signals:

    void pairingFinished(bool successful);

    //Devices discovered
    void lightDiscovered(QHash<QString, HueLight *>);
    void remoteDiscovered(QHash<QString, HueRemote *>);
    void motionSensorDiscovered(QHash<QString, HueMotionSensor *>);

    void lightStateChanged();
    void remoteStateChanged();
    void onRemoteButtonEvent(int buttonCode);

    void motionSensorReachableChanged(bool reachable);
    void motionSensorBatteryLevelChanged(int batteryLevel);
    void motionSensorTemperatureChanged(double temperature);
    void motionSensorPresenceChanged(bool presence);
    void motionSensorLightIntensityChanged(double lightIntensity);

    void networkManagerReplyReady();
    void onDeviceNameChanged();

    void actionExecuted(QUuid actionUuid);
    void actionFailed(const QUuid &uuid, const QString &error);

private:
    HardwareManager *m_hardwareManager = nullptr;

    PluginTimer *m_pluginTimer1Sec = nullptr;
    PluginTimer *m_pluginTimer5Sec = nullptr;
    PluginTimer *m_pluginTimer15Sec = nullptr;

    QHash<QNetworkReply *, PairingInfo *> m_pairingRequests;
    QHash<QNetworkReply *, PairingInfo *> m_informationRequests;

    QHash<QNetworkReply *, QUuid> m_actionRequests;

    QNetworkReply *m_lightRefreshRequests;
    QNetworkReply *m_setNameRequests;
    QNetworkReply *m_bridgeRefreshRequests;
    QNetworkReply *m_lightsRefreshRequests;
    QNetworkReply *m_sensorsRefreshRequests;
    QNetworkReply *m_bridgeLightsDiscoveryRequests;
    QNetworkReply *m_bridgeSensorsDiscoveryRequests;
    QNetworkReply *m_bridgeSearchDevicesRequests;

    HueBridge *m_bridge;
    QHash<QString, HueLight *> m_lights;
    QHash<QString, HueRemote *> m_remotes;
    QHash<QString, HueMotionSensor *> m_motionSensors;

    void processBridgeLightDiscoveryResponse(const QByteArray &data);
    void processBridgeSensorDiscoveryResponse(const QByteArray &data);
    void processLightRefreshResponse(const QByteArray &data);
    void processBridgeRefreshResponse(const QByteArray &data);
    void processLightsRefreshResponse(const QByteArray &data);
    void processSensorsRefreshResponse(const QByteArray &data);
    void processSetNameResponse(const QByteArray &data);
    void processPairingResponse(PairingInfo *pairingInfo, const QByteArray &data);
    void processInformationResponse(PairingInfo *pairingInfo, const QByteArray &data);
    void processActionResponse(const QUuid actionUuid, const QByteArray &data);

    void bridgeReachableChanged(const bool &reachable);

    //bool lightAlreadyAdded(const QString &uuid);
    //bool sensorAlreadyAdded(const QString &uuid);

    int brightnessToPercentage(int brightness);
    int percentageToBrightness(int percentage);

    QUuid scheduleActionRequest(QNetworkRequest *request);
};

#endif // BRIDGECONNECTION_H
