/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *                                                                         *
 *  Copyright (C) 2019 Simon Stürz <simon.stuerz@nymea.io>                 *
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

#include "plugininfo.h"
#include "devicepluginusbrelay.h"

#include <QTimer>
#include <hidapi/hidapi.h>

DevicePluginUsbRelay::DevicePluginUsbRelay()
{

}

void DevicePluginUsbRelay::init()
{
    // Initialize/create objects
}

void DevicePluginUsbRelay::startMonitoringAutoDevices()
{
    // Start seaching for devices which can be discovered and added automatically
}

void DevicePluginUsbRelay::postSetupDevice(Device *device)
{
    qCDebug(dcUsbRelay()) << "Post setup device" << device;

    if (device->deviceClassId() == usbRelayConnectorDeviceClassId) {

        // Initialize the states
        UsbRelay *relay = m_relayDevices.key(device);
        if (!relay) {
            qCWarning(dcUsbRelay()) << "Could not find relay in post setup.";
            return;
        }

        device->setStateValue(usbRelayConnectorConnectedStateTypeId, relay->connected());

        // Check if we have to create child devices (relays)
        if (myDevices().filterByParentDeviceId(device->id()).isEmpty()) {

            DeviceDescriptors descriptors;
            for (int i = 0; i < relay->relayCount(); i++) {
                int relayNumber = i + 1;
                DeviceDescriptor descriptor(usbRelayDeviceClassId, QString("Relay %1").arg(relayNumber), QString(), device->id());
                ParamList params;
                params.append(Param(usbRelayDeviceRelayNumberParamTypeId, relayNumber));
                descriptor.setParams(params);
                descriptors.append(descriptor);
            }

            emit autoDevicesAppeared(descriptors);
        }
    } else if (device->deviceClassId() == usbRelayDeviceClassId) {

        UsbRelay *relay = getRelayForDevice(device);
        if (!relay) return;

        // Set the current states
        int relayNumber = device->paramValue(usbRelayDeviceRelayNumberParamTypeId).toInt();
        device->setStateValue(usbRelayConnectedStateTypeId, relay->connected());
        device->setStateValue(usbRelayPowerStateTypeId, relay->relayPower(relayNumber));
    }
}

void DevicePluginUsbRelay::deviceRemoved(Device *device)
{
    qCDebug(dcUsbRelay()) << "Remove device" << device;
    if (device->deviceClassId() == usbRelayConnectorDeviceClassId) {
        UsbRelay *relay = m_relayDevices.key(device);
        if (!relay) return;
        m_relayDevices.remove(relay);
        relay->deleteLater();
    }
}

void DevicePluginUsbRelay::discoverDevices(DeviceDiscoveryInfo *info)
{
    // Init
    if (hid_init() < 0) {
        qCWarning(dcUsbRelay()) << "Could not initialize HID.";
        info->finish(Device::DeviceErrorHardwareFailure, QT_TR_NOOP("Cannot discover USB devices. HID initialisation failed on this system."));
        return;
    }

    // Enumerate hid devices
    struct hid_device_info *devices = nullptr;
    struct hid_device_info *currentDevice = nullptr;
    devices = hid_enumerate(0x16c0, 0x05df);
    int relayCount = 0;
    currentDevice = devices;

    for (relayCount = 0; currentDevice != nullptr; relayCount++) {
        currentDevice = currentDevice->next;
    }

    qCDebug(dcUsbRelay()) << "Found" << relayCount << "usb relay devices";
    currentDevice = devices;
    for (int i = 0; i < relayCount; i++) {
        QString path = QString::fromLatin1(currentDevice->path);
        QString manufacturer = QString::fromWCharArray(currentDevice->manufacturer_string);
        QString product = QString::fromWCharArray(currentDevice->product_string);
        QString serialnumber = QString::fromWCharArray(currentDevice->serial_number);
        quint16 releaseNumber = static_cast<quint16>(currentDevice->release_number);
        quint16 productId = static_cast<quint16>(currentDevice->product_id);
        quint16 vendorId = static_cast<quint16>(currentDevice->vendor_id);
        int relayCount = QString(product.at(product.count() -1)).toInt();

        qCDebug(dcUsbRelay()) << path << manufacturer << product << serialnumber << "Relay count" << relayCount << QString("%1:%2").arg(QString::number(vendorId, 16)).arg(QString::number(productId, 16)) << releaseNumber;

        // Open it to get more details
        hid_device *hidDevice = nullptr;
        hidDevice = hid_open_path(currentDevice->path);
        if (!hidDevice) {
            qCWarning(dcUsbRelay()) << "Could not create HID device for" << path;
            continue;
        }

        unsigned char buf[9];
        buf[0] = 0x01;
        int ret = hid_get_feature_report(hidDevice, buf, sizeof(buf));
        if (ret < 0) {
            qCWarning(dcUsbRelay()) << "Could not create HID device hidDevice for" << path;
            continue;
        }

        quint8 relayStatus = static_cast<quint8>(buf[7]);
        for (int i = 0; i < relayCount; i++) {
            int relayNumber = i + 1;
            bool power = static_cast<bool>(relayStatus & 1 << i);
            qCDebug(dcUsbRelay()) << "--> Relay" << relayNumber << ":" << power;
        }

        hid_close(hidDevice);

        DeviceDescriptor descriptor(usbRelayConnectorDeviceClassId, "USB Relay Connector", QString("%1 (%2)").arg(product).arg(path));
        ParamList params;
        params.append(Param(usbRelayConnectorDevicePathParamTypeId, path));
        params.append(Param(usbRelayConnectorDeviceRelayCountParamTypeId, relayCount));
        descriptor.setParams(params);

        // Set the current device id if we already have a device on this path
        foreach (Device *existingDevice, myDevices()) {
            if (existingDevice->paramValue(usbRelayConnectorDevicePathParamTypeId).toString() == path &&
                    existingDevice->paramValue(usbRelayConnectorDeviceRelayCountParamTypeId).toInt() == relayCount) {
                descriptor.setDeviceId(existingDevice->id());
                break;
            }
        }
        info->addDeviceDescriptor(descriptor);
    }

    hid_free_enumeration(devices);
    hid_exit();

    info->finish(Device::DeviceErrorNoError);
}

void DevicePluginUsbRelay::setupDevice(DeviceSetupInfo *info)
{
    qCDebug(dcUsbRelay()) << "Setup device" << info->device();

    // Relay connector
    if (info->device()->deviceClassId() == usbRelayConnectorDeviceClassId) {
        Device *device = info->device();
        QString path = device->paramValue(usbRelayConnectorDevicePathParamTypeId).toString();
        int relayCount = device->paramValue(usbRelayConnectorDeviceRelayCountParamTypeId).toInt();

        UsbRelay *relay = new UsbRelay(path, relayCount, this);
        m_relayDevices.insert(relay, device);

        connect(relay, &UsbRelay::connectedChanged, [this, device, relay](bool connected) {
            qCDebug(dcUsbRelay()) << "Device" << device->name() << (connected ? "connected" : "disconnected");
            device->setStateValue(usbRelayConnectorConnectedStateTypeId, connected);

            // Set the connected state of all child devices
            foreach (Device *childDevice, myDevices().filterByParentDeviceId(device->id())) {
                if (childDevice->deviceClassId() == usbRelayDeviceClassId && childDevice->setupComplete()) {
                    childDevice->setStateValue(usbRelayConnectedStateTypeId, connected);
                    if (connected) {
                        childDevice->setStateValue(usbRelayPowerStateTypeId, relay->relayPower(childDevice->paramValue(usbRelayDeviceRelayNumberParamTypeId).toInt()));
                    }
                }
            }
        });

        connect(relay, &UsbRelay::relayPowerChanged, [this, device](int relayNumber, bool power) {
            Device *relayDevice = getRelayDevice(device, relayNumber);
            if (!relayDevice) {
                // Note: probably not set up yet
                qCWarning(dcUsbRelay()) << "Could not find USB relay child device for" << device << relayNumber;
                return;
            }

            relayDevice->setStateValue(usbRelayPowerStateTypeId, power);
        });

        info->finish(Device::DeviceErrorNoError);
        return;
    }

    // Relay
    if (info->device()->deviceClassId() == usbRelayDeviceClassId) {
        info->finish(Device::DeviceErrorNoError);
        return;
    }

    info->finish(Device::DeviceErrorSetupFailed);
}

void DevicePluginUsbRelay::executeAction(DeviceActionInfo *info)
{
    qCDebug(dcUsbRelay()) << "Executing action for device" << info->device() << info->action().actionTypeId().toString() << info->action().params();

    if (info->device()->deviceClassId() == usbRelayDeviceClassId) {

        Device *device = info->device();
        UsbRelay *relay = getRelayForDevice(device);

        if (!relay) {
            qCWarning(dcUsbRelay()) << "Could execute action because could not find USB relay for" << device;
            info->finish(Device::DeviceErrorHardwareNotAvailable);
            return;
        }

        if (!relay->connected()) {
            qCWarning(dcUsbRelay()) << "Relay is not connected";
            info->finish(Device::DeviceErrorHardwareNotAvailable);
            return;
        }

        int relayNumber = device->paramValue(usbRelayDeviceRelayNumberParamTypeId).toInt();

        if (info->action().actionTypeId() == usbRelayPowerActionTypeId) {
            bool power = info->action().param(usbRelayPowerActionPowerParamTypeId).value().toBool();
            if (!relay->setRelayPower(relayNumber, power)) {
                info->finish(Device::DeviceErrorHardwareFailure);
                return;
            }
            info->finish(Device::DeviceErrorNoError);
            return;
        }

        info->finish(Device::DeviceErrorActionTypeNotFound);
    }

    info->finish(Device::DeviceErrorDeviceClassNotFound);
}

Device *DevicePluginUsbRelay::getRelayDevice(Device *parentDevice, int relayNumber)
{
    foreach (Device *childDevice, myDevices().filterByParentDeviceId(parentDevice->id())) {
        if (childDevice->deviceClassId() == usbRelayDeviceClassId) {
            if (childDevice->paramValue(usbRelayDeviceRelayNumberParamTypeId).toInt() == relayNumber) {
                return childDevice;
            }
        }
    }

    return nullptr;
}

UsbRelay *DevicePluginUsbRelay::getRelayForDevice(Device *relayDevice)
{
    Device *parentDevice = myDevices().findById(relayDevice->parentId());
    if (!parentDevice) {
        qCWarning(dcUsbRelay()) << "Could not find the parent device for" << relayDevice;
        return nullptr;
    }

    UsbRelay *relay = m_relayDevices.key(parentDevice);
    if (!relay) {
        qCWarning(dcUsbRelay()) << "Could not find USB relay for" << relayDevice;
        return nullptr;
    }

    return relay;
}
