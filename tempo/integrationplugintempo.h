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

#ifndef INTEGRATIONPLUGINTEMPO_H
#define INTEGRATIONPLUGINTEMPO_H

#include <integrations/integrationplugin.h>

#include "tempo.h"

class PluginTimer;

class IntegrationPluginTempo : public IntegrationPlugin
{
    Q_OBJECT

    Q_PLUGIN_METADATA(IID "io.nymea.IntegrationPlugin" FILE "integrationplugintempo.json")
    Q_INTERFACES(IntegrationPlugin)

public:
    explicit IntegrationPluginTempo();

    void startPairing(ThingPairingInfo *info) override;
    void confirmPairing(ThingPairingInfo *info, const QString &username, const QString &secret) override;

    void discoverThings(ThingDiscoveryInfo *info) override;
    void setupThing(ThingSetupInfo *info) override;
    void postSetupThing(Thing *thing) override;
    void thingRemoved(Thing *thing) override;

private:
    PluginTimer *m_pluginTimer15min = nullptr;

    QHash<ThingId, QList<Tempo::Worklog>> m_worklogBuffer;
    QHash<ThingId, Tempo *> m_setupTempoConnections;
    QHash<ThingId, Tempo *> m_tempoConnections;

private slots:
    void onConnectionChanged(bool connected);
    void onAuthenticationStatusChanged(bool authenticated);

    void onAccountsReceived(const QList<Tempo::Account> accounts);
    void onTeamsReceived(const QList<Tempo::Team> teams);

    void onAccountWorkloadReceived(const QString &accountKey, QList<Tempo::Worklog> workloads, int limit, int offset);
    void onTeamWorkloadReceived(int teamId, QList<Tempo::Worklog> workloads, int limit, int offset);

};
#endif // INTEGRATIONPLUGINTEMPO_H
