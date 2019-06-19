/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *                                                                         *
 *  Copyright (C) 2015 Simon Stuerz <simon.stuerz@guh.guru>                *
 *  Copyright (C) 2014 Michael Zanetti <michael_zanetti@gmx.net>           *
 *                                                                         *
 *  This file is part of guh.                                              *
 *                                                                         *
 *  Guh is free software: you can redistribute it and/or modify            *
 *  it under the terms of the GNU General Public License as published by   *
 *  the Free Software Foundation, version 2 of the License.                *
 *                                                                         *
 *  Guh is distributed in the hope that it will be useful,                 *
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of         *
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the          *
 *  GNU General Public License for more details.                           *
 *                                                                         *
 *  You should have received a copy of the GNU General Public License      *
 *  along with guh. If not, see <http://www.gnu.org/licenses/>.            *
 *                                                                         *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

#include "devicepluginboblight.h"

#include "plugin/device.h"
#include "devicemanager.h"

#include "bobclient.h"
#include "plugininfo.h"
#include "plugintimer.h"

#include <QDebug>
#include <QStringList>
#include <QtMath>

DevicePluginBoblight::DevicePluginBoblight()
{
}

void DevicePluginBoblight::init()
{
    m_pluginTimer = hardwareManager()->pluginTimerManager()->registerTimer(15);
    connect(m_pluginTimer, &PluginTimer::timeout, this, &DevicePluginBoblight::guhTimer);
}

void DevicePluginBoblight::deviceRemoved(Device *device)
{
    if (device->deviceClassId() == boblightServerDeviceClassId) {
        BobClient *client = m_bobClients.take(device->id());
        client->deleteLater();
    }
}

void DevicePluginBoblight::startMonitoringAutoDevices()
{
    m_canCreateAutoDevices = true;
    qCDebug(dcBoblight()) << "Populating auto devices" << myDevices().count();
    QHash<DeviceId, int> parentDevices;
    QHash<DeviceId, Device*> deviceIds;
    foreach (Device *device, myDevices()) {
        deviceIds.insert(device->id(), device);
        if (device->deviceClassId() == boblightServerDeviceClassId) {
            qWarning() << "Device" << device->id() << "is bridge";
            if (!parentDevices.contains(device->id())) {
                parentDevices[device->id()] = 0;
            }
        } else if (device->deviceClassId() == boblightDeviceClassId) {
            qWarning() << "Device" << device->id() << "is child to bridge" << device->parentId();
            parentDevices[device->parentId()] += 1;
        }
    }
    qWarning() << "have bridge devices:" << parentDevices.count();
    QList<DeviceDescriptor> descriptors;
    foreach (const DeviceId &id, parentDevices.keys()) {
        if (parentDevices.value(id) < deviceIds.value(id)->paramValue(boblightServerDeviceChannelsParamTypeId).toInt()) {
            for (int i = parentDevices.value(id); i < deviceIds.value(id)->paramValue(boblightServerDeviceChannelsParamTypeId).toInt(); i++) {
                DeviceDescriptor descriptor(boblightDeviceClassId, deviceIds.value(id)->name() + " " + QString::number(i + 1), QString(), id);
                descriptor.setParams(ParamList() << Param(boblightDeviceChannelParamTypeId, i));
                qCDebug(dcBoblight()) << "Adding new boblight channel" << i + 1;
                descriptors.append(descriptor);
            }
        }
    }
    if (!descriptors.isEmpty()) {
        emit autoDevicesAppeared(boblightDeviceClassId, descriptors);
    }
}

void DevicePluginBoblight::guhTimer()
{
    foreach (BobClient *client, m_bobClients) {
        if (!client->connected()) {
            client->connectToBoblight();
        }
    }
}

void DevicePluginBoblight::onPowerChanged(int channel, bool power)
{
    qCDebug(dcBoblight()) << "power changed" << channel << power;
    BobClient *sndr = dynamic_cast<BobClient*>(sender());
    foreach (Device* device, myDevices()) {
        if (m_bobClients.value(device->parentId()) == sndr && device->paramValue(boblightDeviceChannelParamTypeId).toInt() == channel) {
            qCWarning(dcBoblight()) << "setting state power" << power;
            device->setStateValue(boblightPowerStateTypeId, power);
        }
    }
}

void DevicePluginBoblight::onBrightnessChanged(int channel, int brightness)
{
    BobClient *sndr = dynamic_cast<BobClient*>(sender());
    foreach (Device* device, myDevices()) {
        if (m_bobClients.value(device->parentId()) == sndr && device->paramValue(boblightDeviceChannelParamTypeId).toInt() == channel) {
            device->setStateValue(boblightBrightnessStateTypeId, brightness);
        }
    }
}

void DevicePluginBoblight::onColorChanged(int channel, const QColor &color)
{
    BobClient *sndr = dynamic_cast<BobClient*>(sender());
    foreach (Device* device, myDevices()) {
        if (m_bobClients.value(device->parentId()) == sndr && device->paramValue(boblightDeviceChannelParamTypeId).toInt() == channel) {
            device->setStateValue(boblightColorStateTypeId, color);
        }
    }
}

void DevicePluginBoblight::onPriorityChanged(int priority)
{
    BobClient *sndr = dynamic_cast<BobClient*>(sender());
    foreach (Device* device, myDevices()) {
        if (m_bobClients.value(device->id()) == sndr) {
            device->setStateValue(boblightServerPriorityStateTypeId, priority);
        }
    }
}

QColor DevicePluginBoblight::tempToRgb(int temp)
{
    //  153   cold: 0.839216, 1, 0.827451
    //  500   warm: 0.870588, 1, 0.266667

    //   =>  0 : 214,255,212
    //       100 : 222,255,67

    //  r => 0 : 214 = 100 : 222
    //    => temp : (x-214) = 100 : (255 - 214)
    //    => x = temp * (255-214) / 100
    //       r = x + 214

    int red = temp * 41 / 100 + 214;

    int green = 255;

    //  b => 0 : 212 = 100 : 67
    //    => temp : (212 - x) = 100 : (212 - 145)
    //    => x = temp * 145 / 100
    //       g = 212 - x

    int blue = 212 - temp * 145 / 100;

    qWarning() << "temp:" << temp << "rgb" << red << green << blue;
    return QColor(red, green, blue);
}

DeviceManager::DeviceSetupStatus DevicePluginBoblight::setupDevice(Device *device)
{
    if (device->deviceClassId() == boblightServerDeviceClassId) {

        BobClient *bobClient = new BobClient(device->paramValue(boblightServerDeviceHostAddressParamTypeId).toString(), device->paramValue(boblightServerDevicePortParamTypeId).toInt(), this);
        bool connected = bobClient->connectToBoblight();
        if (!connected) {
            qCWarning(dcBoblight()) << "Error connecting to boblight...";
        } else {
            qCDebug(dcBoblight()) << "Connected to boblight";
        }
        bobClient->setPriority(device->stateValue(boblightServerPriorityStateTypeId).toInt());
        device->setStateValue(boblightServerConnectedStateTypeId, connected);
        m_bobClients.insert(device->id(), bobClient);
        connect(bobClient, &BobClient::connectionChanged, this, &DevicePluginBoblight::onConnectionChanged);
        connect(bobClient, &BobClient::powerChanged, this, &DevicePluginBoblight::onPowerChanged);
        connect(bobClient, &BobClient::brightnessChanged, this, &DevicePluginBoblight::onBrightnessChanged);
        connect(bobClient, &BobClient::colorChanged, this, &DevicePluginBoblight::onColorChanged);
        connect(bobClient, &BobClient::priorityChanged, this, &DevicePluginBoblight::onPriorityChanged);
    } else if (device->deviceClassId() == boblightDeviceClassId) {
        BobClient *bobClient = m_bobClients.value(device->parentId());
        device->setStateValue(boblightConnectedStateTypeId, bobClient->connected());
        m_bobClients.insert(device->id(), bobClient);
    }

    return DeviceManager::DeviceSetupStatusSuccess;
}

void DevicePluginBoblight::postSetupDevice(Device *device)
{
    if (device->deviceClassId() == boblightServerDeviceClassId && m_canCreateAutoDevices) {
        startMonitoringAutoDevices();
    }
    if (device->deviceClassId() == boblightDeviceClassId) {
        BobClient *bobClient = m_bobClients.value(device->parentId());
        if (bobClient && bobClient->connected()) {
            device->setStateValue(boblightConnectedStateTypeId, bobClient->connected());

            QColor color = device->stateValue(boblightColorStateTypeId).value<QColor>();
            int brightness = device->stateValue(boblightBrightnessStateTypeId).toInt();
            bool power = device->stateValue(boblightPowerStateTypeId).toBool();

            bobClient->setColor(device->paramValue(boblightDeviceChannelParamTypeId).toInt(), color);
            bobClient->setBrightness(device->paramValue(boblightDeviceChannelParamTypeId).toInt(), brightness);
            bobClient->setPower(device->paramValue(boblightDeviceChannelParamTypeId).toInt(), power);

        }
    }
}

DeviceManager::DeviceError DevicePluginBoblight::executeAction(Device *device, const Action &action)
{
    if (!device->setupComplete()) {
        return DeviceManager::DeviceErrorHardwareNotAvailable;
    }
    qCDebug(dcBoblight()) << "Execute action for boblight" << action.params();
    if (device->deviceClassId() == boblightServerDeviceClassId) {
        BobClient *bobClient = m_bobClients.value(device->id());
        if (!bobClient || !bobClient->connected()) {
            qCWarning(dcBoblight()) << "Boblight on" << device->paramValue(boblightServerDeviceHostAddressParamTypeId).toString() << "not connected";
            return DeviceManager::DeviceErrorHardwareNotAvailable;
        }

        if (action.actionTypeId() == boblightServerPriorityActionTypeId) {
            bobClient->setPriority(action.param(boblightServerPriorityActionPriorityParamTypeId).value().toInt());
            return DeviceManager::DeviceErrorNoError;
        }
        qCWarning(dcBoblight()) << "Unhandled action" << action.actionTypeId() << "for BoblightServer device" << device;
        return DeviceManager::DeviceErrorActionTypeNotFound;
    }

    if (device->deviceClassId() == boblightDeviceClassId) {
        BobClient *bobClient = m_bobClients.value(device->parentId());
        if (!bobClient || !bobClient->connected()) {
            qCWarning(dcBoblight()) << "Boblight not connected";
            return DeviceManager::DeviceErrorHardwareNotAvailable;
        }

        if (action.actionTypeId() == boblightPowerActionTypeId) {
            bobClient->setPower(device->paramValue(boblightDeviceChannelParamTypeId).toInt(), action.param(boblightPowerActionPowerParamTypeId).value().toBool());
            return DeviceManager::DeviceErrorNoError;
        }
        if (action.actionTypeId() == boblightColorActionTypeId) {
            bobClient->setColor(device->paramValue(boblightDeviceChannelParamTypeId).toInt(), action.param(boblightColorActionColorParamTypeId).value().value<QColor>());
            return DeviceManager::DeviceErrorNoError;
        }
        if (action.actionTypeId() == boblightBrightnessActionTypeId) {
            bobClient->setBrightness(device->paramValue(boblightDeviceChannelParamTypeId).toInt(), action.param(boblightBrightnessActionBrightnessParamTypeId).value().toInt());
            return DeviceManager::DeviceErrorNoError;
        }
        if (action.actionTypeId() == boblightColorTemperatureActionTypeId) {
            bobClient->setColor(device->paramValue(boblightDeviceChannelParamTypeId).toInt(), tempToRgb(action.param(boblightColorTemperatureActionColorTemperatureParamTypeId).value().toInt()));
            return DeviceManager::DeviceErrorNoError;
        }
        return DeviceManager::DeviceErrorActionTypeNotFound;
    }
    return DeviceManager::DeviceErrorDeviceClassNotFound;
}

void DevicePluginBoblight::onConnectionChanged()
{
    BobClient *bobClient = static_cast<BobClient *>(sender());
    qCDebug(dcBoblight()) << "Connection changed. BobClient:" << bobClient << bobClient->connected() << m_bobClients.keys(bobClient);
    foreach (const DeviceId &deviceId, m_bobClients.keys(bobClient)) {
        foreach (Device *device, myDevices()) {
            if (device->id() == deviceId || device->parentId() == deviceId) {
                device->setStateValue(boblightConnectedStateTypeId, bobClient->connected());
                break;
            }
        }
    }
}

