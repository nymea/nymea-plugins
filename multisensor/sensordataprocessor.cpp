#include "sensordataprocessor.h"
#include "extern-plugininfo.h"
#include "math.h"

#include <QVector3D>
#include <QByteArray>
#include <QDataStream>
#include <QDateTime>

SensorDataProcessor::SensorDataProcessor(Device *device, QObject *parent) :
    QObject(parent),
    m_device(device)
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
            qCWarning(dcMultiSensor()) << "Could not open log file" << m_logFile->fileName();
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

    double scaleFactor = 0.03125;
    double objectTemperature = static_cast<double>(rawObjectTemperature) / 4 * scaleFactor;
    double ambientTemperature = static_cast<double>(rawAmbientTemperature) / 4 * scaleFactor;

    //qCDebug(dcMultiSensor()) << "Temperature value" << data.toHex();
    //qCDebug(dcMultiSensor()) << "Object temperature" << roundValue(objectTemperature) << "°C";
    //qCDebug(dcMultiSensor()) << "Ambient temperature" << roundValue(ambientTemperature) << "°C";

    double objectTemperatureFiltered = m_objectTemperatureFilter->filterValue(objectTemperature);
    double ambientTemperatureFiltered = m_temperatureFilter->filterValue(ambientTemperature);

    if (m_objectTemperatureFilter->isReady()) {
        m_device->setStateValue(sensorTagObjectTemperatureStateTypeId, roundValue(objectTemperatureFiltered));
    }

    // Note: only change the state once the filter has collected enought data
    if (m_temperatureFilter->isReady()) {
        m_device->setStateValue(sensorTagTemperatureStateTypeId, roundValue(ambientTemperatureFiltered));
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
        m_device->setStateValue(sensorTagHumidityStateTypeId, roundValue(humidityFiltered));
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
    //qCDebug(dcMultiSensor()) << "Pressure temperature:" << roundValue(rawTemperature / 100.0) << "°C";
    //qCDebug(dcMultiSensor()) << "Pressure:" << roundValue(rawPressure / 100.0) << "mBar";

    double pressureFiltered = m_pressureFilter->filterValue(rawPressure / 100.0);
    if (m_pressureFilter->isReady()) {
        m_device->setStateValue(sensorTagPressureStateTypeId, roundValue(pressureFiltered));
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

    //qCDebug(dcMultiSensor()) << "Lux:" << lux;

    double luxFiltered = m_opticalFilter->filterValue(lux);
    if (m_opticalFilter->isReady()) {
        m_device->setStateValue(sensorTagLightIntensityStateTypeId, qRound(luxFiltered));
    }

    logSensorValue(lux, qRound(luxFiltered));
}

void SensorDataProcessor::processMovementData(const QByteArray &data)
{
    //qCDebug(dcMultiSensor()) << "--> Movement value" << data.toHex();

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
    double gyroX = static_cast<double>(gyroXRaw) / (65536 / 500);
    double gyroY = static_cast<double>(gyroYRaw) / (65536 / 500);
    double gyroZ = static_cast<double>(gyroZRaw) / (65536 / 500);

    // Calculate acceleration [G], Range +- m_accelerometerRange
    double accX = static_cast<double>(accXRaw)  / (32768 / static_cast<int>(m_accelerometerRange));
    double accY = static_cast<double>(accYRaw)  / (32768 / static_cast<int>(m_accelerometerRange));
    double accZ = static_cast<double>(accZRaw)  / (32768 / static_cast<int>(m_accelerometerRange));

    // Calculate magnetism [uT], Range +- 4900
    double magX = static_cast<double>(magXRaw);
    double magY = static_cast<double>(magYRaw);
    double magZ = static_cast<double>(magZRaw);


    //qCDebug(dcMultiSensor()) << "Accelerometer x:" << accX << "   y:" << accY << "    z:" << accZ;
    //qCDebug(dcMultiSensor()) << "Gyroscope     x:" << gyroX << "   y:" << gyroY << "    z:" << gyroZ;
    //qCDebug(dcMultiSensor()) << "Magnetometer  x:" << magX << "   y:" << magY << "    z:" << magZ;

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
    m_device->setStateValue(sensorTagMovingStateTypeId, motionDetected);
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

    qCDebug(dcMultiSensor()) << "Left button" << (pressed ? "pressed" : "released");
    m_leftButtonPressed = pressed;
    m_device->setStateValue(sensorTagLeftButtonPressedStateTypeId, m_leftButtonPressed);
}

void SensorDataProcessor::setRightButtonPressed(bool pressed)
{
    if (m_rightButtonPressed == pressed)
        return;

    qCDebug(dcMultiSensor()) << "Right button" << (pressed ? "pressed" : "released");
    m_rightButtonPressed = pressed;
    m_device->setStateValue(sensorTagRightButtonPressedStateTypeId, m_rightButtonPressed);
}

void SensorDataProcessor::setMagnetDetected(bool detected)
{
    if (m_magnetDetected == detected)
        return;

    qCDebug(dcMultiSensor()) << "Magnet detector" << (detected ? "active" : "inactive");
    m_magnetDetected = detected;
    m_device->setStateValue(sensorTagMagnetDetectedStateTypeId, m_magnetDetected);
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
