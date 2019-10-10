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

#ifndef DEVICEPLUGINDATETIME_H
#define DEVICEPLUGINDATETIME_H

#include "devices/deviceplugin.h"
#include "alarm.h"
#include "countdown.h"

#include <QDateTime>
#include <QTimeZone>
#include <QTime>
#include <QTimer>
#include <QNetworkReply>

class DevicePluginDateTime : public DevicePlugin
{
    Q_OBJECT

    Q_PLUGIN_METADATA(IID "io.nymea.DevicePlugin" FILE "deviceplugindatetime.json")
    Q_INTERFACES(DevicePlugin)

public:
    explicit DevicePluginDateTime();

    void setupDevice(DeviceSetupInfo *info) override;
    void postSetupDevice(Device *device) override;
    void deviceRemoved(Device *device) override;

    void executeAction(DeviceActionInfo *info) override;

    void startMonitoringAutoDevices() override;

private:
    QTimer *m_timer;
    Device *m_todayDevice;
    QTimeZone m_timeZone;
    QDateTime m_currentDateTime;

    QHash<Device *, Alarm *> m_alarms;
    QHash<Device *, Countdown *> m_countdowns;

    QDateTime m_dusk;
    QDateTime m_sunrise;
    QDateTime m_noon;
    QDateTime m_sunset;
    QDateTime m_dawn;

    QList<QNetworkReply *> m_locationReplies;
    QList<QNetworkReply *> m_timeReplies;

    void searchGeoLocation();
    void processGeoLocationData(const QByteArray &data);

    void getTimes(const QString &latitude, const QString &longitude);
    void processTimesData(const QByteArray &data);

    QTime parseTime(const QString &timeString) const;

signals:
    void dusk();
    void sunset();
    void noon();
    void sunrise();
    void dawn();

private slots:
    void onNetworkReplayFinished();
    void onAlarm();
    void onCountdownTimeout();
    void onCountdownRunningChanged(const bool &running);
    void onSecondChanged();
    void onMinuteChanged(const QDateTime &dateTime);
    void onHourChanged(const QDateTime &dateTime);
    void onDayChanged(const QDateTime &dateTime);

    void updateTimes();

    void validateTimeTypes(const QDateTime &dateTime);

};

#endif // DEVICEPLUGINDATETIME_H
