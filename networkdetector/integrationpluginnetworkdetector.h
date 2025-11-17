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

#ifndef INTEGRATIONPLUGINNETWORKDETECTOR_H
#define INTEGRATIONPLUGINNETWORKDETECTOR_H

#include <integrations/integrationplugin.h>
#include <network/networkdevicediscovery.h>
#include <plugintimer.h>

#include <QHostInfo>

class IntegrationPluginNetworkDetector : public IntegrationPlugin
{
    Q_OBJECT

    Q_PLUGIN_METADATA(IID "io.nymea.IntegrationPlugin" FILE "integrationpluginnetworkdetector.json")
    Q_INTERFACES(IntegrationPlugin)

public:
    explicit IntegrationPluginNetworkDetector();
    ~IntegrationPluginNetworkDetector();

    void discoverThings(ThingDiscoveryInfo *info) override;
    void setupThing(ThingSetupInfo *info) override;
    void thingRemoved(Thing *thing) override;

    void executeAction(ThingActionInfo *info) override;

private:
    QHash<Thing *, NetworkDeviceMonitor *> m_monitors;
    QHash<Thing *, PluginTimer *> m_gracePeriodTimers;
    QHash<int, ThingActionInfo *> m_pendingHostLookup;

    void updateStates(Thing *thing, NetworkDeviceMonitor *monitor);

private slots:
    void onHostLookupFinished(const QHostInfo &info);

};

#endif // INTEGRATIONPLUGINNETWORKDETECTOR_H
