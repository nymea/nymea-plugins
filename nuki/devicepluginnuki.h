/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*
* Copyright 2013 - 2020, nymea GmbH
* Contact: contact@nymea.io
*
* This file is part of nymea.
* This project including source code and documentation is protected by
* copyright law, and remains the property of nymea GmbH. All rights, including
* reproduction, publication, editing and translation, are reserved. The use of
* this project is subject to the terms of a license agreement to be concluded
* with nymea GmbH in accordance with the terms of use of nymea GmbH, available
* under https://nymea.io/license
*
* GNU Lesser General Public License Usage
* Alternatively, this project may be redistributed and/or modified under the
* terms of the GNU Lesser General Public License as published by the Free
* Software Foundation; version 3. This project is distributed in the hope that
* it will be useful, but WITHOUT ANY WARRANTY; without even the implied
* warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
* Lesser General Public License for more details.
*
* You should have received a copy of the GNU Lesser General Public License
* along with this project. If not, see <https://www.gnu.org/licenses/>.
*
* For any further details and any questions please contact us under
* contact@nymea.io or see our FAQ/Licensing Information on
* https://nymea.io/license/faq
*
* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

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
