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

void LukeRoberts::ping()
{
    QByteArray data;
    data.append(0xa0);
    data.append(0x02);
    data.resize(3); //appending 0 not allowed
    m_controlService->writeCharacteristic(m_externalApiEndpoint, data);
}

void LukeRoberts::queryScene(uint8_t id)
{
    QByteArray data;
    data.append(0xa0);
    data.append(0x01);
    data.append(0x01);
    data << id;
    m_controlService->writeCharacteristic(m_externalApiEndpoint, data);
}

void LukeRoberts::getSceneList()
{
    m_sceneList.clear();
    queryScene(0); //get first scene
}


/*
 * duration in ms, 0 for infinite
 * saturation 0 .. 255
 * hue 0 .. 65535 - kelvin 2700 .. 4000 for white light when ​SS​= 0
 * brightness 0 .. 255
*/
void LukeRoberts::setImmediateLight(uint16_t duration, uint8_t saturation, uint16_t hue, uint8_t brightness)
{
    QByteArray data;
    data.append(0xa0);
    data.append(0x01);
    data.append(0x02);
    data.append(0x02);
    data << duration;
    data << saturation;
    data << hue;
    data << brightness;
    m_controlService->writeCharacteristic(m_externalApiEndpoint, data);
}

/*
 * Lowers the brightness of the currently selected light scene.
 * This modification is reverted when the user selects a different scene via app or Click Detection.
 */
void LukeRoberts::modifyBrightness(uint8_t percent)
{
    QByteArray data;
    data.append(0xa0);
    data.append(0x01);
    data.append(0x03);
    data << percent;
    m_controlService->writeCharacteristic(m_externalApiEndpoint, data);
}

void LukeRoberts::modifyColorTemperature(uint16_t kelvin)
{
    QByteArray data;
    data.append(0xa0); //Prefix
    data.append(0x01); //Protocol version V1
    data.append(0x04); //Opcode "Color Temperature"
    data << kelvin;
    m_controlService->writeCharacteristic(m_externalApiEndpoint, data);
}


/*
 * Send 0xFF as ​II​to select the default scene, e.g. the one that would also appear when
 * powering up the lamp.
*/
void LukeRoberts::selectScene(uint8_t id)
{
    QByteArray data;
    data.append(0xa0); //Prefix
    data.append(0x02); //Protocol version V2
    data.append(0x05); //Opcode "Select Scene"
    data << id;
    m_controlService->writeCharacteristic(m_externalApiEndpoint, data);
}

void LukeRoberts::setNextSceneByBrightness(int8_t direction)
{
    QByteArray data;
    data.append(0xa0); //Prefix
    data.append(0x02); //Protocol version V2
    data.append(0x06); //Opcode "Next Scene by Brightness"
    data.append(direction); //1 selects the next brighter scene and -1 selects the next less bright scene.
    m_controlService->writeCharacteristic(m_externalApiEndpoint, data);
}

/*
* Increments or decrements the currently visible color temperature in the Downlight part of the selected scene.
*/
void LukeRoberts::adjustColorTemperature(uint16_t kelvinIncrement)
{
    QByteArray data;
    data.append(0xa0); //Prefix
    data.append(0x02); //Protocol version V2
    data.append(0x07); //Opcode "Adjust Color Temperature"
    data << kelvinIncrement;
    m_controlService->writeCharacteristic(m_externalApiEndpoint, data);
}

/*
 * Scales the brightness of the current scene by multiplication.
 * As opposed to ​03 Modify Brightness​, this command respects the brightness set by previous 03 and 08 commands.
*/
void LukeRoberts::setRelativeBrightness(uint8_t percent)
{
    QByteArray data;
    data.append(0xa0); //Prefix
    data.append(0x02); //Protocol version V2
    data.append(0x08); //Opcode "Relative Brightness"
    data << percent;
    m_controlService->writeCharacteristic(m_externalApiEndpoint, data);
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
    if (value.length() > 4) { //its a scene


        if (value.at(2) != 0xff) { //check it we are already at the end of the list
            queryScene(value.at(2));
        } else {
            emit sceneListReceived(m_sceneList);
        }

    }


    qCDebug(dcLukeRoberts()) << "Service characteristic changed" << characteristic.name() << value.toHex();
}
