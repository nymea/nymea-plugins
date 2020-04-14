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

#include "sensordataprocessor.h"
#include "extern-plugininfo.h"
#include "math.h"

#include <QVector3D>
#include <QByteArray>
#include <QDataStream>
#include <QDateTime>

SensorDataProcessor::SensorDataProcessor(Thing *thing, QObject *parent) :
    QObject(parent),
    m_thing(thing)
{
    // Create data filters
    m_temperatureFilter = new SensorFilter(SensorFilter::TypeLowPass, this);
    m_temperatureFilter->setLowPassAlpha(0.1);
    m_temperatureFilter->setFilterWindowSize(30);

    m_objectTemperatureFilter = new SensorFilter(SensorFilter::TypeLowPass, this);
    m_objectTemperatureFilter->setLowPassAlpha(0.4);
    m_objectTemperatureFilter->setFilterWindowSize(20);

    m_humidityFilter = new SensorFilter(SensorFilter::TypeLowPass, this);
    m_humidityFilter->setLowPassAlpha(0.1);
    m_humidityFilter->setFilterWindowSize(30);

    m_pressureFilter = new SensorFilter(SensorFilter::TypeLowPass, this);
    m_pressureFilter->setLowPassAlpha(0.1);
    m_pressureFilter->setFilterWindowSize(30);

    m_opticalFilter = new SensorFilter(SensorFilter::TypeLowPass, this);
    m_opticalFilter->setLowPassAlpha(0.01);
    m_opticalFilter->setFilterWindowSize(10);

    m_accelerometerFilter = new SensorFilter(SensorFilter::TypeLowPass, this);
    m_accelerometerFilter->setLowPassAlpha(0.6);
    m_accelerometerFilter->setFilterWindowSize(40);

    // Check if the data should be logged
    if (m_filterDebug) {
        m_logFile = new QFile("/tmp/multisensor.log", this);
        if (!m_logFile->open(QIODevice::Append | QIODevice::Text)) {
            qCWarning(dcTexasInstruments()) << "Could not open log file" << m_logFile->fileName();
            delete m_logFile;
            m_logFile = nullptr;
        }
    }
}

SensorDataProcessor::~SensorDataProcessor()
{
    if (m_logFile) {
        m_logFile->close();
    }
}

void SensorDataProcessor::setAccelerometerRange(int accelerometerRange)
{
    m_accelerometerRange = accelerometerRange;
}

void SensorDataProcessor::setMovementSensitivity(int movementSensitivity)
{
    m_movementSensitivity = movementSensitivity;
}

double SensorDataProcessor::roundValue(float value)
{
    int tmpValue = static_cast<int>(value * 10);
    return static_cast<double>(tmpValue) / 10.0;
}

bool SensorDataProcessor::testBitUint8(quint8 value, int bitPosition)
{
    return (((value)>> (bitPosition)) & 1);
}

void SensorDataProcessor::processTemperatureData(const QByteArray &data)
{
    Q_ASSERT(data.count() == 4);

    quint16 rawObjectTemperature = 0;
    quint16 rawAmbientTemperature = 0;

    QByteArray payload(data);
    QDataStream stream(&payload, QIODevice::ReadOnly);
    stream.setByteOrder(QDataStream::LittleEndian);
    stream >> rawObjectTemperature >> rawAmbientTemperature ;

    float scaleFactor = 0.03125;
    float objectTemperature = static_cast<float>(rawObjectTemperature) / 4 * scaleFactor;
    float ambientTemperature = static_cast<float>(rawAmbientTemperature) / 4 * scaleFactor;

    //qCDebug(dcTexasInstruments()) << "Temperature value" << data.toHex();
    //qCDebug(dcTexasInstruments()) << "Object temperature" << roundValue(objectTemperature) << "°C";
    //qCDebug(dcTexasInstruments()) << "Ambient temperature" << roundValue(ambientTemperature) << "°C";

    float objectTemperatureFiltered = m_objectTemperatureFilter->filterValue(objectTemperature);
    float ambientTemperatureFiltered = m_temperatureFilter->filterValue(ambientTemperature);

    if (m_objectTemperatureFilter->isReady()) {
        m_thing->setStateValue(sensorTagObjectTemperatureStateTypeId, roundValue(objectTemperatureFiltered));
    }

    // Note: only change the state once the filter has collected enough data
    if (m_temperatureFilter->isReady()) {
        m_thing->setStateValue(sensorTagTemperatureStateTypeId, roundValue(ambientTemperatureFiltered));
    }

}

void SensorDataProcessor::processKeyData(const QByteArray &data)
{
    Q_ASSERT(data.count() == 1);
    quint8 flags = static_cast<quint8>(data.at(0));
    setLeftButtonPressed(testBitUint8(flags, 0));
    setRightButtonPressed(testBitUint8(flags, 1));
    setMagnetDetected(testBitUint8(flags, 2));
}

void SensorDataProcessor::processHumidityData(const QByteArray &data)
{
    Q_ASSERT(data.count() == 4);
    quint16 rawHumidityTemperature = 0;
    quint16 rawHumidity = 0;

    QByteArray payload(data);
    QDataStream stream(&payload, QIODevice::ReadOnly);
    stream.setByteOrder(QDataStream::LittleEndian);
    stream >> rawHumidityTemperature >> rawHumidity ;

    // Note: we don't need the temperature measurement from the humidity sensor
    //double humidityTemperature = (rawHumidityTemperature / 65536.0 * 165.0) - 40;
    double humidity = rawHumidity / 65536.0 * 100.0;
    double humidityFiltered = m_humidityFilter->filterValue(humidity);

    if (m_humidityFilter->isReady()) {
        m_thing->setStateValue(sensorTagHumidityStateTypeId, roundValue(humidityFiltered));
    }
}

void SensorDataProcessor::processPressureData(const QByteArray &data)
{
    Q_ASSERT(data.count() == 6);

    QByteArray temperatureData(data.left(3));
    quint32 rawTemperature = static_cast<quint8>(temperatureData.at(2));
    rawTemperature <<= 8;
    rawTemperature |= static_cast<quint8>(temperatureData.at(1));
    rawTemperature <<= 8;
    rawTemperature |= static_cast<quint8>(temperatureData.at(0));

    QByteArray pressureData(data.right(3));
    quint32 rawPressure = static_cast<quint8>(pressureData.at(2));
    rawPressure <<= 8;
    rawPressure |= static_cast<quint8>(pressureData.at(1));
    rawPressure <<= 8;
    rawPressure |= static_cast<quint8>(pressureData.at(0));

    // Note: we don't need the temperature measurement from the barometic pressure sensor
    //qCDebug(dcTexasInstruments()) << "Pressure temperature:" << roundValue(rawTemperature / 100.0) << "°C";
    //qCDebug(dcTexasInstruments()) << "Pressure:" << roundValue(rawPressure / 100.0) << "mBar";

    double pressureFiltered = m_pressureFilter->filterValue(rawPressure / 100.0);
    if (m_pressureFilter->isReady()) {
        m_thing->setStateValue(sensorTagPressureStateTypeId, roundValue(pressureFiltered));
    }
}

void SensorDataProcessor::processOpticalData(const QByteArray &data)
{
    Q_ASSERT(data.count() == 2);

    quint16 rawOptical = 0;
    QByteArray payload(data);
    QDataStream stream(&payload, QIODevice::ReadOnly);
    stream.setByteOrder(QDataStream::LittleEndian);
    stream >> rawOptical;

    quint16 lumm = rawOptical & 0x0FFF;
    quint16 lume = (rawOptical & 0xF000) >> 12;

    double lux = lumm * (0.01 * pow(2,lume));

    //qCDebug(dcTexasInstruments()) << "Lux:" << lux;

    double luxFiltered = m_opticalFilter->filterValue(lux);
    if (m_opticalFilter->isReady()) {
        m_thing->setStateValue(sensorTagLightIntensityStateTypeId, qRound(luxFiltered));
    }

    logSensorValue(lux, qRound(luxFiltered));
}

void SensorDataProcessor::processMovementData(const QByteArray &data)
{
    //qCDebug(dcTexasInstruments()) << "--> Movement value" << data.toHex();

    QByteArray payload(data);
    QDataStream stream(&payload, QIODevice::ReadOnly);
    stream.setByteOrder(QDataStream::LittleEndian);

    qint16 gyroXRaw = 0; qint16 gyroYRaw = 0; qint16 gyroZRaw = 0;
    stream >> gyroXRaw >> gyroYRaw >> gyroZRaw;

    qint16 accXRaw = 0; qint16 accYRaw = 0; qint16 accZRaw = 0;
    stream >> accXRaw >> accYRaw >> accZRaw;

    qint16 magXRaw = 0; qint16 magYRaw = 0; qint16 magZRaw = 0;
    stream >> magXRaw >> magYRaw >> magZRaw;


    // Calculate rotation [deg/s], Range +- 250
    float gyroX = static_cast<float>(gyroXRaw) / (65536 / 500);
    float gyroY = static_cast<float>(gyroYRaw) / (65536 / 500);
    float gyroZ = static_cast<float>(gyroZRaw) / (65536 / 500);

    // Calculate acceleration [G], Range +- m_accelerometerRange
    float accX = static_cast<float>(accXRaw)  / (32768 / static_cast<int>(m_accelerometerRange));
    float accY = static_cast<float>(accYRaw)  / (32768 / static_cast<int>(m_accelerometerRange));
    float accZ = static_cast<float>(accZRaw)  / (32768 / static_cast<int>(m_accelerometerRange));

    // Calculate magnetism [uT], Range +- 4900
    float magX = static_cast<float>(magXRaw);
    float magY = static_cast<float>(magYRaw);
    float magZ = static_cast<float>(magZRaw);


    //qCDebug(dcTexasInstruments()) << "Accelerometer x:" << accX << "   y:" << accY << "    z:" << accZ;
    //qCDebug(dcTexasInstruments()) << "Gyroscope     x:" << gyroX << "   y:" << gyroY << "    z:" << gyroZ;
    //qCDebug(dcTexasInstruments()) << "Magnetometer  x:" << magX << "   y:" << magY << "    z:" << magZ;

    QVector3D accelerometerVector(accX, accY, accZ);
    QVector3D gyroscopeVector(gyroX, gyroY, gyroZ);
    QVector3D magnetometerVector(magX, magY, magZ);

    Q_UNUSED(gyroscopeVector)
    Q_UNUSED(magnetometerVector)

    double filteredVectorLength = m_accelerometerFilter->filterValue(accelerometerVector.length());

    // Initialize the accelerometer value if no data known yet
    if (m_lastAccelerometerVectorLenght == -99999) {
        m_lastAccelerometerVectorLenght = filteredVectorLength;
        return;
    }

    double delta = qAbs(qAbs(m_lastAccelerometerVectorLenght) - qAbs(filteredVectorLength));
    bool motionDetected = (delta >= m_movementSensitivity);
    m_thing->setStateValue(sensorTagMovingStateTypeId, motionDetected);
    m_lastAccelerometerVectorLenght = filteredVectorLength;
}

void SensorDataProcessor::reset()
{
    m_lastAccelerometerVectorLenght = -99999;

    // Reset data filters
    m_temperatureFilter->reset();
    m_objectTemperatureFilter->reset();
    m_humidityFilter->reset();
    m_pressureFilter->reset();
    m_opticalFilter->reset();
    m_accelerometerFilter->reset();
}

void SensorDataProcessor::setLeftButtonPressed(bool pressed)
{
    if (m_leftButtonPressed == pressed)
        return;

    qCDebug(dcTexasInstruments()) << "Left button" << (pressed ? "pressed" : "released");
    m_leftButtonPressed = pressed;
    m_thing->setStateValue(sensorTagLeftButtonPressedStateTypeId, m_leftButtonPressed);
}

void SensorDataProcessor::setRightButtonPressed(bool pressed)
{
    if (m_rightButtonPressed == pressed)
        return;

    qCDebug(dcTexasInstruments()) << "Right button" << (pressed ? "pressed" : "released");
    m_rightButtonPressed = pressed;
    m_thing->setStateValue(sensorTagRightButtonPressedStateTypeId, m_rightButtonPressed);
}

void SensorDataProcessor::setMagnetDetected(bool detected)
{
    if (m_magnetDetected == detected)
        return;

    qCDebug(dcTexasInstruments()) << "Magnet detector" << (detected ? "active" : "inactive");
    m_magnetDetected = detected;
    m_thing->setStateValue(sensorTagMagnetDetectedStateTypeId, m_magnetDetected);
}

void SensorDataProcessor::logSensorValue(double originalValue, double filteredValue)
{
    if (!m_filterDebug || !m_logFile)
        return;

    QString logLine = QString("%1 %2 %3\n").arg(QDateTime::currentDateTime().toTime_t()).arg(originalValue).arg(filteredValue);

    QTextStream logStream(m_logFile);
    logStream.setCodec("UTF-8");
    logStream << logLine;
}
