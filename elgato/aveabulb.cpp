/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
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

#include "aveabulb.h"
#include "extern-plugininfo.h"

AveaBulb::AveaBulb(Device *device, BluetoothLowEnergyDevice *bluetoothDevice, QObject *parent) :
    QObject(parent),
    m_device(device),
    m_bluetoothDevice(bluetoothDevice)
{
    connect(m_bluetoothDevice, &BluetoothLowEnergyDevice::connectedChanged, this, &AveaBulb::onConnectedChanged);
    connect(m_bluetoothDevice, &BluetoothLowEnergyDevice::servicesDiscoveryFinished, this, &AveaBulb::onServiceDiscoveryFinished);
}

Device *AveaBulb::device()
{
    return m_device;
}

BluetoothLowEnergyDevice *AveaBulb::bluetoothDevice()
{
    return m_bluetoothDevice;
}

bool AveaBulb::setColor(const QColor &color)
{
    if (!m_bluetoothDevice->connected())
        return false;

    if (!m_colorService)
        return false;

    // Convert rgb to wrgb
    QByteArray command;
    command.append(QByteArray::fromHex("35"));

    qCDebug(dcElgato()) << color << command;

    return true;
}

void AveaBulb::onConnectedChanged(const bool &connected)
{
    qCDebug(dcElgato()) << "Bulb" << m_bluetoothDevice->name() << m_bluetoothDevice->address().toString() << (connected ? "connected" : "disconnected");
    m_device->setStateValue(aveaConnectedStateTypeId, connected);

    if (!connected) {
        // Clean up services
        m_colorService->deleteLater();
        //m_imageService->deleteLater();

        m_colorService = nullptr;
        m_imageService = nullptr;
    }

}

void AveaBulb::onServiceDiscoveryFinished()
{
    qCDebug(dcElgato()) << "Service discovery finished";

    if (!m_bluetoothDevice->serviceUuids().contains(colorServiceUuid)) {
        qCWarning(dcElgato()) << "Could not find color service";
        return;
    }

    if (!m_bluetoothDevice->serviceUuids().contains(imageServiceUuid)) {
        qCWarning(dcElgato()) << "Could not find image service";
        return;
    }

    // Color service
    if (!m_colorService) {
        m_colorService = m_bluetoothDevice->controller()->createServiceObject(colorServiceUuid, this);
        if (!m_colorService) {
            qCWarning(dcElgato()) << "Could not create color service.";
            return;
        }

        connect(m_colorService, &QLowEnergyService::stateChanged, this, &AveaBulb::onColorServiceStateChanged);
        connect(m_colorService, &QLowEnergyService::characteristicChanged, this, &AveaBulb::onColorServiceCharacteristicChanged);

        m_colorService->discoverDetails();
    }

}

void AveaBulb::onColorServiceStateChanged(const QLowEnergyService::ServiceState &state)
{
    // Only continue if discovered
    if (state != QLowEnergyService::ServiceDiscovered)
        return;

    qCDebug(dcElgato()) << "Color service discovered.";

    foreach (const QLowEnergyCharacteristic &characteristic, m_colorService->characteristics()) {
        qCDebug(dcElgato()) << "    -->" << characteristic.name() << characteristic.uuid().toString() << characteristic.value();
        foreach (const QLowEnergyDescriptor &desciptor, characteristic.descriptors()) {
            qCDebug(dcElgato()) << "        -->" << desciptor.name() << desciptor.uuid().toString() << desciptor.value();
        }
    }

    // Data characteristic
    m_colorCharacteristic = m_colorService->characteristic(QBluetoothUuid(QUuid("f815e811-456c-6761-746f-4d756e696368")));
    if (!m_colorCharacteristic.isValid()) {
        qCWarning(dcElgato()) << "Invalid color data characteristic.";
    }

    // Enable notifications
    QLowEnergyDescriptor notificationDescriptor = m_colorCharacteristic.descriptor(QBluetoothUuid::ClientCharacteristicConfiguration);
    m_colorService->writeDescriptor(notificationDescriptor, QByteArray::fromHex("0100"));

    // Get current configuration

    // Color
    m_colorService->writeCharacteristic(m_colorCharacteristic, QByteArray::fromHex("35"));

    // Brightness
    m_colorService->writeCharacteristic(m_colorCharacteristic, QByteArray::fromHex("57"));

    // Name
    m_colorService->writeCharacteristic(m_colorCharacteristic, QByteArray::fromHex("58"));

}

void AveaBulb::onColorServiceCharacteristicChanged(const QLowEnergyCharacteristic &characteristic, const QByteArray &value)
{
    qCDebug(dcElgato()) << "Color characteristic changed" << characteristic.uuid().toString() << value;

    if (value.startsWith(QByteArray::fromHex("35"))) {
        qCDebug(dcElgato()) << "Received color notification";
        qCDebug(dcElgato()) << "    Fade" << value.mid(1, 2);
        qCDebug(dcElgato()) << "    Fixed value" << value.mid(3, 2);
        qCDebug(dcElgato()) << "    White" << value.mid(5, 2);
        qCDebug(dcElgato()) << "    Red" << value.mid(7, 2);
        qCDebug(dcElgato()) << "    Green" << value.mid(9, 2);
        qCDebug(dcElgato()) << "    Blue" << value.mid(11, 2);
    } else if (value.startsWith(QByteArray::fromHex("57"))) {
        qCDebug(dcElgato()) << "Received brightness notification";
        qCDebug(dcElgato()) << "    Fade" << value.mid(1, 2);
    } else if (value.startsWith(QByteArray::fromHex("58"))) {
        qCDebug(dcElgato()) << "Received name notification";
        qCDebug(dcElgato()) << "    Name" << value.mid(1, value.count() - 2);
    }
}

