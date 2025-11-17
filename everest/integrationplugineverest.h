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

#ifndef INTEGRATIONPLUGINEVEREST_H
#define INTEGRATIONPLUGINEVEREST_H

#include "integrations/integrationplugin.h"
#include "extern-plugininfo.h"

#include "mqtt/everestmqttclient.h"
#include "jsonrpc/everestconnection.h"

#include <mqttclient.h>

class IntegrationPluginEverest: public IntegrationPlugin
{
    Q_OBJECT

    Q_PLUGIN_METADATA(IID "io.nymea.IntegrationPlugin" FILE "integrationplugineverest.json")
    Q_INTERFACES(IntegrationPlugin)

public:
    explicit IntegrationPluginEverest();

    void init() override;
    void startMonitoringAutoThings() override;
    void discoverThings(ThingDiscoveryInfo *info) override;

    void setupThing(ThingSetupInfo *info) override;
    void thingRemoved(Thing *thing) override;

    void executeAction(ThingActionInfo *info) override;

private:
    int m_autodetectCounter = 0;
    int m_autodetectCounterLimit = 4;
    bool m_useMqtt = false;
    QList<EverestMqttClient *> m_everstMqttClients;
    QHash<Thing *, EverestMqttClient *> m_thingClients;

    QHash<Thing *, EverestConnection *> m_everstConnections;
};

#endif // INTEGRATIONPLUGINEVEREST_H
