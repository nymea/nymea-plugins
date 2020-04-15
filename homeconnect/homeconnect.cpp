/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*
* Copyright 2013 - 2020, nymea GmbH
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

#include "homeconnect.h"
#include "extern-plugininfo.h"

#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QUrlQuery>

HomeConnect::HomeConnect(NetworkAccessManager *networkmanager,  const QByteArray &clientKey,  const QByteArray &clientSecret, QObject *parent) :
    QObject(parent),
    m_clientKey(clientKey),
    m_clientSecret(clientSecret),
    m_networkManager(networkmanager)
{
    if(!m_tokenRefreshTimer) {
        m_tokenRefreshTimer = new QTimer(this);
        m_tokenRefreshTimer->setSingleShot(true);
        connect(m_tokenRefreshTimer, &QTimer::timeout, this, &HomeConnect::onRefreshTimeout);
    }
}

QUrl HomeConnect::getLoginUrl(const QUrl &redirectUrl, const QString &scope)
{
    if (m_clientKey.isEmpty()) {
        qWarning(dcHomeConnect) << "Client key not defined!";
        return QUrl("");
    }

    if (redirectUrl.isEmpty()){
        qWarning(dcHomeConnect) << "No redirect uri defined!";
    }
    m_redirectUri = QUrl::toPercentEncoding(redirectUrl.toString());

    QUrl url(m_baseAuthorizationUrl);
    QUrlQuery queryParams;
    queryParams.addQueryItem("client_id", m_clientKey);
    queryParams.addQueryItem("redirect_uri", m_redirectUri);
    queryParams.addQueryItem("response_type", "code");
    queryParams.addQueryItem("scope", "TODO");
    queryParams.addQueryItem("state", QUuid::createUuid().toString());
    queryParams.addQueryItem("nonce", QUuid::createUuid().toString());
    //queryParams.addQueryItem("code_challenge", QUuid::createUuid().toString());
    //queryParams.addQueryItem("code_challenge_method", QUuid::createUuid().toString());
    url.setQuery(queryParams);

    return url;
}

void HomeConnect::onRefreshTimeout()
{
    qCDebug(dcHomeConnect) << "Refresh authentication token";
    getAccessTokenFromRefreshToken(m_refreshToken);
}

void HomeConnect::checkStatusCode(int status, const QByteArray &payload)
{
    QJsonDocument jsonDoc = QJsonDocument::fromJson(payload);

    switch (status){
    case 400:
        if(!jsonDoc.toVariant().toMap().contains("error")) {
            if(jsonDoc.toVariant().toMap().value("error").toString() == "invalid_client") {
                qWarning(dcHomeConnect()) << "Client token provided doesn’t correspond to client that generated auth code.";
            }
            if(jsonDoc.toVariant().toMap().value("error").toString() == "invalid_redirect_uri") {
                qWarning(dcHomeConnect()) << "Missing redirect_uri parameter.";
            }
            if(jsonDoc.toVariant().toMap().value("error").toString() == "invalid_code") {
                qWarning(dcHomeConnect()) << "Expired authorization code.";
            }
        }
        return;
    case 401:
        qWarning(dcHomeConnect()) << "Client does not have permission to use this API.";
        return;
    case 405:
        qWarning(dcHomeConnect()) << "Wrong HTTP method used.";
        return;
    default:
        break;
    }
}

void HomeConnect::getAccessTokenFromRefreshToken(const QByteArray &refreshToken)
{
    if (refreshToken.isEmpty()) {
        qWarning(dcHomeConnect) << "No refresh token given!";
        emit authenticationStatusChanged(false);
        return;
    }

    QUrl url(m_baseAuthorizationUrl);
    QUrlQuery query;
    query.clear();
    query.addQueryItem("grant_type", "refresh_token");
    query.addQueryItem("refresh_token", refreshToken);
    url.setQuery(query);

    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/x-www-form-urlencoded; charset=UTF-8");

    QByteArray auth = QByteArray(m_clientKey + ':' + m_clientSecret).toBase64(QByteArray::Base64Encoding | QByteArray::KeepTrailingEquals);
    request.setRawHeader("Authorization", QString("Basic %1").arg(QString(auth)).toUtf8());

    QNetworkReply *reply = m_networkManager->post(request, QByteArray());
    connect(reply, &QNetworkReply::finished, this, [this, reply](){
        reply->deleteLater();

        QJsonDocument jsonDoc = QJsonDocument::fromJson(reply->readAll());
        int status = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
        if (status != 200 || reply->error() != QNetworkReply::NoError) {
            if(jsonDoc.toVariant().toMap().contains("error_description")) {
                qWarning(dcHomeConnect()) << "Access token error:" << jsonDoc.toVariant().toMap().value("error_description").toString();
            }
            emit authenticationStatusChanged(false);
            return;
        }
        if(!jsonDoc.toVariant().toMap().contains("access_token")) {
            emit authenticationStatusChanged(false);
            return;
        }
        m_accessToken = jsonDoc.toVariant().toMap().value("access_token").toByteArray();

        if (jsonDoc.toVariant().toMap().contains("expires_in")) {
            int expireTime = jsonDoc.toVariant().toMap().value("expires_in").toInt();
            qCDebug(dcHomeConnect) << "Access token expires at" << QDateTime::currentDateTime().addSecs(expireTime).toString();
            if (!m_tokenRefreshTimer) {
                qWarning(dcHomeConnect()) << "Access token refresh timer not initialized";
                return;
            }
            m_tokenRefreshTimer->start((expireTime - 20) * 1000);
        }
        emit authenticationStatusChanged(true);;
    });
}

void HomeConnect::getAccessTokenFromAuthorizationCode(const QByteArray &authorizationCode)
{
    // Obtaining access token
    if(authorizationCode.isEmpty())
        qWarning(dcHomeConnect) << "No auhtorization code given!";
    if(m_clientKey.isEmpty())
        qWarning(dcHomeConnect) << "Client key not set!";
    if(m_clientSecret.isEmpty())
        qWarning(dcHomeConnect) << "Client secret not set!";

    QUrl url = QUrl(m_baseAuthorizationUrl);
    QUrlQuery query;
    query.clear();
    query.addQueryItem("grant_type", "authorization_code");
    query.addQueryItem("code", authorizationCode);
    query.addQueryItem("redirect_uri", m_redirectUri);
    url.setQuery(query);

    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/x-www-form-urlencoded;charset=utf-8");

    QByteArray auth = QByteArray(m_clientKey + ':' + m_clientSecret).toBase64(QByteArray::Base64Encoding | QByteArray::KeepTrailingEquals);
    request.setRawHeader("Authorization", QString("Basic %1").arg(QString(auth)).toUtf8());

    QNetworkReply *reply = m_networkManager->post(request, QByteArray());
    connect(reply, &QNetworkReply::finished, this, [this, reply](){
        reply->deleteLater();
        QJsonDocument jsonDoc = QJsonDocument::fromJson(reply->readAll());

        int status = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();

        qCDebug(dcHomeConnect()) << "HomeConnect accessToken reply:" << this << reply->error() << reply->errorString() << jsonDoc.toJson();
        if(!jsonDoc.toVariant().toMap().contains("access_token") || !jsonDoc.toVariant().toMap().contains("refresh_token") ) {
            emit authenticationStatusChanged(false);
            return;
        }
        qCDebug(dcHomeConnect()) << "Access token:" << jsonDoc.toVariant().toMap().value("access_token").toString();
        m_accessToken = jsonDoc.toVariant().toMap().value("access_token").toByteArray();

        qCDebug(dcHomeConnect()) << "Refresh token:" << jsonDoc.toVariant().toMap().value("refresh_token").toString();
        m_refreshToken = jsonDoc.toVariant().toMap().value("refresh_token").toByteArray();

        if (jsonDoc.toVariant().toMap().contains("expires_in")) {
            int expireTime = jsonDoc.toVariant().toMap().value("expires_in").toInt();
            qCDebug(dcHomeConnect()) << "expires at" << QDateTime::currentDateTime().addSecs(expireTime).toString();
            if (!m_tokenRefreshTimer) {
                qWarning(dcHomeConnect()) << "Token refresh timer not initialized";
                emit authenticationStatusChanged(false);
                return;
            }
            m_tokenRefreshTimer->start((expireTime - 20) * 1000);
        }
        emit authenticationStatusChanged(true);
    });
}
