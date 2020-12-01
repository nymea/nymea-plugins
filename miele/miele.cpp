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

#include "miele.h"
#include "extern-plugininfo.h"

#include <QNetworkReply>
#include <QNetworkRequest>
#include <QUrl>
#include <QJsonObject>
#include <QUrlQuery>
#include <QMetaEnum>

Miele::Miele(NetworkAccessManager *networkmanager, const QByteArray &clientId, const QByteArray &clientSecret, const QString &language, QObject *parent) :
    QObject(parent),
    m_language(language),
    m_clientId(clientId),
    m_clientSecret(clientSecret),
    m_networkManager(networkmanager)
{
    m_tokenRefreshTimer = new QTimer(this);
    m_tokenRefreshTimer->setSingleShot(true);
    connect(m_tokenRefreshTimer, &QTimer::timeout, this, &Miele::onRefreshTimeout);
}

QByteArray Miele::accessToken()
{
    return m_accessToken;
}

QByteArray Miele::refreshToken()
{
    return m_refreshToken;
}

QUrl Miele::getLoginUrl(const QUrl &redirectUrl, const QString &state)
{
    if (m_clientId.isEmpty()) {
        qCWarning(dcMiele) << "Client key not defined!";
        return QUrl("");
    }

    if (redirectUrl.isEmpty()){
        qCWarning(dcMiele()) << "No redirect uri defined!";
    }
    m_redirectUri = QUrl::toPercentEncoding(redirectUrl.toString());

    QUrl url(m_authorizationUrl);
    QUrlQuery queryParams;
    queryParams.addQueryItem("client_id", m_clientId);
    queryParams.addQueryItem("redirect_uri", m_redirectUri);
    queryParams.addQueryItem("response_type", "code");
    if (!state.isEmpty())
        queryParams.addQueryItem("state", state);
    url.setQuery(queryParams);

    return url;
}

void Miele::getAccessTokenFromRefreshToken(const QByteArray &refreshToken)
{
    if (refreshToken.isEmpty()) {
        qCWarning(dcMiele()) << "No refresh token given!";
        setAuthenticated(false);
        return;
    }

    QUrl url(m_tokenUrl);
    QUrlQuery query;
    query.clear();
    query.addQueryItem("grant_type", "refresh_token");
    query.addQueryItem("refresh_token", refreshToken);
    query.addQueryItem("client_id", m_clientId);
    query.addQueryItem("client_secret", m_clientSecret);

    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/x-www-form-urlencoded");

    QNetworkReply *reply = m_networkManager->post(request, query.toString().toUtf8());
    connect(reply, &QNetworkReply::finished, reply, &QNetworkReply::deleteLater);
    connect(reply, &QNetworkReply::finished, this, [this, reply](){

        QByteArray rawData = reply->readAll();
        if (!checkStatusCode(reply, rawData)) {
            return;
        }
        QJsonDocument data = QJsonDocument::fromJson(rawData);

        if(!data.toVariant().toMap().contains("access_token")) {
            setAuthenticated(false);
            return;
        }
        m_accessToken = data.toVariant().toMap().value("access_token").toByteArray();
        emit receivedAccessToken(m_accessToken);

        if (data.toVariant().toMap().contains("expires_in")) {
            int expireTime = data.toVariant().toMap().value("expires_in").toInt();
            qCDebug(dcMiele) << "Access token expires int" << expireTime << "s, at" << QDateTime::currentDateTime().addSecs(expireTime).toString();
            if (!m_tokenRefreshTimer) {
                qWarning(dcMiele()) << "Access token refresh timer not initialized";
                return;
            }
            if (expireTime < 20) {
                qCWarning(dcMiele()) << "Expire time too short";
                return;
            }
            m_tokenRefreshTimer->start((expireTime - 20) * 1000);
        }
    });
}

void Miele::getAccessTokenFromAuthorizationCode(const QByteArray &authorizationCode)
{
    if(authorizationCode.isEmpty())
        qCWarning(dcMiele) << "No authorization code given!";

    QUrl url = QUrl(m_tokenUrl);
    QUrlQuery query;
    query.clear();
    query.addQueryItem("client_id", m_clientId);
    query.addQueryItem("client_secret", m_clientSecret);
    query.addQueryItem("vg", "de-DE");
    query.addQueryItem("redirect_uri", m_redirectUri);
    query.addQueryItem("grant_type", "authorization_code");
    query.addQueryItem("code", authorizationCode);
    url.setQuery(query);

    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/x-www-form-urlencoded");

    QNetworkReply *reply = m_networkManager->post(request, query.toString().toUtf8());
    connect(reply, &QNetworkReply::finished, reply, &QNetworkReply::deleteLater);
    connect(reply, &QNetworkReply::finished, this, [this, reply](){

        QByteArray rawData = reply->readAll();
        if (!checkStatusCode(reply, rawData)) {
            return;
        }
        QJsonDocument jsonDoc = QJsonDocument::fromJson(rawData);
        if(!jsonDoc.toVariant().toMap().contains("access_token") || !jsonDoc.toVariant().toMap().contains("refresh_token") ) {
            setAuthenticated(false);
            return;
        }
        m_accessToken = jsonDoc.toVariant().toMap().value("access_token").toByteArray();
        receivedAccessToken(m_accessToken);
        m_refreshToken = jsonDoc.toVariant().toMap().value("refresh_token").toByteArray();
        receivedRefreshToken(m_refreshToken);

        if (jsonDoc.toVariant().toMap().contains("expires_in")) {
            int expireTime = jsonDoc.toVariant().toMap().value("expires_in").toInt();
            qCDebug(dcMiele()) << "Token expires in" << expireTime << "s, at" << QDateTime::currentDateTime().addSecs(expireTime).toString();
            if (!m_tokenRefreshTimer) {
                qWarning(dcMiele()) << "Token refresh timer not initialized";
                setAuthenticated(false);
                return;
            }
            if (expireTime < 20) {
                qCWarning(dcMiele()) << "Expire time too short";
                return;
            }
            m_tokenRefreshTimer->start((expireTime - 20) * 1000);
        }
    });
}



void Miele::getDevices()
{
    QUrl url = m_apiUrl;
    url.setPath("/v1/devices");
    url.setQuery("language=en");

    QNetworkRequest request(url);
    request.setRawHeader("access_token", m_accessToken);
    request.setRawHeader("language", "en");
    request.setRawHeader("accept", "application/json; charset=utf-8");

    QNetworkReply *reply = m_networkManager->get(request);
    connect(reply, &QNetworkReply::finished, reply, &QNetworkReply::deleteLater);
    connect(reply, &QNetworkReply::finished, this, [this, reply]{

        QByteArray rawData = reply->readAll();
        if (!checkStatusCode(reply, rawData)) {
            return;
        }
        QVariantMap map = QJsonDocument::fromJson(rawData).toVariant().toMap();
        qCDebug(dcMiele()) << "Get devices" << map;
        if (map.contains("data")) {
            QVariantMap dataMap = map.value("data").toMap();
            qCDebug(dcMiele()) << "key" << dataMap.value("key").toString() << "value" << dataMap.value("value").toString() << dataMap.value("unit").toString();
        } else if (map.contains("error")) {
            qCWarning(dcMiele()) << "Send command" << map.value("error").toMap().value("description").toString();
        }
    });
}

void Miele::getDevice(const QString &deviceId)
{
    QUrl url = m_apiUrl;
    url.setPath("/v1/devices/"+deviceId);
    url.setQuery("language=en");

    QNetworkRequest request(url);
    request.setRawHeader("access_token", m_accessToken);
    request.setRawHeader("language", "en");
    request.setRawHeader("accept", "application/json; charset=utf-8");

    QNetworkReply *reply = m_networkManager->get(request);
    connect(reply, &QNetworkReply::finished, reply, &QNetworkReply::deleteLater);
    connect(reply, &QNetworkReply::finished, this, [this, reply]{

        QByteArray rawData = reply->readAll();
        if (!checkStatusCode(reply, rawData)) {
            return;
        }
        QVariantMap map = QJsonDocument::fromJson(rawData).toVariant().toMap();
        qCDebug(dcMiele()) << "Get device" << map;
    });
}

void Miele::getActions(const QString &deviceId)
{
    Q_UNUSED(deviceId)
}


QUuid Miele::processAction(const QString &deviceId, Miele::ProcessAction action)
{
    QJsonDocument doc;
    QJsonObject object;
    object.insert("processAction", action);
    doc.setObject(object);
    return putAction(deviceId, doc);
}

QUuid Miele::setPower(const QString &deviceId, bool power)
{
    QJsonDocument doc;
    QJsonObject object;
    if (power) {
        object.insert("powerOn", true);
    } else {
        object.insert("powerOff", true);
    }
    doc.setObject(object);
    return putAction(deviceId, doc);
}

QUuid Miele::setDeviceName(const QString &deviceId, const QString &deviceName)
{
    QJsonDocument doc;
    QJsonObject object;
    object.insert("description", deviceName);
    doc.setObject(object);
    return putAction(deviceId, doc);
}

QUuid Miele::setLight(const QString &deviceId, bool power)
{
    QJsonDocument doc;
    QJsonObject object;
    if (power) {
        object.insert("light", 1);
    } else {
        object.insert("light", 2);
    }
    doc.setObject(object);
    return putAction(deviceId, doc);
}

QUuid Miele::setTargetTemperature(const QString &deviceId, int zone, int targetTemperature)
{
    QJsonDocument doc;
    QJsonObject object;
    QJsonObject temperatureObj;
    temperatureObj.insert("zone", zone);
    temperatureObj.insert("value", targetTemperature);
    object.insert("targetTemperature", temperatureObj);
    doc.setObject(object);
    return putAction(deviceId, doc);
}

QUuid Miele::setColors(const QString &deviceId, Miele::Color color)
{
    QJsonDocument doc;
    QJsonObject object;
    QString colorString = QMetaEnum::fromType<Miele::Color>().valueToKey(color);
    object.insert("color", colorString.remove("Color").toLower());
    doc.setObject(object);
    return putAction(deviceId, doc);
}

QUuid Miele::setModes(const QString &deviceId, Miele::Mode mode)
{
    QJsonDocument doc;
    QJsonObject object;
    object.insert("modes", mode);
    doc.setObject(object);
    return putAction(deviceId, doc);
}

QUuid Miele::setVentilationStep(const QString &deviceId, int step)
{
    QJsonDocument doc;
    QJsonObject object;
    object.insert("ventilationStep", step);
    doc.setObject(object);
    return putAction(deviceId, doc);
}

QUuid Miele::setStartTime(const QString &deviceId, int seconds)
{
    QJsonDocument doc;
    QJsonObject object;
    object.insert("startTime", seconds);
    doc.setObject(object);
    return putAction(deviceId, doc);
}

void Miele::getAllEvents()
{
    QUrl url = m_apiUrl;
    url.setPath("/v1/devices/all/events");

    QNetworkRequest request(url);
    request.setRawHeader("authorization", m_accessToken);
    request.setRawHeader("accept", "*/*");

    QNetworkReply *reply = m_networkManager->get(request);
    connect(reply, &QNetworkReply::finished, [reply, this] {
        reply->deleteLater();
        QTimer::singleShot(5000, this, [this] {getAllEvents();}); //try to reconnect every 5 seconds
    });
    connect(reply, &QNetworkReply::readyRead, this, [this, reply]{

        while (reply->canReadLine()) {

        }
    });
}

QUuid Miele::putAction(const QString &deviceId, const QJsonDocument &action)
{
    QUuid commandId = QUuid::createUuid();
    QUrl url = m_apiUrl;
    url.setPath("/v1/devices/"+deviceId+"/actions");
    url.setQuery("language=en");

    QNetworkRequest request(url);
    request.setRawHeader("Authorization", "Bearer "+m_accessToken);
    // request.setRawHeader("Accept-Language", "en-US");
    request.setRawHeader("accept", "application/json; charset=utf-8");
    request.setHeader(QNetworkRequest::KnownHeaders::ContentTypeHeader, "application/json");

    QNetworkReply *reply = m_networkManager->put(request, action.toJson());
    connect(reply, &QNetworkReply::finished, reply, &QNetworkReply::deleteLater);
    connect(reply, &QNetworkReply::finished, this, [this, commandId, reply]{

        QByteArray rawData = reply->readAll();
        if (!checkStatusCode(reply, rawData)) {
            return;
        }
        QVariantMap map = QJsonDocument::fromJson(rawData).toVariant().toMap();
        qCDebug(dcMiele()) << "Send command" << map;
        if (map.contains("data")) {
            QVariantMap dataMap = map.value("data").toMap();
            qCDebug(dcMiele()) << "key" << dataMap.value("key").toString() << "value" << dataMap.value("value").toString() << dataMap.value("unit").toString();
        } else if (map.contains("error")) {
            qCWarning(dcMiele()) << "Send command" << map.value("error").toMap().value("description").toString();
        }
        emit commandExecuted(commandId, true);
    });
    return commandId;
}

void Miele::setAuthenticated(bool state)
{
    if (state != m_authenticated) {
        m_authenticated = state;
        emit authenticationStatusChanged(state);
    }
}

void Miele::setConnected(bool state)
{
    if (state != m_connected) {
        m_connected = state;
        emit connectionChanged(state);
    }
}

bool Miele::checkStatusCode(QNetworkReply *reply, const QByteArray &rawData)
{
    // Check for the internet connection
    if (reply->error() == QNetworkReply::NetworkError::HostNotFoundError ||
            reply->error() == QNetworkReply::NetworkError::UnknownNetworkError ||
            reply->error() == QNetworkReply::NetworkError::TemporaryNetworkFailureError) {
        qCWarning(dcMiele()) << "Connection error" << reply->errorString();
        setConnected(false);
        setAuthenticated(false);
        return false;
    } else {
        setConnected(true);
    }
    int status = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
    QJsonParseError error;
    QJsonDocument jsonDoc = QJsonDocument::fromJson(rawData, &error);

    switch (status){
    case 200: //The request was successful. Typically returned for successful GET requests.
    case 204: //The request was successful. Typically returned for successful PUT/DELETE requests with no payload.
        break;
    case 400: //Error occurred (e.g. validation error - value is out of range)
        if(!jsonDoc.toVariant().toMap().contains("error")) {
            if(jsonDoc.toVariant().toMap().value("error").toString() == "invalid_client") {
                qWarning(dcMiele()) << "Client token provided doesn’t correspond to client that generated auth code.";
            }
            if(jsonDoc.toVariant().toMap().value("error").toString() == "invalid_redirect_uri") {
                qWarning(dcMiele()) << "Missing redirect_uri parameter.";
            }
            if(jsonDoc.toVariant().toMap().value("error").toString() == "invalid_code") {
                qWarning(dcMiele()) << "Expired authorization code.";
            }
        }
        setAuthenticated(false);
        return false;
    case 401:
        qWarning(dcMiele()) << "Client does not have permission to use this API.";
        setAuthenticated(false);
        return false;
    case 403:
        qCWarning(dcMiele()) << "Forbidden, Scope has not been granted or home appliance is not assigned to HC account";
        setAuthenticated(false);
        return false;
    case 404:
        qCWarning(dcMiele()) << "Not Found. This resource is not available (e.g. no images on washing machine)";
        return false;
    case 405:
        qWarning(dcMiele()) << "Wrong HTTP method used.";
        setAuthenticated(false);
        return false;
    case 408:
        qCWarning(dcMiele())<< "Request Timeout, API Server failed to produce an answer or has no connection to backend service";
        return false;
    case 409:
        qCWarning(dcMiele()) << "Conflict - Command/Query cannot be executed for the home appliance, the error response contains the error details";
        qCWarning(dcMiele()) << "Error" << jsonDoc.toVariant().toMap().value("error").toString();
        return false;
    case 415:
        qCWarning(dcMiele())<< "Unsupported Media Type. The request's Content-Type is not supported";
        return false;
    case 429:
        qCWarning(dcMiele())<< "Too Many Requests, the number of requests for a specific endpoint exceeded the quota of the client";
        return false;
    case 500:
        qCWarning(dcMiele())<< "Internal Server Error, in case of a server configuration error or any errors in resource files";
        return false;
    case 503:
        qCWarning(dcMiele())<< "Service Unavailable,if a required backend service is not available";
        return false;
    default:
        break;
    }

    if (error.error != QJsonParseError::NoError) {
        qCWarning(dcMiele()) << "Received invalide JSON object" << rawData;
        qCWarning(dcMiele()) << "Status" << status;
        setAuthenticated(false);
        return false;
    }

    setAuthenticated(true);
    return true;
}

void Miele::onRefreshTimeout()
{
    qCDebug(dcMiele()) << "Refresh authentication token";
    getAccessTokenFromRefreshToken(m_refreshToken);
}
