/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *                                                                         *
 *  Copyright (C) 2016-2018 Simon Stürz <simon.stuerz@guh.io>              *
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

#include "LukeRoberts.h"
#include "extern-plugininfo.h"

#include <QBitArray>
#include <QtEndian>

static QBluetoothUuid ledMatrinxServiceUuid             = QBluetoothUuid(QUuid("f29b1523-cb19-40f3-be5c-7241ecb82fd1"));
static QBluetoothUuid ledMatrixCharacteristicUuid       = QBluetoothUuid(QUuid("f29b1524-cb19-40f3-be5c-7241ecb82fd1"));


LukeRoberts::LukeRoberts(BluetoothLowEnergyDevice *bluetoothDevice, QObject *parent) :
    QObject(parent),
    m_bluetoothDevice(bluetoothDevice)
{
    connect(m_bluetoothDevice, &BluetoothLowEnergyDevice::connectedChanged, this, &LukeRoberts::onConnectedChanged);
    connect(m_bluetoothDevice, &BluetoothLowEnergyDevice::servicesDiscoveryFinished, this, &LukeRoberts::onServiceDiscoveryFinished);
}

BluetoothLowEnergyDevice *LukeRoberts::bluetoothDevice()
{
    return m_bluetoothDevice;
}


void LukeRoberts::printService(QLowEnergyService *service)
{
    foreach (const QLowEnergyCharacteristic &characteristic, service->characteristics()) {
        qCDebug(dcSenic()) << "    -->" << characteristic.name() << characteristic.uuid().toString() << characteristic.value();
        foreach (const QLowEnergyDescriptor &descriptor, characteristic.descriptors()) {
            qCDebug(dcSenic()) << "        -->" << descriptor.name() << descriptor.uuid().toString() << descriptor.value();
        }
    }
}


void LukeRoberts::onConnectedChanged(bool connected)
{
    qCDebug(dcSenic()) << m_bluetoothDevice->name() << m_bluetoothDevice->address().toString() << (connected ? "connected" : "disconnected");

    m_longPressTimer->stop();
    emit connectedChanged(connected);

    if (!connected) {
        // Clean up services
        m_deviceInfoService->deleteLater();
        m_batteryService->deleteLater();
        m_ledMatrixService->deleteLater();
        m_inputService->deleteLater();

        m_deviceInfoService = nullptr;
        m_batteryService = nullptr;
        m_ledMatrixService = nullptr;
        m_inputService = nullptr;
    }

}

void LukeRoberts::onServiceDiscoveryFinished()
{
    qCDebug(dcSenic()) << "Service scan finised";

    if (!m_bluetoothDevice->serviceUuids().contains(QBluetoothUuid::DeviceInformation)) {
        qCWarning(dcSenic()) << "Device Information service not found for device" << bluetoothDevice()->name() << bluetoothDevice()->address().toString();
        emit deviceInitializationFinished(false);
        return;
    }

    if (!m_bluetoothDevice->serviceUuids().contains(ledMatrinxServiceUuid)) {
        qCWarning(dcSenic()) << "Led matrix service not found for device" << bluetoothDevice()->name() << bluetoothDevice()->address().toString();
        emit deviceInitializationFinished(false);
        return;
    }

    if (!m_bluetoothDevice->serviceUuids().contains(inputServiceUuid)) {
        qCWarning(dcSenic()) << "Input service not found for device" << bluetoothDevice()->name() << bluetoothDevice()->address().toString();
        emit deviceInitializationFinished(false);
        return;
    }

    // Device info service
    if (!m_deviceInfoService) {
        m_deviceInfoService = m_bluetoothDevice->controller()->createServiceObject(QBluetoothUuid::DeviceInformation, this);
        if (!m_deviceInfoService) {
            qCWarning(dcSenic()) << "Could not create device info service.";
            emit deviceInitializationFinished(false);
            return;
        }

        connect(m_deviceInfoService, &QLowEnergyService::stateChanged, this, &LukeRoberts::onDeviceInfoServiceStateChanged);

        if (m_deviceInfoService->state() == QLowEnergyService::DiscoveryRequired) {
            m_deviceInfoService->discoverDetails();
        }
    }

    // Custom control service
    if (!m_ledMatrixService) {
        m_ledMatrixService = m_bluetoothDevice->controller()->createServiceObject(ledMatrinxServiceUuid, this);
        if (!m_ledMatrixService) {
            qCWarning(dcSenic()) << "Could not create led matrix service.";
            emit deviceInitializationFinished(false);
            return;
        }

        connect(m_ledMatrixService, &QLowEnergyService::stateChanged, this, &LukeRoberts::onLedMatrixServiceStateChanged);

        if (m_ledMatrixService->state() == QLowEnergyService::DiscoveryRequired) {
            m_ledMatrixService->discoverDetails();
        }
    }
    emit deviceInitializationFinished(true);
}

void LukeRoberts::onDeviceInfoServiceStateChanged(const QLowEnergyService::ServiceState &state)
{
    // Only continue if discovered
    if (state != QLowEnergyService::ServiceDiscovered)
        return;

    qCDebug(dcSenic()) << "Device info service discovered.";

    printService(m_deviceInfoService);
    QString firmware = QString::fromUtf8(m_deviceInfoService->characteristic(QBluetoothUuid::FirmwareRevisionString).value());
    QString hardware = QString::fromUtf8(m_deviceInfoService->characteristic(QBluetoothUuid::HardwareRevisionString).value());
    QString software = QString::fromUtf8(m_deviceInfoService->characteristic(QBluetoothUuid::SoftwareRevisionString).value());

    emit deviceInformationChanged(firmware, hardware, software);
}

void LukeRoberts::onBatteryServiceStateChanged(const QLowEnergyService::ServiceState &state)
{
    // Only continue if discovered
    if (state != QLowEnergyService::ServiceDiscovered)
        return;

    qCDebug(dcSenic()) << "Battery service discovered.";

    printService(m_batteryService);

    m_batteryCharacteristic = m_batteryService->characteristic(QBluetoothUuid::BatteryLevel);
    if (!m_batteryCharacteristic.isValid()) {
        qCWarning(dcSenic()) << "Battery characteristc not found for device " << bluetoothDevice()->name() << bluetoothDevice()->address().toString();
        return;
    }

    // Enable notifications
    QLowEnergyDescriptor notificationDescriptor = m_batteryCharacteristic.descriptor(QBluetoothUuid::ClientCharacteristicConfiguration);
    m_batteryService->writeDescriptor(notificationDescriptor, QByteArray::fromHex("0100"));

    uint batteryPercentage = m_batteryCharacteristic.value().toHex().toUInt(nullptr, 16);
    emit batteryValueChanged(batteryPercentage);
}

void LukeRoberts::onBatteryCharacteristicChanged(const QLowEnergyCharacteristic &characteristic, const QByteArray &value)
{
    if (characteristic.uuid() == m_batteryCharacteristic.uuid()) {
        uint batteryPercentage = value.toHex().toUInt(nullptr, 16);
        emit batteryValueChanged(batteryPercentage);
    }
}

void LukeRoberts::onInputServiceStateChanged(const QLowEnergyService::ServiceState &state)
{
    // Only continue if discovered
    if (state != QLowEnergyService::ServiceDiscovered)
        return;

    qCDebug(dcSenic()) << "Input service discovered.";

    printService(m_inputService);

    // Button
    m_inputButtonCharacteristic = m_inputService->characteristic(inputButtonCharacteristicUuid);
    if (!m_inputButtonCharacteristic.isValid()) {
        qCWarning(dcSenic()) << "Input button characteristc not found for device " << bluetoothDevice()->name() << bluetoothDevice()->address().toString();
        return;
    }
    // Enable notifications
    QLowEnergyDescriptor notificationDescriptor = m_inputButtonCharacteristic.descriptor(QBluetoothUuid::ClientCharacteristicConfiguration);
    m_inputService->writeDescriptor(notificationDescriptor, QByteArray::fromHex("0100"));


    // Swipe
    m_inputSwipeCharacteristic = m_inputService->characteristic(inputSwipeCharacteristicUuid);
    if (!m_inputSwipeCharacteristic.isValid()) {
        qCWarning(dcSenic()) << "Input swipe characteristc not found for device " << bluetoothDevice()->name() << bluetoothDevice()->address().toString();
        return;
    }
    // Enable notifications
    notificationDescriptor = m_inputSwipeCharacteristic.descriptor(QBluetoothUuid::ClientCharacteristicConfiguration);
    m_inputService->writeDescriptor(notificationDescriptor, QByteArray::fromHex("0100"));


    // Rotation
    m_inputRotationCharacteristic = m_inputService->characteristic(inputRotationCharacteristicUuid);
    if (!m_inputRotationCharacteristic.isValid()) {
        qCWarning(dcSenic()) << "Input rotation characteristc not found for device " << bluetoothDevice()->name() << bluetoothDevice()->address().toString();
        return;
    }
    // Enable notifications
    notificationDescriptor = m_inputRotationCharacteristic.descriptor(QBluetoothUuid::ClientCharacteristicConfiguration);
    m_inputService->writeDescriptor(notificationDescriptor, QByteArray::fromHex("0100"));
}

void LukeRoberts::onInputCharacteristicChanged(const QLowEnergyCharacteristic &characteristic, const QByteArray &value)
{
    if (characteristic.uuid() == m_inputButtonCharacteristic.uuid()) {
        bool pressed = (bool)value.toHex().toUInt(nullptr, 16);
        qCDebug(dcSenic()) << "Button:" << (pressed ? "pressed": "released");
        if (pressed) {
            m_longPressTimer->start(m_longPressTime);
        } else {
            if (m_longPressTimer->isActive()) {
                m_longPressTimer->stop();
                emit buttonPressed();
            }
            // else the time run out and has the long pressed event emittted
        }
        return;
    }

    if (characteristic.uuid() == m_inputSwipeCharacteristic.uuid()) {
        quint8 swipe = (quint8)value.toHex().toUInt(nullptr, 16);
        switch (swipe) {
        case 0:
            qCDebug(dcSenic()) << "Swipe: Left";
            emit swipeDetected(SwipeDirectionLeft);
            break;
        case 1:
            qCDebug(dcSenic()) << "Swipe: Right";
            emit swipeDetected(SwipeDirectionRight);
            break;
        case 2:
            qCDebug(dcSenic()) << "Swipe: Up";
            emit swipeDetected(SwipeDirectionUp);
            break;
        case 3:
            qCDebug(dcSenic()) << "Swipe: Down";
            emit swipeDetected(SwipeDirectionDown);
            break;
        default:
            break;
        }
        return;
    }

    if (characteristic.uuid() == m_inputRotationCharacteristic.uuid()) {
        qint16 intValue = qFromLittleEndian<quint16>((uchar *)value.constData());
        qCDebug(dcSenic()) << "Rotation" << value.toHex() << intValue;
        int finalValue = m_rotationValue + qRound(intValue / 10.0);
        if (finalValue <= 0) {
            m_rotationValue = 0;
        } else if (finalValue >= 100) {
            m_rotationValue = 100;
        } else {
            m_rotationValue = finalValue;
        }
        emit rotationValueChanged(m_rotationValue);
        return;
    }

    qCDebug(dcSenic()) << "Service characteristic changed" << characteristic.name() << value.toHex();
}

void LukeRoberts::onLedMatrixServiceStateChanged(const QLowEnergyService::ServiceState &state)
{
    // Only continue if discovered
    if (state != QLowEnergyService::ServiceDiscovered)
        return;

    qCDebug(dcSenic()) << "Led matrix service discovered.";

    printService(m_ledMatrixService);

    // Led matrix
    m_ledMatrixCharacteristic = m_ledMatrixService->characteristic(ledMatrixCharacteristicUuid);
    if (!m_ledMatrixCharacteristic.isValid()) {
        qCWarning(dcSenic()) << "Led matrix characteristc not found for device " << bluetoothDevice()->name() << bluetoothDevice()->address().toString();
        return;
    }
}
