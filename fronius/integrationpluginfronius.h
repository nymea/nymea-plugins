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

#ifndef INTEGRATIONPLUGINFRONIUS_H
#define INTEGRATIONPLUGINFRONIUS_H

#include <integrations/integrationplugin.h>
#include <network/networkaccessmanager.h>
#include <network/networkdevicediscovery.h>

#include "froniussolarconnection.h"

class PluginTimer;

class IntegrationPluginFronius : public IntegrationPlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "io.nymea.IntegrationPlugin" FILE "integrationpluginfronius.json")
    Q_INTERFACES(IntegrationPlugin)

public:
    explicit IntegrationPluginFronius(QObject *parent = nullptr);

    void discoverThings(ThingDiscoveryInfo *info) override;
    void setupThing(ThingSetupInfo *thing) override;
    void postSetupThing(Thing* thing) override;
    void executeAction(ThingActionInfo *info) override;
    void thingRemoved(Thing* thing) override;

private:
    PluginTimer *m_connectionRefreshTimer = nullptr;

    QHash<FroniusSolarConnection *, Thing *> m_froniusConnections;
    QHash<Thing *, NetworkDeviceMonitor *> m_monitors;
    QHash<FroniusSolarConnection *, bool> m_weakMeterConnections;

    QHash<Thing *, uint> m_thingRequestErrorCounter;
    uint m_thingRequestErrorCountLimit = 3;

    void refreshConnection(FroniusSolarConnection *connection);

    void updatePowerFlow(FroniusSolarConnection *connection);
    void updateInverters(FroniusSolarConnection *connection);
    void updateMeters(FroniusSolarConnection *connection);
    void updateStorages(FroniusSolarConnection *connection);

    void markInverterAsDisconnected(Thing *thing);
    void markMeterAsDisconnected(Thing *thing);
    void markStorageAsDisconnected(Thing *thing);
};

#endif // INTEGRATIONPLUGINFRONIUS_H
