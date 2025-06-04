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

#include "everestmqttdiscovery.h"
#include "extern-plugininfo.h"

#include <QJsonDocument>
#include <QJsonParseError>


EverestMqttDiscovery::EverestMqttDiscovery(NetworkDeviceDiscovery *networkDeviceDiscovery, QObject *parent)
    : QObject{parent},
    m_networkDeviceDiscovery{networkDeviceDiscovery}
{

}

void EverestMqttDiscovery::start()
{
    qCInfo(dcEverest()) << "Discovery: Start discovering Everest MQTT brokers in the network...";
    m_startDateTime = QDateTime::currentDateTime();

    NetworkDeviceDiscoveryReply *discoveryReply = m_networkDeviceDiscovery->discover();

    connect(discoveryReply, &NetworkDeviceDiscoveryReply::hostAddressDiscovered, this, &EverestMqttDiscovery::checkHostAddress);
    connect(discoveryReply, &NetworkDeviceDiscoveryReply::finished, discoveryReply, &NetworkDeviceDiscoveryReply::deleteLater);
    connect(discoveryReply, &NetworkDeviceDiscoveryReply::finished, this, [discoveryReply, this](){
        qCDebug(dcEverest()) << "Discovery: Network device discovery finished. Found" << discoveryReply->networkDeviceInfos().count() << "network devices";
        m_networkDeviceInfos = discoveryReply->networkDeviceInfos();

        // Give the last connections added right before the network discovery finished a chance to check the device...
        QTimer::singleShot(3000, this, [this](){
            qCDebug(dcEverest()) << "Discovery: Grace period timer triggered.";
            finishDiscovery();
        });
    });

    // For development, check local host
    NetworkDeviceInfo localHostInfo;
    checkHostAddress(QHostAddress::LocalHost);
}

void EverestMqttDiscovery::startLocalhost()
{
    qCInfo(dcEverest()) << "Discovery: Start discovering EVerest on localhost ...";
    m_startDateTime = QDateTime::currentDateTime();
    m_localhostDiscovery = true;

    // For development, check local host
    NetworkDeviceInfo localHostInfo;
    checkHostAddress(QHostAddress::LocalHost);
}

QList<EverestMqttDiscovery::Result> EverestMqttDiscovery::results() const
{
    return m_results;
}

void EverestMqttDiscovery::checkHostAddress(const QHostAddress &address)
{
    MqttClient *client = new MqttClient("nymea-" + QUuid::createUuid().toString().left(8), 300,
                                        QString(), QByteArray(), Mqtt::QoS0, false, this);
    client->setAutoReconnect(false);

    m_clients.append(client);

    connect(client, &MqttClient::error, this, [this, client, address](QAbstractSocket::SocketError socketError){
        qCDebug(dcEverest()) << "Discovery: MQTT client error occurred on"
                             << address.toString() << socketError
                             << "...skip connection";
        // We give up on the first error here
        cleanupClient(client);

        if (m_localhostDiscovery) {
            finishDiscovery();
        }
    });

    connect(client, &MqttClient::disconnected, this, [this, client](){
        cleanupClient(client);
    });

    connect(client, &MqttClient::connected, this, [this, client, address](){
        // We found a mqtt server, let's check if we find everest_api module on it...
        qCDebug(dcEverest()) << "Discovery: Successfully connected to host" << address.toString();

        connect(client, &MqttClient::publishReceived, client, [this, client, address]
                (const QString &topic, const QByteArray &payload, bool retained) {

                    qCDebug(dcEverest()) << "Discovery: Received publish on" << topic
                                         << "retained:" << retained << qUtf8Printable(payload);

                    if (topic == m_everestApiModuleTopicConnectors) {
                        QJsonParseError jsonError;
                        QJsonDocument jsonDoc = QJsonDocument::fromJson(payload, &jsonError);
                        if (jsonError.error) {
                            qCDebug(dcEverest()) << "Discovery: Received payload on topic" << topic
                                                 << "with JSON error:" << jsonError.errorString();
                            cleanupClient(client);
                            return;
                        }

                        QStringList connectors = jsonDoc.toVariant().toStringList();
                        qCInfo(dcEverest()) << "Discovery: Found Everest on" << address.toString() << connectors;
                        Result result;
                        result.address = address;
                        result.connectors = connectors;
                        m_results.append(result);

                        cleanupClient(client);
                    }
                });

        connect(client, &MqttClient::subscribeResult, client, [this, client]
                (quint16 packetId, const Mqtt::SubscribeReturnCodes &subscribeReturnCodes) {
                    Q_UNUSED(packetId)
                    if (subscribeReturnCodes.contains(Mqtt::SubscribeReturnCodeFailure)) {
                        qCDebug(dcEverest()) << "Discovery: Failed to subscribe to topic ...skip connection";
                        cleanupClient(client);
                    }
                });

        client->subscribe(m_everestApiModuleTopicConnectors);

        // // Subscribe in the next event loop due to IO error on socket
        // QTimer::singleShot(100, client, [this, client](){
        //     qCDebug(dcEverest()) << "Discovery: Try to subscribe to the API module on the broker...";
        //     client->subscribe(m_everestApiModuleTopicConnectors);
        // });
    });

    qCDebug(dcEverest()) << "Discovery: Verifying host" << address.toString();
    client->connectToHost(address.toString(), 1883);
}


void EverestMqttDiscovery::cleanupClient(MqttClient *client)
{
    if (!m_clients.contains(client))
        return;

    m_clients.removeAll(client);

    client->disconnectFromHost();
    client->deleteLater();

    if (m_localhostDiscovery) {
        finishDiscovery();
    }
}

void EverestMqttDiscovery::finishDiscovery()
{
    qint64 durationMilliSeconds = QDateTime::currentMSecsSinceEpoch() - m_startDateTime.toMSecsSinceEpoch();

    // Cleanup any leftovers...we don't care any more
    foreach (MqttClient *client, m_clients)
        cleanupClient(client);

    // Update results with final network device infos
    for (int i = 0; i < m_results.count(); i++)
        m_results[i].networkDeviceInfo = m_networkDeviceInfos.get(m_results.at(i).address);

    qCInfo(dcEverest()) << "Discovery: Finished the discovery process. Found"
                        << m_results.count() << "Everest instances in"
                        << QTime::fromMSecsSinceStartOfDay(durationMilliSeconds).toString("mm:ss.zzz");

    emit finished();
}
