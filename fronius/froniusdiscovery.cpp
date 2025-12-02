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
    connect(discoveryReply, &NetworkDeviceDiscoveryReply::hostAddressDiscovered, this, &FroniusDiscovery::checkHostAddress);
    connect(discoveryReply, &NetworkDeviceDiscoveryReply::finished, this, [this, discoveryReply](){

        m_networkDeviceInfos = discoveryReply->networkDeviceInfos();

        qCDebug(dcFronius()) << "Discovery: Network discovery finished. Found" << discoveryReply->networkDeviceInfos().count() << "network devices";
        m_gracePeriodTimer.start();

        discoveryReply->deleteLater();
    });
}

QList<NetworkDeviceInfo> FroniusDiscovery::discoveryResults() const
{
    return m_discoveryResults;
}

void FroniusDiscovery::checkHostAddress(const QHostAddress &address)
{
    qCDebug(dcFronius()) << "Discovery: Checking host address" << address.toString();

    FroniusSolarConnection *connection = new FroniusSolarConnection(m_networkManager, address, this);
    m_connections.append(connection);

    FroniusNetworkReply *reply = connection->getVersion();
    connect(reply, &FroniusNetworkReply::finished, this, [=] {
        QByteArray data = reply->networkReply()->readAll();
        if (reply->networkReply()->error() != QNetworkReply::NoError) {
            if (reply->networkReply()->error() == QNetworkReply::ContentNotFoundError) {
                qCInfo(dcFronius()) << "Discovery: The device on" << address.toString() << "does not reply to our requests. Please verify that the Fronius Solar API is enabled on the device.";
            } else {
                qCDebug(dcFronius()) << "Discovery: Reply finished with error on" << address.toString() << reply->networkReply()->errorString();
            }
            cleanupConnection(connection);
            return;
        }

        QJsonParseError error;
        QJsonDocument jsonDoc = QJsonDocument::fromJson(data, &error);
        if (error.error != QJsonParseError::NoError) {
            qCDebug(dcFronius()) << "Discovery: Failed to parse JSON data from" << address.toString() << ":" << error.errorString() << data;
            cleanupConnection(connection);
            return;
        }

        QVariantMap versionResponseMap = jsonDoc.toVariant().toMap();
        if (!versionResponseMap.contains("CompatibilityRange")) {
            qCDebug(dcFronius()) << "Discovery: Unexpected JSON reply from" << address.toString() << "Probably not a Fronius device.";
            cleanupConnection(connection);
            return;
        }
        qCDebug(dcFronius()) << "Discovery: Compatibility version" << versionResponseMap.value("CompatibilityRange").toString();

        // Knwon version with broken JSON API. Still allowing to discover so the user will get a proper error message during setup
        if (versionResponseMap.value("CompatibilityRange").toString() == "1.6-2") {
            qCWarning(dcFronius()) << "Discovery: The Fronius data logger has a version which is known to have a broken JSON API firmware.";
        }

        m_discoveredAddresses.append(address);
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

    foreach (FroniusSolarConnection *connection, m_connections)
        cleanupConnection(connection);

    foreach (const QHostAddress &address, m_discoveredAddresses)
        m_discoveryResults.append(m_networkDeviceInfos.get(address));

    qCDebug(dcFronius()) << "Discovery: Finished the discovery process. Found" << m_discoveryResults.count()
                         << "Fronius devices in" << QTime::fromMSecsSinceStartOfDay(durationMilliSeconds).toString("mm:ss.zzz");
    m_gracePeriodTimer.stop();

    emit discoveryFinished();
}
