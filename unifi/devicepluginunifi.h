/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *                                                                         *
 *  Copyright (C) 2019 Michael Zanetti <michael.zanetti@nymea.io>          *
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

#ifndef DEVICEPLUGINUNIFI_H
#define DEVICEPLUGINUNIFI_H

#include <QObject>

#include "devices/deviceplugin.h"

#include <QNetworkRequest>

class PluginTimer;

class DevicePluginUnifi : public DevicePlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "io.nymea.DevicePlugin" FILE "devicepluginunifi.json")
    Q_INTERFACES(DevicePlugin)

public:
    explicit DevicePluginUnifi(QObject *parent = nullptr);
    ~DevicePluginUnifi() override;

    void init() override;
    void discoverDevices(DeviceDiscoveryInfo *info) override;
    void startPairing(DevicePairingInfo *info) override;
    void confirmPairing(DevicePairingInfo *info, const QString &username, const QString &secret) override;
    void setupDevice(DeviceSetupInfo *info) override;
    void postSetupDevice(Device *device) override;
    void deviceRemoved(Device *device) override;
//    void executeAction(DeviceActionInfo *info) override;

private:
    QNetworkRequest createRequest(const QString &address, const QString &path);
    QNetworkRequest createRequest(Device *device, const QString &path);

    void markOffline(Device *device);
private:
    QHash<DeviceDiscoveryInfo*, Devices> m_pendingDiscoveries;
    QHash<Device*, QStringList> m_pendingSiteDiscoveries;

    PluginTimer *m_loginTimer = nullptr;
    PluginTimer *m_pollTimer = nullptr;
};

#endif // DEVICEPLUGINUNIFI_H
