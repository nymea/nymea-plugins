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

#ifndef KEBADISCOVERY_H
#define KEBADISCOVERY_H

#include <QTimer>
#include <QObject>

#include <network/networkdeviceinfos.h>

class KeContactDataLayer;
class NetworkDeviceDiscovery;

class KebaDiscovery : public QObject
{
    Q_OBJECT
public:
    typedef struct KebaDiscoveryResult {
        QString product;
        QString serialNumber;
        QString firmwareVersion;
        QHostAddress address;
        NetworkDeviceInfo networkDeviceInfo;
    } KebaDiscoveryResult;

    explicit KebaDiscovery(KeContactDataLayer *kebaDataLayer, NetworkDeviceDiscovery *networkDeviceDiscovery, QObject *parent = nullptr);
    ~KebaDiscovery();

    void startDiscovery();

    QList<KebaDiscoveryResult> discoveryResults() const;

signals:
    void discoveryFinished();

private slots:
    void sendReportRequest(const QHostAddress &address);

private:
    KeContactDataLayer *m_kebaDataLayer = nullptr;
    NetworkDeviceDiscovery *m_networkDeviceDiscovery = nullptr;
    QTimer m_responseTimer;

    NetworkDeviceInfos m_networkDeviceInfos;
    QList<QHostAddress> m_verifiedAddresses;
    QList<KebaDiscoveryResult> m_results;

    void cleanup();
};

#endif // KEBADISCOVERY_H
