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

#ifndef INTEGRATIONPLUGINTADO_H
#define INTEGRATIONPLUGINTADO_H

#include <integrations/integrationplugin.h>
#include <plugintimer.h>
#include <network/oauth2.h>

#include <QHash>
#include <QTimer>

#include "extern-plugininfo.h"
#include "tado.h"

class IntegrationPluginTado : public IntegrationPlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "io.nymea.IntegrationPlugin" FILE "integrationplugintado.json")
    Q_INTERFACES(IntegrationPlugin)

public:
    explicit IntegrationPluginTado();

    void init() override;

    void startPairing(ThingPairingInfo *info) override;
    void confirmPairing(ThingPairingInfo *info, const QString &username, const QString &secret) override;

    void setupThing(ThingSetupInfo *info) override;
    void thingRemoved(Thing *thing) override;
    void postSetupThing(Thing *thing) override;
    void executeAction(ThingActionInfo *info) override;

private:
    PluginTimer *m_pluginTimer = nullptr;
    QHash<ThingId, Tado *> m_unfinishedTadoAccounts;

    QHash<ThingId, Tado *> m_tadoAccounts;
    QHash<QUuid, ThingActionInfo *> m_asyncActions;

private slots:
    void onPluginTimer();

    void onConnectionChanged(bool connected);
    void onAuthenticationStatusChanged(bool authenticated);
    void onUsernameChanged(const QString &username);
    void onRequestExecuted(QUuid requestId, bool success);
    void onHomesReceived(QList<Tado::Home> homes);
    void onZonesReceived(const QString &homeId, QList<Tado::Zone> zones);
    void onZoneStateReceived(const QString &homeId,const QString &zoneId, Tado::ZoneState sate);
    void onOverlayReceived(const QString &homeId, const QString &zoneId, const Tado::Overlay &overlay);
};

#endif // INTEGRATIONPLUGINTADO_H
