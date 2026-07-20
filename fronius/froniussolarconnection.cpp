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
    m_requestDispatchTimer.setSingleShot(true);
    connect(&m_requestDispatchTimer, &QTimer::timeout, this, &FroniusSolarConnection::sendNextRequest);
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
    m_requestDispatchTimer.stop();
    cancelPendingRequests();
    resetCustomNetworkManager();
    m_errorCount = 0;
    m_errorOperationCanceledCount = 0;
    m_availabilityErrorCount = 0;

    if (m_address.isNull() && m_available) {
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
    return m_currentReply || !m_requestQueue.isEmpty();
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
    connect(reply, &FroniusNetworkReply::finished, this, [this, reply](){
        if (reply->wasCanceled())
            return;

        if (reply->error() == QNetworkReply::NoError) {
            m_availabilityErrorCount = 0;

            // Reply was successfully, we can communicate
            if (!m_available) {
                qCDebug(dcFronius()) << "Connection: the connection is now available";
                m_available = true;
                emit availableChanged(m_available);
            }
        } else if (++m_availabilityErrorCount >= m_errorCountLimit && m_available) {
            qCWarning(dcFronius()) << "Connection: active-device probe failed" << m_availabilityErrorCount << "times. Marking the connection unavailable:" << reply->errorString();
            m_available = false;
            emit availableChanged(m_available);
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
    request.setTransferTimeout(requestTimeout);
    request.setRawHeader("Connection", "close");
    return request;
}

void FroniusSolarConnection::sendNextRequest()
{
    if (m_currentReply)
        return;

    if (m_requestDispatchTimer.isActive())
        return;

    if (m_requestQueue.isEmpty())
        return;

    m_currentReply = m_requestQueue.dequeue();
    FroniusNetworkReply *reply = m_currentReply;

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


    connect(reply, &FroniusNetworkReply::finished, this, [this, reply](){

        // The address may have changed while this request was running.
        if (reply != m_currentReply) {
            reply->deleteLater();
            return;
        }

        // Note: the network reply will be deleted in the destructor
        reply->deleteLater();

        if (reply->error() != QNetworkReply::NoError) {
            m_errorCount++;
            qCWarning(dcFronius()).nospace() << "Connection: <-- Request finished with error (count: " << m_errorCount << ") " << reply->error() << " (" << reply->errorString() << ") for url " << reply->request().url().toString();
            if (reply->error() == QNetworkReply::OperationCanceledError) {
                m_errorOperationCanceledCount++;
                if (!m_useCustomNetworkManager && m_errorOperationCanceledCount >= m_errorOperationCanceledCountLimit) {
                    qCWarning(dcFronius()) << "Received" << m_errorOperationCanceledCountLimit << "in a row, skipping to internal network access manager. This is a workaround in order to free all requests after each reply.";
                    m_useCustomNetworkManager = true;
                }
            }
        } else {
            qCDebug(dcFronius()) << "Connection: <-- Request finished successfully for" << reply->request().url().toString();
            m_errorCount = 0;
            m_errorOperationCanceledCount = 0;
        }

        m_currentReply = nullptr;

        // Note: this is a workaround for some fronius devices, we recreate the networkaccessmanager after each request
        resetCustomNetworkManager();

        // Back off after repeated failures to give the Fronius webserver time to recover.
        const int nextRequestDelay = m_errorCount >= m_errorCountLimit ? unavailableRetryInterval : requestInterval;
        qCDebug(dcFronius()) << "Connection: next request in" << nextRequestDelay << "ms (queue:" << m_requestQueue.size() << ")";
        m_requestDispatchTimer.start(nextRequestDelay);
    });
}

void FroniusSolarConnection::cancelPendingRequests()
{
    if (m_currentReply) {
        FroniusNetworkReply *reply = m_currentReply;
        m_currentReply = nullptr;
        reply->cancel();
        reply->deleteLater();
    }

    while (!m_requestQueue.isEmpty()) {
        FroniusNetworkReply *reply = m_requestQueue.dequeue();
        reply->cancel();
        reply->deleteLater();
    }
}

void FroniusSolarConnection::resetCustomNetworkManager()
{
    if (!m_customNetworkManager)
        return;

    m_customNetworkManager->deleteLater();
    m_customNetworkManager = nullptr;
}
