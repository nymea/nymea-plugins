/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *                                                                         *
 *  Copyright (C) 2019 Bernhard Trinnes <bernhard.trinnes@nymea.io>        *
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

#include "lukeroberts.h"
#include "extern-plugininfo.h"

#include <QBitArray>
#include <QtEndian>

static QBluetoothUuid customControlServiceUuid             = QBluetoothUuid(QUuid("​44092840-0567-11E6-B862-0002A5D5C51B"));
static QBluetoothUuid externalApiEndpointCharacteristicUuid       = QBluetoothUuid(QUuid("44092842-0567-11E6-B862-0002A5D5C51B"));


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
        qCDebug(dcLukeRoberts()) << "    -->" << characteristic.name() << characteristic.uuid().toString() << characteristic.value();
        foreach (const QLowEnergyDescriptor &descriptor, characteristic.descriptors()) {
            qCDebug(dcLukeRoberts()) << "        -->" << descriptor.name() << descriptor.uuid().toString() << descriptor.value();
        }
    }
}


void LukeRoberts::onConnectedChanged(bool connected)
{
    qCDebug(dcLukeRoberts()) << m_bluetoothDevice->name() << m_bluetoothDevice->address().toString() << (connected ? "connected" : "disconnected");

    emit connectedChanged(connected);

    if (!connected) {
        // Clean up services
        m_deviceInfoService->deleteLater();
        m_controlService->deleteLater();

        m_deviceInfoService = nullptr;
        m_controlService = nullptr;
    }

}

void LukeRoberts::onServiceDiscoveryFinished()
{
    qCDebug(dcLukeRoberts()) << "Service scan finised";

    if (!m_bluetoothDevice->serviceUuids().contains(QBluetoothUuid::DeviceInformation)) {
        qCWarning(dcLukeRoberts()) << "Device Information service not found for device" << bluetoothDevice()->name() << bluetoothDevice()->address().toString();
        emit deviceInitializationFinished(false);
        return;
    }

    if (!m_bluetoothDevice->serviceUuids().contains(customControlServiceUuid)) {
        qCWarning(dcLukeRoberts()) << "Input service not found for device" << bluetoothDevice()->name() << bluetoothDevice()->address().toString();
        emit deviceInitializationFinished(false);
        return;
    }

    // Device info service
    if (!m_deviceInfoService) {
        m_deviceInfoService = m_bluetoothDevice->controller()->createServiceObject(QBluetoothUuid::DeviceInformation, this);
        if (!m_deviceInfoService) {
            qCWarning(dcLukeRoberts()) << "Could not create device info service.";
            emit deviceInitializationFinished(false);
            return;
        }

        connect(m_deviceInfoService, &QLowEnergyService::stateChanged, this, &LukeRoberts::onDeviceInfoServiceStateChanged);

        if (m_deviceInfoService->state() == QLowEnergyService::DiscoveryRequired) {
            m_deviceInfoService->discoverDetails();
        }
    }

    // Custom control service
    if (!m_controlService) {
        m_controlService= m_bluetoothDevice->controller()->createServiceObject(customControlServiceUuid, this);
        if (!m_controlService) {
            qCWarning(dcLukeRoberts()) << "Could not create led matrix service.";
            emit deviceInitializationFinished(false);
            return;
        }

        connect(m_controlService, &QLowEnergyService::stateChanged, this, &LukeRoberts::onControlServiceChanged);

        if (m_controlService->state() == QLowEnergyService::DiscoveryRequired) {
            m_controlService->discoverDetails();
        }
    }
    emit deviceInitializationFinished(true);
}

void LukeRoberts::onDeviceInfoServiceStateChanged(const QLowEnergyService::ServiceState &state)
{
    // Only continue if discovered
    if (state != QLowEnergyService::ServiceDiscovered)
        return;

    qCDebug(dcLukeRoberts()) << "Device info service discovered.";

    printService(m_deviceInfoService);
    QString firmware = QString::fromUtf8(m_deviceInfoService->characteristic(QBluetoothUuid::FirmwareRevisionString).value());
    QString hardware = QString::fromUtf8(m_deviceInfoService->characteristic(QBluetoothUuid::HardwareRevisionString).value());
    QString software = QString::fromUtf8(m_deviceInfoService->characteristic(QBluetoothUuid::SoftwareRevisionString).value());

    emit deviceInformationChanged(firmware, hardware, software);
}

void LukeRoberts::onControlServiceChanged(const QLowEnergyService::ServiceState &state)
{
    // Only continue if discovered
    if (state != QLowEnergyService::ServiceDiscovered)
        return;

    qCDebug(dcLukeRoberts()) << "Custom control service discovered.";

    printService(m_controlService);

    // Custom API endpoint
    m_externalApiEndpoint = m_controlService->characteristic(externalApiEndpointCharacteristicUuid);
    if (!m_externalApiEndpoint.isValid()) {
        qCWarning(dcLukeRoberts()) << "Input button characteristc not found for device " << bluetoothDevice()->name() << bluetoothDevice()->address().toString();
        return;
    }
    // Enable notifications
    QLowEnergyDescriptor notificationDescriptor = m_externalApiEndpoint.descriptor(QBluetoothUuid::ClientCharacteristicConfiguration);
    m_controlService->writeDescriptor(notificationDescriptor, QByteArray::fromHex("0100"));
}

void LukeRoberts::onExternalApiEndpointCharacteristicChanged(const QLowEnergyCharacteristic &characteristic, const QByteArray &value)
{
    if (characteristic.uuid() == m_externalApiEndpoint.uuid()) {
        qCDebug(dcLukeRoberts()) << "Data received" << value;
    }

    qCDebug(dcLukeRoberts()) << "Service characteristic changed" << characteristic.name() << value.toHex();
}
