/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*
* Copyright 2013 - 2024, nymea GmbH
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

#include "everestclient.h"
#include "extern-plugininfo.h"

EverestClient::EverestClient(QObject *parent)
    : QObject{parent}
{
    m_client = new MqttClient("nymea-" + QUuid::createUuid().toString().left(8), 300,
                              QString(), QByteArray(), Mqtt::QoS0, false, this);

    connect(m_client, &MqttClient::disconnected, this, [this](){
        qCDebug(dcEverest()) << "The MQTT client is now disconnected" << this;
        if (!m_address.isNull()) {
            // Start the reconnect timer
            qCDebug(dcEverest()) << "Starting reconnect timer for mqtt connection to" << m_address.toString();
            m_reconnectTimer.start();
        }
    });

    connect(m_client, &MqttClient::connected, this, [this](){
        qCDebug(dcEverest()) << "The MQTT client is now connected" << this;
        m_reconnectTimer.stop();
    });

    connect(m_client, &MqttClient::error, this, [this](QAbstractSocket::SocketError socketError){
        qCWarning(dcEverest()) << "An MQTT error occurred on" << this << socketError;
    });

    // Reconnect timer is only required for IP based connections, otherwise we have the NetworkDeviceMonitor
    m_reconnectTimer.setInterval(5000);
    m_reconnectTimer.setSingleShot(false);

    connect(&m_reconnectTimer, &QTimer::timeout, this, [this](){
        if (m_client->isConnected())
            return;

        if (!m_running) {
            qCDebug(dcEverest()) << "The everest client is not running. Ignoring event...";
            return;
        }

        m_client->connectToHost(m_address.toString(), m_port);
    });
}

EverestClient::~EverestClient()
{
    foreach (Everest *everest, m_everests) {
        removeThing(everest->thing());
    }
}

MqttClient *EverestClient::client() const
{
    return m_client;
}

Things EverestClient::things() const
{
    return m_everests.keys();
}

void EverestClient::addThing(Thing *thing)
{
    if (m_everests.contains(thing)) {
        qCWarning(dcEverest()) << "The" << thing << "has already been added to the everest client. "
                                                    "Please report a bug if you see this message.";
        // TODO: maybe cleanup and recreate the client due to reconfigure
        return;
    }

    Everest *everest = new Everest(m_client, thing, this);
    m_everests.insert(thing, everest);
}

void EverestClient::removeThing(Thing *thing)
{
    if (!m_everests.contains(thing)) {
        qCWarning(dcEverest()) << "The" << thing << "has already been removed from the everest client. "
                                                    "Please report a bug if you see this message.";
        return;
    }

    Everest *everest = m_everests.take(thing);
    everest->deinitialize();
    everest->deleteLater();
}

Everest *EverestClient::getEverest(Thing *thing) const
{
    if (!m_everests.contains(thing))
        return nullptr;

    return m_everests.value(thing);
}

QHostAddress EverestClient::address() const
{
    return m_address;
}

void EverestClient::setAddress(const QHostAddress &address)
{
    m_address = address;
}

MacAddress EverestClient::macAddress() const
{
    return m_macAddress;
}

void EverestClient::setMacAddress(const MacAddress &macAddress)
{
    m_macAddress = macAddress;
}

NetworkDeviceMonitor *EverestClient::monitor() const
{
    return m_monitor;
}

void EverestClient::setMonitor(NetworkDeviceMonitor *monitor)
{
    m_monitor = monitor;
    connect(m_monitor, &NetworkDeviceMonitor::reachableChanged, this, &EverestClient::onMonitorReachableChanged);
}

void EverestClient::start()
{
    qCDebug(dcEverest()) << "Starting" << this;
    m_running = true;

    if (m_monitor) {
        if (m_monitor->reachable()) {
            qCDebug(dcEverest()) << "Connecting MQTT client to" << m_monitor->networkDeviceInfo();
            if (m_client->isConnected())
                m_client->disconnectFromHost();

            m_client->connectToHost(m_monitor->networkDeviceInfo().address().toString(), m_port);
        }
    } else {
        qCDebug(dcEverest()) << "Connecting MQTT client to" << m_address.toString();
        m_client->connectToHost(m_address.toString(), m_port);

        // Note: on connected this will be stopped, otherwise we want the timer running
        m_reconnectTimer.start();
    }
}

void EverestClient::stop()
{
    qCDebug(dcEverest()) << "Stopping" << this;
    m_running = false;
    m_reconnectTimer.stop();
    m_client->disconnectFromHost();
}

void EverestClient::onMonitorReachableChanged(bool reachable)
{
    qCDebug(dcEverest()) << "Network monitor for" << m_monitor->macAddress().toString()
    << (reachable ? " is now reachable" : "is not reachable any more");

    if (!m_running) {
        qCDebug(dcEverest()) << "The everest client is not running. Ignoring event...";
        return;
    }

    if (reachable) {
        // The monitor is reachable which means we have a verfied IP, (re)connecting the mqtt client
        qCDebug(dcEverest()) << "Connecting MQTT client to" << m_monitor->networkDeviceInfo();
        if (m_client->isConnected())
            m_client->disconnectFromHost();

        m_client->connectToHost(m_monitor->networkDeviceInfo().address().toString(), m_port);
    }
}

QDebug operator<<(QDebug debug, EverestClient *everestClient)
{
    QDebugStateSaver saver(debug);
    debug.nospace() << "EverestClient(";
    if (everestClient->monitor()) {
        debug.nospace() << everestClient->monitor()->networkDeviceInfo().macAddress() << ", ";
        debug.nospace() << everestClient->monitor()->networkDeviceInfo().address().toString() << ", ";
    } else {
        debug.nospace() << everestClient->address().toString() << ", ";
    }

    debug.nospace() << "MQTT connected: " << everestClient->client()->isConnected() << ")";
    return debug;
}
