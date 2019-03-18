/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *                                                                         *
 *  Copyright (C) 2019 Simon Stürz <simon.stuerz@guh.io>                   *
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

#ifndef HUEOUTDOORSENSOR_H
#define HUEOUTDOORSENSOR_H

#include <QObject>

#include "huedevice.h"

class HueOutdoorSensor : public HueDevice
{
    Q_OBJECT
public:
    explicit HueOutdoorSensor(QObject *parent = nullptr);

    QString uuid() const;
    void setUuid(const QString &uuid);

    int temperatureSensorId() const;
    void setTemperatureSensorId(int sensorId);

    QString temperatureSensorUuid() const;
    void setTemperatureSensorUuid(const QString &temperatureSensorUuid);

    int presenceSensorId() const;
    void setPresenceSensorId(int sensorId);

    QString presenceSensorUuid() const;
    void setPresenceSensorUuid(const QString &presenceSensorUuid);

    int lightSensorId() const;
    void setLightSensorId(int sensorId);

    QString lightSensorUuid() const;
    void setLightSensorUuid(const QString &lightSensorUuid);

    double temperature() const;
    double lightIntensity() const;
    bool present() const;
    int batteryLevel() const;

    void updateStates(const QVariantMap &sensorMap);

    bool isValid();
    bool hasSensor(int sensorId);
    bool hasSensor(const QString &sensorUuid);

private:
    // Params
    QString m_uuid;

    int m_temperatureSensorId;
    QString m_temperatureSensorUuid;

    int m_presenceSensorId;
    QString m_presenceSensorUuid;

    int m_lightSensorId;
    QString m_lightSensorUuid;

    // States
    QString m_lastUpdate;
    double m_temperature = 0;
    double m_lightIntensity = 0;
    bool m_presence = false;
    int m_batteryLevel = 0;

signals:
    void temperatureChanged(double temperature);
    void lightIntensityChanged(double lightIntensity);
    void presenceChanged(bool presence);
    void batteryLevelChanged(int batteryLevel);

};

#endif // HUEOUTDOORSENSOR_H
