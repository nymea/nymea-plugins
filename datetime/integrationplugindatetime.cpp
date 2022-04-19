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

#include "integrationplugindatetime.h"

#include "integrations/thing.h"
#include "plugininfo.h"
#include "hardwaremanager.h"
#include "network/networkaccessmanager.h"

#include <QJsonDocument>
#include <QUrlQuery>

IntegrationPluginDateTime::IntegrationPluginDateTime() :
    m_timer(nullptr),
    m_todayDevice(nullptr),
    m_timeZone(QTimeZone::systemTimeZoneId()),
    m_dusk(QDateTime()),
    m_sunrise(QDateTime()),
    m_noon(QDateTime()),
    m_sunset(QDateTime()),
    m_dawn(QDateTime())
{
    m_timer = new QTimer(this);
    m_timer->setInterval(1000);

    m_currentDateTime = QDateTime(QDate::currentDate(), QTime::currentTime(), m_timeZone);

    connect(m_timer, &QTimer::timeout, this, &IntegrationPluginDateTime::onSecondChanged);
}

void IntegrationPluginDateTime::setupThing(ThingSetupInfo *info)
{
    Thing *thing = info->thing();

    // check timezone
    if(!m_timeZone.isValid()){
        qCWarning(dcDateTime) << "Invalid time zone.";
        info->finish(Thing::ThingErrorInvalidParameter);
        return;
    }

    // date
    if (thing->thingClassId() == todayThingClassId) {
        if (m_todayDevice != 0) {
            qCWarning(dcDateTime) << "There is already a date thing or not deleted correctly! this should never happen!!";
            info->finish(Thing::ThingErrorHardwareNotAvailable);
            return;
        }
        m_todayDevice = thing;
        qCDebug(dcDateTime) << "Create today thing: current time" << m_currentDateTime.currentDateTime().toString();
    }

    // alarm
    if (thing->thingClassId() == alarmThingClassId) {
        Alarm *alarm = new Alarm(this);
        alarm->setName(thing->name());
        alarm->setMonday(thing->paramValue(alarmThingMondayParamTypeId).toBool());
        alarm->setTuesday(thing->paramValue(alarmThingTuesdayParamTypeId).toBool());
        alarm->setWednesday(thing->paramValue(alarmThingWednesdayParamTypeId).toBool());
        alarm->setThursday(thing->paramValue(alarmThingThursdayParamTypeId).toBool());
        alarm->setFriday(thing->paramValue(alarmThingFridayParamTypeId).toBool());
        alarm->setSaturday(thing->paramValue(alarmThingSaturdayParamTypeId).toBool());
        alarm->setSunday(thing->paramValue(alarmThingSundayParamTypeId).toBool());
        alarm->setMinutes(thing->paramValue(alarmThingMinutesParamTypeId).toInt());
        alarm->setHours(thing->paramValue(alarmThingHoursParamTypeId).toInt());
        alarm->setTimeType(thing->paramValue(alarmThingTimeTypeParamTypeId).toString());
        alarm->setOffset(thing->paramValue(alarmThingOffsetParamTypeId).toInt());
        alarm->setDusk(m_dusk);
        alarm->setSunrise(m_sunrise);
        alarm->setNoon(m_noon);
        alarm->setDawn(m_dawn);
        alarm->setSunset(m_sunset);

        connect(alarm, &Alarm::alarm, this, &IntegrationPluginDateTime::onAlarm);

        m_alarms.insert(thing, alarm);
    }

    if (thing->thingClassId() == countdownThingClassId) {
        Countdown *countdown = new Countdown(thing->name(),
                                             QTime(thing->paramValue(countdownThingHoursParamTypeId).toInt(),
                                                   thing->paramValue(countdownThingMinutesParamTypeId).toInt(),
                                                   thing->paramValue(countdownThingSecondsParamTypeId).toInt()),
                                                   thing->paramValue(countdownThingRepeatingParamTypeId).toBool());

        connect(countdown, &Countdown::countdownTimeout, this, &IntegrationPluginDateTime::onCountdownTimeout);
        connect(countdown, &Countdown::runningStateChanged, this, &IntegrationPluginDateTime::onCountdownRunningChanged);

        qCDebug(dcDateTime) << "Setup countdown" << countdown->name() << countdown->time().toString();
        m_countdowns.insert(thing, countdown);
    }

    m_timer->start();

    info->finish(Thing::ThingErrorNoError);
}

void IntegrationPluginDateTime::postSetupThing(Thing *thing)
{
    if (thing->thingClassId() == todayThingClassId) {
        QDateTime zoneTime = QDateTime::currentDateTime().toTimeZone(m_timeZone);
        updateTimes();
        onMinuteChanged(zoneTime);
        onHourChanged(zoneTime);
        onDayChanged(zoneTime);
    }
}

void IntegrationPluginDateTime::thingRemoved(Thing *thing)
{
    // check if we still need the timer
    if (myThings().count() == 0) {
        m_timer->stop();
    }

    // date
    if (thing->thingClassId() == todayThingClassId) {
        m_todayDevice = nullptr;
    }

    // alarm
    if (thing->thingClassId() == alarmThingClassId) {
        Alarm *alarm = m_alarms.take(thing);
        alarm->deleteLater();
    }

    // countdown
    if (thing->thingClassId() == countdownThingClassId) {
        Countdown *countdown = m_countdowns.take(thing);
        countdown->deleteLater();
    }

    //startMonitoringAutoThings();
}

void IntegrationPluginDateTime::executeAction(ThingActionInfo *info)
{
    if (info->thing()->thingClassId() != countdownThingClassId) {
        info->finish(Thing::ThingErrorThingClassNotFound);
        return;
    }

    Countdown *countdown = m_countdowns.value(info->thing());
    if (info->action().actionTypeId() == countdownStartActionTypeId) {
        countdown->start();
    } else if (info->action().actionTypeId() == countdownRestartActionTypeId) {
        countdown->restart();
    } else if (info->action().actionTypeId() == countdownStopActionTypeId) {
        countdown->stop();
    }

    info->finish(Thing::ThingErrorNoError);
}

void IntegrationPluginDateTime::startMonitoringAutoThings()
{
//    foreach (Thing *thing, myThings()) {
//        if (thing->thingClassId() == todayThingClassId) {
//            return; // We already have the date thing... do nothing.
//        }
//    }

//    ThingDescriptor dateDescriptor(todayThingClassId, "Date", "Time");
//    emit autoThingsAppeared(todayThingClassId, QList<ThingDescriptor>() << dateDescriptor);
}

void IntegrationPluginDateTime::searchGeoLocation()
{
    if (!m_todayDevice)
        return;

    QNetworkRequest request;
    request.setUrl(QUrl("http://ip-api.com/json"));

    qCDebug(dcDateTime()) << "Requesting geo location.";

    QNetworkReply *reply = hardwareManager()->networkManager()->get(request);
    connect(reply, &QNetworkReply::finished, this, [reply, this](){
        reply->deleteLater();

        if (reply->error() != QNetworkReply::NoError) {
            qCWarning(dcDateTime) << "Http error status for location request:" << reply->error();
            return;
        }
        processGeoLocationData(reply->readAll());
    });
}

void IntegrationPluginDateTime::processGeoLocationData(const QByteArray &data)
{
    QJsonParseError error;
    QJsonDocument jsonDoc = QJsonDocument::fromJson(data, &error);

    if(error.error != QJsonParseError::NoError) {
        qCWarning(dcDateTime) << "failed to parse location JSON data:" << error.errorString() << ":" << data ;
        return;
    }

    //qCDebug(dcDateTime) << "geo location data received:" << jsonDoc.toJson();
    QVariantMap response = jsonDoc.toVariant().toMap();
    if (response.value("status") != "success") {
        qCWarning(dcDateTime) << "failed to request geo location:" << response.value("status");
    }

    // check timezone
    QString timeZone = response.value("timezone").toString();

    m_todayDevice->setStateValue(todayTimeZoneStateTypeId, timeZone);
    m_todayDevice->setStateValue(todayCityStateTypeId, response.value("city").toString());
    m_todayDevice->setStateValue(todayCountryStateTypeId, response.value("country").toString());

    qCDebug(dcDateTime) << "---------------------------------------------";
    qCDebug(dcDateTime) << "autodetected location for" << response.value("query").toString();
    qCDebug(dcDateTime) << " city     :" << response.value("city").toString();
    qCDebug(dcDateTime) << " country  :" << response.value("country").toString();
    qCDebug(dcDateTime) << " code     :" << response.value("countryCode").toString();
    qCDebug(dcDateTime) << " zip code :" << response.value("zip").toString();
    qCDebug(dcDateTime) << " lon      :" << response.value("lon").toByteArray();
    qCDebug(dcDateTime) << " lat      :" << response.value("lat").toByteArray();
    qCDebug(dcDateTime) << "---------------------------------------------";

    getTimes(response.value("lat").toString(), response.value("lon").toString());
}

void IntegrationPluginDateTime::getTimes(const QString &latitude, const QString &longitude)
{
    QUrlQuery urlQuery;
    urlQuery.addQueryItem("lat", latitude);
    urlQuery.addQueryItem("lng", longitude);
    urlQuery.addQueryItem("date", "today");

    QUrl url = QUrl("https://api.sunrise-sunset.org/json");
    url.setQuery(urlQuery.toString());

    QNetworkRequest request;
    request.setUrl(url);

    QNetworkReply *reply = hardwareManager()->networkManager()->get(request);
    connect(reply, &QNetworkReply::finished, this, [reply, this](){
        reply->deleteLater();

        if (reply->error() != QNetworkReply::NoError) {
            qCWarning(dcDateTime) << "Http error status for time request:" << reply->error();
            return;
        }
        processTimesData(reply->readAll());
    });
}

void IntegrationPluginDateTime::processTimesData(const QByteArray &data)
{
    QJsonParseError error;
    QJsonDocument jsonDoc = QJsonDocument::fromJson(data, &error);

    if(error.error != QJsonParseError::NoError) {
        qCWarning(dcDateTime) << "failed to parse time JSON data:" << error.errorString() << ":" << data ;
        return;
    }

    qCDebug(dcDateTime()) << "Raw data:" << qUtf8Printable(jsonDoc.toJson(QJsonDocument::Indented));
    QVariantMap response = jsonDoc.toVariant().toMap();
    if (response.value("status") != "OK") {
        qCWarning(dcDateTime) << "failed to request time data:" << response.value("status");
        return;
    }

    // given time is always in UTC
    QVariantMap result = response.value("results").toMap();
    QString dawnString = result.value("civil_twilight_begin").toString();
    QString sunriseString = result.value("sunrise").toString();
    QString noonString = result.value("solar_noon").toString();
    QString sunsetString = result.value("sunset").toString();
    QString duskString = result.value("civil_twilight_end").toString();

    // calculate the times in each alarm
    m_dawn = QDateTime(QDate::currentDate(), QTime::fromString(dawnString, "h:mm:ss AP"), Qt::UTC).toTimeZone(m_timeZone);
    m_sunrise = QDateTime(QDate::currentDate(), QTime::fromString(sunriseString, "h:mm:ss AP"), Qt::UTC).toTimeZone(m_timeZone);
    m_noon = QDateTime(QDate::currentDate(), QTime::fromString(noonString, "h:mm:ss AP"), Qt::UTC).toTimeZone(m_timeZone);
    m_sunset = QDateTime(QDate::currentDate(), QTime::fromString(sunsetString, "h:mm:ss AP"), Qt::UTC).toTimeZone(m_timeZone);
    m_dusk = QDateTime(QDate::currentDate(), QTime::fromString(duskString, "h:mm:ss AP"), Qt::UTC).toTimeZone(m_timeZone);

    qCDebug(dcDateTime) << " dawn     :" << m_dawn.toString() << dawnString;
    qCDebug(dcDateTime) << " sunrise  :" << m_sunrise.toString() << sunriseString;
    qCDebug(dcDateTime) << " noon     :" << m_noon.toString() << noonString;
    qCDebug(dcDateTime) << " sunset   :" << m_sunset.toString() << sunsetString;
    qCDebug(dcDateTime) << " dusk     :" << m_dusk.toString() << duskString;
    qCDebug(dcDateTime) << "---------------------------------------------";

    updateTimes();
}

void IntegrationPluginDateTime::onAlarm()
{
    Alarm *alarm = static_cast<Alarm *>(sender());
    Thing *thing = m_alarms.key(alarm);

    emit emitEvent(Event(alarmAlarmEventTypeId, thing->id()));
}

void IntegrationPluginDateTime::onCountdownTimeout()
{
    Countdown *countdown = static_cast<Countdown *>(sender());
    Thing *thing = m_countdowns.key(countdown);

    emit emitEvent(Event(countdownTimeoutEventTypeId, thing->id()));
}

void IntegrationPluginDateTime::onCountdownRunningChanged(const bool &running)
{
    Countdown *countdown = static_cast<Countdown *>(sender());
    Thing *thing = m_countdowns.key(countdown);

    thing->setStateValue(countdownRunningStateTypeId, running);
}

void IntegrationPluginDateTime::onSecondChanged()
{
    QDateTime currentTime = QDateTime::currentDateTime().toTimeZone(m_timeZone);
    // make shore that ms are 0
    QDateTime zoneTime = QDateTime(QDate(currentTime.date()), QTime(currentTime.time().hour(), currentTime.time().minute(), currentTime.time().second(), 0));

    validateTimeTypes(zoneTime);

    if (zoneTime.date() != m_currentDateTime.date())
        onDayChanged(zoneTime);

    if (zoneTime.time().hour() != m_currentDateTime.time().hour())
        onHourChanged(zoneTime);

    if (zoneTime.time().minute() != m_currentDateTime.time().minute())
        onMinuteChanged(zoneTime);

    // just store for compairing
    m_currentDateTime = zoneTime;
}

void IntegrationPluginDateTime::onMinuteChanged(const QDateTime &dateTime)
{
    //qCDebug(dcDateTime) << "minute changed" << dateTime.toString();

    // validate alerts
    foreach (Alarm *alarm, m_alarms.values()) {
        alarm->validate(dateTime);
    }
}

void IntegrationPluginDateTime::onHourChanged(const QDateTime &dateTime)
{
    Q_UNUSED(dateTime)
    //qCDebug(dcDateTime) << "hour changed" <<  dateTime.toString();
    // check every hour in case we are offline in the wrong moment
    searchGeoLocation();
}

void IntegrationPluginDateTime::onDayChanged(const QDateTime &dateTime)
{
    qCDebug(dcDateTime) << "day changed" << dateTime.toString();

    if (!m_todayDevice)
        return;

    m_todayDevice->setStateValue(todayDayStateTypeId, dateTime.date().day());
    m_todayDevice->setStateValue(todayMonthStateTypeId, dateTime.date().month());
    m_todayDevice->setStateValue(todayYearStateTypeId, dateTime.date().year());
    m_todayDevice->setStateValue(todayWeekdayStateTypeId, dateTime.date().dayOfWeek());
    m_todayDevice->setStateValue(todayWeekdayNameStateTypeId, QLocale().dayName(dateTime.date().dayOfWeek()));
    m_todayDevice->setStateValue(todayMonthNameStateTypeId, QLocale().monthName(dateTime.date().month()));
    if(dateTime.date().dayOfWeek() == 6 || dateTime.date().dayOfWeek() == 7){
        m_todayDevice->setStateValue(todayWeekendStateTypeId, true);
    }else{
        m_todayDevice->setStateValue(todayWeekendStateTypeId, false);
    }
}

void IntegrationPluginDateTime::updateTimes()
{
    // alarms
    foreach (Alarm *alarm, m_alarms.values()) {
        alarm->setDusk(m_dusk);
        alarm->setSunrise(m_sunrise);
        alarm->setNoon(m_noon);
        alarm->setDawn(m_dawn);
        alarm->setSunset(m_sunset);
    }

    // date
    if (!m_todayDevice)
        return;

    if (m_dusk.isValid()) {
        m_todayDevice->setStateValue(todayDuskTimeStateTypeId, m_dusk.toTime_t());
    } else {
        m_todayDevice->setStateValue(todayDuskTimeStateTypeId, 0);
    }
    if (m_sunrise.isValid() && m_sunset.isValid()) {
        m_todayDevice->setStateValue(todaySunriseTimeStateTypeId, m_sunrise.toTime_t());
        m_todayDevice->setStateValue(todaySunsetTimeStateTypeId, m_sunset.toTime_t());
        m_todayDevice->setStateValue(todayDaylightStateTypeId, m_sunrise < m_currentDateTime && m_currentDateTime < m_sunset);
    } else {
        m_todayDevice->setStateValue(todaySunriseTimeStateTypeId, 0);
        m_todayDevice->setStateValue(todaySunsetTimeStateTypeId, 0);
        m_todayDevice->setStateValue(todayDaylightStateTypeId, false);
    }
    if (m_dusk.isValid()) {
        m_todayDevice->setStateValue(todayNoonTimeStateTypeId, m_noon.toTime_t());
    } else {
        m_todayDevice->setStateValue(todayNoonTimeStateTypeId, 0);
    }
    if (m_dusk.isValid()) {
        m_todayDevice->setStateValue(todayDawnTimeStateTypeId, m_dawn.toTime_t());
    } else {
        m_todayDevice->setStateValue(todayDawnTimeStateTypeId, 0);
    }
}


void IntegrationPluginDateTime::validateTimeTypes(const QDateTime &dateTime)
{
    if (!m_todayDevice)
        return;

    if (dateTime == m_dusk) {
        emit emitEvent(Event(todayDuskEventTypeId, m_todayDevice->id()));
    } else if (dateTime == m_sunrise) {
        emit emitEvent(Event(todaySunriseEventTypeId, m_todayDevice->id()));
    } else if (dateTime == m_noon) {
        emit emitEvent(Event(todayNoonEventTypeId, m_todayDevice->id()));
    } else if (dateTime == m_dawn) {
        emit emitEvent(Event(todayDawnEventTypeId, m_todayDevice->id()));
    } else if (dateTime == m_sunset) {
        emit emitEvent(Event(todaySunsetEventTypeId, m_todayDevice->id()));
    }

    foreach (Alarm *alarm, m_alarms.values()) {
        alarm->validateTimes(dateTime);
    }
}
