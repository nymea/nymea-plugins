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

#include "tado.h"
#include "extern-plugininfo.h"

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QUrlQuery>

Tado::Tado(NetworkAccessManager *networkManager, QObject *parent)
    : QObject(parent)
    , m_networkManager(networkManager)
{
    m_baseControlUrl = "https://my.tado.com/api/v2";
    m_baseAuthorizationUrl = "https://login.tado.com/oauth2";

    m_clientId = "1bb50063-6b0c-4d11-bd99-387f4a91cc46";

    m_refreshTimer.setSingleShot(true);
    connect(&m_refreshTimer, &QTimer::timeout, this, [this]() {
        qCDebug(dcTado()) << "Refresh token...";
        getAccessToken();
    });

    m_pollAuthenticationTimer.setSingleShot(true);
    m_pollAuthenticationTimer.setInterval(2000);
    connect(&m_pollAuthenticationTimer, &QTimer::timeout, this, [this]() {
        qCDebug(dcTado()) << "Checking authentication status...";
        requestAuthenticationToken();
    });
}

bool Tado::apiAvailable()
{
    return m_apiAvailable;
}

bool Tado::authenticated()
{
    return m_authenticationStatus;
}

bool Tado::connected()
{
    return m_connectionStatus;
}

QString Tado::loginUrl() const
{
    return m_loginUrl;
}

QString Tado::username() const
{
    return m_username;
}

QString Tado::refreshToken() const
{
    return m_refreshToken;
}

void Tado::setRefreshToken(const QString &refreshToken)
{
    m_refreshToken = refreshToken;
}

void Tado::startAuthentication()
{
    qCDebug(dcTado()) << "Start authentication process...";
    m_pollAuthenticationCount = 0;
    requestAuthenticationToken();
}

void Tado::getLoginUrl()
{
    QNetworkRequest request = QNetworkRequest(QUrl(m_baseAuthorizationUrl + "/device_authorize"));
    request.setHeader(QNetworkRequest::KnownHeaders::ContentTypeHeader, "application/x-www-form-urlencoded");

    QUrlQuery query;
    query.addQueryItem("client_id", m_clientId);
    query.addQueryItem("scope", "offline_access");

    QByteArray payload = query.toString(QUrl::FullyEncoded).toUtf8();

    qCDebug(dcTado()) << "Get login url request" << request.url() << payload;

    QNetworkReply *reply = m_networkManager->post(request, payload);
    connect(reply, &QNetworkReply::finished, reply, &QNetworkReply::deleteLater);
    connect(reply, &QNetworkReply::finished, this, [reply, this] {
        int status = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
        // Check HTTP status code
        if (status != 200 || reply->error() != QNetworkReply::NoError) {
            emit getLoginUrlFinished(false);

            emit connectionError(reply->error());

            if (reply->error() == QNetworkReply::HostNotFoundError)
                setConnectionStatus(false);

            if (status == 401 || status == 400)
                setAuthenticationStatus(false);

            qCWarning(dcTado()) << "Request error:" << status << reply->errorString();
            return;
        }

        m_apiAvailable = true;
        setConnectionStatus(true);

        QByteArray data = reply->readAll();
        QJsonParseError error;
        QJsonDocument responseJsonDoc = QJsonDocument::fromJson(data, &error);
        if (error.error != QJsonParseError::NoError) {
            qCDebug(dcTado()) << "Get Token: Received invalid JSON object:" << data;
            emit getLoginUrlFinished(false);
            return;
        }

        qCDebug(dcTado()) << "Get login url response" << qUtf8Printable(responseJsonDoc.toJson());
        QVariantMap responseMap = responseJsonDoc.toVariant().toMap();
        m_deviceCode = responseMap.value("device_code").toString();
        m_loginUrl = responseMap.value("verification_uri_complete").toString();
        uint pollInterval = responseMap.value("interval").toUInt();

        qCDebug(dcTado()) << "Login url:" << m_loginUrl;
        qCDebug(dcTado()) << "Device code:" << m_deviceCode;
        qCDebug(dcTado()) << "Poll interval:" << pollInterval;
        m_pollAuthenticationTimer.setInterval(pollInterval * 1000);

        emit getLoginUrlFinished(true);
    });
}

void Tado::getAccessToken()
{
    QNetworkRequest request = QNetworkRequest(QUrl(m_baseAuthorizationUrl + "/token"));
    request.setHeader(QNetworkRequest::KnownHeaders::ContentTypeHeader, "application/x-www-form-urlencoded");

    QUrlQuery query;
    query.addQueryItem("grant_type", "refresh_token");
    query.addQueryItem("refresh_token", m_refreshToken);
    query.addQueryItem("client_id", m_clientId);

    QByteArray payload = query.toString(QUrl::FullyEncoded).toUtf8();

    qCDebug(dcTado()) << "Get access token request" << request.url() << payload;

    QNetworkReply *reply = m_networkManager->post(request, payload);
    connect(reply, &QNetworkReply::finished, reply, &QNetworkReply::deleteLater);
    connect(reply, &QNetworkReply::finished, this, [reply, this] {
        QByteArray data = reply->readAll();
        int status = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
        // Check HTTP status code
        if (status != 200 || reply->error() != QNetworkReply::NoError) {
            emit connectionError(reply->error());

            if (reply->error() == QNetworkReply::HostNotFoundError)
                setConnectionStatus(false);

            if (status == 401 || status == 400)
                setAuthenticationStatus(false);

            qCWarning(dcTado()) << "Request error:" << status << reply->errorString() << qUtf8Printable(data);
            return;
        }

        m_apiAvailable = true;
        setConnectionStatus(true);

        QJsonParseError error;
        QJsonDocument responseJsonDoc = QJsonDocument::fromJson(data, &error);
        if (error.error != QJsonParseError::NoError) {
            qCDebug(dcTado()) << "Get access token received invalid JSON object:" << data;
            emit getLoginUrlFinished(false);
            return;
        }

        qCDebug(dcTado()) << "Get access token response" << qUtf8Printable(responseJsonDoc.toJson());
        QVariantMap responseMap = responseJsonDoc.toVariant().toMap();

        m_accessToken = responseMap.value("access_token").toString();
        emit accessTokenReceived();

        QString refreshToken = responseMap.value("refresh_token").toString();
        if (m_refreshToken != refreshToken) {
            m_refreshToken = refreshToken;
            emit refreshTokenReceived(m_refreshToken);
        }

        setAuthenticationStatus(true);

        // Refresh 10 sekonds before expiration
        m_refreshTimer.setInterval((responseMap.value("expires_in").toUInt() - 10) * 1000);
        m_refreshTimer.start();
    });
}

void Tado::getHomes()
{
    if (!m_apiAvailable) {
        qCWarning(dcTado()) << "Not sending request, get API credentials first";
        return;
    }

    if (m_accessToken.isEmpty()) {
        qCWarning(dcTado()) << "Not sending request, get the access token first";
        return;
    }
    QNetworkRequest request;
    request.setUrl(QUrl(m_baseControlUrl + "/me"));
    request.setHeader(QNetworkRequest::KnownHeaders::ContentTypeHeader, "application/x-www-form-urlencoded");
    request.setRawHeader("Authorization", "Bearer " + m_accessToken.toLocal8Bit());
    QNetworkReply *reply = m_networkManager->get(request);
    //qCDebug(dcTado()) << "Sending request" << request.url() << request.rawHeaderList();
    connect(reply, &QNetworkReply::finished, reply, &QNetworkReply::deleteLater);
    connect(reply, &QNetworkReply::finished, this, [reply, this] {
        int status = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();

        // Check HTTP status code
        if (status != 200 || reply->error() != QNetworkReply::NoError) {
            emit connectionError(reply->error());
            if (reply->error() == QNetworkReply::HostNotFoundError) {
                setConnectionStatus(false);
            }
            if (status == 401 || status == 400) {
                setAuthenticationStatus(false);
            }
            qCWarning(dcTado()) << "Request error:" << status << reply->errorString();
            return;
        }
        setConnectionStatus(true);
        setAuthenticationStatus(true);

        QJsonParseError error;
        QJsonDocument data = QJsonDocument::fromJson(reply->readAll(), &error);
        if (error.error != QJsonParseError::NoError) {
            qCDebug(dcTado()) << "Get Homes: Recieved invalid JSON object";
            return;
        }

        qCDebug(dcTado()) << "Get homes response" << qUtf8Printable(data.toJson());

        QVariantMap responseMap = data.toVariant().toMap();
        QString username = responseMap.value("username").toString();
        if (m_username != username) {
            m_username = username;
            emit usernameChanged(m_username);
        }

        QList<Home> homes;
        QVariantList homeList = responseMap.value("homes").toList();
        foreach (QVariant variant, homeList) {
            QVariantMap obj = variant.toMap();
            Home home;
            home.id = obj["id"].toString();
            home.name = obj["name"].toString();
            homes.append(home);
        }

        emit homesReceived(homes);
    });
}

void Tado::getZones(const QString &homeId)
{
    if (!m_apiAvailable) {
        qCWarning(dcTado()) << "Not sending request, get API credentials first";
        return;
    }

    if (m_accessToken.isEmpty()) {
        qCWarning(dcTado()) << "Not sending request, get the access token first";
        return;
    }

    QNetworkRequest request;
    request.setUrl(QUrl(m_baseControlUrl + "/homes/" + homeId + "/zones"));
    request.setHeader(QNetworkRequest::KnownHeaders::ContentTypeHeader, "application/x-www-form-urlencoded");
    request.setRawHeader("Authorization", "Bearer " + m_accessToken.toLocal8Bit());
    QNetworkReply *reply = m_networkManager->get(request);
    //qCDebug(dcTado()) << "Sending request" << request.url() << request.rawHeaderList();
    connect(reply, &QNetworkReply::finished, reply, &QNetworkReply::deleteLater);
    connect(reply, &QNetworkReply::finished, this, [reply, homeId, this] {
        int status = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();

        // Check HTTP status code
        if (status != 200 || reply->error() != QNetworkReply::NoError) {
            if (reply->error() == QNetworkReply::HostNotFoundError) {
                setConnectionStatus(false);
            }
            if (status == 401 || status == 400) {
                setAuthenticationStatus(false);
            }
            qCWarning(dcTado()) << "Request error:" << status << reply->errorString();
            return;
        }
        setConnectionStatus(true);
        setAuthenticationStatus(true);

        QJsonParseError error;
        QJsonDocument data = QJsonDocument::fromJson(reply->readAll(), &error);
        if (error.error != QJsonParseError::NoError) {
            qCDebug(dcTado()) << "Get Token: Recieved invalid JSON object";
            return;
        }
        QList<Zone> zones;
        QVariantList list = data.toVariant().toList();
        foreach (QVariant variant, list) {
            QVariantMap obj = variant.toMap();
            Zone zone;
            zone.id = obj["id"].toString();
            zone.name = obj["name"].toString();
            zone.type = obj["type"].toString();
            zones.append(zone);
        }
        emit zonesReceived(homeId, zones);
    });
}

void Tado::getZoneState(const QString &homeId, const QString &zoneId)
{
    if (!m_apiAvailable) {
        qCWarning(dcTado()) << "Not sending request, get API credentials first";
        return;
    }

    if (m_accessToken.isEmpty()) {
        qCWarning(dcTado()) << "Not sending request, get the access token first";
        return;
    }

    QNetworkRequest request;
    request.setUrl(QUrl(m_baseControlUrl + "/homes/" + homeId + "/zones/" + zoneId + "/state"));
    request.setHeader(QNetworkRequest::KnownHeaders::ContentTypeHeader, "application/x-www-form-urlencoded");
    request.setRawHeader("Authorization", "Bearer " + m_accessToken.toLocal8Bit());
    QNetworkReply *reply = m_networkManager->get(request);
    //qCDebug(dcTado()) << "Sending request" << request.url() << request.rawHeaderList();
    connect(reply, &QNetworkReply::finished, reply, &QNetworkReply::deleteLater);
    connect(reply, &QNetworkReply::finished, this, [reply, homeId, zoneId, this] {
        int status = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();

        // Check HTTP status code
        if (status != 200 || reply->error() != QNetworkReply::NoError) {
            emit connectionError(reply->error());

            if (reply->error() == QNetworkReply::HostNotFoundError) {
                setConnectionStatus(false);
            }
            if (status == 401 || status == 400) {
                setAuthenticationStatus(false);
            }
            qCWarning(dcTado()) << "Request error:" << status << reply->errorString();
            return;
        }

        setConnectionStatus(true);
        setAuthenticationStatus(true);

        QJsonParseError error;
        QJsonDocument data = QJsonDocument::fromJson(reply->readAll(), &error);
        if (error.error != QJsonParseError::NoError) {
            qCDebug(dcTado()) << "Get Token: Recieved invalid JSON object";
            return;
        }
        qCDebug(dcTado()) << "Zone status received:" << qUtf8Printable(data.toJson(QJsonDocument::Indented));
        ZoneState state;
        QVariantMap map = data.toVariant().toMap();
        state.tadoMode = map["tadoMode"].toString();
        state.windowOpenDetected = map["openWindowDetected"].toBool();

        QVariantMap settingsMap = map["setting"].toMap();
        state.settingType = settingsMap["type"].toString();
        state.settingPower = (settingsMap["power"].toString() == "ON");
        state.settingTemperature = settingsMap["temperature"].toMap().value("celsius").toDouble();
        state.connected = (map["link"].toMap().value("state").toString() == "ONLINE");

        QVariantMap activityDataMap = map["activityDataPoints"].toMap();
        state.heatingPowerPercentage = activityDataMap["heatingPower"].toMap().value("percentage").toDouble();
        state.heatingPowerType = activityDataMap["heatingPower"].toMap().value("type").toString();

        QVariantMap dataMap = map["sensorDataPoints"].toMap();
        state.temperature = dataMap["insideTemperature"].toMap().value("celsius").toDouble();
        state.humidity = dataMap["humidity"].toMap().value("percentage").toDouble();

        if (!map["overlay"].toMap().isEmpty()) {
            state.overlayIsSet = true;
            QVariantMap overlayMap = map["overlay"].toMap();
            state.overlayType = map["overlayType"].toString();
            state.overlaySettingPower = (overlayMap["setting"].toMap().value("power").toString() == "ON");
            state.overlaySettingTemperature = overlayMap["setting"].toMap().value("temperature").toDouble();
        } else {
            state.overlayIsSet = false;
        }
        emit zoneStateReceived(homeId, zoneId, state);
    });
}

QUuid Tado::setOverlay(const QString &homeId, const QString &zoneId, bool power, double targetTemperature)
{
    if (!m_apiAvailable) {
        qCWarning(dcTado()) << "Not sending request, get API credentials first";
        return QUuid();
    }

    if (m_accessToken.isEmpty()) {
        qCWarning(dcTado()) << "Not sending request, get the access token first";
        return QUuid();
    }

    QUuid requestId = QUuid::createUuid();
    QNetworkRequest request;
    request.setUrl(QUrl(m_baseControlUrl + "/homes/" + homeId + "/zones/" + zoneId + "/overlay"));
    request.setHeader(QNetworkRequest::KnownHeaders::ContentTypeHeader, "application/json;charset=utf-8");
    request.setRawHeader("Authorization", "Bearer " + m_accessToken.toLocal8Bit());

    QByteArray body;
    QByteArray powerString;
    if (power)
        powerString = "ON";
    else
        powerString = "OFF";

    body.append("{\"setting\":{\"type\":\"HEATING\",\"power\":\"" + powerString + "\",\"temperature\":{\"celsius\":" + QVariant(targetTemperature).toByteArray()
                + "}},\"termination\":{\"type\":\"MANUAL\"}}");

    //qCDebug(dcTado()) << "Sending request" << body;
    QNetworkReply *reply = m_networkManager->put(request, body);
    connect(reply, &QNetworkReply::finished, reply, &QNetworkReply::deleteLater);
    connect(reply, &QNetworkReply::finished, this, [homeId, zoneId, requestId, reply, this] {
        int status = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
        if (status != 200 || reply->error() != QNetworkReply::NoError) {
            emit requestExecuted(requestId, false);
            emit connectionError(reply->error());

            if (reply->error() == QNetworkReply::HostNotFoundError) {
                setConnectionStatus(false);
            }

            if (status == 401 || status == 400) { //Unauthorized
                setAuthenticationStatus(false);
            } else if (status == 422) { //Unprocessable Entity
                qCWarning(dcTado()) << "Unprocessable Entity, probably a value out of range";
            } else {
                qCWarning(dcTado()) << "Request error:" << status << reply->errorString();
            }
            return;
        }

        setAuthenticationStatus(true);
        setConnectionStatus(true);
        emit requestExecuted(requestId, true);

        QJsonParseError error;
        QJsonDocument data = QJsonDocument::fromJson(reply->readAll(), &error);
        if (error.error != QJsonParseError::NoError) {
            qCDebug(dcTado()) << "Get Token: Recieved invalid JSON object";
            return;
        }
        QVariantMap map = data.toVariant().toMap();

        Overlay overlay;
        QVariantMap settingsMap = map["setting"].toMap();
        overlay.zoneType = settingsMap["type"].toString();
        overlay.power = (settingsMap["power"].toString() == "ON");
        overlay.temperature = settingsMap["temperature"].toMap().value("celsius").toDouble();
        QVariantMap terminationMap = map["termination"].toMap();
        overlay.terminationType = terminationMap["type"].toString();

        overlay.tadoMode = map["type"].toString();
        emit overlayReceived(homeId, zoneId, overlay);
    });
    return requestId;
}

QUuid Tado::deleteOverlay(const QString &homeId, const QString &zoneId)
{
    if (!m_apiAvailable) {
        qCWarning(dcTado()) << "Not sending request, get API credentials first";
        return QUuid();
    }

    if (m_accessToken.isEmpty()) {
        qCWarning(dcTado()) << "Not sending request, get the access token first";
        return QUuid();
    }

    QUuid requestId = QUuid::createUuid();
    QNetworkRequest request;
    request.setUrl(QUrl(m_baseControlUrl + "/homes/" + homeId + "/zones/" + zoneId + "/overlay"));
    request.setRawHeader("Authorization", "Bearer " + m_accessToken.toLocal8Bit());
    QNetworkReply *reply = m_networkManager->deleteResource(request);
    connect(reply, &QNetworkReply::finished, reply, &QNetworkReply::deleteLater);
    connect(reply, &QNetworkReply::finished, this, [homeId, zoneId, requestId, reply, this] {
        int status = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
        if (status < 200 || status > 210 || reply->error() != QNetworkReply::NoError) {
            emit requestExecuted(requestId, false);
            emit connectionError(reply->error());

            if (reply->error() == QNetworkReply::HostNotFoundError) {
                setConnectionStatus(false);
            }

            if (status == 401 || status == 400) { //Unauthorized
                setAuthenticationStatus(false);
            } else if (status == 422) { //Unprocessable Entity
                qCWarning(dcTado()) << "Unprocessable Entity, probably a value out of range";
            } else {
                qCWarning(dcTado()) << "Request error:" << status << reply->errorString();
            }

            return;
        }

        setAuthenticationStatus(true);
        setConnectionStatus(true);
        emit requestExecuted(requestId, true);

        QJsonParseError error;
        QJsonDocument data = QJsonDocument::fromJson(reply->readAll(), &error);
        if (error.error != QJsonParseError::NoError) {
            qCDebug(dcTado()) << "Get Token: Recieved invalid JSON object";
            return;
        }

        QVariantMap map = data.toVariant().toMap();

        Overlay overlay;
        QVariantMap settingsMap = map["setting"].toMap();
        overlay.zoneType = settingsMap["type"].toString();
        overlay.power = (settingsMap["power"].toString() == "ON");
        overlay.temperature = settingsMap["temperature"].toMap().value("celsius").toDouble();
        QVariantMap terminationMap = map["termination"].toMap();
        overlay.terminationType = terminationMap["type"].toString();

        overlay.tadoMode = map["type"].toString();
        emit overlayReceived(homeId, zoneId, overlay);
    });
    return requestId;
}

void Tado::requestAuthenticationToken()
{
    QNetworkRequest request = QNetworkRequest(QUrl(m_baseAuthorizationUrl + "/token"));
    request.setHeader(QNetworkRequest::KnownHeaders::ContentTypeHeader, "application/x-www-form-urlencoded");

    QUrlQuery query;
    query.addQueryItem("client_id", m_clientId);
    query.addQueryItem("device_code", m_deviceCode);
    query.addQueryItem("grant_type", "urn:ietf:params:oauth:grant-type:device_code");

    QByteArray payload = query.toString(QUrl::FullyEncoded).toUtf8();
    qCDebug(dcTado()) << "Request authentication token" << request.url() << payload;

    QNetworkReply *reply = m_networkManager->post(request, payload);
    connect(reply, &QNetworkReply::finished, reply, &QNetworkReply::deleteLater);
    connect(reply, &QNetworkReply::finished, this, [reply, this] {
        int status = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
        if (status != 200 || reply->error() != QNetworkReply::NoError) {
            qCDebug(dcTado()) << "Request error:" << status << "Retrying:" << m_pollAuthenticationCount << "/" << m_pollAuthenticationLimit;

            if (m_pollAuthenticationCount >= m_pollAuthenticationLimit) {
                qCWarning(dcTado()) << "Authentication request failed" << m_pollAuthenticationCount << "times. Giving up.";
                emit startAuthenticationFinished(false);
                setAuthenticationStatus(false);
                return;
            }

            // We poll until the user finished the login or until we reached the limit
            m_pollAuthenticationTimer.start();
            m_pollAuthenticationCount++;
            return;
        }

        QByteArray data = reply->readAll();
        QJsonParseError error;
        QJsonDocument responseJsonDoc = QJsonDocument::fromJson(data, &error);
        if (error.error != QJsonParseError::NoError) {
            qCDebug(dcTado()) << "Authentication received invalid JSON object:" << data;
            emit startAuthenticationFinished(false);
            setAuthenticationStatus(false);
            return;
        }

        qCDebug(dcTado()) << "Authentication finished successfully:" << qUtf8Printable(responseJsonDoc.toJson());
        QVariantMap responseMap = responseJsonDoc.toVariant().toMap();

        m_accessToken = responseMap.value("access_token").toString();
        QString refreshToken = responseMap.value("refresh_token").toString();

        if (m_refreshToken != refreshToken) {
            m_refreshToken = refreshToken;
            emit refreshTokenReceived(m_refreshToken);
        }

        emit startAuthenticationFinished(true);
        setAuthenticationStatus(true);
    });
}

void Tado::setAuthenticationStatus(bool status)
{
    if (m_authenticationStatus != status) {
        m_authenticationStatus = status;
        emit authenticationStatusChanged(status);
    }

    if (!status)
        m_refreshTimer.stop();
}

void Tado::setConnectionStatus(bool status)
{
    if (m_connectionStatus != status) {
        m_connectionStatus = status;
        emit connectionChanged(status);
    }
}
