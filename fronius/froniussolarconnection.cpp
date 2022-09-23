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

#include "froniussolarconnection.h"
#include "extern-plugininfo.h"

#include <QUrlQuery>

FroniusSolarConnection::FroniusSolarConnection(NetworkAccessManager *networkManager, const QHostAddress &address, QObject *parent) :
    QObject(parent),
    m_networkManager(networkManager),
    m_address(address)
{

}

QHostAddress FroniusSolarConnection::address() const
{
    return m_address;
}

bool FroniusSolarConnection::available() const
{
    return m_available;
}

bool FroniusSolarConnection::busy() const
{
    return m_requestQueue.count() > 1;
}

FroniusNetworkReply *FroniusSolarConnection::getVersion()
{
    QUrl requestUrl;
    requestUrl.setScheme("http");
    requestUrl.setHost(m_address.toString());
    requestUrl.setPath("/solar_api/GetAPIVersion.cgi");

    FroniusNetworkReply *reply = new FroniusNetworkReply(QNetworkRequest(requestUrl), this);
    m_requestQueue.enqueue(reply);
    sendNextRequest();
    return reply;
}

FroniusNetworkReply *FroniusSolarConnection::getActiveDevices()
{
    QUrl requestUrl;
    requestUrl.setScheme("http");
    requestUrl.setHost(m_address.toString());
    requestUrl.setPath("/solar_api/v1/GetActiveDeviceInfo.cgi");

    QUrlQuery query;
    query.addQueryItem("DeviceClass", "System");
    requestUrl.setQuery(query);

    FroniusNetworkReply *reply = new FroniusNetworkReply(QNetworkRequest(requestUrl), this);
    m_requestQueue.enqueue(reply);

    // Note: we use this request for detecting if the logger is available or not.
    connect(reply, &FroniusNetworkReply::finished, this, [=](){
        if (reply->networkReply()->error() == QNetworkReply::NoError) {
            // Reply was successfully, we can communicate
            if (!m_available) {
                qCDebug(dcFronius()) << "Connection: the connection is now available";
                m_available = true;
                emit availableChanged(m_available);

                // Destroy any pending requests
                qDeleteAll(m_requestQueue);
                m_requestQueue.clear();
            }
        } else {
            // Ther have been errors, seems like we not available any more
            if (m_available) {
                qCDebug(dcFronius()) << "Connection: the connection is not available any more:" << reply->networkReply()->errorString();
                m_available = false;
                emit availableChanged(m_available);
            }
        }
    });

    sendNextRequest();
    return reply;
}

FroniusNetworkReply *FroniusSolarConnection::getPowerFlowRealtimeData()
{
    QUrl requestUrl;
    requestUrl.setScheme("http");
    requestUrl.setHost(m_address.toString());
    requestUrl.setPath("/solar_api/v1/GetPowerFlowRealtimeData.fcgi");

    FroniusNetworkReply *reply = new FroniusNetworkReply(QNetworkRequest(requestUrl), this);
    m_requestQueue.enqueue(reply);
    sendNextRequest();
    return reply;
}

FroniusNetworkReply *FroniusSolarConnection::getInverterRealtimeData(int inverterId)
{
    QUrl requestUrl;
    requestUrl.setScheme("http");
    requestUrl.setHost(m_address.toString());
    requestUrl.setPath("/solar_api/v1/GetInverterRealtimeData.cgi");

    QUrlQuery query;
    query.addQueryItem("Scope", "Device");
    query.addQueryItem("DeviceId", QString::number(inverterId));
    query.addQueryItem("DataCollection", "CommonInverterData");
    requestUrl.setQuery(query);

    FroniusNetworkReply *reply = new FroniusNetworkReply(QNetworkRequest(requestUrl), this);
    m_requestQueue.enqueue(reply);
    sendNextRequest();
    return reply;
}

FroniusNetworkReply *FroniusSolarConnection::getMeterRealtimeData(int meterId)
{
    QUrl requestUrl;
    requestUrl.setScheme("http");
    requestUrl.setHost(m_address.toString());
    requestUrl.setPath("/solar_api/v1/GetMeterRealtimeData.cgi");

    QUrlQuery query;
    query.addQueryItem("Scope", "Device");
    query.addQueryItem("DeviceId", QString::number(meterId));
    requestUrl.setQuery(query);

    FroniusNetworkReply *reply = new FroniusNetworkReply(QNetworkRequest(requestUrl), this);
    m_requestQueue.enqueue(reply);
    sendNextRequest();
    return reply;
}

FroniusNetworkReply *FroniusSolarConnection::getStorageRealtimeData(int meterId)
{
    QUrl requestUrl;
    requestUrl.setScheme("http");
    requestUrl.setHost(m_address.toString());
    requestUrl.setPath("/solar_api/v1/GetStorageRealtimeData.cgi");

    QUrlQuery query;
    query.addQueryItem("Scope", "Device");
    query.addQueryItem("DeviceId", QString::number(meterId));
    requestUrl.setQuery(query);

    FroniusNetworkReply *reply = new FroniusNetworkReply(QNetworkRequest(requestUrl), this);
    m_requestQueue.enqueue(reply);
    sendNextRequest();
    return reply;
}

void FroniusSolarConnection::sendNextRequest()
{
    if (m_currentReply)
        return;

    if (m_requestQueue.isEmpty())
        return;

    m_currentReply = m_requestQueue.dequeue();

//    qCDebug(dcFronius()) << "Connection: Sending request" << m_currentReply->request().url().toString();
    m_currentReply->setNetworkReply(m_networkManager->get(m_currentReply->request()));

    connect(m_currentReply, &FroniusNetworkReply::finished, this, [=](){
        if (m_currentReply->networkReply()->error() != QNetworkReply::NoError) {
            qCWarning(dcFronius()) << "Connection: Request finished with error:" << m_currentReply->networkReply()->error() << "for url" << m_currentReply->request().url().toString();
        }

        // Note: the network reply will be deleted in the destructor
        m_currentReply->deleteLater();

        m_currentReply = nullptr;
        sendNextRequest();
    });
}
