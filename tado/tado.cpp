/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*
* Copyright 2013 - 2020, nymea GmbH
* Contact: contact@nymea.io
*
* This file is part of nymea.
* This project including source code and documentation is protected by copyright law, and
* remains the property of nymea GmbH. All rights, including reproduction, publication,
* editing and translation, are reserved. The use of this project is subject to the terms of a
* license agreement to be concluded with nymea GmbH in accordance with the terms
* of use of nymea GmbH, available under https://nymea.io/license
*
* GNU Lesser General Public License Usage
* This project may also contain libraries licensed under the open source software license GNU GPL v.3.
* Alternatively, this project may be redistributed and/or modified under the terms of the GNU
* Lesser General Public License as published by the Free Software Foundation; version 3.
* this project is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;
* without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
* See the GNU Lesser General Public License for more details.
*
* You should have received a copy of the GNU Lesser General Public License along with this project.
* If not, see <https://www.gnu.org/licenses/>.
*
* For any further details and any questions please contact us under contact@nymea.io
* or see our FAQ/Licensing Information on https://nymea.io/license/faq
*
* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

#include "tado.h"
#include "extern-plugininfo.h"

#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QUrlQuery>

Tado::Tado(NetworkAccessManager *networkManager, const QString &username, QObject *parent) :
    QObject(parent),
    m_networkManager(networkManager),
    m_username(username)
{
    m_refreshTimer = new QTimer(this);
    m_refreshTimer->setSingleShot(true);
    connect(m_refreshTimer, &QTimer::timeout, this, &Tado::onRefreshTimer);
}

void Tado::setUsername(const QString &username)
{
    m_username = username;
}

QString Tado::username()
{
    return m_username;
}

void Tado::getToken(const QString &password)
{
    QNetworkRequest request;
    request.setUrl(QUrl(m_baseAuthorizationUrl));
    request.setHeader(QNetworkRequest::KnownHeaders::ContentTypeHeader, "application/x-www-form-urlencoded");
    QByteArray body;
    body.append("client_id=" + m_clientId);
    body.append("&client_secret=" + m_clientSecret);
    body.append("&grant_type=password");
    body.append("&scope=home.user");
    body.append("&username=" + m_username);
    body.append("&password=" + password);

    QNetworkReply *reply = m_networkManager->post(request, body);
    //qCDebug(dcTado()) << "Sending request" << request.url() << body;
    connect(reply, &QNetworkReply::finished, this, [reply, this] {
        reply->deleteLater();
        int status = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();

        // Check HTTP status code
        if (status != 200 || reply->error() != QNetworkReply::NoError) {
            if (reply->error() == QNetworkReply::HostNotFoundError) {
                emit connectionChanged(false);
            }
            if (status == 401) {
                emit authenticationStatusChanged(false);
            }
            qCWarning(dcTado()) << "Request error:" << status << reply->errorString();
            return;
        }
        emit connectionChanged(true);

        QJsonParseError error;
        QJsonDocument data = QJsonDocument::fromJson(reply->readAll(), &error);
        if (error.error != QJsonParseError::NoError) {
            qDebug(dcTado()) << "Get Token: Received invalide JSON object";
            return;
        }
        if (data.isObject()) {
            Token token;
            QVariantMap obj = data.toVariant().toMap();
            if (obj.contains("access_token")) {
                token.accesToken = obj["access_token"].toString();
                m_accessToken = token.accesToken;
            } else {
                qCWarning(dcTado()) << "Received response doesnt contain an access token";
            }

            token.tokenType = obj["token_type"].toString();
            token.refreshToken = obj["refresh_token"].toString();
            m_refreshToken = token.refreshToken;
            if (obj.contains("expires_in")) {
                token.expires = obj["expires_in"].toInt();
                m_refreshTimer->start((token.expires - 10)*1000);
            } else {
                  qCWarning(dcTado()) << "Received response doesn't contain an expire time";
            }
            token.scope = obj["scope"].toString();
            token.jti = obj["jti"].toString();
            emit authenticationStatusChanged(true);
            emit tokenReceived(token);
        } else {
            qCWarning(dcTado()) << "Received response isn't an object" << data.toJson();
            emit authenticationStatusChanged(false);
        }
    });
}

void Tado::getHomes()
{
    QNetworkRequest request;
    request.setUrl(QUrl(m_baseControlUrl + "/me"));
    request.setHeader(QNetworkRequest::KnownHeaders::ContentTypeHeader, "application/x-www-form-urlencoded");
    request.setRawHeader("Authorization", "Bearer " + m_accessToken.toLocal8Bit());
    QNetworkReply *reply = m_networkManager->get(request);
    //qDebug(dcTado()) << "Sending request" << request.url() << request.rawHeaderList();
    connect(reply, &QNetworkReply::finished, this, [reply, this] {
        reply->deleteLater();
        int status = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();

        // Check HTTP status code
        if (status != 200 || reply->error() != QNetworkReply::NoError) {
            if (reply->error() == QNetworkReply::HostNotFoundError) {
                emit connectionChanged(false);
            }
            if (status == 401) {
                emit authenticationStatusChanged(false);
            }
            qCWarning(dcTado()) << "Request error:" << status << reply->errorString();
            return;
        }
        emit connectionChanged(true);
        emit authenticationStatusChanged(true);

        QJsonParseError error;
        QJsonDocument data = QJsonDocument::fromJson(reply->readAll(), &error);
        if (error.error != QJsonParseError::NoError) {
            qDebug(dcTado()) << "Get Token: Recieved invalide JSON object";
            return;
        }
        QList<Home> homes;
        QVariantList homeList = data.toVariant().toMap().value("homes").toList();
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
    QNetworkRequest request;
    request.setUrl(QUrl(m_baseControlUrl+"/homes/"+homeId+"/zones"));
    request.setHeader(QNetworkRequest::KnownHeaders::ContentTypeHeader, "application/x-www-form-urlencoded");
    request.setRawHeader("Authorization", "Bearer " + m_accessToken.toLocal8Bit());
    QNetworkReply *reply = m_networkManager->get(request);
    //qDebug(dcTado()) << "Sending request" << request.url() << request.rawHeaderList();
    connect(reply, &QNetworkReply::finished, this, [reply, homeId, this] {
        reply->deleteLater();
        int status = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();

        // Check HTTP status code
        if (status != 200 || reply->error() != QNetworkReply::NoError) {
            if (reply->error() == QNetworkReply::HostNotFoundError) {
                emit connectionChanged(false);
            }
            if (status == 401) {
                emit authenticationStatusChanged(false);
            }
            qCWarning(dcTado()) << "Request error:" << status << reply->errorString();
            return;
        }
        emit connectionChanged(true);
        emit authenticationStatusChanged(true);

        QJsonParseError error;
        QJsonDocument data = QJsonDocument::fromJson(reply->readAll(), &error);
        if (error.error != QJsonParseError::NoError) {
            qDebug(dcTado()) << "Get Token: Recieved invalide JSON object";
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
    QNetworkRequest request;
    request.setUrl(QUrl(m_baseControlUrl+"/homes/"+homeId+"/zones/"+zoneId+"/state"));
    request.setHeader(QNetworkRequest::KnownHeaders::ContentTypeHeader, "application/x-www-form-urlencoded");
    request.setRawHeader("Authorization", "Bearer " + m_accessToken.toLocal8Bit());
    QNetworkReply *reply = m_networkManager->get(request);
    //qDebug(dcTado()) << "Sending request" << request.url() << request.rawHeaderList();
    connect(reply, &QNetworkReply::finished, this, [reply, homeId, zoneId, this] {
        reply->deleteLater();
        int status = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();

        // Check HTTP status code
        if (status != 200 || reply->error() != QNetworkReply::NoError) {
            if (reply->error() == QNetworkReply::HostNotFoundError) {
                emit connectionChanged(false);
            }
            if (status == 401) {
                emit authenticationStatusChanged(false);
            }
            qCWarning(dcTado()) << "Request error:" << status << reply->errorString();
            return;
        }

        emit connectionChanged(true);
        emit authenticationStatusChanged(true);

        QJsonParseError error;
        QJsonDocument data = QJsonDocument::fromJson(reply->readAll(), &error);
        if (error.error != QJsonParseError::NoError) {
            qDebug(dcTado()) << "Get Token: Recieved invalide JSON object";
            return;
        }
        ZoneState state;
        QVariantMap map = data.toVariant().toMap();
        state.tadoMode = map["tadoMode"].toString();
        state.windowOpen = map["openWindow"].toBool();

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

        if (!map["overlay"].toMap().isEmpty()){
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
    QUuid requestId = QUuid::createUuid();
    QNetworkRequest request;
    request.setUrl(QUrl(m_baseControlUrl+"/homes/"+homeId+"/zones/"+zoneId+"/overlay"));
    request.setHeader(QNetworkRequest::KnownHeaders::ContentTypeHeader, "application/json;charset=utf-8");
    request.setRawHeader("Authorization", "Bearer " + m_accessToken.toLocal8Bit());

    QByteArray body;
    QByteArray powerString;
    if (power)
       powerString = "ON";
    else
       powerString = "OFF";

    body.append("{\"setting\":{\"type\":\"HEATING\",\"power\":\""+ powerString + "\",\"temperature\":{\"celsius\":" + QVariant(targetTemperature).toByteArray() + "}},\"termination\":{\"type\":\"MANUAL\"}}");
    //qCDebug(dcTado()) << "Sending request" << body;
    QNetworkReply *reply = m_networkManager->put(request, body);
    connect(reply, &QNetworkReply::finished, this, [homeId, zoneId, requestId, reply, this] {
        reply->deleteLater();

        int status = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
        // Check HTTP status code
        if (status != 200 || reply->error() != QNetworkReply::NoError) {
            emit requestExecuted(requestId, false);
            if (reply->error() == QNetworkReply::HostNotFoundError) {
                emit connectionChanged(false);
            }
            if (status == 401) { //Unauthorized
                emit authenticationStatusChanged(false);
            } else if (status == 422) { //Unprocessable Entity
                qCWarning(dcTado()) << "Unprocessable Entity, probably a value out of range";
            } else {
                qCWarning(dcTado()) << "Request error:" << status << reply->errorString();
            }
            return;
        }
        emit authenticationStatusChanged(true);
        emit connectionChanged(true);
        emit requestExecuted(requestId, true);

        QJsonParseError error;
        QJsonDocument data = QJsonDocument::fromJson(reply->readAll(), &error);
        if (error.error != QJsonParseError::NoError) {
            qDebug(dcTado()) << "Get Token: Recieved invalide JSON object";
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
    QUuid requestId = QUuid::createUuid();
    QNetworkRequest request;
    request.setUrl(QUrl(m_baseControlUrl+"/homes/"+homeId+"/zones/"+zoneId+"/overlay"));
    request.setRawHeader("Authorization", "Bearer " + m_accessToken.toLocal8Bit());
    QNetworkReply *reply = m_networkManager->deleteResource(request);
    connect(reply, &QNetworkReply::finished, this, [homeId, zoneId, requestId, reply, this] {
        reply->deleteLater();
        int status = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();

        // Check HTTP status code
        if (status < 200 || status > 210 || reply->error() != QNetworkReply::NoError) {
            emit requestExecuted(requestId ,false);
            if (reply->error() == QNetworkReply::HostNotFoundError) {
                emit connectionChanged(false);
            }
            if (status == 401) { //Unauthorized
                emit authenticationStatusChanged(false);
            } else if (status == 422) { //Unprocessable Entity
                qCWarning(dcTado()) << "Unprocessable Entity, probably a value out of range";
            } else {
                qCWarning(dcTado()) << "Request error:" << status << reply->errorString();
            }
            qCWarning(dcTado()) << "Request error:" << status << reply->errorString();
            return;
        }
        emit authenticationStatusChanged(true);
        emit connectionChanged(true);
        emit requestExecuted(requestId, true);

        QJsonParseError error;
        QJsonDocument data = QJsonDocument::fromJson(reply->readAll(), &error);
        if (error.error != QJsonParseError::NoError) {
            qDebug(dcTado()) << "Get Token: Recieved invalide JSON object";
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

void Tado::onRefreshTimer()
{
    QNetworkRequest request;
    request.setUrl(QUrl(m_baseAuthorizationUrl));
    request.setHeader(QNetworkRequest::KnownHeaders::ContentTypeHeader, "application/x-www-form-urlencoded");
    QByteArray body;
    body.append("client_id=" + m_clientId);
    body.append("&client_secret=" + m_clientSecret);
    body.append("&grant_type=refresh_token");
    body.append("&refresh_token=" + m_refreshToken);
    body.append("&scope=home.user");

    QNetworkReply *reply = m_networkManager->post(request, body);
    connect(reply, &QNetworkReply::finished, this, [reply, this] {
        reply->deleteLater();
        int status = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();

        // Check HTTP status code
        if (status != 200 || reply->error() != QNetworkReply::NoError) {
            if (reply->error() == QNetworkReply::HostNotFoundError) {
                emit connectionChanged(false);
            }
            if (status == 400 || status == 401) {
                emit authenticationStatusChanged(false);
            }
            qCWarning(dcTado()) << "Request error:" << status << reply->errorString();
            return;
        }
        emit connectionChanged(true);
        emit authenticationStatusChanged(true);

        QJsonParseError error;
        QJsonDocument data = QJsonDocument::fromJson(reply->readAll(), &error);
        if (error.error != QJsonParseError::NoError) {
            qDebug(dcTado()) << "Get Token: Recieved invalide JSON object";
            return;
        }
        Token token;
        QVariantMap obj = data.toVariant().toMap();
        token.accesToken = obj["access_token"].toString();
        m_accessToken = token.accesToken;
        token.tokenType = obj["token_type"].toString();
        token.refreshToken = obj["refresh_token"].toString();
        m_refreshToken = token.refreshToken;
        token.expires = obj["expires_in"].toInt();
        m_refreshTimer->start((token.expires - 10)*1000);
        token.scope = obj["scope"].toString();
        token.jti = obj["jti"].toString();
        emit tokenReceived(token);
    });
}
