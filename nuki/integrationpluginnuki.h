// SPDX-License-Identifier: GPL-3.0-or-later

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*
* Copyright (C) 2013 - 2024, nymea GmbH
* Copyright (C) 2024 - 2025, chargebyte austria GmbH
*
* This file is part of nymea-plugins.
*
* nymea-plugins is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* (at your option) any later version.
*
* nymea-plugins is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
* General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with nymea-plugins. If not, see <https://www.gnu.org/licenses/>.
*
* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

#ifndef INTEGRATIONPLUGINNUKI_H
#define INTEGRATIONPLUGINNUKI_H

#include <integrations/integrationplugin.h>
#include <bluez/bluetoothmanager.h>
#include <plugintimer.h>

#include "nuki.h"

class IntegrationPluginNuki : public IntegrationPlugin
{
    Q_OBJECT

    Q_PLUGIN_METADATA(IID "io.nymea.IntegrationPlugin" FILE "integrationpluginnuki.json")
    Q_INTERFACES(IntegrationPlugin)

public:
    explicit IntegrationPluginNuki();
    ~IntegrationPluginNuki();

    void init() override;
    void setupThing(ThingSetupInfo *info) override;
    void discoverThings(ThingDiscoveryInfo *info) override;
    void startPairing(ThingPairingInfo *info) override;
    void confirmPairing(ThingPairingInfo *info, const QString &username, const QString &secret) override;
    void postSetupThing(Thing *thing) override;
    void executeAction(ThingActionInfo *info) override;
    void thingRemoved(Thing *thing) override;

private:
    QHash<Nuki *, Thing *> m_nukiDevices;
    PluginTimer *m_refreshTimer = nullptr;

    BluetoothManager *m_bluetoothManager = nullptr;
    BluetoothAdapter *m_bluetoothAdapter = nullptr;

    Nuki *m_asyncSetupNuki = nullptr;
    ThingPairingInfo *m_pairingInfo = nullptr;

    bool m_encrytionLibraryInitialized = false;

    bool bluetoothDeviceAlreadyAdded(const QBluetoothAddress &address);

private slots:
    void onRefreshTimeout();
    void onBluetoothEnabledChanged(const bool &enabled);
    void onBluetoothDiscoveryFinished(ThingDiscoveryInfo *info);

};

#endif // INTEGRATIONPLUGINNUKI_H
