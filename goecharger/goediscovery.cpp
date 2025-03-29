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

#include "goediscovery.h"
#include "extern-plugininfo.h"

#include <QJsonDocument>
#include <QJsonParseError>

GoeDiscovery::GoeDiscovery(NetworkAccessManager *networkAccessManager, NetworkDeviceDiscovery *networkDeviceDiscovery, ZeroConfServiceBrowser *serviceBrowser, QObject *parent) :
    QObject{parent},
    m_networkAccessManager{networkAccessManager},
    m_networkDeviceDiscovery{networkDeviceDiscovery},
    m_serviceBrowser{serviceBrowser}
{

}

GoeDiscovery::~GoeDiscovery()
{
    qCDebug(dcGoECharger()) << "Discovery: destroy discovery object";
    cleanupPendingReplies();
}

void GoeDiscovery::startDiscovery()
{
    // Clean up
    m_discoveryResults.clear();
    m_verifiedHostAddresses.clear();

    m_startDateTime = QDateTime::currentDateTime();

    qCInfo(dcGoECharger()) << "Discovery: Start discovering the network...";

    // ZeroConf
    connect(m_serviceBrowser, &ZeroConfServiceBrowser::serviceEntryAdded, this, &GoeDiscovery::onServiceEntryAdded);
    foreach (const ZeroConfServiceEntry &serviceEntry, m_serviceBrowser->serviceEntries()) {
        onServiceEntryAdded(serviceEntry);
    }

    // Network discovery
    m_discoveryReply = m_networkDeviceDiscovery->discover();

    // Test any network device beeing discovered
    connect(m_discoveryReply, &NetworkDeviceDiscoveryReply::hostAddressDiscovered, this, &GoeDiscovery::checkHostAddress);

    // When the network discovery has finished, we process the rest and give some time to finish the pending replies
    connect(m_discoveryReply, &NetworkDeviceDiscoveryReply::finished, this, [this](){
        // The network device discovery is done
        m_discoveredNetworkDeviceInfos = m_discoveryReply->networkDeviceInfos();
        m_discoveryReply->deleteLater();
        m_discoveryReply = nullptr;

        // If there might be some response after the grace period time,
        // we don't care any more since there might just waiting for some timeouts...
        // If there would be a device, it would have responded.
        QTimer::singleShot(3000, this, [this](){
            qCDebug(dcGoECharger()) << "Discovery: Grace period timer triggered.";
            finishDiscovery();
        });
    });
}

QList<GoeDiscovery::Result> GoeDiscovery::discoveryResults() const
{
    return m_discoveryResults.values();
}

QNetworkRequest GoeDiscovery::buildRequestV1(const QHostAddress &address)
{
    QUrl requestUrl;
    requestUrl.setScheme("http");
    requestUrl.setHost(address.toString());
    requestUrl.setPath("/status");

    return QNetworkRequest(requestUrl);
}

QNetworkRequest GoeDiscovery::buildRequestV2(const QHostAddress &address)
{
    QUrl requestUrl;
    requestUrl.setScheme("http");
    requestUrl.setHost(address.toString());
    requestUrl.setPath("/api/status");

    return QNetworkRequest(requestUrl);
}

bool GoeDiscovery::isGoeCharger(const ZeroConfServiceEntry &serviceEntry)
{
    return serviceEntry.name().toLower().contains("go-echarger");
}

void GoeDiscovery::checkHostAddress(const QHostAddress &address)
{
    // Make sure we have not checked this host yet
    if (m_verifiedHostAddresses.contains(address))
        return;

    qCDebug(dcGoECharger()) << "Discovery: Start inspecting" << address.toString();
    checkHostAddressApiV2(address);
    checkHostAddressApiV1(address);

    m_verifiedHostAddresses.append(address);
}

void GoeDiscovery::checkHostAddressApiV1(const QHostAddress &address)
{
    // Check if API V1 is available: http://<host>/status
    QNetworkReply *reply = m_networkAccessManager->get(buildRequestV1(address));
    m_pendingReplies.append(reply);
    connect(reply, &QNetworkReply::finished, this, [=](){
        m_pendingReplies.removeAll(reply);
        reply->deleteLater();

        if (reply->error() != QNetworkReply::NoError) {
            qCDebug(dcGoECharger()) << "Discovery:" << address.toString() << "API V1 verification HTTP error" << reply->errorString() << "Continue...";
            return;
        }

        QByteArray data = reply->readAll();
        QJsonParseError error;
        QJsonDocument jsonDoc = QJsonDocument::fromJson(data, &error);
        if (error.error != QJsonParseError::NoError) {
            qCDebug(dcGoECharger()) << "Discovery:" << address.toString() << "API V1 verification invalid JSON data. Continue...";
            return;
        }

        // Verify if we have the required values in the response map
        // https://github.com/goecharger/go-eCharger-API-v1/blob/master/go-eCharger%20API%20v1%20EN.md
        QVariantMap responseMap = jsonDoc.toVariant().toMap();
        if (responseMap.contains("fwv") && responseMap.contains("sse") && responseMap.contains("nrg") && responseMap.contains("amp")) {
            // Looks like we have found a go-e V1 api endpoint, nice
            qCDebug(dcGoECharger()) << "Discovery: --> Found API V1 on" << address.toString();

            if (m_discoveryResults.contains(address) && m_discoveryResults.value(address).discoveryMethod == DiscoveryMethodZeroConf) {
                qCDebug(dcGoECharger()) << "Discovery: Network discovery found API V1 go-eCharger on" << address.toString()
                << "but this host has already been discovered using ZeroConf. Prefering ZeroConf over MAC address due to Repeater missbehaviours.";
                return;
            }

            if (m_discoveryResults.contains(address)) {
                // We use the information from API V2 since there are more information available
                m_discoveryResults[address].apiAvailableV1 = true;
            } else {
                GoeDiscovery::Result result;
                result.serialNumber = responseMap.value("sse").toString();
                result.firmwareVersion = responseMap.value("fwv").toString();
                //result.networkDeviceInfo = networkDeviceInfo;
                result.apiAvailableV1 = true;
                m_discoveryResults[address] = result;
            }
        } else {
            qCDebug(dcGoECharger()) << "Discovery:" << address.toString() << "API V1 verification returned JSON data but not the right one. Continue...";
        }

    });
}

void GoeDiscovery::checkHostAddressApiV2(const QHostAddress &address)
{
    // Check if API V2 is available: http://<host>/api/status
    qCDebug(dcGoECharger()) << "Discovery: verify API V2 on" << address.toString();
    QNetworkReply *reply = m_networkAccessManager->get(buildRequestV2(address));
    m_pendingReplies.append(reply);
    connect(reply, &QNetworkReply::finished, this, [=](){
        m_pendingReplies.removeAll(reply);
        reply->deleteLater();

        if (reply->error() != QNetworkReply::NoError) {
            qCDebug(dcGoECharger()) << "Discovery:" << address.toString() << "API V2 verification HTTP error" << reply->errorString() << "Continue...";
            return;
        }

        QByteArray data = reply->readAll();
        QJsonParseError error;
        QJsonDocument jsonDoc = QJsonDocument::fromJson(data, &error);
        if (error.error != QJsonParseError::NoError) {
            qCDebug(dcGoECharger()) << "Discovery:" << address.toString() << "API V2 verification invalid JSON data. Continue...";
            return;
        }

        // Verify if we have the required values in the response map
        // https://github.com/goecharger/go-eCharger-API-v2/blob/main/http-en.md
        QVariantMap responseMap = jsonDoc.toVariant().toMap();
        if (responseMap.contains("fwv") && responseMap.contains("sse") && responseMap.contains("typ") && responseMap.contains("fna")) {
            // Looks like we have found a go-e V2 api endpoint, nice
            qCDebug(dcGoECharger()) << "Discovery: --> Found API V2 on" << address.toString();

            GoeDiscovery::Result result;
            result.serialNumber = responseMap.value("sse").toString();
            result.firmwareVersion = responseMap.value("fwv").toString();
            result.manufacturer = responseMap.value("oem").toString();
            result.product = responseMap.value("typ").toString();
            result.friendlyName = responseMap.value("fna").toString();
            //result.networkDeviceInfo = networkDeviceInfo;
            result.discoveryMethod = DiscoveryMethodNetwork;
            result.apiAvailableV2 = true;

            if (m_discoveryResults.contains(address) && m_discoveryResults.value(address).discoveryMethod == DiscoveryMethodZeroConf) {
                qCDebug(dcGoECharger()) << "Discovery: Network discovery found API V2 go-eCharger on" << address.toString()
                << "but this host has already been discovered using ZeroConf. Prefering ZeroConf over MAC address due to Repeater missbehaviours.";
                return;
            }

            if (m_discoveryResults.contains(address)) {
                result.apiAvailableV1 = m_discoveryResults.value(address).apiAvailableV1;
            }

            // Overwrite result from V1 since V2 contains more information
            m_discoveryResults[address] = result;
        } else {
            qCDebug(dcGoECharger()) << "Discovery:" << address.toString() << "API V2 verification returned JSON data but not the right one. Continue...";
        }
    });
}

void GoeDiscovery::onServiceEntryAdded(const ZeroConfServiceEntry &serviceEntry)
{
    // Note: we always prefere the zeroconf discovery over the network discovery. Some networks use wifi repeaters,
    // which spoof the mac address and multipe IP have the same mac address. Using zeroconf and have IP based discovery
    // solves this issue

    if (isGoeCharger(serviceEntry) && serviceEntry.protocol() == QAbstractSocket::IPv4Protocol) {
        qCDebug(dcGoECharger()) << "Discovery: Found ZeroConf go-eCharger" << serviceEntry;

        GoeDiscovery::Result result;
        result.serialNumber = serviceEntry.txt("serial");
        result.firmwareVersion = serviceEntry.txt("version");
        result.manufacturer = serviceEntry.txt("manufacturer");
        result.product = serviceEntry.txt("devicetype");
        result.friendlyName = serviceEntry.txt("friendly_name");
        result.discoveryMethod = DiscoveryMethodZeroConf;
        result.apiAvailableV1 = serviceEntry.txt("protocol").toUInt() == 1;
        result.apiAvailableV2 = serviceEntry.txt("protocol").toUInt() == 2;
        result.address = serviceEntry.hostAddress();

        qCDebug(dcGoECharger()) << "Discovery:" << result;

        // Overwrite any already discovered result for this host, we always prefere ZeroConf over Networkdiscovery...
        m_discoveryResults[result.address] = result;
        m_verifiedHostAddresses.append(result.address);
    }
}

void GoeDiscovery::cleanupPendingReplies()
{
    foreach (QNetworkReply *reply, m_pendingReplies) {
        m_pendingReplies.removeAll(reply);
        reply->abort();
    }
}

void GoeDiscovery::finishDiscovery()
{
    disconnect(m_serviceBrowser, &ZeroConfServiceBrowser::serviceEntryAdded, this, &GoeDiscovery::onServiceEntryAdded);

    foreach (const Result &result, m_discoveryResults) {
        int index = m_discoveredNetworkDeviceInfos.indexFromHostAddress(result.address);
        if (index >= 0) {
            m_discoveryResults[result.address].networkDeviceInfo = m_discoveredNetworkDeviceInfos.at(index);
        }
    }

    qint64 durationMilliSeconds = QDateTime::currentMSecsSinceEpoch() - m_startDateTime.toMSecsSinceEpoch();
    qCInfo(dcGoECharger()) << "Discovery: Finished the discovery process. Found" << m_discoveryResults.count() << "go-eChargers in" << QTime::fromMSecsSinceStartOfDay(durationMilliSeconds).toString("mm:ss.zzz");
    cleanupPendingReplies();
    emit discoveryFinished();
}

QDebug operator<<(QDebug dbg, const GoeDiscovery::Result &result)
{
    dbg.nospace() << "GoeDiscovery:Result(" << result.product;
    dbg.nospace() << ", " << result.manufacturer;
    dbg.nospace() << ", Version: " << result.firmwareVersion;
    dbg.nospace() << ", SN: " << result.serialNumber;
    dbg.nospace() << ", V1: " << result.apiAvailableV1;
    dbg.nospace() << ", V2: " << result.apiAvailableV2;
    if (result.discoveryMethod == GoeDiscovery::DiscoveryMethodZeroConf) {
        dbg.nospace() << ", " << result.discoveryMethod;
        dbg.nospace() << ", " << result.address.toString();
    } else {
        dbg.nospace() << ", " << result.networkDeviceInfo.address().toString();
    }
    dbg.nospace() << ") ";
    return dbg.maybeSpace();
}
