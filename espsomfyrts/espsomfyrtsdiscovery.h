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

#ifndef ESPSOMFYRTSDISCOVERY_H
#define ESPSOMFYRTSDISCOVERY_H

#include <QObject>
#include <QTimer>

#include <network/networkaccessmanager.h>
#include <network/networkdevicediscovery.h>

class EspSomfyRtsDiscovery : public QObject
{
    Q_OBJECT
public:
    explicit EspSomfyRtsDiscovery(NetworkAccessManager *networkManager, NetworkDeviceDiscovery *networkDeviceDiscovery, QObject *parent = nullptr);

    typedef struct Result {
        QString name;
        QString firmwareVersion;
        QHostAddress address;
        NetworkDeviceInfo networkDeviceInfo;
    } Result;

    void startDiscovery();

    QList<EspSomfyRtsDiscovery::Result> results() const;

signals:
    void discoveryFinished();

private:
    NetworkAccessManager *m_networkManager = nullptr;
    NetworkDeviceDiscovery *m_networkDeviceDiscovery = nullptr;

    QTimer m_gracePeriodTimer;
    QDateTime m_startDateTime;

    NetworkDeviceInfos m_networkDeviceInfos;
    QList<EspSomfyRtsDiscovery::Result> m_results;

    void checkNetworkDevice(const QHostAddress &address);

    void finishDiscovery();
};

#endif // ESPSOMFYRTSDISCOVERY_H
