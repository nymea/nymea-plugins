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

#include "sensortag.h"
#include "extern-plugininfo.h"
#include "math.h"

#include <QTimer>
#include <QVector3D>
#include <QDataStream>

SensorTag::SensorTag(Thing *thing, BluetoothLowEnergyDevice *bluetoothDevice, QObject *parent) :
    QObject(parent),
    m_thing(thing),
    m_bluetoothDevice(bluetoothDevice),
    m_dataProcessor(new SensorDataProcessor(m_thing, this))
{
    connect(m_bluetoothDevice, &BluetoothLowEnergyDevice::connectedChanged, this, &SensorTag::onConnectedChanged);
    connect(m_bluetoothDevice, &BluetoothLowEnergyDevice::servicesDiscoveryFinished, this, &SensorTag::onServiceDiscoveryFinished);
}

Thing *SensorTag::thing()
{
    return m_thing;
}

BluetoothLowEnergyDevice *SensorTag::bluetoothDevice()
{
    return m_bluetoothDevice;
}

void SensorTag::setTemperatureSensorEnabled(bool enabled)
{
    qCDebug(dcTexasInstruments()) << "Temperature sensor" << (enabled ? "enabled" : "disabled");

    if (m_temperatureEnabled == enabled)
        return;

    m_temperatureEnabled = enabled;
    setTemperatureSensorPower(m_temperatureEnabled);
}

void SensorTag::setHumiditySensorEnabled(bool enabled)
{
    qCDebug(dcTexasInstruments()) << "Humidity sensor" << (enabled ? "enabled" : "disabled");

    if (m_humidityEnabled == enabled)
        return;

    m_humidityEnabled = enabled;
    setHumiditySensorPower(m_humidityEnabled);
}

void SensorTag::setPressureSensorEnabled(bool enabled)
{
    qCDebug(dcTexasInstruments()) << "Pressure sensor" << (enabled ? "enabled" : "disabled");

    if (m_pressureEnabled == enabled)
        return;

    m_pressureEnabled = enabled;
    setPressureSensorPower(m_pressureEnabled);
}

void SensorTag::setOpticalSensorEnabled(bool enabled)
{
    qCDebug(dcTexasInstruments()) << "Optical sensor" << (enabled ? "enabled" : "disabled");

    if (m_opticalEnabled == enabled)
        return;

    m_opticalEnabled = enabled;
    setOpticalSensorPower(m_opticalEnabled);
}

void SensorTag::setAccelerometerEnabled(bool enabled)
{
    qCDebug(dcTexasInstruments()) << "Accelerometer" << (enabled ? "enabled" : "disabled");

    if (m_accelerometerEnabled == enabled)
        return;

    m_accelerometerEnabled = enabled;
    configureMovement();
}

void SensorTag::setAccelerometerRange(const SensorTag::SensorAccelerometerRange &range)
{
    qCDebug(dcTexasInstruments()) << "Accelerometer" << range;

    if (m_accelerometerRange == range)
        return;

    m_accelerometerRange = range;
    configureMovement();
}

void SensorTag::setGyroscopeEnabled(bool enabled)
{
    qCDebug(dcTexasInstruments()) << "Gyroscope" << (enabled ? "enabled" : "disabled");

    if (m_gyroscopeEnabled == enabled)
        return;

    m_gyroscopeEnabled = enabled;
    configureMovement();
}

void SensorTag::setMagnetometerEnabled(bool enabled)
{
    qCDebug(dcTexasInstruments()) << "Magnetometer" << (enabled ? "enabled" : "disabled");

    if (m_magnetometerEnabled == enabled)
        return;

    m_magnetometerEnabled = enabled;
    configureMovement();
}

void SensorTag::setMeasurementPeriod(int period)
{
    qCDebug(dcTexasInstruments()) << "Set sensor measurement period to" << period << "ms";

    if (period % 10 != 0) {
        int adjustedValue = qRound(static_cast<float>(period) / 10.0) * 10;
        qCWarning(dcTexasInstruments()) << "Measurement period of sensors" << period << "must be a multiple of 10ms. Adjusting it to" << adjustedValue;
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
    qCDebug(dcTexasInstruments()) << "Set movement sensor measurement period to" << period << "ms";

    if (period % 10 != 0) {
        int adjustedValue = qRound(static_cast<float>(period) / 10.0) * 10;
        qCWarning(dcTexasInstruments()) << "Measurement period of movement sensor" << period << "must be a multiple of 10ms. Adjusting it to" << adjustedValue;
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
    qCDebug(dcTexasInstruments()) << "Green LED" << (power ? "enabled" : "disabled");
    configureIo();
    m_thing->setStateValue(sensorTagGreenLedStateTypeId, m_greenLedEnabled);
}

void SensorTag::setRedLedPower(bool power)
{
    m_redLedEnabled = power;
    qCDebug(dcTexasInstruments()) << "Red LED" << (power ? "enabled" : "disabled");
    configureIo();
    m_thing->setStateValue(sensorTagRedLedStateTypeId, m_redLedEnabled);
}

void SensorTag::setBuzzerPower(bool power)
{
    m_buzzerEnabled = power;
    qCDebug(dcTexasInstruments()) << "Buzzer" << (power ? "enabled" : "disabled");
    configureIo();
    m_thing->setStateValue(sensorTagBuzzerStateTypeId, m_buzzerEnabled);
}

void SensorTag::buzzerImpulse()
{
    qCDebug(dcTexasInstruments()) << "Buzzer impulse";
    setBuzzerPower(true);
    QTimer::singleShot(1000, this, &SensorTag::onBuzzerImpulseTimeout);
}

void SensorTag::configurePeriod(QLowEnergyService *serice, const QLowEnergyCharacteristic &characteristic, int measurementPeriod)
{
    Q_ASSERT(measurementPeriod % 10 == 0);
    QByteArray payload;
    QDataStream stream(&payload, QIODevice::WriteOnly);
    stream << static_cast<quint8>(measurementPeriod / 10);

    qCDebug(dcTexasInstruments()) << "Configure period to" << measurementPeriod << payload.toHex();
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

    qCDebug(dcTexasInstruments()) << "Configure movement sensor" << data.toHex();
    m_movementService->writeCharacteristic(m_movementConfigurationCharacteristic, data);
}

void SensorTag::configureSensorMode(const SensorTag::SensorMode &mode)
{
    if (!m_ioService || !m_ioDataCharacteristic.isValid())
        return;

    qCDebug(dcTexasInstruments()) << "Setting" << mode;
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

void SensorTag::setTemperatureSensorPower(bool power)
{
    if (!m_temperatureService || !m_temperatureConfigurationCharacteristic.isValid())
        return;

    QByteArray payload = (power ? QByteArray::fromHex("01") : QByteArray::fromHex("00"));
    m_temperatureService->writeCharacteristic(m_temperatureConfigurationCharacteristic, payload);
}

void SensorTag::setHumiditySensorPower(bool power)
{
    if (!m_humidityService || !m_humidityConfigurationCharacteristic.isValid())
        return;

    QByteArray payload = (power ? QByteArray::fromHex("01") : QByteArray::fromHex("00"));
    m_humidityService->writeCharacteristic(m_humidityConfigurationCharacteristic, payload);
}

void SensorTag::setPressureSensorPower(bool power)
{
    if (!m_pressureService || !m_pressureConfigurationCharacteristic.isValid())
        return;

    QByteArray payload = (power ? QByteArray::fromHex("01") : QByteArray::fromHex("00"));
    m_pressureService->writeCharacteristic(m_pressureConfigurationCharacteristic, payload);
}

void SensorTag::setOpticalSensorPower(bool power)
{
    if (!m_opticalService || !m_opticalConfigurationCharacteristic.isValid())
        return;

    QByteArray payload = (power ? QByteArray::fromHex("01") : QByteArray::fromHex("00"));
    m_opticalService->writeCharacteristic(m_opticalConfigurationCharacteristic, payload);
}


void SensorTag::onConnectedChanged(const bool &connected)
{
    qCDebug(dcTexasInstruments()) << "Sensor" << m_bluetoothDevice->name() << m_bluetoothDevice->address().toString() << (connected ? "connected" : "disconnected");
    m_thing->setStateValue(sensorTagConnectedStateTypeId, connected);

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

        m_dataProcessor->reset();
    }
}

void SensorTag::onServiceDiscoveryFinished()
{
    foreach (const QBluetoothUuid serviceUuid, m_bluetoothDevice->serviceUuids()) {
        qCDebug(dcTexasInstruments()) << "-->" << serviceUuid;
    }

    if (!m_bluetoothDevice->serviceUuids().contains(temperatureServiceUuid)) {
        qCWarning(dcTexasInstruments()) << "Could not find temperature service";
        m_bluetoothDevice->disconnectDevice();
        return;
    }

    if (!m_bluetoothDevice->serviceUuids().contains(humidityServiceUuid)) {
        qCWarning(dcTexasInstruments()) << "Could not find humidity service";
        m_bluetoothDevice->disconnectDevice();
        return;
    }

    if (!m_bluetoothDevice->serviceUuids().contains(pressureServiceUuid)) {
        qCWarning(dcTexasInstruments()) << "Could not find pressure service";
        m_bluetoothDevice->disconnectDevice();
        return;
    }

    if (!m_bluetoothDevice->serviceUuids().contains(opticalServiceUuid)) {
        qCWarning(dcTexasInstruments()) << "Could not find optical service";
        m_bluetoothDevice->disconnectDevice();
        return;
    }

    if (!m_bluetoothDevice->serviceUuids().contains(keyServiceUuid)) {
        qCWarning(dcTexasInstruments()) << "Could not find key service";
        m_bluetoothDevice->disconnectDevice();
        return;
    }

    if (!m_bluetoothDevice->serviceUuids().contains(movementServiceUuid)) {
        qCWarning(dcTexasInstruments()) << "Could not find movement service";
        m_bluetoothDevice->disconnectDevice();
        return;
    }

    if (!m_bluetoothDevice->serviceUuids().contains(ioServiceUuid)) {
        qCWarning(dcTexasInstruments()) << "Could not find IO service";
        m_bluetoothDevice->disconnectDevice();
        return;
    }

    // IR Temperature
    if (!m_temperatureService) {
        m_temperatureService = m_bluetoothDevice->controller()->createServiceObject(temperatureServiceUuid, this);
        if (!m_temperatureService) {
            qCWarning(dcTexasInstruments()) << "Could not create temperature service.";
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
            qCWarning(dcTexasInstruments()) << "Could not create humidity service.";
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
            qCWarning(dcTexasInstruments()) << "Could not create pressure service.";
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
            qCWarning(dcTexasInstruments()) << "Could not create optical service.";
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
            qCWarning(dcTexasInstruments()) << "Could not create key service.";
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
            qCWarning(dcTexasInstruments()) << "Could not create movement service.";
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
            qCWarning(dcTexasInstruments()) << "Could not create IO service.";
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

void SensorTag::onBuzzerImpulseTimeout()
{
    setBuzzerPower(false);
}

void SensorTag::onTemperatureServiceStateChanged(const QLowEnergyService::ServiceState &state)
{
    // Only continue if discovered
    if (state != QLowEnergyService::ServiceDiscovered)
        return;

    qCDebug(dcTexasInstruments()) << "Temperature sensor service discovered.";

    foreach (const QLowEnergyCharacteristic &characteristic, m_temperatureService->characteristics()) {
        qCDebug(dcTexasInstruments()) << "    -->" << characteristic.name() << characteristic.uuid().toString() << characteristic.value();
        foreach (const QLowEnergyDescriptor &descriptor, characteristic.descriptors()) {
            qCDebug(dcTexasInstruments()) << "        -->" << descriptor.name() << descriptor.uuid().toString() << descriptor.value();
        }
    }

    // Data characteristic
    m_temperatureDataCharacteristic = m_temperatureService->characteristic(temperatureDataCharacteristicUuid);
    if (!m_temperatureDataCharacteristic.isValid()) {
        qCWarning(dcTexasInstruments()) << "Invalid temperature data characteristic.";
        m_bluetoothDevice->disconnectDevice();
        return;
    }

    // Enable notifications
    QLowEnergyDescriptor notificationDescriptor = m_temperatureDataCharacteristic.descriptor(QBluetoothUuid::ClientCharacteristicConfiguration);
    m_temperatureService->writeDescriptor(notificationDescriptor, QByteArray::fromHex("0100"));


    // Config characteristic
    m_temperatureConfigurationCharacteristic = m_temperatureService->characteristic(temperatureConfigurationCharacteristicUuid);
    if (!m_temperatureConfigurationCharacteristic.isValid()) {
        qCWarning(dcTexasInstruments()) << "Invalid temperature configuration characteristic.";
        m_bluetoothDevice->disconnectDevice();
        return;
    }

    // Period characteristic
    m_temperaturePeriodCharacteristic = m_temperatureService->characteristic(temperaturePeriodCharacteristicUuid);
    if (!m_temperaturePeriodCharacteristic.isValid()) {
        qCWarning(dcTexasInstruments()) << "Invalid temperature period characteristic.";
        m_bluetoothDevice->disconnectDevice();
        return;
    }

    configurePeriod(m_temperatureService, m_temperaturePeriodCharacteristic, m_temperaturePeriod);

    // Enable/disable measuring
    setTemperatureSensorPower(m_temperatureEnabled);
}

void SensorTag::onTemperatureServiceCharacteristicChanged(const QLowEnergyCharacteristic &characteristic, const QByteArray &value)
{
    if (characteristic == m_temperatureDataCharacteristic) {
        m_dataProcessor->processTemperatureData(value);
    }
}

void SensorTag::onHumidityServiceStateChanged(const QLowEnergyService::ServiceState &state)
{
    // Only continue if discovered
    if (state != QLowEnergyService::ServiceDiscovered)
        return;

    qCDebug(dcTexasInstruments()) << "Humidity sensor service discovered.";

    foreach (const QLowEnergyCharacteristic &characteristic, m_humidityService->characteristics()) {
        qCDebug(dcTexasInstruments()) << "    -->" << characteristic.name() << characteristic.uuid().toString() << characteristic.value();
        foreach (const QLowEnergyDescriptor &descriptor, characteristic.descriptors()) {
            qCDebug(dcTexasInstruments()) << "        -->" << descriptor.name() << descriptor.uuid().toString() << descriptor.value();
        }
    }

    // Data characteristic
    m_humidityDataCharacteristic = m_humidityService->characteristic(humidityDataCharacteristicUuid);
    if (!m_humidityDataCharacteristic.isValid()) {
        qCWarning(dcTexasInstruments()) << "Invalid humidity data characteristic.";
        m_bluetoothDevice->disconnectDevice();
        return;
    }

    // Enable notifications
    QLowEnergyDescriptor notificationDescriptor = m_humidityDataCharacteristic.descriptor(QBluetoothUuid::ClientCharacteristicConfiguration);
    m_humidityService->writeDescriptor(notificationDescriptor, QByteArray::fromHex("0100"));

    // Config characteristic
    m_humidityConfigurationCharacteristic = m_humidityService->characteristic(humidityConfigurationCharacteristicUuid);
    if (!m_humidityConfigurationCharacteristic.isValid()) {
        qCWarning(dcTexasInstruments()) << "Invalid humidity configuration characteristic.";
        m_bluetoothDevice->disconnectDevice();
        return;
    }

    // Period characteristic
    m_humidityPeriodCharacteristic = m_humidityService->characteristic(humidityPeriodCharacteristicUuid);
    if (!m_humidityPeriodCharacteristic.isValid()) {
        qCWarning(dcTexasInstruments()) << "Invalid humidity period characteristic.";
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
        m_dataProcessor->processHumidityData(value);
    }
}

void SensorTag::onPressureServiceStateChanged(const QLowEnergyService::ServiceState &state)
{
    // Only continue if discovered
    if (state != QLowEnergyService::ServiceDiscovered)
        return;

    qCDebug(dcTexasInstruments()) << "Pressure sensor service discovered.";

    foreach (const QLowEnergyCharacteristic &characteristic, m_pressureService->characteristics()) {
        qCDebug(dcTexasInstruments()) << "    -->" << characteristic.name() << characteristic.uuid().toString() << characteristic.value();
        foreach (const QLowEnergyDescriptor &descriptor, characteristic.descriptors()) {
            qCDebug(dcTexasInstruments()) << "        -->" << descriptor.name() << descriptor.uuid().toString() << descriptor.value();
        }
    }

    // Data characteristic
    m_pressureDataCharacteristic = m_pressureService->characteristic(pressureDataCharacteristicUuid);
    if (!m_pressureDataCharacteristic.isValid()) {
        qCWarning(dcTexasInstruments()) << "Invalid pressure data characteristic.";
        m_bluetoothDevice->disconnectDevice();
        return;
    }

    // Enable notifications
    QLowEnergyDescriptor notificationDescriptor = m_pressureDataCharacteristic.descriptor(QBluetoothUuid::ClientCharacteristicConfiguration);
    m_pressureService->writeDescriptor(notificationDescriptor, QByteArray::fromHex("0100"));

    // Config characteristic
    m_pressureConfigurationCharacteristic = m_pressureService->characteristic(pressureConfigurationCharacteristicUuid);
    if (!m_pressureConfigurationCharacteristic.isValid()) {
        qCWarning(dcTexasInstruments()) << "Invalid pressure configuration characteristic.";
        m_bluetoothDevice->disconnectDevice();
        return;
    }

    // Period characteristic
    m_pressurePeriodCharacteristic = m_pressureService->characteristic(pressurePeriodCharacteristicUuid);
    if (!m_pressurePeriodCharacteristic.isValid()) {
        qCWarning(dcTexasInstruments()) << "Invalid pressure period characteristic.";
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
        m_dataProcessor->processPressureData(value);
    }
}

void SensorTag::onOpticalServiceStateChanged(const QLowEnergyService::ServiceState &state)
{
    // Only continue if discovered
    if (state != QLowEnergyService::ServiceDiscovered)
        return;

    qCDebug(dcTexasInstruments()) << "Optical sensor service discovered.";

    foreach (const QLowEnergyCharacteristic &characteristic, m_pressureService->characteristics()) {
        qCDebug(dcTexasInstruments()) << "    -->" << characteristic.name() << characteristic.uuid().toString() << characteristic.value();
        foreach (const QLowEnergyDescriptor &descriptor, characteristic.descriptors()) {
            qCDebug(dcTexasInstruments()) << "        -->" << descriptor.name() << descriptor.uuid().toString() << descriptor.value();
        }
    }

    // Data characteristic
    m_opticalDataCharacteristic = m_opticalService->characteristic(opticalDataCharacteristicUuid);
    if (!m_opticalDataCharacteristic.isValid()) {
        qCWarning(dcTexasInstruments()) << "Invalid optical data characteristic.";
        m_bluetoothDevice->disconnectDevice();
        return;
    }

    // Enable notifications
    QLowEnergyDescriptor notificationDescriptor = m_opticalDataCharacteristic.descriptor(QBluetoothUuid::ClientCharacteristicConfiguration);
    m_opticalService->writeDescriptor(notificationDescriptor, QByteArray::fromHex("0100"));

    // Config characteristic
    m_opticalConfigurationCharacteristic = m_opticalService->characteristic(opticalConfigurationCharacteristicUuid);
    if (!m_opticalConfigurationCharacteristic.isValid()) {
        qCWarning(dcTexasInstruments()) << "Invalid optical configuration characteristic.";
        m_bluetoothDevice->disconnectDevice();
    }

    // Period characteristic
    m_opticalPeriodCharacteristic = m_opticalService->characteristic(opticalPeriodCharacteristicUuid);
    if (!m_opticalPeriodCharacteristic.isValid()) {
        qCWarning(dcTexasInstruments()) << "Invalid optical period characteristic.";
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
        m_dataProcessor->processOpticalData(value);
    }
}

void SensorTag::onKeyServiceStateChanged(const QLowEnergyService::ServiceState &state)
{
    // Only continue if discovered
    if (state != QLowEnergyService::ServiceDiscovered)
        return;

    qCDebug(dcTexasInstruments()) << "Key service discovered.";
    foreach (const QLowEnergyCharacteristic &characteristic, m_keyService->characteristics()) {
        qCDebug(dcTexasInstruments()) << "    -->" << characteristic.name() << characteristic.uuid().toString() << characteristic.value();
        foreach (const QLowEnergyDescriptor &descriptor, characteristic.descriptors()) {
            qCDebug(dcTexasInstruments()) << "        -->" << descriptor.name() << descriptor.uuid().toString() << descriptor.value();
        }
    }

    // Data characteristic
    m_keyDataCharacteristic = m_keyService->characteristic(keyDataCharacteristicUuid);
    if (!m_keyDataCharacteristic.isValid()) {
        qCWarning(dcTexasInstruments()) << "Invalid button data characteristic.";
        m_bluetoothDevice->disconnectDevice();
    }

    // Enable notifications
    QLowEnergyDescriptor notificationDescriptor = m_keyDataCharacteristic.descriptor(QBluetoothUuid::ClientCharacteristicConfiguration);
    m_keyService->writeDescriptor(notificationDescriptor, QByteArray::fromHex("0100"));
}

void SensorTag::onKeyServiceCharacteristicChanged(const QLowEnergyCharacteristic &characteristic, const QByteArray &value)
{
    if (characteristic == m_keyDataCharacteristic) {
        m_dataProcessor->processKeyData(value);
    }
}

void SensorTag::onMovementServiceStateChanged(const QLowEnergyService::ServiceState &state)
{
    // Only continue if discovered
    if (state != QLowEnergyService::ServiceDiscovered)
        return;

    qCDebug(dcTexasInstruments()) << "Movement sensor service discovered.";

    foreach (const QLowEnergyCharacteristic &characteristic, m_pressureService->characteristics()) {
        qCDebug(dcTexasInstruments()) << "    -->" << characteristic.name() << characteristic.uuid().toString() << characteristic.value();
        foreach (const QLowEnergyDescriptor &descriptor, characteristic.descriptors()) {
            qCDebug(dcTexasInstruments()) << "        -->" << descriptor.name() << descriptor.uuid().toString() << descriptor.value();
        }
    }

    // Data characteristic
    m_movementDataCharacteristic = m_movementService->characteristic(movementDataCharacteristicUuid);
    if (!m_movementDataCharacteristic.isValid()) {
        qCWarning(dcTexasInstruments()) << "Invalid movement data characteristic.";
        m_bluetoothDevice->disconnectDevice();
    }

    // Enable notifications
    QLowEnergyDescriptor notificationDescriptor = m_movementDataCharacteristic.descriptor(QBluetoothUuid::ClientCharacteristicConfiguration);
    m_movementService->writeDescriptor(notificationDescriptor, QByteArray::fromHex("0100"));

    // Config characteristic
    m_movementConfigurationCharacteristic = m_movementService->characteristic(movementConfigurationCharacteristicUuid);
    if (!m_movementConfigurationCharacteristic.isValid()) {
        qCWarning(dcTexasInstruments()) << "Invalid movement configuration characteristic.";
        m_bluetoothDevice->disconnectDevice();
    }

    // Period characteristic
    m_movementPeriodCharacteristic = m_movementService->characteristic(movementPeriodCharacteristicUuid);
    if (!m_movementPeriodCharacteristic.isValid()) {
        qCWarning(dcTexasInstruments()) << "Invalid movement period characteristic.";
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
        m_dataProcessor->processMovementData(value);
    }
}

void SensorTag::onIoServiceStateChanged(const QLowEnergyService::ServiceState &state)
{
    // Only continue if discovered
    if (state != QLowEnergyService::ServiceDiscovered)
        return;

    qCDebug(dcTexasInstruments()) << "IO service discovered.";

    foreach (const QLowEnergyCharacteristic &characteristic, m_pressureService->characteristics()) {
        qCDebug(dcTexasInstruments()) << "    -->" << characteristic.name() << characteristic.uuid().toString() << characteristic.value();
        foreach (const QLowEnergyDescriptor &descriptor, characteristic.descriptors()) {
            qCDebug(dcTexasInstruments()) << "        -->" << descriptor.name() << descriptor.uuid().toString() << descriptor.value();
        }
    }

    // Data characteristic
    m_ioDataCharacteristic = m_ioService->characteristic(ioDataCharacteristicUuid);
    if (!m_ioDataCharacteristic.isValid()) {
        qCWarning(dcTexasInstruments()) << "Invalid IO data characteristic.";
        m_bluetoothDevice->disconnectDevice();
    }

    // Enable notifications
    QLowEnergyDescriptor notificationDescriptor = m_ioDataCharacteristic.descriptor(QBluetoothUuid::ClientCharacteristicConfiguration);
    m_ioService->writeDescriptor(notificationDescriptor, QByteArray::fromHex("0100"));

    // Config characteristic
    m_ioConfigurationCharacteristic = m_ioService->characteristic(ioConfigurationCharacteristicUuid);
    if (!m_ioConfigurationCharacteristic.isValid()) {
        qCWarning(dcTexasInstruments()) << "Invalid IO configuration characteristic.";
        m_bluetoothDevice->disconnectDevice();
    }

    configureIo();
    configureSensorMode(SensorModeRemote);
    configureIo();
}

void SensorTag::onIoServiceCharacteristicChanged(const QLowEnergyCharacteristic &characteristic, const QByteArray &value)
{
    qCDebug(dcTexasInstruments()) << characteristic.uuid().toString() << value.toHex();
}
