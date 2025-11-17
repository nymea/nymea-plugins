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

#ifndef HUEBRIDGE_H
#define HUEBRIDGE_H

#include <QObject>
#include <QHostAddress>
#include <QNetworkRequest>

class HueBridge : public QObject
{
    Q_OBJECT
public:
    explicit HueBridge(QObject *parent = nullptr);

    QString name() const;
    void setName(const QString &name);

    QString id() const;
    void setId(const QString &id);

    QString apiKey() const;
    void setApiKey(const QString &apiKey);

    QHostAddress hostAddress() const;
    void setHostAddress(const QHostAddress &hostAddress);

    QString macAddress() const;
    void setMacAddress(const QString &macAddress);

    QString apiVersion() const;
    void setApiVersion(const QString &apiVersion);

    QString softwareVersion() const;
    void setSoftwareVersion(const QString &softwareVersion);

    int zigbeeChannel() const;
    void setZigbeeChannel(const int &zigbeeChannel);

    QPair<QNetworkRequest, QByteArray> createDiscoverLightsRequest();
    QPair<QNetworkRequest, QByteArray> createSearchLightsRequest(const QString &deviceId);
    QPair<QNetworkRequest, QByteArray> createSearchSensorsRequest();
    QPair<QNetworkRequest, QByteArray> createCheckUpdatesRequest();
    QPair<QNetworkRequest, QByteArray> createUpgradeRequest();

private:
    QString m_id;
    QString m_apiKey;
    QHostAddress m_hostAddress;
    QString m_name;
    QString m_macAddress;
    QString m_apiVersion;
    QString m_softwareVersion;
    int m_zigbeeChannel;
};

#endif // HUEBRIDGE_H
