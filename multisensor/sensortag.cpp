/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *                                                                         *
 *  Copyright (C) 2015-2018 Simon Stuerz <simon.stuerz@guh.io>             *
 *                                                                         *
 *  This file is part of guh.                                              *
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

#include "sensortag.h"
#include "extern-plugininfo.h"
#include "math.h"

#include <QTimer>
#include <QVector3D>
#include <QDataStream>

SensorTag::SensorTag(Device *device, BluetoothLowEnergyDevice *bluetoothDevice, QObject *parent) :
    QObject(parent),
    m_device(device),
    m_bluetoothDevice(bluetoothDevice)
{
    connect(m_bluetoothDevice, &BluetoothLowEnergyDevice::connectedChanged, this, &SensorTag::onConnectedChanged);
    connect(m_bluetoothDevice, &BluetoothLowEnergyDevice::servicesDiscoveryFinished, this, &SensorTag::onServiceDiscoveryFinished);
}

Device *SensorTag::device()
{
    return m_device;
}

BluetoothLowEnergyDevice *SensorTag::bluetoothDevice()
{
    return m_bluetoothDevice;
}

void SensorTag::setAccelerometerEnabled(bool enabled)
{
    qCDebug(dcMultiSensor()) << "Accelerometer" << (enabled ? "enabled" : "disabled");

    if (m_accelerometerEnabled == enabled)
        return;

    m_accelerometerEnabled = enabled;
    configureMovement();
}

void SensorTag::setAccelerometerRange(const SensorTag::SensorAccelerometerRange &range)
{
    qCDebug(dcMultiSensor()) << "Accelerometer" << range;

    if (m_accelerometerRange == range)
        return;

    m_accelerometerRange = range;
    configureMovement();
}

void SensorTag::setGyroscopeEnabled(bool enabled)
{
    qCDebug(dcMultiSensor()) << "Gyroscope" << (enabled ? "enabled" : "disabled");

    if (m_gyroscopeEnabled == enabled)
        return;

    m_gyroscopeEnabled = enabled;
    configureMovement();
}

void SensorTag::setMagnetometerEnabled(bool enabled)
{
    qCDebug(dcMultiSensor()) << "Magnetometer" << (enabled ? "enabled" : "disabled");

    if (m_magnetometerEnabled == enabled)
        return;

    m_magnetometerEnabled = enabled;
    configureMovement();
}

void SensorTag::setMeasurementPeriod(int period)
{
    qCDebug(dcMultiSensor()) << "Set sensor measurement period to" << period << "ms";

    if (period % 10 != 0) {
        int adjustedValue = qRound(static_cast<float>(period) / 10.0) * 10;
        qCWarning(dcMultiSensor()) << "Measurement period of sensors" << period << "must be a multiple of 10ms. Adjusting it to" << adjustedValue;
        period = adjustedValue;
    }

    m_temperaturePeriod = period;
    if (m_temperatureService && m_temperaturePeriodCharacteristic.isValid())
        configurePeriod(m_temperatureService, m_temperaturePeriodCharacteristic, m_temperaturePeriod);

    m_humidityPeriod = period;
    if (m_humidityService && m_humidityPeriodCharacteristic.isValid())
        configurePeriod(m_humidityService, m_humidityPeriodCharacteristic, m_humidityPeriod);

    m_pressurePeriod = period;
    if (m_pressureService && m_pressurePeriodCharacteristic.isValid())
        configurePeriod(m_pressureService, m_pressurePeriodCharacteristic, m_pressurePeriod);

    m_opticalPeriod = period;
    if (m_opticalService && m_opticalPeriodCharacteristic.isValid())
        configurePeriod(m_opticalService, m_opticalPeriodCharacteristic, m_opticalPeriod);

}

void SensorTag::setMeasurementPeriodMovement(int period)
{
    qCDebug(dcMultiSensor()) << "Set movement sensor measurement period to" << period << "ms";

    if (period % 10 != 0) {
        int adjustedValue = qRound(static_cast<float>(period) / 10.0) * 10;
        qCWarning(dcMultiSensor()) << "Measurement period of movement sensor" << period << "must be a multiple of 10ms. Adjusting it to" << adjustedValue;
        period = adjustedValue;
    }

    m_movementPeriod = period;
    if (m_movementService && m_movementPeriodCharacteristic.isValid())
        configurePeriod(m_movementService, m_movementPeriodCharacteristic, m_movementPeriod);

}

void SensorTag::setMovementSensitivity(int percentage)
{
    m_movementSensitivity = static_cast<double>(percentage) / 100.0;
}

void SensorTag::setGreenLedPower(bool power)
{
    m_greenLedEnabled = power;
    qCDebug(dcMultiSensor()) << "Green LED" << (power ? "enabled" : "disabled");
    configureIo();
    m_device->setStateValue(sensortagGreenLedStateTypeId, m_greenLedEnabled);
}

void SensorTag::setRedLedPower(bool power)
{
    m_redLedEnabled = power;
    qCDebug(dcMultiSensor()) << "Red LED" << (power ? "enabled" : "disabled");
    configureIo();
    m_device->setStateValue(sensortagRedLedStateTypeId, m_redLedEnabled);
}

void SensorTag::setBuzzerPower(bool power)
{
    m_buzzerEnabled = power;
    qCDebug(dcMultiSensor()) << "Buzzer" << (power ? "enabled" : "disabled");
    configureIo();
    m_device->setStateValue(sensortagBuzzerStateTypeId, m_buzzerEnabled);
}

void SensorTag::buzzerImpulse()
{
    qCDebug(dcMultiSensor()) << "Buzzer impulse";
    setBuzzerPower(true);
    QTimer::singleShot(1000, this, &SensorTag::onBuzzerImpulseTimeout);
}

void SensorTag::configurePeriod(QLowEnergyService *serice, const QLowEnergyCharacteristic &characteristic, int measurementPeriod)
{
    Q_ASSERT(measurementPeriod % 10 == 0);
    QByteArray payload;
    QDataStream stream(&payload, QIODevice::WriteOnly);
    stream << static_cast<quint8>(measurementPeriod / 10);

    qCDebug(dcMultiSensor()) << "Configure period to" << measurementPeriod << payload.toHex();
    serice->writeCharacteristic(characteristic, payload);
}

void SensorTag::configureMovement()
{
    if (!m_movementService || !m_movementConfigurationCharacteristic.isValid())
        return;

    quint16 configuration = 0;
    if (m_gyroscopeEnabled) {
        configuration |= (1 << 0); // enable x-axis
        configuration |= (1 << 1); // enable y-axis
        configuration |= (1 << 2); // enable z-axis
    }

    if (m_accelerometerEnabled) {
        configuration |= (1 << 3); // enable x-axis
        configuration |= (1 << 4); // enable y-axis
        configuration |= (1 << 5); // enable z-axis
    }

    if (m_magnetometerEnabled) {
        configuration |= (1 << 6); // enable all axis
    }

    // Always enable wake on movement in order to save energy
    configuration |= (1 << 8); // enable

    // Accelerometer range 2 Bit ( 0 = 2G, 1 = 4G, 2 = 8G, 3 = 16G)
    switch (m_accelerometerRange) {
    case SensorAccelerometerRange2G:
        // Bit 9 = 0
        // Bit 10 = 0
        break;
    case SensorAccelerometerRange4G:
        configuration |= (1 << 11);
        // Bit 12 = 0
        break;
    case SensorAccelerometerRange8G:
        // Bit 13 = 0
        configuration |= (1 << 14);
        break;
    case SensorAccelerometerRange16G:
        configuration |= (1 << 15);
        configuration |= (1 << 16);
        break;
    default:
        break;
    }

    QByteArray data;
    QDataStream stream(&data, QIODevice::WriteOnly);
    stream.setByteOrder(QDataStream::LittleEndian);
    stream << configuration;

    qCDebug(dcMultiSensor()) << "Configure movement sensor" << data.toHex();
    m_movementService->writeCharacteristic(m_movementConfigurationCharacteristic, data);
}

void SensorTag::configureSensorMode(const SensorTag::SensorMode &mode)
{
    if (!m_ioService || !m_ioDataCharacteristic.isValid())
        return;

    qCDebug(dcMultiSensor()) << "Setting" << mode;
    QByteArray data;
    QDataStream stream(&data, QIODevice::WriteOnly);
    stream.setByteOrder(QDataStream::LittleEndian);
    stream << static_cast<quint8>(mode);

    m_ioService->writeCharacteristic(m_ioConfigurationCharacteristic, data);
}

void SensorTag::configureIo()
{
    if (!m_ioService || !m_ioDataCharacteristic.isValid())
        return;

    // Write value to IO
    quint8 configuration = 0;
    if (m_redLedEnabled)
        configuration |= (1 << 0); // Red LED

    if (m_greenLedEnabled)
        configuration |= (1 << 1); // Green LED

    if (m_buzzerEnabled)
        configuration |= (1 << 2); // Buzzer

    QByteArray payload;
    QDataStream stream(&payload, QIODevice::WriteOnly);
    stream << configuration;

    m_ioService->writeCharacteristic(m_ioDataCharacteristic, payload);
}

void SensorTag::processTemperatureData(const QByteArray &data)
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

    //qCDebug(dcMultiSensor()) << "Temperature value" << data.toHex();
    //qCDebug(dcMultiSensor()) << "Object temperature" << roundValue(objectTemperature) << "°C";
    //qCDebug(dcMultiSensor()) << "Ambient temperature" << roundValue(ambientTemperature) << "°C";

    m_device->setStateValue(sensortagObjectTemperatureStateTypeId, roundValue(objectTemperature));
    m_device->setStateValue(sensortagTemperatureStateTypeId, roundValue(ambientTemperature));
}

void SensorTag::processKeyData(const QByteArray &data)
{
    Q_ASSERT(data.count() == 1);
    quint8 flags = static_cast<quint8>(data.at(0));
    setLeftButtonPressed(testBitUint8(flags, 0));
    setRightButtonPressed(testBitUint8(flags, 1));
    setMagnetDetected(testBitUint8(flags, 2));
}

void SensorTag::processHumidityData(const QByteArray &data)
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

    //qCDebug(dcMultiSensor()) << "Humidity" << humidity << "%" << humidityTemperature << "°C";
    m_device->setStateValue(sensortagHumidityStateTypeId, roundValue(humidity));
}

void SensorTag::processPressureData(const QByteArray &data)
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
    m_device->setStateValue(sensortagPressureStateTypeId, roundValue(rawPressure / 100.0));
}

void SensorTag::processOpticalData(const QByteArray &data)
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
    device()->setStateValue(sensortagLightIntensityStateTypeId, roundValue(lux));
}

void SensorTag::processMovementData(const QByteArray &data)
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

    // Initialize the accelerometer value if no data known yet
    if (m_lastAccelerometerVectorLenght == -99999) {
        m_lastAccelerometerVectorLenght = accelerometerVector.length();
        return;
    }


    double delta = qAbs(qAbs(m_lastAccelerometerVectorLenght) - qAbs(accelerometerVector.length()));
    bool motionDetected = (delta >= m_movementSensitivity);
    //qCDebug(dcMultiSensor()) <<  accelerometerVector.length() << "  |  " << delta << m_movementSensitivity << (motionDetected ? "motion" : "-");
    m_device->setStateValue(sensortagMovingStateTypeId, motionDetected);
    m_lastAccelerometerVectorLenght = accelerometerVector.length();
}

void SensorTag::setLeftButtonPressed(bool pressed)
{
    if (m_leftButtonPressed == pressed)
        return;

    qCDebug(dcMultiSensor()) << "Left button" << (pressed ? "pressed" : "released");
    m_leftButtonPressed = pressed;
    emit leftButtonPressedChainged(m_leftButtonPressed);
    m_device->setStateValue(sensortagLeftButtonPressedStateTypeId, m_leftButtonPressed);
}

void SensorTag::setRightButtonPressed(bool pressed)
{
    if (m_rightButtonPressed == pressed)
        return;

    qCDebug(dcMultiSensor()) << "Right button" << (pressed ? "pressed" : "released");
    m_rightButtonPressed = pressed;
    emit rightButtonPressedChainged(m_rightButtonPressed);
    m_device->setStateValue(sensortagRightButtonPressedStateTypeId, m_rightButtonPressed);
}

void SensorTag::setMagnetDetected(bool detected)
{
    if (m_magnetDetected == detected)
        return;

    qCDebug(dcMultiSensor()) << "Magnet detector" << (detected ? "active" : "inactive");
    m_magnetDetected = detected;
    emit magnetDetectedChainged(m_magnetDetected);
    m_device->setStateValue(sensortagMagnetDetectedStateTypeId, m_magnetDetected);
}

bool SensorTag::testBitUint8(quint8 value, int bitPosition)
{
    return (((value)>> (bitPosition)) & 1);
}

double SensorTag::roundValue(float value)
{
    int tmpValue = static_cast<int>(value * 10);
    return static_cast<double>(tmpValue) / 10.0;
}

void SensorTag::onConnectedChanged(const bool &connected)
{
    qCDebug(dcMultiSensor()) << "Sensor" << m_bluetoothDevice->name() << m_bluetoothDevice->address().toString() << (connected ? "connected" : "disconnected");
    m_device->setStateValue(sensortagConnectedStateTypeId, connected);

    if (!connected) {
        // Clean up services
        m_temperatureService->deleteLater();
        m_humidityService->deleteLater();
        m_pressureService->deleteLater();
        m_opticalService->deleteLater();
        m_keyService->deleteLater();
        m_movementService->deleteLater();
        m_ioService->deleteLater();

        m_temperatureService = nullptr;
        m_humidityService = nullptr;
        m_pressureService = nullptr;
        m_opticalService = nullptr;
        m_keyService = nullptr;
        m_movementService = nullptr;
        m_ioService = nullptr;

        m_lastAccelerometerVectorLenght = -99999;
    }
}

void SensorTag::onServiceDiscoveryFinished()
{
    foreach (const QBluetoothUuid serviceUuid, m_bluetoothDevice->serviceUuids()) {
        qCDebug(dcMultiSensor()) << "-->" << serviceUuid;
    }

    if (!m_bluetoothDevice->serviceUuids().contains(temperatureServiceUuid)) {
        qCWarning(dcMultiSensor()) << "Could not find temperature service";
        m_bluetoothDevice->disconnectDevice();
        return;
    }

    if (!m_bluetoothDevice->serviceUuids().contains(humidityServiceUuid)) {
        qCWarning(dcMultiSensor()) << "Could not find humidity service";
        m_bluetoothDevice->disconnectDevice();
        return;
    }

    if (!m_bluetoothDevice->serviceUuids().contains(pressureServiceUuid)) {
        qCWarning(dcMultiSensor()) << "Could not find pressure service";
        m_bluetoothDevice->disconnectDevice();
        return;
    }

    if (!m_bluetoothDevice->serviceUuids().contains(opticalServiceUuid)) {
        qCWarning(dcMultiSensor()) << "Could not find optical service";
        m_bluetoothDevice->disconnectDevice();
        return;
    }

    if (!m_bluetoothDevice->serviceUuids().contains(keyServiceUuid)) {
        qCWarning(dcMultiSensor()) << "Could not find key service";
        m_bluetoothDevice->disconnectDevice();
        return;
    }

    if (!m_bluetoothDevice->serviceUuids().contains(movementServiceUuid)) {
        qCWarning(dcMultiSensor()) << "Could not find movement service";
        m_bluetoothDevice->disconnectDevice();
        return;
    }

    if (!m_bluetoothDevice->serviceUuids().contains(ioServiceUuid)) {
        qCWarning(dcMultiSensor()) << "Could not find IO service";
        m_bluetoothDevice->disconnectDevice();
        return;
    }

    // IR Temperature
    if (!m_temperatureService) {
        m_temperatureService = m_bluetoothDevice->controller()->createServiceObject(temperatureServiceUuid, this);
        if (!m_temperatureService) {
            qCWarning(dcMultiSensor()) << "Could not create temperature service.";
            m_bluetoothDevice->disconnectDevice();
            return;
        }

        connect(m_temperatureService, &QLowEnergyService::stateChanged, this, &SensorTag::onTemperatureServiceStateChanged);
        connect(m_temperatureService, &QLowEnergyService::characteristicChanged, this, &SensorTag::onTemperatureServiceCharacteristicChanged);

        if (m_temperatureService->state() == QLowEnergyService::DiscoveryRequired) {
            m_temperatureService->discoverDetails();
        }
    }

    // Humidity
    if (!m_humidityService) {
        m_humidityService = m_bluetoothDevice->controller()->createServiceObject(humidityServiceUuid, this);
        if (!m_humidityService) {
            qCWarning(dcMultiSensor()) << "Could not create humidity service.";
            m_bluetoothDevice->disconnectDevice();
            return;
        }

        connect(m_humidityService, &QLowEnergyService::stateChanged, this, &SensorTag::onHumidityServiceStateChanged);
        connect(m_humidityService, &QLowEnergyService::characteristicChanged, this, &SensorTag::onHumidityServiceCharacteristicChanged);
        if (m_humidityService->state() == QLowEnergyService::DiscoveryRequired) {
            m_humidityService->discoverDetails();
        }
    }

    // Pressure
    if (!m_pressureService) {
        m_pressureService = m_bluetoothDevice->controller()->createServiceObject(pressureServiceUuid, this);
        if (!m_pressureService) {
            qCWarning(dcMultiSensor()) << "Could not create pressure service.";
            m_bluetoothDevice->disconnectDevice();
            return;
        }

        connect(m_pressureService, &QLowEnergyService::stateChanged, this, &SensorTag::onPressureServiceStateChanged);
        connect(m_pressureService, &QLowEnergyService::characteristicChanged, this, &SensorTag::onPressureServiceCharacteristicChanged);
        if (m_pressureService->state() == QLowEnergyService::DiscoveryRequired) {
            m_pressureService->discoverDetails();
        }
    }


    // Optical
    if (!m_opticalService) {
        m_opticalService = m_bluetoothDevice->controller()->createServiceObject(opticalServiceUuid, this);
        if (!m_opticalService) {
            qCWarning(dcMultiSensor()) << "Could not create optical service.";
            m_bluetoothDevice->disconnectDevice();
            return;
        }

        connect(m_opticalService, &QLowEnergyService::stateChanged, this, &SensorTag::onOpticalServiceStateChanged);
        connect(m_opticalService, &QLowEnergyService::characteristicChanged, this, &SensorTag::onOpticalServiceCharacteristicChanged);

        if (m_opticalService->state() == QLowEnergyService::DiscoveryRequired) {
            m_opticalService->discoverDetails();
        }
    }

    // Key
    if (!m_keyService) {
        m_keyService = m_bluetoothDevice->controller()->createServiceObject(keyServiceUuid, this);
        if (!m_keyService) {
            qCWarning(dcMultiSensor()) << "Could not create key service.";
            m_bluetoothDevice->disconnectDevice();
            return;
        }

        connect(m_keyService, &QLowEnergyService::stateChanged, this, &SensorTag::onKeyServiceStateChanged);
        connect(m_keyService, &QLowEnergyService::characteristicChanged, this, &SensorTag::onKeyServiceCharacteristicChanged);

        if (m_keyService->state() == QLowEnergyService::DiscoveryRequired) {
            m_keyService->discoverDetails();
        }
    }

    // Movement
    if (!m_movementService) {
        m_movementService = m_bluetoothDevice->controller()->createServiceObject(movementServiceUuid, this);
        if (!m_movementService) {
            qCWarning(dcMultiSensor()) << "Could not create movement service.";
            m_bluetoothDevice->disconnectDevice();
            return;
        }

        connect(m_movementService, &QLowEnergyService::stateChanged, this, &SensorTag::onMovementServiceStateChanged);
        connect(m_movementService, &QLowEnergyService::characteristicChanged, this, &SensorTag::onMovementServiceCharacteristicChanged);

        if (m_movementService->state() == QLowEnergyService::DiscoveryRequired) {
            m_movementService->discoverDetails();
        }
    }

    // IO
    if (!m_ioService) {
        m_ioService = m_bluetoothDevice->controller()->createServiceObject(ioServiceUuid, this);
        if (!m_ioService) {
            qCWarning(dcMultiSensor()) << "Could not create IO service.";
            m_bluetoothDevice->disconnectDevice();
            return;
        }

        connect(m_ioService, &QLowEnergyService::stateChanged, this, &SensorTag::onIoServiceStateChanged);
        connect(m_ioService, &QLowEnergyService::characteristicChanged, this, &SensorTag::onIoServiceCharacteristicChanged);

        if (m_ioService->state() == QLowEnergyService::DiscoveryRequired) {
            m_ioService->discoverDetails();
        }
    }
}

void SensorTag::onTemperatureServiceStateChanged(const QLowEnergyService::ServiceState &state)
{
    // Only continue if discovered
    if (state != QLowEnergyService::ServiceDiscovered)
        return;

    qCDebug(dcMultiSensor()) << "Temperature sensor service discovered.";

    foreach (const QLowEnergyCharacteristic &characteristic, m_temperatureService->characteristics()) {
        qCDebug(dcMultiSensor()) << "    -->" << characteristic.name() << characteristic.uuid().toString() << characteristic.value();
        foreach (const QLowEnergyDescriptor &desciptor, characteristic.descriptors()) {
            qCDebug(dcMultiSensor()) << "        -->" << desciptor.name() << desciptor.uuid().toString() << desciptor.value();
        }
    }

    // Data characteristic
    m_temperatureDataCharacteristic = m_temperatureService->characteristic(QBluetoothUuid(QUuid("f000aa01-0451-4000-b000-000000000000")));
    if (!m_temperatureDataCharacteristic.isValid()) {
        qCWarning(dcMultiSensor()) << "Invalid temperature data characteristic.";
        m_bluetoothDevice->disconnectDevice();
        return;
    }

    // Enable notifications
    QLowEnergyDescriptor notificationDescriptor = m_temperatureDataCharacteristic.descriptor(QBluetoothUuid::ClientCharacteristicConfiguration);
    m_temperatureService->writeDescriptor(notificationDescriptor, QByteArray::fromHex("0100"));


    // Config characteristic
    m_temperatureConfigurationCharacteristic = m_temperatureService->characteristic(temperatureConfigurationCharacteristicUuid);
    if (!m_temperatureConfigurationCharacteristic.isValid()) {
        qCWarning(dcMultiSensor()) << "Invalid temperature configuration characteristic.";
        m_bluetoothDevice->disconnectDevice();
        return;
    }

    // Period characteristic
    m_temperaturePeriodCharacteristic = m_temperatureService->characteristic(temperaturePeriodCharacteristicUuid);
    if (!m_temperaturePeriodCharacteristic.isValid()) {
        qCWarning(dcMultiSensor()) << "Invalid temperature period characteristic.";
        m_bluetoothDevice->disconnectDevice();
        return;
    }

    configurePeriod(m_temperatureService, m_temperaturePeriodCharacteristic, m_temperaturePeriod);

    // Enable measuring
    m_temperatureService->writeCharacteristic(m_temperatureConfigurationCharacteristic, QByteArray::fromHex("01"));
}

void SensorTag::onTemperatureServiceCharacteristicChanged(const QLowEnergyCharacteristic &characteristic, const QByteArray &value)
{
    if (characteristic == m_temperatureDataCharacteristic) {
        processTemperatureData(value);

        // FIXME: Disable measuring
        // m_temperatureService->writeCharacteristic(m_temperatureConfigCharacteristic, QByteArray::fromHex("00"));
    }
}

void SensorTag::onHumidityServiceStateChanged(const QLowEnergyService::ServiceState &state)
{
    // Only continue if discovered
    if (state != QLowEnergyService::ServiceDiscovered)
        return;

    qCDebug(dcMultiSensor()) << "Humidity sensor service discovered.";

    foreach (const QLowEnergyCharacteristic &characteristic, m_humidityService->characteristics()) {
        qCDebug(dcMultiSensor()) << "    -->" << characteristic.name() << characteristic.uuid().toString() << characteristic.value();
        foreach (const QLowEnergyDescriptor &desciptor, characteristic.descriptors()) {
            qCDebug(dcMultiSensor()) << "        -->" << desciptor.name() << desciptor.uuid().toString() << desciptor.value();
        }
    }

    // Data characteristic
    m_humidityDataCharacteristic = m_humidityService->characteristic(humidityDataCharacteristicUuid);
    if (!m_humidityDataCharacteristic.isValid()) {
        qCWarning(dcMultiSensor()) << "Invalid humidity data characteristic.";
        m_bluetoothDevice->disconnectDevice();
        return;
    }

    // Enable notifications
    QLowEnergyDescriptor notificationDescriptor = m_humidityDataCharacteristic.descriptor(QBluetoothUuid::ClientCharacteristicConfiguration);
    m_humidityService->writeDescriptor(notificationDescriptor, QByteArray::fromHex("0100"));

    // Config characteristic
    m_humidityConfigurationCharacteristic = m_humidityService->characteristic(humidityConfigurationCharacteristicUuid);
    if (!m_humidityConfigurationCharacteristic.isValid()) {
        qCWarning(dcMultiSensor()) << "Invalid humidity configuration characteristic.";
        m_bluetoothDevice->disconnectDevice();
        return;
    }

    // Period characteristic
    m_humidityPeriodCharacteristic = m_humidityService->characteristic(humidityPeriodCharacteristicUuid);
    if (!m_humidityPeriodCharacteristic.isValid()) {
        qCWarning(dcMultiSensor()) << "Invalid humidity period characteristic.";
        m_bluetoothDevice->disconnectDevice();
        return;
    }

    configurePeriod(m_humidityService, m_humidityPeriodCharacteristic, m_humidityPeriod);

    // Enable measuring
    m_humidityService->writeCharacteristic(m_humidityConfigurationCharacteristic, QByteArray::fromHex("01"));
}

void SensorTag::onHumidityServiceCharacteristicChanged(const QLowEnergyCharacteristic &characteristic, const QByteArray &value)
{
    if (characteristic == m_humidityDataCharacteristic) {
        processHumidityData(value);
        // FIXME: Disable measuring
        // m_humidityService->writeCharacteristic(m_humidityConfigCharacteristic, QByteArray::fromHex("00"));
    }
}

void SensorTag::onPressureServiceStateChanged(const QLowEnergyService::ServiceState &state)
{
    // Only continue if discovered
    if (state != QLowEnergyService::ServiceDiscovered)
        return;

    qCDebug(dcMultiSensor()) << "Pressure sensor service discovered.";

    foreach (const QLowEnergyCharacteristic &characteristic, m_pressureService->characteristics()) {
        qCDebug(dcMultiSensor()) << "    -->" << characteristic.name() << characteristic.uuid().toString() << characteristic.value();
        foreach (const QLowEnergyDescriptor &desciptor, characteristic.descriptors()) {
            qCDebug(dcMultiSensor()) << "        -->" << desciptor.name() << desciptor.uuid().toString() << desciptor.value();
        }
    }

    // Data characteristic
    m_pressureDataCharacteristic = m_pressureService->characteristic(pressureDataCharacteristicUuid);
    if (!m_pressureDataCharacteristic.isValid()) {
        qCWarning(dcMultiSensor()) << "Invalid pressure data characteristic.";
        m_bluetoothDevice->disconnectDevice();
        return;
    }

    // Enable notifications
    QLowEnergyDescriptor notificationDescriptor = m_pressureDataCharacteristic.descriptor(QBluetoothUuid::ClientCharacteristicConfiguration);
    m_pressureService->writeDescriptor(notificationDescriptor, QByteArray::fromHex("0100"));

    // Config characteristic
    m_pressureConfigurationCharacteristic = m_pressureService->characteristic(pressureConfigurationCharacteristicUuid);
    if (!m_pressureConfigurationCharacteristic.isValid()) {
        qCWarning(dcMultiSensor()) << "Invalid pressure configuration characteristic.";
        m_bluetoothDevice->disconnectDevice();
        return;
    }

    // Period characteristic
    m_pressurePeriodCharacteristic = m_pressureService->characteristic(pressurePeriodCharacteristicUuid);
    if (!m_pressurePeriodCharacteristic.isValid()) {
        qCWarning(dcMultiSensor()) << "Invalid pressure period characteristic.";
        m_bluetoothDevice->disconnectDevice();
        return;
    }

    configurePeriod(m_pressureService, m_pressurePeriodCharacteristic, m_pressurePeriod);

    // Enable measuring
    m_pressureService->writeCharacteristic(m_pressureConfigurationCharacteristic, QByteArray::fromHex("01"));
}

void SensorTag::onPressureServiceCharacteristicChanged(const QLowEnergyCharacteristic &characteristic, const QByteArray &value)
{
    if (characteristic == m_pressureDataCharacteristic) {
        processPressureData(value);
        // FIXME: Disable measuring
        // m_pressureService->writeCharacteristic(m_pressureConfigCharacteristic, QByteArray::fromHex("00"));
    }
}

void SensorTag::onOpticalServiceStateChanged(const QLowEnergyService::ServiceState &state)
{
    // Only continue if discovered
    if (state != QLowEnergyService::ServiceDiscovered)
        return;

    qCDebug(dcMultiSensor()) << "Optical sensor service discovered.";

    foreach (const QLowEnergyCharacteristic &characteristic, m_pressureService->characteristics()) {
        qCDebug(dcMultiSensor()) << "    -->" << characteristic.name() << characteristic.uuid().toString() << characteristic.value();
        foreach (const QLowEnergyDescriptor &desciptor, characteristic.descriptors()) {
            qCDebug(dcMultiSensor()) << "        -->" << desciptor.name() << desciptor.uuid().toString() << desciptor.value();
        }
    }

    // Data characteristic
    m_opticalDataCharacteristic = m_opticalService->characteristic(opticalDataCharacteristicUuid);
    if (!m_opticalDataCharacteristic.isValid()) {
        qCWarning(dcMultiSensor()) << "Invalid optical data characteristic.";
        m_bluetoothDevice->disconnectDevice();
        return;
    }

    // Enable notifications
    QLowEnergyDescriptor notificationDescriptor = m_opticalDataCharacteristic.descriptor(QBluetoothUuid::ClientCharacteristicConfiguration);
    m_opticalService->writeDescriptor(notificationDescriptor, QByteArray::fromHex("0100"));

    // Config characteristic
    m_opticalConfigurationCharacteristic = m_opticalService->characteristic(opticalConfigurationCharacteristicUuid);
    if (!m_opticalConfigurationCharacteristic.isValid()) {
        qCWarning(dcMultiSensor()) << "Invalid optical configuration characteristic.";
        m_bluetoothDevice->disconnectDevice();
    }

    // Period characteristic
    m_opticalPeriodCharacteristic = m_opticalService->characteristic(opticalPeriodCharacteristicUuid);
    if (!m_opticalPeriodCharacteristic.isValid()) {
        qCWarning(dcMultiSensor()) << "Invalid optical period characteristic.";
        m_bluetoothDevice->disconnectDevice();
    }

    // Set measurement period
    configurePeriod(m_opticalService, m_opticalPeriodCharacteristic, m_opticalPeriod);

    // Enable measuring
    m_opticalService->writeCharacteristic(m_opticalConfigurationCharacteristic, QByteArray::fromHex("01"));
}

void SensorTag::onOpticalServiceCharacteristicChanged(const QLowEnergyCharacteristic &characteristic, const QByteArray &value)
{
    if (characteristic == m_opticalDataCharacteristic) {
        processOpticalData(value);
        // FIXME: Disable measuring
        // m_opticalService->writeCharacteristic(m_pressureopticalCharacteristic, QByteArray::fromHex("00"));
    }

}

void SensorTag::onKeyServiceStateChanged(const QLowEnergyService::ServiceState &state)
{
    // Only continue if discovered
    if (state != QLowEnergyService::ServiceDiscovered)
        return;

    qCDebug(dcMultiSensor()) << "Key service discovered.";
    foreach (const QLowEnergyCharacteristic &characteristic, m_keyService->characteristics()) {
        qCDebug(dcMultiSensor()) << "    -->" << characteristic.name() << characteristic.uuid().toString() << characteristic.value();
        foreach (const QLowEnergyDescriptor &desciptor, characteristic.descriptors()) {
            qCDebug(dcMultiSensor()) << "        -->" << desciptor.name() << desciptor.uuid().toString() << desciptor.value();
        }
    }

    // Data characteristic
    m_keyDataCharacteristic = m_keyService->characteristic(keyDataCharacteristicUuid);
    if (!m_keyDataCharacteristic.isValid()) {
        qCWarning(dcMultiSensor()) << "Invalid button data characteristic.";
        m_bluetoothDevice->disconnectDevice();
    }

    // Enable notifications
    QLowEnergyDescriptor notificationDescriptor = m_keyDataCharacteristic.descriptor(QBluetoothUuid::ClientCharacteristicConfiguration);
    m_keyService->writeDescriptor(notificationDescriptor, QByteArray::fromHex("0100"));
}

void SensorTag::onKeyServiceCharacteristicChanged(const QLowEnergyCharacteristic &characteristic, const QByteArray &value)
{
    if (characteristic == m_keyDataCharacteristic) {
        processKeyData(value);
    }
}

void SensorTag::onMovementServiceStateChanged(const QLowEnergyService::ServiceState &state)
{
    // Only continue if discovered
    if (state != QLowEnergyService::ServiceDiscovered)
        return;

    qCDebug(dcMultiSensor()) << "Movement sensor service discovered.";

    foreach (const QLowEnergyCharacteristic &characteristic, m_pressureService->characteristics()) {
        qCDebug(dcMultiSensor()) << "    -->" << characteristic.name() << characteristic.uuid().toString() << characteristic.value();
        foreach (const QLowEnergyDescriptor &desciptor, characteristic.descriptors()) {
            qCDebug(dcMultiSensor()) << "        -->" << desciptor.name() << desciptor.uuid().toString() << desciptor.value();
        }
    }

    // Data characteristic
    m_movementDataCharacteristic = m_movementService->characteristic(movementDataCharacteristicUuid);
    if (!m_movementDataCharacteristic.isValid()) {
        qCWarning(dcMultiSensor()) << "Invalid movement data characteristic.";
        m_bluetoothDevice->disconnectDevice();
    }

    // Enable notifications
    QLowEnergyDescriptor notificationDescriptor = m_movementDataCharacteristic.descriptor(QBluetoothUuid::ClientCharacteristicConfiguration);
    m_movementService->writeDescriptor(notificationDescriptor, QByteArray::fromHex("0100"));

    // Config characteristic
    m_movementConfigurationCharacteristic = m_movementService->characteristic(movementConfigurationCharacteristicUuid);
    if (!m_movementConfigurationCharacteristic.isValid()) {
        qCWarning(dcMultiSensor()) << "Invalid movement configuration characteristic.";
        m_bluetoothDevice->disconnectDevice();
    }

    // Period characteristic
    m_movementPeriodCharacteristic = m_movementService->characteristic(movementPeriodCharacteristicUuid);
    if (!m_movementPeriodCharacteristic.isValid()) {
        qCWarning(dcMultiSensor()) << "Invalid movement period characteristic.";
        m_bluetoothDevice->disconnectDevice();
    }

    // Set measurement period
    configurePeriod(m_movementService, m_movementPeriodCharacteristic, m_movementPeriod);
    configureMovement();

    // Enable measuring
    m_movementService->writeCharacteristic(m_movementConfigurationCharacteristic, QByteArray::fromHex("01"));
}

void SensorTag::onMovementServiceCharacteristicChanged(const QLowEnergyCharacteristic &characteristic, const QByteArray &value)
{
    if (characteristic == m_movementDataCharacteristic) {
        processMovementData(value);
    }
}

void SensorTag::onIoServiceStateChanged(const QLowEnergyService::ServiceState &state)
{
    // Only continue if discovered
    if (state != QLowEnergyService::ServiceDiscovered)
        return;

    qCDebug(dcMultiSensor()) << "IO service discovered.";

    foreach (const QLowEnergyCharacteristic &characteristic, m_pressureService->characteristics()) {
        qCDebug(dcMultiSensor()) << "    -->" << characteristic.name() << characteristic.uuid().toString() << characteristic.value();
        foreach (const QLowEnergyDescriptor &desciptor, characteristic.descriptors()) {
            qCDebug(dcMultiSensor()) << "        -->" << desciptor.name() << desciptor.uuid().toString() << desciptor.value();
        }
    }

    // Data characteristic
    m_ioDataCharacteristic = m_ioService->characteristic(ioDataCharacteristicUuid);
    if (!m_ioDataCharacteristic.isValid()) {
        qCWarning(dcMultiSensor()) << "Invalid IO data characteristic.";
        m_bluetoothDevice->disconnectDevice();
    }

    // Enable notifications
    QLowEnergyDescriptor notificationDescriptor = m_ioDataCharacteristic.descriptor(QBluetoothUuid::ClientCharacteristicConfiguration);
    m_ioService->writeDescriptor(notificationDescriptor, QByteArray::fromHex("0100"));

    // Config characteristic
    m_ioConfigurationCharacteristic = m_ioService->characteristic(ioConfigurationCharacteristicUuid);
    if (!m_ioConfigurationCharacteristic.isValid()) {
        qCWarning(dcMultiSensor()) << "Invalid IO configuration characteristic.";
        m_bluetoothDevice->disconnectDevice();
    }

    configureIo();
    configureSensorMode(SensorModeRemote);
    configureIo();
}

void SensorTag::onIoServiceCharacteristicChanged(const QLowEnergyCharacteristic &characteristic, const QByteArray &value)
{
    qCDebug(dcMultiSensor()) << characteristic.uuid().toString() << value.toHex();
}

void SensorTag::onBuzzerImpulseTimeout()
{
    setBuzzerPower(false);
}
