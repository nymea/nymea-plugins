/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*
* Copyright 2013 - 2022, nymea GmbH
* Contact: contact@nymea.io
*
* This file is part of nymea.
* This project including source code and documentation is protected by
* copyright law, and remains the property of nymea GmbH. All rights, including
* reproduction, publication, editing and translation, are reserved. The use of
* this project is subject to the terms of a license agreement to be concluded
* with nymea GmbH in accordance with the terms of use of nymea GmbH, available
* under https://nymea.io/license
*
* GNU Lesser General Public License Usage
* Alternatively, this project may be redistributed and/or modified under the
* terms of the GNU Lesser General Public License as published by the Free
* Software Foundation; version 3. This project is distributed in the hope that
* it will be useful, but WITHOUT ANY WARRANTY; without even the implied
* warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
* Lesser General Public License for more details.
*
* You should have received a copy of the GNU Lesser General Public License
* along with this project. If not, see <https://www.gnu.org/licenses/>.
*
* For any further details and any questions please contact us under
* contact@nymea.io or see our FAQ/Licensing Information on
* https://nymea.io/license/faq
*
* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

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
