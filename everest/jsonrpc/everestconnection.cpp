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

#include "everestconnection.h"
#include "extern-plugininfo.h"
#include "jsonrpc/everestjsonrpcclient.h"

EverestConnection::EverestConnection(quint16 port, QObject *parent)
    : QObject{parent},
    m_port{port}
{
    m_client = new EverestJsonRpcClient(this);

    connect(m_client, &EverestJsonRpcClient::availableChanged, this, &EverestConnection::availableChanged);
    connect(m_client, &EverestJsonRpcClient::availableChanged, this, [this](bool available){
        if (available) {
            qCDebug(dcEverest()) << "The JsonRpc client is now available" << this;
            m_reconnectTimer.stop();
        } else {
            qCDebug(dcEverest()) << "The JsonRpc client is not available any more" << this;
            if (m_monitor->reachable()) {
                // Start the reconnect timer
                qCDebug(dcEverest()) << "Starting reconnect timer for JsonRpc client to" << m_client->serverUrl().toString();
                m_reconnectTimer.start();
            }
        }
    });

    // Reconnect timer is only required for IP based connections, otherwise we have the NetworkDeviceMonitor
    m_reconnectTimer.setInterval(5000);
    m_reconnectTimer.setSingleShot(false);

    connect(&m_reconnectTimer, &QTimer::timeout, this, [this](){
        if (m_client->available())
            return;

        if (!m_running) {
            qCDebug(dcEverest()) << "The everest client is not running. Ignoring event...";
            return;
        }

        m_client->connectToServer(buildUrl());
    });
}

bool EverestConnection::available() const
{
    return m_client->available();
}

EverestJsonRpcClient *EverestConnection::client() const
{
    return m_client;
}

Things EverestConnection::things() const
{
    return Things();
}

void EverestConnection::addThing(Thing *thing)
{
    Q_UNUSED(thing)
}

void EverestConnection::removeThing(Thing *thing)
{
    Q_UNUSED(thing)
}

NetworkDeviceMonitor *EverestConnection::monitor() const
{
    return m_monitor;
}

void EverestConnection::setMonitor(NetworkDeviceMonitor *monitor)
{
    if (!monitor && m_monitor)
        disconnect(m_monitor, &NetworkDeviceMonitor::reachableChanged, this, &EverestConnection::onMonitorReachableChanged);

    m_monitor = monitor;
    if (m_monitor) {
        connect(m_monitor, &NetworkDeviceMonitor::reachableChanged, this, &EverestConnection::onMonitorReachableChanged);
    }
}

void EverestConnection::start()
{
    qCDebug(dcEverest()) << "Starting" << this;
    m_running = true;

    if (m_monitor) {
        if (m_monitor->reachable()) {
            QUrl url = buildUrl();
            qCDebug(dcEverest()) << "Connecting JsonRpc client to" << url.toString();
            if (m_client->available())
                m_client->disconnectFromServer();

            m_client->connectToServer(url);
        }
    } else {
        qCDebug(dcEverest()) << "Connecting MQTT client to" << m_monitor->networkDeviceInfo().address().toString();
        m_client->connectToServer(buildUrl());

        // Note: on connected this will be stopped, otherwise we want the timer running
        m_reconnectTimer.start();
    }
}

void EverestConnection::stop()
{
    qCDebug(dcEverest()) << "Stopping" << this;
    m_running = false;
    m_reconnectTimer.stop();
    m_client->disconnectFromServer();
}

void EverestConnection::onMonitorReachableChanged(bool reachable)
{
    qCDebug(dcEverest()) << "Network monitor for" << m_monitor << (reachable ? " is now reachable" : "is not reachable any more");

    if (!m_running) {
        qCDebug(dcEverest()) << "The everest client is not running. Ignoring event...";
        return;
    }

    if (reachable) {
        // The monitor is reachable which means we have a verfied IP, (re)connecting the JsonRpc client
        QUrl url = buildUrl();
        qCDebug(dcEverest()) << "Connecting JsonRpc client to" << url.toString();
        if (m_client->available())
            m_client->disconnectFromServer();

        m_client->connectToServer(url);
    } else {
        m_reconnectTimer.stop();
    }
}

QUrl EverestConnection::buildUrl() const
{
    QUrl url;
    url.setScheme("ws");
    url.setHost(m_monitor->networkDeviceInfo().address().toString());
    url.setPort(m_port);
    return url;
}
