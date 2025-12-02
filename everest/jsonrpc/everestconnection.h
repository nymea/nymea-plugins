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
