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

#include "devicepluginlukeroberts.h"
#include "devices/device.h"
#include "plugininfo.h"
#include "hardware/bluetoothlowenergy/bluetoothlowenergymanager.h"

#include <QColor>
#include <QRgb>

DevicePluginLukeRoberts::DevicePluginLukeRoberts()
{

}

void DevicePluginLukeRoberts::init()
{
}


void DevicePluginLukeRoberts::discoverDevices(DeviceDiscoveryInfo *info)
{
    if (!hardwareManager()->bluetoothLowEnergyManager()->available())
        return info->finish(Device::DeviceErrorHardwareNotAvailable, QT_TR_NOOP("Bluetooth is not available on this system."));

    if (!hardwareManager()->bluetoothLowEnergyManager()->enabled())
        return info->finish(Device::DeviceErrorHardwareNotAvailable, QT_TR_NOOP("Bluetooth is disabled. Please enable Bluetooth and try again."));

    BluetoothDiscoveryReply *reply = hardwareManager()->bluetoothLowEnergyManager()->discoverDevices();
    connect(reply, &BluetoothDiscoveryReply::finished, reply, &BluetoothDiscoveryReply::deleteLater);

    connect(reply, &BluetoothDiscoveryReply::finished, info, [this, info, reply](){

        if (reply->error() != BluetoothDiscoveryReply::BluetoothDiscoveryReplyErrorNoError) {
            qCWarning(dcLukeRoberts()) << "Bluetooth discovery error:" << reply->error();
            info->finish(Device::DeviceErrorHardwareFailure, QT_TR_NOOP("An error happened during Bluetooth discovery."));
            return;
        }

        foreach (const QBluetoothDeviceInfo &deviceInfo, reply->discoveredDevices()) {
            if (deviceInfo.serviceUuids().contains(customControlServiceUuid)) {
                DeviceDescriptor descriptor(modelFDeviceClassId, "Model F", deviceInfo.name() + " (" + deviceInfo.address().toString() + ")");
                ParamList params;

                foreach (Device *existingDevice, myDevices()) {
                    if (existingDevice->paramValue(modelFDeviceMacParamTypeId).toString() == deviceInfo.address().toString()) {
                        descriptor.setDeviceId(existingDevice->id());
                        break;
                    }
                }
                params.append(Param(modelFDeviceMacParamTypeId, deviceInfo.address().toString()));
                descriptor.setParams(params);
                info->addDeviceDescriptor(descriptor);
            }
        }
        info->finish(Device::DeviceErrorNoError);
    });
}


void DevicePluginLukeRoberts::setupDevice(DeviceSetupInfo *info)
{
    Device *device = info->device();

    qCDebug(dcLukeRoberts()) << "Setup device" << device->name() << device->params();

    QBluetoothAddress address = QBluetoothAddress(device->paramValue(modelFDeviceMacParamTypeId).toString());
    QBluetoothDeviceInfo deviceInfo = QBluetoothDeviceInfo(address, device->name(), 0);

    BluetoothLowEnergyDevice *bluetoothDevice = hardwareManager()->bluetoothLowEnergyManager()->registerDevice(deviceInfo, QLowEnergyController::RandomAddress);

    LukeRoberts *lamp = new LukeRoberts(bluetoothDevice, this);
    connect(lamp, &LukeRoberts::connectedChanged, this, &DevicePluginLukeRoberts::onConnectedChanged);
    connect(lamp, &LukeRoberts::deviceInformationChanged, this, &DevicePluginLukeRoberts::onDeviceInformationChanged);

    m_lamps.insert(lamp, device);

    connect(lamp, &LukeRoberts::deviceInitializationFinished, info, [this, info, lamp](bool success){
        Device *device = info->device();

        if (!device->setupComplete()) {
            if (success) {
                info->finish(Device::DeviceErrorNoError);
            } else {
                m_lamps.take(lamp);

                hardwareManager()->bluetoothLowEnergyManager()->unregisterDevice(lamp->bluetoothDevice());
                lamp->deleteLater();

                info->finish(Device::DeviceErrorHardwareFailure, QT_TR_NOOP("Error connecting to lamp."));
            }
        }

    });
    lamp->bluetoothDevice()->connectDevice();
}

void DevicePluginLukeRoberts::postSetupDevice(Device *device)
{
    Q_UNUSED(device)

    if (!m_reconnectTimer) {
        m_reconnectTimer = hardwareManager()->pluginTimerManager()->registerTimer(10);
        connect(m_reconnectTimer, &PluginTimer::timeout, this, &DevicePluginLukeRoberts::onReconnectTimeout);
    }
}


void DevicePluginLukeRoberts::executeAction(DeviceActionInfo *info)
{
    Device *device = info->device();
    Action action = info->action();

    QPointer<LukeRoberts> lamp = m_lamps.key(device);
    if (lamp.isNull())
        return info->finish(Device::DeviceErrorHardwareFailure);

    if (!lamp->bluetoothDevice()->connected()) {
        return info->finish(Device::DeviceErrorHardwareNotAvailable);
    }

    if (action.actionTypeId() == modelFPowerActionTypeId) {
        bool power = action.param(modelFPowerActionPowerParamTypeId).value().toBool();
        if (!power) {
            lamp->selectScene(0); //Scene 0 is the off scene
        } else {
            lamp->selectScene(0xff); // Scene 0xff is the default scene
        }

    } else if (action.actionTypeId() == modelFBrightnessActionTypeId) {
        int brightness = action.param(modelFBrightnessActionBrightnessParamTypeId).value().toInt();
        lamp->modifyBrightness(brightness);

    } else if (action.actionTypeId() == modelFColorActionTypeId) {
        QColor rgb = QColor::fromRgb(QRgb(action.param(modelFColorActionColorParamTypeId).value().toInt()));
        lamp->setImmediateLight(0, rgb.saturation(), rgb.hue(), rgb.lightness());

    } else if (action.actionTypeId() == modelFColorTemperatureActionTypeId) {
        int kelvin = action.param(modelFColorTemperatureActionColorTemperatureParamTypeId).value().toInt();
        lamp->modifyColorTemperature(kelvin);
    }
}


void DevicePluginLukeRoberts::deviceRemoved(Device *device)
{
    if (!m_lamps.values().contains(device))
        return;

    LukeRoberts *lamp = m_lamps.key(device);
    m_lamps.take(lamp);

    hardwareManager()->bluetoothLowEnergyManager()->unregisterDevice(lamp->bluetoothDevice());
    lamp->deleteLater();

    if (myDevices().isEmpty()) {
        hardwareManager()->pluginTimerManager()->unregisterTimer(m_reconnectTimer);
        m_reconnectTimer = nullptr;
    }
}

void DevicePluginLukeRoberts::browseDevice(BrowseResult *result)
{
    LukeRoberts *lamp = m_lamps.key(result->device());
    if (!lamp) {
        result->finish(Device::DeviceErrorHardwareNotAvailable);
        return;
    }
    qDebug(dcLukeRoberts()) << "Browse device called";
    m_pendingBrowseResults.insert(lamp, result);
    lamp->getSceneList();
}

void DevicePluginLukeRoberts::browserItem(BrowserItemResult *result)
{
    LukeRoberts *lamp = m_lamps.key(result->device());
    if (!lamp) {
        result->finish(Device::DeviceErrorHardwareNotAvailable);
        return;
    }
    qDebug(dcLukeRoberts()) << "Browse item called";

}

void DevicePluginLukeRoberts::executeBrowserItem(BrowserActionInfo *info)
{
    LukeRoberts *lamp = m_lamps.key(info->device());
    if (!lamp) {
        info->finish(Device::DeviceErrorHardwareNotAvailable);
        return;
    }
    lamp->selectScene(info->browserAction().itemId().toInt());
    info->finish(Device::DeviceErrorNoError);
}

void DevicePluginLukeRoberts::executeBrowserItemAction(BrowserItemActionInfo *info)
{
    LukeRoberts *lamp = m_lamps.key(info->device());
    if (!lamp) {
        info->finish(Device::DeviceErrorHardwareNotAvailable);
        return;
    }
}

void DevicePluginLukeRoberts::onPluginConfigurationChanged(const ParamTypeId &paramTypeId, const QVariant &value)
{
    Q_UNUSED(paramTypeId)
    Q_UNUSED(value)
}


void DevicePluginLukeRoberts::onReconnectTimeout()
{
    foreach (LukeRoberts *lamp, m_lamps.keys()) {
        if (!lamp->bluetoothDevice()->connected()) {
            lamp->bluetoothDevice()->connectDevice();
        }
    }
}

void DevicePluginLukeRoberts::onConnectedChanged(bool connected)
{
    LukeRoberts *lamp = static_cast<LukeRoberts *>(sender());
    Device *device = m_lamps.value(lamp);
    device->setStateValue(modelFConnectedStateTypeId, connected);
}

void DevicePluginLukeRoberts::onDeviceInformationChanged(const QString &firmwareRevision, const QString &hardwareRevision, const QString &softwareRevision)
{
    LukeRoberts *lamp = static_cast<LukeRoberts *>(sender());
    Device *device = m_lamps.value(lamp);

    qDebug(dcLukeRoberts()) << device->name() << "Firmware" << firmwareRevision << "Hardware" << hardwareRevision << "Software" << softwareRevision;
    //device->setStateValue(modelFFirmwareRevisionStateTypeId, firmwareRevision);
    //device->setStateValue(modelFardwareRevisionStateTypeId, hardwareRevision);
    //device->setStateValue(modelFSoftwareRevisionStateTypeId, softwareRevision);
}

void DevicePluginLukeRoberts::onSceneListReceived(QList<LukeRoberts::Scene> scenes)
{
    LukeRoberts *lamp = static_cast<LukeRoberts *>(sender());
    if (m_pendingBrowseResults.contains(lamp)) {
        BrowseResult *result = m_pendingBrowseResults.value(lamp);
        foreach (LukeRoberts::Scene scene, scenes) {
            BrowserItem item;
            item.setId(QString::number(scene.id));
            item.setDisplayName(scene.name);
            item.setBrowsable(false);
            item.setExecutable(true);
            item.setIcon(BrowserItem::BrowserIconApplication);
            result->addItem(item);
        }
    }
}

void DevicePluginLukeRoberts::onStatusCodeReceived(LukeRoberts::StatusCodes statusCode)
{
    qDebug(dcLukeRoberts()) << "Status code received" << statusCode;
}
