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

#include "airqualityindex.h"
#include "extern-plugininfo.h"

#include <QNetworkReply>
#include <QUrlQuery>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>

AirQualityIndex::AirQualityIndex(NetworkAccessManager *networkAccessManager, const QString &apiKey, QObject *parent) :
    QObject(parent),
    m_networkAccessManager(networkAccessManager),
    m_apiKey(apiKey)
{

}

void AirQualityIndex::setApiKey(const QString &apiKey)
{
    m_apiKey = apiKey;
}

QUuid AirQualityIndex::searchByName(const QString &name)
{
    if (m_apiKey.isEmpty()) {
        qCWarning(dcAirQualityIndex()) << "API key is not set, not sending request";
        return QUuid();
    }

    QUuid requestId = QUuid::createUuid();;
    QUrl url;
    url.setUrl(m_baseUrl);
    url.setPath("/search/");
    QUrlQuery query;
    query.addQueryItem("token", m_apiKey);
    query.addQueryItem("keyword", name);
    url.setQuery(query);
    QNetworkRequest request;
    request.setUrl(url);
    request.setRawHeader("User-Agent", "nymea");

    QNetworkReply *reply = m_networkAccessManager->get(request);
    connect(reply, &QNetworkReply::finished, this, [requestId, reply, this] {
        reply->deleteLater();
        int status = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();

        // Check HTTP status code
        if (status != 200 || reply->error() != QNetworkReply::NoError) {
            if (status == 400)
                qCWarning(dcAirQualityIndex()) << "Request error due to exceeded request quota";

            emit requestExecuted(requestId, false);
            qCWarning(dcAirQualityIndex()) << "Request error:" << status << reply->errorString();
            return;
        }

        QByteArray rawData = reply->readAll();
        qCDebug(dcAirQualityIndex()) << "Search response" << rawData;

        QJsonParseError error;
        QJsonDocument doc = QJsonDocument::fromJson(rawData, &error);
        if (error.error != QJsonParseError::NoError) {
            emit requestExecuted(requestId, false);
            qCWarning(dcAirQualityIndex()) << "Received invalide JSON object";
            return;
        }

        QList<Station> stations;
        QVariantList stationList = doc.toVariant().toMap().value("data").toList();
        foreach (const QVariant &stationVariant, stationList) {
            Station station;
            station.aqi = stationVariant.toMap().value("aqi").toInt();
            station.idx = stationVariant.toMap().value("idx").toInt();
            station.measurementTime = QTime::fromString(stationVariant.toMap().value("time").toMap().value("s").toString());
            station.timezone = stationVariant.toMap().value("time").toMap().value("tz").toString();
            station.name = stationVariant.toMap().value("city").toMap().value("name").toString();
            station.url = QUrl(stationVariant.toMap().value("city").toMap().value("url").toString());
            station.location.latitude = stationVariant.toMap().value("city").toMap().value("geo").toList().first().toDouble();
            station.location.longitude = stationVariant.toMap().value("city").toMap().value("geo").toList().last().toDouble();
            stations.append(station);
        }

        if (!stations.isEmpty())
            emit stationsReceived(requestId, stations);

        emit requestExecuted(requestId, true);
    });
    return requestId;
}

QUuid AirQualityIndex::getDataByIp()
{
    if (m_apiKey.isEmpty()) {
        qCWarning(dcAirQualityIndex()) << "API key is not set, not sending request";
        return QUuid();
    }

    QUuid requestId = QUuid::createUuid();

    QUrl url;
    url.setUrl(m_baseUrl);
    url.setPath("/feed/here/");

    QUrlQuery query;
    query.addQueryItem("token", m_apiKey);
    url.setQuery(query);

    QNetworkRequest request;
    request.setUrl(url);
    request.setRawHeader("User-Agent", "nymea");

    qCDebug(dcAirQualityIndex()) << "Get data by IP request" << url.toString();
    QNetworkReply *reply = m_networkAccessManager->get(request);
    connect(reply, &QNetworkReply::finished, this, [requestId, reply, this] {
        reply->deleteLater();
        int status = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();

        // Check HTTP status code
        if (status != 200 || reply->error() != QNetworkReply::NoError) {
            if (status == 400)
                qCWarning(dcAirQualityIndex()) << "Request error due to exceeded request quota";

            emit requestExecuted(requestId, false);
            qCWarning(dcAirQualityIndex()) << "Request error:" << status << reply->errorString();
            return;
        }

        if (!parseData(requestId, reply->readAll()))
            emit requestExecuted(requestId, false);

        emit requestExecuted(requestId, true);
    });

    return requestId;
}

QUuid AirQualityIndex::getDataByGeolocation(double lat, double lng)
{
    if (m_apiKey.isEmpty()) {
        qCWarning(dcAirQualityIndex()) << "API key is not set, not sending request";
        return QUuid();
    }

    QUuid requestId = QUuid::createUuid();

    QUrl url;
    url.setUrl(m_baseUrl);
    url.setPath(QString("/feed/geo:%1;%2/").arg(lat).arg(lng));

    QUrlQuery query;
    query.addQueryItem("token", m_apiKey);
    url.setQuery(query);

    QNetworkRequest request;
    request.setUrl(url);
    request.setRawHeader("User-Agent", "nymea");

    qCDebug(dcAirQualityIndex()) << "Get data by geo location request" << url.toString();
    QNetworkReply *reply = m_networkAccessManager->get(request);
    connect(reply, &QNetworkReply::finished, this, [requestId, reply, this] {
        reply->deleteLater();
        int status = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();

        // Check HTTP status code
        if (status != 200 || reply->error() != QNetworkReply::NoError) {
            if (status == 400)
                qCWarning(dcAirQualityIndex()) << "Request error due to exceeded request quota";

            emit requestExecuted(requestId, false);
            qCWarning(dcAirQualityIndex()) << "Request error:" << status << reply->errorString();
            return;
        }
        if (!parseData(requestId, reply->readAll()))
            emit requestExecuted(requestId, false);

        emit requestExecuted(requestId, true);
    });

    return requestId;
}

bool AirQualityIndex::parseData(QUuid requestId, const QByteArray &data)
{
    qCDebug(dcAirQualityIndex()) << "Parsing data" << data;
    QJsonParseError error;
    QJsonDocument doc = QJsonDocument::fromJson(data, &error);
    if (error.error != QJsonParseError::NoError) {
        qCWarning(dcAirQualityIndex()) << "Received invalide JSON object";
        return false;
    }

    if (doc.toVariant().toMap().contains("status")) {
        if (doc.toVariant().toMap().value("status") == "error") {
            qCWarning(dcAirQualityIndex()) << "Server responded with error:" << doc.toVariant().toMap().value("data").toString();
            return false;
        }
    }

    Station station;
    station.aqi = doc.toVariant().toMap().value("data").toMap().value("aqi").toInt();
    station.idx = doc.toVariant().toMap().value("data").toMap().value("idx").toInt();

    QVariantMap city = doc.toVariant().toMap().value("data").toMap().value("city").toMap();
    if (city["geo"].toList().length() == 2) {
        station.location.latitude = city["geo"].toList().first().toDouble();
        station.location.longitude = city["geo"].toList().last().toDouble();
    } else {
        qCWarning(dcAirQualityIndex()) << "Parse data: geo location data list error" << city["geo"];
    }
    station.name = city["name"].toString();
    station.url = city["url"].toString();

    QVariantMap time = doc.toVariant().toMap().value("data").toMap().value("time").toMap();
    station.timezone = time["tz"].toString();
    station.measurementTime = QTime::fromString(time["s"].toString());
    emit stationsReceived(requestId, QList<Station>() << station);

    QVariantMap iaqi = doc.toVariant().toMap().value("data").toMap().value("iaqi").toMap();
    AirQualityData aqiData;
    aqiData.humidity = iaqi["h"].toMap().value("v").toDouble();
    aqiData.pressure = iaqi["p"].toMap().value("v").toDouble();
    aqiData.pm25 = iaqi["pm25"].toMap().value("v").toInt();
    aqiData.pm10 = iaqi["pm10"].toMap().value("v").toInt();
    aqiData.so2 = iaqi["so2"].toMap().value("v").toDouble();
    aqiData.no2 = iaqi["no2"].toMap().value("v").toDouble();
    aqiData.o3 = iaqi["o3"].toMap().value("v").toDouble();
    aqiData.co = iaqi["co"].toMap().value("v").toDouble();
    aqiData.temperature = iaqi["t"].toMap().value("v").toDouble();
    aqiData.windSpeed = iaqi["w"].toMap().value("v").toDouble();

    emit dataReceived(requestId, aqiData);
    return true;
}
