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

#include "kebadiscovery.h"
#include "kecontactdatalayer.h"
#include "extern-plugininfo.h"

#include <QJsonDocument>
#include <network/networkdevicediscovery.h>

KebaDiscovery::KebaDiscovery(KeContactDataLayer *kebaDataLayer, NetworkDeviceDiscovery *networkDeviceDiscovery,  QObject *parent) :
    QObject(parent),
    m_kebaDataLayer(kebaDataLayer),
    m_networkDeviceDiscovery(networkDeviceDiscovery)
{
    // Timer for waiting if network devices responded to the "report 1 request"
    m_responseTimer.setInterval(2000);
    m_responseTimer.setSingleShot(true);
    connect(&m_responseTimer, &QTimer::timeout, this, [=](){

        // Fill in all network device infos we have
        for (int i = 0; i < m_results.count(); i++) {
            m_results[i].networkDeviceInfo = m_networkDeviceInfos.get(m_results.at(i).address);
        }

        qCInfo(dcKeba()) << "Discovery: Finished successfully. Found" << m_results.count() << "Keba Wallbox";
        emit discoveryFinished();
    });

    // Read data from the keba data layer and verify if it is a keba report
    connect (m_kebaDataLayer, &KeContactDataLayer::datagramReceived, this, [=](const QHostAddress &address, const QByteArray &datagram){

        // Try to convert the received data to a json document
        QJsonParseError error;
        QJsonDocument jsonDoc = QJsonDocument::fromJson(datagram, &error);
        if (error.error != QJsonParseError::NoError) {
            qCWarning(dcKeba()) << "Discovery: Received data from the keba data link but failed to parse the data as JSON:" << datagram << ":" << error.errorString();
            return;
        }

        // Verify JSON data
        QVariantMap dataMap = jsonDoc.toVariant().toMap();
        if (!dataMap.contains("ID") || !dataMap.contains("Serial") || !dataMap.contains("Product") || !dataMap.contains("Firmware")) {
            qCDebug(dcKeba()) << "Discovery: Received valid JSON data on data layer but they don't seem to be what we are listening for:" << qUtf8Printable(jsonDoc.toJson());
            return;
        }

        if (dataMap.value("ID").toInt() != 1) {
            qCDebug(dcKeba()) << "Discovery: Received valid Keba JSON data on data layer but this is not a report 1 we requested for:" << qUtf8Printable(jsonDoc.toJson());
            return;
        }

        // We have received a report 1 datagram, let's add it to the result
        KebaDiscoveryResult result;
        result.address = address;
        result.product = dataMap.value("Product").toString();
        result.serialNumber = dataMap.value("Serial").toString();
        result.firmwareVersion = dataMap.value("Firmware").toString();

        bool alreadyDiscovered = false;
        foreach (const KebaDiscoveryResult &r, m_results) {
            if (r.serialNumber == result.serialNumber) {
                alreadyDiscovered = true;
                break;
            }
        }

        if (!alreadyDiscovered) {
            m_results.append(result);
            qCDebug(dcKeba()) << "Discovery: -->" << address.toString() << result.product << result.serialNumber << result.firmwareVersion;
        }
    });
}

KebaDiscovery::~KebaDiscovery()
{
    qCDebug(dcKeba()) << "Discovery: Destructing";
}

void KebaDiscovery::startDiscovery()
{
    // Clean up
    cleanup();

    qCInfo(dcKeba()) << "Discovery: Start searching for Keba wallboxes in the network...";
    NetworkDeviceDiscoveryReply *discoveryReply = m_networkDeviceDiscovery->discover();

    // Imedialty check any new device gets discovered
    connect(discoveryReply, &NetworkDeviceDiscoveryReply::hostAddressDiscovered, this, &KebaDiscovery::sendReportRequest);

    // Check what might be left on finished
    connect(discoveryReply, &NetworkDeviceDiscoveryReply::finished, discoveryReply, &NetworkDeviceDiscoveryReply::deleteLater);
    connect(discoveryReply, &NetworkDeviceDiscoveryReply::finished, this, [=](){
        qCDebug(dcKeba()) << "Discovery: Network discovery finished. Found" << discoveryReply->networkDeviceInfos().count() << "network devices";
        m_networkDeviceInfos = discoveryReply->networkDeviceInfos();

        qCDebug(dcKeba()) << "Discovery: Network discovery finished. Start finishing discovery...";
        m_responseTimer.start();
    });
}

QList<KebaDiscovery::KebaDiscoveryResult> KebaDiscovery::discoveryResults() const
{
    return m_results;
}

void KebaDiscovery::sendReportRequest(const QHostAddress &address)
{
    m_verifiedAddresses.append(address);
    m_kebaDataLayer->write(address, QByteArray("report 1\n"));
}

void KebaDiscovery::cleanup()
{
    m_networkDeviceInfos.clear();
    m_verifiedAddresses.clear();
    m_results.clear();
}


