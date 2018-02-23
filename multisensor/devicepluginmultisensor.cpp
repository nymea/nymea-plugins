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
    connect(this, &DevicePluginMultiSensor::configValueChanged, this, &DevicePluginMultiSensor::onPluginConfigurationChanged);

    m_reconnectTimer = hardwareManager()->pluginTimerManager()->registerTimer(10);
    connect(m_reconnectTimer, &PluginTimer::timeout, this, &DevicePluginMultiSensor::onPluginTimer);
}

DeviceManager::DeviceError DevicePluginMultiSensor::discoverDevices(const DeviceClassId &deviceClassId, const ParamList &params)
{
    Q_UNUSED(params)

    if (deviceClassId != sensortagDeviceClassId)
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

    if (device->deviceClassId() == sensortagDeviceClassId) {

        QBluetoothAddress address = QBluetoothAddress(device->paramValue(sensortagMacParamTypeId).toString());
        QString name = device->paramValue(sensortagNameParamTypeId).toString();
        QBluetoothDeviceInfo deviceInfo = QBluetoothDeviceInfo(address, name, 0);

        BluetoothLowEnergyDevice *bluetoothDevice = hardwareManager()->bluetoothLowEnergyManager()->registerDevice(deviceInfo, QLowEnergyController::PublicAddress);

        SensorTag *sensor = new SensorTag(device, bluetoothDevice, this);
        // Preconfigure with plugin configurations
        sensor->setAccelerometerEnabled(configValue(MultiSensorAccelerometerEnabledParamTypeId).toBool());
        sensor->setGyroscopeEnabled(configValue(MultiSensorGyroscopeEnabledParamTypeId).toBool());
        sensor->setMagnetometerEnabled(configValue(MultiSensorMagnetometerEnabledParamTypeId).toBool());
        sensor->setMeasurementPeriod(configValue(MultiSensorMeasurementPeriodParamTypeId).toInt());
        sensor->setMeasurementPeriodMovement(configValue(MultiSensorMeasurementPeriodMovementParamTypeId).toInt());

        m_sensors.insert(device, sensor);

        return DeviceManager::DeviceSetupStatusSuccess;
    }
    return DeviceManager::DeviceSetupStatusFailure;
}

void DevicePluginMultiSensor::postSetupDevice(Device *device)
{
    // Try to connect right after setup
    SensorTag *sensor = m_sensors.value(device);
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
    if (action.actionTypeId() == sensortagBuzzerActionTypeId) {
        sensor->setBuzzerPower(action.param(sensortagBuzzerStateParamTypeId).value().toBool());
        return DeviceManager::DeviceErrorNoError;
    } else if (action.actionTypeId() == sensortagGreenLedActionTypeId) {
        sensor->setGreenLedPower(action.param(sensortagGreenLedStateParamTypeId).value().toBool());
        return DeviceManager::DeviceErrorNoError;
    } else if (action.actionTypeId() == sensortagRedLedActionTypeId) {
        sensor->setRedLedPower(action.param(sensortagRedLedStateParamTypeId).value().toBool());
        return DeviceManager::DeviceErrorNoError;
    } else if (action.actionTypeId() == sensortagBuzzerImpulseActionTypeId) {
        sensor->buzzerImpulse();
        return DeviceManager::DeviceErrorNoError;
    }

    return DeviceManager::DeviceErrorActionTypeNotFound;
}

bool DevicePluginMultiSensor::verifyExistingDevices(const QBluetoothDeviceInfo &deviceInfo)
{
    foreach (Device *device, m_sensors.keys()) {
        if (device->paramValue(sensortagMacParamTypeId).toString() == deviceInfo.address().toString())
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
        emit devicesDiscovered(sensortagDeviceClassId, QList<DeviceDescriptor>());
        return;
    }

    QList<DeviceDescriptor> deviceDescriptors;
    foreach (const QBluetoothDeviceInfo &deviceInfo, reply->discoveredDevices()) {
        if (deviceInfo.name().contains("SensorTag")) {
            if (!verifyExistingDevices(deviceInfo)) {
                DeviceDescriptor descriptor(sensortagDeviceClassId, "Sensor Tag", deviceInfo.address().toString());
                ParamList params;
                params.append(Param(sensortagNameParamTypeId, deviceInfo.name()));
                params.append(Param(sensortagMacParamTypeId, deviceInfo.address().toString()));
                descriptor.setParams(params);
                deviceDescriptors.append(descriptor);
            }
        }
    }

    reply->deleteLater();
    emit devicesDiscovered(sensortagDeviceClassId, deviceDescriptors);
}

void DevicePluginMultiSensor::onPluginConfigurationChanged(const ParamTypeId &paramTypeId, const QVariant &value)
{
    qCDebug(dcMultiSensor()) << "Plugin configuration changed" << paramTypeId.toString() << value;

    foreach (SensorTag *sensor, m_sensors.values()) {
        if (paramTypeId == MultiSensorAccelerometerEnabledParamTypeId) {
            sensor->setAccelerometerEnabled(value.toBool());
        } else if (paramTypeId == MultiSensorGyroscopeEnabledParamTypeId) {
            sensor->setGyroscopeEnabled(value.toBool());
        } else if (paramTypeId == MultiSensorMagnetometerEnabledParamTypeId) {
            sensor->setMagnetometerEnabled(value.toBool());
        } else if (paramTypeId == MultiSensorMeasurementPeriodParamTypeId) {
            sensor->setMeasurementPeriod(value.toInt());
        } else if (paramTypeId == MultiSensorMeasurementPeriodParamTypeId) {
            sensor->setMeasurementPeriodMovement(value.toInt());
        } else if (paramTypeId == MultiSensorMovementSensitivityParamTypeId) {
            sensor->setMovementSensitivity(value.toInt());
        }
    }
}
