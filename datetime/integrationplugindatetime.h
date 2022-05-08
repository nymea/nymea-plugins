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

#ifndef INTEGRATIONPLUGINDATETIME_H
#define INTEGRATIONPLUGINDATETIME_H

#include "integrations/integrationplugin.h"
#include "alarm.h"
#include "countdown.h"

#include <QDateTime>
#include <QTimeZone>
#include <QTime>
#include <QTimer>
#include <QNetworkReply>

class IntegrationPluginDateTime : public IntegrationPlugin
{
    Q_OBJECT

    Q_PLUGIN_METADATA(IID "io.nymea.IntegrationPlugin" FILE "integrationplugindatetime.json")
    Q_INTERFACES(IntegrationPlugin)

public:
    explicit IntegrationPluginDateTime();

    void setupThing(ThingSetupInfo *info) override;
    void postSetupThing(Thing *thing) override;
    void thingRemoved(Thing *thing) override;

    void executeAction(ThingActionInfo *info) override;

    void startMonitoringAutoThings() override;

private:
    QTimer *m_timer;
    Thing *m_todayDevice;
    QTimeZone m_timeZone;
    QDateTime m_currentDateTime;

    QHash<Thing *, Alarm *> m_alarms;
    QHash<Thing *, Countdown *> m_countdowns;

    QDateTime m_dusk;
    QDateTime m_sunrise;
    QDateTime m_noon;
    QDateTime m_sunset;
    QDateTime m_dawn;

    void searchGeoLocation();
    void processGeoLocationData(const QByteArray &data);

    void getTimes(const QString &latitude, const QString &longitude);
    void processTimesData(const QByteArray &data);

signals:
    void dusk();
    void sunset();
    void noon();
    void sunrise();
    void dawn();

private slots:
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

#endif // INTEGRATIONPLUGINDATETIME_H
