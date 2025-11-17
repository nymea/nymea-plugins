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

#ifndef SOMFYTAHOMAREQUESTS_H
#define SOMFYTAHOMAREQUESTS_H

#include <QNetworkReply>

class NetworkAccessManager;

class SomfyTahomaRequest : public QObject
{
    Q_OBJECT

public:
    SomfyTahomaRequest(QNetworkReply *reply, QObject *parent);

signals:
    void error(QNetworkReply::NetworkError error);
    void finished(const QVariant &results);
};

SomfyTahomaRequest *createCloudSomfyTahomaPostRequest(NetworkAccessManager *networkManager, const QString &path, const QString &contentType, const QByteArray &body, QObject *parent);
SomfyTahomaRequest *createCloudSomfyTahomaGetRequest(NetworkAccessManager *networkManager, const QString &path, QObject *parent);
SomfyTahomaRequest *createCloudSomfyTahomaDeleteRequest(NetworkAccessManager *networkManager, const QString &path, QObject *parent);
SomfyTahomaRequest *createCloudSomfyTahomaLoginRequest(NetworkAccessManager *networkManager, const QString &username, const QString &password, QObject *parent);

SomfyTahomaRequest *createLocalSomfyTahomaPostRequest(NetworkAccessManager *networkManager, const QString &host, const QString &token, const QString &path, const QString &contentType, const QByteArray &body, QObject *parent);
SomfyTahomaRequest *createLocalSomfyTahomaGetRequest(NetworkAccessManager *networkManager, const QString &host, const QString &token, const QString &path, QObject *parent);
SomfyTahomaRequest *createLocalSomfyTahomaEventFetchRequest(NetworkAccessManager *networkManager, const QString &host, const QString &token, const QString &eventListenerId, QObject *parent);


#endif // SOMFYTAHOMAREQUESTS_H
