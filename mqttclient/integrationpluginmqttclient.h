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

#ifndef INTEGRATIONPLUGINMQTTCLIENT_H
#define INTEGRATIONPLUGINMQTTCLIENT_H

#include <integrations/integrationplugin.h>

#include <QHash>
#include <QDebug>
#include <QUdpSocket>

#include "extern-plugininfo.h"

class MqttClient;

class IntegrationPluginMqttClient: public IntegrationPlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "io.nymea.IntegrationPlugin" FILE "integrationpluginmqttclient.json")
    Q_INTERFACES(IntegrationPlugin)

public:
    explicit IntegrationPluginMqttClient();

    void setupThing(ThingSetupInfo *info) override;
    void thingRemoved(Thing *thing) override;

    void executeAction(ThingActionInfo *info) override;

private slots:
    void subscribe(Thing *thing);

    void publishReceived(const QString &topic, const QByteArray &payload, bool retained);

private:
    QHash<Thing*, MqttClient*> m_clients;
};

#endif // INTEGRATIONPLUGINMQTTCLIENT_H
