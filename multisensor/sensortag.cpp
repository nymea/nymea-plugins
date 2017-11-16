#include "sensortag.h"
#include "extern-plugininfo.h"

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

void SensorTag::updateInfraredValue(const QByteArray &value)
{
    qCDebug(dcMultiSensor()) << "Infrared value" << value;
}

void SensorTag::updateButtonValue(const QByteArray &value)
{
    const quint8 *data = reinterpret_cast<const quint8 *>(value.constData());
    if (*data & 1)
        emit leftKeyPressed();
    if (*data & 2)
        emit rightKeyPressed();
}

void SensorTag::updateHumidityValue(const QByteArray &value)
{
    qCDebug(dcMultiSensor()) << "Humidity value" << value;
}

void SensorTag::updatePressureValue(const QByteArray &value)
{
    qCDebug(dcMultiSensor()) << "Pressure value" << value;
}

void SensorTag::onConnectedChanged(const bool &connected)
{
    qCDebug(dcMultiSensor()) << "Sensor" << m_bluetoothDevice->name() << m_bluetoothDevice->address().toString() << (connected ? "connected" : "disconnected");
    m_device->setStateValue(sensortagConnectedStateTypeId, connected);

    if (!connected) {
        // Clean up services
        m_infraredService->deleteLater();
        m_buttonService->deleteLater();
        m_humidityService->deleteLater();
        m_pressureService->deleteLater();

        m_infraredService = nullptr;
        m_buttonService = nullptr;
        m_humidityService = nullptr;
        m_pressureService = nullptr;
    }

}

void SensorTag::onServiceDiscoveryFinished()
{
    if (!m_bluetoothDevice->serviceUuids().contains(infraredServiceUuid)) {
        qCWarning(dcMultiSensor()) << "Could not find infrared service";
        return;
    }

    if (!m_bluetoothDevice->serviceUuids().contains(accelerometerServiceUuid)) {
        qCWarning(dcMultiSensor()) << "Could not find accelereometer service";
        return;
    }

    if (!m_bluetoothDevice->serviceUuids().contains(humidityServiceUuid)) {
        qCWarning(dcMultiSensor()) << "Could not find humidity service";
        return;
    }

    if (!m_bluetoothDevice->serviceUuids().contains(magnetometerServiceUuid)) {
        qCWarning(dcMultiSensor()) << "Could not find magnetometer service";
        return;
    }

    if (!m_bluetoothDevice->serviceUuids().contains(pressureServiceUuid)) {
        qCWarning(dcMultiSensor()) << "Could not find pressure service";
        return;
    }

    if (!m_bluetoothDevice->serviceUuids().contains(gyroscopeServiceUuid)) {
        qCWarning(dcMultiSensor()) << "Could not find magnetometer service";
        return;
    }

    // IR Temperature
    if (!m_infraredService) {
        m_infraredService = m_bluetoothDevice->controller()->createServiceObject(infraredServiceUuid, this);
        if (!m_infraredService) {
            qCWarning(dcMultiSensor()) << "Could not create infrared service.";
            return;
        }

        connect(m_infraredService, &QLowEnergyService::stateChanged, this, &SensorTag::onInfraredServiceStateChanged);
        connect(m_infraredService, &QLowEnergyService::characteristicChanged, this, &SensorTag::onInfraredServiceCharacteristicChanged);

        if (m_infraredService->state() == QLowEnergyService::DiscoveryRequired) {
            m_infraredService->discoverDetails();
        }
    }

    // Buttons
    if (!m_buttonService) {
        m_buttonService = m_bluetoothDevice->controller()->createServiceObject(buttonServiceUuid, this);
        if (!m_buttonService) {
            qCWarning(dcMultiSensor()) << "Could not create button service.";
            return;
        }

        connect(m_buttonService, &QLowEnergyService::stateChanged, this, &SensorTag::onButtonServiceStateChanged);
        connect(m_buttonService, &QLowEnergyService::characteristicChanged, this, &SensorTag::onButtonServiceCharacteristicChanged);

        if (m_buttonService->state() == QLowEnergyService::DiscoveryRequired) {
            m_buttonService->discoverDetails();
        }
    }

    // Humidity
    if (!m_humidityService) {
        m_humidityService = m_bluetoothDevice->controller()->createServiceObject(humidityServiceUuid, this);
        if (!m_humidityService) {
            qCWarning(dcMultiSensor()) << "Could not create humidity service.";
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
            return;
        }

        connect(m_pressureService, &QLowEnergyService::stateChanged, this, &SensorTag::onPressureServiceStateChanged);
        connect(m_pressureService, &QLowEnergyService::characteristicChanged, this, &SensorTag::onPressureServiceCharacteristicChanged);
        if (m_pressureService->state() == QLowEnergyService::DiscoveryRequired) {
            m_pressureService->discoverDetails();
        }
    }
}

void SensorTag::onInfraredServiceStateChanged(const QLowEnergyService::ServiceState &state)
{
    // Only continue if discovered
    if (state != QLowEnergyService::ServiceDiscovered)
        return;

    qCDebug(dcMultiSensor()) << "Infrared sensor service discovered.";

    foreach (const QLowEnergyCharacteristic &characteristic, m_infraredService->characteristics()) {
        qCDebug(dcMultiSensor()) << "    -->" << characteristic.name() << characteristic.uuid().toString() << characteristic.value();
        foreach (const QLowEnergyDescriptor &desciptor, characteristic.descriptors()) {
            qCDebug(dcMultiSensor()) << "        -->" << desciptor.name() << desciptor.uuid().toString() << desciptor.value();
        }
    }

    // Data characteristic
    m_infraredDataCharacteristic = m_infraredService->characteristic(QBluetoothUuid(QUuid("f000aa01-0451-4000-b000-000000000000")));
    if (!m_infraredDataCharacteristic.isValid()) {
        qCWarning(dcMultiSensor()) << "Invalid infrared data characteristic.";
    }

    // Enable notifications
    QLowEnergyDescriptor notificationDescriptor = m_infraredDataCharacteristic.descriptor(QBluetoothUuid::ClientCharacteristicConfiguration);
    m_infraredService->writeDescriptor(notificationDescriptor, QByteArray::fromHex("0100"));


    // Config characteristic
    m_infraredConfigCharacteristic = m_infraredService->characteristic(QBluetoothUuid(QUuid("f000aa02-0451-4000-b000-000000000000")));
    if (!m_infraredConfigCharacteristic.isValid()) {
        qCWarning(dcMultiSensor()) << "Invalid infrared configuration characteristic.";
    }

    // Enable measuring
    m_infraredService->writeCharacteristic(m_infraredConfigCharacteristic, QByteArray::fromHex("01"));
}

void SensorTag::onInfraredServiceCharacteristicChanged(const QLowEnergyCharacteristic &characteristic, const QByteArray &value)
{
    if (characteristic == m_infraredDataCharacteristic) {
        updateInfraredValue(value);

        // FIXME: Disable measuring
        // m_infraredService->writeCharacteristic(m_infraredConfigCharacteristic, QByteArray::fromHex("00"));
    }
}

void SensorTag::onButtonServiceStateChanged(const QLowEnergyService::ServiceState &state)
{
    // Only continue if discovered
    if (state != QLowEnergyService::ServiceDiscovered)
        return;

    qCDebug(dcMultiSensor()) << "Button sensor service discovered.";

    foreach (const QLowEnergyCharacteristic &characteristic, m_buttonService->characteristics()) {
        qCDebug(dcMultiSensor()) << "    -->" << characteristic.name() << characteristic.uuid().toString() << characteristic.value();
        foreach (const QLowEnergyDescriptor &desciptor, characteristic.descriptors()) {
            qCDebug(dcMultiSensor()) << "        -->" << desciptor.name() << desciptor.uuid().toString() << desciptor.value();
        }
    }

    // Data characteristic
    m_buttonCharacteristic = m_buttonService->characteristic(QBluetoothUuid(QUuid("0000ffe1-0000-1000-8000-00805f9b34fb")));
    if (!m_buttonCharacteristic.isValid()) {
        qCWarning(dcMultiSensor()) << "Invalid button data characteristic.";
    }

    // Enable notifications
    QLowEnergyDescriptor notificationDescriptor = m_buttonCharacteristic.descriptor(QBluetoothUuid::ClientCharacteristicConfiguration);
    m_buttonService->writeDescriptor(notificationDescriptor, QByteArray::fromHex("0100"));
}

void SensorTag::onButtonServiceCharacteristicChanged(const QLowEnergyCharacteristic &characteristic, const QByteArray &value)
{
    if (characteristic == m_buttonCharacteristic) {
        updateButtonValue(value);
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
    m_humidityDataCharacteristic = m_humidityService->characteristic(QBluetoothUuid(QUuid("f000aa21-0451-4000-b000-000000000000")));
    if (!m_humidityDataCharacteristic.isValid()) {
        qCWarning(dcMultiSensor()) << "Invalid humidity data characteristic.";
    }

    // Enable notifications
    QLowEnergyDescriptor notificationDescriptor = m_humidityDataCharacteristic.descriptor(QBluetoothUuid::ClientCharacteristicConfiguration);
    m_humidityService->writeDescriptor(notificationDescriptor, QByteArray::fromHex("0100"));

    // Config characteristic
    m_humidityConfigCharacteristic = m_humidityService->characteristic(QBluetoothUuid(QUuid("f000aa22-0451-4000-b000-000000000000")));
    if (!m_humidityConfigCharacteristic.isValid()) {
        qCWarning(dcMultiSensor()) << "Invalid humidity configuration characteristic.";
    }

    // Enable measuring
    m_humidityService->writeCharacteristic(m_humidityConfigCharacteristic, QByteArray::fromHex("01"));
}

void SensorTag::onHumidityServiceCharacteristicChanged(const QLowEnergyCharacteristic &characteristic, const QByteArray &value)
{
    if (characteristic == m_humidityDataCharacteristic) {
        updateHumidityValue(value);
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
    m_pressureDataCharacteristic = m_pressureService->characteristic(QBluetoothUuid(QUuid("f000aa41-0451-4000-b000-000000000000")));
    if (!m_pressureDataCharacteristic.isValid()) {
        qCWarning(dcMultiSensor()) << "Invalid pressure data characteristic.";
    }

    // Enable notifications
    QLowEnergyDescriptor notificationDescriptor = m_pressureDataCharacteristic.descriptor(QBluetoothUuid::ClientCharacteristicConfiguration);
    m_pressureService->writeDescriptor(notificationDescriptor, QByteArray::fromHex("0100"));

    // Config characteristic
    m_pressureConfigCharacteristic = m_pressureService->characteristic(QBluetoothUuid(QUuid("f000aa42-0451-4000-b000-000000000000")));
    if (!m_pressureConfigCharacteristic.isValid()) {
        qCWarning(dcMultiSensor()) << "Invalid pressure configuration characteristic.";
    }

    // Enable measuring
    m_pressureService->writeCharacteristic(m_pressureConfigCharacteristic, QByteArray::fromHex("01"));
}

void SensorTag::onPressureServiceCharacteristicChanged(const QLowEnergyCharacteristic &characteristic, const QByteArray &value)
{
    if (characteristic == m_pressureDataCharacteristic) {
        updatePressureValue(value);
        // FIXME: Disable measuring
        // m_pressureService->writeCharacteristic(m_pressureConfigCharacteristic, QByteArray::fromHex("00"));
    }
}

void SensorTag::measure()
{
    if (!m_bluetoothDevice->connected())
        return;

    // TODO: measure using plugintimer to save energy

//    qCDebug(dcMultiSensor()) << "Measure data" << m_bluetoothDevice->name() << m_bluetoothDevice->address().toString();

//    m_infraredService->writeCharacteristic(m_infraredConfigCharacteristic, QByteArray::fromHex("01"));
//    m_humidityService->writeCharacteristic(m_humidityConfigCharacteristic, QByteArray::fromHex("01"));
//    m_pressureService->writeCharacteristic(m_pressureConfigCharacteristic, QByteArray::fromHex("01"));

}

