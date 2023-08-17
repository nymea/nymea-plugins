/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*
* Copyright 2013 - 2023, nymea GmbH
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

#include "froniusdiscovery.h"

#include "extern-plugininfo.h"

#include <QJsonDocument>
#include <QJsonParseError>

FroniusDiscovery::FroniusDiscovery(NetworkAccessManager *networkManager, NetworkDeviceDiscovery *networkDeviceDiscovery, QObject *parent):
    QObject(parent),
    m_networkManager(networkManager),
    m_networkDeviceDiscovery{networkDeviceDiscovery}
{
    m_gracePeriodTimer.setSingleShot(true);
    m_gracePeriodTimer.setInterval(3000);
    connect(&m_gracePeriodTimer, &QTimer::timeout, this, [this](){
        qCDebug(dcFronius()) << "Discovery: Grace period timer triggered.";
        finishDiscovery();
    });

}

void FroniusDiscovery::startDiscovery()
{
    qCDebug(dcFronius()) << "Discovery: Searching for Fronius solar devices in the network...";
    m_startDateTime = QDateTime::currentDateTime();

    NetworkDeviceDiscoveryReply *discoveryReply = m_networkDeviceDiscovery->discover();
    connect(discoveryReply, &NetworkDeviceDiscoveryReply::networkDeviceInfoAdded, this, &FroniusDiscovery::checkNetworkDevice);
    connect(discoveryReply, &NetworkDeviceDiscoveryReply::finished, this, [=](){
        qCDebug(dcFronius()) << "Discovery: Network discovery finished. Found" << discoveryReply->networkDeviceInfos().count() << "network devices";
        m_gracePeriodTimer.start();
        discoveryReply->deleteLater();
    });
}

QList<NetworkDeviceInfo> FroniusDiscovery::discoveryResults() const
{
    return m_discoveryResults;
}

void FroniusDiscovery::checkNetworkDevice(const NetworkDeviceInfo &networkDeviceInfo)
{
    qCDebug(dcFronius()) << "Discovery: Checking network device:" << networkDeviceInfo;

    FroniusSolarConnection *connection = new FroniusSolarConnection(m_networkManager, networkDeviceInfo.address(), this);
    m_connections.append(connection);

    FroniusNetworkReply *reply = connection->getVersion();
    connect(reply, &FroniusNetworkReply::finished, this, [=] {
        QByteArray data = reply->networkReply()->readAll();
        if (reply->networkReply()->error() != QNetworkReply::NoError) {
            if (reply->networkReply()->error() == QNetworkReply::ContentNotFoundError) {
                qCInfo(dcFronius()) << "Discovery: The device on" << networkDeviceInfo.address().toString() << "does not reply to our requests. Please verify that the Fronius Solar API is enabled on the device.";
            } else {
                qCDebug(dcFronius()) << "Discovery: Reply finished with error on" << networkDeviceInfo.address().toString() << reply->networkReply()->errorString();
            }
            cleanupConnection(connection);
            return;
        }

        QJsonParseError error;
        QJsonDocument jsonDoc = QJsonDocument::fromJson(data, &error);
        if (error.error != QJsonParseError::NoError) {
            qCDebug(dcFronius()) << "Discovery: Failed to parse JSON data from" << networkDeviceInfo.address().toString() << ":" << error.errorString() << data;
            cleanupConnection(connection);
            return;
        }

        QVariantMap versionResponseMap = jsonDoc.toVariant().toMap();
        if (!versionResponseMap.contains("CompatibilityRange")) {
            qCDebug(dcFronius()) << "Discovery: Unexpected JSON reply from" << networkDeviceInfo.address().toString() << "Probably not a Fronius device.";
            cleanupConnection(connection);
            return;
        }
        qCDebug(dcFronius()) << "Discovery: Compatibility version" << versionResponseMap.value("CompatibilityRange").toString();

        // Knwon version with broken JSON API. Still allowing to discover so the user will get a proper error message during setup
        if (versionResponseMap.value("CompatibilityRange").toString() == "1.6-2") {
            qCWarning(dcFronius()) << "Discovery: The Fronius data logger has a version which is known to have a broken JSON API firmware.";
        }

        m_discoveryResults.append(networkDeviceInfo);
        cleanupConnection(connection);
    });
}

void FroniusDiscovery::cleanupConnection(FroniusSolarConnection *connection)
{
    m_connections.removeAll(connection);
    connection->deleteLater();
}

void FroniusDiscovery::finishDiscovery()
{
    qint64 durationMilliSeconds = QDateTime::currentMSecsSinceEpoch() - m_startDateTime.toMSecsSinceEpoch();

    foreach (FroniusSolarConnection *connection, m_connections) {
        cleanupConnection(connection);
    }

    qCDebug(dcFronius()) << "Discovery: Finished the discovery process. Found" << m_discoveryResults.count()
                       << "Fronius devices in" << QTime::fromMSecsSinceStartOfDay(durationMilliSeconds).toString("mm:ss.zzz");
    m_gracePeriodTimer.stop();

    emit discoveryFinished();

}
