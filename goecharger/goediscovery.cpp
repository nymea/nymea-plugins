/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*
* Copyright 2013 - 2022, nymea GmbH
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

GoeDiscovery::GoeDiscovery(NetworkAccessManager *networkAccessManager, NetworkDeviceDiscovery *networkDeviceDiscovery, QObject *parent) :
    QObject(parent),
    m_networkAccessManager(networkAccessManager),
    m_networkDeviceDiscovery(networkDeviceDiscovery)
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
    m_verifiedNetworkDeviceInfos.clear();

    m_startDateTime = QDateTime::currentDateTime();

    qCInfo(dcGoECharger()) << "Discovery: Start discovering the network...";
    m_discoveryReply = m_networkDeviceDiscovery->discover();

    // Check if all network device infos which might already be discovered here to save time...
    foreach (const NetworkDeviceInfo &networkDeviceInfo, m_discoveryReply->networkDeviceInfos()) {
        checkNetworkDevice(networkDeviceInfo);
    }

    // Test any network device beeing discovered
    connect(m_discoveryReply, &NetworkDeviceDiscoveryReply::networkDeviceInfoAdded, this, &GoeDiscovery::checkNetworkDevice);

    // When the network discovery has finished, we process the rest and give some time to finish the pending replies
    connect(m_discoveryReply, &NetworkDeviceDiscoveryReply::finished, this, [=](){
        // The network device discovery is done
        m_discoveredNetworkDeviceInfos = m_discoveryReply->networkDeviceInfos();
        m_discoveryReply = nullptr;

        // Check if all network device infos have been verified
        foreach (const NetworkDeviceInfo &networkDeviceInfo, m_discoveredNetworkDeviceInfos) {
            if (m_verifiedNetworkDeviceInfos.contains(networkDeviceInfo))
                continue;

            checkNetworkDevice(networkDeviceInfo);
        }

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

void GoeDiscovery::checkNetworkDevice(const NetworkDeviceInfo &networkDeviceInfo)
{
    // Make sure we have not checked this host yet
    if (m_verifiedNetworkDeviceInfos.contains(networkDeviceInfo))
        return;

    qCDebug(dcGoECharger()) << "Discovery: Start inspecting" << networkDeviceInfo.address().toString();
    checkNetworkDeviceApiV2(networkDeviceInfo);
    checkNetworkDeviceApiV1(networkDeviceInfo);

    m_verifiedNetworkDeviceInfos.append(networkDeviceInfo);
}

void GoeDiscovery::checkNetworkDeviceApiV1(const NetworkDeviceInfo &networkDeviceInfo)
{
    // First check if API V1 is available: http://<host>/status
    QNetworkReply *reply = m_networkAccessManager->get(buildRequestV1(networkDeviceInfo.address()));
    m_pendingReplies.append(reply);
    connect(reply, &QNetworkReply::finished, this, [=](){
        m_pendingReplies.removeAll(reply);
        reply->deleteLater();

        if (reply->error() != QNetworkReply::NoError) {
            qCDebug(dcGoECharger()) << "Discovery:" << networkDeviceInfo.address().toString() << "API V1 verification HTTP error" << reply->errorString() << "Continue...";
            return;
        }

        QByteArray data = reply->readAll();
        QJsonParseError error;
        QJsonDocument jsonDoc = QJsonDocument::fromJson(data, &error);
        if (error.error != QJsonParseError::NoError) {
            qCDebug(dcGoECharger()) << "Discovery:" << networkDeviceInfo.address().toString() << "API V1 verification invalid JSON data. Continue...";
            return;
        }

        // Verify if we have the required values in the response map
        // https://github.com/goecharger/go-eCharger-API-v1/blob/master/go-eCharger%20API%20v1%20EN.md
        QVariantMap responseMap = jsonDoc.toVariant().toMap();
        if (responseMap.contains("fwv") && responseMap.contains("sse") && responseMap.contains("nrg") && responseMap.contains("amp")) {
            // Looks like we have found a go-e V1 api endpoint, nice
            qCDebug(dcGoECharger()) << "Discovery: --> Found API V1 on" << networkDeviceInfo.address().toString();

            if (m_discoveryResults.contains(networkDeviceInfo.address())) {
                // We use the information from API V2 since there are more information available
                m_discoveryResults[networkDeviceInfo.address()].apiAvailableV1 = true;
            } else {
                GoeDiscovery::Result result;
                result.serialNumber = responseMap.value("sse").toString();
                result.firmwareVersion = responseMap.value("fwv").toString();
                result.networkDeviceInfo = networkDeviceInfo;
                result.apiAvailableV1 = true;
                m_discoveryResults[networkDeviceInfo.address()] = result;
            }
        } else {
            qCDebug(dcGoECharger()) << "Discovery:" << networkDeviceInfo.address().toString() << "API V1 verification returned JSON data but not the right one. Continue...";
        }

    });
}

void GoeDiscovery::checkNetworkDeviceApiV2(const NetworkDeviceInfo &networkDeviceInfo)
{
    // Check if API V2 is available: http://<host>/api/status
    qCDebug(dcGoECharger()) << "Discovery: verify API V2 on" << networkDeviceInfo.address().toString();
    QNetworkReply *reply = m_networkAccessManager->get(buildRequestV2(networkDeviceInfo.address()));
    m_pendingReplies.append(reply);
    connect(reply, &QNetworkReply::finished, this, [=](){
        m_pendingReplies.removeAll(reply);
        reply->deleteLater();

        if (reply->error() != QNetworkReply::NoError) {
            qCDebug(dcGoECharger()) << "Discovery:" << networkDeviceInfo.address().toString() << "API V2 verification HTTP error" << reply->errorString() << "Continue...";
            return;
        }

        QByteArray data = reply->readAll();
        QJsonParseError error;
        QJsonDocument jsonDoc = QJsonDocument::fromJson(data, &error);
        if (error.error != QJsonParseError::NoError) {
            qCDebug(dcGoECharger()) << "Discovery:" << networkDeviceInfo.address().toString() << "API V2 verification invalid JSON data. Continue...";
            return;
        }

        // Verify if we have the required values in the response map
        // https://github.com/goecharger/go-eCharger-API-v2/blob/main/http-en.md
        QVariantMap responseMap = jsonDoc.toVariant().toMap();
        if (responseMap.contains("fwv") && responseMap.contains("sse") && responseMap.contains("typ") && responseMap.contains("fna")) {
            // Looks like we have found a go-e V2 api endpoint, nice
            qCDebug(dcGoECharger()) << "Discovery: --> Found API V2 on" << networkDeviceInfo.address().toString();

            GoeDiscovery::Result result;
            result.serialNumber = responseMap.value("sse").toString();
            result.firmwareVersion = responseMap.value("fwv").toString();
            result.manufacturer = responseMap.value("oem").toString();
            result.product = responseMap.value("typ").toString();
            result.friendlyName = responseMap.value("fna").toString();
            result.networkDeviceInfo = networkDeviceInfo;
            result.apiAvailableV2 = true;

            if (m_discoveryResults.contains(networkDeviceInfo.address())) {
                result.apiAvailableV1 = m_discoveryResults.value(networkDeviceInfo.address()).apiAvailableV1;
            }
            // Overwrite result from V1 since V2 contains more information
            m_discoveryResults[networkDeviceInfo.address()] = result;
        } else {
            qCDebug(dcGoECharger()) << "Discovery:" << networkDeviceInfo.address().toString() << "API V2 verification returned JSON data but not the right one. Continue...";
        }
    });
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
    dbg.nospace() << ", " << result.networkDeviceInfo.address().toString();
    dbg.nospace() << ", " << result.networkDeviceInfo.macAddress();
    dbg.nospace() << ") ";
    return dbg.maybeSpace();
}
