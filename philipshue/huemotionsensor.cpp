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

#include "huemotionsensor.h"
#include "extern-plugininfo.h"

#include <QtMath>

HueMotionSensor::HueMotionSensor(QObject *parent) :
    HueDevice(parent)
{
    m_timeout.setInterval(10000);
    connect(&m_timeout, &QTimer::timeout, this, [this](){
        if (m_presence) {
            qCDebug(dcPhilipsHue) << "Motion sensor timeout reached" << m_timeout.interval();
            m_presence = false;
            emit presenceChanged(m_presence);
        }
    });
}

void HueMotionSensor::setTimeout(int timeout)
{
    // The sensor keeps emitting presence = true for 10 secs, let's subtract that time from the timeout to compensate
    m_timeout.setInterval((timeout - 9)* 1000);
}

int HueMotionSensor::temperatureSensorId() const
{
    return m_temperatureSensorId;
}

void HueMotionSensor::setTemperatureSensorId(int sensorId)
{
    m_temperatureSensorId = sensorId;
}

QString HueMotionSensor::temperatureSensorUuid() const
{
    return m_temperatureSensorUuid;
}

void HueMotionSensor::setTemperatureSensorUuid(const QString &temperatureSensorUuid)
{
    m_temperatureSensorUuid = temperatureSensorUuid;
}

int HueMotionSensor::presenceSensorId() const
{
    return m_presenceSensorId;
}

void HueMotionSensor::setPresenceSensorId(int sensorId)
{
    m_presenceSensorId = sensorId;
}

QString HueMotionSensor::presenceSensorUuid() const
{
    return m_presenceSensorUuid;
}

void HueMotionSensor::setPresenceSensorUuid(const QString &presenceSensorUuid)
{
    m_presenceSensorUuid = presenceSensorUuid;
}

int HueMotionSensor::lightSensorId() const
{
    return m_lightSensorId;
}

void HueMotionSensor::setLightSensorId(int sensorId)
{
    m_lightSensorId = sensorId;
}

QString HueMotionSensor::lightSensorUuid() const
{
    return m_lightSensorUuid;
}

void HueMotionSensor::setLightSensorUuid(const QString &lightSensorUuid)
{
    m_lightSensorUuid = lightSensorUuid;
}

double HueMotionSensor::temperature() const
{
    return m_temperature;
}

double HueMotionSensor::lightIntensity() const
{
    return m_lightIntensity;
}

bool HueMotionSensor::present() const
{
    return m_presence;
}

int HueMotionSensor::batteryLevel() const
{
    return m_batteryLevel;
}

void HueMotionSensor::updateStates(const QVariantMap &sensorMap)
{
//    qCDebug(dcPhilipsHue()) << "Motion sensor data:" << qUtf8Printable(QJsonDocument::fromVariant(sensorMap).toJson(QJsonDocument::Indented));

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
            qCDebug(dcPhilipsHue) << "Motion sensor temperature changed" << m_temperature;
        }
    }

    // If presence sensor
    if (sensorMap.value("uniqueid").toString() == m_presenceSensorUuid) {
        bool presence = stateMap.value("presence", false).toBool();
        if (presence) {
            if (!m_presence) {
                m_presence = true;
                emit presenceChanged(m_presence);
                qCDebug(dcPhilipsHue) << "Motion sensor presence changed" << presence;
            }
            qCDebug(dcPhilipsHue) << "Motion sensor restarting timeout" << m_timeout.interval();
            m_timeout.start();
        }
    }

    // If light sensor
    if (sensorMap.value("uniqueid").toString() == m_lightSensorUuid) {
        // Hue Light level is "10000 * log10(lux) + 1"
        // => lux = 10^((lightLevel - 1) / 10000)
        double lightIntensity = qPow(10, (stateMap.value("lightlevel", 0).toDouble()-1)/10000.0);
        // Round to 2 digits
        lightIntensity = qRound(lightIntensity * 100) / 100.0;
        if (!qFuzzyCompare(m_lightIntensity, lightIntensity)) {
            m_lightIntensity = lightIntensity;
            qCDebug(dcPhilipsHue) << "Motion sensor light intensity changed" << m_lightIntensity;
            emit lightIntensityChanged(m_lightIntensity);
        }
    }
}

bool HueMotionSensor::isValid()
{
    return !m_temperatureSensorUuid.isEmpty() && !m_presenceSensorUuid.isEmpty() && !m_lightSensorUuid.isEmpty();
}

bool HueMotionSensor::hasSensor(int sensorId)
{
    return m_temperatureSensorId == sensorId || m_presenceSensorId == sensorId || m_lightSensorId == sensorId;
}

bool HueMotionSensor::hasSensor(const QString &sensorUuid)
{
    return m_temperatureSensorUuid == sensorUuid || m_presenceSensorUuid == sensorUuid || m_lightSensorUuid == sensorUuid;
}
