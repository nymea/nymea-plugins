/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *                                                                         *
 *  Copyright (C) 2015-2018 Simon Stuerz <simon.stuerz@guh.io>             *
 *  Copyright (C) 2016 nicc                                                *
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
    \page multisensor.html
    \title MultiSensor
    \brief Plugin for TI SensorTag.

    \ingroup plugins
    \ingroup nymea-plugins

    This plugin allows finding and controlling the Bluetooth Low Energy SensorTag from Texas Instruments.

    \chapter Plugin properties
    Following JSON file contains the definition and the description of all available \l{DeviceClass}{DeviceClasses}
    and \l{Vendor}{Vendors} of this \l{DevicePlugin}.

    For more details on how to read this JSON file please check out the documentation for \l{The plugin JSON File}.

    \quotefile plugins/deviceplugins/multisensor/devicepluginmultisensor.json
*/

#include "plugininfo.h"
#include "devicemanager.h"
#include "hardware/bluetoothlowenergy/bluetoothlowenergymanager.h"
#include "devicepluginmultisensor.h"

// http://processors.wiki.ti.com/index.php/SensorTag_User_Guide

DevicePluginMultiSensor::DevicePluginMultiSensor()
{

}

DevicePluginMultiSensor::~DevicePluginMultiSensor()
{
    hardwareManager()->pluginTimerManager()->unregisterTimer(m_reconnectTimer);
}

void DevicePluginMultiSensor::init()
{
    m_reconnectTimer = hardwareManager()->pluginTimerManager()->registerTimer(10);
    connect(m_reconnectTimer, &PluginTimer::timeout, this, &DevicePluginMultiSensor::onPluginTimer);
}

DeviceManager::DeviceError DevicePluginMultiSensor::discoverDevices(const DeviceClassId &deviceClassId, const ParamList &params)
{
    Q_UNUSED(params)

    if (deviceClassId != sensorTagDeviceClassId)
        return DeviceManager::DeviceErrorDeviceClassNotFound;

    if (!hardwareManager()->bluetoothLowEnergyManager()->available())
        return DeviceManager::DeviceErrorHardwareNotAvailable;

    if (!hardwareManager()->bluetoothLowEnergyManager()->enabled())
        return DeviceManager::DeviceErrorHardwareNotAvailable;

    BluetoothDiscoveryReply *reply = hardwareManager()->bluetoothLowEnergyManager()->discoverDevices();
    connect(reply, &BluetoothDiscoveryReply::finished, this, &DevicePluginMultiSensor::onBluetoothDiscoveryFinished);
    return DeviceManager::DeviceErrorAsync;
}

DeviceManager::DeviceSetupStatus DevicePluginMultiSensor::setupDevice(Device *device)
{
    qCDebug(dcMultiSensor) << "Setting up Multi Sensor" << device->name() << device->params();

    if (device->deviceClassId() == sensorTagDeviceClassId) {

        QBluetoothAddress address = QBluetoothAddress(device->paramValue(sensorTagDeviceMacParamTypeId).toString());
        QString name = device->paramValue(sensorTagDeviceNameParamTypeId).toString();
        QBluetoothDeviceInfo deviceInfo = QBluetoothDeviceInfo(address, name, 0);

        BluetoothLowEnergyDevice *bluetoothDevice = hardwareManager()->bluetoothLowEnergyManager()->registerDevice(deviceInfo, QLowEnergyController::PublicAddress);

        SensorTag *sensor = new SensorTag(device, bluetoothDevice, this);
        m_sensors.insert(device, sensor);

        return DeviceManager::DeviceSetupStatusSuccess;
    }
    return DeviceManager::DeviceSetupStatusFailure;
}

void DevicePluginMultiSensor::postSetupDevice(Device *device)
{
    // Try to connect right after setup
    SensorTag *sensor = m_sensors.value(device);

    // Configure sensor with state configurations
    sensor->setTemperatureSensorEnabled(device->stateValue(sensorTagTemperatureSensorEnabledStateTypeId).toBool());
    sensor->setHumiditySensorEnabled(device->stateValue(sensorTagHumiditySensorEnabledStateTypeId).toBool());
    sensor->setPressureSensorEnabled(device->stateValue(sensorTagPressureSensorEnabledStateTypeId).toBool());
    sensor->setOpticalSensorEnabled(device->stateValue(sensorTagOpticalSensorEnabledStateTypeId).toBool());
    sensor->setAccelerometerEnabled(device->stateValue(sensorTagAccelerometerEnabledStateTypeId).toBool());
    sensor->setGyroscopeEnabled(device->stateValue(sensorTagGyroscopeEnabledStateTypeId).toBool());
    sensor->setMagnetometerEnabled(device->stateValue(sensorTagMagnetometerEnabledStateTypeId).toBool());
    sensor->setMeasurementPeriod(device->stateValue(sensorTagMeasurementPeriodStateTypeId).toInt());
    sensor->setMeasurementPeriodMovement(device->stateValue(sensorTagMeasurementPeriodMovementStateTypeId).toInt());

    // Connect to the sensor
    sensor->bluetoothDevice()->connectDevice();
}

void DevicePluginMultiSensor::deviceRemoved(Device *device)
{
    if (!m_sensors.contains(device))
        return;

    SensorTag *sensor = m_sensors.value(device);
    m_sensors.remove(device);
    hardwareManager()->bluetoothLowEnergyManager()->unregisterDevice(sensor->bluetoothDevice());
    sensor->deleteLater();
}

DeviceManager::DeviceError DevicePluginMultiSensor::executeAction(Device *device, const Action &action)
{
    SensorTag *sensor = m_sensors.value(device);
    if (action.actionTypeId() == sensorTagBuzzerActionTypeId) {
        sensor->setBuzzerPower(action.param(sensorTagBuzzerActionBuzzerParamTypeId).value().toBool());
        return DeviceManager::DeviceErrorNoError;
    } else if (action.actionTypeId() == sensorTagGreenLedActionTypeId) {
        sensor->setGreenLedPower(action.param(sensorTagGreenLedActionGreenLedParamTypeId).value().toBool());
        return DeviceManager::DeviceErrorNoError;
    } else if (action.actionTypeId() == sensorTagRedLedActionTypeId) {
        sensor->setRedLedPower(action.param(sensorTagRedLedActionRedLedParamTypeId).value().toBool());
        return DeviceManager::DeviceErrorNoError;
    } else if (action.actionTypeId() == sensorTagBuzzerImpulseActionTypeId) {
        sensor->buzzerImpulse();
        return DeviceManager::DeviceErrorNoError;
    } else if (action.actionTypeId() == sensorTagTemperatureSensorEnabledActionTypeId) {
        bool enabled = action.param(sensorTagTemperatureSensorEnabledActionTemperatureSensorEnabledParamTypeId).value().toBool();
        device->setStateValue(sensorTagTemperatureSensorEnabledStateTypeId, enabled);
        sensor->setTemperatureSensorEnabled(enabled);
        return DeviceManager::DeviceErrorNoError;
    } else if (action.actionTypeId() == sensorTagHumiditySensorEnabledActionTypeId) {
        bool enabled = action.param(sensorTagHumiditySensorEnabledActionHumiditySensorEnabledParamTypeId).value().toBool();
        device->setStateValue(sensorTagHumiditySensorEnabledStateTypeId, enabled);
        sensor->setHumiditySensorEnabled(enabled);
        return DeviceManager::DeviceErrorNoError;
    } else if (action.actionTypeId() == sensorTagPressureSensorEnabledActionTypeId) {
        bool enabled = action.param(sensorTagPressureSensorEnabledActionPressureSensorEnabledParamTypeId).value().toBool();
        device->setStateValue(sensorTagPressureSensorEnabledStateTypeId, enabled);
        sensor->setPressureSensorEnabled(enabled);
        return DeviceManager::DeviceErrorNoError;
    } else if (action.actionTypeId() == sensorTagOpticalSensorEnabledActionTypeId) {
        bool enabled = action.param(sensorTagOpticalSensorEnabledActionOpticalSensorEnabledParamTypeId).value().toBool();
        device->setStateValue(sensorTagOpticalSensorEnabledStateTypeId, enabled);
        sensor->setOpticalSensorEnabled(enabled);
        return DeviceManager::DeviceErrorNoError;
    } else if (action.actionTypeId() == sensorTagAccelerometerEnabledActionTypeId) {
        bool enabled = action.param(sensorTagAccelerometerEnabledActionAccelerometerEnabledParamTypeId).value().toBool();
        device->setStateValue(sensorTagAccelerometerEnabledStateTypeId, enabled);
        sensor->setAccelerometerEnabled(enabled);
        return DeviceManager::DeviceErrorNoError;
    } else if (action.actionTypeId() == sensorTagGyroscopeEnabledActionTypeId) {
        bool enabled = action.param(sensorTagGyroscopeEnabledActionGyroscopeEnabledParamTypeId).value().toBool();
        device->setStateValue(sensorTagGyroscopeEnabledStateTypeId, enabled);
        sensor->setGyroscopeEnabled(enabled);
        return DeviceManager::DeviceErrorNoError;
    } else if (action.actionTypeId() == sensorTagMagnetometerEnabledActionTypeId) {
        bool enabled = action.param(sensorTagMagnetometerEnabledActionMagnetometerEnabledParamTypeId).value().toBool();
        device->setStateValue(sensorTagMagnetometerEnabledStateTypeId, enabled);
        sensor->setMagnetometerEnabled(enabled);
        return DeviceManager::DeviceErrorNoError;
    } else if (action.actionTypeId() == sensorTagMeasurementPeriodActionTypeId) {
        int period = action.param(sensorTagMeasurementPeriodActionMeasurementPeriodParamTypeId).value().toInt();
        device->setStateValue(sensorTagMeasurementPeriodStateTypeId, period);
        sensor->setMeasurementPeriod(period);
        return DeviceManager::DeviceErrorNoError;
    } else if (action.actionTypeId() == sensorTagMeasurementPeriodMovementActionTypeId) {
        int period = action.param(sensorTagMeasurementPeriodMovementActionMeasurementPeriodMovementParamTypeId).value().toInt();
        device->setStateValue(sensorTagMeasurementPeriodMovementStateTypeId, period);
        sensor->setMeasurementPeriodMovement(period);
        return DeviceManager::DeviceErrorNoError;
    } else if (action.actionTypeId() == sensorTagMovementSensitivityActionTypeId) {
        int sensitivity = action.param(sensorTagMovementSensitivityActionMovementSensitivityParamTypeId).value().toInt();
        device->setStateValue(sensorTagMovementSensitivityStateTypeId, sensitivity);
        sensor->setMovementSensitivity(sensitivity);
        return DeviceManager::DeviceErrorNoError;
    }

    return DeviceManager::DeviceErrorActionTypeNotFound;
}

bool DevicePluginMultiSensor::verifyExistingDevices(const QBluetoothDeviceInfo &deviceInfo)
{
    foreach (Device *device, m_sensors.keys()) {
        if (device->paramValue(sensorTagDeviceMacParamTypeId).toString() == deviceInfo.address().toString())
            return true;
    }

    return false;
}

void DevicePluginMultiSensor::onPluginTimer()
{
    foreach (SensorTag *sensor, m_sensors.values()) {
        if (!sensor->bluetoothDevice()->connected()) {
            sensor->bluetoothDevice()->connectDevice();
        }
    }
}

void DevicePluginMultiSensor::onBluetoothDiscoveryFinished()
{
    BluetoothDiscoveryReply *reply = static_cast<BluetoothDiscoveryReply *>(sender());
    if (reply->error() != BluetoothDiscoveryReply::BluetoothDiscoveryReplyErrorNoError) {
        qCWarning(dcMultiSensor()) << "Bluetooth discovery error:" << reply->error();
        reply->deleteLater();
        emit devicesDiscovered(sensorTagDeviceClassId, QList<DeviceDescriptor>());
        return;
    }

    QList<DeviceDescriptor> deviceDescriptors;
    foreach (const QBluetoothDeviceInfo &deviceInfo, reply->discoveredDevices()) {
        if (deviceInfo.name().contains("SensorTag")) {
            if (!verifyExistingDevices(deviceInfo)) {
                DeviceDescriptor descriptor(sensorTagDeviceClassId, "Sensor Tag", deviceInfo.address().toString());
                ParamList params;
                params.append(Param(sensorTagDeviceNameParamTypeId, deviceInfo.name()));
                params.append(Param(sensorTagDeviceMacParamTypeId, deviceInfo.address().toString()));
                descriptor.setParams(params);
                deviceDescriptors.append(descriptor);
            }
        }
    }

    reply->deleteLater();
    emit devicesDiscovered(sensorTagDeviceClassId, deviceDescriptors);
}
