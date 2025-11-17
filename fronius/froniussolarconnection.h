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

#ifndef FRONIUSSOLARCONNECTION_H
#define FRONIUSSOLARCONNECTION_H

#include <QObject>
#include <QQueue>
#include <QHostAddress>
#include <QNetworkAccessManager>

#include <network/networkaccessmanager.h>

#include "froniusnetworkreply.h"

class FroniusSolarConnection : public QObject
{
    Q_OBJECT
public:
    explicit FroniusSolarConnection(NetworkAccessManager *networkManager, const QHostAddress &address, QObject *parent = nullptr);

    QHostAddress address() const;
    void setAddress(const QHostAddress &address);

    bool available() const;

    bool busy() const;

    FroniusNetworkReply *getVersion();
    FroniusNetworkReply *getActiveDevices();
    FroniusNetworkReply *getPowerFlowRealtimeData();

    FroniusNetworkReply *getInverterRealtimeData(int inverterId);
    FroniusNetworkReply *getMeterRealtimeData(int meterId);
    FroniusNetworkReply *getStorageRealtimeData(int meterId);

signals:
    void availableChanged(bool available);

private:
    NetworkAccessManager *m_networkManager = nullptr;
    QHostAddress m_address;

    bool m_available = false;

    // Fallback solution for dead nam requests, this happens on some platforms
    // Note: we enable for now the custom network access manager
    // Some fronius inverters keep the connection alive and get stuck somehow.
    // In order to workaround this issue, we have to recreate the nam after each request.
    // Stuff like disableing pipelining, queueing requests did not fix the issue, only
    // destroying and re-creating the nam helped here. Current guess: the persistant TCP connection
    // keeps some resources blocked. The issue is actually on the fronius webserver side, and just on some
    // rare hardware so far.
    QNetworkAccessManager *m_customNetworkManager = nullptr;
    bool m_useCustomNetworkManager = true; // Force for now
    uint m_errorOperationCanceledCount = 0;
    uint m_errorOperationCanceledCountLimit = 3;
    uint m_errorCount = 0;
    uint m_errorCountLimit = 5;

    // Request queue to prevent overloading the device with requests
    FroniusNetworkReply *m_currentReply = nullptr;
    QQueue<FroniusNetworkReply *> m_requestQueue;

    QNetworkRequest buildRequest(const QUrl &url);

    void sendNextRequest();

};

#endif // FRONIUSSOLARCONNECTION_H
