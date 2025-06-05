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

#include "everestjsonrpcdiscovery.h"
#include "extern-plugininfo.h"

EverestJsonRpcDiscovery::EverestJsonRpcDiscovery(NetworkDeviceDiscovery *networkDeviceDiscovery, quint16 port, QObject *parent)
    : QObject{parent},
    m_networkDeviceDiscovery{networkDeviceDiscovery},
    m_port{port}
{

}

void EverestJsonRpcDiscovery::start()
{
    qCInfo(dcEverest()) << "Discovery: Start discovering Everest JsonRpc instances in the network...";
    m_startDateTime = QDateTime::currentDateTime();

    NetworkDeviceDiscoveryReply *discoveryReply = m_networkDeviceDiscovery->discover();

    connect(discoveryReply, &NetworkDeviceDiscoveryReply::hostAddressDiscovered, this, &EverestJsonRpcDiscovery::checkHostAddress);
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

    NetworkDeviceInfo localHostInfo;
    checkHostAddress(QHostAddress::LocalHost);
}

void EverestJsonRpcDiscovery::startLocalhost()
{
    qCInfo(dcEverest()) << "Discovery: Start discovering EVerest on localhost ...";
    m_startDateTime = QDateTime::currentDateTime();
    m_localhostDiscovery = true;

    // For development, check local host
    NetworkDeviceInfo localHostInfo;
    checkHostAddress(QHostAddress::LocalHost);
}

QList<EverestJsonRpcDiscovery::Result> EverestJsonRpcDiscovery::results() const
{
    return m_results;
}

void EverestJsonRpcDiscovery::checkHostAddress(const QHostAddress &address)
{
    QUrl url;
    url.setScheme("ws");
    url.setHost(address.toString());
    url.setPort(m_port);

    EverestJsonRpcClient *client = new EverestJsonRpcClient(this);
    connect(client, &EverestJsonRpcClient::availableChanged, this, [this, client, address](bool available){
        if (available) {
            qCDebug(dcEverest()) << "Discovery: Found JsonRpc interface on" << client->serverUrl().toString();

            Result result;
            result.address = address;
            m_results.append(result);

            cleanupClient(client);
        }
    });

    connect(client, &EverestJsonRpcClient::connectionErrorOccurred, this, [this, client](){
        qCDebug(dcEverest()) << "Discovery: The connection to" << client->serverUrl().toString() << "failed. Skipping host";
        cleanupClient(client);
    });

    client->connectToServer(url);
}

void EverestJsonRpcDiscovery::cleanupClient(EverestJsonRpcClient *client)
{
    if (!m_clients.contains(client))
        return;

    m_clients.removeAll(client);

    client->disconnectFromServer();
    client->deleteLater();

    if (m_localhostDiscovery)
        finishDiscovery();

}

void EverestJsonRpcDiscovery::finishDiscovery()
{
    qint64 durationMilliSeconds = QDateTime::currentMSecsSinceEpoch() - m_startDateTime.toMSecsSinceEpoch();

    // Cleanup any leftovers
    foreach (EverestJsonRpcClient *client, m_clients)
        cleanupClient(client);

    // Update results with final network device infos
    for (int i = 0; i < m_results.count(); i++)
        m_results[i].networkDeviceInfo = m_networkDeviceInfos.get(m_results.at(i).address);

    qCInfo(dcEverest()) << "Discovery: Finished the discovery process. Found"
                        << m_results.count() << "Everest JsonRpc instances in"
                        << QTime::fromMSecsSinceStartOfDay(durationMilliSeconds).toString("mm:ss.zzz");

    emit finished();
}
