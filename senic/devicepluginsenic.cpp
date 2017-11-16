/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *                                                                         *
 *  Copyright (C) 2016 Simon Stürz <simon.stuerz@guh.io>                   *
 *                                                                         *
 *  This file is part of guh.                                              *
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
    \page senic.html
    \title Senic - Nuimo
    \brief Plugin for Senic Nuimo.

    \ingroup plugins
    \ingroup guh-plugins

    \chapter Plugin properties
    Following JSON file contains the definition and the description of all available \l{DeviceClass}{DeviceClasses}
    and \l{Vendor}{Vendors} of this \l{DevicePlugin}.

    For more details how to read this JSON file please check out the documentation for \l{The plugin JSON File}.

    \quotefile plugins/deviceplugins/senic/devicepluginsenic.json
*/

#include "devicepluginsenic.h"
#include "plugin/device.h"
#include "devicemanager.h"
#include "plugininfo.h"
#include "hardware/bluetoothlowenergy/bluetoothlowenergymanager.h"

DevicePluginSenic::DevicePluginSenic()
{

}

void DevicePluginSenic::init()
{
    m_reconnectTimer = hardwareManager()->pluginTimerManager()->registerTimer(10);
    connect(m_reconnectTimer, &PluginTimer::timeout, this, &DevicePluginSenic::onReconnectTimeout);
}

DeviceManager::DeviceError DevicePluginSenic::discoverDevices(const DeviceClassId &deviceClassId, const ParamList &params)
{
    Q_UNUSED(params)

    if (deviceClassId != nuimoDeviceClassId)
        return DeviceManager::DeviceErrorDeviceClassNotFound;

    if (!hardwareManager()->bluetoothLowEnergyManager()->available())
        return DeviceManager::DeviceErrorHardwareNotAvailable;

    if (!hardwareManager()->bluetoothLowEnergyManager()->enabled())
        return DeviceManager::DeviceErrorHardwareNotAvailable;

    BluetoothDiscoveryReply *reply = hardwareManager()->bluetoothLowEnergyManager()->discoverDevices();
    connect(reply, &BluetoothDiscoveryReply::finished, this, &DevicePluginSenic::onBluetoothDiscoveryFinished);

    return DeviceManager::DeviceErrorAsync;
}

DeviceManager::DeviceSetupStatus DevicePluginSenic::setupDevice(Device *device)
{
    qCDebug(dcSenic()) << "Setup device" << device->name() << device->params();

    QString name = device->paramValue(nuimoNameParamTypeId).toString();
    QBluetoothAddress address = QBluetoothAddress(device->paramValue(nuimoMacParamTypeId).toString());
    QBluetoothDeviceInfo deviceInfo = QBluetoothDeviceInfo(address, name, 0);

    BluetoothLowEnergyDevice *bluetoothDevice = hardwareManager()->bluetoothLowEnergyManager()->registerDevice(deviceInfo, QLowEnergyController::RandomAddress);

    Nuimo *nuimo = new Nuimo(device, bluetoothDevice, this);
    connect(nuimo, &Nuimo::buttonPressed, this, &DevicePluginSenic::onButtonPressed);
    connect(nuimo, &Nuimo::buttonReleased, this, &DevicePluginSenic::onButtonReleased);
    connect(nuimo, &Nuimo::swipeDetected, this, &DevicePluginSenic::onSwipeDetected);
    connect(nuimo, &Nuimo::rotationValueChanged, this, &DevicePluginSenic::onRotationValueChanged);

    m_nuimos.insert(nuimo, device);
    nuimo->bluetoothDevice()->connectDevice();

    return DeviceManager::DeviceSetupStatusSuccess;
}

DeviceManager::DeviceError DevicePluginSenic::executeAction(Device *device, const Action &action)
{
    QPointer<Nuimo> nuimo = m_nuimos.key(device);
    if (nuimo.isNull())
        return DeviceManager::DeviceErrorHardwareFailure;

    if (action.actionTypeId() == nuimoShowLogoActionTypeId) {
        if (action.param(nuimoLogoParamTypeId).value().toString() == "Guh")
            nuimo->showGuhLogo();

        if (action.param(nuimoLogoParamTypeId).value().toString() == "Arrow up")
            nuimo->showArrowUp();

        if (action.param(nuimoLogoParamTypeId).value().toString() == "Arrow down")
            nuimo->showArrowDown();

        return DeviceManager::DeviceErrorNoError;
    }

    return DeviceManager::DeviceErrorActionTypeNotFound;
}

void DevicePluginSenic::deviceRemoved(Device *device)
{
    if (!m_nuimos.values().contains(device))
        return;

    Nuimo *nuimo = m_nuimos.key(device);
    m_nuimos.take(nuimo);
    delete nuimo;
}

bool DevicePluginSenic::verifyExistingDevices(const QBluetoothDeviceInfo &deviceInfo)
{
    foreach (Device *device, myDevices()) {
        if (device->paramValue(nuimoMacParamTypeId).toString() == deviceInfo.address().toString())
            return true;
    }

    return false;
}

void DevicePluginSenic::onReconnectTimeout()
{
    foreach (Nuimo *nuimo, m_nuimos.keys()) {
        if (!nuimo->bluetoothDevice()->connected()) {
            nuimo->bluetoothDevice()->connectDevice();
        }
    }
}

void DevicePluginSenic::onBluetoothDiscoveryFinished()
{
    BluetoothDiscoveryReply *reply = static_cast<BluetoothDiscoveryReply *>(sender());
    if (reply->error() != BluetoothDiscoveryReply::BluetoothDiscoveryReplyErrorNoError) {
        qCWarning(dcSenic()) << "Bluetooth discovery error:" << reply->error();
        reply->deleteLater();
        emit devicesDiscovered(nuimoDeviceClassId, QList<DeviceDescriptor>());
        return;
    }


    QList<DeviceDescriptor> deviceDescriptors;
    foreach (const QBluetoothDeviceInfo &deviceInfo, reply->discoveredDevices()) {
        if (deviceInfo.name().contains("Nuimo")) {
            if (!verifyExistingDevices(deviceInfo)) {
                DeviceDescriptor descriptor(nuimoDeviceClassId, "Nuimo", deviceInfo.address().toString());
                ParamList params;
                params.append(Param(nuimoNameParamTypeId, deviceInfo.name()));
                params.append(Param(nuimoMacParamTypeId, deviceInfo.address().toString()));
                descriptor.setParams(params);
                deviceDescriptors.append(descriptor);
            }
        }
    }

    reply->deleteLater();
    emit devicesDiscovered(nuimoDeviceClassId, deviceDescriptors);
}


void DevicePluginSenic::onButtonPressed()
{
    Nuimo *nuimo = static_cast<Nuimo *>(sender());
    Device *device = m_nuimos.value(nuimo);
    emitEvent(Event(nuimoClickedEventTypeId, device->id()));
}

void DevicePluginSenic::onButtonReleased()
{
    // TODO: user timer for detekt long pressed (if needed)
}

void DevicePluginSenic::onSwipeDetected(const Nuimo::SwipeDirection &direction)
{
    Nuimo *nuimo = static_cast<Nuimo *>(sender());
    Device *device = m_nuimos.value(nuimo);

    switch (direction) {
    case Nuimo::SwipeDirectionLeft:
        emitEvent(Event(nuimoSwipeLeftEventTypeId, device->id()));
        break;
    case Nuimo::SwipeDirectionRight:
        emitEvent(Event(nuimoSwipeRightEventTypeId, device->id()));
        break;
    case Nuimo::SwipeDirectionUp:
        emitEvent(Event(nuimoSwipeUpEventTypeId, device->id()));
        break;
    case Nuimo::SwipeDirectionDown:
        emitEvent(Event(nuimoSwipeDownEventTypeId, device->id()));
        break;
    default:
        break;
    }
}

void DevicePluginSenic::onRotationValueChanged(const uint &value)
{
    Nuimo *nuimo = static_cast<Nuimo *>(sender());
    Device *device = m_nuimos.value(nuimo);
    device->setStateValue(nuimoRotationStateTypeId, value);
}


