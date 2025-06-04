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

#ifndef EVERESTCLIENT_H
#define EVERESTCLIENT_H

#include <QTimer>
#include <QObject>

#include <mqttclient.h>

#include <integrations/thing.h>
#include <network/macaddress.h>
#include <network/networkdevicemonitor.h>

#include <QDebug>

#include "everestmqtt.h"

class EverestMqttClient : public QObject
{
    Q_OBJECT
public:
    explicit EverestMqttClient(QObject *parent = nullptr);
    ~EverestMqttClient();

    MqttClient *client() const;

    Things things() const;
    void addThing(Thing *thing);
    void removeThing(Thing *thing);

    EverestMqtt *getEverest(Thing *thing) const;

    NetworkDeviceMonitor *monitor() const;
    void setMonitor(NetworkDeviceMonitor *monitor);

public slots:
    void start();
    void stop();

private slots:
    void onMonitorReachableChanged(bool reachable);

private:
    MqttClient *m_client = nullptr;
    QTimer m_reconnectTimer;
    quint16 m_port = 1883;

    bool m_running = false;

    QHash<Thing *, EverestMqtt *> m_everests;
    NetworkDeviceMonitor *m_monitor = nullptr;
};

QDebug operator<<(QDebug debug, EverestMqttClient *everestClient);

#endif // EVERESTCLIENT_H
