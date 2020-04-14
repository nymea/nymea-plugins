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

#include "integrationpluginopenweathermap.h"
#include "integrations/thing.h"
#include "plugininfo.h"

#include <QDebug>
#include <QJsonDocument>
#include <QVariantMap>
#include <QUrl>
#include <QUrlQuery>
#include <QDateTime>
#include <QTimeZone>
#include <QSettings>
#include "nymeasettings.h"

IntegrationPluginOpenweathermap::IntegrationPluginOpenweathermap()
{
    // max 60 calls/minute
    // max 50000 calls/day
    m_apiKey = "c1b9d5584bb740804871583f6c62744f";

    QSettings settings(NymeaSettings::settingsPath() + "/nymead.conf", QSettings::IniFormat);
    settings.beginGroup("OpenWeatherMap");
    if (settings.contains("apiKey")) {
        m_apiKey = settings.value("apiKey").toString();
        QString printedCopy = m_apiKey;
        qCDebug(dcOpenWeatherMap()) << "Using custom API key:" << printedCopy.replace(printedCopy.length() - 10, 10, "**********");
    }

    settings.endGroup();
}

IntegrationPluginOpenweathermap::~IntegrationPluginOpenweathermap()
{
    hardwareManager()->pluginTimerManager()->unregisterTimer(m_pluginTimer);
}

void IntegrationPluginOpenweathermap::init()
{
    m_pluginTimer = hardwareManager()->pluginTimerManager()->registerTimer(900);
    connect(m_pluginTimer, &PluginTimer::timeout, this, &IntegrationPluginOpenweathermap::onPluginTimer);
}

void IntegrationPluginOpenweathermap::discoverThings(ThingDiscoveryInfo *info)
{
    QString location = info->params().paramValue(openweathermapDiscoveryLocationParamTypeId).toString();

    // if we have an empty search string, perform an autodetection of the location with the WAN ip...
    if (location.isEmpty()){
        searchAutodetect(info);
    } else {
        search(location, info);
    }
}

void IntegrationPluginOpenweathermap::setupThing(ThingSetupInfo *info)
{
    update(info->thing());
    info->finish(Thing::ThingErrorNoError);
}

void IntegrationPluginOpenweathermap::executeAction(ThingActionInfo *info)
{
    update(info->thing());
    info->finish(Thing::ThingErrorNoError);
}

void IntegrationPluginOpenweathermap::thingRemoved(Thing *thing)
{
    // check if a thing gets removed while we still have a reply!
    foreach (Thing *d, m_weatherReplies.values()) {
        if (d->id() == thing->id()) {
            QNetworkReply *reply = m_weatherReplies.key(thing);
            m_weatherReplies.take(reply);
            reply->deleteLater();
        }
    }
}

void IntegrationPluginOpenweathermap::networkManagerReplyReady()
{
    QNetworkReply *reply = static_cast<QNetworkReply *>(sender());

    if (reply->error()) {
        qCWarning(dcOpenWeatherMap) << "OpenWeatherMap reply error: " << reply->errorString();
        reply->deleteLater();
        return;
    }

    if (m_weatherReplies.contains(reply)) {
        QByteArray data = reply->readAll();
        Thing* thing = m_weatherReplies.take(reply);
        processWeatherData(data, thing);
    }
    reply->deleteLater();
}

void IntegrationPluginOpenweathermap::update(Thing *thing)
{
    qCDebug(dcOpenWeatherMap()) << "Refresh data for" << thing->name();
    QUrl url("http://api.openweathermap.org/data/2.5/weather");
    QUrlQuery query;
    query.addQueryItem("id", thing->paramValue(openweathermapThingIdParamTypeId).toString());
    query.addQueryItem("mode", "json");
    query.addQueryItem("units", "metric");
    query.addQueryItem("appid", m_apiKey);
    url.setQuery(query);

    QNetworkReply *reply = hardwareManager()->networkManager()->get(QNetworkRequest(url));
    connect(reply, &QNetworkReply::finished, this, &IntegrationPluginOpenweathermap::networkManagerReplyReady);
    m_weatherReplies.insert(reply, thing);
}

void IntegrationPluginOpenweathermap::searchAutodetect(ThingDiscoveryInfo *info)
{
    QNetworkReply *reply = hardwareManager()->networkManager()->get(QNetworkRequest(QUrl("http://ip-api.com/json")));
    connect(reply, &QNetworkReply::finished, reply, &QNetworkReply::deleteLater);
    connect(reply, &QNetworkReply::finished, info, [this, info, reply](){
        if (reply->error()) {
            qCWarning(dcOpenWeatherMap) << "OpenWeatherMap reply error: " << reply->errorString();
            info->finish(Thing::ThingErrorHardwareFailure, QT_TR_NOOP("Error detecting current location."));
            return;
        }
        QByteArray data = reply->readAll();
        QJsonParseError error;
        QJsonDocument jsonDoc = QJsonDocument::fromJson(data, &error);

        if(error.error != QJsonParseError::NoError) {
            qCWarning(dcOpenWeatherMap) << "failed to parse data" << data << ":" << error.errorString();
            info->finish(Thing::ThingErrorHardwareFailure, QT_TR_NOOP("Received unexpected data detecting current location."));
            return;
        }

        // search by geographic coordinates
        QVariantMap dataMap = jsonDoc.toVariant().toMap();
        QString country = dataMap.value("countryCode").toString();
        QString cityName = dataMap.value("city").toString();
        QHostAddress wanIp = QHostAddress(dataMap.value("query").toString());
        double longitude = dataMap.value("lon").toDouble();
        double latitude = dataMap.value("lat").toDouble();

        qCDebug(dcOpenWeatherMap) << "----------------------------------------";
        qCDebug(dcOpenWeatherMap) << "Autodetection of location: ";
        qCDebug(dcOpenWeatherMap) << "----------------------------------------";
        qCDebug(dcOpenWeatherMap) << "       name:" << cityName;
        qCDebug(dcOpenWeatherMap) << "    country:" << country;
        qCDebug(dcOpenWeatherMap) << "     WAN IP:" << wanIp.toString();
        qCDebug(dcOpenWeatherMap) << "   latitude:" << latitude;
        qCDebug(dcOpenWeatherMap) << "  longitude:" << longitude;
        qCDebug(dcOpenWeatherMap) << "----------------------------------------";
        searchGeoLocation(latitude, longitude, country, info);

    });
}

void IntegrationPluginOpenweathermap::search(QString searchString, ThingDiscoveryInfo *info)
{
    QUrl url("http://api.openweathermap.org/data/2.5/find");
    QUrlQuery query;
    query.addQueryItem("q", searchString);
    query.addQueryItem("type", "like");
    query.addQueryItem("mode", "json");
    query.addQueryItem("units", "metric");
    query.addQueryItem("appid", m_apiKey);
    url.setQuery(query);

    QNetworkReply *reply = hardwareManager()->networkManager()->get(QNetworkRequest(url));
    connect(reply, &QNetworkReply::finished, reply, &QNetworkReply::deleteLater);

    connect(reply, &QNetworkReply::finished, info, [this, info, reply](){
        if (reply->error()) {
            qCWarning(dcOpenWeatherMap) << "OpenWeatherMap reply error: " << reply->errorString();
            info->finish(Thing::ThingErrorHardwareFailure, QT_TR_NOOP("Error searching for weather stations."));
            return;
        }
        QByteArray data = reply->readAll();
        QJsonParseError error;
        QJsonDocument jsonDoc = QJsonDocument::fromJson(data, &error);

        if(error.error != QJsonParseError::NoError) {
            qCWarning(dcOpenWeatherMap) << "failed to parse data" << data << ":" << error.errorString();
            info->finish(Thing::ThingErrorHardwareFailure, QT_TR_NOOP("Received unexpected data while searching for weather stations."));
            return;
        }

        QVariantMap dataMap = jsonDoc.toVariant().toMap();
        QList<QVariantMap> cityList;
        if (dataMap.contains("list")) {
            QVariantList list = dataMap.value("list").toList();
            foreach (QVariant key, list) {
                QVariantMap elemant = key.toMap();
                QVariantMap city;
                city.insert("name",elemant.value("name").toString());
                city.insert("country", elemant.value("sys").toMap().value("country").toString());
                city.insert("id",elemant.value("id").toString());
                cityList.append(city);
            }
        }
        processSearchResults(cityList, info);

    });
}

void IntegrationPluginOpenweathermap::searchGeoLocation(double lat, double lon, const QString &country, ThingDiscoveryInfo *info)
{
    QUrl url("http://api.openweathermap.org/data/2.5/find");
    QUrlQuery query;
    query.addQueryItem("lat", QString::number(lat));
    query.addQueryItem("lon", QString::number(lon));
    query.addQueryItem("cnt", QString::number(3)); // 3 km radius
    query.addQueryItem("type", "like");
    query.addQueryItem("mode", "json");
    query.addQueryItem("units", "metric");
    query.addQueryItem("appid", m_apiKey);
    url.setQuery(query);

    QNetworkReply *reply = hardwareManager()->networkManager()->get(QNetworkRequest(url));
    connect(reply, &QNetworkReply::finished, reply, &QNetworkReply::deleteLater);
    connect(reply, &QNetworkReply::finished, info, [this, info, reply, country](){
        if (reply->error()) {
            qCWarning(dcOpenWeatherMap) << "OpenWeatherMap reply error: " << reply->errorString();
            info->finish(Thing::ThingErrorHardwareFailure, QT_TR_NOOP("Error searching for weather stations in current location."));
            return;
        }
        QByteArray data = reply->readAll();
        QJsonParseError error;
        QJsonDocument jsonDoc = QJsonDocument::fromJson(data, &error);

        if(error.error != QJsonParseError::NoError) {
            qCWarning(dcOpenWeatherMap) << "failed to parse data" << data << ":" << error.errorString();
            info->finish(Thing::ThingErrorHardwareFailure, QT_TR_NOOP("Received unexpected data while searching for weather stations."));
            return;
        }

        QVariantMap dataMap = jsonDoc.toVariant().toMap();
        QList<QVariantMap> cityList;
        if (dataMap.contains("list")) {
            QVariantList list = dataMap.value("list").toList();
            foreach (QVariant key, list) {
                QVariantMap elemant = key.toMap();
                QVariantMap city;
                city.insert("name",elemant.value("name").toString());
                if(elemant.value("sys").toMap().value("country").toString().isEmpty()){
                    city.insert("country",country);
                }else{
                    city.insert("country", elemant.value("sys").toMap().value("country").toString());
                }
                city.insert("id",elemant.value("id").toString());
                cityList.append(city);
            }
        }
        processSearchResults(cityList, info);
    });
}

void IntegrationPluginOpenweathermap::processSearchResults(const QList<QVariantMap> &cityList, ThingDiscoveryInfo *info)
{
    foreach (QVariantMap element, cityList) {
        ThingDescriptor descriptor(openweathermapThingClassId, element.value("name").toString(), element.value("country").toString());
        ParamList params;
        Param nameParam(openweathermapThingNameParamTypeId, element.value("name"));
        params.append(nameParam);
        Param countryParam(openweathermapThingCountryParamTypeId, element.value("country"));
        params.append(countryParam);
        Param idParam(openweathermapThingIdParamTypeId, element.value("id"));
        params.append(idParam);
        descriptor.setParams(params);
        foreach (Thing *existingThing, myThings()) {
            if (existingThing->paramValue(openweathermapThingIdParamTypeId).toString() == element.value("id")) {
                descriptor.setThingId(existingThing->id());
                break;
            }
        }
        info->addThingDescriptor(descriptor);
    }
    info->finish(Thing::ThingErrorNoError);
}

void IntegrationPluginOpenweathermap::processWeatherData(const QByteArray &data, Thing *thing)
{
    QJsonParseError error;
    QJsonDocument jsonDoc = QJsonDocument::fromJson(data, &error);

    //qCDebug(dcOpenWeatherMap) << jsonDoc.toJson();

    if (error.error != QJsonParseError::NoError) {
        qCWarning(dcOpenWeatherMap) << "failed to parse weather data for thing " << thing->name() << ": " << data << ":" << error.errorString();
        return;
    }

    // http://openweathermap.org/current
    QVariantMap dataMap = jsonDoc.toVariant().toMap();
    if (dataMap.contains("clouds")) {
        int cloudiness = dataMap.value("clouds").toMap().value("all").toInt();
        thing->setStateValue(openweathermapCloudinessStateTypeId, cloudiness);
    }
    if (dataMap.contains("dt")) {
        uint lastUpdate = dataMap.value("dt").toUInt();
        thing->setStateValue(openweathermapUpdateTimeStateTypeId, lastUpdate);
    }

    if (dataMap.contains("main")) {

        double temperatur = dataMap.value("main").toMap().value("temp").toDouble();
        double temperaturMax = dataMap.value("main").toMap().value("temp_max").toDouble();
        double temperaturMin = dataMap.value("main").toMap().value("temp_min").toDouble();
        double pressure = dataMap.value("main").toMap().value("pressure").toDouble();
        int humidity = dataMap.value("main").toMap().value("humidity").toInt();

        thing->setStateValue(openweathermapTemperatureStateTypeId, temperatur);
        thing->setStateValue(openweathermapTemperatureMinStateTypeId, temperaturMin);
        thing->setStateValue(openweathermapTemperatureMaxStateTypeId, temperaturMax);
        thing->setStateValue(openweathermapPressureStateTypeId, pressure);
        thing->setStateValue(openweathermapHumidityStateTypeId, humidity);
    }

    if (dataMap.contains("sys")) {
        qint64 sunrise = dataMap.value("sys").toMap().value("sunrise").toLongLong();
        qint64 sunset = dataMap.value("sys").toMap().value("sunset").toLongLong();

        thing->setStateValue(openweathermapSunriseTimeStateTypeId, sunrise);
        thing->setStateValue(openweathermapSunsetTimeStateTypeId, sunset);
        QTimeZone tz = QTimeZone(QTimeZone::systemTimeZoneId());
        QDateTime up = QDateTime::fromMSecsSinceEpoch(sunrise * 1000);
        QDateTime down = QDateTime::fromMSecsSinceEpoch(sunset * 1000);
        QDateTime now = QDateTime::currentDateTime().toTimeZone(tz);
        thing->setStateValue(openweathermapDaylightStateTypeId, up < now && down > now);
    }

    if (dataMap.contains("visibility")) {
        int visibility = dataMap.value("visibility").toInt();
        thing->setStateValue(openweathermapVisibilityStateTypeId, visibility);
    }

    // http://openweathermap.org/weather-conditions
    if (dataMap.contains("weather") && dataMap.value("weather").toList().count() > 0) {
        int conditionId = dataMap.value("weather").toList().first().toMap().value("id").toInt();
        if (conditionId == 800) {
            if (thing->stateValue(openweathermapUpdateTimeStateTypeId).toInt() > thing->stateValue(openweathermapSunriseTimeStateTypeId).toInt() &&
                thing->stateValue(openweathermapUpdateTimeStateTypeId).toInt() < thing->stateValue(openweathermapSunsetTimeStateTypeId).toInt()) {
                thing->setStateValue(openweathermapWeatherConditionStateTypeId, "clear-day");
            } else {
                thing->setStateValue(openweathermapWeatherConditionStateTypeId, "clear-night");
            }
        } else if (conditionId == 801) {
            if (thing->stateValue(openweathermapUpdateTimeStateTypeId).toInt() > thing->stateValue(openweathermapSunriseTimeStateTypeId).toInt() &&
                thing->stateValue(openweathermapUpdateTimeStateTypeId).toInt() < thing->stateValue(openweathermapSunsetTimeStateTypeId).toInt()) {
                thing->setStateValue(openweathermapWeatherConditionStateTypeId, "few-clouds-day");
            } else {
                thing->setStateValue(openweathermapWeatherConditionStateTypeId, "few-clouds-night");
            }
        } else if (conditionId == 802) {
            thing->setStateValue(openweathermapWeatherConditionStateTypeId, "clouds");
        } else if (conditionId >= 803 && conditionId < 900) {
            thing->setStateValue(openweathermapWeatherConditionStateTypeId, "overcast");
        } else if (conditionId >= 300 && conditionId < 400) {
            thing->setStateValue(openweathermapWeatherConditionStateTypeId, "light-rain");
        } else if (conditionId >= 500 && conditionId < 600) {
            thing->setStateValue(openweathermapWeatherConditionStateTypeId, "shower-rain");
        } else if (conditionId >= 200 && conditionId < 300) {
            thing->setStateValue(openweathermapWeatherConditionStateTypeId, "thunderstorm");
        } else if (conditionId >= 600 && conditionId < 700) {
            thing->setStateValue(openweathermapWeatherConditionStateTypeId, "snow");
        } else if (conditionId >= 700 && conditionId < 800) {
            thing->setStateValue(openweathermapWeatherConditionStateTypeId, "fog");
        }
        QString description = dataMap.value("weather").toList().first().toMap().value("description").toString();
        thing->setStateValue(openweathermapWeatherDescriptionStateTypeId, description);
    }

    if (dataMap.contains("wind")) {
        int windDirection = dataMap.value("wind").toMap().value("deg").toInt();
        double windSpeed = dataMap.value("wind").toMap().value("speed").toDouble();

        thing->setStateValue(openweathermapWindDirectionStateTypeId, windDirection);
        thing->setStateValue(openweathermapWindSpeedStateTypeId, windSpeed);
    }
}

void IntegrationPluginOpenweathermap::onPluginTimer()
{
    foreach (Thing *thing, myThings()) {
        update(thing);
    }
}

