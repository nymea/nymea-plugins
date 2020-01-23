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

#include "deviceplugintexasinstruments.h"
#include "plugininfo.h"
#include "sensortag.h"

#include "hardware/bluetoothlowenergy/bluetoothlowenergymanager.h"
#include "plugintimer.h"

#include <QBluetoothDeviceInfo>

DevicePluginTexasInstruments::DevicePluginTexasInstruments(QObject *parent) : DevicePlugin(parent)
{

}

DevicePluginTexasInstruments::~DevicePluginTexasInstruments()
{

}

void DevicePluginTexasInstruments::discoverDevices(DeviceDiscoveryInfo *info)
{
    Q_ASSERT_X(info->deviceClassId() == sensorTagDeviceClassId, "DevicePluginTexasInstruments", "Unhandled DeviceClassId!");

    if (!hardwareManager()->bluetoothLowEnergyManager()->available() || !hardwareManager()->bluetoothLowEnergyManager()->enabled()) {
        return info->finish(Device::DeviceErrorHardwareNotAvailable, QT_TR_NOOP("Bluetooth is not available on this system."));
    }

    BluetoothDiscoveryReply *reply = hardwareManager()->bluetoothLowEnergyManager()->discoverDevices();
    connect(reply, &BluetoothDiscoveryReply::finished, reply, &BluetoothDiscoveryReply::deleteLater);

    connect(reply, &BluetoothDiscoveryReply::finished, info, [this, info, reply](){

        if (reply->error() != BluetoothDiscoveryReply::BluetoothDiscoveryReplyErrorNoError) {
            qCWarning(dcTexasInstruments()) << "Bluetooth discovery error:" << reply->error();
            info->finish(Device::DeviceErrorHardwareFailure, QT_TR_NOOP("An error happened during Bluetooth discovery."));
            return;
        }

        foreach (const QBluetoothDeviceInfo &deviceInfo, reply->discoveredDevices()) {

            if (deviceInfo.name().contains("SensorTag")) {

                DeviceDescriptor descriptor(sensorTagDeviceClassId, "Sensor Tag", deviceInfo.address().toString());

                Devices existingDevice = myDevices().filterByParam(sensorTagDeviceMacParamTypeId, deviceInfo.address().toString());
                if (!existingDevice.isEmpty()) {
                    descriptor.setDeviceId(existingDevice.first()->id());
                }

                ParamList params;
                params.append(Param(sensorTagDeviceMacParamTypeId, deviceInfo.address().toString()));
                foreach (Device *existingDevice, myDevices()) {
                    if (existingDevice->paramValue(sensorTagDeviceMacParamTypeId).toString() == deviceInfo.address().toString()) {
                        descriptor.setDeviceId(existingDevice->id());
                        break;
                    }
                }
                descriptor.setParams(params);
                info->addDeviceDescriptor(descriptor);
            }
        }

        info->finish(Device::DeviceErrorNoError);
    });
}

void DevicePluginTexasInstruments::setupDevice(DeviceSetupInfo *info)
{
    Device *device = info->device();
    qCDebug(dcTexasInstruments()) << "Setting up Multi Sensor" << device->name() << device->params();

    QBluetoothAddress address = QBluetoothAddress(device->paramValue(sensorTagDeviceMacParamTypeId).toString());
    QBluetoothDeviceInfo deviceInfo = QBluetoothDeviceInfo(address, device->name(), 0);

    BluetoothLowEnergyDevice *bluetoothDevice = hardwareManager()->bluetoothLowEnergyManager()->registerDevice(deviceInfo, QLowEnergyController::PublicAddress);

    SensorTag *sensorTag = new SensorTag(device, bluetoothDevice, this);
    m_sensorTags.insert(device, sensorTag);

    if (!m_reconnectTimer) {
        m_reconnectTimer = hardwareManager()->pluginTimerManager()->registerTimer(10);
        connect(m_reconnectTimer, &PluginTimer::timeout, this, [this](){
            foreach (SensorTag *sensorTag, m_sensorTags) {
                if (!sensorTag->bluetoothDevice()->connected()) {
                    sensorTag->bluetoothDevice()->connectDevice();
                }
            }
        });
    }

    info->finish(Device::DeviceErrorNoError);
}

void DevicePluginTexasInstruments::postSetupDevice(Device *device)
{
    // Try to connect right after setup
    SensorTag *sensorTag = m_sensorTags.value(device);

    // Configure sensor with state configurations
    sensorTag->setTemperatureSensorEnabled(device->stateValue(sensorTagTemperatureSensorEnabledStateTypeId).toBool());
    sensorTag->setHumiditySensorEnabled(device->stateValue(sensorTagHumiditySensorEnabledStateTypeId).toBool());
    sensorTag->setPressureSensorEnabled(device->stateValue(sensorTagPressureSensorEnabledStateTypeId).toBool());
    sensorTag->setOpticalSensorEnabled(device->stateValue(sensorTagOpticalSensorEnabledStateTypeId).toBool());
    sensorTag->setAccelerometerEnabled(device->stateValue(sensorTagAccelerometerEnabledStateTypeId).toBool());
    sensorTag->setGyroscopeEnabled(device->stateValue(sensorTagGyroscopeEnabledStateTypeId).toBool());
    sensorTag->setMagnetometerEnabled(device->stateValue(sensorTagMagnetometerEnabledStateTypeId).toBool());
    sensorTag->setMeasurementPeriod(device->stateValue(sensorTagMeasurementPeriodStateTypeId).toInt());
    sensorTag->setMeasurementPeriodMovement(device->stateValue(sensorTagMeasurementPeriodMovementStateTypeId).toInt());

    // Connect to the sensor
    sensorTag->bluetoothDevice()->connectDevice();
}

void DevicePluginTexasInstruments::deviceRemoved(Device *device)
{
    if (!m_sensorTags.contains(device)) {
        return;
    }

    SensorTag *sensorTag = m_sensorTags.take(device);
    hardwareManager()->bluetoothLowEnergyManager()->unregisterDevice(sensorTag->bluetoothDevice());
    sensorTag->deleteLater();

    if (myDevices().isEmpty()) {
        hardwareManager()->pluginTimerManager()->unregisterTimer(m_reconnectTimer);
        m_reconnectTimer = nullptr;
    }
}

void DevicePluginTexasInstruments::executeAction(DeviceActionInfo *info)
{
    Device *device = info->device();
    Action action = info->action();

    SensorTag *sensorTag = m_sensorTags.value(device);
    if (action.actionTypeId() == sensorTagBuzzerActionTypeId) {
        sensorTag->setBuzzerPower(action.param(sensorTagBuzzerActionBuzzerParamTypeId).value().toBool());
        return info->finish(Device::DeviceErrorNoError);
    } else if (action.actionTypeId() == sensorTagGreenLedActionTypeId) {
        sensorTag->setGreenLedPower(action.param(sensorTagGreenLedActionGreenLedParamTypeId).value().toBool());
        return info->finish(Device::DeviceErrorNoError);
    } else if (action.actionTypeId() == sensorTagRedLedActionTypeId) {
        sensorTag->setRedLedPower(action.param(sensorTagRedLedActionRedLedParamTypeId).value().toBool());
        return info->finish(Device::DeviceErrorNoError);
    } else if (action.actionTypeId() == sensorTagBuzzerImpulseActionTypeId) {
        sensorTag->buzzerImpulse();
        return info->finish(Device::DeviceErrorNoError);
    } else if (action.actionTypeId() == sensorTagTemperatureSensorEnabledActionTypeId) {
        bool enabled = action.param(sensorTagTemperatureSensorEnabledActionTemperatureSensorEnabledParamTypeId).value().toBool();
        device->setStateValue(sensorTagTemperatureSensorEnabledStateTypeId, enabled);
        sensorTag->setTemperatureSensorEnabled(enabled);
        return info->finish(Device::DeviceErrorNoError);
    } else if (action.actionTypeId() == sensorTagHumiditySensorEnabledActionTypeId) {
        bool enabled = action.param(sensorTagHumiditySensorEnabledActionHumiditySensorEnabledParamTypeId).value().toBool();
        device->setStateValue(sensorTagHumiditySensorEnabledStateTypeId, enabled);
        sensorTag->setHumiditySensorEnabled(enabled);
        return info->finish(Device::DeviceErrorNoError);
    } else if (action.actionTypeId() == sensorTagPressureSensorEnabledActionTypeId) {
        bool enabled = action.param(sensorTagPressureSensorEnabledActionPressureSensorEnabledParamTypeId).value().toBool();
        device->setStateValue(sensorTagPressureSensorEnabledStateTypeId, enabled);
        sensorTag->setPressureSensorEnabled(enabled);
        return info->finish(Device::DeviceErrorNoError);
    } else if (action.actionTypeId() == sensorTagOpticalSensorEnabledActionTypeId) {
        bool enabled = action.param(sensorTagOpticalSensorEnabledActionOpticalSensorEnabledParamTypeId).value().toBool();
        device->setStateValue(sensorTagOpticalSensorEnabledStateTypeId, enabled);
        sensorTag->setOpticalSensorEnabled(enabled);
        return info->finish(Device::DeviceErrorNoError);
    } else if (action.actionTypeId() == sensorTagAccelerometerEnabledActionTypeId) {
        bool enabled = action.param(sensorTagAccelerometerEnabledActionAccelerometerEnabledParamTypeId).value().toBool();
        device->setStateValue(sensorTagAccelerometerEnabledStateTypeId, enabled);
        sensorTag->setAccelerometerEnabled(enabled);
        return info->finish(Device::DeviceErrorNoError);
    } else if (action.actionTypeId() == sensorTagGyroscopeEnabledActionTypeId) {
        bool enabled = action.param(sensorTagGyroscopeEnabledActionGyroscopeEnabledParamTypeId).value().toBool();
        device->setStateValue(sensorTagGyroscopeEnabledStateTypeId, enabled);
        sensorTag->setGyroscopeEnabled(enabled);
        return info->finish(Device::DeviceErrorNoError);
    } else if (action.actionTypeId() == sensorTagMagnetometerEnabledActionTypeId) {
        bool enabled = action.param(sensorTagMagnetometerEnabledActionMagnetometerEnabledParamTypeId).value().toBool();
        device->setStateValue(sensorTagMagnetometerEnabledStateTypeId, enabled);
        sensorTag->setMagnetometerEnabled(enabled);
        return info->finish(Device::DeviceErrorNoError);
    } else if (action.actionTypeId() == sensorTagMeasurementPeriodActionTypeId) {
        int period = action.param(sensorTagMeasurementPeriodActionMeasurementPeriodParamTypeId).value().toInt();
        device->setStateValue(sensorTagMeasurementPeriodStateTypeId, period);
        sensorTag->setMeasurementPeriod(period);
        return info->finish(Device::DeviceErrorNoError);
    } else if (action.actionTypeId() == sensorTagMeasurementPeriodMovementActionTypeId) {
        int period = action.param(sensorTagMeasurementPeriodMovementActionMeasurementPeriodMovementParamTypeId).value().toInt();
        device->setStateValue(sensorTagMeasurementPeriodMovementStateTypeId, period);
        sensorTag->setMeasurementPeriodMovement(period);
        return info->finish(Device::DeviceErrorNoError);
    } else if (action.actionTypeId() == sensorTagMovementSensitivityActionTypeId) {
        int sensitivity = action.param(sensorTagMovementSensitivityActionMovementSensitivityParamTypeId).value().toInt();
        device->setStateValue(sensorTagMovementSensitivityStateTypeId, sensitivity);
        sensorTag->setMovementSensitivity(sensitivity);
        return info->finish(Device::DeviceErrorNoError);
    }

    Q_ASSERT_X(false, "TexasInstruments", "Unhandled action type: " + action.actionTypeId().toString().toUtf8());
}
