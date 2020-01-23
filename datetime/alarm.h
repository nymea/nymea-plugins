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

#ifndef ALARM_H
#define ALARM_H

#include <QObject>
#include <QDateTime>

class Alarm : public QObject
{
    Q_OBJECT
public:
    enum TimeType {
        TimeTypeTime,
        TimeTypeDusk,
        TimeTypeSunrise,
        TimeTypeNoon,
        TimeTypeSunset,
        TimeTypeDawn
    };

    explicit Alarm(QObject *parent = 0);

    void setName(const QString &name);
    QString name() const;

    void setMonday(const bool &monday);
    bool monday() const;

    void setTuesday(const bool &tuesday);
    bool tuesday() const;

    void setWednesday(const bool &wednesday);
    bool wednesday() const;

    void setThursday(const bool &thursday);
    bool thursday() const;

    void setFriday(const bool &friday);
    bool friday() const;

    void setSaturday(const bool &saturday);
    bool saturday() const;

    void setSunday(const bool &sunday);
    bool sunday() const;

    void setMinutes(const int &minutes);
    int minutes() const;

    void setHours(const int &hours);
    int hours() const;

    void setOffset(const int &offset);
    int offset() const;

    void setDusk(const QDateTime &dusk);
    void setSunrise(const QDateTime &sunrise);
    void setNoon(const QDateTime &noon);
    void setSunset(const QDateTime &sunset);
    void setDawn(const QDateTime &dawn);

    void setTimeType(const TimeType &timeType);
    void setTimeType(const QString &timeType);
    TimeType timeType() const;

private:
    QString m_name;
    bool m_monday;
    bool m_tuesday;
    bool m_wednsday;
    bool m_thursday;
    bool m_friday;
    bool m_saturday;
    bool m_sunday;

    int m_minutes;
    int m_hours;
    int m_offset;
    TimeType m_timeType;

    QDateTime m_duskOffset;
    QDateTime m_sunriseOffset;
    QDateTime m_noonOffset;
    QDateTime m_sunsetOffset;
    QDateTime m_dawnOffset;

    QDateTime getAlertTime() const;
    QDateTime calculateOffsetTime(const QDateTime &dateTime) const;

    bool checkDayOfWeek(const QDateTime &dateTime);
    bool checkHour(const QDateTime &dateTime);
    bool checkMinute(const QDateTime &dateTime);

    bool checkTimeTypes(const QDateTime &dateTime);

signals:
    void alarm();

public slots:
    void validate(const QDateTime &dateTime);
    void validateTimes(const QDateTime &dateTime);

};

#endif // ALARM_H
