/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *                                                                         *
 *  Copyright (C) 2018 Bernhard Trinnes <benhard.trinnes@guh.io>           *
 *  Copyright (C) 2018 Simon Stürz <simon.stuerz@guh.io>                   *
 *                                                                         *
 *  This file is part of guh.                                              *
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

#ifndef DEVICEPLUGINNUKI_H
#define DEVICEPLUGINNUKI_H

#include "plugintimer.h"
#include "devices/deviceplugin.h"
#include "bluez/bluetoothmanager.h"

#include "nuki.h"

class DevicePluginNuki : public DevicePlugin
{
    Q_OBJECT

    Q_PLUGIN_METADATA(IID "guru.guh.DevicePlugin" FILE "devicepluginnuki.json")
    Q_INTERFACES(DevicePlugin)

public:
    explicit DevicePluginNuki();
    ~DevicePluginNuki();

    void init() override;
    void setupDevice(DeviceSetupInfo *info) override;
    void discoverDevices(DeviceDiscoveryInfo *info) override;
    void startPairing(DevicePairingInfo *info) override;
    void confirmPairing(DevicePairingInfo *info, const QString &username, const QString &secret) override;
    void postSetupDevice(Device *device) override;
    void executeAction(DeviceActionInfo *info) override;
    void deviceRemoved(Device *device) override;

private:
    QHash<Nuki *, Device *> m_nukiDevices;
    PluginTimer *m_refreshTimer = nullptr;

    BluetoothManager *m_bluetoothManager = nullptr;
    BluetoothAdapter *m_bluetoothAdapter = nullptr;

    Nuki *m_asyncSetupNuki = nullptr;
    DevicePairingInfo *m_pairingInfo = nullptr;

    bool m_encrytionLibraryInitialized = false;

    bool bluetoothDeviceAlreadyAdded(const QBluetoothAddress &address);

private slots:
    void onRefreshTimeout();
    void onBluetoothEnabledChanged(const bool &enabled);
    void onBluetoothDiscoveryFinished(DeviceDiscoveryInfo *info);

    void onAsyncSetupNukiAvailableChanged(bool available);

    void onNukiAuthenticationProcessFinished(const PairingTransactionId &pairingTransactionId, bool success);

};

#endif // DEVICEPLUGINNUKI_H
