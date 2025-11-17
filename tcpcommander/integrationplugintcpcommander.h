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

#ifndef INTEGRATIONPLUGINDEVTCPCOMMANDER_H
#define INTEGRATIONPLUGINDEVTCPCOMMANDER_H

#include <integrations/integrationplugin.h>

#include <QTcpServer>

class TcpServer;

class IntegrationPluginTcpCommander : public IntegrationPlugin
{
    Q_OBJECT

    Q_PLUGIN_METADATA(IID "io.nymea.IntegrationPlugin" FILE "integrationplugintcpcommander.json")
    Q_INTERFACES(IntegrationPlugin)

public:
    explicit IntegrationPluginTcpCommander();

    void setupThing(ThingSetupInfo *info) override;

    void thingRemoved(Thing *thing) override;

    void executeAction(ThingActionInfo *info) override;

private:
    QHash<Thing *, QTcpSocket *> m_tcpSockets;
    QHash<Thing *, TcpServer *> m_tcpServers;

private slots:
    void onTcpSocketConnectionChanged(bool connected);

    void onTcpServerConnectionCountChanged(int connections);
    void onTcpServerCommandReceived(const QString &clientIp, const QByteArray &message);
};

#endif // INTEGRATIONPLUGINTCPCOMMANDER_H
