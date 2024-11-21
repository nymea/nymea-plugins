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
}

IntegrationPluginOpenweathermap::~IntegrationPluginOpenweathermap()
{
}

void IntegrationPluginOpenweathermap::init()
{
    updateApiKey();

    connect(this, &IntegrationPlugin::configValueChanged, this, &IntegrationPluginOpenweathermap::updateApiKey);
    connect(apiKeyStorage(), &ApiKeyStorage::keyAdded, this, &IntegrationPluginOpenweathermap::updateApiKey);
    connect(apiKeyStorage(), &ApiKeyStorage::keyUpdated, this, &IntegrationPluginOpenweathermap::updateApiKey);
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
    weather(info->thing());
    forecast(info->thing());

    if (m_apiKey.isEmpty()) {
        info->finish(Thing::ThingErrorAuthenticationFailure, QT_TR_NOOP("No API key for OpenWeatherMap available."));
        return;
    }
    info->finish(Thing::ThingErrorNoError);

    if (!m_pluginTimer) {
        m_pluginTimer = hardwareManager()->pluginTimerManager()->registerTimer(900);
        connect(m_pluginTimer, &PluginTimer::timeout, this, [this](){
            foreach (Thing *thing, myThings()) {
                weather(thing);
                //TODO get once a day the forecast
            }
        });
    }
}

void IntegrationPluginOpenweathermap::executeAction(ThingActionInfo *info)
{
    weather(info->thing());
    info->finish(Thing::ThingErrorNoError);
}

void IntegrationPluginOpenweathermap::thingRemoved(Thing *thing)
{
    Q_UNUSED(thing)
    if (myThings().isEmpty()) {
        hardwareManager()->pluginTimerManager()->unregisterTimer(m_pluginTimer);
        m_pluginTimer = nullptr;
    }
}

void IntegrationPluginOpenweathermap::updateApiKey()
{
    if (!(m_apiKey = configValue(openWeatherMapPluginCustomApiKeyParamTypeId).toString()).isEmpty()) {
        qCDebug(dcOpenWeatherMap()) << "Using API key from plugin settings.";
    } else if (!(m_apiKey = apiKeyStorage()->requestKey("openweathermap").data("appid")).isEmpty()) {
        qCDebug(dcOpenWeatherMap()) << "Using API key from nymea API keys provider";
    } else {
        qCWarning(dcOpenWeatherMap()) << "No API key set. This plugin might not work correctly.";
        qCWarning(dcOpenWeatherMap()) << "Either install an API key pacakge (nymea-apikeysprovider-plugin-*) or provide a key in the plugin settings.";
    }
}

void IntegrationPluginOpenweathermap::weather(Thing *thing)
{
    qCDebug(dcOpenWeatherMap()) << "Refreshing weather data for" << thing->name();
    QUrl url("http://api.openweathermap.org/data/2.5/weather");
    QUrlQuery query;
    query.addQueryItem("id", thing->paramValue(openweathermapThingIdParamTypeId).toString());
    query.addQueryItem("mode", "json");
    query.addQueryItem("units", "metric");
    query.addQueryItem("appid", m_apiKey);
    url.setQuery(query);

    QNetworkReply *reply = hardwareManager()->networkManager()->get(QNetworkRequest(url));
    connect(reply, &QNetworkReply::finished, reply, &QNetworkReply::deleteLater);
    connect(reply, &QNetworkReply::finished, thing, [thing, reply](){
        if (reply->error() != QNetworkReply::NoError) {
            qCWarning(dcOpenWeatherMap) << "OpenWeatherMap reply error: " << reply->errorString();
            return;
        }

        QByteArray data = reply->readAll();

        QJsonParseError error;
        QJsonDocument jsonDoc = QJsonDocument::fromJson(data, &error);

        if (error.error != QJsonParseError::NoError) {
            qCWarning(dcOpenWeatherMap()) << "failed to parse weather data for thing " << thing->name() << "\n" << data << "\n" << error.errorString();
            return;
        }

        qCDebug(dcOpenWeatherMap()) << "Received" << qUtf8Printable(jsonDoc.toJson());

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
    });
}

void IntegrationPluginOpenweathermap::forecast(Thing *thing)
{
    qCDebug(dcOpenWeatherMap()) << "Refreshing forecast data for" << thing->name();
    QUrl url("http://api.openweathermap.org/data/2.5/forecast");
    QUrlQuery query;
    query.addQueryItem("id", thing->paramValue(openweathermapThingIdParamTypeId).toString());
    query.addQueryItem("mode", "json");
    query.addQueryItem("units", "metric");
    query.addQueryItem("appid", m_apiKey);
    url.setQuery(query);

    QNetworkReply *reply = hardwareManager()->networkManager()->get(QNetworkRequest(url));
    connect(reply, &QNetworkReply::finished, reply, &QNetworkReply::deleteLater);
    connect(reply, &QNetworkReply::finished, thing, [thing, reply](){
        if (reply->error() != QNetworkReply::NoError) {
            qCWarning(dcOpenWeatherMap) << "OpenWeatherMap reply error: " << reply->errorString();
            return;
        }

        QByteArray data = reply->readAll();

        QJsonParseError error;
        QJsonDocument jsonDoc = QJsonDocument::fromJson(data, &error);

        if (error.error != QJsonParseError::NoError) {
            qCWarning(dcOpenWeatherMap()) << "Failed to parse forecast data for thing " << thing->name() << "\n" << data << "\n" << error.errorString();
            return;
        }

        //qCDebug(dcOpenWeatherMap()) << "Received" << qUtf8Printable(jsonDoc.toJson());
        QVariantMap dataMap = jsonDoc.toVariant().toMap();
        int count = dataMap.value("cnt").toInt();

        QHash<int, int> conditionIdList;
        QHash<int, double> temperatureList;
        QHash<int, double> humidityList;
        QHash<int, double> pressureList;
        QHash<int, double> cloudinessList;
        QHash<int, int> windDirectionList;
        QHash<int, double> windSpeedList;
        QHash<int, bool> daylightList;
        QHash<int, int> rainList;
        QHash<int, int> snowList;

        if (dataMap.contains("list")) {
            Q_FOREACH(QVariant listEntry, dataMap.value("list").toList()) {
                QVariantMap map = listEntry.toMap();

                if (!map.contains("dt")) {
                    continue;
                }

                int dateTime = map.value("dt").toUInt();

                if (map.contains("main")) {
                    temperatureList.insert(dateTime, map.value("main").toMap().value("temp").toDouble());
                    //temperatureMinList.insert(dateTime, map.value("main").toMap().value("temp_min").toDouble());
                    //temperatureMaxList.insert(dateTime, map.value("main").toMap().value("temp_max").toDouble());
                    humidityList.insert(dateTime, map.value("main").toMap().value("humidity").toDouble());
                    pressureList.insert(dateTime, map.value("main").toMap().value("pressure").toDouble());
                }
                if (map.contains("weather")) {
                    conditionIdList.insert(dateTime, map.value("weather").toMap().value("id").toDouble());
                }
                if (map.contains("clouds")) {
                    cloudinessList.insert(dateTime, map.value("clouds").toMap().value("all").toInt());
                }
                if (map.contains("wind")) {
                    windDirectionList.insert(dateTime, map.value("wind").toMap().value("deg").toInt());
                    windSpeedList.insert(dateTime, map.value("wind").toMap().value("speed").toDouble());
                }
                if (map.contains("rain")) {
                    rainList.insert(dateTime, map.value("rain").toMap().first().toInt()); // 3h or 1h
                }
                if (map.contains("snow")) {
                    snowList.insert(dateTime, map.value("snow").toMap().first().toInt()); // 3h or 1h
                }
                if (map.contains("sys")) {
                    daylightList.insert(dateTime, (map.value("sys").toMap().value("pod").toString() == "d"));
                }
            }

            qCDebug(dcOpenWeatherMap()) << "Received forecast";
            qCDebug(dcOpenWeatherMap()) << "        - Expected entry count" << count;
            qCDebug(dcOpenWeatherMap()) << "        - Condition list entries" << conditionIdList.count();
            qCDebug(dcOpenWeatherMap()) << "        - Temperature list entries" << temperatureList.count();
            qCDebug(dcOpenWeatherMap()) << "        - Humidity list entries" << humidityList.count();
            qCDebug(dcOpenWeatherMap()) << "        - Pressure list entries" << pressureList.count();
            qCDebug(dcOpenWeatherMap()) << "        - Cloudiness list entries" << cloudinessList.count();
            qCDebug(dcOpenWeatherMap()) << "        - Wind direction list entries" << windDirectionList.count();
            qCDebug(dcOpenWeatherMap()) << "        - Wind speed list entries" << windSpeedList.count();
            qCDebug(dcOpenWeatherMap()) << "        - Daylight list entries" << daylightList.count();
            qCDebug(dcOpenWeatherMap()) << "        - Rain list entries" << rainList.count();
            qCDebug(dcOpenWeatherMap()) << "        - Snowlist entries" << snowList.count();

            //TODO set forecast states
        }
    });
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


