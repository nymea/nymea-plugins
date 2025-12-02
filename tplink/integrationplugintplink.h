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

#ifndef INTEGRATIONPLUGINTPLINK_H
#define INTEGRATIONPLUGINTPLINK_H

#include <integrations/integrationplugin.h>

#include <QUdpSocket>

#include <QNetworkAccessManager>
#include <QTimer>

class PluginTimer;

class IntegrationPluginTPLink: public IntegrationPlugin
{
    Q_OBJECT

    Q_PLUGIN_METADATA(IID "io.nymea.IntegrationPlugin" FILE "integrationplugintplink.json")
    Q_INTERFACES(IntegrationPlugin)

public:
    explicit IntegrationPluginTPLink();
    ~IntegrationPluginTPLink() override;

    void init() override;
    void discoverThings(ThingDiscoveryInfo *info) override;
    void setupThing(ThingSetupInfo *info) override;
    void postSetupThing(Thing *thing) override;
    void thingRemoved(Thing *thing) override;
    void executeAction(ThingActionInfo *info) override;

private:
    QByteArray encryptPayload(const QByteArray &payload);
    QByteArray decryptPayload(const QByteArray &payload);

    void connectToDevice(Thing *thing, const QHostAddress &address);
    void fetchState(Thing *thing, ThingActionInfo *info = nullptr);

    void processQueue(Thing *thing);

private:
    class Job {
    public:
        int id = 0;
        QByteArray data;
        ThingActionInfo *actionInfo = nullptr;
        bool operator==(const Job &other) const { return id == other.id; }
    };

    QHash<Thing *, Job> m_pendingJobs;
    QHash<Thing *, QList<Job>> m_jobQueue;
    QHash<Thing *, QTimer *> m_jobTimers;
    int m_jobIdx = 0;

    QUdpSocket *m_broadcastSocket = nullptr;
    QHash<Thing *, QTcpSocket *> m_sockets;
    QHash<ThingSetupInfo *, int> m_setupRetries;

    QHash<Thing *, QByteArray> m_inputBuffers;

    PluginTimer *m_timer = nullptr;
};

#endif // INTEGRATIONPLUGINANEL_H
