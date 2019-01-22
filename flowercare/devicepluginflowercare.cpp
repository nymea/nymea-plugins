/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *                                                                         *
 *  Copyright (C) 2018 Michael Zanetti <michael.zanetti@guh.io>            *
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


/*
 * ServieUUIDs:
 * {00001204-0000-1000-8000-00805f9b34fb}
 * {00001206-0000-1000-8000-00805f9b34fb}
 * {00001800-0000-1000-8000-00805f9b34fb}
 * {00001801-0000-1000-8000-00805f9b34fb}
 * {0000fe95-0000-1000-8000-00805f9b34fb}
 * {0000fef5-0000-1000-8000-00805f9b34fb}
 */
#include "plugininfo.h"
#include "devicemanager.h"
#include "hardware/bluetoothlowenergy/bluetoothlowenergymanager.h"
#include "devicepluginflowercare.h"
#include "flowercare.h"

DevicePluginFlowercare::DevicePluginFlowercare()
{

}

DevicePluginFlowercare::~DevicePluginFlowercare()
{
    if (m_reconnectTimer) {
        hardwareManager()->pluginTimerManager()->unregisterTimer(m_reconnectTimer);
    }
}

void DevicePluginFlowercare::init()
{
}

DeviceManager::DeviceError DevicePluginFlowercare::discoverDevices(const DeviceClassId &deviceClassId, const ParamList &params)
{
    Q_UNUSED(params)
    Q_UNUSED(deviceClassId)

    if (!hardwareManager()->bluetoothLowEnergyManager()->available())
        return DeviceManager::DeviceErrorHardwareNotAvailable;

    if (!hardwareManager()->bluetoothLowEnergyManager()->enabled())
        return DeviceManager::DeviceErrorHardwareNotAvailable;

    BluetoothDiscoveryReply *reply = hardwareManager()->bluetoothLowEnergyManager()->discoverDevices();
    connect(reply, &BluetoothDiscoveryReply::finished, this, &DevicePluginFlowercare::onBluetoothDiscoveryFinished);
    return DeviceManager::DeviceErrorAsync;
}

DeviceManager::DeviceSetupStatus DevicePluginFlowercare::setupDevice(Device *device)
{
    qCDebug(dcFlowerCare) << "Setting up Flower care" << device->name() << device->params();

    if (device->deviceClassId() == flowerCareDeviceClassId) {
        QBluetoothAddress address = QBluetoothAddress(device->paramValue(flowerCareDeviceMacParamTypeId).toString());
        QString name = device->paramValue(flowerCareDeviceNameParamTypeId).toString();
        QBluetoothDeviceInfo deviceInfo = QBluetoothDeviceInfo(address, name, 0);

        BluetoothLowEnergyDevice *bluetoothDevice = hardwareManager()->bluetoothLowEnergyManager()->registerDevice(deviceInfo, QLowEnergyController::PublicAddress);
        FlowerCare *flowerCare = new FlowerCare(bluetoothDevice, this);
        connect(flowerCare, &FlowerCare::finished, this, &DevicePluginFlowercare::onSensorDataReceived);
        m_list.insert(device, flowerCare);

        m_refreshMinutes[flowerCare] = 0;

        if (!m_reconnectTimer) {
            m_reconnectTimer = hardwareManager()->pluginTimerManager()->registerTimer();
            connect(m_reconnectTimer, &PluginTimer::timeout, this, &DevicePluginFlowercare::onPluginTimer);
        }
        return DeviceManager::DeviceSetupStatusSuccess;
    }
    return DeviceManager::DeviceSetupStatusFailure;
}

void DevicePluginFlowercare::postSetupDevice(Device *device)
{
    FlowerCare *flowerCare = m_list.value(device);
    flowerCare->refreshData();
}

void DevicePluginFlowercare::deviceRemoved(Device *device)
{
    FlowerCare *flowerCare = m_list.take(device);
    if (!flowerCare) {
        return;
    }

    hardwareManager()->bluetoothLowEnergyManager()->unregisterDevice(flowerCare->btDevice());
    flowerCare->deleteLater();

    if (m_list.isEmpty() && m_reconnectTimer) {
        hardwareManager()->pluginTimerManager()->unregisterTimer(m_reconnectTimer);
        m_reconnectTimer = nullptr;
    }
}

DeviceManager::DeviceError DevicePluginFlowercare::executeAction(Device *device, const Action &action)
{
    Q_UNUSED(device)
    Q_UNUSED(action)
    return DeviceManager::DeviceErrorActionTypeNotFound;
}

bool DevicePluginFlowercare::verifyExistingDevices(const QBluetoothDeviceInfo &deviceInfo)
{
    foreach (Device *device, m_list.keys()) {
        if (device->paramValue(flowerCareDeviceMacParamTypeId).toString() == deviceInfo.address().toString())
            return true;
    }

    return false;
}

void DevicePluginFlowercare::onPluginTimer()
{

    foreach (FlowerCare *flowerCare, m_list) {
        if (--m_refreshMinutes[flowerCare] <= 0) {
            qCDebug(dcFlowerCare()) << "Refreshing" << flowerCare->btDevice()->address();
            flowerCare->refreshData();
        } else {
            qCDebug(dcFlowerCare()) << "Not refreshing" << flowerCare->btDevice()->address() << " Next refresh in" << m_refreshMinutes[flowerCare] << "minutes";
        }

        // If we had 2 or more failed connection attempts, mark it as disconnected
        if (m_refreshMinutes[flowerCare] < -2) {
            qCDebug(dcFlowerCare()) << "Failed to refresh for"<< (m_refreshMinutes[flowerCare] * -1) << "minutes. Marking as unreachable";
            m_list.key(flowerCare)->setStateValue(flowerCareConnectedStateTypeId, false);
        }
    }
}

void DevicePluginFlowercare::onBluetoothDiscoveryFinished()
{
    BluetoothDiscoveryReply *reply = static_cast<BluetoothDiscoveryReply *>(sender());
    if (reply->error() != BluetoothDiscoveryReply::BluetoothDiscoveryReplyErrorNoError) {
        qCWarning(dcFlowerCare()) << "Bluetooth discovery error:" << reply->error();
        reply->deleteLater();
        emit devicesDiscovered(flowerCareDeviceClassId, QList<DeviceDescriptor>());
        return;
    }

    QList<DeviceDescriptor> deviceDescriptors;
    qCDebug(dcFlowerCare()) << "Discovery finished";
    foreach (const QBluetoothDeviceInfo &deviceInfo, reply->discoveredDevices()) {
        qCDebug(dcFlowerCare()) << "Discovered device" << deviceInfo.name();
        if (deviceInfo.name().contains("Flower care")) {
            if (!verifyExistingDevices(deviceInfo)) {
                DeviceDescriptor descriptor(flowerCareDeviceClassId, "Flower Care", deviceInfo.address().toString());
                ParamList params;
                params.append(Param(flowerCareDeviceNameParamTypeId, deviceInfo.name()));
                params.append(Param(flowerCareDeviceMacParamTypeId, deviceInfo.address().toString()));
                descriptor.setParams(params);
                foreach (Device *existingDevice, myDevices()) {
                    if (existingDevice->paramValue(flowerCareDeviceMacParamTypeId).toString() == deviceInfo.address().toString()) {
                        descriptor.setDeviceId(existingDevice->id());
                        break;
                    }
                }
                deviceDescriptors.append(descriptor);
            }
        }
    }

    reply->deleteLater();
    emit devicesDiscovered(flowerCareDeviceClassId, deviceDescriptors);
}

void DevicePluginFlowercare::onSensorDataReceived(quint8 batteryLevel, double degreeCelsius, double lux, double moisture, double fertility)
{
    FlowerCare *flowerCare = static_cast<FlowerCare*>(sender());
    Device *device = m_list.key(flowerCare);
    device->setStateValue(flowerCareConnectedStateTypeId, true);
    device->setStateValue(flowerCareBatteryLevelStateTypeId, batteryLevel);
    device->setStateValue(flowerCareBatteryCriticalStateTypeId, batteryLevel <= 10);
    device->setStateValue(flowerCareTemperatureStateTypeId, degreeCelsius);
    device->setStateValue(flowerCareLightIntensityStateTypeId, lux);
    device->setStateValue(flowerCareMoistureStateTypeId, moisture);
    device->setStateValue(flowerCareConductivityStateTypeId, fertility);

    m_refreshMinutes[flowerCare] = 20;
}
