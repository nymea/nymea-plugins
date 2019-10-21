/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *                                                                         *
 *  Copyright (C) 2016-2018 Simon Stürz <simon.stuerz@guh.io>              *
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
        ----------------------------------------------------------------
        name           : "Generic Access"
        type           : "<primary>"
        uuid           : "{00001800-0000-1000-8000-00805f9b34fb}"
        uuid (hex)     : "0x1800"
        ----------------------------------------------------------------
        name           : "Generic Attribute"
        type           : "<primary>"
        uuid           : "{00001801-0000-1000-8000-00805f9b34fb}"
        uuid (hex)     : "0x1801"
        ----------------------------------------------------------------
        name           : "Device Information"
        type           : "<primary>"
        uuid           : "{0000180a-0000-1000-8000-00805f9b34fb}"
        uuid (hex)     : "0x180a"
        ----------------------------------------------------------------
        name           : "Unknown Service"
        type           : "<primary>"
        uuid           : "{f815e600-456c-6761-746f-4d756e696368}"
        uuid (hex)     : "f815e600-456c-6761-746f-4d756e696368"
        ----------------------------------------------------------------
        name           : "Unknown Service"
        type           : "<primary>"
        uuid           : "{f815e500-456c-6761-746f-4d756e696368}"
        uuid (hex)     : "f815e500-456c-6761-746f-4d756e696368"
        ----------------------------------------------------------------
        name           : "Unknown Service"
        type           : "<primary>"
        uuid           : "{f815e810-456c-6761-746f-4d756e696368}"
        uuid (hex)     : "f815e810-456c-6761-746f-4d756e696368"
        ----------------------------------------------------------------
        name           : "Unknown Service"
        type           : "<primary>"
        uuid           : "{f815e900-456c-6761-746f-4d756e696368}"
        uuid (hex)     : "f815e900-456c-6761-746f-4d756e696368"
        ----------------------------------------------------------------

    Service: "Generic Access" {00001800-0000-1000-8000-00805f9b34fb} details

        ----------------------------------------------------------------
        characteristics:
           name            : "GAP Device Name"
           uuid            : "{00002a00-0000-1000-8000-00805f9b34fb}"
           uuid (hex)      : "0x2a00"
           handle          : "0x3"
           permission      : ("Read")
           value           : "Avea_44A9"
           value (hex)     : "417665615f34344139"
           ---------------------------------------------------------
           descriptor count: 0
           ---------------------------------------------------------
           name            : "GAP Appearance"
           uuid            : "{00002a01-0000-1000-8000-00805f9b34fb}"
           uuid (hex)      : "0x2a01"
           handle          : "0x5"
           permission      : ("Read")
           value           : "
           value (hex)     : "0000"
           ---------------------------------------------------------
           descriptor count: 0
           ---------------------------------------------------------
           name            : "GAP Peripheral Privacy Flag"
           uuid            : "{00002a02-0000-1000-8000-00805f9b34fb}"
           uuid (hex)      : "0x2a02"
           handle          : "0x7"
           permission      : ("Read", "Write")
           value           : "
           value (hex)     : "00"
           ---------------------------------------------------------
           descriptor count: 0
           ---------------------------------------------------------
           name            : "GAP Reconnection Address"
           uuid            : "{00002a03-0000-1000-8000-00805f9b34fb}"
           uuid (hex)      : "0x2a03"
           handle          : "0x9"
           permission      : ("Read", "Write")
           value           : "
           value (hex)     : "000000000000"
           ---------------------------------------------------------
           descriptor count: 0
           ---------------------------------------------------------
           name            : "GAP Peripheral Preferred Connection Parameters"
           uuid            : "{00002a04-0000-1000-8000-00805f9b34fb}"
           uuid (hex)      : "0x2a04"
           handle          : "0xb"
           permission      : ("Read")
           value           : "P
           value (hex)     : "5000a0000000e803"
           ---------------------------------------------------------
           descriptor count: 0
           ---------------------------------------------------------


    Service: "Generic Attribute" {00001801-0000-1000-8000-00805f9b34fb} details

        ----------------------------------------------------------------
        characteristics:
           name            : "GATT Service Changed"
           uuid            : "{00002a05-0000-1000-8000-00805f9b34fb}"
           uuid (hex)      : "0x2a05"
           handle          : "0xe"
           permission      : ("Indicate")
           value           : ""
           value (hex)     : ""
           ---------------------------------------------------------
           descriptor count: 1
           ---------------------------------------------------------
               name        : "Client Characteristic Configuration"
               uuid        : "{00002902-0000-1000-8000-00805f9b34fb}"
               handle      : 15
               value       : "
               value (hex) : "0000"
               -----------------------------------------------------


    Service: "Device Information" {0000180a-0000-1000-8000-00805f9b34fb} details

        ----------------------------------------------------------------
        characteristics:
           name            : "System ID"
           uuid            : "{00002a23-0000-1000-8000-00805f9b34fb}"
           uuid (hex)      : "0x2a23"
           handle          : "0x12"
           permission      : ("Read")
           value           : "D�j����"
           value (hex)     : "44a96afeff18eb84"
           ---------------------------------------------------------
           descriptor count: 0
           ---------------------------------------------------------
           name            : "Model Number String"
           uuid            : "{00002a24-0000-1000-8000-00805f9b34fb}"
           uuid (hex)      : "0x2a24"
           handle          : "0x14"
           permission      : ("Read")
           value           : "1
           value (hex)     : "3100"
           ---------------------------------------------------------
           descriptor count: 0
           ---------------------------------------------------------
           name            : "Serial Number String"
           uuid            : "{00002a25-0000-1000-8000-00805f9b34fb}"
           uuid (hex)      : "0x2a25"
           handle          : "0x16"
           permission      : ("Read")
           value           : "44A96A18EB84"
           value (hex)     : "343441393641313845423834"
           ---------------------------------------------------------
           descriptor count: 0
           ---------------------------------------------------------
           name            : "Firmware Revision String"
           uuid            : "{00002a26-0000-1000-8000-00805f9b34fb}"
           uuid (hex)      : "0x2a26"
           handle          : "0x18"
           permission      : ("Read")
           value           : "1.0.0.296Af
           value (hex)     : "312e302e302e323936416600"
           ---------------------------------------------------------
           descriptor count: 0
           ---------------------------------------------------------
           name            : "Hardware Revision String"
           uuid            : "{00002a27-0000-1000-8000-00805f9b34fb}"
           uuid (hex)      : "0x2a27"
           handle          : "0x1a"
           permission      : ("Read")
           value           : "Elgato Avea
           value (hex)     : "456c6761746f204176656100"
           ---------------------------------------------------------
           descriptor count: 0
           ---------------------------------------------------------
           name            : "Software Revision String"
           uuid            : "{00002a28-0000-1000-8000-00805f9b34fb}"
           uuid (hex)      : "0x2a28"
           handle          : "0x1c"
           permission      : ("Read")
           value           : "1.0
           value (hex)     : "312e3000"
           ---------------------------------------------------------
           descriptor count: 0
           ---------------------------------------------------------
           name            : "Manufacturer Name String"
           uuid            : "{00002a29-0000-1000-8000-00805f9b34fb}"
           uuid (hex)      : "0x2a29"
           handle          : "0x1e"
           permission      : ("Read")
           value           : "Elgato Systems GmbH
           value (hex)     : "456c6761746f2053797374656d7320476d624800"
           ---------------------------------------------------------
           descriptor count: 0
           ---------------------------------------------------------
           name            : "PnP ID"
           uuid            : "{00002a50-0000-1000-8000-00805f9b34fb}"
           uuid (hex)      : "0x2a50"
           handle          : "0x20"
           permission      : ("Read")
           value           : "�
           value (hex)     : "02d90f00000001"
           ---------------------------------------------------------
           descriptor count: 0
           ---------------------------------------------------------

    Service: "Unknown Service" {f815e600-456c-6761-746f-4d756e696368} details

        ----------------------------------------------------------------
        characteristics:
           name            : "Alert"
           uuid            : "{f815e601-456c-6761-746f-4d756e696368}"
           uuid (hex)      : "f815e601-456c-6761-746f-4d756e696368"
           handle          : "0x23"
           permission      : ("Notify")
           value           : ""
           value (hex)     : ""
           ---------------------------------------------------------
           descriptor count: 2
           ---------------------------------------------------------
               name        : "Client Characteristic Configuration"
               uuid        : "{00002902-0000-1000-8000-00805f9b34fb}"
               handle      : 36
               value       : "
               value (hex) : "0000"
               -----------------------------------------------------
               name        : "Characteristic User Description"
               uuid        : "{00002901-0000-1000-8000-00805f9b34fb}"
               handle      : 37
               value       : "Alert"
               value (hex) : "416c657274"
               -----------------------------------------------------


    Service: "Unknown Service" {f815e500-456c-6761-746f-4d756e696368} details

        ----------------------------------------------------------------
        characteristics:
           name            : "Seq Upload"
           uuid            : "{f815e501-456c-6761-746f-4d756e696368}"
           uuid (hex)      : "f815e501-456c-6761-746f-4d756e696368"
           handle          : "0x28"
           permission      : ("Write", "Notify", "WriteNoResp")
           value           : ""
           value (hex)     : ""
           ---------------------------------------------------------
           descriptor count: 2
           ---------------------------------------------------------
               name        : "Client Characteristic Configuration"
               uuid        : "{00002902-0000-1000-8000-00805f9b34fb}"
               handle      : 41
               value       : "
               value (hex) : "0000"
               -----------------------------------------------------
               name        : "Characteristic User Description"
               uuid        : "{00002901-0000-1000-8000-00805f9b34fb}"
               handle      : 42
               value       : "Seq Upload"
               value (hex) : "5365712055706c6f6164"
               -----------------------------------------------------


    Service: "Unknown Service" {f815e810-456c-6761-746f-4d756e696368} details
    Tis service will be used to set the color (handle 0x2d).

        ----------------------------------------------------------------
        characteristics:
           name            : "Debug"
           uuid            : "{f815e811-456c-6761-746f-4d756e696368}"
           uuid (hex)      : "f815e811-456c-6761-746f-4d756e696368"
           handle          : "0x2d"
           permission      : ("Write", "Notify")
           value           : ""
           value (hex)     : ""
           ---------------------------------------------------------
           descriptor count: 2
           ---------------------------------------------------------
               name        : "Client Characteristic Configuration"
               uuid        : "{00002902-0000-1000-8000-00805f9b34fb}"
               handle      : 46
               value       : "
               value (hex) : "0000"
               -----------------------------------------------------
               name        : "Characteristic User Description"
               uuid        : "{00002901-0000-1000-8000-00805f9b34fb}"
               handle      : 47
               value       : "Debug"
               value (hex) : "4465627567"
               -----------------------------------------------------
           name            : "User Name"
           uuid            : "{f815e812-456c-6761-746f-4d756e696368}"
           uuid (hex)      : "f815e812-456c-6761-746f-4d756e696368"
           handle          : "0x31"
           permission      : ("Read", "Write")
           value           : ""
           value (hex)     : ""
           ---------------------------------------------------------
           descriptor count: 1
           ---------------------------------------------------------
               name        : "Characteristic User Description"
               uuid        : "{00002901-0000-1000-8000-00805f9b34fb}"
               handle      : 50
               value       : "User Name"
               value (hex) : "55736572204e616d65"
               -----------------------------------------------------

    Service: "Unknown Service" {f815e900-456c-6761-746f-4d756e696368} details

        ----------------------------------------------------------------
        characteristics:
           name            : "Img Identify"
           uuid            : "{f815e901-456c-6761-746f-4d756e696368}"
           uuid (hex)      : "f815e901-456c-6761-746f-4d756e696368"
           handle          : "0x35"
           permission      : ("Write", "Notify", "WriteNoResp")
           value           : ""
           value (hex)     : ""
           ---------------------------------------------------------
           descriptor count: 2
           ---------------------------------------------------------
               name        : "Client Characteristic Configuration"
               uuid        : "{00002902-0000-1000-8000-00805f9b34fb}"
               handle      : 54
               value       : "
               value (hex) : "0000"
               -----------------------------------------------------
               name        : "Characteristic User Description"
               uuid        : "{00002901-0000-1000-8000-00805f9b34fb}"
               handle      : 55
               value       : "Img Identify"
               value (hex) : "496d67204964656e74696679"
               -----------------------------------------------------
           name            : "Img Block"
           uuid            : "{f815e902-456c-6761-746f-4d756e696368}"
           uuid (hex)      : "f815e902-456c-6761-746f-4d756e696368"
           handle          : "0x39"
           permission      : ("Write", "Notify", "WriteNoResp")
           value           : ""
           value (hex)     : ""
           ---------------------------------------------------------
           descriptor count: 2
           ---------------------------------------------------------
               name        : "Client Characteristic Configuration"
               uuid        : "{00002902-0000-1000-8000-00805f9b34fb}"
               handle      : 58
               value       : "
               value (hex) : "0000"
               -----------------------------------------------------
               name        : "Characteristic User Description"
               uuid        : "{00002901-0000-1000-8000-00805f9b34fb}"
               handle      : 59
               value       : "Img Block"
               value (hex) : "496d6720426c6f636b"
               -----------------------------------------------------
*/



#include "devicepluginelgato.h"

#include "devices/device.h"
#include "plugininfo.h"
#include "hardware/bluetoothlowenergy/bluetoothlowenergymanager.h"

DevicePluginElgato::DevicePluginElgato()
{

}

DevicePluginElgato::~DevicePluginElgato()
{
    hardwareManager()->pluginTimerManager()->unregisterTimer(m_pluginTimer);
}

void DevicePluginElgato::init()
{
    m_pluginTimer = hardwareManager()->pluginTimerManager()->registerTimer(10);
    connect(m_pluginTimer, &PluginTimer::timeout, this, &DevicePluginElgato::onPluginTimer);
}

void DevicePluginElgato::discoverDevices(DeviceDiscoveryInfo *info)
{
    DeviceClassId deviceClassId = info->deviceClassId();

    if (deviceClassId != aveaDeviceClassId)
        return info->finish(Device::DeviceErrorDeviceClassNotFound);

    if (!hardwareManager()->bluetoothLowEnergyManager()->available())
        return info->finish(Device::DeviceErrorHardwareNotAvailable);

    if (!hardwareManager()->bluetoothLowEnergyManager()->enabled())
        return info->finish(Device::DeviceErrorHardwareNotAvailable);

    BluetoothDiscoveryReply *reply = hardwareManager()->bluetoothLowEnergyManager()->discoverDevices();
    connect(reply, &BluetoothDiscoveryReply::finished, info, [this, info, reply]{
        reply->deleteLater();

        if (reply->error() != BluetoothDiscoveryReply::BluetoothDiscoveryReplyErrorNoError) {
            qCWarning(dcElgato()) << "Bluetooth discovery error:" << reply->error();
            info->finish(Device::DeviceErrorHardwareFailure);
            return;
        }

        foreach (const QBluetoothDeviceInfo &deviceInfo, reply->discoveredDevices()) {
            if (deviceInfo.name().contains("Avea")) {
                if (!verifyExistingDevices(deviceInfo)) {
                    DeviceDescriptor descriptor(aveaDeviceClassId, "Avea", deviceInfo.address().toString());
                    ParamList params;
                    params.append(Param(aveaDeviceNameParamTypeId, deviceInfo.name()));
                    params.append(Param(aveaDeviceMacAddressParamTypeId, deviceInfo.address().toString()));
                    descriptor.setParams(params);
                    foreach (Device *existingDevice, myDevices()) {
                        if (existingDevice->paramValue(aveaDeviceMacAddressParamTypeId).toString() == deviceInfo.address().toString()) {
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

void DevicePluginElgato::setupDevice(DeviceSetupInfo *info)
{
    Device *device = info->device();

    qCDebug(dcElgato()) << "Setup device" << device->name() << device->params();

    if (device->deviceClassId() == aveaDeviceClassId) {
        QBluetoothAddress address = QBluetoothAddress(device->paramValue(aveaDeviceMacAddressParamTypeId).toString());
        QString name = device->paramValue(aveaDeviceNameParamTypeId).toString();
        QBluetoothDeviceInfo deviceInfo = QBluetoothDeviceInfo(address, name, 0);

        BluetoothLowEnergyDevice *bluetoothDevice = hardwareManager()->bluetoothLowEnergyManager()->registerDevice(deviceInfo, QLowEnergyController::PublicAddress);

        AveaBulb *bulb = new AveaBulb(device, bluetoothDevice, this);
        m_bulbs.insert(device, bulb);

        return info->finish(Device::DeviceErrorNoError);
    }
    return info->finish(Device::DeviceErrorDeviceClassNotFound);
}

void DevicePluginElgato::postSetupDevice(Device *device)
{
    AveaBulb *bulb = m_bulbs.value(device);
    // Init values for restore
    bulb->setBrightness(device->stateValue(aveaBrightnessStateTypeId).toInt());
    bulb->setFade(device->stateValue(aveaFadeStateTypeId).toInt());
    bulb->setWhite(device->stateValue(aveaWhiteStateTypeId).toInt());
    bulb->setRed(device->stateValue(aveaRedStateTypeId).toInt());
    bulb->setGreen(device->stateValue(aveaGreenStateTypeId).toInt());
    bulb->setBlue(device->stateValue(aveaBlueStateTypeId).toInt());

    bulb->bluetoothDevice()->connectDevice();
}

void DevicePluginElgato::executeAction(DeviceActionInfo *info)
{
    Device *device = info->device();
    Action action = info->action();

    if (device->deviceClassId() == aveaDeviceClassId) {
        AveaBulb *bulb = m_bulbs.value(device);

        if (action.actionTypeId() == aveaPowerActionTypeId) {
            bool power = action.param(aveaPowerActionPowerParamTypeId).value().toBool();
            device->setStateValue(aveaPowerStateTypeId, power);
            if (!bulb->setPower(power))
                return info->finish(Device::DeviceErrorHardwareNotAvailable);

            return info->finish(Device::DeviceErrorNoError);
        } else if (action.actionTypeId() == aveaBrightnessActionTypeId) {
            int percentage = action.param(aveaBrightnessActionBrightnessParamTypeId).value().toInt();
            if (!bulb->setBrightness(percentage))
                return info->finish(Device::DeviceErrorHardwareNotAvailable);

            return info->finish(Device::DeviceErrorNoError);
        } else if (action.actionTypeId() == aveaColorActionTypeId) {
            QColor color = action.param(aveaColorActionColorParamTypeId).value().value<QColor>();
            color.setAlpha(0); // Alpha is white
            if (!bulb->setColor(color))
                return info->finish(Device::DeviceErrorHardwareNotAvailable);

            return info->finish(Device::DeviceErrorNoError);
        } else if (action.actionTypeId() == aveaColorTemperatureActionTypeId) {
            int ctValue = action.param(aveaColorTemperatureActionColorTemperatureParamTypeId).value().toInt();
            // normalize from 0 to 347 instead of 153 to 500
            int ct = ctValue - 153;
            // for blue: lower half fades blue from 255 to 0
            // ct : (255-blue) = (347 / 2) : 255
            int blue = qMax(0, 255 - (ct * 255 / (347 / 2)));
            // for red: upper half fades red from 0 255
            // ct - (347/2) : red = (347 / 2) : 255
            int red = qMax(0, (ct - (347/2)) * 255 / ((500-153)/2));

            QColor color;
            color.setRed(red);
            color.setGreen(0);
            color.setBlue(blue);
            color.setAlpha(255); // Alpha is white
            if (!bulb->setColor(color)) {
                return info->finish(Device::DeviceErrorHardwareNotAvailable);
            }
            device->setStateValue(aveaColorTemperatureStateTypeId, ctValue);
            return info->finish(Device::DeviceErrorNoError);
        } else if (action.actionTypeId() == aveaWhiteActionTypeId) {
            int whiteValue = action.param(aveaWhiteActionWhiteParamTypeId).value().toInt();
            if (!bulb->setWhite(whiteValue))
                return info->finish(Device::DeviceErrorHardwareNotAvailable);

            return info->finish(Device::DeviceErrorNoError);
        } else if (action.actionTypeId() == aveaGreenActionTypeId) {
            int greenValue = action.param(aveaGreenActionGreenParamTypeId).value().toInt();
            if (!bulb->setGreen(greenValue))
                return info->finish(Device::DeviceErrorHardwareNotAvailable);

            return info->finish(Device::DeviceErrorNoError);
        } else if (action.actionTypeId() == aveaRedActionTypeId) {
            int redValue = action.param(aveaRedActionRedParamTypeId).value().toInt();
            if (!bulb->setRed(redValue))
                return info->finish(Device::DeviceErrorHardwareNotAvailable);

            return info->finish(Device::DeviceErrorNoError);
        } else if (action.actionTypeId() == aveaBlueActionTypeId) {
            int blueValue = action.param(aveaBlueActionBlueParamTypeId).value().toInt();
            if (!bulb->setBlue(blueValue))
                return info->finish(Device::DeviceErrorHardwareNotAvailable);

            return info->finish(Device::DeviceErrorNoError);
        } else if (action.actionTypeId() == aveaFadeActionTypeId) {
            int fadeValue = action.param(aveaFadeActionFadeParamTypeId).value().toInt();
            if (!bulb->setFade(fadeValue))
                return info->finish(Device::DeviceErrorHardwareNotAvailable);

            return info->finish(Device::DeviceErrorNoError);
        }

        return info->finish(Device::DeviceErrorActionTypeNotFound);
    }
    return info->finish(Device::DeviceErrorDeviceClassNotFound);
}

void DevicePluginElgato::deviceRemoved(Device *device)
{
    if (!m_bulbs.keys().contains(device))
        return;

    AveaBulb *bulb = m_bulbs.value(device);
    m_bulbs.remove(device);
    hardwareManager()->bluetoothLowEnergyManager()->unregisterDevice(bulb->bluetoothDevice());
    bulb->deleteLater();
}

bool DevicePluginElgato::verifyExistingDevices(const QBluetoothDeviceInfo &deviceInfo)
{
    foreach (Device *device, myDevices()) {
        if (device->paramValue(aveaDeviceMacAddressParamTypeId).toString() == deviceInfo.address().toString())
            return true;
    }

    return false;
}

void DevicePluginElgato::onPluginTimer()
{
    foreach (AveaBulb *bulb, m_bulbs.values()) {
        if (!bulb->bluetoothDevice()->connected()) {
            bulb->bluetoothDevice()->connectDevice();
        }
    }
}

