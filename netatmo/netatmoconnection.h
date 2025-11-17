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

#ifndef NETATMOCONNECTION_H
#define NETATMOCONNECTION_H

#include <QObject>
#include <QTimer>

#include <network/networkaccessmanager.h>

class NetatmoConnection : public QObject
{
    Q_OBJECT
public:
    explicit NetatmoConnection(NetworkAccessManager *networkmanager, const QByteArray &clientId, const QByteArray &clientSecret, QObject *parent = nullptr);

    QByteArray accessToken();
    QByteArray refreshToken();

    QUrl getLoginUrl(const QUrl &redirectUrl = QUrl("https://127.0.0.1:8888"));

    // Note: Use only for migration since this login type is deprecated at netatmo and is
    // not working any more with new accounts or after changing the password of an older existing account
    bool getAccessTokenFromUsernamePassword(const QString username, const QString &password);

    bool getAccessTokenFromRefreshToken(const QByteArray &refreshToken);
    bool getAccessTokenFromAuthorizationCode(const QByteArray &authorizationCode);

    QNetworkReply *getStationData();

    static QString censorDebugOutput(const QString &text);

signals:
    void authenticatedChanged(bool authenticated);
    void receivedRefreshToken(const QByteArray &refreshToken);
    void receivedAccessToken(const QByteArray &accessToken);

private:
    NetworkAccessManager *m_networkManager = nullptr;
    QTimer *m_tokenRefreshTimer = nullptr;
    bool m_authenticated = false;

    QStringList m_scopes;

    QUrl m_baseUrl = QUrl("https://api.netatmo.com");
    QUrl m_redirectUrl = QUrl("https://127.0.0.1:8888");

    QByteArray m_clientId;
    QByteArray m_clientSecret;
    QByteArray m_accessToken;
    QByteArray m_refreshToken;

    void setAuthenticated(bool authenticated);
    void processLoginResponse(const QByteArray &data);
};

#endif // NETATMOCONNECTION_H
