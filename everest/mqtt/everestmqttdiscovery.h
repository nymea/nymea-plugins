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

#ifndef EVERESTDISCOVERY_H
#define EVERESTDISCOVERY_H

#include <QList>
#include <QObject>

#include <mqttclient.h>
#include <network/networkdevicediscovery.h>

class EverestMqttDiscovery : public QObject
{
    Q_OBJECT
public:
    typedef struct Result {
        QHostAddress address;
        QStringList connectors;
        NetworkDeviceInfo networkDeviceInfo;
    } Result;

    explicit EverestMqttDiscovery(NetworkDeviceDiscovery *networkDeviceDiscovery, QObject *parent = nullptr);

    void start();
    void startLocalhost();

    QList<EverestMqttDiscovery::Result> results() const;

signals:
    void finished();

private:
    NetworkDeviceDiscovery *m_networkDeviceDiscovery = nullptr;
    QDateTime m_startDateTime;
    QList<EverestMqttDiscovery::Result> m_results;
    QList<MqttClient *> m_clients;
    NetworkDeviceInfos m_networkDeviceInfos;

    bool m_localhostDiscovery = false;

    QString m_everestApiModuleTopicConnectors = "everest_api/connectors";

    void checkHostAddress(const QHostAddress &address);
    void cleanupClient(MqttClient *client);
    void finishDiscovery();
};

#endif // EVERESTDISCOVERY_H
