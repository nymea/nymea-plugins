/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*
* Copyright 2013 - 2021, nymea GmbH
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
    m_responseTimer.setInterval(5000);
    m_responseTimer.setSingleShot(true);
    connect(&m_responseTimer, &QTimer::timeout, this, [=](){
        qCDebug(dcKeba()) << "Discovery: Report response timeout. Found" << m_results.count() << "Keba Wallbox";
        emit discoveryFinished();
    });

    // Read data from the keba data layer and verify if it is a keba report
    connect (m_kebaDataLayer, &KeContactDataLayer::datagramReceived, this, [=](const QHostAddress &address, const QByteArray &datagram){

        // Just continue if this is a new address we have no result for
        if (alreadyDiscovered(address)) {
            qCDebug(dcKeba()) << "Discovery: Skipping datagram from already discovered Keba on" << address.toString();
            return;
        }

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
            qCDebug(dcKeba()) << "Discovery: Received valid Keba JSON data on data layer but this is not a report 1 message:" << qUtf8Printable(jsonDoc.toJson());
            return;
        }

        // We have received a report 1 datagram, let's add it to the result
        foreach (const NetworkDeviceInfo &networkDeviceInfo, m_networkDeviceInfos) {
            if (networkDeviceInfo.address() == address) {
                KebaDiscoveryResult result;
                result.networkDeviceInfo = networkDeviceInfo;
                result.product = dataMap.value("Product").toString();
                result.serialNumber = dataMap.value("Serial").toString();
                result.firmwareVersion = dataMap.value("Firmware").toString();
                m_results.append(result);
                qCDebug(dcKeba()) << "Discovery: -->" << networkDeviceInfo.address().toString() << networkDeviceInfo.macAddress() << result.product << result.serialNumber << result.firmwareVersion;
            }
        }
    });

}

KebaDiscovery::~KebaDiscovery()
{
    qCDebug(dcKeba()) << "Discovery: Destroying object.";
}

void KebaDiscovery::startDiscovery()
{
    // Clean up
    m_networkDeviceInfos.clear();
    m_results.clear();

    qCDebug(dcKeba()) << "Discovery: Start discovering Keba Wallboxs...";
    NetworkDeviceDiscoveryReply *discoveryReply = m_networkDeviceDiscovery->discover();
    connect(discoveryReply, &NetworkDeviceDiscoveryReply::finished, this, [=](){
        qCDebug(dcKeba()) << "Discovery: Network discovery finished. Found" << discoveryReply->networkDeviceInfos().count() << "network devices";
        m_networkDeviceInfos = discoveryReply->networkDeviceInfos();

        // Send a report 1 request to all discovered network devices and see which one responds
        qCDebug(dcKeba()) << "Discovery: Start sending \"report 1\" request to all discovered network devices";
        foreach (const NetworkDeviceInfo &networkDeviceInfo, m_networkDeviceInfos)
            m_kebaDataLayer->write(networkDeviceInfo.address(), QByteArray("report 1\n"));

        m_responseTimer.start();
    });
}

QList<KebaDiscovery::KebaDiscoveryResult> KebaDiscovery::discoveryResults() const
{
    return m_results;
}

bool KebaDiscovery::alreadyDiscovered(const QHostAddress &address)
{
    foreach (const KebaDiscoveryResult &result, m_results) {
        if (result.networkDeviceInfo.address() == address) {
            return true;
        }
    }

    return false;
}
