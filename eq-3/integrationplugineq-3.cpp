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

#include "integrationplugineq-3.h"

#include "integrations/thing.h"
#include "types/param.h"
#include "plugininfo.h"

#include "eqivabluetooth.h"

#include <QDebug>

IntegrationPluginEQ3::IntegrationPluginEQ3()
{

}

IntegrationPluginEQ3::~IntegrationPluginEQ3()
{
    hardwareManager()->pluginTimerManager()->unregisterTimer(m_pluginTimer);
}

void IntegrationPluginEQ3::init()
{
    qCDebug(dcEQ3()) << "Initializing EQ-3 Plugin";

    m_pluginTimer = hardwareManager()->pluginTimerManager()->registerTimer(10);
    connect(m_pluginTimer, &PluginTimer::timeout, this, &IntegrationPluginEQ3::onPluginTimer);
}

void IntegrationPluginEQ3::discoverThings(ThingDiscoveryInfo *info)
{
    ThingClassId deviceClassId = info->thingClassId();
    qCDebug(dcEQ3()) << "Discover devices called";
    if(deviceClassId == cubeThingClassId){

        MaxCubeDiscovery *cubeDiscovery = new MaxCubeDiscovery(this);

        connect(info, &QObject::destroyed, cubeDiscovery, &MaxCubeDiscovery::deleteLater);

        connect(cubeDiscovery, &MaxCubeDiscovery::cubesDetected, info, [this, info](const QList<MaxCubeDiscovery::CubeInfo> &cubeList){

            foreach (const MaxCubeDiscovery::CubeInfo &cube, cubeList) {
                ThingDescriptor descriptor(cubeThingClassId, "Max! Cube LAN Gateway", cube.serialNumber);
                ParamList params;
                params << Param(cubeThingHostParamTypeId, cube.hostAddress.toString());
                params << Param(cubeThingPortParamTypeId, cube.port);
                params << Param(cubeThingFirmwareParamTypeId, cube.firmware);
                params << Param(cubeThingSerialParamTypeId, cube.serialNumber);

                foreach (Thing *existingThing, myThings()) {
                    if (existingThing->paramValue(cubeThingSerialParamTypeId).toString() == cube.serialNumber) {
                        descriptor.setThingId(existingThing->id());
                        break;
                    }
                }

                descriptor.setParams(params);
                info->addThingDescriptor(descriptor);
            }

            info->finish(Thing::ThingErrorNoError);
        });

        cubeDiscovery->detectCubes();
        return;
    }
    if (deviceClassId == eqivaBluetoothThingClassId) {
        EqivaBluetoothDiscovery *eqivaBluetoothDiscovery = new EqivaBluetoothDiscovery(hardwareManager()->bluetoothLowEnergyManager(), this);

        // Clean up the discovery when the DiscoveryInfo goes away...
        connect(info, &QObject::destroyed, eqivaBluetoothDiscovery, &EqivaBluetoothDiscovery::deleteLater);

        // Discovery result handler
        connect(eqivaBluetoothDiscovery, &EqivaBluetoothDiscovery::finished, info, [this, info](const QStringList results){
            qCDebug(dcEQ3()) << "Discovery finished";

            foreach (const QString &result, results) {
                qCDebug(dcEQ3()) << "Discovered EQ-3 device" << result;
                ThingDescriptor descriptor(eqivaBluetoothThingClassId, "Eqiva Bluetooth Thermostat", result);
                ParamList params;
                params << Param(eqivaBluetoothThingMacAddressParamTypeId, result);
                descriptor.setParams(params);
                foreach (Thing* existingThing, myThings()) {
                    if (existingThing->paramValue(eqivaBluetoothThingMacAddressParamTypeId).toString() == result) {
                        descriptor.setThingId(existingThing->id());
                        break;
                    }
                }
                info->addThingDescriptor(descriptor);
            }

            info->finish(Thing::ThingErrorNoError);
        });

        // start the discovery
        bool ret = eqivaBluetoothDiscovery->startDiscovery();
        if (!ret) {
            return info->finish(Thing::ThingErrorHardwareNotAvailable, QT_TR_NOOP("Bluetooth discovery failed. Is Bluetooth available and enabled?"));
        }
        return;
    }

    info->finish(Thing::ThingErrorThingClassNotFound);
}


void IntegrationPluginEQ3::setupThing(ThingSetupInfo *info)
{
    Thing *thing = info->thing();

    qCDebug(dcEQ3) << "Setup thing" << thing->params();

    if(thing->thingClassId() == cubeThingClassId){
        MaxCube *cube = new MaxCube(this,thing->paramValue(cubeThingSerialParamTypeId).toString(),QHostAddress(thing->paramValue(cubeThingHostParamTypeId).toString()),thing->paramValue(cubeThingPortParamTypeId).toInt());
        m_cubes.insert(cube,thing);

        connect(cube, &MaxCube::cubeConnectionStatusChanged, this, &IntegrationPluginEQ3::cubeConnectionStatusChanged);
        connect(cube, &MaxCube::cubeConfigReady, this, &IntegrationPluginEQ3::updateCubeConfig);
        connect(cube, &MaxCube::wallThermostatFound, this, &IntegrationPluginEQ3::wallThermostatFound);
        connect(cube, &MaxCube::wallThermostatDataUpdated, this, &IntegrationPluginEQ3::wallThermostatDataUpdated);
        connect(cube, &MaxCube::radiatorThermostatFound, this, &IntegrationPluginEQ3::radiatorThermostatFound);
        connect(cube, &MaxCube::radiatorThermostatDataUpdated, this, &IntegrationPluginEQ3::radiatorThermostatDataUpdated);

        cube->connectToCube();
        connect(cube, &MaxCube::cubeConnectionStatusChanged, info, [info](bool connected){
            if (connected) {
                info->finish(Thing::ThingErrorNoError);
            } else {
                info->finish(Thing::ThingErrorHardwareFailure);
            }
        });
        return;
    }

    if (thing->thingClassId() == eqivaBluetoothThingClassId) {
        EqivaBluetooth *eqivaDevice = new EqivaBluetooth(hardwareManager()->bluetoothLowEnergyManager(), QBluetoothAddress(thing->paramValue(eqivaBluetoothThingMacAddressParamTypeId).toString()), thing->name(), this);
        m_eqivaDevices.insert(thing, eqivaDevice);

        connect(thing, &Thing::nameChanged, eqivaDevice, [thing, eqivaDevice](){
            eqivaDevice->setName(thing->name());
        });

        // Connected state
        thing->setStateValue(eqivaBluetoothConnectedStateTypeId, eqivaDevice->available());
        connect(eqivaDevice, &EqivaBluetooth::availableChanged, thing, [thing, eqivaDevice](){
            thing->setStateValue(eqivaBluetoothConnectedStateTypeId, eqivaDevice->available());
        });
        // Power state
        thing->setStateValue(eqivaBluetoothHeatingOnStateTypeId, eqivaDevice->valveOpen() > 0);
        connect(eqivaDevice, &EqivaBluetooth::valveOpenChanged, thing, [thing, eqivaDevice](){
            thing->setStateValue(eqivaBluetoothHeatingOnStateTypeId, eqivaDevice->valveOpen() > 0);
        });
        // Boost state
        thing->setStateValue(eqivaBluetoothBoostStateTypeId, eqivaDevice->boostEnabled());
        connect(eqivaDevice, &EqivaBluetooth::boostEnabledChanged, thing, [thing, eqivaDevice](){
            thing->setStateValue(eqivaBluetoothBoostStateTypeId, eqivaDevice->boostEnabled());
        });
        // Lock state
        thing->setStateValue(eqivaBluetoothLockStateTypeId, eqivaDevice->locked());
        connect(eqivaDevice, &EqivaBluetooth::lockedChanged, thing, [thing, eqivaDevice](){
            thing->setStateValue(eqivaBluetoothLockStateTypeId, eqivaDevice->locked());
        });
        // Mode state
        thing->setStateValue(eqivaBluetoothModeStateTypeId, modeToString(eqivaDevice->mode()));
        connect(eqivaDevice, &EqivaBluetooth::modeChanged, thing, [this, thing, eqivaDevice](){
            thing->setStateValue(eqivaBluetoothModeStateTypeId, modeToString(eqivaDevice->mode()));
        });
        // Target temp state
        thing->setStateValue(eqivaBluetoothTargetTemperatureStateTypeId, eqivaDevice->targetTemperature());
        connect(eqivaDevice, &EqivaBluetooth::targetTemperatureChanged, thing, [thing, eqivaDevice](){
            thing->setStateValue(eqivaBluetoothTargetTemperatureStateTypeId, eqivaDevice->targetTemperature());
        });
        // Window open state
        thing->setStateValue(eqivaBluetoothWindowOpenDetectedStateTypeId, eqivaDevice->windowOpen());
        connect(eqivaDevice, &EqivaBluetooth::windowOpenChanged, thing, [thing, eqivaDevice](){
            thing->setStateValue(eqivaBluetoothWindowOpenDetectedStateTypeId, eqivaDevice->windowOpen());
        });
        // Valve open state
        thing->setStateValue(eqivaBluetoothValveOpenStateTypeId, eqivaDevice->valveOpen());
        connect(eqivaDevice, &EqivaBluetooth::valveOpenChanged, thing, [thing, eqivaDevice](){
            thing->setStateValue(eqivaBluetoothValveOpenStateTypeId, eqivaDevice->valveOpen());
        });
        // Battery critical state
        thing->setStateValue(eqivaBluetoothBatteryCriticalStateTypeId, eqivaDevice->batteryCritical());
        connect(eqivaDevice, &EqivaBluetooth::batteryCriticalChanged, thing, [thing, eqivaDevice](){
            thing->setStateValue(eqivaBluetoothBatteryCriticalStateTypeId, eqivaDevice->batteryCritical());
        });
    }

    info->finish(Thing::ThingErrorNoError);
}

void IntegrationPluginEQ3::thingRemoved(Thing *thing)
{
    if (thing->thingClassId() == cubeThingClassId) {
        MaxCube *cube = m_cubes.key(thing);
        qCDebug(dcEQ3) << "Removing cube" << thing->name() << cube->serialNumber();
        cube->disconnectFromCube();
        m_cubes.remove(cube);
        cube->deleteLater();
    }

    if (thing->thingClassId() == eqivaBluetoothThingClassId) {
        qCDebug(dcEQ3) << "Removing Eqiva device" << thing->name();
        m_eqivaDevices.take(thing)->deleteLater();
    }

}

QString IntegrationPluginEQ3::modeToString(EqivaBluetooth::Mode mode)
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

EqivaBluetooth::Mode IntegrationPluginEQ3::stringToMode(const QString &string)
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

void IntegrationPluginEQ3::executeAction(ThingActionInfo *info)
{
    Action action = info->action();

    if(info->thing()->thingClassId() == wallThermostatThingClassId){
        Thing *thing = myThings().findById(info->thing()->parentId());
        MaxCube *cube = m_cubes.key(thing);
        QByteArray rfAddress = thing->paramValue(wallThermostatThingRfParamTypeId).toByteArray();
        int roomId = thing->paramValue(wallThermostatThingRoomParamTypeId).toInt();

        // FIXME: The MaxCube class needs a reworkto queue commands instead of overwriting each other's actionId

        int id = -1;
        if (action.actionTypeId() == wallThermostatTargetTemperatureActionTypeId){
            id = cube->setDeviceSetpointTemp(rfAddress, roomId, action.param(wallThermostatTargetTemperatureActionTargetTemperatureParamTypeId).value().toDouble());
        } else if (action.actionTypeId() == wallThermostatPowerActionTypeId) {
            id = cube->setDeviceSetpointTemp(rfAddress, roomId, action.param(wallThermostatPowerActionPowerParamTypeId).value().toBool() ? thing->stateValue(wallThermostatComfortTemperatureStateTypeId).toDouble() : 4.5);
        } else if (action.actionTypeId() == wallThermostatSetAutoModeActionTypeId){
            id = cube->setDeviceAutoMode(rfAddress, roomId);
        } else if (action.actionTypeId() == wallThermostatSetManualModeActionTypeId){
            id = cube->setDeviceManuelMode(rfAddress, roomId);
        } else if (action.actionTypeId() == wallThermostatSetEcoModeActionTypeId){
            id = cube->setDeviceEcoMode(rfAddress, roomId);
        } else if (action.actionTypeId() == wallThermostatDisplayCurrentTempActionTypeId){
            id = cube->displayCurrentTemperature(rfAddress, roomId, action.param(wallThermostatDisplayCurrentTempActionDisplayParamTypeId).value().toBool());
        }
        if (id == -1) {
            info->finish(Thing::ThingErrorHardwareFailure);
            return;
        }

        // Connect this info object to the next coming commandActionFinished
        connect(cube, &MaxCube::commandActionFinished, info, [info, id](bool success, int replyId){
            if (replyId == id) {
                info->finish(success ? Thing::ThingErrorNoError : Thing::ThingErrorHardwareFailure);
            }
        });
        return;
    }

    if (info->thing()->thingClassId() == radiatorThermostatThingClassId){
        Thing *thing = myThings().findById(info->thing()->parentId());
        MaxCube *cube = m_cubes.key(thing);
        QByteArray rfAddress = thing->paramValue(radiatorThermostatThingRfParamTypeId).toByteArray();
        int roomId = thing->paramValue(radiatorThermostatThingRoomParamTypeId).toInt();

        int id = -1;
        if (action.actionTypeId() == radiatorThermostatTargetTemperatureActionTypeId){
            id = cube->setDeviceSetpointTemp(rfAddress, roomId, action.param(radiatorThermostatTargetTemperatureActionTargetTemperatureParamTypeId).value().toDouble());
        } else if (action.actionTypeId() == radiatorThermostatPowerActionTypeId) {
            id = cube->setDeviceSetpointTemp(rfAddress, roomId, action.param(radiatorThermostatPowerActionPowerParamTypeId).value().toBool() ? thing->stateValue(radiatorThermostatComfortTempStateTypeId).toDouble() : 4.5);
        } else if (action.actionTypeId() == radiatorThermostatSetAutoModeActionTypeId){
            id = cube->setDeviceAutoMode(rfAddress, roomId);
        } else if (action.actionTypeId() == radiatorThermostatSetManualModeActionTypeId){
            id = cube->setDeviceManuelMode(rfAddress, roomId);
        } else if (action.actionTypeId() == radiatorThermostatSetEcoModeActionTypeId){
            id = cube->setDeviceEcoMode(rfAddress, roomId);
        }
        if (id == -1) {
            info->finish(Thing::ThingErrorHardwareFailure);
            return;
        }

        // Connect this info object to the next coming commandActionFinished
        connect(cube, &MaxCube::commandActionFinished, info, [info, id](bool success, int replyId){
            if (replyId == id) {
                info->finish(success ? Thing::ThingErrorNoError : Thing::ThingErrorHardwareFailure);
            }
        });

        return;
    }

    if (info->thing()->thingClassId() == eqivaBluetoothThingClassId) {
        Thing *thing = info->thing();
        int commandId;
        EqivaBluetooth *eqivaDevice = m_eqivaDevices.value(thing);

        if (action.actionTypeId() == eqivaBluetoothTargetTemperatureActionTypeId) {
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
            info->finish(Thing::ThingErrorActionTypeNotFound);
            return;
        }

        connect(eqivaDevice, &EqivaBluetooth::commandResult, info, [info, commandId](int commandIdResult, bool success){
            if (!success) {
                qCWarning(dcEQ3()) << "Error writing characteristic";
            }
            if (commandId == commandIdResult) {
                info->finish(success ? Thing::ThingErrorNoError : Thing::ThingErrorHardwareFailure);
            }
        });
        return;
    }
}

void IntegrationPluginEQ3::onPluginTimer()
{
    foreach (MaxCube *cube, m_cubes.keys()){
        if(cube->isConnected() && cube->isInitialized()){
            cube->refresh();
        }
    }
}

void IntegrationPluginEQ3::cubeConnectionStatusChanged(const bool &connected)
{
    if (connected) {
        MaxCube *cube = static_cast<MaxCube*>(sender());
        Thing *thing;
        if (m_cubes.contains(cube)) {
            thing = m_cubes.value(cube);
            thing->setName("Max! Cube " + cube->serialNumber());
            thing->setStateValue(cubeConnectedStateTypeId,true);
        }
    } else {
        MaxCube *cube = static_cast<MaxCube*>(sender());
        Thing *thing;
        if (m_cubes.contains(cube)){
            thing = m_cubes.value(cube);
            thing->setStateValue(cubeConnectedStateTypeId,false);
        }
    }
}

void IntegrationPluginEQ3::wallThermostatFound()
{
    MaxCube *cube = static_cast<MaxCube*>(sender());

    QList<ThingDescriptor> descriptorList;

    foreach (WallThermostat *wallThermostat, cube->wallThermostatList()) {
        bool alreadyAdded = false;
        foreach (Thing *thing, myThings().filterByThingClassId(wallThermostatThingClassId)){
            if(wallThermostat->serialNumber() == thing->paramValue(wallThermostatThingSerialParamTypeId).toString()){
                alreadyAdded = true;
                break;
            }
        }
        if(!alreadyAdded){
            ThingDescriptor descriptor(wallThermostatThingClassId, wallThermostat->serialNumber());
            descriptor.setParentId(m_cubes.value(cube)->id());
            ParamList params;
            params.append(Param(wallThermostatThingSerialParamTypeId, wallThermostat->serialNumber()));
            params.append(Param(wallThermostatThingRfParamTypeId, wallThermostat->rfAddress()));
            params.append(Param(wallThermostatThingRoomParamTypeId, wallThermostat->roomId()));
            params.append(Param(wallThermostatThingRoomNameParamTypeId, wallThermostat->roomName()));
            descriptor.setParams(params);
            descriptorList.append(descriptor);
        }
    }

    if(!descriptorList.isEmpty()){
        emit autoThingsAppeared(descriptorList);
    }
}

void IntegrationPluginEQ3::radiatorThermostatFound()
{
    MaxCube *cube = static_cast<MaxCube*>(sender());

    QList<ThingDescriptor> descriptorList;

    foreach (RadiatorThermostat *radiatorThermostat, cube->radiatorThermostatList()) {
        bool alreadyAdded = false;
        foreach (Thing *thing, myThings().filterByThingClassId(radiatorThermostatThingClassId)){
            if(radiatorThermostat->serialNumber() == thing->paramValue(radiatorThermostatThingSerialParamTypeId).toString()){
                alreadyAdded = true;
                break;
            }
        }
        if(!alreadyAdded){
            ThingDescriptor descriptor(radiatorThermostatThingClassId, "Max! Radiator Thermostat (" + radiatorThermostat->serialNumber() + ")");
            descriptor.setParentId(m_cubes.value(cube)->id());
            ParamList params;
            params.append(Param(radiatorThermostatThingSerialParamTypeId, radiatorThermostat->serialNumber()));
            params.append(Param(radiatorThermostatThingRfParamTypeId, radiatorThermostat->rfAddress()));
            params.append(Param(radiatorThermostatThingRoomParamTypeId, radiatorThermostat->roomId()));
            params.append(Param(radiatorThermostatThingRoomNameParamTypeId, radiatorThermostat->roomName()));
            descriptor.setParams(params);
            descriptorList.append(descriptor);
        }
    }

    if(!descriptorList.isEmpty()){
        emit autoThingsAppeared(descriptorList);
    }
}

void IntegrationPluginEQ3::updateCubeConfig()
{
    MaxCube *cube = static_cast<MaxCube*>(sender());
    Thing *thing;
    if (m_cubes.contains(cube)) {
        thing = m_cubes.value(cube);
        thing->setStateValue(cubePortalEnabledStateTypeId,cube->portalEnabeld());
        return;
    }
}

void IntegrationPluginEQ3::wallThermostatDataUpdated()
{
    MaxCube *cube = static_cast<MaxCube*>(sender());

    foreach (WallThermostat *wallThermostat, cube->wallThermostatList()) {
        foreach (Thing *thing, myThings().filterByThingClassId(wallThermostatThingClassId)){
            if(thing->paramValue(wallThermostatThingSerialParamTypeId).toString() == wallThermostat->serialNumber()){
                thing->setStateValue(wallThermostatConnectedStateTypeId, wallThermostat->initialized() && wallThermostat->linkStatusOK());
                thing->setStateValue(wallThermostatComfortTemperatureStateTypeId, wallThermostat->comfortTemp());
                thing->setStateValue(wallThermostatEcoTempStateTypeId, wallThermostat->ecoTemp());
                thing->setStateValue(wallThermostatMaxSetpointTempStateTypeId, wallThermostat->maxSetPointTemp());
                thing->setStateValue(wallThermostatMinSetpointTempStateTypeId, wallThermostat->minSetPointTemp());
                thing->setStateValue(wallThermostatErrorOccurredStateTypeId, wallThermostat->errorOccurred());
                thing->setStateValue(wallThermostatInitializedStateTypeId, wallThermostat->initialized());
                thing->setStateValue(wallThermostatBatteryCriticalStateTypeId, wallThermostat->batteryLow());
                thing->setStateValue(wallThermostatPanelLockedStateTypeId, wallThermostat->panelLocked());
                thing->setStateValue(wallThermostatGatewayKnownStateTypeId, wallThermostat->gatewayKnown());
                thing->setStateValue(wallThermostatDtsActiveStateTypeId, wallThermostat->dtsActive());
                thing->setStateValue(wallThermostatDeviceModeStateTypeId, wallThermostat->deviceMode());
                thing->setStateValue(wallThermostatDeviceModeStringStateTypeId, wallThermostat->deviceModeString());
                thing->setStateValue(wallThermostatTargetTemperatureStateTypeId, wallThermostat->setpointTemperature());
                thing->setStateValue(wallThermostatPowerStateTypeId, wallThermostat->setpointTemperature() > 4.5);
                thing->setStateValue(wallThermostatTemperatureStateTypeId, wallThermostat->currentTemperature());
            }
        }
    }
}

void IntegrationPluginEQ3::radiatorThermostatDataUpdated()
{
    MaxCube *cube = static_cast<MaxCube*>(sender());

    foreach (RadiatorThermostat *radiatorThermostat, cube->radiatorThermostatList()) {
        foreach (Thing *thing, myThings().filterByThingClassId(radiatorThermostatThingClassId)){
            if(thing->paramValue(radiatorThermostatThingSerialParamTypeId).toString() == radiatorThermostat->serialNumber()){
                thing->setStateValue(radiatorThermostatConnectedStateTypeId, radiatorThermostat->initialized() && radiatorThermostat->linkStatusOK());
                thing->setStateValue(radiatorThermostatComfortTempStateTypeId, radiatorThermostat->comfortTemp());
                thing->setStateValue(radiatorThermostatEcoTempStateTypeId, radiatorThermostat->ecoTemp());
                thing->setStateValue(radiatorThermostatMaxSetpointTempStateTypeId, radiatorThermostat->maxSetPointTemp());
                thing->setStateValue(radiatorThermostatMinSetpointTempStateTypeId, radiatorThermostat->minSetPointTemp());
                thing->setStateValue(radiatorThermostatErrorOccurredStateTypeId, radiatorThermostat->errorOccurred());
                thing->setStateValue(radiatorThermostatInitializedStateTypeId, radiatorThermostat->initialized());
                thing->setStateValue(radiatorThermostatBatteryCriticalStateTypeId, radiatorThermostat->batteryLow());
                thing->setStateValue(radiatorThermostatPanelLockedStateTypeId, radiatorThermostat->panelLocked());
                thing->setStateValue(radiatorThermostatGatewayKnownStateTypeId, radiatorThermostat->gatewayKnown());
                thing->setStateValue(radiatorThermostatDtsActiveStateTypeId, radiatorThermostat->dtsActive());
                thing->setStateValue(radiatorThermostatDeviceModeStateTypeId, radiatorThermostat->deviceMode());
                thing->setStateValue(radiatorThermostatDeviceModeStringStateTypeId, radiatorThermostat->deviceModeString());
                thing->setStateValue(radiatorThermostatTargetTemperatureStateTypeId, radiatorThermostat->setpointTemperature());
                thing->setStateValue(radiatorThermostatPowerStateTypeId, radiatorThermostat->setpointTemperature() > 4.5);
                thing->setStateValue(radiatorThermostatOffsetTempStateTypeId, radiatorThermostat->offsetTemp());
                thing->setStateValue(radiatorThermostatWindowOpenDurationStateTypeId, radiatorThermostat->windowOpenDuration());
                thing->setStateValue(radiatorThermostatBoostValveValueStateTypeId, radiatorThermostat->boostValveValue());
                thing->setStateValue(radiatorThermostatBoostDurationStateTypeId, radiatorThermostat->boostDuration());
                thing->setStateValue(radiatorThermostatDiscalcWeekDayStateTypeId, radiatorThermostat->discalcingWeekDay());
                thing->setStateValue(radiatorThermostatDiscalcTimeStateTypeId, radiatorThermostat->discalcingTime().toString("HH:mm"));
                thing->setStateValue(radiatorThermostatValveMaximumSettingsStateTypeId, radiatorThermostat->valveMaximumSettings());
                thing->setStateValue(radiatorThermostatValveOffsetStateTypeId, radiatorThermostat->valveOffset());
                thing->setStateValue(radiatorThermostatValvePositionStateTypeId, radiatorThermostat->valvePosition());
            }
        }
    }
}
