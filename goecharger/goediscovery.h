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

#ifndef GOEDISCOVERY_H
#define GOEDISCOVERY_H

#include <QObject>
#include <QDebug>

#include <network/networkaccessmanager.h>
#include <network/networkdevicediscovery.h>
#include <network/networkdevicediscovery.h>
#include <platform/platformzeroconfcontroller.h>
#include <network/zeroconf/zeroconfservicebrowser.h>

class GoeDiscovery : public QObject
{
    Q_OBJECT
public:
    enum DiscoveryMethod {
        DiscoveryMethodNetwork,
        DiscoveryMethodZeroConf,
    };
    Q_ENUM(DiscoveryMethod)

    typedef struct Result {
        QString product = "go-eCharger";
        QString manufacturer = "go-e";
        QString friendlyName;
        QString serialNumber;
        QString firmwareVersion;
        DiscoveryMethod discoveryMethod;
        NetworkDeviceInfo networkDeviceInfo; // Network discovery
        QHostAddress address; // ZeroConf
        bool apiAvailableV1 = false;
        bool apiAvailableV2 = false;
    } Result;

    explicit GoeDiscovery(NetworkAccessManager *networkAccessManager, NetworkDeviceDiscovery *networkDeviceDiscovery, ZeroConfServiceBrowser *serviceBrowser, QObject *parent = nullptr);
    ~GoeDiscovery();

    void startDiscovery();

    QList<GoeDiscovery::Result> discoveryResults() const;

    static QNetworkRequest buildRequestV1(const QHostAddress &address);
    static QNetworkRequest buildRequestV2(const QHostAddress &address);

    // Zeroconf service helpers
    static bool isGoeCharger(const ZeroConfServiceEntry &serviceEntry);

signals:
    void discoveryFinished();

private:
    QDateTime m_startDateTime;
    NetworkAccessManager *m_networkAccessManager = nullptr;
    NetworkDeviceDiscovery *m_networkDeviceDiscovery = nullptr;
    NetworkDeviceDiscoveryReply *m_discoveryReply = nullptr;
    ZeroConfServiceBrowser *m_serviceBrowser = nullptr;

    QHash<QHostAddress, GoeDiscovery::Result> m_discoveryResults;
    NetworkDeviceInfos m_discoveredNetworkDeviceInfos;
    QList<QHostAddress> m_verifiedHostAddresses;

    QList<QNetworkReply *> m_pendingReplies;

private slots:
    void checkHostAddress(const QHostAddress &address);
    void checkHostAddressApiV1(const QHostAddress &address);
    void checkHostAddressApiV2(const QHostAddress &address);

    void onServiceEntryAdded(const ZeroConfServiceEntry &serviceEntry);

    void cleanupPendingReplies();

    void finishDiscovery();
};

QDebug operator<<(QDebug debug, const GoeDiscovery::Result &result);

#endif // GOEDISCOVERY_H
