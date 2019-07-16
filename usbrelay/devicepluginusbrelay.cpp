/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *                                                                         *
 *  Copyright (C) 2018 Simon Stürz <simon.stuerz@nymea.io>                 *
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
    qCDebug(dcUsbRelay()) << "Post setup device" << device->name() << device->params();
    if (device->deviceClassId() == usbrelayDeviceClassId) {
        // Initialize the states
        device->setStateValue(usbrelayConnectedStateTypeId, m_relayDevices.key(device)->connected());
        device->setStateValue(usbrelayPowerOneStateTypeId, m_relayDevices.key(device)->relayOnePower());
        device->setStateValue(usbrelayPowerTwoStateTypeId, m_relayDevices.key(device)->relayTwoPower());
    }
}

void DevicePluginUsbRelay::deviceRemoved(Device *device)
{
    qCDebug(dcUsbRelay()) << "Remove device" << device->name() << device->params();

    if (device->deviceClassId() == usbrelayDeviceClassId) {
        UsbRelay *relay = m_relayDevices.key(device);
        m_relayDevices.remove(relay);
        relay->deleteLater();
    }
}

Device::DeviceSetupStatus DevicePluginUsbRelay::setupDevice(Device *device)
{
    qCDebug(dcUsbRelay()) << "Setup device" << device->name() << device->params();

    if (device->deviceClassId() == usbrelayDeviceClassId) {
        UsbRelay *relay = new UsbRelay(device->paramValue(usbrelayDevicePathParamTypeId).toString(), this);
        m_relayDevices.insert(relay, device);

        connect(relay, &UsbRelay::connectedChanged, this, [device](bool connected) {
            qCDebug(dcUsbRelay()) << "Device" << device->name() << (connected ? "connected" : "disconnected");
            device->setStateValue(usbrelayConnectedStateTypeId, connected);
        });

        connect(relay, &UsbRelay::relayOneChanged, this, [device](bool power) {
            device->setStateValue(usbrelayPowerOneStateTypeId, power);
        });

        connect(relay, &UsbRelay::relayTwoChanged, this, [device](bool power) {
            device->setStateValue(usbrelayPowerTwoStateTypeId, power);
        });

        device->setStateValue(usbrelayConnectedStateTypeId, m_relayDevices.key(device)->connected());
        device->setStateValue(usbrelayPowerOneStateTypeId, m_relayDevices.key(device)->relayOnePower());
        device->setStateValue(usbrelayPowerTwoStateTypeId, m_relayDevices.key(device)->relayTwoPower());
        return Device::DeviceSetupStatusSuccess;
    }

    return Device::DeviceSetupStatusSuccess;
}

Device::DeviceError DevicePluginUsbRelay::executeAction(Device *device, const Action &action)
{
    qCDebug(dcUsbRelay()) << "Executing action for device" << device->name() << action.actionTypeId().toString() << action.params();

    if (device->deviceClassId() == usbrelayDeviceClassId) {
        UsbRelay *relay = m_relayDevices.key(device);
        if (!relay) {
            qCWarning(dcUsbRelay()) << "Could not find relay device for" << device;
            return Device::DeviceErrorHardwareFailure;
        }

        if (!relay->connected()) {
            qCWarning(dcUsbRelay()) << "Relay is not connected";
            return Device::DeviceErrorHardwareNotAvailable;
        }

        if (action.actionTypeId() == usbrelayPowerOneActionTypeId) {
            bool power = action.param(usbrelayPowerOneActionPowerOneParamTypeId).value().toBool();
            if (!relay->setRelayOnePower(power)) {
                return Device::DeviceErrorHardwareFailure;
            }
        }

        if (action.actionTypeId() == usbrelayImpulseOneActionTypeId) {
            if (relay->relayOnePower()) {
                if (!relay->setRelayOnePower(false)) {
                    return Device::DeviceErrorHardwareFailure;
                }

                QTimer::singleShot(250, [relay](){
                    relay->setRelayOnePower(true);

                    QTimer::singleShot(250, [relay](){
                        relay->setRelayOnePower(false);
                    });
                });
            } else {
                if (!relay->setRelayOnePower(true)) {
                    return Device::DeviceErrorHardwareFailure;
                }

                QTimer::singleShot(250, [relay](){
                    relay->setRelayOnePower(false);
                });
            }
        }

        if (action.actionTypeId() == usbrelayPowerTwoActionTypeId) {
            bool power = action.param(usbrelayPowerTwoActionPowerTwoParamTypeId).value().toBool();
            if (!relay->setRelayTwoPower(power)) {
                return Device::DeviceErrorHardwareFailure;
            }
        }

        if (action.actionTypeId() == usbrelayImpulseTwoActionTypeId) {
            if (relay->relayTwoPower()) {
                if (!relay->setRelayTwoPower(false)) {
                    return Device::DeviceErrorHardwareFailure;
                }

                QTimer::singleShot(250, [relay](){
                    relay->setRelayTwoPower(true);

                    QTimer::singleShot(250, [relay](){
                        relay->setRelayTwoPower(false);
                    });
                });
            } else {
                if (!relay->setRelayTwoPower(true)) {
                    return Device::DeviceErrorHardwareFailure;
                }

                QTimer::singleShot(250, [relay](){
                    relay->setRelayTwoPower(false);
                });
            }
        }
    }

    return Device::DeviceErrorNoError;
}

Device::DeviceError DevicePluginUsbRelay::discoverDevices(const DeviceClassId &deviceClassId, const ParamList &params)
{
    Q_UNUSED(params)

    QList<DeviceDescriptor> deviceDescriptors;

    // Init
    if (hid_init() < 0) {
        qCWarning(dcUsbRelay()) << "Could not initialize HID.";
        return Device::DeviceErrorHardwareFailure;
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

        qCDebug(dcUsbRelay()) << path << manufacturer << product << serialnumber << QString("%1:%2").arg(QString::number(vendorId, 16)).arg(QString::number(productId, 16)) << releaseNumber;

        if (product == "USBRelay2") {
            //Open it to get more details
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
            bool relayStatusOne = static_cast<bool>(relayStatus &1 << 0);
            bool relayStatusTwo = static_cast<bool>(relayStatus &1 << 1);

            qCDebug(dcUsbRelay()) << "Relay 1:" << relayStatusOne << "| Relay 2:" << relayStatusTwo;
            hid_close(hidDevice);

            DeviceDescriptor descriptor(usbrelayDeviceClassId, product, path);
            ParamList params;
            params.append(Param(usbrelayDevicePathParamTypeId, path));
            descriptor.setParams(params);
            Device *existingDevice = findDeviceForPath(path);
            if (existingDevice) {
                descriptor.setDeviceId(existingDevice->id());
            }
            deviceDescriptors.append(descriptor);
        }
    }

    hid_free_enumeration(devices);
    hid_exit();

    emit devicesDiscovered(deviceClassId, deviceDescriptors);
    return Device::DeviceErrorAsync;
}

Device *DevicePluginUsbRelay::findDeviceForPath(const QString &path)
{
    foreach (Device *d, m_relayDevices.values()) {
        if (d->paramValue(usbrelayDevicePathParamTypeId).toString() == path) {
            return d;
        }
    }

    return nullptr;
}
