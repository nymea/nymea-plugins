/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *                                                                         *
 *  Copyright (C) 2019 Bernhard Trinnes <bernhard.trinnes@nymea.io>        *
 *                                                                         *
 *  This file is part of nymea.                                            *
 *                                                                         *
 *  This library is free software; you can redistribute it and/or          *
 *  modify it under the terms of the GNU Lesser General Public             *
 *  License as published by the Free Software Foundation; either           *
 *  version 2.1 of the License, or (at your option) any later version.     *
 *                                                                         *
 *  This library is distributed in the hope that it will be useful,        *
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of         *
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU      *
 *  Lesser General Public License for more details.                        *
 *                                                                         *
 *  You should have received a copy of the GNU Lesser General Public       *
 *  License along with this library; If not, see                           *
 *  <http://www.gnu.org/licenses/>.                                        *
 *                                                                         *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

#ifndef HOMECONNECT_H
#define HOMECONNECT_H

#include <QObject>
#include <QTimer>

#include "network/networkaccessmanager.h"
#include "devices/device.h"

class HomeConnect : public QObject
{
    Q_OBJECT
public:
    HomeConnect(NetworkAccessManager *networkmanager,  const QByteArray &clientKey,  const QByteArray &clientSecret, QObject *parent = nullptr);

    QUrl getLoginUrl(const QUrl &redirectUrl, const QString &scope);
    void checkStatusCode(int status, const QByteArray &payload);
    void getAccessTokenFromRefreshToken(const QByteArray &refreshToken);
    void getAccessTokenFromAuthorizationCode(const QByteArray &authorizationCode);

private:
    QByteArray m_baseAuthorizationUrl = "https://api.home-connect.com/security/oauth/authorize";
    QByteArray m_baseTokenUrl = "https://api.home-connect.com/security/oauth/token";
    QByteArray m_baseControlUrl = "https://api.home-connect.com";
    QByteArray m_clientKey;
    QByteArray m_clientSecret;

    QByteArray m_accessToken;
    QByteArray m_refreshToken;
    QByteArray m_redirectUri;

    NetworkAccessManager *m_networkManager = nullptr;
    QTimer *m_tokenRefreshTimer = nullptr;

private slots:
    void onRefreshTimeout();

signals:
    void connectionChanged(bool connected);
    void authenticationStatusChanged(bool authenticated);
    void actionExecuted(QUuid actionId,bool success);
};
#endif // HOMECONNECT_H
