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
#include <QJsonArray>
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
    m_tokenRefreshTimer->start(m_refreshInterval*1000); // 10 minutes
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
    qCDebug(dcMiele()) << "Getting access token from refresh token";
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
    connect(reply, &QNetworkReply::finished, this, [this, reply, refreshToken](){

        QByteArray rawData = reply->readAll();
        if (!checkStatusCode(reply, rawData)) {
            return;
        }
        QJsonDocument data = QJsonDocument::fromJson(rawData);

        if(!data.toVariant().toMap().contains("access_token") || !data.toVariant().toMap().contains("refresh_token")) {
            setAuthenticated(false);
            return;
        }
        m_accessToken = data.toVariant().toMap().value("access_token").toByteArray();
        emit receivedAccessToken(m_accessToken);

        m_refreshToken = data.toVariant().toMap().value("refresh_token").toByteArray();
        emit receivedRefreshToken(m_refreshToken);

        if (data.toVariant().toMap().contains("expires_in")) {
            int expireTime = data.toVariant().toMap().value("expires_in").toInt();
            qCDebug(dcMiele) << "Access token expires int" << expireTime << "s, at" << QDateTime::currentDateTime().addSecs(expireTime).toString();

            if (expireTime < m_refreshInterval) {
                qCWarning(dcMiele()) << "Expire time too short";
                return;
            }
            m_accessTokenExpireTime = QDateTime::currentMSecsSinceEpoch() + (expireTime-(m_refreshInterval*1000));
        }
    });
}

void Miele::getAccessTokenFromAuthorizationCode(const QByteArray &authorizationCode)
{
    qCDebug(dcMiele()) << "Getting accsss token from authorization code";
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
            int expireTime = jsonDoc.toVariant().toMap().value("expires_in").toUInt();
            qCDebug(dcMiele()) << "Token expires in" << expireTime << "s, at" << QDateTime::currentDateTime().addSecs(expireTime).toString();

            if (expireTime < m_refreshInterval) {
                qCWarning(dcMiele()) << "Expire time too short";
                return;
            }
            m_accessTokenExpireTime = QDateTime::currentMSecsSinceEpoch() + (expireTime-(m_refreshInterval*1000));
        }
    });
}



void Miele::getDevices()
{
    qCDebug(dcMiele()) << "Get devices";
    QUrl url = m_apiUrl;
    url.setPath("/v1/devices");
    url.setQuery("language=en");

    QNetworkRequest request(url);
    request.setRawHeader("Authorization", "Bearer "+m_accessToken);
    request.setRawHeader("accept", "application/json; charset=utf-8");

    qCDebug(dcMiele()) << "Sending GET request" << request.url() << request.rawHeader("Authorization");
    QNetworkReply *reply = m_networkManager->get(request);
    connect(reply, &QNetworkReply::finished, reply, &QNetworkReply::deleteLater);
    connect(reply, &QNetworkReply::finished, this, [this, reply] {

        QByteArray rawData = reply->readAll();
        if (!checkStatusCode(reply, rawData)) {
            return;
        }
        QVariantMap map = QJsonDocument::fromJson(rawData).toVariant().toMap();

        QList<DeviceShort> foundDevices;
        foreach (QVariant deviceData, map.values()) {
            QVariantMap deviceDetails = deviceData.toMap();
            if (!deviceDetails.contains("ident")) {
                qCWarning(dcMiele()) << "Got device but no ident available";
                continue;
            }
            DeviceShort ds;
            QVariantMap deviceIdentMap = deviceDetails.value("ident").toMap();
            ds.fabNumber = deviceIdentMap.value("deviceIdentLabel").toMap().value("fabNumber").toString();
            ds.name = deviceIdentMap.value("deviceName").toString();
            ds.type = static_cast<DeviceType>(deviceIdentMap.value("type").toMap().value("value_raw").toInt());
            foundDevices.append(ds);
        }
        emit devicesFound(foundDevices);
    });
}

void Miele::getDevicesShort()
{
    qCDebug(dcMiele()) << "Get devices short";
    QUrl url = m_apiUrl;
    url.setPath("/v1/short/devices");
    url.setQuery("language=en");

    QNetworkRequest request(url);
    request.setRawHeader("Authorization", "Bearer "+m_accessToken);
    request.setRawHeader("accept", "application/json; charset=utf-8");

    qCDebug(dcMiele()) << "Sending GET request" << request.url() << request.rawHeader("Authorization");
    QNetworkReply *reply = m_networkManager->get(request);
    connect(reply, &QNetworkReply::finished, reply, &QNetworkReply::deleteLater);
    connect(reply, &QNetworkReply::finished, this, [this, reply] {

        QByteArray rawData = reply->readAll();
        if (!checkStatusCode(reply, rawData)) {
            return;
        }

        QVariantList devices = QJsonDocument::fromJson(rawData).toVariant().toList();
        QList<DeviceShort> foundDevices;
        foreach (QVariant device, devices) {
            QVariantMap deviceObj = device.toMap();
            qCDebug(dcMiele()) << "Got device: " << deviceObj;
            DeviceShort ds;
            ds.fabNumber = deviceObj["fabNumber"].toString();
            ds.name = deviceObj["deviceName"].toString();
            ds.state = deviceObj["state"].toString();
            //ds.type = deviceObj["type"].toString();
            foundDevices.append(ds);
        }
        emit devicesFound(foundDevices);
    });
}

void Miele::getDevice(const QString &deviceId)
{
    qCDebug(dcMiele()) << "Get device" << deviceId;
    QUrl url = m_apiUrl;
    url.setPath("/v1/devices/" + deviceId);
    url.setQuery("language=en");

    QNetworkRequest request(url);
    request.setRawHeader("Authorization", "Bearer "+m_accessToken);
    request.setRawHeader("accept", "application/json; charset=utf-8");

    QNetworkReply *reply = m_networkManager->get(request);
    connect(reply, &QNetworkReply::finished, reply, &QNetworkReply::deleteLater);
    connect(reply, &QNetworkReply::finished, this, [this, reply]{

        QByteArray rawData = reply->readAll();
        if (!checkStatusCode(reply, rawData)) {
            return;
        }
        QVariantMap map = QJsonDocument::fromJson(rawData).toVariant().toMap();
        qCDebug(dcMiele()) << "Got device: " << map;
    });
}

void Miele::getDeviceState(const QString &deviceId) {
    qCDebug(dcMiele()) << "Get device state for: " << deviceId;
    QUrl url = m_apiUrl;
    url.setPath("/v1/devices/" + deviceId + "/state");
    url.setQuery("language=en");

    QNetworkRequest request(url);
    request.setRawHeader("Authorization", "Bearer "+m_accessToken);
    request.setRawHeader("accept", "application/json; charset=utf-8");

    QNetworkReply *reply = m_networkManager->get(request);
    connect(reply, &QNetworkReply::finished, reply, &QNetworkReply::deleteLater);
    connect(reply, &QNetworkReply::finished, this, [this, reply, deviceId]{

        QByteArray rawData = reply->readAll();
        if (!checkStatusCode(reply, rawData)) {
            int status = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
            if (status == 404) {
                emit deviceNotFound(deviceId);
            }
            return;
        }
        QVariantMap map = QJsonDocument::fromJson(rawData).toVariant().toMap();
        qCDebug(dcMiele()) << "Got device state for: " << deviceId;
        emit deviceStateReceived(deviceId, map);
    });
}

void Miele::getActions(const QString &deviceId)
{
    qCDebug(dcMiele()) << "Get actions" << deviceId;
    //TODO
}


QUuid Miele::processAction(const QString &deviceId, Miele::ProcessAction action)
{
    qCDebug(dcMiele()) << "Process action" << deviceId << action;
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
    QJsonArray tempZones;
    tempZones.push_front(temperatureObj);
    object.insert("targetTemperature", tempZones);
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

QUuid Miele::setColorsStr(const QString &deviceId, const QString &color)
{
    QJsonDocument doc;
    QJsonObject object;
    object.insert("colors", color.toLower());
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

void Miele::connectEventStream()
{
    QUrl url = m_apiUrl;
    url.setPath("/v1/devices/all/events");

    QNetworkRequest request(url);
    request.setRawHeader("Authorization", "Bearer "+m_accessToken);
    request.setRawHeader("accept", "text/event-stream");

    QNetworkReply *reply = m_networkManager->get(request);
    connect(reply, &QNetworkReply::finished, [reply, this] {
        reply->deleteLater();
        QTimer::singleShot(5000, this, [this] {connectEventStream();}); //try to reconnect every 5 seconds
    });
    connect(reply, &QNetworkReply::readyRead, this, [this, reply]{

        while (reply->canReadLine()) {
            QByteArray eventTypeLine = reply->readLine();
            if (eventTypeLine == "\n") {
                continue;
            }
            if (!eventTypeLine.startsWith("event")) {
                qCWarning(dcMiele()) << "Invalid event type line: " << eventTypeLine;
                return;
            }
            QString eventType = eventTypeLine.split(':').last().trimmed();
            if (eventType != "devices") {
                qCWarning(dcMiele()) << "Ignoring '" << eventType << "' events";
                return;
            }
            QByteArray eventDataLine = reply->readLine();
            if (!eventDataLine.startsWith("data")) {
                qCWarning(dcMiele()) << "Invalid data received: " << eventDataLine;
                return;
            }

            qCDebug(dcMiele()) << "Raw events data: " << eventDataLine;
            QJsonDocument data = QJsonDocument::fromJson(eventDataLine.replace("data: ", ""));
            qCDebug(dcMiele()) << "Got events data: " << data.toJson(QJsonDocument::Compact);
            QVariantMap eventsData = data.toVariant().toMap();
            foreach (QVariant eventEntry, eventsData.values()) {
                QVariantMap eventEntryMap = eventEntry.toMap();
                if (!eventEntryMap.contains("ident")) {
                    qCWarning(dcMiele()) << "Got event but no device ident available";
                    continue;
                }
                QVariantMap deviceIdent = eventEntryMap.value("ident").toMap().value("deviceIdentLabel").toMap();
                QString deviceId = deviceIdent.value("fabNumber").toString();
                QVariantMap state = eventEntryMap.value("state").toMap();
                emit deviceStateReceived(deviceId, state);
            }
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
        qCWarning(dcMiele()) << "Error" << error.errorString();
        setAuthenticated(false);
        return false;
    }

    setAuthenticated(true);
    return true;
}

void Miele::onRefreshTimeout()
{
    if (QDateTime::currentMSecsSinceEpoch() > m_accessTokenExpireTime) {
        qCDebug(dcMiele()) << "Refresh access token";
        getAccessTokenFromRefreshToken(m_refreshToken);
    }
}
