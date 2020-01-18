/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *                                                                         *
 *  Copyright (C) 2015 Simon Stürz <simon.stuerz@guh.io>                   *
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

#include "deviceplugineq-3.h"

#include "devices/device.h"
#include "types/param.h"
#include "plugininfo.h"

#include "eqivabluetooth.h"

#include <QDebug>

DevicePluginEQ3::DevicePluginEQ3()
{

}

DevicePluginEQ3::~DevicePluginEQ3()
{
    hardwareManager()->pluginTimerManager()->unregisterTimer(m_pluginTimer);
}

void DevicePluginEQ3::init()
{
    qCDebug(dcEQ3()) << "Initializing EQ-3 Plugin";

    m_pluginTimer = hardwareManager()->pluginTimerManager()->registerTimer(10);
    connect(m_pluginTimer, &PluginTimer::timeout, this, &DevicePluginEQ3::onPluginTimer);
}

void DevicePluginEQ3::discoverDevices(DeviceDiscoveryInfo *info)
{
    DeviceClassId deviceClassId = info->deviceClassId();
    qCDebug(dcEQ3()) << "Discover devices called";
    if(deviceClassId == cubeDeviceClassId){

        MaxCubeDiscovery *cubeDiscovery = new MaxCubeDiscovery(this);

        connect(info, &QObject::destroyed, cubeDiscovery, &MaxCubeDiscovery::deleteLater);

        connect(cubeDiscovery, &MaxCubeDiscovery::cubesDetected, info, [this, info, cubeDiscovery](const QList<MaxCubeDiscovery::CubeInfo> &cubeList){

            foreach (const MaxCubeDiscovery::CubeInfo &cube, cubeList) {
                DeviceDescriptor descriptor(cubeDeviceClassId, "Max! Cube LAN Gateway", cube.serialNumber);
                ParamList params;
                params << Param(cubeDeviceHostParamTypeId, cube.hostAddress.toString());
                params << Param(cubeDevicePortParamTypeId, cube.port);
                params << Param(cubeDeviceFirmwareParamTypeId, cube.firmware);
                params << Param(cubeDeviceSerialParamTypeId, cube.serialNumber);

                foreach (Device *existingDevice, myDevices()) {
                    if (existingDevice->paramValue(cubeDeviceSerialParamTypeId).toString() == cube.serialNumber) {
                        descriptor.setDeviceId(existingDevice->id());
                        break;
                    }
                }

                descriptor.setParams(params);
                info->addDeviceDescriptor(descriptor);
            }

            info->finish(Device::DeviceErrorNoError);
        });

        cubeDiscovery->detectCubes();
        return;
    }
    if (deviceClassId == eqivaBluetoothDeviceClassId) {
        EqivaBluetoothDiscovery *eqivaBluetoothDiscovery = new EqivaBluetoothDiscovery(hardwareManager()->bluetoothLowEnergyManager(), this);

        // Clean up the discovery when the DiscoveryInfo goes away...
        connect(info, &QObject::destroyed, eqivaBluetoothDiscovery, &EqivaBluetoothDiscovery::deleteLater);

        // Discovery result handler
        connect(eqivaBluetoothDiscovery, &EqivaBluetoothDiscovery::finished, info, [this, info](const QStringList results){
            qCDebug(dcEQ3()) << "Discovery finished";

            foreach (const QString &result, results) {
                qCDebug(dcEQ3()) << "Discovered EQ-3 device" << result;
                DeviceDescriptor descriptor(eqivaBluetoothDeviceClassId, "Eqiva Bluetooth Thermostat", result);
                ParamList params;
                params << Param(eqivaBluetoothDeviceMacAddressParamTypeId, result);
                descriptor.setParams(params);
                foreach (Device* existingDevice, myDevices()) {
                    if (existingDevice->paramValue(eqivaBluetoothDeviceMacAddressParamTypeId).toString() == result) {
                        descriptor.setDeviceId(existingDevice->id());
                        break;
                    }
                }
                info->addDeviceDescriptor(descriptor);
            }

            info->finish(Device::DeviceErrorNoError);
        });

        // start the discovery
        bool ret = eqivaBluetoothDiscovery->startDiscovery();
        if (!ret) {
            return info->finish(Device::DeviceErrorHardwareNotAvailable, QT_TR_NOOP("Bluetooth discovery failed. Is Bluetooth available and enabled?"));
        }
        return;
    }

    info->finish(Device::DeviceErrorDeviceClassNotFound);
}


void DevicePluginEQ3::setupDevice(DeviceSetupInfo *info)
{
    Device *device = info->device();

    qCDebug(dcEQ3) << "Setup device" << device->params();

    if(device->deviceClassId() == cubeDeviceClassId){
        MaxCube *cube = new MaxCube(this,device->paramValue(cubeDeviceSerialParamTypeId).toString(),QHostAddress(device->paramValue(cubeDeviceHostParamTypeId).toString()),device->paramValue(cubeDevicePortParamTypeId).toInt());
        m_cubes.insert(cube,device);

        connect(cube,SIGNAL(cubeConnectionStatusChanged(bool)),this,SLOT(cubeConnectionStatusChanged(bool)));
        connect(cube,SIGNAL(commandActionFinished(bool,ActionId)),this,SLOT(commandActionFinished(bool,ActionId)));
        connect(cube,SIGNAL(cubeConfigReady()),this,SLOT(updateCubeConfig()));
        connect(cube,SIGNAL(wallThermostatFound()),this,SLOT(wallThermostatFound()));
        connect(cube,SIGNAL(wallThermostatDataUpdated()),this,SLOT(wallThermostatDataUpdated()));
        connect(cube,SIGNAL(radiatorThermostatFound()),this,SLOT(radiatorThermostatFound()));
        connect(cube,SIGNAL(radiatorThermostatDataUpdated()),this,SLOT(radiatorThermostatDataUpdated()));

        cube->connectToCube();
        connect(cube, &MaxCube::cubeConnectionStatusChanged, info, [info](bool connected){
            if (connected) {
                info->finish(Device::DeviceErrorNoError);
            } else {
                info->finish(Device::DeviceErrorHardwareFailure);
            }
        });
        return;
    }

    if(device->deviceClassId() == wallThermostateDeviceClassId){
        device->setName("Max! Wall Thermostat (" + device->paramValue(wallThermostateDeviceSerialParamTypeId).toString() + ")");
    }

    if (device->deviceClassId() == eqivaBluetoothDeviceClassId) {
        EqivaBluetooth *eqivaDevice = new EqivaBluetooth(hardwareManager()->bluetoothLowEnergyManager(), QBluetoothAddress(device->paramValue(eqivaBluetoothDeviceMacAddressParamTypeId).toString()), device->name(), this);
        m_eqivaDevices.insert(device, eqivaDevice);

        connect(device, &Device::nameChanged, eqivaDevice, [device, eqivaDevice](){
            eqivaDevice->setName(device->name());
        });

        // Connected state
        device->setStateValue(eqivaBluetoothConnectedStateTypeId, eqivaDevice->available());
        connect(eqivaDevice, &EqivaBluetooth::availableChanged, device, [device, eqivaDevice](){
            device->setStateValue(eqivaBluetoothConnectedStateTypeId, eqivaDevice->available());
        });
        // Power state
        device->setStateValue(eqivaBluetoothPowerStateTypeId, eqivaDevice->enabled());
        connect(eqivaDevice, &EqivaBluetooth::enabledChanged, device, [device, eqivaDevice](){
            device->setStateValue(eqivaBluetoothPowerStateTypeId, eqivaDevice->enabled());
        });
        // Boost state
        device->setStateValue(eqivaBluetoothBoostStateTypeId, eqivaDevice->boostEnabled());
        connect(eqivaDevice, &EqivaBluetooth::boostEnabledChanged, device, [device, eqivaDevice](){
            device->setStateValue(eqivaBluetoothBoostStateTypeId, eqivaDevice->boostEnabled());
        });
        // Lock state
        device->setStateValue(eqivaBluetoothLockStateTypeId, eqivaDevice->locked());
        connect(eqivaDevice, &EqivaBluetooth::lockedChanged, device, [device, eqivaDevice](){
            device->setStateValue(eqivaBluetoothLockStateTypeId, eqivaDevice->locked());
        });
        // Mode state
        device->setStateValue(eqivaBluetoothModeStateTypeId, modeToString(eqivaDevice->mode()));
        connect(eqivaDevice, &EqivaBluetooth::modeChanged, device, [this, device, eqivaDevice](){
            device->setStateValue(eqivaBluetoothModeStateTypeId, modeToString(eqivaDevice->mode()));
        });
        // Target temp state
        device->setStateValue(eqivaBluetoothTargetTemperatureStateTypeId, eqivaDevice->targetTemperature());
        connect(eqivaDevice, &EqivaBluetooth::targetTemperatureChanged, device, [device, eqivaDevice](){
            device->setStateValue(eqivaBluetoothTargetTemperatureStateTypeId, eqivaDevice->targetTemperature());
        });
        // Window open state
        device->setStateValue(eqivaBluetoothWindowOpenStateTypeId, eqivaDevice->windowOpen());
        connect(eqivaDevice, &EqivaBluetooth::windowOpenChanged, device, [device, eqivaDevice](){
            device->setStateValue(eqivaBluetoothWindowOpenStateTypeId, eqivaDevice->windowOpen());
        });
        // Valve open state
        device->setStateValue(eqivaBluetoothValveOpenStateTypeId, eqivaDevice->valveOpen());
        connect(eqivaDevice, &EqivaBluetooth::valveOpenChanged, device, [device, eqivaDevice](){
            device->setStateValue(eqivaBluetoothValveOpenStateTypeId, eqivaDevice->valveOpen());
        });
        // Battery critical state
        device->setStateValue(eqivaBluetoothBatteryCriticalStateTypeId, eqivaDevice->batteryCritical());
        connect(eqivaDevice, &EqivaBluetooth::batteryCriticalChanged, device, [device, eqivaDevice](){
            device->setStateValue(eqivaBluetoothBatteryCriticalStateTypeId, eqivaDevice->batteryCritical());
        });
    }

    info->finish(Device::DeviceErrorNoError);
}

void DevicePluginEQ3::deviceRemoved(Device *device)
{
    if (device->deviceClassId() == cubeDeviceClassId) {
        MaxCube *cube = m_cubes.key(device);
        qCDebug(dcEQ3) << "Removing cube" << device->name() << cube->serialNumber();
        cube->disconnectFromCube();
        m_cubes.remove(cube);
        cube->deleteLater();
    }

    if (device->deviceClassId() == eqivaBluetoothDeviceClassId) {
        qCDebug(dcEQ3) << "Removing Eqiva device" << device->name();
        m_eqivaDevices.take(device)->deleteLater();
    }

}

QString DevicePluginEQ3::modeToString(EqivaBluetooth::Mode mode)
{
    switch (mode) {
    case EqivaBluetooth::ModeAuto:
        return "Auto";
    case EqivaBluetooth::ModeManual:
        return "Manual";
    case EqivaBluetooth::ModeHoliday:
        return "Holiday";
    }
    Q_ASSERT_X(false, "ModeToString", "Unhandled mode");
    return QString();
}

EqivaBluetooth::Mode DevicePluginEQ3::stringToMode(const QString &string)
{
    if (string == "Holiday") {
        return EqivaBluetooth::ModeHoliday;
    }
    if (string == "Manual") {
        return EqivaBluetooth::ModeManual;
    }
    if (string == "Auto") {
        return EqivaBluetooth::ModeAuto;
    }
    Q_ASSERT_X(false, "StringToMode", "Unhandled string:" + string.toUtf8());
    return  EqivaBluetooth::ModeAuto;
}

void DevicePluginEQ3::executeAction(DeviceActionInfo *info)
{
    Device *device = info->device();
    Action action = info->action();

    if(device->deviceClassId() == wallThermostateDeviceClassId){

        MaxCube *cube = m_cubes.key(device);
        QByteArray rfAddress = device->paramValue(wallThermostateDeviceRfParamTypeId).toByteArray();
        int roomId = device->paramValue(wallThermostateDeviceRoomParamTypeId).toInt();

        // FIXME: The MaxCube class needs a reworkto queue commands instead of overwriting each other's actionId

        if (action.actionTypeId() == wallThermostateTargetTemperatureActionTypeId){
            cube->setDeviceSetpointTemp(rfAddress, roomId, action.param(wallThermostateTargetTemperatureActionTargetTemperatureParamTypeId).value().toDouble(), action.id());
        } else if (action.actionTypeId() == wallThermostateSetAutoModeActionTypeId){
            cube->setDeviceAutoMode(rfAddress, roomId, action.id());
        } else if (action.actionTypeId() == wallThermostateSetManualModeActionTypeId){
            cube->setDeviceManuelMode(rfAddress, roomId, action.id());
        } else if (action.actionTypeId() == wallThermostateSetEcoModeActionTypeId){
            cube->setDeviceEcoMode(rfAddress, roomId, action.id());
        } else if (action.actionTypeId() == wallThermostateDisplayCurrentTempActionTypeId){
            cube->displayCurrentTemperature(rfAddress, roomId, action.param(wallThermostateDisplayCurrentTempActionDisplayParamTypeId).value().toBool(), action.id());
        }

        // Connect this info object to the next coming commandActionFinished
        connect(cube, &MaxCube::commandActionFinished, info, [info](bool success, const ActionId &){
            info->finish(success ? Device::DeviceErrorNoError : Device::DeviceErrorHardwareFailure);
        });
        return;
    }

    if (device->deviceClassId() == radiatorThermostateDeviceClassId){
        MaxCube *cube = m_cubes.key(device);
        QByteArray rfAddress = device->paramValue(radiatorThermostateDeviceRfParamTypeId).toByteArray();
        int roomId = device->paramValue(radiatorThermostateDeviceRoomParamTypeId).toInt();

        if (action.actionTypeId() == radiatorThermostateDesiredTemperatureActionTypeId){
            cube->setDeviceSetpointTemp(rfAddress, roomId, action.param(radiatorThermostateDesiredTemperatureActionDesiredTemperatureParamTypeId).value().toDouble(), action.id());
        } else if (action.actionTypeId() == radiatorThermostateSetAutoModeActionTypeId){
            cube->setDeviceAutoMode(rfAddress, roomId, action.id());
        } else if (action.actionTypeId() == radiatorThermostateSetManualModeActionTypeId){
            cube->setDeviceManuelMode(rfAddress, roomId, action.id());
        } else if (action.actionTypeId() == radiatorThermostateSetEcoModeActionTypeId){
            cube->setDeviceEcoMode(rfAddress, roomId, action.id());
        }

        // Connect this info object to the next coming commandActionFinished
        connect(cube, &MaxCube::commandActionFinished, info, [info](bool success, const ActionId &){
            info->finish(success ? Device::DeviceErrorNoError : Device::DeviceErrorHardwareFailure);
        });

        return;
    }

    if (device->deviceClassId() == eqivaBluetoothDeviceClassId) {
        int commandId;
        EqivaBluetooth *eqivaDevice = m_eqivaDevices.value(device);

        if (action.actionTypeId() == eqivaBluetoothPowerActionTypeId) {
            commandId = eqivaDevice->setEnabled(action.param(eqivaBluetoothPowerActionPowerParamTypeId).value().toBool());
        } else if (action.actionTypeId() == eqivaBluetoothTargetTemperatureActionTypeId) {
            commandId = eqivaDevice->setTargetTemperature(action.param(eqivaBluetoothTargetTemperatureActionTargetTemperatureParamTypeId).value().toReal());
        } else if (action.actionTypeId() == eqivaBluetoothLockActionTypeId) {
            commandId = eqivaDevice->setLocked(action.param(eqivaBluetoothLockActionLockParamTypeId).value().toBool());
        } else if (action.actionTypeId() == eqivaBluetoothModeActionTypeId) {
            commandId = eqivaDevice->setMode(stringToMode(action.param(eqivaBluetoothModeActionModeParamTypeId).value().toString()));
        } else if (action.actionTypeId() == eqivaBluetoothBoostActionTypeId) {
            commandId = eqivaDevice->setBoostEnabled(action.param(eqivaBluetoothBoostActionBoostParamTypeId).value().toBool());
        } else {
            Q_ASSERT_X(false, "DevicePluginEQ3", "An action type has not been handled!");
            qCWarning(dcEQ3()) << "An action type has not been handled!";
            info->finish(Device::DeviceErrorActionTypeNotFound);
            return;
        }

        connect(eqivaDevice, &EqivaBluetooth::commandResult, info, [info, commandId](int commandIdResult, bool success){
            if (!success) {
                qCWarning(dcEQ3()) << "Error writing characteristic";
            }
            if (commandId == commandIdResult) {
                info->finish(success ? Device::DeviceErrorNoError : Device::DeviceErrorHardwareFailure);
            }
        });
        return;
    }
}

void DevicePluginEQ3::onPluginTimer()
{
    foreach (MaxCube *cube, m_cubes.keys()){
        if(cube->isConnected() && cube->isInitialized()){
            cube->refresh();
        }
    }
}

void DevicePluginEQ3::cubeConnectionStatusChanged(const bool &connected)
{
    if (connected) {
        MaxCube *cube = static_cast<MaxCube*>(sender());
        Device *device;
        if (m_cubes.contains(cube)) {
            device = m_cubes.value(cube);
            device->setName("Max! Cube " + cube->serialNumber());
            device->setStateValue(cubeConnectedStateTypeId,true);
        }
    } else {
        MaxCube *cube = static_cast<MaxCube*>(sender());
        Device *device;
        if (m_cubes.contains(cube)){
            device = m_cubes.value(cube);
            device->setStateValue(cubeConnectedStateTypeId,false);
        }
    }
}

void DevicePluginEQ3::wallThermostatFound()
{
    MaxCube *cube = static_cast<MaxCube*>(sender());

    QList<DeviceDescriptor> descriptorList;

    foreach (WallThermostat *wallThermostat, cube->wallThermostatList()) {
        bool allreadyAdded = false;
        foreach (Device *device, myDevices().filterByDeviceClassId(wallThermostateDeviceClassId)){
            if(wallThermostat->serialNumber() == device->paramValue(wallThermostateDeviceSerialParamTypeId).toString()){
                allreadyAdded = true;
                break;
            }
        }
        if(!allreadyAdded){
            DeviceDescriptor descriptor(wallThermostateDeviceClassId, wallThermostat->serialNumber());
            ParamList params;
            params.append(Param(wallThermostateDeviceNameParamTypeId, wallThermostat->deviceName()));
            params.append(Param(wallThermostateDeviceParentParamTypeId, cube->serialNumber()));
            params.append(Param(wallThermostateDeviceSerialParamTypeId, wallThermostat->serialNumber()));
            params.append(Param(wallThermostateDeviceRfParamTypeId, wallThermostat->rfAddress()));
            params.append(Param(wallThermostateDeviceRoomParamTypeId, wallThermostat->roomId()));
            params.append(Param(wallThermostateDeviceRoomNameParamTypeId, wallThermostat->roomName()));
            descriptor.setParams(params);
            descriptorList.append(descriptor);
        }
    }

    if(!descriptorList.isEmpty()){
        emit autoDevicesAppeared(descriptorList);
    }
}

void DevicePluginEQ3::radiatorThermostatFound()
{
    MaxCube *cube = static_cast<MaxCube*>(sender());

    QList<DeviceDescriptor> descriptorList;

    foreach (RadiatorThermostat *radiatorThermostat, cube->radiatorThermostatList()) {
        bool allreadyAdded = false;
        foreach (Device *device, myDevices().filterByDeviceClassId(radiatorThermostateDeviceClassId)){
            if(radiatorThermostat->serialNumber() == device->paramValue(radiatorThermostateDeviceSerialParamTypeId).toString()){
                allreadyAdded = true;
                break;
            }
        }
        if(!allreadyAdded){
            DeviceDescriptor descriptor(radiatorThermostateDeviceClassId, radiatorThermostat->serialNumber());
            ParamList params;
            params.append(Param(radiatorThermostateDeviceNameParamTypeId, radiatorThermostat->deviceName()));
            params.append(Param(radiatorThermostateDeviceParentParamTypeId, cube->serialNumber()));
            params.append(Param(radiatorThermostateDeviceSerialParamTypeId, radiatorThermostat->serialNumber()));
            params.append(Param(radiatorThermostateDeviceRfParamTypeId, radiatorThermostat->rfAddress()));
            params.append(Param(radiatorThermostateDeviceRoomParamTypeId, radiatorThermostat->roomId()));
            params.append(Param(radiatorThermostateDeviceRoomNameParamTypeId, radiatorThermostat->roomName()));
            descriptor.setParams(params);
            descriptorList.append(descriptor);
        }
    }

    if(!descriptorList.isEmpty()){
        emit autoDevicesAppeared(descriptorList);
    }
}

void DevicePluginEQ3::updateCubeConfig()
{
    MaxCube *cube = static_cast<MaxCube*>(sender());
    Device *device;
    if (m_cubes.contains(cube)) {
        device = m_cubes.value(cube);
        device->setStateValue(cubePortalEnabledStateTypeId,cube->portalEnabeld());
        return;
    }
}

void DevicePluginEQ3::wallThermostatDataUpdated()
{
    MaxCube *cube = static_cast<MaxCube*>(sender());

    foreach (WallThermostat *wallThermostat, cube->wallThermostatList()) {
        foreach (Device *device, myDevices().filterByDeviceClassId(wallThermostateDeviceClassId)){
            if(device->paramValue(wallThermostateDeviceSerialParamTypeId).toString() == wallThermostat->serialNumber()){
                device->setStateValue(wallThermostateComfortTemperatureStateTypeId, wallThermostat->comfortTemp());
                device->setStateValue(wallThermostateEcoTempStateTypeId, wallThermostat->ecoTemp());
                device->setStateValue(wallThermostateMaxSetpointTempStateTypeId, wallThermostat->maxSetPointTemp());
                device->setStateValue(wallThermostateMinSetpointTempStateTypeId, wallThermostat->minSetPointTemp());
                device->setStateValue(wallThermostateErrorOccurredStateTypeId, wallThermostat->errorOccurred());
                device->setStateValue(wallThermostateInitializedStateTypeId, wallThermostat->initialized());
                device->setStateValue(wallThermostateBatteryCriticalStateTypeId, wallThermostat->batteryLow());
                device->setStateValue(wallThermostateLinkStatusOKStateTypeId, wallThermostat->linkStatusOK());
                device->setStateValue(wallThermostatePanelLockedStateTypeId, wallThermostat->panelLocked());
                device->setStateValue(wallThermostateGatewayKnownStateTypeId, wallThermostat->gatewayKnown());
                device->setStateValue(wallThermostateDtsActiveStateTypeId, wallThermostat->dtsActive());
                device->setStateValue(wallThermostateDeviceModeStateTypeId, wallThermostat->deviceMode());
                device->setStateValue(wallThermostateDeviceModeStringStateTypeId, wallThermostat->deviceModeString());
                device->setStateValue(wallThermostateTargetTemperatureStateTypeId, wallThermostat->setpointTemperature());
                device->setStateValue(wallThermostateTemperatureStateTypeId, wallThermostat->currentTemperature());
            }
        }
    }
}

void DevicePluginEQ3::radiatorThermostatDataUpdated()
{
    MaxCube *cube = static_cast<MaxCube*>(sender());

    foreach (RadiatorThermostat *radiatorThermostat, cube->radiatorThermostatList()) {
        foreach (Device *device, myDevices().filterByDeviceClassId(radiatorThermostateDeviceClassId)){
            if(device->paramValue(radiatorThermostateDeviceSerialParamTypeId).toString() == radiatorThermostat->serialNumber()){
                device->setStateValue(radiatorThermostateComfortTempStateTypeId, radiatorThermostat->comfortTemp());
                device->setStateValue(radiatorThermostateEcoTempStateTypeId, radiatorThermostat->ecoTemp());
                device->setStateValue(radiatorThermostateMaxSetpointTempStateTypeId, radiatorThermostat->maxSetPointTemp());
                device->setStateValue(radiatorThermostateMinSetpointTempStateTypeId, radiatorThermostat->minSetPointTemp());
                device->setStateValue(radiatorThermostateErrorOccurredStateTypeId, radiatorThermostat->errorOccurred());
                device->setStateValue(radiatorThermostateInitializedStateTypeId, radiatorThermostat->initialized());
                device->setStateValue(radiatorThermostateBatteryLowStateTypeId, radiatorThermostat->batteryLow());
                device->setStateValue(radiatorThermostatePanelLockedStateTypeId, radiatorThermostat->panelLocked());
                device->setStateValue(radiatorThermostateGatewayKnownStateTypeId, radiatorThermostat->gatewayKnown());
                device->setStateValue(radiatorThermostateDtsActiveStateTypeId, radiatorThermostat->dtsActive());
                device->setStateValue(radiatorThermostateDeviceModeStateTypeId, radiatorThermostat->deviceMode());
                device->setStateValue(radiatorThermostateDeviceModeStringStateTypeId, radiatorThermostat->deviceModeString());
                device->setStateValue(radiatorThermostateDesiredTemperatureStateTypeId, radiatorThermostat->setpointTemperature());
                device->setStateValue(radiatorThermostateOffsetTempStateTypeId, radiatorThermostat->offsetTemp());
                device->setStateValue(radiatorThermostateWindowOpenDurationStateTypeId, radiatorThermostat->windowOpenDuration());
                device->setStateValue(radiatorThermostateBoostValveValueStateTypeId, radiatorThermostat->boostValveValue());
                device->setStateValue(radiatorThermostateBoostDurationStateTypeId, radiatorThermostat->boostDuration());
                device->setStateValue(radiatorThermostateDiscalcWeekDayStateTypeId, radiatorThermostat->discalcingWeekDay());
                device->setStateValue(radiatorThermostateDiscalcTimeStateTypeId, radiatorThermostat->discalcingTime().toString("HH:mm"));
                device->setStateValue(radiatorThermostateValveMaximumSettingsStateTypeId, radiatorThermostat->valveMaximumSettings());
                device->setStateValue(radiatorThermostateValveOffsetStateTypeId, radiatorThermostat->valveOffset());
                device->setStateValue(radiatorThermostateValvePositionStateTypeId, radiatorThermostat->valvePosition());
            }
        }
    }
}
