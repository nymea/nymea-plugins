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

#ifndef INTEGRATIONPLUGINTPLINK_H
#define INTEGRATIONPLUGINTPLINK_H

#include "integrations/integrationplugin.h"

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
    ~IntegrationPluginTPLink();

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
        bool operator==(const Job &other) { return id == other.id; }
    };
    QHash<Thing*, Job> m_pendingJobs;
    QHash<Thing*, QList<Job>> m_jobQueue;
    QHash<Thing*, QTimer*> m_jobTimers;
    int m_jobIdx = 0;

    QUdpSocket *m_broadcastSocket = nullptr;
    QHash<Thing*, QTcpSocket*> m_sockets;
    QHash<ThingSetupInfo*, int> m_setupRetries;

    QHash<Thing*, QByteArray> m_inputBuffers;

    PluginTimer *m_timer = nullptr;
};

#endif // INTEGRATIONPLUGINANEL_H
