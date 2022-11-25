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

#include "netatmoconnection.h"
#include "extern-plugininfo.h"

#include <QUrlQuery>
#include <QJsonDocument>

NetatmoConnection::NetatmoConnection(NetworkAccessManager *networkmanager, const QByteArray &clientId, const QByteArray &clientSecret, QObject *parent)
    : QObject{parent},
      m_networkManager{networkmanager},
      m_clientId{clientId},
      m_clientSecret{clientSecret}
{
    // Scopes we need to fetch all information we need
    m_scopes << "read_station";
    m_scopes << "read_thermostat";
    m_scopes << "write_thermostat";
    //m_scopes << "read_presence";
    //m_scopes << "read_smokedetector";
    //m_scopes << "read_homecoach";

    m_tokenRefreshTimer = new QTimer(this);
    m_tokenRefreshTimer->setSingleShot(true);
    connect(m_tokenRefreshTimer, &QTimer::timeout, this, [this](){
        qCDebug(dcNetatmo()) << "Refreshing authentication token...";
        getAccessTokenFromRefreshToken(m_refreshToken);
    });
}

QByteArray NetatmoConnection::accessToken()
{
    return m_accessToken;
}

QByteArray NetatmoConnection::refreshToken()
{
    return m_refreshToken;
}

QUrl NetatmoConnection::getLoginUrl(const QUrl &redirectUrl)
{
    m_redirectUrl = redirectUrl;

    QUrl requestUrl(m_baseUrl);
    requestUrl.setPath("/oauth2/authorize");

    QUrlQuery queryParams;
    queryParams.addQueryItem("client_id", m_clientId);
    queryParams.addQueryItem("redirect_uri", redirectUrl.toString());
    queryParams.addQueryItem("response_type", "code");
    queryParams.addQueryItem("scope", m_scopes.join(' '));
    queryParams.addQueryItem("state", QUuid::createUuid().toString());
    requestUrl.setQuery(queryParams);

    return requestUrl;
}

bool NetatmoConnection::getAccessTokenFromUsernamePassword(const QString username, const QString &password)
{
    qCDebug(dcNetatmo()) << "Starting deprecated username and password authentication" << username << censorDebugOutput(password);

    if (username.isEmpty() || password.isEmpty()) {
        qCWarning(dcNetatmo()) << "OAuth2: Failed to get tokens. Username or password is empty.";
        return false;
    }

    if (m_clientId.isEmpty()) {
        qCWarning(dcNetatmo()) << "OAuth2: Failed to refresh access token. OAuth2 client id is not set.";
        return false;
    }

    if (m_clientSecret.isEmpty()) {
        qCWarning(dcNetatmo()) << "OAuth2: Failed to refresh access token. Client secret is not set.";
        return false;
    }

    QUrl requestUrl(m_baseUrl);
    requestUrl.setPath("/oauth2/token");

    QNetworkRequest request(requestUrl);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/x-www-form-urlencoded;charset=UTF-8");

    QUrlQuery payloadQuery;
    payloadQuery.addQueryItem("grant_type", "password");
    payloadQuery.addQueryItem("client_id", m_clientId);
    payloadQuery.addQueryItem("client_secret", m_clientSecret);
    payloadQuery.addQueryItem("username", username);
    payloadQuery.addQueryItem("password", password);
    payloadQuery.addQueryItem("scope", m_scopes.join(' '));

    QNetworkReply *reply = m_networkManager->post(request, payloadQuery.toString().toUtf8());
    connect(reply, &QNetworkReply::finished, reply, &QNetworkReply::deleteLater);
    connect(reply, &QNetworkReply::finished, this, [this, reply](){
        processLoginResponse(reply->readAll());
    });

    return true;
}

bool NetatmoConnection::getAccessTokenFromRefreshToken(const QByteArray &refreshToken)
{
    if (refreshToken.isEmpty()) {
        qCWarning(dcNetatmo()) << "OAuth2: Failed to refresh access token. No refresh token given.";
        return false;
    }

    if (m_clientId.isEmpty()) {
        qCWarning(dcNetatmo()) << "OAuth2: Failed to refresh access token. OAuth2 client id is not set.";
        return false;
    }

    if (m_clientSecret.isEmpty()) {
        qCWarning(dcNetatmo()) << "OAuth2: Failed to refresh access token. Client secret is not set.";
        return false;
    }

    QUrl requestUrl(m_baseUrl);
    requestUrl.setPath("/oauth2/token");

    QNetworkRequest request(requestUrl);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/x-www-form-urlencoded;charset=UTF-8");

    QUrlQuery payloadQuery;
    payloadQuery.addQueryItem("grant_type", "refresh_token");
    payloadQuery.addQueryItem("refresh_token", refreshToken);
    payloadQuery.addQueryItem("client_id", m_clientId);
    payloadQuery.addQueryItem("client_secret", m_clientSecret);

    QNetworkReply *reply = m_networkManager->post(request, payloadQuery.toString().toUtf8());
    connect(reply, &QNetworkReply::finished, reply, &QNetworkReply::deleteLater);
    connect(reply, &QNetworkReply::finished, this, [this, reply](){
        processLoginResponse(reply->readAll());
    });

    return true;
}

bool NetatmoConnection::getAccessTokenFromAuthorizationCode(const QByteArray &authorizationCode)
{
    if (authorizationCode.isEmpty()) {
        qCWarning(dcNetatmo()) << "OAuth2: Failed to get access token. No authorization code given.";
        return false;
    }

    if (m_clientId.isEmpty()) {
        qCWarning(dcNetatmo()) << "OAuth2: Failed to get access token. OAuth2 client id is not set.";
        return false;
    }

    if (m_clientSecret.isEmpty()) {
        qCWarning(dcNetatmo()) << "OAuth2: Failed to get access token. Client secret is not set.";
        return false;
    }

    QUrl requestUrl(m_baseUrl);
    requestUrl.setPath("/oauth2/token");

    QNetworkRequest request(requestUrl);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/x-www-form-urlencoded;charset=UTF-8");

    QUrlQuery payloadQuery;
    payloadQuery.addQueryItem("grant_type", "authorization_code");
    payloadQuery.addQueryItem("client_id", m_clientId);
    payloadQuery.addQueryItem("client_secret", m_clientSecret);
    payloadQuery.addQueryItem("redirect_uri", m_redirectUrl.toString());
    payloadQuery.addQueryItem("code", authorizationCode);
    payloadQuery.addQueryItem("scope", m_scopes.join(' '));

    QNetworkReply *reply = m_networkManager->post(request, payloadQuery.toString().toUtf8());
    connect(reply, &QNetworkReply::finished, reply, &QNetworkReply::deleteLater);
    connect(reply, &QNetworkReply::finished, this, [this, reply](){
        processLoginResponse(reply->readAll());
    });

    return true;
}

QNetworkReply *NetatmoConnection::getStationData()
{
    QUrl requestUrl(m_baseUrl);
    requestUrl.setPath("/api/getstationsdata");

    QNetworkRequest request(requestUrl);
    request.setRawHeader("Authorization", "Bearer " + m_accessToken);

    return m_networkManager->get(request);
}

QString NetatmoConnection::censorDebugOutput(const QString &text)
{
    return text.mid(0, 4) + QString().fill('*', text.length() - 4);
}

void NetatmoConnection::setAuthenticated(bool authenticated)
{
    m_authenticated = authenticated;
    emit authenticatedChanged(m_authenticated);
}

void NetatmoConnection::processLoginResponse(const QByteArray &data)
{
    QJsonParseError error;
    QJsonDocument jsonDoc = QJsonDocument::fromJson(data, &error);
    if (error.error != QJsonParseError::NoError) {
        qCWarning(dcNetatmo()) << "OAuth2: Failed to get token. Refresh data reply JSON error:" << error.errorString();
        setAuthenticated(false);
        return;
    }

    QVariantMap responseMap = jsonDoc.toVariant().toMap();
    if (!responseMap.contains("access_token") || !responseMap.contains("refresh_token") ) {
        setAuthenticated(false);
        return;
    }

    m_accessToken = responseMap.value("access_token").toByteArray();
    receivedAccessToken(m_accessToken);

    m_refreshToken = responseMap.value("refresh_token").toByteArray();
    receivedRefreshToken(m_refreshToken);

    if (responseMap.contains("expires_in")) {
        int expireTime = responseMap.value("expires_in").toInt();
        qCDebug(dcNetatmo()) << "OAuth2: Token expires in" << expireTime << "s, at" << QDateTime::currentDateTime().addSecs(expireTime).toString();
        if (expireTime < 20) {
            qCWarning(dcNetatmo()) << "OAuth2: Expire time too short";
            // Refresh right the way...
            getAccessTokenFromRefreshToken(m_refreshToken);
        } else {
            m_tokenRefreshTimer->start((expireTime - 20) * 1000);
        }
    }

    setAuthenticated(true);
}
