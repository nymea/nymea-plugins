/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *                                                                         *
 *  Copyright (C) 2016 Simon Stürz <simon.stuerz@guh.io>                   *
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

#include "nuimo.h"
#include "extern-plugininfo.h"

#include <QBitArray>
#include <QtEndian>

static QBluetoothUuid ledMatrinxServiceUuid             = QBluetoothUuid(QUuid("f29b1523-cb19-40f3-be5c-7241ecb82fd1"));
static QBluetoothUuid ledMatrixCharacteristicUuid       = QBluetoothUuid(QUuid("f29b1524-cb19-40f3-be5c-7241ecb82fd1"));

static QBluetoothUuid inputServiceUuid                  = QBluetoothUuid(QUuid("f29b1525-cb19-40f3-be5c-7241ecb82fd2"));
static QBluetoothUuid inputSwipeCharacteristicUuid      = QBluetoothUuid(QUuid("f29b1527-cb19-40f3-be5c-7241ecb82fd2"));
static QBluetoothUuid inputRotationCharacteristicUuid   = QBluetoothUuid(QUuid("f29b1528-cb19-40f3-be5c-7241ecb82fd2"));
static QBluetoothUuid inputButtonCharacteristicUuid     = QBluetoothUuid(QUuid("f29b1529-cb19-40f3-be5c-7241ecb82fd2"));


Nuimo::Nuimo(Device *device, BluetoothLowEnergyDevice *bluetoothDevice, QObject *parent) :
    QObject(parent),
    m_device(device),
    m_bluetoothDevice(bluetoothDevice)
{
    connect(m_bluetoothDevice, &BluetoothLowEnergyDevice::connectedChanged, this, &Nuimo::onConnectedChanged);
    connect(m_bluetoothDevice, &BluetoothLowEnergyDevice::servicesDiscoveryFinished, this, &Nuimo::onServiceDiscoveryFinished);
}

Device *Nuimo::device()
{
    return m_device;
}

BluetoothLowEnergyDevice *Nuimo::bluetoothDevice()
{
    return m_bluetoothDevice;
}

void Nuimo::showGuhLogo()
{
    QByteArray matrix(
                "         "
                "  *      "
                " **      "
                " *** **  "
                "  *****  "
                "   **    "
                "   **    "
                "    *    "
                "         ");

    showMatrix(matrix, 10);
}

void Nuimo::showArrowUp()
{
    QByteArray matrix(
                "    *    "
                "   ***   "
                "  * * *  "
                " *  *  * "
                "*   *   *"
                "    *    "
                "    *    "
                "    *    "
                "    *    ");

    showMatrix(matrix, 3);
}

void Nuimo::showArrowDown()
{
    QByteArray matrix(
                "    *    "
                "    *    "
                "    *    "
                "    *    "
                "*   *   *"
                " *  *  * "
                "  * * *  "
                "   ***   "
                "    *    ");

    showMatrix(matrix, 3);
}

void Nuimo::showMatrix(const QByteArray &matrix, const int &seconds)
{
    if (!m_ledMatrixService)
        return;

    QBitArray bits;
    bits.resize(81);
    for (int i = 0; i < matrix.size(); i++) {
        if (matrix.at(i) != ' ') {
            bits[i] = true;
        }
    }

    QByteArray bytes;
    bytes.resize(bits.count() / 8 + 1);
    bytes.fill(0);
    // Convert from QBitArray to QByteArray
    for(int b = 0; b < bits.count(); ++b)
        bytes[b / 8] = (bytes.at(b / 8) | ((bits[b] ? 1 : 0) << (b % 8)));

    bytes.append(QByteArray::fromHex("FF"));
    quint8 time = quint8(seconds * 10);
    bytes.append((char)time);
    m_ledMatrixService->writeCharacteristic(m_ledMatrixCharacteristic, bytes);
}

void Nuimo::printService(QLowEnergyService *service)
{
    foreach (const QLowEnergyCharacteristic &characteristic, service->characteristics()) {
        qCDebug(dcSenic()) << "    -->" << characteristic.name() << characteristic.uuid().toString() << characteristic.value();
        foreach (const QLowEnergyDescriptor &desciptor, characteristic.descriptors()) {
            qCDebug(dcSenic()) << "        -->" << desciptor.name() << desciptor.uuid().toString() << desciptor.value();
        }
    }
}

void Nuimo::setBatteryValue(const QByteArray &data)
{
    int batteryPercentage = data.toHex().toUInt(0, 16);
    qCDebug(dcSenic()) << "Battery:" << batteryPercentage << "%";

    device()->setStateValue(nuimoBatteryStateTypeId, batteryPercentage);
}

void Nuimo::onConnectedChanged(const bool &connected)
{
    qCDebug(dcSenic()) << m_bluetoothDevice->name() << m_bluetoothDevice->address().toString() << (connected ? "connected" : "disconnected");
    m_device->setStateValue(nuimoConnectedStateTypeId, connected);

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

void Nuimo::onServiceDiscoveryFinished()
{
    qCDebug(dcSenic()) << "Service scan finised";

    if (!m_bluetoothDevice->serviceUuids().contains(QBluetoothUuid::DeviceInformation)) {
        qCWarning(dcSenic()) << "Device Information service not found for device" << bluetoothDevice()->name() << bluetoothDevice()->address().toString();
        return;
    }

    if (!m_bluetoothDevice->serviceUuids().contains(QBluetoothUuid::BatteryService)) {
        qCWarning(dcSenic()) << "Battery service not found for device" << bluetoothDevice()->name() << bluetoothDevice()->address().toString();
        return;
    }

    if (!m_bluetoothDevice->serviceUuids().contains(ledMatrinxServiceUuid)) {
        qCWarning(dcSenic()) << "Led matrix service not found for device" << bluetoothDevice()->name() << bluetoothDevice()->address().toString();
        return;
    }

    if (!m_bluetoothDevice->serviceUuids().contains(inputServiceUuid)) {
        qCWarning(dcSenic()) << "Input service not found for device" << bluetoothDevice()->name() << bluetoothDevice()->address().toString();
        return;
    }

    // Device info service
    if (!m_deviceInfoService) {
        m_deviceInfoService = m_bluetoothDevice->controller()->createServiceObject(QBluetoothUuid::DeviceInformation, this);
        if (!m_deviceInfoService) {
            qCWarning(dcSenic()) << "Could not create device info service.";
            return;
        }

        connect(m_deviceInfoService, &QLowEnergyService::stateChanged, this, &Nuimo::onDeviceInfoServiceStateChanged);

        if (m_deviceInfoService->state() == QLowEnergyService::DiscoveryRequired) {
            m_deviceInfoService->discoverDetails();
        }
    }

    // Battery service
    if (!m_batteryService) {
        m_batteryService = m_bluetoothDevice->controller()->createServiceObject(QBluetoothUuid::BatteryService, this);
        if (!m_batteryService) {
            qCWarning(dcSenic()) << "Could not create battery service.";
            return;
        }

        connect(m_batteryService, &QLowEnergyService::stateChanged, this, &Nuimo::onBatteryServiceStateChanged);
        connect(m_batteryService, &QLowEnergyService::characteristicChanged, this, &Nuimo::onBatteryCharacteristicChanged);

        if (m_batteryService->state() == QLowEnergyService::DiscoveryRequired) {
            m_batteryService->discoverDetails();
        }
    }

    // Input service
    if (!m_inputService) {
        m_inputService = m_bluetoothDevice->controller()->createServiceObject(inputServiceUuid, this);
        if (!m_inputService) {
            qCWarning(dcSenic()) << "Could not create input service.";
            return;
        }

        connect(m_inputService, &QLowEnergyService::stateChanged, this, &Nuimo::onInputServiceStateChanged);
        connect(m_inputService, &QLowEnergyService::characteristicChanged, this, &Nuimo::onInputCharacteristicChanged);

        if (m_inputService->state() == QLowEnergyService::DiscoveryRequired) {
            m_inputService->discoverDetails();
        }
    }

    // Input service
    if (!m_ledMatrixService) {
        m_ledMatrixService = m_bluetoothDevice->controller()->createServiceObject(ledMatrinxServiceUuid, this);
        if (!m_ledMatrixService) {
            qCWarning(dcSenic()) << "Could not create led matrix service.";
            return;
        }

        connect(m_ledMatrixService, &QLowEnergyService::stateChanged, this, &Nuimo::onLedMatrixServiceStateChanged);

        if (m_ledMatrixService->state() == QLowEnergyService::DiscoveryRequired) {
            m_ledMatrixService->discoverDetails();
        }
    }

}

void Nuimo::onDeviceInfoServiceStateChanged(const QLowEnergyService::ServiceState &state)
{
    // Only continue if discovered
    if (state != QLowEnergyService::ServiceDiscovered)
        return;

    qCDebug(dcSenic()) << "Device info service discovered.";

    printService(m_deviceInfoService);

    device()->setStateValue(nuimoFirmwareRevisionStateTypeId, QString::fromUtf8(m_deviceInfoService->characteristic(QBluetoothUuid::FirmwareRevisionString).value()));
    device()->setStateValue(nuimoHardwareRevisionStateTypeId, QString::fromUtf8(m_deviceInfoService->characteristic(QBluetoothUuid::HardwareRevisionString).value()));
    device()->setStateValue(nuimoSoftwareRevisionStateTypeId, QString::fromUtf8(m_deviceInfoService->characteristic(QBluetoothUuid::SoftwareRevisionString).value()));

}

void Nuimo::onBatteryServiceStateChanged(const QLowEnergyService::ServiceState &state)
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

    setBatteryValue(m_batteryCharacteristic.value());
}

void Nuimo::onBatteryCharacteristicChanged(const QLowEnergyCharacteristic &characteristic, const QByteArray &value)
{
    if (characteristic.uuid() == m_batteryCharacteristic.uuid()) {
        setBatteryValue(value);
    }
}

void Nuimo::onInputServiceStateChanged(const QLowEnergyService::ServiceState &state)
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

void Nuimo::onInputCharacteristicChanged(const QLowEnergyCharacteristic &characteristic, const QByteArray &value)
{
    if (characteristic.uuid() == m_inputButtonCharacteristic.uuid()) {
        bool pressed = (bool)value.toHex().toUInt(0, 16);
        qCDebug(dcSenic()) << "Button:" << (pressed ? "pressed": "released");
        if (pressed) {
            emit buttonPressed();
        } else {
            emit buttonReleased();
        }
        return;
    }

    if (characteristic.uuid() == m_inputSwipeCharacteristic.uuid()) {
        quint8 swipe = (quint8)value.toHex().toUInt(0, 16);
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

void Nuimo::onLedMatrixServiceStateChanged(const QLowEnergyService::ServiceState &state)
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
