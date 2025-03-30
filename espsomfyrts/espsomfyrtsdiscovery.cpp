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


#include "espsomfyrtsdiscovery.h"
#include "extern-plugininfo.h"

#include <QJsonDocument>
#include <QJsonParseError>

EspSomfyRtsDiscovery::EspSomfyRtsDiscovery(NetworkAccessManager *networkManager, NetworkDeviceDiscovery *networkDeviceDiscovery, QObject *parent) :
    QObject{parent},
    m_networkManager{networkManager},
    m_networkDeviceDiscovery{networkDeviceDiscovery}
{
    m_gracePeriodTimer.setSingleShot(true);
    m_gracePeriodTimer.setInterval(3000);
    connect(&m_gracePeriodTimer, &QTimer::timeout, this, [this](){
        finishDiscovery();
    });
}

void EspSomfyRtsDiscovery::startDiscovery()
{
    qCDebug(dcESPSomfyRTS()) << "Discovery: Searching for Fronius solar devices in the network...";
    m_startDateTime = QDateTime::currentDateTime();

    NetworkDeviceDiscoveryReply *discoveryReply = m_networkDeviceDiscovery->discover();
    connect(discoveryReply, &NetworkDeviceDiscoveryReply::hostAddressDiscovered, this, &EspSomfyRtsDiscovery::checkNetworkDevice);
    connect(discoveryReply, &NetworkDeviceDiscoveryReply::finished, this, [=](){
        qCDebug(dcESPSomfyRTS()) << "Discovery: Network discovery finished. Found" << discoveryReply->networkDeviceInfos().count() << "network devices";
        m_networkDeviceInfos = discoveryReply->networkDeviceInfos();
        m_gracePeriodTimer.start();
        discoveryReply->deleteLater();
    });
}

QList<EspSomfyRtsDiscovery::Result> EspSomfyRtsDiscovery::results() const
{
    return m_results;
}

void EspSomfyRtsDiscovery::checkNetworkDevice(const QHostAddress &address)
{
    qCDebug(dcESPSomfyRTS()) << "Discovery: Verifying" << address;
    QUrl url;
    url.setScheme("http");
    url.setHost(address.toString());
    url.setPort(8081);
    url.setPath("/discovery");

    QNetworkReply *reply = m_networkManager->get(QNetworkRequest(url));
    connect(reply, &QNetworkReply::finished, reply, &QNetworkReply::deleteLater);
    connect(reply, &QNetworkReply::finished, this, [this, reply, address](){
        if (reply->error() != QNetworkReply::NoError) {
            qCDebug(dcESPSomfyRTS()) << "Discovery: Reply finished with error" << reply->errorString() << "Continue...";
            return;
        }

        QJsonParseError jsonError;
        QJsonDocument jsonDoc = QJsonDocument::fromJson(reply->readAll(), &jsonError);

        if (jsonError.error != QJsonParseError::NoError) {
            qCDebug(dcESPSomfyRTS()) << "Discovery: Reply contains invalid JSON data" << jsonError.errorString() << "Continue...";
            return;
        }

        QVariantMap responseMap = jsonDoc.toVariant().toMap();
        if (responseMap.contains("model") && responseMap.value("model").toString().toLower().contains("espsomfyrts")) {

            Result result;
            result.address = address;
            result.name = responseMap.value("serverId").toString();
            result.firmwareVersion = responseMap.value("version").toString();
            m_results.append(result);

            qCDebug(dcESPSomfyRTS()) << "Discovery: --> Found ESPSomfy-RTS device" << result.name << result.firmwareVersion
                                     << "on" << result.networkDeviceInfo.address().toString() ;
        }
    });
}

void EspSomfyRtsDiscovery::finishDiscovery()
{
    qint64 durationMilliSeconds = QDateTime::currentMSecsSinceEpoch() - m_startDateTime.toMSecsSinceEpoch();

    // Fill in all network device infos we have
    for (int i = 0; i < m_results.count(); i++)
        m_results[i].networkDeviceInfo = m_networkDeviceInfos.get(m_results.at(i).address);

    qCDebug(dcESPSomfyRTS()) << "Discovery: Finished the discovery process. Found" << m_results.count()
                         << "ESPSomfy-RTS devices in" << QTime::fromMSecsSinceStartOfDay(durationMilliSeconds).toString("mm:ss.zzz");
    m_gracePeriodTimer.stop();

    emit discoveryFinished();
}

