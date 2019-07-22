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

#include "deviceplugindatetime.h"

#include "plugin/device.h"
#include "devicemanager.h"
#include "plugininfo.h"
#include "hardwaremanager.h"
#include "network/networkaccessmanager.h"

#include <QJsonDocument>
#include <QUrlQuery>

DevicePluginDateTime::DevicePluginDateTime() :
    m_timer(nullptr),
    m_todayDevice(nullptr),
    m_timeZone(QTimeZone(QTimeZone::systemTimeZoneId())),
    m_dusk(QDateTime()),
    m_sunrise(QDateTime()),
    m_noon(QDateTime()),
    m_sunset(QDateTime()),
    m_dawn(QDateTime())
{
    m_timer = new QTimer(this);
    m_timer->setInterval(1000);

    m_currentDateTime = QDateTime(QDate::currentDate(), QTime::currentTime(), m_timeZone);

    connect(m_timer, &QTimer::timeout, this, &DevicePluginDateTime::onSecondChanged);
}

DeviceManager::DeviceSetupStatus DevicePluginDateTime::setupDevice(Device *device)
{
    // check timezone
    if(!m_timeZone.isValid()){
        qCWarning(dcDateTime) << "Invalid time zone.";
        return DeviceManager::DeviceSetupStatusFailure;
    }

    // date
    if (device->deviceClassId() == todayDeviceClassId) {
        if (m_todayDevice != 0) {
            qCWarning(dcDateTime) << "There is already a date device or not deleted correctly! this should never happen!!";
            return DeviceManager::DeviceSetupStatusFailure;
        }
        m_todayDevice = device;
        qCDebug(dcDateTime) << "Create today device: current time" << m_currentDateTime.currentDateTime().toString();
    }

    // alarm
    if (device->deviceClassId() == alarmDeviceClassId) {
        Alarm *alarm = new Alarm(this);
        alarm->setName(device->name());
        alarm->setMonday(device->paramValue(alarmDeviceMondayParamTypeId).toBool());
        alarm->setTuesday(device->paramValue(alarmDeviceTuesdayParamTypeId).toBool());
        alarm->setWednesday(device->paramValue(alarmDeviceWednesdayParamTypeId).toBool());
        alarm->setThursday(device->paramValue(alarmDeviceThursdayParamTypeId).toBool());
        alarm->setFriday(device->paramValue(alarmDeviceFridayParamTypeId).toBool());
        alarm->setSaturday(device->paramValue(alarmDeviceSaturdayParamTypeId).toBool());
        alarm->setSunday(device->paramValue(alarmDeviceSundayParamTypeId).toBool());
        alarm->setMinutes(device->paramValue(alarmDeviceMinutesParamTypeId).toInt());
        alarm->setHours(device->paramValue(alarmDeviceHoursParamTypeId).toInt());
        alarm->setTimeType(device->paramValue(alarmDeviceTimeTypeParamTypeId).toString());
        alarm->setOffset(device->paramValue(alarmDeviceOffsetParamTypeId).toInt());
        alarm->setDusk(m_dusk);
        alarm->setSunrise(m_sunrise);
        alarm->setNoon(m_noon);
        alarm->setDawn(m_dawn);
        alarm->setSunset(m_sunset);

        connect(alarm, &Alarm::alarm, this, &DevicePluginDateTime::onAlarm);

        m_alarms.insert(device, alarm);
    }

    if (device->deviceClassId() == countdownDeviceClassId) {
        Countdown *countdown = new Countdown(device->name(),
                                             QTime(device->paramValue(countdownDeviceHoursParamTypeId).toInt(),
                                                   device->paramValue(countdownDeviceMinutesParamTypeId).toInt(),
                                                   device->paramValue(countdownDeviceSecondsParamTypeId).toInt()),
                                             device->paramValue(countdownDeviceRepeatingParamTypeId).toBool());

        connect(countdown, &Countdown::countdownTimeout, this, &DevicePluginDateTime::onCountdownTimeout);
        connect(countdown, &Countdown::runningStateChanged, this, &DevicePluginDateTime::onCountdownRunningChanged);

        qCDebug(dcDateTime) << "Setup countdown" << countdown->name() << countdown->time().toString();
        m_countdowns.insert(device, countdown);
    }

    m_timer->start();

    return DeviceManager::DeviceSetupStatusSuccess;
}

void DevicePluginDateTime::postSetupDevice(Device *device)
{
    if (device->deviceClassId() == todayDeviceClassId) {
        QDateTime zoneTime = QDateTime::currentDateTime().toTimeZone(m_timeZone);
        updateTimes();
        onMinuteChanged(zoneTime);
        onHourChanged(zoneTime);
        onDayChanged(zoneTime);
    }
}

void DevicePluginDateTime::deviceRemoved(Device *device)
{
    // check if we still need the timer
    if (myDevices().count() == 0) {
        m_timer->stop();
    }

    // date
    if (device->deviceClassId() == todayDeviceClassId) {
        m_todayDevice = nullptr;
    }

    // alarm
    if (device->deviceClassId() == alarmDeviceClassId) {
        Alarm *alarm = m_alarms.take(device);
        alarm->deleteLater();
    }

    // countdown
    if (device->deviceClassId() == countdownDeviceClassId) {
        Countdown *countdown = m_countdowns.take(device);
        countdown->deleteLater();
    }

    //startMonitoringAutoDevices();
}

DeviceManager::DeviceError DevicePluginDateTime::executeAction(Device *device, const Action &action)
{
    if (device->deviceClassId() == countdownDeviceClassId) {
        Countdown *countdown = m_countdowns.value(device);
        if (action.actionTypeId() == countdownStartActionTypeId) {
            countdown->start();
            return DeviceManager::DeviceErrorNoError;
        } else if (action.actionTypeId() == countdownRestartActionTypeId) {
            countdown->restart();
            return DeviceManager::DeviceErrorNoError;
        } else if (action.actionTypeId() == countdownStopActionTypeId) {
            countdown->stop();
            return DeviceManager::DeviceErrorNoError;
        }
        return DeviceManager::DeviceErrorActionTypeNotFound;
    }
    return DeviceManager::DeviceErrorNoError;
}

void DevicePluginDateTime::startMonitoringAutoDevices()
{
//    foreach (Device *device, myDevices()) {
//        if (device->deviceClassId() == todayDeviceClassId) {
//            return; // We already have the date device... do nothing.
//        }
//    }

//    DeviceDescriptor dateDescriptor(todayDeviceClassId, "Date", "Time");
//    emit autoDevicesAppeared(todayDeviceClassId, QList<DeviceDescriptor>() << dateDescriptor);
}

void DevicePluginDateTime::searchGeoLocation()
{
    if (!m_todayDevice)
        return;

    QNetworkRequest request;
    request.setUrl(QUrl("http://ip-api.com/json"));

    qCDebug(dcDateTime()) << "Requesting geo location.";

    QNetworkReply *reply = hardwareManager()->networkManager()->get(request);
    connect(reply, &QNetworkReply::finished, this, &DevicePluginDateTime::onNetworkReplayFinished);
    m_locationReplies.append(reply);
}

void DevicePluginDateTime::processGeoLocationData(const QByteArray &data)
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

void DevicePluginDateTime::getTimes(const QString &latitude, const QString &longitude)
{
    QUrlQuery urlQuery;
    urlQuery.addQueryItem("lat", latitude);
    urlQuery.addQueryItem("lng", longitude);
    urlQuery.addQueryItem("date", "today");

    QUrl url = QUrl("http://api.sunrise-sunset.org/json");
    url.setQuery(urlQuery.toString());

    QNetworkRequest request;
    request.setUrl(url);

    QNetworkReply *reply = hardwareManager()->networkManager()->get(request);
    connect(reply, &QNetworkReply::finished, this, &DevicePluginDateTime::onNetworkReplayFinished);

    m_timeReplies.append(reply);
}

void DevicePluginDateTime::processTimesData(const QByteArray &data)
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

    m_dawn = QDateTime(QDate::currentDate(), parseTime(dawnString), Qt::UTC).toTimeZone(m_timeZone);
    m_sunrise = QDateTime(QDate::currentDate(), parseTime(sunriseString), Qt::UTC).toTimeZone(m_timeZone);
    m_noon = QDateTime(QDate::currentDate(), parseTime(noonString), Qt::UTC).toTimeZone(m_timeZone);
    m_sunset = QDateTime(QDate::currentDate(), parseTime(sunsetString), Qt::UTC).toTimeZone(m_timeZone);
    m_dusk = QDateTime(QDate::currentDate(), parseTime(duskString), Qt::UTC).toTimeZone(m_timeZone);

    qCDebug(dcDateTime) << " dawn     :" << m_dawn.toString() << dawnString;
    qCDebug(dcDateTime) << " sunrise  :" << m_sunrise.toString() << sunriseString;
    qCDebug(dcDateTime) << " noon     :" << m_noon.toString() << noonString;
    qCDebug(dcDateTime) << " sunset   :" << m_sunset.toString() << sunsetString;
    qCDebug(dcDateTime) << " dusk     :" << m_dusk.toString() << duskString;
    qCDebug(dcDateTime) << "---------------------------------------------";

    updateTimes();
}

QTime DevicePluginDateTime::parseTime(const QString &timeString) const
{
    bool isPm = timeString.endsWith(" PM");
    QString tmp = QString(timeString).remove(QRegExp("[ APM]*"));
    QStringList parts = tmp.split(":");
    if (parts.count() != 3) {
        qCWarning(dcDateTime()) << "Error parsing timeString:" << timeString;
        return QTime();
    }
    return QTime(parts.first().toInt(), parts.at(1).toInt(), parts.last().toInt()).addSecs(isPm ? 60 * 60 * 12 : 0);
}

void DevicePluginDateTime::onNetworkReplayFinished()
{
    QNetworkReply *reply = static_cast<QNetworkReply *>(sender());

    int status = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();

    if (m_locationReplies.contains(reply)) {
        m_locationReplies.removeAll(reply);
        if (status != 200) {
            qCWarning(dcDateTime) << "Http error status for location request:" << status << reply->error();
        } else {
            processGeoLocationData(reply->readAll());
        }
    } else if (m_timeReplies.contains(reply)) {
        m_timeReplies.removeAll(reply);
        if (status != 200) {
            qCWarning(dcDateTime) << "Http error status for time request:" << status << reply->error();
        } else {
            processTimesData(reply->readAll());
        }
    }
    reply->deleteLater();
}

void DevicePluginDateTime::onAlarm()
{
    Alarm *alarm = static_cast<Alarm *>(sender());
    Device *device = m_alarms.key(alarm);

    emit emitEvent(Event(alarmAlarmEventTypeId, device->id()));
}

void DevicePluginDateTime::onCountdownTimeout()
{
    Countdown *countdown = static_cast<Countdown *>(sender());
    Device *device = m_countdowns.key(countdown);

    emit emitEvent(Event(countdownTimeoutEventTypeId, device->id()));
}

void DevicePluginDateTime::onCountdownRunningChanged(const bool &running)
{
    Countdown *countdown = static_cast<Countdown *>(sender());
    Device *device = m_countdowns.key(countdown);

    device->setStateValue(countdownRunningStateTypeId, running);
}

void DevicePluginDateTime::onSecondChanged()
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

void DevicePluginDateTime::onMinuteChanged(const QDateTime &dateTime)
{
    //qCDebug(dcDateTime) << "minute changed" << dateTime.toString();

    // validate alerts
    foreach (Alarm *alarm, m_alarms.values()) {
        alarm->validate(dateTime);
    }
}

void DevicePluginDateTime::onHourChanged(const QDateTime &dateTime)
{
    Q_UNUSED(dateTime)
    //qCDebug(dcDateTime) << "hour changed" <<  dateTime.toString();
    // check every hour in case we are offline in the wrong moment
    searchGeoLocation();
}

void DevicePluginDateTime::onDayChanged(const QDateTime &dateTime)
{
    qCDebug(dcDateTime) << "day changed" << dateTime.toString();

    if (!m_todayDevice)
        return;

    m_todayDevice->setStateValue(todayDayStateTypeId, dateTime.date().day());
    m_todayDevice->setStateValue(todayMonthStateTypeId, dateTime.date().month());
    m_todayDevice->setStateValue(todayYearStateTypeId, dateTime.date().year());
    m_todayDevice->setStateValue(todayWeekdayStateTypeId, dateTime.date().dayOfWeek());
    m_todayDevice->setStateValue(todayWeekdayNameStateTypeId, dateTime.date().longDayName(dateTime.date().dayOfWeek()));
    m_todayDevice->setStateValue(todayMonthNameStateTypeId, dateTime.date().longMonthName(dateTime.date().month()));
    if(dateTime.date().dayOfWeek() == 6 || dateTime.date().dayOfWeek() == 7){
        m_todayDevice->setStateValue(todayWeekendStateTypeId, true);
    }else{
        m_todayDevice->setStateValue(todayWeekendStateTypeId, false);
    }
}

void DevicePluginDateTime::updateTimes()
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
        m_todayDevice->setStateValue(todayDuskStateTypeId, m_dusk.toTime_t());
    } else {
        m_todayDevice->setStateValue(todayDuskStateTypeId, 0);
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
        m_todayDevice->setStateValue(todayNoonStateTypeId, m_noon.toTime_t());
    } else {
        m_todayDevice->setStateValue(todayNoonStateTypeId, 0);
    }
    if (m_dusk.isValid()) {
        m_todayDevice->setStateValue(todayDawnStateTypeId, m_dawn.toTime_t());
    } else {
        m_todayDevice->setStateValue(todayDawnStateTypeId, 0);
    }
}


void DevicePluginDateTime::validateTimeTypes(const QDateTime &dateTime)
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
