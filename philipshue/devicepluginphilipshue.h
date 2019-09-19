/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *                                                                         *
 *  Copyright (C) 2014 Michael Zanetti <michael_zanetti@gmx.net>           *
 *  Copyright (C) 2015 - 2019 Simon Stürz <simon.stuerz@guh.io>            *
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

#ifndef DEVICEPLUGINPHILIPSHUE_H
#define DEVICEPLUGINPHILIPSHUE_H

#include "devices/deviceplugin.h"
#include "huebridge.h"
#include "huelight.h"
#include "hueremote.h"
#include "huemotionsensor.h"
#include "huemotionsensor.h"

#include "plugintimer.h"
#include "network/networkaccessmanager.h"
#include "network/upnp/upnpdiscovery.h"

class QNetworkReply;

class DevicePluginPhilipsHue: public DevicePlugin
{
    Q_OBJECT

    Q_PLUGIN_METADATA(IID "io.nymea.DevicePlugin" FILE "devicepluginphilipshue.json")
    Q_INTERFACES(DevicePlugin)

public:
    explicit DevicePluginPhilipsHue();
    ~DevicePluginPhilipsHue();

    void init() override;
    void discoverDevices(DeviceDiscoveryInfo *info) override;
    void startPairing(DevicePairingInfo *info) override;
    void confirmPairing(DevicePairingInfo *info, const QString &username, const QString &secret) override;
    void setupDevice(DeviceSetupInfo *info) override;
    void deviceRemoved(Device *device) override;
    void executeAction(DeviceActionInfo *info) override;

private slots:
    void lightStateChanged();
    void remoteStateChanged();
    void onRemoteButtonEvent(int buttonCode);

    // Motion sensor
    void onMotionSensorReachableChanged(bool reachable);
    void onMotionSensorBatteryLevelChanged(int batteryLevel);
    void onMotionSensorTemperatureChanged(double temperature);
    void onMotionSensorPresenceChanged(bool presence);
    void onMotionSensorLightIntensityChanged(double lightIntensity);

private slots:
    void networkManagerReplyReady();
    void onDeviceNameChanged();

private:
    class DiscoveryJob {
    public:
        UpnpDiscoveryReply* upnpReply;
        QNetworkReply* nUpnpReply;
        DeviceDescriptors results;
    };
    QHash<DeviceDiscoveryInfo*, DiscoveryJob*> m_discoveries;
    void finishDiscovery(DiscoveryJob* job);

    PluginTimer *m_pluginTimer1Sec = nullptr;
    PluginTimer *m_pluginTimer5Sec = nullptr;
    PluginTimer *m_pluginTimer15Sec = nullptr;

    QList<HueLight *> m_unconfiguredLights;

    QHash<QNetworkReply *, Device *> m_lightRefreshRequests;
    QHash<QNetworkReply *, Device *> m_setNameRequests;
    QHash<QNetworkReply *, Device *> m_bridgeRefreshRequests;
    QHash<QNetworkReply *, Device *> m_lightsRefreshRequests;
    QHash<QNetworkReply *, Device *> m_sensorsRefreshRequests;
    QHash<QNetworkReply *, Device *> m_bridgeLightsDiscoveryRequests;
    QHash<QNetworkReply *, Device *> m_bridgeSensorsDiscoveryRequests;
    QHash<QNetworkReply *, Device *> m_bridgeSearchDevicesRequests;

    QHash<QNetworkReply *, QPair<Device *, ActionId> > m_asyncActions;

    QHash<HueBridge *, Device *> m_bridges;
    QHash<HueLight *, Device *> m_lights;
    QHash<HueRemote *, Device *> m_remotes;
    QHash<HueMotionSensor *, Device *> m_motionSensors;

    void refreshLight(Device *device);
    void refreshBridge(Device *device);

    void refreshLights(HueBridge *bridge);
    void refreshSensors(HueBridge *bridge);

    void discoverBridgeDevices(HueBridge *bridge);
    void searchNewDevices(HueBridge *bridge, const QString &serialNumber);

    void setLightName(Device *device);
    void setRemoteName(Device *device);

    void processNUpnpResponse(const QByteArray &data);
    void processBridgeLightDiscoveryResponse(Device *device, const QByteArray &data);
    void processBridgeSensorDiscoveryResponse(Device *device, const QByteArray &data);
    void processLightRefreshResponse(Device *device, const QByteArray &data);
    void processBridgeRefreshResponse(Device *device, const QByteArray &data);
    void processLightsRefreshResponse(Device *device, const QByteArray &data);
    void processSensorsRefreshResponse(Device *device, const QByteArray &data);
    void processSetNameResponse(Device *device, const QByteArray &data);
    void processActionResponse(Device *device, const ActionId actionId, const QByteArray &data);

    void bridgeReachableChanged(Device *device, const bool &reachable);

    Device* bridgeForBridgeId(const QString &id);
    bool lightAlreadyAdded(const QString &uuid);
    bool sensorAlreadyAdded(const QString &uuid);

    int brightnessToPercentage(int brightness);
    int percentageToBrightness(int percentage);

    void abortRequests(QHash<QNetworkReply *, Device *> requestList, Device* device);
};

#endif // DEVICEPLUGINBOBLIGHT_H
