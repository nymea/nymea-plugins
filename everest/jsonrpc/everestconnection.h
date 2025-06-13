/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*
* Copyright 2013 - 2025, nymea GmbH
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

#ifndef EVERESTCONNECTION_H
#define EVERESTCONNECTION_H

#include <QTimer>
#include <QObject>

#include <integrations/thing.h>
#include <network/macaddress.h>
#include <network/networkdevicemonitor.h>

class EverestEvse;
class EverestJsonRpcClient;

class EverestConnection : public QObject
{
    Q_OBJECT
public:
    explicit EverestConnection(quint16 port, QObject *parent = nullptr);

    bool available() const;

    EverestJsonRpcClient *client() const;

    EverestEvse *getEvse(Thing *thing);

    void addThing(Thing *thing);
    void removeThing(Thing *thing);

    NetworkDeviceMonitor *monitor() const;
    void setMonitor(NetworkDeviceMonitor *monitor);

public slots:
    void start();
    void stop();

signals:
    void availableChanged(bool available);

private slots:
    void onMonitorReachableChanged(bool reachable);

private:
    NetworkDeviceMonitor *m_monitor = nullptr;
    QTimer m_reconnectTimer;
    quint16 m_port = 8080;

    EverestJsonRpcClient *m_client = nullptr;
    bool m_running = false;

    QHash<Thing *, EverestEvse *> m_everestEvses;
    QUrl buildUrl() const;
};

QDebug operator<<(QDebug debug, EverestConnection *connection);

#endif // EVERESTCONNECTION_H
