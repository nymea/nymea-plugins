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

#ifndef INTEGRATIONPLUGINDYNATRACE_H
#define INTEGRATIONPLUGINDYNATRACE_H

#include <integrations/integrationplugin.h>
#include <plugintimer.h>
#include <network/oauth2.h>

#include <QHash>
#include <QTimer>
#include <QHostInfo>

#include "ufo.h"

class IntegrationPluginDynatrace : public IntegrationPlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "io.nymea.IntegrationPlugin" FILE "integrationplugindynatrace.json")
    Q_INTERFACES(IntegrationPlugin)

public:
    explicit IntegrationPluginDynatrace();
    void discoverThings(ThingDiscoveryInfo *info) override;
    void setupThing(ThingSetupInfo *info) override;
    void postSetupThing(Thing *thing) override;
    void executeAction(ThingActionInfo *info) override;
    void thingRemoved(Thing *thing) override;


private slots:
    void onConnectionChanged(bool connected);

    void resolveIds(const QHostInfo &host);

private:
    PluginTimer *m_pluginTimer = nullptr;
    QHash<ThingId, Ufo *> m_ufoConnections;
    QHash<QUuid, ThingActionInfo *> m_asyncActions;
    QHash<QString, ThingSetupInfo *> m_asyncSetup;
    QHash<int, ThingDiscoveryInfo *> m_asyncDiscoveries;

    void getId(const QHostAddress &address);
};

#endif // INTEGRATIONPLUGINDYNATRACE_H
