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

#include "froniussolarconnection.h"
#include "extern-plugininfo.h"

#include <QUrlQuery>
#include <QTimer>

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

void FroniusSolarConnection::setAddress(const QHostAddress &address)
{
    if (m_address == address)
        return;

    m_address = address;

    // The address has changed, let's clean up any queue and refresh

    // Note: the destructor will take care about the cleanup of any pending replies
    qDeleteAll(m_requestQueue);
    m_requestQueue.clear();

    if (m_currentReply) {
        m_currentReply->deleteLater();
        m_currentReply = nullptr;
    }

    if (m_address.isNull()) {
        m_available = false;
        emit availableChanged(m_available);
    }
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

    FroniusNetworkReply *reply = new FroniusNetworkReply(buildRequest(requestUrl), this);
    m_requestQueue.enqueue(reply);
    qCDebug(dcFronius()).nospace() << "Connection: Enqueued request (queue: " << m_requestQueue.size() << "): " << requestUrl.toString();
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

    FroniusNetworkReply *reply = new FroniusNetworkReply(buildRequest(requestUrl), this);
    m_requestQueue.enqueue(reply);
    qCDebug(dcFronius()).nospace() << "Connection: Enqueued request (queue: " << m_requestQueue.size() << "): " << requestUrl.toString();

    // Note: we use this request for detecting if the logger is available or not.
    // Some other requests are only available if the device actually is loaded
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
            // There have been multiple errors in a row, seems like we not available any more
            if (m_available && m_errorCount >= m_errorCountLimit) {
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

    FroniusNetworkReply *reply = new FroniusNetworkReply(buildRequest(requestUrl), this);
    m_requestQueue.enqueue(reply);
    qCDebug(dcFronius()).nospace() << "Connection: Enqueued request (queue: " << m_requestQueue.size() << "): " << requestUrl.toString();
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

    FroniusNetworkReply *reply = new FroniusNetworkReply(buildRequest(requestUrl), this);
    m_requestQueue.enqueue(reply);
    qCDebug(dcFronius()).nospace() << "Connection: Enqueued request (queue: " << m_requestQueue.size() << "): " << requestUrl.toString();
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

    FroniusNetworkReply *reply = new FroniusNetworkReply(buildRequest(requestUrl), this);
    m_requestQueue.enqueue(reply);
    qCDebug(dcFronius()).nospace() << "Connection: Enqueued request (queue: " << m_requestQueue.size() << "): " << requestUrl.toString();
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

    FroniusNetworkReply *reply = new FroniusNetworkReply(buildRequest(requestUrl), this);
    m_requestQueue.enqueue(reply);
    qCDebug(dcFronius()).nospace() << "Connection: Enqueued request (queue: " << m_requestQueue.size() << "): " << requestUrl.toString();
    sendNextRequest();
    return reply;
}

QNetworkRequest FroniusSolarConnection::buildRequest(const QUrl &url)
{
    QNetworkRequest request;
    request.setUrl(url);
    // Note: some inverter stop accepting requests, this might help
    request.setAttribute(QNetworkRequest::HttpPipeliningAllowedAttribute, false);
    return request;
}

void FroniusSolarConnection::sendNextRequest()
{
    if (m_currentReply)
        return;

    if (m_requestQueue.isEmpty())
        return;

    m_currentReply = m_requestQueue.dequeue();

    if (m_useCustomNetworkManager) {
        qCDebug(dcFronius()) << "Connection: --> Sending request using custom network manager (queue: " << m_requestQueue.size() << "): " << m_currentReply->request().url().toString();
        if (!m_customNetworkManager) {
            m_customNetworkManager = new QNetworkAccessManager(this);
        }

        m_currentReply->setNetworkReply(m_customNetworkManager->get(m_currentReply->request()));
    } else {
        qCDebug(dcFronius()).nospace() << "Connection: --> Sending request (queue: " << m_requestQueue.size() << "): " << m_currentReply->request().url().toString();
        m_currentReply->setNetworkReply(m_networkManager->get(m_currentReply->request()));
    }


    connect(m_currentReply, &FroniusNetworkReply::finished, this, [=](){

        // Note: the network reply will be deleted in the destructor
        m_currentReply->deleteLater();

        if (m_currentReply->networkReply()->error() != QNetworkReply::NoError) {
            m_errorCount++;
            qCWarning(dcFronius()).nospace() << "Connection: <--  Request finished with error (count: " << m_errorCount << ") " << m_currentReply->networkReply()->error() << " for url " << m_currentReply->request().url().toString();
            if (m_currentReply->networkReply()->error() == QNetworkReply::OperationCanceledError) {
                m_errorOperationCanceledCount++;
                if (!m_useCustomNetworkManager && m_errorOperationCanceledCount >= m_errorOperationCanceledCountLimit) {
                    qCWarning(dcFronius()) << "Received" << m_errorOperationCanceledCountLimit << "in a row, skipping to internal network access manager. This is a workaround in order to free all requests after each reply.";
                    m_useCustomNetworkManager = true;
                }
            }
        } else {
            qCDebug(dcFronius()) << "Connection: <-- Request finished successfully for" << m_currentReply->request().url().toString();
            m_errorCount = 0;
            m_errorOperationCanceledCount = 0;
        }

        m_currentReply = nullptr;

        // Note: this is a workaround for some fronius devices, we recreate the networkaccessmanager after each request
        if (m_useCustomNetworkManager && m_customNetworkManager) {
            m_customNetworkManager->deleteLater();
            m_customNetworkManager = nullptr;
        }

        // Wait some time until we send the next request
        QTimer::singleShot(500, this, &FroniusSolarConnection::sendNextRequest);
    });
}
