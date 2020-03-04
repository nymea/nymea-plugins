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

#include "flowercare.h"

#include "extern-plugininfo.h"

#include <QDataStream>

FlowerCare::FlowerCare(BluetoothLowEnergyDevice *thing, QObject *parent):
    QObject(parent),
    m_bluetoothDevice(thing)
{
    connect(m_bluetoothDevice, &BluetoothLowEnergyDevice::connectedChanged, this, &FlowerCare::onConnectedChanged);
    connect(m_bluetoothDevice, &BluetoothLowEnergyDevice::servicesDiscoveryFinished, this, &FlowerCare::onServiceDiscoveryFinished);
}

void FlowerCare::refreshData()
{
    qCDebug(dcFlowerCare()) << "Connecting to device";
    m_bluetoothDevice->connectDevice();
}

BluetoothLowEnergyDevice *FlowerCare::btDevice() const
{
    return m_bluetoothDevice;
}

void FlowerCare::onConnectedChanged(bool connected)
{
    qCDebug(dcFlowerCare()) << "Connection changed:" << connected;
    if (!connected) {
        m_sensorService->deleteLater();
        m_sensorService = nullptr;
    }
}

void FlowerCare::onServiceDiscoveryFinished()
{
    BluetoothLowEnergyDevice *btDev = static_cast<BluetoothLowEnergyDevice*>(sender());
    qCDebug(dcFlowerCare()) << "have service uuids" << btDev->serviceUuids();

    m_sensorService = btDev->controller()->createServiceObject(sensorServiceUuid, this);
    connect(m_sensorService, &QLowEnergyService::stateChanged, this, &FlowerCare::onSensorServiceStateChanged);
    connect(m_sensorService, &QLowEnergyService::characteristicRead, this, &FlowerCare::onSensorServiceCharacteristicRead);
    connect(m_sensorService, &QLowEnergyService::characteristicChanged, this, &FlowerCare::onSensorServiceCharacteristicChanged);
    m_sensorService->discoverDetails();
}

void FlowerCare::onSensorServiceStateChanged(const QLowEnergyService::ServiceState &state)
{
    if (state != QLowEnergyService::ServiceDiscovered) {
        return;
    }
//    printServiceDetails(m_sensorService);

    QLowEnergyCharacteristic batteryFirmwareCharacteristic = m_sensorService->characteristic(batteryFirmwareCharacteristicUuid);
    if (!batteryFirmwareCharacteristic.isValid()) {
        qCWarning(dcFlowerCare()) << "Invalid battery/firmware characteristic.";
        emit failed();
        return;
    }

    QByteArray value = batteryFirmwareCharacteristic.value();
    QDataStream stream(&value, QIODevice::ReadOnly);
    stream.setByteOrder(QDataStream::LittleEndian);
    stream >> m_batteryLevel;

    QString firmwareVersionString = value.right(5);
    qCDebug(dcFlowerCare()) << "Battery level:" << m_batteryLevel;

    qCDebug(dcFlowerCare()) << "Firmware version:" << firmwareVersionString;

    if (firmwareVersionString >= "2.6.6") {
        QLowEnergyCharacteristic sensorControlCharacteristic = m_sensorService->characteristic(sensorControlCharacteristicUuid);
        m_sensorService->writeCharacteristic(sensorControlCharacteristic, QByteArray::fromHex("A01F"));
        qCDebug(dcFlowerCare()) << "Wrote to handle 0x0033: A01F";
    }

    m_sensorDataCharacteristic = m_sensorService->characteristic(sensorDataCharacteristicUuid);
    if (!m_sensorDataCharacteristic.isValid()) {
        qCWarning(dcFlowerCare()) << "Invalid sensor data characteristic.";
    }

    // Enable notifications
    QLowEnergyDescriptor notificationDescriptor = m_sensorDataCharacteristic.descriptor(QBluetoothUuid::ClientCharacteristicConfiguration);
    m_sensorService->writeDescriptor(notificationDescriptor, QByteArray::fromHex("0100"));

    // Read the data manually
    // Sometimes if we read the light intensty right now we might get wrong values
    // because the LED flashes upon connect. Let's not read it manually but instead wait for
    // the values to come in with the notification
//    m_sensorService->readCharacteristic(m_sensorDataCharacteristic);
}

void FlowerCare::onSensorServiceCharacteristicRead(const QLowEnergyCharacteristic &characteristic, const QByteArray &value)
{
    qCDebug(dcFlowerCare()) << "Characteristic read" << QString::number(characteristic.handle(), 16) << value.toHex();
    if (characteristic != m_sensorDataCharacteristic) {
        return;
    }
    processSensorData(value);
}

void FlowerCare::onSensorServiceCharacteristicChanged(const QLowEnergyCharacteristic &characteristic, const QByteArray &value)
{
    qCDebug(dcFlowerCare()) << "Notification received" << QString::number(characteristic.handle(), 16) << value.toHex();
    if (characteristic != m_sensorDataCharacteristic) {
        return;
    }
    processSensorData(value);
}


void FlowerCare::printServiceDetails(QLowEnergyService *service) const
{
    foreach (const QLowEnergyCharacteristic &characteristic, service->characteristics()) {
        qCDebug(dcFlowerCare()).nospace() << "C:    --> " << characteristic.uuid().toString() << " (" << characteristic.handle() << " Name: " << characteristic.name() << "): " << characteristic.value() << ", " << characteristic.value().toHex();
        foreach (const QLowEnergyDescriptor &descriptor, characteristic.descriptors()) {
            qCDebug(dcFlowerCare()).nospace() << "D:        --> " << descriptor.uuid().toString() << " (" << descriptor.handle() << " Name: " << descriptor.name() << "): " << descriptor.value() << ", " << descriptor.value().toHex();
        }
    }
}

void FlowerCare::processSensorData(const QByteArray &data)
{
    QByteArray copy = data;
    QDataStream stream(&copy, QIODevice::ReadOnly);
    stream.setByteOrder(QDataStream::LittleEndian);
    qint16 temp;
    stream >> temp;
    qint8 skip;
    stream >> skip;
    quint32 lux;
    stream >> lux;
    qint8 moisture;
    stream >> moisture;
    qint16 fertility;
    stream >> fertility;

    qCDebug(dcFlowerCare()) << "Temperature:" << temp << "Lux:" << lux << "moisture:" << moisture << "fertility" << fertility;

    m_bluetoothDevice->disconnectDevice();
    emit finished(m_batteryLevel, 1.0 * temp / 10, lux, moisture, fertility);
}
