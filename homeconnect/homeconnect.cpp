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

HomeConnect::HomeConnect(NetworkAccessManager *networkmanager,  const QByteArray &clientKey,  const QByteArray &clientSecret, bool simulationMode, QObject *parent) :
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
    setSimulationMode(simulationMode);
}

QByteArray HomeConnect::accessToken()
{
    return m_accessToken;
}

QByteArray HomeConnect::refreshToken()
{
    return m_refreshToken;
}

void HomeConnect::setSimulationMode(bool simulation)
{
    m_simulationMode = simulation;
    if (simulation) {
        m_baseAuthorizationUrl = "https://simulator.home-connect.com/security/oauth/authorize";
        m_baseTokenUrl = "https://simulator.home-connect.com/security/oauth/token";
        m_baseControlUrl = "https://simulator.home-connect.com";
    } else {
        m_baseAuthorizationUrl = "https://api.home-connect.com/security/oauth/authorize";
        m_baseTokenUrl = "https://api.home-connect.com/security/oauth/token";
        m_baseControlUrl = "https://api.home-connect.com";
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
    queryParams.addQueryItem("scope", scope);
    queryParams.addQueryItem("state", QUuid::createUuid().toString());
    queryParams.addQueryItem("nonce", QUuid::createUuid().toString());
    m_codeChallenge = QUuid::createUuid().toString().remove('{').remove('}').remove("-");
    queryParams.addQueryItem("code_challenge", m_codeChallenge);
    queryParams.addQueryItem("code_challenge_method", "plain");
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

    QUrl url(m_baseTokenUrl);
    QUrlQuery query;
    query.clear();
    query.addQueryItem("grant_type", "refresh_token");
    query.addQueryItem("refresh_token", refreshToken);
    query.addQueryItem("client_secret", m_clientSecret);

    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/x-www-form-urlencoded");

    QNetworkReply *reply = m_networkManager->post(request, query.toString().toUtf8());
    connect(reply, &QNetworkReply::finished, reply, &QNetworkReply::deleteLater);
    connect(reply, &QNetworkReply::finished, this, [this, reply](){


        int status = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
        if (status != 200 || reply->error() != QNetworkReply::NoError) {
            qWarning(dcHomeConnect()) << "Access token error:" << reply->errorString() << reply->readAll();
            emit authenticationStatusChanged(false);
            return;
        }
        QJsonParseError error;
        QJsonDocument data = QJsonDocument::fromJson(reply->readAll(), &error);
        if (error.error != QJsonParseError::NoError) {
            emit authenticationStatusChanged(false);
            qCDebug(dcHomeConnect()) << "Received invalide JSON object" << data.toJson();
            return;
        }
        qCDebug(dcHomeConnect()) << "get access token from refresh token" << data.toJson();

        if(!data.toVariant().toMap().contains("access_token")) {
            emit authenticationStatusChanged(false);
            return;
        }
        m_accessToken = data.toVariant().toMap().value("access_token").toByteArray();

        if (data.toVariant().toMap().contains("expires_in")) {
            int expireTime = data.toVariant().toMap().value("expires_in").toInt();
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
        qWarning(dcHomeConnect) << "No authorization code given!";
    if(m_clientKey.isEmpty())
        qWarning(dcHomeConnect) << "Client key not set!";
    if(m_clientSecret.isEmpty())
        qWarning(dcHomeConnect) << "Client secret not set!";

    QUrl url = QUrl(m_baseTokenUrl);
    QUrlQuery query;    url.setQuery(query);

    query.clear();
    query.addQueryItem("client_id", m_clientKey);
    query.addQueryItem("client_secret", m_clientSecret);
    query.addQueryItem("redirect_uri", m_redirectUri);
    query.addQueryItem("grant_type", "authorization_code");
    query.addQueryItem("code", authorizationCode);
    query.addQueryItem("code_verifier", m_codeChallenge);

    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/x-www-form-urlencoded");

    QNetworkReply *reply = m_networkManager->post(request, query.toString().toUtf8());
    connect(reply, &QNetworkReply::finished, reply, &QNetworkReply::deleteLater);
    connect(reply, &QNetworkReply::finished, this, [this, reply](){
        int status = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
        if (status != 200 || reply->error() != QNetworkReply::NoError) {
            qWarning(dcHomeConnect()) << reply->errorString() << status << reply->readAll();
            emit authenticationStatusChanged(false);
            return;
        }
        QJsonDocument jsonDoc = QJsonDocument::fromJson(reply->readAll());
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
            qCDebug(dcHomeConnect()) << "Token expires at" << QDateTime::currentDateTime().addSecs(expireTime).toString();
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

void HomeConnect::getHomeAppliances()
{
    QUrl url = QUrl(m_baseControlUrl+"/api/homeappliances");

    QNetworkRequest request(url);
    request.setRawHeader("Authorization", "Bearer "+m_accessToken);
    request.setRawHeader("accept", "application/vnd.bsh.sdk.v1+json");

    QNetworkReply *reply = m_networkManager->get(request);
    connect(reply, &QNetworkReply::finished, this, [this, reply](){
        reply->deleteLater();

        QJsonParseError error;
        QJsonDocument data = QJsonDocument::fromJson(reply->readAll(), &error);
        if (error.error != QJsonParseError::NoError) {
            qCDebug(dcHomeConnect()) << "Get home appliances: Recieved invalide JSON object";
            return;
        }
        qCDebug(dcHomeConnect()) << "Get home appliances" << data.toJson();
        if (data.toVariant().toMap().contains("data")) {
            QVariantMap dataMap = data.toVariant().toMap().value("data").toMap();
            QList<HomeAppliance> appliances;
            foreach (const QVariant &variant, dataMap.value("homeappliances").toList()) {
                QVariantMap obj = variant.toMap();
                HomeAppliance appliance;
                appliance.name = obj["name"].toString();
                appliance.brand = obj["brand"].toString();
                appliance.vib = obj["vib"].toString();
                appliance.type = obj["type"].toString();
                appliance.homeApplianceId = obj["haId"].toString();
                appliance.enumber = obj["enumber"].toString();
                appliance.connected = obj["connected"].toBool();
                appliances.append(appliance);
            }
            if (!appliances.isEmpty())
                emit receivedHomeAppliances(appliances);
        } else if (data.toVariant().toMap().contains("error")) {
            qCWarning(dcHomeConnect()) << "Get home appliences" << data.toVariant().toMap().value("error").toMap().value("description").toString();
        }
    });
}

void HomeConnect::getProgramsAvailable(const QString &haId)
{
    QUrl url = QUrl(m_baseControlUrl+"/api/homeappliances/"+haId+"/programs/available");

    QNetworkRequest request(url);
    request.setRawHeader("Authorization", "Bearer "+m_accessToken);
    request.setRawHeader("Accept-Language", "en-US");
    request.setRawHeader("accept", "application/vnd.bsh.sdk.v1+json");

    QNetworkReply *reply = m_networkManager->get(request);
    connect(reply, &QNetworkReply::finished, reply, &QNetworkReply::deleteLater);
    connect(reply, &QNetworkReply::finished, this, [this, reply]{

        QJsonParseError error;
        QJsonDocument data = QJsonDocument::fromJson(reply->readAll(), &error);
        if (error.error != QJsonParseError::NoError) {
            qCWarning(dcHomeConnect()) << "Get programs available: Recieved invalide JSON object";
            return;
        }
        qCDebug(dcHomeConnect()) << "Get programs available" << data.toJson();

    });
}

void HomeConnect::getProgramsActiveOption(const QString &haId, const QString &optionKey)
{
    QUrl url = QUrl(m_baseControlUrl+"/api/homeappliances/"+haId+"/programs/active/options/"+optionKey);

    QNetworkRequest request(url);
    request.setRawHeader("Authorization", "Bearer "+m_accessToken);
    request.setRawHeader("Accept-Language", "en-US");
    request.setRawHeader("accept", "application/vnd.bsh.sdk.v1+json");

    QNetworkReply *reply = m_networkManager->get(request);
    connect(reply, &QNetworkReply::finished, reply, &QNetworkReply::deleteLater);
    connect(reply, &QNetworkReply::finished, this, [this, reply]{

        QJsonParseError error;
        QJsonDocument data = QJsonDocument::fromJson(reply->readAll(), &error);
        if (error.error != QJsonParseError::NoError) {
            qCDebug(dcHomeConnect()) << "Get home appliances: Recieved invalide JSON object";
            return;
        }
        qCDebug(dcHomeConnect()) << "Get home appliances" << data.toJson();
        if (data.toVariant().toMap().contains("data")) {
            QVariantMap dataMap = data.toVariant().toMap().value("data").toMap();
            qCDebug(dcHomeConnect()) << "key" << dataMap.value("key").toString() << "value" << dataMap.value("value").toString() << dataMap.value("unit").toString();
        } else if (data.toVariant().toMap().contains("error")) {
            qCWarning(dcHomeConnect()) << "Get home appliences" << data.toVariant().toMap().value("error").toMap().value("description").toString();
        }
    });
}

void HomeConnect::getStatus(const QString &haid)
{
    QUrl url = QUrl(m_baseControlUrl+"/api/homeappliances/"+haid+"/status");

    QNetworkRequest request(url);
    request.setRawHeader("Authorization", "Bearer "+m_accessToken);
    request.setRawHeader("Accept-Language", "en-US");
    request.setRawHeader("accept", "application/vnd.bsh.sdk.v1+json");

    QNetworkReply *reply = m_networkManager->get(request);
    connect(reply, &QNetworkReply::finished, reply, &QNetworkReply::deleteLater);
    connect(reply, &QNetworkReply::finished, this, [this, reply]{


        // Remote control activation state
        // Remote start allowance state
        // Local control state
        // Operation state
        // Door state

        QJsonParseError error;
        QJsonDocument data = QJsonDocument::fromJson(reply->readAll(), &error);
        if (error.error != QJsonParseError::NoError) {
            qCDebug(dcHomeConnect()) << "Get home appliances: Recieved invalide JSON object";
            return;
        }
        qCDebug(dcHomeConnect()) << "Get home appliances" << data.toJson();
        if (data.toVariant().toMap().contains("data")) {
            QVariantMap dataMap = data.toVariant().toMap().value("data").toMap();
            qCDebug(dcHomeConnect()) << "key" << dataMap.value("key").toString() << "value" << dataMap.value("value").toString() << dataMap.value("unit").toString();
        } else if (data.toVariant().toMap().contains("error")) {
            qCWarning(dcHomeConnect()) << "Get home appliences" << data.toVariant().toMap().value("error").toMap().value("description").toString();
        }
    });
}

/* Get a list of available setting of the home appliance.
 * Possible Settings:
 *      Power state
 *      Fridge temperature
 *      Fridge super mode
 *      Freezer temperature
 *      Freezer super mode
 */

void HomeConnect::getSettings(const QString &haid)
{
    QUrl url = QUrl(m_baseControlUrl+"/api/homeappliances/"+haid+"/settings");

    QNetworkRequest request(url);
    request.setRawHeader("Authorization", "Bearer "+m_accessToken);
    request.setRawHeader("Accept-Language", "en-US");
    request.setRawHeader("accept", "application/vnd.bsh.sdk.v1+json");

    QNetworkReply *reply = m_networkManager->get(request);
    connect(reply, &QNetworkReply::finished, reply, &QNetworkReply::deleteLater);
    connect(reply, &QNetworkReply::finished, this, [this, reply]{

        QJsonParseError error;
        QJsonDocument data = QJsonDocument::fromJson(reply->readAll(), &error);
        if (error.error != QJsonParseError::NoError) {
            qCDebug(dcHomeConnect()) << "Get settings: Recieved invalide JSON object";
            return;
        }
        qCDebug(dcHomeConnect()) << "Get settings" << data.toJson();
        if (data.toVariant().toMap().contains("data")) {
            QVariantMap dataMap = data.toVariant().toMap().value("data").toMap();
            qCDebug(dcHomeConnect()) << "key" << dataMap.value("key").toString() << "value" << dataMap.value("value").toString() << dataMap.value("unit").toString();
        } else if (data.toVariant().toMap().contains("error")) {
            qCWarning(dcHomeConnect()) << "Get settings" << data.toVariant().toMap().value("error").toMap().value("description").toString();
        }
    });
}

QUuid HomeConnect::sendCommand(const QString &haid, const QString &command)
{
    QUuid commandId = QUuid::createUuid();
    QUrl url = QUrl(m_baseControlUrl+"/api/homeappliances/"+haid+"/commands/"+command);

    QNetworkRequest request(url);
    request.setRawHeader("Authorization", "Bearer "+m_accessToken);
    request.setRawHeader("Accept-Language", "en-US");
    request.setRawHeader("accept", "application/vnd.bsh.sdk.v1+json");

    QJsonDocument doc;
    QJsonObject data;
    data.insert("key", command);
    data.insert("value", true);
    QJsonObject obj;
    obj.insert("data", data);
    doc.setObject(obj);
    QNetworkReply *reply = m_networkManager->put(request, doc.toJson());
    connect(reply, &QNetworkReply::finished, reply, &QNetworkReply::deleteLater);
    connect(reply, &QNetworkReply::finished, this, [this, commandId, reply]{

        QJsonParseError error;
        QJsonDocument data = QJsonDocument::fromJson(reply->readAll(), &error);
        if (error.error != QJsonParseError::NoError) {
            qCDebug(dcHomeConnect()) << "Send command: Recieved invalide JSON object";
            return;
        }
        qCDebug(dcHomeConnect()) << "Send command" << data.toJson();
        if (data.toVariant().toMap().contains("data")) {
            QVariantMap dataMap = data.toVariant().toMap().value("data").toMap();
            qCDebug(dcHomeConnect()) << "key" << dataMap.value("key").toString() << "value" << dataMap.value("value").toString() << dataMap.value("unit").toString();
        } else if (data.toVariant().toMap().contains("error")) {
            qCWarning(dcHomeConnect()) << "Send command" << data.toVariant().toMap().value("error").toMap().value("description").toString();
        }
        emit commandExecuted(commandId, true);
    });
    return commandId;
}
