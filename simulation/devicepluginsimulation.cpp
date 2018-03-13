/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *                                                                         *
 *  Copyright (C) 2017 Bernhard Trinnes <bernhard.trinnes@guh.io>          *
 *  Copyright (C) 2018 Simon Stürz <simon.stuerz@guh.io>                   *
 *                                                                         *
 *  This file is part of nymea.                                            *
 *                                                                         *
 *  nymea is free software: you can redistribute it and/or modify          *
 *  it under the terms of the GNU General Public License as published by   *
 *  the Free Software Foundation, version 2 of the License.                *
 *                                                                         *
 *  nymea is distributed in the hope that it will be useful,               *
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of         *
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the           *
 *  GNU General Public License for more details.                           *
 *                                                                         *
 *  You should have received a copy of the GNU General Public License      *
 *  along with nymea. If not, see <http://www.gnu.org/licenses/>.          *
 *                                                                         *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

#include "devicepluginsimulation.h"
#include "plugininfo.h"


DevicePluginSimulation::DevicePluginSimulation()
{

}

void DevicePluginSimulation::init()
{
    m_pluginTimer = hardwareManager()->pluginTimerManager()->registerTimer(10);
    connect(m_pluginTimer, &PluginTimer::timeout, this, &DevicePluginSimulation::onPluginTimer);
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
        if (action.actionTypeId() == simpleButtonTriggerActionTypeId) {


            // Emit the "button pressed" event
            Event event(simpleButtonPressedEventTypeId, device->id());
            emit emitEvent(event);

            return DeviceManager::DeviceErrorNoError;
        }
        return DeviceManager::DeviceErrorActionTypeNotFound;
    }

    // Check the DeviceClassId for "Alternative Button"
    if (device->deviceClassId() == alternativeButtonDeviceClassId) {

        // check if this is the "set power" action
        if (action.actionTypeId() == alternativeButtonPowerActionTypeId) {

            // get the param value
            Param powerParam = action.param(alternativeButtonPowerStateParamTypeId);
            bool power = powerParam.value().toBool();

            qCDebug(dcSimulation) << "ActionTypeId :" << action.actionTypeId().toString();
            qCDebug(dcSimulation) << "StateTypeId  :" << alternativeButtonPowerStateTypeId.toString();

            // Set the "power" state
            device->setStateValue(alternativeButtonPowerStateTypeId, power);

            return DeviceManager::DeviceErrorNoError;
        }
        return DeviceManager::DeviceErrorActionTypeNotFound;
    }

    if (device->deviceClassId() == heatingDeviceClassId) {

        // check if this is the "set power" action
        if (action.actionTypeId() == heatingPowerActionTypeId) {

            // get the param value
            Param powerParam = action.param(heatingPowerStateParamTypeId);
            bool power = powerParam.value().toBool();
            // Set the "power" state
            device->setStateValue(heatingPowerStateTypeId, power);
            return DeviceManager::DeviceErrorNoError;

        } else if (action.actionTypeId() == heatingTargetTemperatureActionTypeId) {

            // get the param value
            Param temperatureParam = action.param(heatingTargetTemperatureStateParamTypeId);
            int temperature = temperatureParam.value().toInt();

            // Set the "temperature" state
            device->setStateValue(heatingTargetTemperatureStateTypeId, temperature);
            return DeviceManager::DeviceErrorNoError;
        }
        return DeviceManager::DeviceErrorActionTypeNotFound;
    }

    if (device->deviceClassId() == evChargerDeviceClassId){

        if (action.actionTypeId() == evChargerPowerActionTypeId){
            // get the param value
            Param powerParam = action.param(evChargerPowerStateParamTypeId);
            bool power = powerParam.value().toBool();
            // Set the "power" state
            device->setStateValue(evChargerPowerStateTypeId, power);
            return DeviceManager::DeviceErrorNoError;

        } else if(action.actionTypeId() == evChargerCurrentActionTypeId){
            // get the param value
            Param currentParam = action.param(evChargerCurrentStateParamTypeId);
            int current = currentParam.value().toInt();
            // Set the "current" state
            device->setStateValue(evChargerCurrentStateTypeId, current);
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

    if(device->deviceClassId() == colorBulbDeviceClassId){

        if(action.actionTypeId() == colorBulbBrightnessActionTypeId){
            int brightness = action.param(colorBulbBrightnessStateParamTypeId).value().toInt();
            device->setStateValue(colorBulbBrightnessStateTypeId, brightness);
            return DeviceManager::DeviceErrorNoError;

        } else if (action.actionTypeId() == colorBulbColorTemperatureActionTypeId){
            int temperature = action.param(colorBulbColorTemperatureStateParamTypeId).value().toInt();
            device->setStateValue(colorBulbColorTemperatureStateTypeId, temperature);
            return DeviceManager::DeviceErrorNoError;

        } else if (action.actionTypeId() == colorBulbColorActionTypeId) {
            QVariant color = action.param(colorBulbColorStateParamTypeId).value();
            device->setStateValue(colorBulbColorStateTypeId, color);
            return DeviceManager::DeviceErrorNoError;

        } else if (action.actionTypeId() == colorBulbPowerActionTypeId) {
            bool power = action.param(colorBulbPowerStateParamTypeId).value().toBool();
            device->setStateValue(colorBulbPowerStateTypeId, power);
            return DeviceManager::DeviceErrorNoError;
        }

        return DeviceManager::DeviceErrorActionTypeNotFound;
    }

    if (device->deviceClassId() == heatingRodDeviceClassId) {

        if (action.actionTypeId() == heatingRodPowerActionTypeId) {
            bool power = action.param(heatingRodPowerStateParamTypeId).value().toBool();
            device->setStateValue(heatingRodPowerStateTypeId, power);
            return DeviceManager::DeviceErrorNoError;
        } else if (action.actionTypeId() == heatingRodWaterTemperatureActionTypeId) {
            int temperature = action.param(heatingRodWaterTemperatureStateParamTypeId).value().toInt();
            device->setStateValue(heatingRodWaterTemperatureStateTypeId, temperature);
            return DeviceManager::DeviceErrorNoError;
        } else if (action.actionTypeId() == heatingRodMaxPowerActionTypeId) {
            double maxPower = action.param(heatingRodMaxPowerStateParamTypeId).value().toDouble();
            device->setStateValue(heatingRodMaxPowerStateTypeId, maxPower);
            return DeviceManager::DeviceErrorNoError;
        }

        return DeviceManager::DeviceErrorActionTypeNotFound;
    }

    if (device->deviceClassId() == batteryDeviceClassId) {
        if (action.actionTypeId() == batteryMaxChargingActionTypeId) {
            int maxCharging = action.param(batteryMaxChargingStateParamTypeId).value().toInt();
            device->setStateValue(batteryMaxChargingStateTypeId, maxCharging);
            device->setStateValue(batteryChargingStateTypeId, ((double)maxCharging-10)/1000);
            return DeviceManager::DeviceErrorNoError;
        }
        return DeviceManager::DeviceErrorActionTypeNotFound;
    }

    if (device->deviceClassId() == waterValveDeviceClassId) {
        if (action.actionTypeId() == waterValvePowerActionTypeId) {
            Param powerParam = action.param(waterValvePowerStateParamTypeId);
            bool power = powerParam.value().toBool();
            device->setStateValue(waterValvePowerStateTypeId, power);
            return DeviceManager::DeviceErrorNoError;
        }
        return DeviceManager::DeviceErrorActionTypeNotFound;

    }

    return DeviceManager::DeviceErrorDeviceClassNotFound;
}

int DevicePluginSimulation::getRandomNumber(const int Min, const int Max)
{
    return ((qrand() % ((Max + 1) - Min)) + Min);
}

void DevicePluginSimulation::onPluginTimer()
{
    foreach (Device *device, myDevices()) {
        if (device->deviceClassId() == temperatureSensorDeviceClassId) {
            //generate Random Number
            double temperature = ((double)getRandomNumber(200, 230)/10.0);
            device->setStateValue(temperatureSensorTemperatureStateTypeId, temperature);
            device->setStateValue(temperatureSensorHumidityStateTypeId, getRandomNumber(40, 60));
            device->setStateValue(temperatureSensorBatteryLevelStateTypeId, getRandomNumber(10, 100));
            device->setStateValue(temperatureSensorBatteryCriticalStateTypeId, device->stateValue(temperatureSensorBatteryLevelStateTypeId).toDouble() <= 30);
            device->setStateValue(temperatureSensorConnectedStateTypeId, true);

        } else if (device->deviceClassId() == motionDetectorDeviceClassId) {
            bool active = true;
            if(getRandomNumber(0, 60)){
                active = false;
            }
            device->setStateValue(motionDetectorActiveStateTypeId, active);
            device->setStateValue(motionDetectorBatteryLevelStateTypeId, getRandomNumber(10, 100));
            device->setStateValue(motionDetectorBatteryCriticalStateTypeId, device->stateValue(motionDetectorBatteryLevelStateTypeId).toDouble() <= 30);
            device->setStateValue(motionDetectorConnectedStateTypeId, true);
        } else if (device->deviceClassId() == evChargerDeviceClassId) {


        } else if (device->deviceClassId() == gardenSensorDeviceClassId) {

            //generate Random Number
            double temperature = ((double)getRandomNumber(200, 230)/10.0);
            device->setStateValue(gardenSensorTemperatureStateTypeId, temperature);
            device->setStateValue(gardenSensorSoilMoistureStateTypeId, getRandomNumber(40, 60));
            device->setStateValue(gardenSensorLightIntensityStateTypeId, getRandomNumber(40, 60));
            device->setStateValue(gardenSensorBatteryLevelStateTypeId, getRandomNumber(10, 100));
            device->setStateValue(gardenSensorBatteryCriticalStateTypeId, device->stateValue(gardenSensorBatteryLevelStateTypeId).toDouble() <= 30);
            device->setStateValue(gardenSensorConnectedStateTypeId, true);
        }
    }
}

