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

#include "hueoutdoorsensor.h"
#include "extern-plugininfo.h"

HueOutdoorSensor::HueOutdoorSensor(QObject *parent) :
    HueDevice(parent)
{

}

QString HueOutdoorSensor::uuid() const
{
    return m_uuid;
}

void HueOutdoorSensor::setUuid(const QString &uuid)
{
    m_uuid = uuid;
}

int HueOutdoorSensor::temperatureSensorId() const
{
    return m_temperatureSensorId;
}

void HueOutdoorSensor::setTemperatureSensorId(int sensorId)
{
    m_temperatureSensorId = sensorId;
}

QString HueOutdoorSensor::temperatureSensorUuid() const
{
    return m_temperatureSensorUuid;
}

void HueOutdoorSensor::setTemperatureSensorUuid(const QString &temperatureSensorUuid)
{
    m_temperatureSensorUuid = temperatureSensorUuid;
}

int HueOutdoorSensor::presenceSensorId() const
{
    return m_presenceSensorId;
}

void HueOutdoorSensor::setPresenceSensorId(int sensorId)
{
    m_presenceSensorId = sensorId;
}

QString HueOutdoorSensor::presenceSensorUuid() const
{
    return m_presenceSensorUuid;
}

void HueOutdoorSensor::setPresenceSensorUuid(const QString &presenceSensorUuid)
{
    m_presenceSensorUuid = presenceSensorUuid;
}

int HueOutdoorSensor::lightSensorId() const
{
    return m_lightSensorId;
}

void HueOutdoorSensor::setLightSensorId(int sensorId)
{
    m_lightSensorId = sensorId;
}

QString HueOutdoorSensor::lightSensorUuid() const
{
    return m_lightSensorUuid;
}

void HueOutdoorSensor::setLightSensorUuid(const QString &lightSensorUuid)
{
    m_lightSensorUuid = lightSensorUuid;
}

double HueOutdoorSensor::temperature() const
{
    return m_temperature;
}

double HueOutdoorSensor::lightIntensity() const
{
    return m_lightIntensity;
}

bool HueOutdoorSensor::present() const
{
    return m_presence;
}

int HueOutdoorSensor::batteryLevel() const
{
    return m_batteryLevel;
}

void HueOutdoorSensor::updateStates(const QVariantMap &sensorMap)
{
    //qCDebug(dcPhilipsHue()) << "Outdoor sensor: Process sensor map" << qUtf8Printable(QJsonDocument::fromVariant(sensorMap).toJson(QJsonDocument::Indented));

    // Config
    QVariantMap configMap = sensorMap.value("config").toMap();
    if (configMap.contains("reachable")) {
        setReachable(configMap.value("reachable", false).toBool());
    }

    if (configMap.contains("battery")) {
        int batteryLevel = configMap.value("battery", 0).toInt();
        if (m_batteryLevel != batteryLevel) {
            m_batteryLevel = batteryLevel;
            emit batteryLevelChanged(m_batteryLevel);
        }
    }

    // If temperature sensor
    QVariantMap stateMap = sensorMap.value("state").toMap();
    if (sensorMap.value("uniqueid").toString() == m_temperatureSensorUuid) {
        double temperature = stateMap.value("temperature", 0).toInt() / 100.0;
        if (m_temperature != temperature) {
            m_temperature = temperature;
            emit temperatureChanged(m_temperature);
            qCDebug(dcPhilipsHue) << "Outdoor sensor temperature changed" << m_temperature;
        }
    }

    // If presence sensor
    if (sensorMap.value("uniqueid").toString() == m_presenceSensorUuid) {
        bool presence = stateMap.value("presence", false).toBool();
        if (m_presence != presence) {
            m_presence = presence;
            emit presenceChanged(m_presence);
            qCDebug(dcPhilipsHue) << "Outdoor sensor presence changed" << presence;
        }
    }

    // If light sensor
    if (sensorMap.value("uniqueid").toString() == m_lightSensorUuid) {
        int lightIntensity = stateMap.value("lightlevel", 0).toInt();
        if (m_lightIntensity != lightIntensity) {
            m_lightIntensity = lightIntensity;
            emit lightIntensityChanged(m_lightIntensity);
            qCDebug(dcPhilipsHue) << "Outdoor sensor light intensity changed" << m_lightIntensity;
        }
    }
}

bool HueOutdoorSensor::isValid()
{
    return !m_temperatureSensorUuid.isEmpty() && !m_presenceSensorUuid.isEmpty() && !m_lightSensorUuid.isEmpty();
}

bool HueOutdoorSensor::hasSensor(int sensorId)
{
    return m_temperatureSensorId == sensorId || m_presenceSensorId == sensorId || m_lightSensorId == sensorId;
}

bool HueOutdoorSensor::hasSensor(const QString &sensorUuid)
{
    return m_temperatureSensorUuid == sensorUuid || m_presenceSensorUuid == sensorUuid || m_lightSensorUuid == sensorUuid;
}
