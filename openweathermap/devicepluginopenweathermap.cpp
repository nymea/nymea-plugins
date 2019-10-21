/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *                                                                         *
 *  Copyright (C) 2015 Simon Stürz <simon.stuerz@guh.io>                   *
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

#include "devicepluginopenweathermap.h"
#include "devices/device.h"
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

DevicePluginOpenweathermap::DevicePluginOpenweathermap()
{
    // max 60 calls/minute
    // max 50000 calls/day
    m_apiKey = "c1b9d5584bb740804871583f6c62744f";

    QSettings settings(NymeaSettings::settingsPath() + "/nymead.conf", QSettings::IniFormat);
    settings.beginGroup("OpenWeatherMap");
    if (settings.contains("apiKey")) {
        m_apiKey = settings.value("apiKey").toString();
        qCDebug(dcOpenWeatherMap()) << "Using custom API key:" << m_apiKey.replace(m_apiKey.length() - 10, 10, "**********");
    }

    settings.endGroup();
}

DevicePluginOpenweathermap::~DevicePluginOpenweathermap()
{
    hardwareManager()->pluginTimerManager()->unregisterTimer(m_pluginTimer);
}

void DevicePluginOpenweathermap::init()
{
    m_pluginTimer = hardwareManager()->pluginTimerManager()->registerTimer(900);
    connect(m_pluginTimer, &PluginTimer::timeout, this, &DevicePluginOpenweathermap::onPluginTimer);
}

void DevicePluginOpenweathermap::discoverDevices(DeviceDiscoveryInfo *info)
{
    QString location = info->params().paramValue(openweathermapDiscoveryLocationParamTypeId).toString();

    // if we have an empty search string, perform an autodetection of the location with the WAN ip...
    if (location.isEmpty()){
        searchAutodetect(info);
    } else {
        search(location, info);
    }
}

void DevicePluginOpenweathermap::setupDevice(DeviceSetupInfo *info)
{
    update(info->device());
    info->finish(Device::DeviceErrorNoError);
}

void DevicePluginOpenweathermap::executeAction(DeviceActionInfo *info)
{
    update(info->device());
    info->finish(Device::DeviceErrorNoError);
}

void DevicePluginOpenweathermap::deviceRemoved(Device *device)
{
    // check if a device gets removed while we still have a reply!
    foreach (Device *d, m_weatherReplies.values()) {
        if (d->id() == device->id()) {
            QNetworkReply *reply = m_weatherReplies.key(device);
            m_weatherReplies.take(reply);
            reply->deleteLater();
        }
    }
}

void DevicePluginOpenweathermap::networkManagerReplyReady()
{
    QNetworkReply *reply = static_cast<QNetworkReply *>(sender());

    if (reply->error()) {
        qCWarning(dcOpenWeatherMap) << "OpenWeatherMap reply error: " << reply->errorString();
        reply->deleteLater();
        return;
    }

    if (m_weatherReplies.contains(reply)) {
        QByteArray data = reply->readAll();
        Device* device = m_weatherReplies.take(reply);
        processWeatherData(data, device);
    }
    reply->deleteLater();
}

void DevicePluginOpenweathermap::update(Device *device)
{
    qCDebug(dcOpenWeatherMap()) << "Refresh data for" << device->name();
    QUrl url("http://api.openweathermap.org/data/2.5/weather");
    QUrlQuery query;
    query.addQueryItem("id", device->paramValue(openweathermapDeviceIdParamTypeId).toString());
    query.addQueryItem("mode", "json");
    query.addQueryItem("units", "metric");
    query.addQueryItem("appid", m_apiKey);
    url.setQuery(query);

    QNetworkReply *reply = hardwareManager()->networkManager()->get(QNetworkRequest(url));
    connect(reply, &QNetworkReply::finished, this, &DevicePluginOpenweathermap::networkManagerReplyReady);
    m_weatherReplies.insert(reply, device);
}

void DevicePluginOpenweathermap::searchAutodetect(DeviceDiscoveryInfo *info)
{
    QNetworkReply *reply = hardwareManager()->networkManager()->get(QNetworkRequest(QUrl("http://ip-api.com/json")));
    connect(reply, &QNetworkReply::finished, reply, &QNetworkReply::deleteLater);
    connect(reply, &QNetworkReply::finished, info, [this, info, reply](){
        if (reply->error()) {
            qCWarning(dcOpenWeatherMap) << "OpenWeatherMap reply error: " << reply->errorString();
            info->finish(Device::DeviceErrorHardwareFailure, QT_TR_NOOP("Error detecting current location."));
            return;
        }
        QByteArray data = reply->readAll();
        QJsonParseError error;
        QJsonDocument jsonDoc = QJsonDocument::fromJson(data, &error);

        if(error.error != QJsonParseError::NoError) {
            qCWarning(dcOpenWeatherMap) << "failed to parse data" << data << ":" << error.errorString();
            info->finish(Device::DeviceErrorHardwareFailure, QT_TR_NOOP("Received unexpected data detecting current location."));
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

void DevicePluginOpenweathermap::search(QString searchString, DeviceDiscoveryInfo *info)
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
            info->finish(Device::DeviceErrorHardwareFailure, QT_TR_NOOP("Error searching for weather stations."));
            return;
        }
        QByteArray data = reply->readAll();
        QJsonParseError error;
        QJsonDocument jsonDoc = QJsonDocument::fromJson(data, &error);

        if(error.error != QJsonParseError::NoError) {
            qCWarning(dcOpenWeatherMap) << "failed to parse data" << data << ":" << error.errorString();
            info->finish(Device::DeviceErrorHardwareFailure, QT_TR_NOOP("Received unexpected data while searching for weather stations."));
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

void DevicePluginOpenweathermap::searchGeoLocation(double lat, double lon, const QString &country, DeviceDiscoveryInfo *info)
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
            info->finish(Device::DeviceErrorHardwareFailure, QT_TR_NOOP("Error searching for weather stations in current location."));
            return;
        }
        QByteArray data = reply->readAll();
        QJsonParseError error;
        QJsonDocument jsonDoc = QJsonDocument::fromJson(data, &error);

        if(error.error != QJsonParseError::NoError) {
            qCWarning(dcOpenWeatherMap) << "failed to parse data" << data << ":" << error.errorString();
            info->finish(Device::DeviceErrorHardwareFailure, QT_TR_NOOP("Received unexpected data while searching for weather stations."));
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

void DevicePluginOpenweathermap::processSearchResults(const QList<QVariantMap> &cityList, DeviceDiscoveryInfo *info)
{
    foreach (QVariantMap element, cityList) {
        DeviceDescriptor descriptor(openweathermapDeviceClassId, element.value("name").toString(), element.value("country").toString());
        ParamList params;
        Param nameParam(openweathermapDeviceNameParamTypeId, element.value("name"));
        params.append(nameParam);
        Param countryParam(openweathermapDeviceCountryParamTypeId, element.value("country"));
        params.append(countryParam);
        Param idParam(openweathermapDeviceIdParamTypeId, element.value("id"));
        params.append(idParam);
        descriptor.setParams(params);
        foreach (Device *existingDevice, myDevices()) {
            if (existingDevice->paramValue(openweathermapDeviceIdParamTypeId).toString() == element.value("id")) {
                descriptor.setDeviceId(existingDevice->id());
                break;
            }
        }
        info->addDeviceDescriptor(descriptor);
    }
    info->finish(Device::DeviceErrorNoError);
}

void DevicePluginOpenweathermap::processWeatherData(const QByteArray &data, Device *device)
{
    QJsonParseError error;
    QJsonDocument jsonDoc = QJsonDocument::fromJson(data, &error);

    //qCDebug(dcOpenWeatherMap) << jsonDoc.toJson();

    if (error.error != QJsonParseError::NoError) {
        qCWarning(dcOpenWeatherMap) << "failed to parse weather data for device " << device->name() << ": " << data << ":" << error.errorString();
        return;
    }

    // http://openweathermap.org/current
    QVariantMap dataMap = jsonDoc.toVariant().toMap();
    if (dataMap.contains("clouds")) {
        int cloudiness = dataMap.value("clouds").toMap().value("all").toInt();
        device->setStateValue(openweathermapCloudinessStateTypeId, cloudiness);
    }
    if (dataMap.contains("dt")) {
        uint lastUpdate = dataMap.value("dt").toUInt();
        device->setStateValue(openweathermapUpdateTimeStateTypeId, lastUpdate);
    }

    if (dataMap.contains("main")) {

        double temperatur = dataMap.value("main").toMap().value("temp").toDouble();
        double temperaturMax = dataMap.value("main").toMap().value("temp_max").toDouble();
        double temperaturMin = dataMap.value("main").toMap().value("temp_min").toDouble();
        double pressure = dataMap.value("main").toMap().value("pressure").toDouble();
        int humidity = dataMap.value("main").toMap().value("humidity").toInt();

        device->setStateValue(openweathermapTemperatureStateTypeId, temperatur);
        device->setStateValue(openweathermapTemperatureMinStateTypeId, temperaturMin);
        device->setStateValue(openweathermapTemperatureMaxStateTypeId, temperaturMax);
        device->setStateValue(openweathermapPressureStateTypeId, pressure);
        device->setStateValue(openweathermapHumidityStateTypeId, humidity);
    }

    if (dataMap.contains("sys")) {
        qint64 sunrise = dataMap.value("sys").toMap().value("sunrise").toLongLong();
        qint64 sunset = dataMap.value("sys").toMap().value("sunset").toLongLong();

        device->setStateValue(openweathermapSunriseTimeStateTypeId, sunrise);
        device->setStateValue(openweathermapSunsetTimeStateTypeId, sunset);
        QTimeZone tz = QTimeZone(QTimeZone::systemTimeZoneId());
        QDateTime up = QDateTime::fromMSecsSinceEpoch(sunrise * 1000);
        QDateTime down = QDateTime::fromMSecsSinceEpoch(sunset * 1000);
        QDateTime now = QDateTime::currentDateTime().toTimeZone(tz);
        device->setStateValue(openweathermapDaylightStateTypeId, up < now && down > now);
    }

    if (dataMap.contains("visibility")) {
        int visibility = dataMap.value("visibility").toInt();
        device->setStateValue(openweathermapVisibilityStateTypeId, visibility);
    }

    // http://openweathermap.org/weather-conditions
    if (dataMap.contains("weather") && dataMap.value("weather").toList().count() > 0) {
        int conditionId = dataMap.value("weather").toList().first().toMap().value("id").toInt();
        if (conditionId == 800) {
            if (device->stateValue(openweathermapUpdateTimeStateTypeId).toInt() > device->stateValue(openweathermapSunriseTimeStateTypeId).toInt() &&
                device->stateValue(openweathermapUpdateTimeStateTypeId).toInt() < device->stateValue(openweathermapSunsetTimeStateTypeId).toInt()) {
                device->setStateValue(openweathermapWeatherConditionStateTypeId, "clear-day");
            } else {
                device->setStateValue(openweathermapWeatherConditionStateTypeId, "clear-night");
            }
        } else if (conditionId == 801) {
            if (device->stateValue(openweathermapUpdateTimeStateTypeId).toInt() > device->stateValue(openweathermapSunriseTimeStateTypeId).toInt() &&
                device->stateValue(openweathermapUpdateTimeStateTypeId).toInt() < device->stateValue(openweathermapSunsetTimeStateTypeId).toInt()) {
                device->setStateValue(openweathermapWeatherConditionStateTypeId, "few-clouds-day");
            } else {
                device->setStateValue(openweathermapWeatherConditionStateTypeId, "few-clouds-night");
            }
        } else if (conditionId == 802) {
            device->setStateValue(openweathermapWeatherConditionStateTypeId, "clouds");
        } else if (conditionId >= 803 && conditionId < 900) {
            device->setStateValue(openweathermapWeatherConditionStateTypeId, "overcast");
        } else if (conditionId >= 300 && conditionId < 400) {
            device->setStateValue(openweathermapWeatherConditionStateTypeId, "light-rain");
        } else if (conditionId >= 500 && conditionId < 600) {
            device->setStateValue(openweathermapWeatherConditionStateTypeId, "shower-rain");
        } else if (conditionId >= 200 && conditionId < 300) {
            device->setStateValue(openweathermapWeatherConditionStateTypeId, "thunderstorm");
        } else if (conditionId >= 600 && conditionId < 700) {
            device->setStateValue(openweathermapWeatherConditionStateTypeId, "snow");
        } else if (conditionId >= 700 && conditionId < 800) {
            device->setStateValue(openweathermapWeatherConditionStateTypeId, "fog");
        }
        QString description = dataMap.value("weather").toList().first().toMap().value("description").toString();
        device->setStateValue(openweathermapWeatherDescriptionStateTypeId, description);
    }

    if (dataMap.contains("wind")) {
        int windDirection = dataMap.value("wind").toMap().value("deg").toInt();
        double windSpeed = dataMap.value("wind").toMap().value("speed").toDouble();

        device->setStateValue(openweathermapWindDirectionStateTypeId, windDirection);
        device->setStateValue(openweathermapWindSpeedStateTypeId, windSpeed);
    }
}

void DevicePluginOpenweathermap::onPluginTimer()
{
    foreach (Device *device, myDevices()) {
        update(device);
    }
}

