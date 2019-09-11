/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *                                                                         *
 *  Copyright (C) 2019 Bernhard Trinnes <bernhard.trinnes@nymea.io         *
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

#ifndef DEVICEPLUGINHOMECONNECT_H
#define DEVICEPLUGINHOMECONNECt_H

#include "devices/deviceplugin.h"
#include "plugintimer.h"
#include "homeconnect.h"

#include <QHash>
#include <QDebug>

class DevicePluginHomeConnect : public DevicePlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "io.nymea.DevicePlugin" FILE "devicepluginHomeConnect.json")
    Q_INTERFACES(DevicePlugin)

public:
    explicit DevicePluginHomeConnect();
    ~DevicePluginHomeConnect() override;

    Device::DeviceSetupStatus setupDevice(Device *device) override;
    DevicePairingInfo pairDevice(DevicePairingInfo &devicePairingInfo) override;
    DevicePairingInfo confirmPairing(DevicePairingInfo &devicePairingInfo, const QString &username, const QString &secret) override;

    void postSetupDevice(Device *device) override;
    void startMonitoringAutoDevices() override;
    void deviceRemoved(Device *device) override;
    Device::DeviceError executeAction(Device *device, const Action &action) override;

private:
    PluginTimer *m_pluginTimer5sec = nullptr;
    PluginTimer *m_pluginTimer60sec = nullptr;

    QHash<DeviceId, HomeConnect *> m_setupHomeConnectConnections;
    QHash<Device *, HomeConnect *> m_homeConnectConnections;

    QHash<QUuid, ActionId> m_pendingActions;

private slots:
    void onConnectionChanged(bool connected);
    void onAuthenticationStatusChanged(bool authenticated);
    void onActionExecuted(QUuid actionId, bool success);
};

#endif // DEVICEPLUGINHOMECONNECT_H
