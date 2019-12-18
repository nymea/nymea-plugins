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
    qCDebug(dcTado()) << "Sending request" << request.url() << body;
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

void Tado::getHomes()
{
    QNetworkRequest request;
    request.setUrl(QUrl(m_baseControlUrl + "/me"));
    request.setHeader(QNetworkRequest::KnownHeaders::ContentTypeHeader, "application/x-www-form-urlencoded");
    request.setRawHeader("Authorization", "Bearer " + m_accessToken.toLocal8Bit());
    QNetworkReply *reply = m_networkManager->get(request);
    qDebug(dcTado()) << "Sending request" << request.url() << request.rawHeaderList();
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
    qDebug(dcTado()) << "Sending request" << request.url() << request.rawHeaderList();
    connect(reply, &QNetworkReply::finished, this, [reply, homeId, this] {
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
    qDebug(dcTado()) << "Sending request" << request.url() << request.rawHeaderList();
    connect(reply, &QNetworkReply::finished, this, [reply, homeId, zoneId, this] {
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
            state.overlaySettingPower = (overlayMap["settings"].toMap().value("power").toString() == "ON");
            state.overlaySettingTemperature = overlayMap["settings"].toMap().value("temperature").toDouble();
        } else {
            state.overlayIsSet = false;
        }
        emit zoneStateReceived(homeId, zoneId, state);
    });
}

void Tado::setOverlay(const QString &homeId, const QString &zoneId, const QString &mode, double targetTemperature)
{
    Q_UNUSED(mode);
    Q_UNUSED(targetTemperature);

    QNetworkRequest request;
    request.setUrl(QUrl(m_baseControlUrl+"/homes/"+homeId+"/zones/"+zoneId+"/overlay"));
    request.setHeader(QNetworkRequest::KnownHeaders::ContentTypeHeader, "application/json;charset=utf-8");
    request.setRawHeader("Authorization", "Bearer " + m_accessToken.toLocal8Bit());
    QJsonDocument doc;
    QJsonObject obj;
    QJsonObject setting;
    setting.insert("type", "HEATING");
    setting.insert("power", "ON");
    QJsonObject temperature;
    temperature.insert("celsius", targetTemperature);
    temperature.insert("fahrenheit", (targetTemperature * (9.0/5.0)) + 32.0);
    setting.insert("temperature", temperature);
    obj.insert("setting", setting);
    QJsonObject termination;
    termination.insert("type", "MANUAL");
    obj.insert("termination", termination);
    doc.setObject(obj);

    QNetworkReply *reply = m_networkManager->put(request, doc.toJson());
    qCDebug(dcTado()) << "Sending request" << request.url() << doc.toJson();
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

        QJsonParseError error;
        QJsonDocument data = QJsonDocument::fromJson(reply->readAll(), &error);
        if (error.error != QJsonParseError::NoError) {
            qDebug(dcTado()) << "Get Token: Recieved invalide JSON object";
            return;
        }
        QVariantMap map = data.toVariant().toMap();
        qCDebug(dcTado()) << map["type"].toString();
    });
}

void Tado::deleteOverlay(const QString &homeId, const QString &zoneId)
{
    QNetworkRequest request;
    request.setUrl(QUrl(m_baseControlUrl+"/homes/"+homeId+"/zones/"+zoneId+"/overlay"));
    request.setRawHeader("Authorization", "Bearer " + m_accessToken.toLocal8Bit());
    QNetworkReply *reply = m_networkManager->deleteResource(request);
    connect(reply, &QNetworkReply::finished, this, [reply, this] {
        reply->deleteLater();
        int status = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();

        // Check HTTP status code
        if (status < 200 || status > 210 || reply->error() != QNetworkReply::NoError) {
            if (reply->error() == QNetworkReply::HostNotFoundError) {
                emit connectionChanged(false);
            }
            if (status == 400 || status == 401) {
                emit authenticationStatusChanged(false);
            }
            qCWarning(dcTado()) << "Request error:" << status << reply->errorString();
            return;
        }
    });
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
