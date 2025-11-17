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

#include "plugininfo.h"
#include "integrationplugindaylightsensor.h"

#include <network/networkaccessmanager.h>

#include <QUrlQuery>
#include <QNetworkReply>
#include <QJsonDocument>
#include <QtMath>
#include <QPair>
#include <QTimeZone>

#define CIVIL_ZENITH  90.83333

IntegrationPluginDaylightSensor::IntegrationPluginDaylightSensor()
{

}

IntegrationPluginDaylightSensor::~IntegrationPluginDaylightSensor()
{

}

void IntegrationPluginDaylightSensor::discoverThings(ThingDiscoveryInfo *info)
{
    QNetworkRequest request(QUrl("http://ip-api.com/json"));
    QNetworkReply* reply = hardwareManager()->networkManager()->get(request);
    connect(reply, &QNetworkReply::finished, reply, &QNetworkReply::deleteLater);
    connect(reply, &QNetworkReply::finished, info, [reply, info]() {
        if (reply->error() != QNetworkReply::NoError) {
            qCWarning(dcDaylightSensor()) << "Error fetching geolocation from ip-api:" << reply->error() << reply->errorString();
            info->finish(Thing::ThingErrorHardwareFailure, QT_TR_NOOP("Failed to fetch data from the internet."));
            return;
        }
        QJsonParseError error;
        QJsonDocument jsonDoc = QJsonDocument::fromJson(reply->readAll(), &error);
        if (error.error != QJsonParseError::NoError) {
            qCWarning(dcDaylightSensor()) << "Failed to parse json from ip-api:" << error.error << error.errorString();
            info->finish(Thing::ThingErrorHardwareFailure, QT_TR_NOOP("The server returned unexpected data."));
            return;
        }
        if (!jsonDoc.toVariant().toMap().contains("lat") || !jsonDoc.toVariant().toMap().contains("lon")) {
            qCWarning(dcDaylightSensor()) << "Reply missing geolocation info" << qUtf8Printable(jsonDoc.toJson(QJsonDocument::Indented));
            info->finish(Thing::ThingErrorHardwareFailure, QT_TR_NOOP("The server returned unexpected data."));
            return;
        }
        qreal lat = jsonDoc.toVariant().toMap().value("lat").toDouble();
        qreal lon = jsonDoc.toVariant().toMap().value("lon").toDouble();

        ThingDescriptor descriptor(info->thingClassId(), tr("Daylight sensor"), jsonDoc.toVariant().toMap().value("city").toString());
        ParamList params;
        params.append(Param(daylightSensorThingLatitudeParamTypeId, lat));
        params.append(Param(daylightSensorThingLongitudeParamTypeId, lon));
        descriptor.setParams(params);
        info->addThingDescriptor(descriptor);
        info->finish(Thing::ThingErrorNoError);
    });
}

void IntegrationPluginDaylightSensor::setupThing(ThingSetupInfo *info)
{
    updateDevice(info->thing());
    info->finish(Thing::ThingErrorNoError);
}

void IntegrationPluginDaylightSensor::thingRemoved(Thing *thing)
{
    hardwareManager()->pluginTimerManager()->unregisterTimer(m_timers.take(thing));
}

void IntegrationPluginDaylightSensor::pluginTimerEvent()
{
    PluginTimer *timer = static_cast<PluginTimer*>(sender());
    Thing *thing = m_timers.key(timer);

    if (!myThings().contains(thing)) {
        qCDebug(dcDaylightSensor()) << "Device has disappeared. Exiting timer loop.";
        return;
    }

    updateDevice(thing);
}

void IntegrationPluginDaylightSensor::updateDevice(Thing *thing)
{
    PluginTimer *timer = m_timers.value(thing);
    if (timer) {
        hardwareManager()->pluginTimerManager()->unregisterTimer(timer);
    }

    QTimeZone tz = QTimeZone(QTimeZone::systemTimeZoneId());
    QDateTime now = QDateTime::currentDateTime().toTimeZone(tz);
    auto sunriseSunset = calculateSunriseSunset(thing->paramValue(daylightSensorThingLatitudeParamTypeId).toDouble(), thing->paramValue(daylightSensorThingLongitudeParamTypeId).toDouble(), now);
    QDateTime sunrise = sunriseSunset.first.toTimeZone(tz);
    QDateTime sunset = sunriseSunset.second.toTimeZone(tz);
    qCDebug(dcDaylightSensor()) << "Setting up daylight sensor:" << thing->name() << "Sunrise:" << sunrise.toString() << "Sunset:" << sunset.toString();
    thing->setStateValue(daylightSensorSunriseTimeStateTypeId, sunrise.toSecsSinceEpoch());
    thing->setStateValue(daylightSensorSunsetTimeStateTypeId, sunset.toSecsSinceEpoch());
    thing->setStateValue(daylightSensorDaylightStateTypeId, sunrise < now && sunset > now);

    qint64 timeToNext = -1;
    if (now < sunrise) {
        timeToNext = now.secsTo(sunrise);
    } else if (now < sunset) {
        timeToNext = now.secsTo(sunset);
    } else {
        timeToNext = (60 * 60 * 24) - (now.time().msecsSinceStartOfDay() / 1000);
    }
    // Refresh at earliest in 5 secs to avoid spamming the system when we get close
    timeToNext = qMax(static_cast<int>(timeToNext), 1);

    timer = hardwareManager()->pluginTimerManager()->registerTimer(static_cast<int>(timeToNext));
    qCDebug(dcDaylightSensor()) << "Recalculating in" << timer->interval() << "seconds";
    connect(timer, &PluginTimer::timeout, this, &IntegrationPluginDaylightSensor::pluginTimerEvent);
    m_timers.insert(thing, timer);
}

QPair<QDateTime, QDateTime> IntegrationPluginDaylightSensor::calculateSunriseSunset(qreal latitude, qreal longitude, const QDateTime &dateTime)
{
    int dayOfYear = dateTime.date().dayOfYear();
    int offset = dateTime.offsetFromUtc() / 60 / 60;

    // Convert the longitude to hour value and calculate an approximate time
    qreal longitudeHour = longitude / 15;
    qreal tRise = dayOfYear + ((6 - longitudeHour) / 24);
    qreal tSet = dayOfYear + ((18 - longitudeHour) / 24);

    // Calculate the Sun's mean anomaly
    qreal mRise = (0.9856 * tRise) - 3.289;
    qreal mSet = (0.9856 * tSet) - 3.289;

    // Calculate the Sun's true longitude, and adjust angle to be between 0 and 360
    qreal tmp = mRise + (1.916 * qSin(qDegreesToRadians(mRise))) + (0.020 * qSin(qDegreesToRadians(2 * mRise))) + 282.634;
    qreal lRise = qFloor(tmp + 360) % 360 + (tmp - qFloor(tmp));
    tmp = mSet + (1.916 * qSin(qDegreesToRadians(mSet))) + (0.020 * qSin(qDegreesToRadians(2 * mSet))) + 282.634;
    qreal lSet = qFloor(tmp + 360) % 360 + (tmp - qFloor(tmp));

    // Calculate the Sun's right ascension, and adjust angle to be between 0 and 360
    tmp = qRadiansToDegrees(qAtan(0.91764 * qTan(qDegreesToRadians(1.0 * lRise))));
    qreal raRise = qFloor(tmp + 360) % 360 + (tmp - qFloor(tmp));
    tmp = qRadiansToDegrees(qAtan(0.91764 * qTan(qDegreesToRadians(1.0 * lSet))));
    qreal raSet = qRound(tmp + 360) % 360 + (tmp - qFloor(tmp));

    // Right ascension value needs to be in the same quadrant as L
    qlonglong lQuadrantRise  = qFloor(lRise/90) * 90;
    qlonglong raQuadrantRise = qFloor(raRise/90) * 90;
    raRise = raRise + (lQuadrantRise - raQuadrantRise);

    qlonglong lQuadrantSet  = qFloor(lSet/90) * 90;
    qlonglong raQuadrantSet = qFloor(raSet/90) * 90;
    raSet = raSet + (lQuadrantSet - raQuadrantSet);

    // Right ascension value needs to be converted into hours
    raRise = raRise / 15;
    raSet = raSet / 15;

    // Calculate the Sun's declination
    qreal sinDecRise = 0.39782 * qSin(qDegreesToRadians(1.0 * lRise));
    qreal cosDecRise = qCos(qAsin(sinDecRise));

    qreal sinDecSet = 0.39782 * qSin(qDegreesToRadians(1.0 * lSet));
    qreal cosDecSet = qCos(qAsin(sinDecSet));

    // Calculate the Sun's local hour angle
    qreal cosZenith = qCos(qDegreesToRadians(CIVIL_ZENITH));
    qreal radianLat = qDegreesToRadians(latitude);
    qreal sinLatitude = qSin(radianLat);
    qreal cosLatitude = qCos(radianLat);
    qreal cosHRise = (cosZenith - (sinDecRise * sinLatitude)) / (cosDecRise * cosLatitude);
    qreal cosHSet = (cosZenith - (sinDecSet * sinLatitude)) / (cosDecSet * cosLatitude);

    // Finish calculating H and convert into hours
    qreal hRise = (360 - qRadiansToDegrees(qAcos(cosHRise))) / 15;
    qreal hSet = qRadiansToDegrees(qAcos(cosHSet)) / 15;

    // Calculate local mean time of rising/setting
    tRise = hRise + raRise - (0.06571 * tRise) - 6.622;
    tSet = hSet + raSet - (0.06571 * tSet) - 6.622;

    // Adjust back to UTC, and keep the time between 0 and 24
    tmp = tRise - longitudeHour;
    qreal utRise = qFloor(tmp + 24) % 24 + (tmp - qFloor(tmp));
    tmp = tSet - longitudeHour;
    qreal utSet = qFloor(tmp + 24) % 24 + (tmp - qFloor(tmp));

    // Adjust again to localtime
    tmp = utRise + offset;
    qreal localtRise = qFloor(tmp + 24) % 24 + (tmp - qFloor(tmp));
    tmp = utSet + offset;
    qreal localtSet = qFloor(tmp + 24) % 24 + (tmp - qFloor(tmp));

    // Conversion
    int hourRise = qFloor(localtRise);
    int minuteRise = qFloor((localtRise - qFloor(localtRise)) * 60);
    int hourSet = qFloor(localtSet);
    int minuteSet = qFloor((localtSet - qFloor(localtSet)) * 60);

    QDateTime sunrise(dateTime.date(), QTime(hourRise, minuteRise));
    QDateTime sunset(dateTime.date(), QTime(hourSet, minuteSet));

    return QPair<QDateTime, QDateTime>(sunrise, sunset);
}
