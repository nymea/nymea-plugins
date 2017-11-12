/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *                                                                         *
 *  Copyright (C) 2017 Bernhard Trinnes <bernhard.trinnes@guh.io>          *
 *                                                                         *
 *  This file is part of guh.                                              *
 *                                                                         *
 *  Guh is free software: you can redistribute it and/or modify            *
 *  it under the terms of the GNU General Public License as published by   *
 *  the Free Software Foundation, version 2 of the License.                *
 *                                                                         *
 *  Guh is distributed in the hope that it will be useful,                 *
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of         *
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the           *
 *  GNU General Public License for more details.                           *
 *                                                                         *
 *  You should have received a copy of the GNU General Public License      *
 *  along with guh. If not, see <http://www.gnu.org/licenses/>.            *
 *                                                                         *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

#include "devicepluginsimulation.h"
#include "plugininfo.h"


DevicePluginSimulation::DevicePluginSimulation()
{
}


DeviceManager::HardwareResources DevicePluginSimulation::requiredHardware() const
{
    return DeviceManager::HardwareResourceTimer;
}


DeviceManager::DeviceSetupStatus DevicePluginSimulation::setupDevice(Device *device)
{
    Q_UNUSED(device)
    qCDebug(dcSimulation) << device->params();

    return DeviceManager::DeviceSetupStatusSuccess;
}


DeviceManager::DeviceError DevicePluginSimulation::executeAction(Device *device, const Action &action)
{

    // Check the DeviceClassId for "Simple Button"
    if (device->deviceClassId() == simpleButtonDeviceClassId ) {

        // check if this is the "press" action
        if (action.actionTypeId() == pressSimpleButtonActionTypeId) {


            // Emit the "button pressed" event
            Event event(simpleButtonPressedEventTypeId, device->id());
            emit emitEvent(event);

            return DeviceManager::DeviceErrorNoError;
        }
        return DeviceManager::DeviceErrorActionTypeNotFound;
    }

    // Check the DeviceClassId for "Alternative Power Button"
    if (device->deviceClassId() == alternativePowerButtonDeviceClassId) {

        // check if this is the "set power" action
        if (action.actionTypeId() == alternativePowerActionTypeId) {

            // get the param value
            Param powerParam = action.param(alternativePowerStateParamTypeId);
            bool power = powerParam.value().toBool();

            qCDebug(dcSimulation) << "ActionTypeId :" << action.actionTypeId().toString();
            qCDebug(dcSimulation) << "StateTypeId  :" << alternativePowerStateTypeId.toString();

            // Set the "power" state
            device->setStateValue(alternativePowerStateTypeId, power);

            return DeviceManager::DeviceErrorNoError;
        }
        return DeviceManager::DeviceErrorActionTypeNotFound;
    }

    if (device->deviceClassId() == heatingDeviceClassId) {

        // check if this is the "set power" action
        if (action.actionTypeId() == powerActionTypeId) {

            // get the param value
            Param powerParam = action.param(powerStateParamTypeId);
            bool power = powerParam.value().toBool();
            // Set the "power" state
            device->setStateValue(powerStateTypeId, power);
            return DeviceManager::DeviceErrorNoError;

        }else if (action.actionTypeId() == targetTemperatureActionTypeId) {

            // get the param value
            Param temperatureParam = action.param(targetTemperatureStateParamTypeId);
            int temperature = temperatureParam.value().toInt();

            // Set the "temperature" state
            device->setStateValue(targetTemperatureStateTypeId, temperature);
            return DeviceManager::DeviceErrorNoError;
        }
        return DeviceManager::DeviceErrorActionTypeNotFound;
    }

    if(device->deviceClassId() == evChargerDeviceClassId){

        if(action.actionTypeId() == evPowerActionTypeId){
            // get the param value
            Param powerParam = action.param(evPowerStateParamTypeId);
            bool power = powerParam.value().toBool();
            // Set the "power" state
            device->setStateValue(evPowerStateTypeId, power);
            return DeviceManager::DeviceErrorNoError;

        }else if(action.actionTypeId() == evCurrentActionTypeId){
            // get the param value
            Param currentParam = action.param(evCurrentStateParamTypeId);
            int current = currentParam.value().toInt();
            // Set the "current" state
            device->setStateValue(evCurrentStateTypeId, current);
            return DeviceManager::DeviceErrorNoError;
        }
        return DeviceManager::DeviceErrorActionTypeNotFound;
    }

    if(device->deviceClassId() == socketDeviceClassId){

        if(action.actionTypeId() == socketPowerActionTypeId){
            // get the param value
            Param powerParam = action.param(socketPowerStateParamTypeId);
            bool power = powerParam.value().toBool();
            // Set the "power" state
            device->setStateValue(socketPowerStateTypeId, power);
            return DeviceManager::DeviceErrorNoError;
        }
        return DeviceManager::DeviceErrorActionTypeNotFound;
    }

    if(device->deviceClassId() == rgbBulbDeviceClassId){

        if(action.actionTypeId() == bulbBrightnessActionTypeId){
            int brightness = action.param(bulbBrightnessStateParamTypeId).value().toInt();
            device->setStateValue(bulbBrightnessStateTypeId, brightness);
            return DeviceManager::DeviceErrorNoError;

        } else if (action.actionTypeId() == bulbTemperatureActionTypeId){
            int temperature = action.param(bulbTemperatureStateParamTypeId).value().toInt();
            device->setStateValue(bulbTemperatureStateTypeId, temperature);
            return DeviceManager::DeviceErrorNoError;

        } else if (action.actionTypeId() == bulbColorActionTypeId) {
            QVariant color = action.param(bulbColorStateParamTypeId).value();
            device->setStateValue(bulbColorStateTypeId, color);
            return DeviceManager::DeviceErrorNoError;

        } else if (action.actionTypeId() == bulbPowerActionTypeId) {
            bool power = action.param(bulbPowerStateParamTypeId).value().toBool();
            device->setStateValue(bulbPowerStateTypeId, power);
            return DeviceManager::DeviceErrorNoError;
        }

        return DeviceManager::DeviceErrorActionTypeNotFound;
    }

    if (device->deviceClassId() == heatingRodDeviceClassId) {

        if (action.actionTypeId() == powerActionTypeId) {
            bool power = action.param(powerStateParamTypeId).value().toBool();
            device->setStateValue(powerStateTypeId, power);
            return DeviceManager::DeviceErrorNoError;
        } else if (action.actionTypeId() == waterTemperatureActionTypeId) {
            int temperature = action.param(waterTemperatureStateParamTypeId).value().toInt();
            device->setStateValue(waterTemperatureStateTypeId, temperature);
            return DeviceManager::DeviceErrorNoError;
        } else if (action.actionTypeId() == maxPowerActionTypeId) {
            double maxPower = action.param(maxPowerStateParamTypeId).value().toDouble();
            device->setStateValue(maxPowerStateTypeId, maxPower);
            return DeviceManager::DeviceErrorNoError;
        }

        return DeviceManager::DeviceErrorActionTypeNotFound;
    }

    if (device->deviceClassId() == batteryDeviceClassId) {
        if (action.actionTypeId() == maxChargingActionTypeId) {
            int maxCharging = action.param(maxChargingStateParamTypeId).value().toInt();
            device->setStateValue(maxChargingStateTypeId, maxCharging);
            device->setStateValue(chargingBatteryStateTypeId, ((double)maxCharging-10)/1000);
            return DeviceManager::DeviceErrorNoError;
        }
        return DeviceManager::DeviceErrorActionTypeNotFound;
    }


    return DeviceManager::DeviceErrorDeviceClassNotFound;
}

void DevicePluginSimulation::guhTimer(){

    foreach (Device *device, myDevices()) {
        if (device->deviceClassId() == temperatureSensorDeviceClassId) {
            //generate Random Number
            double temperature = ((double)getRandomNumber(200, 230)/10.0);
            device->setStateValue(temperatureStateTypeId, temperature);
            device->setStateValue(humidityStateTypeId, getRandomNumber(40, 60));
            device->setStateValue(batteryStateTypeId, 93);
            device->setStateValue(reachableStateTypeId, true);

        }else if(device->deviceClassId() == motionDetectorDeviceClassId){
            bool active = true;
            if(getRandomNumber(0, 60)){
                active = false;
            }
            device->setStateValue(activeStateTypeId, active);
            device->setStateValue(batteryStateTypeId, 82);
            device->setStateValue(reachableStateTypeId, true);
        }else if(device->deviceClassId() == evChargerDeviceClassId){

        }
    }
}

int DevicePluginSimulation::getRandomNumber(const int Min, const int Max)
{
    return ((qrand() % ((Max + 1) - Min)) + Min);
}

