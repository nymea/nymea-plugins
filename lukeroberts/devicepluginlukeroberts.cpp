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

#include "devicepluginsenic.h"
#include "devices/device.h"
#include "plugininfo.h"
#include "hardware/bluetoothlowenergy/bluetoothlowenergymanager.h"

DevicePluginSenic::DevicePluginSenic()
{

}

void DevicePluginSenic::init()
{
    // Initialize plugin configurations
    m_autoSymbolMode = configValue(senicPluginAutoSymbolsParamTypeId).toBool();
    connect(this, &DevicePluginSenic::configValueChanged, this, &DevicePluginSenic::onPluginConfigurationChanged);
}


void DevicePluginSenic::discoverDevices(DeviceDiscoveryInfo *info)
{
    if (!hardwareManager()->bluetoothLowEnergyManager()->available())
        return info->finish(Device::DeviceErrorHardwareNotAvailable, QT_TR_NOOP("Bluetooth is not available on this system."));

    if (!hardwareManager()->bluetoothLowEnergyManager()->enabled())
        return info->finish(Device::DeviceErrorHardwareNotAvailable, QT_TR_NOOP("Bluetooth is disabled. Please enable Bluetooth and try again."));

    BluetoothDiscoveryReply *reply = hardwareManager()->bluetoothLowEnergyManager()->discoverDevices();
    connect(reply, &BluetoothDiscoveryReply::finished, reply, &BluetoothDiscoveryReply::deleteLater);

    connect(reply, &BluetoothDiscoveryReply::finished, info, [this, info, reply](){

        if (reply->error() != BluetoothDiscoveryReply::BluetoothDiscoveryReplyErrorNoError) {
            qCWarning(dcSenic()) << "Bluetooth discovery error:" << reply->error();
            info->finish(Device::DeviceErrorHardwareFailure, QT_TR_NOOP("An error happened during Bluetooth discovery."));
            return;
        }

        foreach (const QBluetoothDeviceInfo &deviceInfo, reply->discoveredDevices()) {
            if (deviceInfo.name().contains("Nuimo")) {
                DeviceDescriptor descriptor(nuimoDeviceClassId, "Nuimo", deviceInfo.name() + " (" + deviceInfo.address().toString() + ")");
                ParamList params;

                foreach (Device *existingDevice, myDevices()) {
                    if (existingDevice->paramValue(nuimoDeviceMacParamTypeId).toString() == deviceInfo.address().toString()) {
                        descriptor.setDeviceId(existingDevice->id());
                        break;
                    }
                }
                params.append(Param(nuimoDeviceMacParamTypeId, deviceInfo.address().toString()));
                descriptor.setParams(params);
                info->addDeviceDescriptor(descriptor);
            }
        }
        info->finish(Device::DeviceErrorNoError);
    });
}


void DevicePluginSenic::setupDevice(DeviceSetupInfo *info)
{
    Device *device = info->device();

    qCDebug(dcSenic()) << "Setup device" << device->name() << device->params();

    QBluetoothAddress address = QBluetoothAddress(device->paramValue(nuimoDeviceMacParamTypeId).toString());
    QBluetoothDeviceInfo deviceInfo = QBluetoothDeviceInfo(address, device->name(), 0);

    BluetoothLowEnergyDevice *bluetoothDevice = hardwareManager()->bluetoothLowEnergyManager()->registerDevice(deviceInfo, QLowEnergyController::RandomAddress);

    Nuimo *nuimo = new Nuimo(bluetoothDevice, this);
    nuimo->setLongPressTime(configValue(senicPluginLongPressTimeParamTypeId).toInt());
    connect(nuimo, &Nuimo::buttonPressed, this, &DevicePluginSenic::onButtonPressed);
    connect(nuimo, &Nuimo::buttonLongPressed, this, &DevicePluginSenic::onButtonLongPressed);
    connect(nuimo, &Nuimo::swipeDetected, this, &DevicePluginSenic::onSwipeDetected);
    connect(nuimo, &Nuimo::rotationValueChanged, this, &DevicePluginSenic::onRotationValueChanged);
    connect(nuimo, &Nuimo::connectedChanged, this, &DevicePluginSenic::onConnectedChanged);
    connect(nuimo, &Nuimo::deviceInformationChanged, this, &DevicePluginSenic::onDeviceInformationChanged);
    connect(nuimo, &Nuimo::batteryValueChanged, this, &DevicePluginSenic::onBatteryValueChanged);

    m_nuimos.insert(nuimo, device);

    connect(nuimo, &Nuimo::deviceInitializationFinished, info, [this, info, nuimo](bool success){
        Device *device = info->device();

        if (!device->setupComplete()) {
            if (success) {
                info->finish(Device::DeviceErrorNoError);
            } else {
                m_nuimos.take(nuimo);

                hardwareManager()->bluetoothLowEnergyManager()->unregisterDevice(nuimo->bluetoothDevice());
                nuimo->deleteLater();

                info->finish(Device::DeviceErrorHardwareFailure, QT_TR_NOOP("Error connecting to nuimo."));
            }
        }

    });


    nuimo->bluetoothDevice()->connectDevice();
}

void DevicePluginSenic::postSetupDevice(Device *device)
{
    Q_UNUSED(device)

    if (!m_reconnectTimer) {
        m_reconnectTimer = hardwareManager()->pluginTimerManager()->registerTimer(10);
        connect(m_reconnectTimer, &PluginTimer::timeout, this, &DevicePluginSenic::onReconnectTimeout);
    }
}


void DevicePluginSenic::executeAction(DeviceActionInfo *info)
{
    Device *device = info->device();
    Action action = info->action();

    QPointer<Nuimo> nuimo = m_nuimos.key(device);
    if (nuimo.isNull())
        return info->finish(Device::DeviceErrorHardwareFailure);

    if (!nuimo->bluetoothDevice()->connected()) {
        return info->finish(Device::DeviceErrorHardwareNotAvailable);
    }

    if (action.actionTypeId() == nuimoShowLogoActionTypeId) {

        if (action.param(nuimoShowLogoActionLogoParamTypeId).value().toString() == "Up")
            nuimo->showImage(Nuimo::MatrixTypeUp);
        if (action.param(nuimoShowLogoActionLogoParamTypeId).value().toString() == "Down")
            nuimo->showImage(Nuimo::MatrixTypeDown);
        if (action.param(nuimoShowLogoActionLogoParamTypeId).value().toString() == "Left")
            nuimo->showImage(Nuimo::MatrixTypeLeft);
        if (action.param(nuimoShowLogoActionLogoParamTypeId).value().toString() == "Right")
            nuimo->showImage(Nuimo::MatrixTypeRight);
        if (action.param(nuimoShowLogoActionLogoParamTypeId).value().toString() == "Play")
            nuimo->showImage(Nuimo::MatrixTypePlay);
        if (action.param(nuimoShowLogoActionLogoParamTypeId).value().toString() == "Pause")
            nuimo->showImage(Nuimo::MatrixTypePause);
        if (action.param(nuimoShowLogoActionLogoParamTypeId).value().toString() == "Stop")
            nuimo->showImage(Nuimo::MatrixTypeStop);
        if (action.param(nuimoShowLogoActionLogoParamTypeId).value().toString() == "Music")
            nuimo->showImage(Nuimo::MatrixTypeMusic);
        if (action.param(nuimoShowLogoActionLogoParamTypeId).value().toString() == "Heart")
            nuimo->showImage(Nuimo::MatrixTypeHeart);
        if (action.param(nuimoShowLogoActionLogoParamTypeId).value().toString() == "Next")
            nuimo->showImage(Nuimo::MatrixTypeNext);
        if (action.param(nuimoShowLogoActionLogoParamTypeId).value().toString() == "Previous")
            nuimo->showImage(Nuimo::MatrixTypePrevious);
        if (action.param(nuimoShowLogoActionLogoParamTypeId).value().toString() == "Circle")
            nuimo->showImage(Nuimo::MatrixTypeCircle);
        if (action.param(nuimoShowLogoActionLogoParamTypeId).value().toString() == "Light")
            nuimo->showImage(Nuimo::MatrixTypeLight);

        return info->finish(Device::DeviceErrorNoError);
    }
}


void DevicePluginSenic::deviceRemoved(Device *device)
{
    if (!m_nuimos.values().contains(device))
        return;

    Nuimo *nuimo = m_nuimos.key(device);
    m_nuimos.take(nuimo);

    hardwareManager()->bluetoothLowEnergyManager()->unregisterDevice(nuimo->bluetoothDevice());
    nuimo->deleteLater();

    if (myDevices().isEmpty()) {
        hardwareManager()->pluginTimerManager()->unregisterTimer(m_reconnectTimer);
        m_reconnectTimer = nullptr;
    }
}


void DevicePluginSenic::onReconnectTimeout()
{
    foreach (Nuimo *nuimo, m_nuimos.keys()) {
        if (!nuimo->bluetoothDevice()->connected()) {
            nuimo->bluetoothDevice()->connectDevice();
        }
    }
}

void DevicePluginSenic::onConnectedChanged(bool connected)
{
    Nuimo *nuimo = static_cast<Nuimo *>(sender());
    Device *device = m_nuimos.value(nuimo);
    device->setStateValue(nuimoConnectedStateTypeId, connected);
}


void DevicePluginSenic::onButtonPressed()
{
    Nuimo *nuimo = static_cast<Nuimo *>(sender());
    Device *device = m_nuimos.value(nuimo);
    emitEvent(Event(nuimoPressedEventTypeId, device->id(), ParamList() << Param(nuimoPressedEventButtonNameParamTypeId, "•")));

    if (m_autoSymbolMode) {
        nuimo->showImage(Nuimo::MatrixTypeCircle);
    }
}


void DevicePluginSenic::onButtonLongPressed()
{
    Nuimo *nuimo = static_cast<Nuimo *>(sender());
    Device *device = m_nuimos.value(nuimo);
    emitEvent(Event(nuimoLongPressedEventTypeId, device->id()));

    if (m_autoSymbolMode) {
        nuimo->showImage(Nuimo::MatrixTypeFilledCircle);
    }
}


void DevicePluginSenic::onSwipeDetected(const Nuimo::SwipeDirection &direction)
{
    Nuimo *nuimo = static_cast<Nuimo *>(sender());
    Device *device = m_nuimos.value(nuimo);

    switch (direction) {
    case Nuimo::SwipeDirectionLeft:
        emitEvent(Event(nuimoPressedEventTypeId, device->id(), ParamList() << Param(nuimoPressedEventButtonNameParamTypeId, "←")));
        break;
    case Nuimo::SwipeDirectionRight:
        emitEvent(Event(nuimoPressedEventTypeId, device->id(), ParamList() << Param(nuimoPressedEventButtonNameParamTypeId, "→")));
        break;
    case Nuimo::SwipeDirectionUp:
        emitEvent(Event(nuimoPressedEventTypeId, device->id(), ParamList() << Param(nuimoPressedEventButtonNameParamTypeId, "↑")));
        break;
    case Nuimo::SwipeDirectionDown:
        emitEvent(Event(nuimoPressedEventTypeId, device->id(), ParamList() << Param(nuimoPressedEventButtonNameParamTypeId, "↓")));
        break;
    }

    if (m_autoSymbolMode) {
        switch (direction) {
        case Nuimo::SwipeDirectionLeft:
            nuimo->showImage(Nuimo::MatrixType::MatrixTypeLeft);
            break;
        case Nuimo::SwipeDirectionRight:
            nuimo->showImage(Nuimo::MatrixType::MatrixTypeRight);
            break;
        case Nuimo::SwipeDirectionUp:
            nuimo->showImage(Nuimo::MatrixType::MatrixTypeUp);
            break;
        case Nuimo::SwipeDirectionDown:
            nuimo->showImage(Nuimo::MatrixType::MatrixTypeDown);
            break;
        }
    }
}


void DevicePluginSenic::onRotationValueChanged(const uint &value)
{
    Nuimo *nuimo = static_cast<Nuimo *>(sender());
    Device *device = m_nuimos.value(nuimo);
    device->setStateValue(nuimoRotationStateTypeId, value);
}


void DevicePluginSenic::onDeviceInformationChanged(const QString &firmwareRevision, const QString &hardwareRevision, const QString &softwareRevision)
{
    Nuimo *nuimo = static_cast<Nuimo *>(sender());
    Device *device = m_nuimos.value(nuimo);

    device->setStateValue(nuimoFirmwareRevisionStateTypeId, firmwareRevision);
    device->setStateValue(nuimoHardwareRevisionStateTypeId, hardwareRevision);
    device->setStateValue(nuimoSoftwareRevisionStateTypeId, softwareRevision);
}


void DevicePluginSenic::onPluginConfigurationChanged(const ParamTypeId &paramTypeId, const QVariant &value)
{
    qCDebug(dcSenic()) << "Plugin configuration changed";

    // Check auto symbol mode
    if (paramTypeId == senicPluginAutoSymbolsParamTypeId) {
        qCDebug(dcSenic()) << "Auto symbol mode" << (value.toBool() ? "enabled." : "disabled.");
        m_autoSymbolMode = value.toBool();
    }

    if (paramTypeId == senicPluginLongPressTimeParamTypeId) {
        qCDebug(dcSenic()) << "Long press time" << value.toInt();
        foreach(Nuimo *nuimo, m_nuimos.keys()) {
            nuimo->setLongPressTime(value.toInt());
        }
    }
}

void DevicePluginSenic::onBatteryValueChanged(const uint &percentage)
{
    Nuimo *nuimo = static_cast<Nuimo *>(sender());
    Device *device = m_nuimos.value(nuimo);

    device->setStateValue(nuimoBatteryLevelStateTypeId, percentage);
    if (percentage < 20) {
        device->setStateValue(nuimoBatteryCriticalStateTypeId, true);
    } else {
        device->setStateValue(nuimoBatteryCriticalStateTypeId, false);
    }
}

void DevicePluginLukeRoberts::onStatusCodeReceived(LukeRoberts::StatusCodes statusCode)
{


}
