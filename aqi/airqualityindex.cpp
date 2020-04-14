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
    if (m_apiKey.isEmpty())
        qCWarning(dcAirQualityIndex()) << "API key is not set";

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
            if (status == 400) {
                qCWarning(dcAirQualityIndex()) << "Request error due to exceeded request quota";
            }
            requestExecuted(requestId, false);
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
        foreach (QVariant stationVariant, stationList) {
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

        requestExecuted(requestId, true);
    });
    return requestId;
}

QUuid AirQualityIndex::getDataByIp()
{
    if (m_apiKey.isEmpty())
        qCWarning(dcAirQualityIndex()) << "API key is not set";

    QUuid requestId = QUuid::createUuid();;
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
            if (status == 400) {
                qCWarning(dcAirQualityIndex()) << "Request error due to exceeded request quota";
            }
            requestExecuted(requestId, false);
            qCWarning(dcAirQualityIndex()) << "Request error:" << status << reply->errorString();
            return;
        }
        if (!parseData(requestId, reply->readAll()))
                requestExecuted(requestId, false);
        requestExecuted(requestId, true);
    });
    return requestId;
}

QUuid AirQualityIndex::getDataByGeolocation(const QString &lat, const QString &lng)
{
    if (m_apiKey.isEmpty())
        qCWarning(dcAirQualityIndex()) << "API key is not set";

    QUuid requestId = QUuid::createUuid();
    QUrl url;
    url.setUrl(m_baseUrl);
    url.setPath("/feed/geo:"+lat+";"+lng+"/");
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
            if (status == 400) {
                qCWarning(dcAirQualityIndex()) << "Request error due to exceeded request quota";
            }
            requestExecuted(requestId, false);
            qCWarning(dcAirQualityIndex()) << "Request error:" << status << reply->errorString();
            return;
        }
        if (!parseData(requestId, reply->readAll()))
                requestExecuted(requestId, false);
        requestExecuted(requestId, true);
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
    Station station;
    station.aqi =  doc.toVariant().toMap().value("data").toMap().value("aqi").toInt();
    station.idx =  doc.toVariant().toMap().value("data").toMap().value("idx").toInt();

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
