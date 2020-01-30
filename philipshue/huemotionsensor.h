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

#ifndef HUEMOTIONSENSOR_H
#define HUEMOTIONSENSOR_H

#include <QObject>
#include <QTimer>

#include "extern-plugininfo.h"
#include "huedevice.h"

class HueMotionSensor : public HueDevice
{
    Q_OBJECT
public:
    explicit HueMotionSensor(QObject *parent = nullptr);
    virtual ~HueMotionSensor() = default;

    void setTimeout(int timeout);

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

    virtual StateTypeId connectedStateTypeId() const = 0;
    virtual StateTypeId temperatureStateTypeId() const = 0;
    virtual StateTypeId lightIntensityStateTypeId() const = 0;
    virtual StateTypeId isPresentStateTypeId() const = 0;
    virtual StateTypeId lastSeenTimeStateTypeId() const = 0;
    virtual StateTypeId batteryLevelStateTypeId() const = 0;
    virtual StateTypeId batteryCriticalStateTypeId() const = 0;

private:
    // Params
    int m_temperatureSensorId;
    QString m_temperatureSensorUuid;

    int m_presenceSensorId;
    QString m_presenceSensorUuid;

    int m_lightSensorId;
    QString m_lightSensorUuid;

    QTimer m_timeout;

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

class HueIndoorSensor: public HueMotionSensor
{
    Q_OBJECT
public:
    HueIndoorSensor(QObject *parent = nullptr): HueMotionSensor(parent) {}

    StateTypeId connectedStateTypeId() const override { return motionSensorConnectedStateTypeId; }
    StateTypeId temperatureStateTypeId() const override { return motionSensorTemperatureStateTypeId; }
    StateTypeId lightIntensityStateTypeId() const override { return motionSensorLightIntensityStateTypeId; }
    StateTypeId isPresentStateTypeId() const override { return motionSensorIsPresentStateTypeId; }
    StateTypeId lastSeenTimeStateTypeId() const override { return motionSensorLastSeenTimeStateTypeId; }
    StateTypeId batteryLevelStateTypeId() const override { return motionSensorBatteryLevelStateTypeId; }
    StateTypeId batteryCriticalStateTypeId() const override { return motionSensorBatteryCriticalStateTypeId; }

};

class HueOutdoorSensor: public HueMotionSensor
{
    Q_OBJECT
public:
    HueOutdoorSensor(QObject *parent = nullptr): HueMotionSensor(parent) {}

    StateTypeId connectedStateTypeId() const override { return outdoorSensorConnectedStateTypeId; }
    StateTypeId temperatureStateTypeId() const override { return outdoorSensorTemperatureStateTypeId; }
    StateTypeId lightIntensityStateTypeId() const override { return outdoorSensorLightIntensityStateTypeId; }
    StateTypeId isPresentStateTypeId() const override { return outdoorSensorIsPresentStateTypeId; }
    StateTypeId lastSeenTimeStateTypeId() const override { return outdoorSensorLastSeenTimeStateTypeId; }
    StateTypeId batteryLevelStateTypeId() const override { return outdoorSensorBatteryLevelStateTypeId; }
    StateTypeId batteryCriticalStateTypeId() const override { return outdoorSensorBatteryCriticalStateTypeId; }

};

#endif // HUEMOTIONSENSOR_H
