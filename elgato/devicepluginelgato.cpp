/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
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

/*!
    \page elgato.html
    \title Elgato
    \brief Plugin for Elgato Avea Blutooth lamp.

    \ingroup plugins
    \ingroup nymea-plugins

    This plugin allows to find and controll the Bluetooth Low Energy bulb from \l{https://www.elgato.com/en/smart/avea}{Elgato Avea}.

    \chapter Device Overviw of \tt{Avea_44A9}
    \section1 Services
    \code
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
    \endcode

    \section2 Service: "Generic Access" {00001800-0000-1000-8000-00805f9b34fb} details
    \code
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
    \endcode

    \section2 Service: "Generic Attribute" {00001801-0000-1000-8000-00805f9b34fb} details
    \code
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
    \endcode

    \section2 Service: "Device Information" {0000180a-0000-1000-8000-00805f9b34fb} details
    \code
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
    \endcode

    \section2 Service: "Unknown Service" {f815e600-456c-6761-746f-4d756e696368} details
    \code
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
    \endcode

    \section2 Service: "Unknown Service" {f815e500-456c-6761-746f-4d756e696368} details
    \code
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
    \endcode

    \section2 Service: "Unknown Service" {f815e810-456c-6761-746f-4d756e696368} details

    Tis service will be used to set the color (handle \tt{0x2d}).

    \code
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
    \endcode

    \section2 Service: "Unknown Service" {f815e900-456c-6761-746f-4d756e696368} details
    \code
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
    \endcode


    \chapter Plugin properties
    Following JSON file contains the definition and the description of all available \l{DeviceClass}{DeviceClasses}
    and \l{Vendor}{Vendors} of this \l{DevicePlugin}.

    For more details how to read this JSON file please check out the documentation for \l{The plugin JSON File}.

    \quotefile plugins/deviceplugins/elgato/devicepluginelgato.json
*/

#include "devicepluginelgato.h"

#include "plugin/device.h"
#include "devicemanager.h"
#include "plugininfo.h"
#include "hardware/bluetoothlowenergy/bluetoothlowenergymanager.h"

DevicePluginElgato::DevicePluginElgato()
{

}

DeviceManager::DeviceError DevicePluginElgato::discoverDevices(const DeviceClassId &deviceClassId, const ParamList &params)
{
    Q_UNUSED(params)

    if (deviceClassId != aveaDeviceClassId)
        return DeviceManager::DeviceErrorDeviceClassNotFound;

    if (!hardwareManager()->bluetoothLowEnergyManager()->available())
        return DeviceManager::DeviceErrorHardwareNotAvailable;

    if (!hardwareManager()->bluetoothLowEnergyManager()->enabled())
        return DeviceManager::DeviceErrorHardwareNotAvailable;

    BluetoothDiscoveryReply *reply = hardwareManager()->bluetoothLowEnergyManager()->discoverDevices();
    connect(reply, &BluetoothDiscoveryReply::finished, this, &DevicePluginElgato::onBluetoothDiscoveryFinished);
    return DeviceManager::DeviceErrorAsync;
}

DeviceManager::DeviceSetupStatus DevicePluginElgato::setupDevice(Device *device)
{
    qCDebug(dcElgato()) << "Setup device" << device->name() << device->params();

    if (device->deviceClassId() == aveaDeviceClassId) {
        QBluetoothAddress address = QBluetoothAddress(device->paramValue(aveaMacAddressParamTypeId).toString());
        QString name = device->paramValue(aveaNameParamTypeId).toString();
        QBluetoothDeviceInfo deviceInfo = QBluetoothDeviceInfo(address, name, 0);

        BluetoothLowEnergyDevice *bluetoothDevice = hardwareManager()->bluetoothLowEnergyManager()->registerDevice(deviceInfo, QLowEnergyController::PublicAddress);

        AveaBulb *bulb = new AveaBulb(device, bluetoothDevice, this);

        m_bulbs.insert(device, bulb);
        bulb->bluetoothDevice()->connectDevice();

        return DeviceManager::DeviceSetupStatusSuccess;
    }
    return DeviceManager::DeviceSetupStatusFailure;
}

DeviceManager::DeviceError DevicePluginElgato::executeAction(Device *device, const Action &action)
{
    Q_UNUSED(action)
    // check deviceClassId
    if (device->deviceClassId() == aveaDeviceClassId) {
        AveaBulb *bulb = m_bulbs.value(device);

        Q_UNUSED(bulb)
//        // check actionTypeId
//        if (action.actionTypeId() == powerOffActionTypeId) {
//            bulb->actionPowerOff(action.id());
//            return DeviceManager::DeviceErrorAsync;
//        } else if (action.actionTypeId() == colorActionTypeId) {

//            return DeviceManager::DeviceErrorNoError;
//        }

        return DeviceManager::DeviceErrorActionTypeNotFound;
    }
    return DeviceManager::DeviceErrorDeviceClassNotFound;
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
        if (device->paramValue(aveaMacAddressParamTypeId).toString() == deviceInfo.address().toString())
            return true;
    }

    return false;
}

void DevicePluginElgato::onBluetoothDiscoveryFinished()
{
    BluetoothDiscoveryReply *reply = static_cast<BluetoothDiscoveryReply *>(sender());
    if (reply->error() != BluetoothDiscoveryReply::BluetoothDiscoveryReplyErrorNoError) {
        qCWarning(dcElgato()) << "Bluetooth discovery error:" << reply->error();
        reply->deleteLater();
        emit devicesDiscovered(aveaDeviceClassId, QList<DeviceDescriptor>());
        return;
    }

    QList<DeviceDescriptor> deviceDescriptors;
    foreach (const QBluetoothDeviceInfo &deviceInfo, reply->discoveredDevices()) {
        if (deviceInfo.name().contains("Avea")) {
            if (!verifyExistingDevices(deviceInfo)) {
                DeviceDescriptor descriptor(aveaDeviceClassId, "Avea", deviceInfo.address().toString());
                ParamList params;
                params.append(Param(aveaNameParamTypeId, deviceInfo.name()));
                params.append(Param(aveaMacAddressParamTypeId, deviceInfo.address().toString()));
                descriptor.setParams(params);
                deviceDescriptors.append(descriptor);
            }
        }
    }

    reply->deleteLater();

    emit devicesDiscovered(aveaDeviceClassId, deviceDescriptors);
}

