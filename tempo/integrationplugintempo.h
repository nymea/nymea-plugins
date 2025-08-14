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
