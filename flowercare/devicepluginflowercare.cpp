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

void DevicePluginFlowercare::discoverDevices(DeviceDiscoveryInfo *info)
{
    if (!hardwareManager()->bluetoothLowEnergyManager()->available())
        return info->finish(Device::DeviceErrorHardwareNotAvailable, QT_TR_NOOP("Cannot discover Bluetooth devices. Bluetooth is not available on this system."));

    if (!hardwareManager()->bluetoothLowEnergyManager()->enabled())
        return info->finish(Device::DeviceErrorHardwareNotAvailable, QT_TR_NOOP("Cannot discover Bluetooth devices. Bluetooth is disabled."));

    BluetoothDiscoveryReply *reply = hardwareManager()->bluetoothLowEnergyManager()->discoverDevices();

    connect(reply, &BluetoothDiscoveryReply::finished, info, [this, info, reply](){
        reply->deleteLater();

        if (reply->error() != BluetoothDiscoveryReply::BluetoothDiscoveryReplyErrorNoError) {
            qCWarning(dcFlowerCare()) << "Bluetooth discovery error:" << reply->error();
            info->finish(Device::DeviceErrorHardwareFailure, QT_TR_NOOP("An error happened during Bluetooth discovery."));
            return;
        }

        qCDebug(dcFlowerCare()) << "Discovery finished";

        foreach (const QBluetoothDeviceInfo &deviceInfo, reply->discoveredDevices()) {
            qCDebug(dcFlowerCare()) << "Discovered device" << deviceInfo.name();
            if (deviceInfo.name().contains("Flower care")) {
                if (!verifyExistingDevices(deviceInfo)) {
                    DeviceDescriptor descriptor(flowerCareDeviceClassId, deviceInfo.name(), deviceInfo.address().toString());
                    ParamList params;
                    params << Param(flowerCareDeviceMacParamTypeId, deviceInfo.address().toString());
                    descriptor.setParams(params);
                    foreach (Device *existingDevice, myDevices()) {
                        if (existingDevice->paramValue(flowerCareDeviceMacParamTypeId).toString() == deviceInfo.address().toString()) {
                            descriptor.setDeviceId(existingDevice->id());
                            break;
                        }
                    }
                    info->addDeviceDescriptor(descriptor);
                }
            }
        }

        info->finish(Device::DeviceErrorNoError);
    });

}

void DevicePluginFlowercare::setupDevice(DeviceSetupInfo *info)
{
    Device *device = info->device();

    qCDebug(dcFlowerCare) << "Setting up Flower care" << device->name() << device->params();

    QBluetoothAddress address = QBluetoothAddress(device->paramValue(flowerCareDeviceMacParamTypeId).toString());
    QBluetoothDeviceInfo deviceInfo = QBluetoothDeviceInfo(address, device->name(), 0);

    BluetoothLowEnergyDevice *bluetoothDevice = hardwareManager()->bluetoothLowEnergyManager()->registerDevice(deviceInfo, QLowEnergyController::PublicAddress);
    FlowerCare *flowerCare = new FlowerCare(bluetoothDevice, this);
    connect(flowerCare, &FlowerCare::finished, this, &DevicePluginFlowercare::onSensorDataReceived);
    m_list.insert(device, flowerCare);

    m_refreshMinutes[flowerCare] = 0;

    if (!m_reconnectTimer) {
        m_reconnectTimer = hardwareManager()->pluginTimerManager()->registerTimer();
        connect(m_reconnectTimer, &PluginTimer::timeout, this, &DevicePluginFlowercare::onPluginTimer);
    }

    // Update refresh schedule when the refresh rate setting is changed
    connect(device, &Device::settingChanged, flowerCare, [this, device] {
        FlowerCare *flowerCare = m_list.value(device);
        int refreshInterval = device->setting(flowerCareSettingsRefreshRateParamTypeId).toInt();
        if (m_refreshMinutes[flowerCare] > refreshInterval) {
            m_refreshMinutes[flowerCare] = refreshInterval;
        }
    });

    info->finish(Device::DeviceErrorNoError);
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

    m_refreshMinutes[flowerCare] = device->setting(flowerCareSettingsRefreshRateParamTypeId).toInt();
}
