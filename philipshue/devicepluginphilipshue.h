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

#ifndef DEVICEPLUGINPHILIPSHUE_H
#define DEVICEPLUGINPHILIPSHUE_H

#include "devices/deviceplugin.h"
#include "bridgeconnection.h"
#include "huebridge.h"
#include "huelight.h"
#include "hueremote.h"
#include "pairinginfo.h"
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

    Device::DeviceSetupStatus setupDevice(Device *device) override;
    Device::DeviceError discoverDevices(const DeviceClassId &deviceClassId, const ParamList &params) override;
    void deviceRemoved(Device *device) override;
    Device::DeviceSetupStatus confirmPairing(const PairingTransactionId &pairingTransactionId, const DeviceClassId &deviceClassId, const ParamList &params, const QString &secret) override;
    Device::DeviceError executeAction(Device *device, const Action &action) override;

private slots:

    //Devices discovered
    void onLightsDiscovered(QHash<QString, HueLight *> remotes);
    void onRemotesDiscovered(QHash<QString, HueRemote *> remotes);
    void onMotionSensorDiscovered(QHash<QString, HueMotionSensor *> motionSensors);

    // Light Device Events
    void onLightStateChanged();

    // Remote Device Eventes
    void onRemoteStateChanged();
    void onRemoteButtonEvent(int buttonCode);

    // Motion Sensor Events
    void onMotionSensorReachableChanged(bool reachable);
    void onMotionSensorBatteryLevelChanged(int batteryLevel);
    void onMotionSensorTemperatureChanged(double temperature);
    void onMotionSensorPresenceChanged(bool presence);
    void onMotionSensorLightIntensityChanged(double lightIntensity);

    void onDeviceNameChanged();

private:
    class DiscoveryJob {
    public:
        UpnpDiscoveryReply* upnpReply;
        QNetworkReply* nUpnpReply;
        QList<DeviceDescriptor> results;
    };
    void finishDiscovery(DiscoveryJob* job);

    QHash<QNetworkReply *, PairingInfo *> m_pairingRequests;
    QHash<QNetworkReply *, PairingInfo *> m_informationRequests;

    QHash<DeviceId, BridgeConnection *> m_bridgeConnections;

    QList<HueBridge *> m_unconfiguredBridges;
    QList<HueLight *> m_unconfiguredLights;

    QHash<QNetworkReply *, QPair<Device *, ActionId> > m_asyncActions;

    bool lightAlreadyAdded(const QString &uuid);
    bool sensorAlreadyAdded(const QString &uuid);
};

#endif // DEVICEPLUGINPHILIPSHUE_H
