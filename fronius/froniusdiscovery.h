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

#ifndef FRONIUSDISCOVERY_H
#define FRONIUSDISCOVERY_H

#include <QObject>
#include <QTimer>

#include <network/networkdevicediscovery.h>
#include "froniussolarconnection.h"

class FroniusDiscovery : public QObject
{
    Q_OBJECT
public:
    explicit FroniusDiscovery(NetworkAccessManager *networkManager, NetworkDeviceDiscovery *networkDeviceDiscovery, QObject *parent = nullptr);

    void startDiscovery();

    QList<NetworkDeviceInfo> discoveryResults() const;

signals:
    void discoveryFinished();

private:
    NetworkAccessManager *m_networkManager = nullptr;
    NetworkDeviceDiscovery *m_networkDeviceDiscovery = nullptr;

    QTimer m_gracePeriodTimer;
    QDateTime m_startDateTime;

    QList<FroniusSolarConnection *> m_connections;

    NetworkDeviceInfos m_networkDeviceInfos;
    QList<QHostAddress> m_discoveredAddresses;
    QList<NetworkDeviceInfo> m_discoveryResults;

    void checkHostAddress(const QHostAddress &address);
    void cleanupConnection(FroniusSolarConnection *connection);

    void finishDiscovery();
};

#endif // FRONIUSDISCOVERY_H
