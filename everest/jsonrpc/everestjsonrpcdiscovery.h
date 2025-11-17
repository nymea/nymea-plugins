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

#ifndef EVERESTJSONRPCDISCOVERY_H
#define EVERESTJSONRPCDISCOVERY_H

#include <QObject>

#include <network/networkdevicediscovery.h>

#include "jsonrpc/everestjsonrpcclient.h"

class EverestJsonRpcDiscovery : public QObject
{
    Q_OBJECT
public:
    typedef struct Result {
        QHostAddress address;
        NetworkDeviceInfo networkDeviceInfo;
    } Result;

    explicit EverestJsonRpcDiscovery(NetworkDeviceDiscovery *networkDeviceDiscovery, quint16 port = 8080, QObject *parent = nullptr);

    void start();
    void startLocalhost();

    QList<EverestJsonRpcDiscovery::Result> results() const;

signals:
    void finished();

private:
    NetworkDeviceDiscovery *m_networkDeviceDiscovery = nullptr;
    quint16 m_port = 8080;

    QDateTime m_startDateTime;
    NetworkDeviceInfos m_networkDeviceInfos;
    QList<EverestJsonRpcClient *> m_clients;
    QList<EverestJsonRpcDiscovery::Result> m_results;

    bool m_localhostDiscovery = false;

    void checkHostAddress(const QHostAddress &address);
    void cleanupClient(EverestJsonRpcClient *client);
    void finishDiscovery();

};

#endif // EVERESTJSONRPCDISCOVERY_H
