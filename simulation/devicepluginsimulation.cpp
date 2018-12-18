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

#include <QtMath>
#include <QColor>
#include <QDateTime>
#include <QSettings>

DevicePluginSimulation::DevicePluginSimulation()
{

}

DevicePluginSimulation::~DevicePluginSimulation()
{
    hardwareManager()->pluginTimerManager()->unregisterTimer(m_pluginTimer20Seconds);
    hardwareManager()->pluginTimerManager()->unregisterTimer(m_pluginTimer5Min);
}

void DevicePluginSimulation::init()
{
    // Seed the random generator with current time
    qsrand(QDateTime::currentMSecsSinceEpoch() / 1000);

    // Change some values every 20 seconds
    m_pluginTimer20Seconds = hardwareManager()->pluginTimerManager()->registerTimer(20);
    connect(m_pluginTimer20Seconds, &PluginTimer::timeout, this, &DevicePluginSimulation::onPluginTimer20Seconds);

    // Change some values every 5 min
    m_pluginTimer5Min = hardwareManager()->pluginTimerManager()->registerTimer(300);
    connect(m_pluginTimer5Min, &PluginTimer::timeout, this, &DevicePluginSimulation::onPluginTimer5Minutes);
}

DeviceManager::DeviceSetupStatus DevicePluginSimulation::setupDevice(Device *device)
{
    qCDebug(dcSimulation()) << "Set up device" << device->name();
    if (device->deviceClassId() == garageGateDeviceClassId ||
            device->deviceClassId() == extendedAwningDeviceClassId ||
            device->deviceClassId() == extendedBlindDeviceClassId ||
            device->deviceClassId() == rollerShutterDeviceClassId ||
            device->deviceClassId() == fingerPrintSensorDeviceClassId) {
        m_simulationTimers.insert(device, new QTimer(device));
        connect(m_simulationTimers[device], &QTimer::timeout, this, &DevicePluginSimulation::simulationTimerTimeout);
    }
    if (device->deviceClassId() == fingerPrintSensorDeviceClassId && device->stateValue(fingerPrintSensorUsersStateTypeId).toStringList().count() > 0) {
        m_simulationTimers.value(device)->start(10000);
    }
    return DeviceManager::DeviceSetupStatusSuccess;
}

void DevicePluginSimulation::deviceRemoved(Device *device)
{
    // Clean up any timers we may have for this device
    if (m_simulationTimers.contains(device)) {
        QTimer *t = m_simulationTimers.take(device);
        t->stop();
        t->deleteLater();
    }
}

DeviceManager::DeviceError DevicePluginSimulation::executeAction(Device *device, const Action &action)
{
    // Check the DeviceClassId for "Simple Button"
    if (device->deviceClassId() == simpleButtonDeviceClassId ) {

        // check if this is the "press" action
        if (action.actionTypeId() == simpleButtonTriggerActionTypeId) {

            // Emit the "button pressed" event
            qCDebug(dcSimulation()) << "Emit button pressed event for" << device->name();
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
            Param powerParam = action.param(alternativeButtonPowerActionPowerParamTypeId);
            bool power = powerParam.value().toBool();

            qCDebug(dcSimulation()) << "Set power" << power << "for button" << device->name();

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
            Param powerParam = action.param(heatingPowerActionPowerParamTypeId);
            bool power = powerParam.value().toBool();
            qCDebug(dcSimulation()) << "Set power" << power << "for heating device" << device->name();
            device->setStateValue(heatingPowerStateTypeId, power);
            return DeviceManager::DeviceErrorNoError;

        } else if (action.actionTypeId() == heatingPercentageActionTypeId) {

            // get the param value
            Param percentageParam = action.param(heatingPercentageActionPercentageParamTypeId);
            int percentage = percentageParam.value().toInt();

            qCDebug(dcSimulation()) << "Set target temperature percentage" << percentage << "for heating device" << device->name();

            device->setStateValue(heatingPercentageStateTypeId, percentage);
            return DeviceManager::DeviceErrorNoError;
        }
        return DeviceManager::DeviceErrorActionTypeNotFound;
    }

    if (device->deviceClassId() == evChargerDeviceClassId){

        if (action.actionTypeId() == evChargerPowerActionTypeId){
            // get the param value
            Param powerParam = action.param(evChargerPowerActionPowerParamTypeId);
            bool power = powerParam.value().toBool();

            qCDebug(dcSimulation()) << "Set power" << power << "for heating device" << device->name();

            device->setStateValue(evChargerPowerStateTypeId, power);
            return DeviceManager::DeviceErrorNoError;

        } else if(action.actionTypeId() == evChargerPercentageActionTypeId){
            // get the param value
            Param currentParam = action.param(evChargerPercentageActionPercentageParamTypeId);
            int percentage = currentParam.value().toInt();
            qCDebug(dcSimulation()) << "Set percentage" << percentage << "for EV Charger device" << device->name();
            device->setStateValue(evChargerPercentageStateTypeId, percentage);
            return DeviceManager::DeviceErrorNoError;
        }
        return DeviceManager::DeviceErrorActionTypeNotFound;
    }

    if(device->deviceClassId() == socketDeviceClassId){

        if(action.actionTypeId() == socketPowerActionTypeId){
            // get the param value
            Param powerParam = action.param(socketPowerActionPowerParamTypeId);
            bool power = powerParam.value().toBool();
            // Set the "power" state
            qCDebug(dcSimulation()) << "Set power" << power << "for socket device" << device->name();
            device->setStateValue(socketPowerStateTypeId, power);
            return DeviceManager::DeviceErrorNoError;
        }
        return DeviceManager::DeviceErrorActionTypeNotFound;
    }

    if(device->deviceClassId() == colorBulbDeviceClassId){

        if(action.actionTypeId() == colorBulbBrightnessActionTypeId){
            int brightness = action.param(colorBulbBrightnessActionBrightnessParamTypeId).value().toInt();
            qCDebug(dcSimulation()) << "Set brightness" << brightness << "for color bulb device" << device->name();
            device->setStateValue(colorBulbBrightnessStateTypeId, brightness);
            return DeviceManager::DeviceErrorNoError;

        } else if (action.actionTypeId() == colorBulbColorTemperatureActionTypeId){
            int temperature = action.param(colorBulbColorTemperatureActionColorTemperatureParamTypeId).value().toInt();
            qCDebug(dcSimulation()) << "Set color temperature" << temperature << "for color bulb device" << device->name();
            device->setStateValue(colorBulbColorTemperatureStateTypeId, temperature);
            return DeviceManager::DeviceErrorNoError;

        } else if (action.actionTypeId() == colorBulbColorActionTypeId) {
            QColor color = action.param(colorBulbColorActionColorParamTypeId).value().value<QColor>();
            qCDebug(dcSimulation()) << "Set color" << color << "for color bulb device" << device->name();
            device->setStateValue(colorBulbColorStateTypeId, color);
            return DeviceManager::DeviceErrorNoError;

        } else if (action.actionTypeId() == colorBulbPowerActionTypeId) {
            bool power = action.param(colorBulbPowerActionPowerParamTypeId).value().toBool();
            qCDebug(dcSimulation()) << "Set power" << power << "for color bulb device" << device->name();
            device->setStateValue(colorBulbPowerStateTypeId, power);
            return DeviceManager::DeviceErrorNoError;
        }

        return DeviceManager::DeviceErrorActionTypeNotFound;
    }

    if (device->deviceClassId() == heatingRodDeviceClassId) {

        if (action.actionTypeId() == heatingRodPowerActionTypeId) {
            bool power = action.param(heatingRodPowerActionPowerParamTypeId).value().toBool();
            qCDebug(dcSimulation()) << "Set power" << power << "for heating rod device" << device->name();
            device->setStateValue(heatingRodPowerStateTypeId, power);
            return DeviceManager::DeviceErrorNoError;
        } else if (action.actionTypeId() == heatingRodPercentageActionTypeId) {
            int percentage = action.param(heatingRodPercentageActionPercentageParamTypeId).value().toInt();
            qCDebug(dcSimulation()) << "Set percentage" << percentage << "for heating rod device" << device->name();
            device->setStateValue(heatingRodPercentageStateTypeId, percentage);
            return DeviceManager::DeviceErrorNoError;
        }

        return DeviceManager::DeviceErrorActionTypeNotFound;
    }

    if (device->deviceClassId() == batteryDeviceClassId) {
        if (action.actionTypeId() == batteryMaxChargingActionTypeId) {
            int maxCharging = action.param(batteryMaxChargingActionMaxChargingParamTypeId).value().toInt();
            device->setStateValue(batteryMaxChargingStateTypeId, maxCharging);
            qCDebug(dcSimulation()) << "Set max charging power" << maxCharging << "for battery device" << device->name();
            device->setStateValue(batteryChargingStateTypeId, maxCharging);
            return DeviceManager::DeviceErrorNoError;
        }
        return DeviceManager::DeviceErrorActionTypeNotFound;
    }

    if (device->deviceClassId() == waterValveDeviceClassId) {
        if (action.actionTypeId() == waterValvePowerActionTypeId) {
            bool power = action.param(waterValvePowerActionPowerParamTypeId).value().toBool();
            device->setStateValue(waterValvePowerStateTypeId, power);
            return DeviceManager::DeviceErrorNoError;
        }
        return DeviceManager::DeviceErrorActionTypeNotFound;
    }

    if (device->deviceClassId() == garageGateDeviceClassId) {
        if (action.actionTypeId() == garageGateOpenActionTypeId) {
            if (device->stateValue(garageGateStateStateTypeId).toString() == "opening") {
                qCDebug(dcSimulation()) << "Garage gate already opening.";
                return DeviceManager::DeviceErrorNoError;
            }
            if (device->stateValue(garageGateStateStateTypeId).toString() == "open" &&
                    !device->stateValue(garageGateIntermediatePositionStateTypeId).toBool()) {
                qCDebug(dcSimulation()) << "Garage gate already open.";
                return DeviceManager::DeviceErrorNoError;
            }
            device->setStateValue(garageGateStateStateTypeId, "opening");
            device->setStateValue(garageGateIntermediatePositionStateTypeId, true);
            m_simulationTimers.value(device)->start(5000);
            return DeviceManager::DeviceErrorNoError;
        }
        if (action.actionTypeId() == garageGateCloseActionTypeId) {
            if (device->stateValue(garageGateStateStateTypeId).toString() == "closing") {
                qCDebug(dcSimulation()) << "Garage gate already closing.";
                return DeviceManager::DeviceErrorNoError;
            }
            if (device->stateValue(garageGateStateStateTypeId).toString() == "closed" &&
                    !device->stateValue(garageGateIntermediatePositionStateTypeId).toBool()) {
                qCDebug(dcSimulation()) << "Garage gate already closed.";
                return DeviceManager::DeviceErrorNoError;
            }
            device->setStateValue(garageGateStateStateTypeId, "closing");
            device->setStateValue(garageGateIntermediatePositionStateTypeId, true);
            m_simulationTimers.value(device)->start(5000);
            return DeviceManager::DeviceErrorNoError;
        }
        if (action.actionTypeId() == garageGateStopActionTypeId) {
            if (device->stateValue(garageGateStateStateTypeId).toString() == "opening" ||
                    device->stateValue(garageGateStateStateTypeId).toString() == "closing") {
                device->setStateValue(garageGateStateStateTypeId, "open");
                return DeviceManager::DeviceErrorNoError;
            }
            qCDebug(dcSimulation()) << "Garage gate not moving";
            return DeviceManager::DeviceErrorNoError;
        }
        if (action.actionTypeId() == garageGatePowerActionTypeId) {
            bool power = action.param(garageGatePowerActionPowerParamTypeId).value().toBool();
            device->setStateValue(garageGatePowerStateTypeId, power);
            return DeviceManager::DeviceErrorNoError;
        }
    }

    if (device->deviceClassId() == rollerShutterDeviceClassId) {
        if (action.actionTypeId() == rollerShutterOpenActionTypeId) {
            qCDebug(dcSimulation()) << "Opening roller shutter";
            m_simulationTimers.value(device)->setProperty("targetValue", 0);
            m_simulationTimers.value(device)->start(500);
            device->setStateValue(rollerShutterMovingStateTypeId, true);
            return DeviceManager::DeviceErrorNoError;
        }
        if (action.actionTypeId() == rollerShutterCloseActionTypeId) {
            qCDebug(dcSimulation()) << "Closing roller shutter";
            m_simulationTimers.value(device)->setProperty("targetValue", 100);
            m_simulationTimers.value(device)->start(500);
            device->setStateValue(rollerShutterMovingStateTypeId, true);
            return DeviceManager::DeviceErrorNoError;
        }
        if (action.actionTypeId() == rollerShutterStopActionTypeId) {
            qCDebug(dcSimulation()) << "Stopping roller shutter";
            m_simulationTimers.value(device)->stop();
            device->setStateValue(rollerShutterMovingStateTypeId, false);
            return DeviceManager::DeviceErrorNoError;
        }
        if (action.actionTypeId() == rollerShutterPercentageActionTypeId) {
            qCDebug(dcSimulation()) << "Setting awning to" << action.param(rollerShutterPercentageActionPercentageParamTypeId);
            m_simulationTimers.value(device)->setProperty("targetValue", action.param(rollerShutterPercentageActionPercentageParamTypeId).value());
            m_simulationTimers.value(device)->start(500);
            device->setStateValue(rollerShutterMovingStateTypeId, true);
            return DeviceManager::DeviceErrorNoError;
        }
    }

    if (device->deviceClassId() == extendedAwningDeviceClassId) {
        if (action.actionTypeId() == extendedAwningOpenActionTypeId) {
            qCDebug(dcSimulation()) << "Opening awning";
            m_simulationTimers.value(device)->setProperty("targetValue", 100);
            m_simulationTimers.value(device)->start(500);
            device->setStateValue(extendedAwningMovingStateTypeId, true);
            return DeviceManager::DeviceErrorNoError;
        }
        if (action.actionTypeId() == extendedAwningCloseActionTypeId) {
            qCDebug(dcSimulation()) << "Closing awning";
            m_simulationTimers.value(device)->setProperty("targetValue", 0);
            m_simulationTimers.value(device)->start(500);
            device->setStateValue(extendedAwningMovingStateTypeId, true);
            return DeviceManager::DeviceErrorNoError;
        }
        if (action.actionTypeId() == extendedAwningStopActionTypeId) {
            qCDebug(dcSimulation()) << "Stopping awning";
            m_simulationTimers.value(device)->stop();
            device->setStateValue(extendedAwningMovingStateTypeId, false);
            return DeviceManager::DeviceErrorNoError;
        }
        if (action.actionTypeId() == extendedAwningPercentageActionTypeId) {
            qCDebug(dcSimulation()) << "Setting awning to" << action.param(extendedAwningPercentageActionPercentageParamTypeId);
            m_simulationTimers.value(device)->setProperty("targetValue", action.param(extendedAwningPercentageActionPercentageParamTypeId).value());
            m_simulationTimers.value(device)->start(500);
            device->setStateValue(extendedAwningMovingStateTypeId, true);
            return DeviceManager::DeviceErrorNoError;
        }
    }

    if (device->deviceClassId() == fingerPrintSensorDeviceClassId) {
        if (action.actionTypeId() == fingerPrintSensorAddUserActionTypeId) {
            QStringList users = device->stateValue(fingerPrintSensorUsersStateTypeId).toStringList();
            QString username = action.param(fingerPrintSensorAddUserActionUserIdParamTypeId).value().toString();
            QString finger = action.param(fingerPrintSensorAddUserActionFingerParamTypeId).value().toString();
            QSettings settings;
            settings.beginGroup(device->id().toString());
            QStringList usedFingers = settings.value(username).toStringList();
            if (users.contains(username) && usedFingers.contains(finger)) {
                return DeviceManager::DeviceErrorDuplicateUuid;
            }
            QTimer::singleShot(5000, this, [this, action, device, username, finger]() {
                if (username.toLower().trimmed() == "john") {
                    emit actionExecutionFinished(action.id(), DeviceManager::DeviceErrorHardwareFailure);
                } else {
                    emit actionExecutionFinished(action.id(), DeviceManager::DeviceErrorNoError);
                    QStringList users = device->stateValue(fingerPrintSensorUsersStateTypeId).toStringList();
                    if (!users.contains(username)) {
                        users.append(username);
                        device->setStateValue(fingerPrintSensorUsersStateTypeId, users);
                        m_simulationTimers.value(device)->start(10000);
                    }

                    QSettings settings;
                    settings.beginGroup(device->id().toString());
                    QStringList usedFingers = settings.value(username).toStringList();
                    usedFingers.append(finger);
                    settings.setValue(username, usedFingers);
                    settings.endGroup();
                }
            });
            return DeviceManager::DeviceErrorAsync;
        }
        if (action.actionTypeId() == fingerPrintSensorRemoveUserActionTypeId) {
            QStringList users = device->stateValue(fingerPrintSensorUsersStateTypeId).toStringList();
            QString username = action.params().first().value().toString();
            if (!users.contains(username)) {
                return DeviceManager::DeviceErrorInvalidParameter;
            }
            users.removeAll(username);
            device->setStateValue(fingerPrintSensorUsersStateTypeId, users);
            if (users.count() == 0) {
                m_simulationTimers.value(device)->stop();
            }
            return DeviceManager::DeviceErrorNoError;
        }
    }

    if (device->deviceClassId() == simpleBlindDeviceClassId) {
        if (action.actionTypeId() == simpleBlindOpenActionTypeId) {
            qCDebug(dcSimulation()) << "Opening simple blind";
            return DeviceManager::DeviceErrorNoError;
        }
        if (action.actionTypeId() == simpleBlindCloseActionTypeId) {
            qCDebug(dcSimulation()) << "Closing simple blind";
            return DeviceManager::DeviceErrorNoError;
        }
        if (action.actionTypeId() == simpleBlindStopActionTypeId) {
            qCDebug(dcSimulation()) << "Stopping simple blind";
            return DeviceManager::DeviceErrorNoError;
        }
    }

    if (device->deviceClassId() == extendedBlindDeviceClassId) {
        if (action.actionTypeId() == extendedBlindOpenActionTypeId) {
            qCDebug(dcSimulation()) << "Opening extended blind";
            m_simulationTimers.value(device)->setProperty("targetValue", 0);
            m_simulationTimers.value(device)->start(500);
            device->setStateValue(extendedBlindMovingStateTypeId, true);
            return DeviceManager::DeviceErrorNoError;
        }
        if (action.actionTypeId() == extendedBlindCloseActionTypeId) {
            qCDebug(dcSimulation()) << "Closing extended blind";
            m_simulationTimers.value(device)->setProperty("targetValue", 100);
            m_simulationTimers.value(device)->start(500);
            device->setStateValue(extendedBlindMovingStateTypeId, true);
            return DeviceManager::DeviceErrorNoError;
        }
        if (action.actionTypeId() == extendedBlindStopActionTypeId) {
            qCDebug(dcSimulation()) << "Stopping extended blind";
            m_simulationTimers.value(device)->stop();
            device->setStateValue(extendedBlindMovingStateTypeId, false);
            return DeviceManager::DeviceErrorNoError;
        }
        if (action.actionTypeId() == extendedBlindPercentageActionTypeId) {
            qCDebug(dcSimulation()) << "Setting extended blind to" << action.param(extendedBlindPercentageActionPercentageParamTypeId);
            m_simulationTimers.value(device)->setProperty("targetValue", action.param(extendedBlindPercentageActionPercentageParamTypeId).value());
            m_simulationTimers.value(device)->start(500);
            device->setStateValue(extendedBlindMovingStateTypeId, true);
            return DeviceManager::DeviceErrorNoError;
        }
    }

    qCWarning(dcSimulation()) << "Unhandled device class" << device->deviceClassId() << "for device" << device->name();

    return DeviceManager::DeviceErrorDeviceClassNotFound;
}

int DevicePluginSimulation::generateRandomIntValue(int min, int max)
{
    int value = ((qrand() % ((max + 1) - min)) + min);
    // qCDebug(dcSimulation()) << "Generateed random int value: [" << min << ", " << max << "] -->" << value;
    return value;
}

double DevicePluginSimulation::generateRandomDoubleValue(double min, double max)
{
    double value = generateRandomIntValue(static_cast<int>(min * 10), static_cast<int>(max * 10)) / 10.0;
    // qCDebug(dcSimulation()) << "Generated random double value: [" << min << ", " << max << "] -->" << value;
    return value;
}

bool DevicePluginSimulation::generateRandomBoolValue()
{
    bool value = static_cast<bool>(generateRandomIntValue(0, 1));
    // qCDebug(dcSimulation()) << "Generated random bool value:" << value;
    return value;
}

qreal DevicePluginSimulation::generateSinValue(int min, int max, int hourOffset, int decimals)
{
    // 00:00 : 23:99 = 0 : PI
    // seconds of day : (60 * 60 * 24) = x : 2*PI
    QDateTime d = QDateTime::currentDateTime();
    int secondsPerDay = 60 * 60 * 24;
    int offsetInSeconds =  hourOffset * 60 * 60;
    int secondsOfDay = d.time().msecsSinceStartOfDay() / 1000;
    // add offset and wrap around
    secondsOfDay = (secondsOfDay - offsetInSeconds) % secondsPerDay;

    qreal interval = secondsOfDay * 2*M_PI / secondsPerDay;
    qreal gain = 1.0 * (max - min) / 2;
    qreal temp = (gain * qSin(interval)) + min + gain;
    return QString::number(temp, 'f', decimals).toDouble();
}

qreal DevicePluginSimulation::generateBatteryValue(int chargeStartHour, int chargeDurationInMinutes)
{
    QDateTime d = QDateTime::currentDateTime();

    int secondsPerDay = 24 * 60 * 60;
    int currentSecond = d.time().msecsSinceStartOfDay() / 1000;
    int chargeStartSecond = chargeStartHour * 60 * 60;
    int chargeEndSecond = chargeStartSecond + (chargeDurationInMinutes * 60);
    int chargeDurationInSeconds = chargeDurationInMinutes * 60;

    // should we be charging?
    if (chargeStartSecond < currentSecond && currentSecond < chargeEndSecond) {
        // Yep, charging...
        int currentChargeSecond = currentSecond - chargeStartSecond;
        // x : 100 = currentChargeSecond : chargeDurationInSeconds
        return 100 * currentChargeSecond / chargeDurationInSeconds;
    }

    int dischargeDurationInSecs = secondsPerDay - chargeDurationInSeconds;
    int currentDischargeSecond;
    if (currentSecond < chargeStartSecond) {
        currentDischargeSecond = currentSecond + (secondsPerDay - chargeEndSecond);
    } else {
        currentDischargeSecond = currentSecond - chargeEndSecond;
    }
    // 100 : x = dischargeDurationInSecs : currentDischargeSecond
    return 100 - (100 * currentDischargeSecond / dischargeDurationInSecs);
}

qreal DevicePluginSimulation::generateNoisyRectangle(int min, int max, int maxNoise, int stablePeriodInMinutes, int &lastValue, QDateTime &lastChangeTimestamp)
{
    QDateTime now = QDateTime::currentDateTime();
    qCDebug(dcSimulation()) << "Generating noisy rect:" << min << "-" << max << "lastValue:" << lastValue << "lastUpdate" << lastChangeTimestamp << lastChangeTimestamp.secsTo(now) << lastChangeTimestamp.isValid();
    if (!lastChangeTimestamp.isValid() || lastChangeTimestamp.secsTo(now) / 60 > stablePeriodInMinutes) {
        lastChangeTimestamp.swap(now);
        lastValue = min + qrand() % (max - min);
        qCDebug(dcSimulation()) << "New last value:" << lastValue;
    }
    qreal noise = 0.1 * (qrand() % (maxNoise * 20)  - maxNoise);
    qreal ret = 1.0 * lastValue + noise;
    return ret;
}

void DevicePluginSimulation::onPluginTimer20Seconds()
{
    foreach (Device *device, myDevices()) {
        if (device->deviceClassId() == temperatureSensorDeviceClassId) {
            // Temperature sensor
            device->setStateValue(temperatureSensorTemperatureStateTypeId, generateSinValue(18, 23, 8));
            device->setStateValue(temperatureSensorHumidityStateTypeId, generateSinValue(40, 55, 20));
            device->setStateValue(temperatureSensorBatteryLevelStateTypeId, generateBatteryValue(8, 10));
            device->setStateValue(temperatureSensorBatteryCriticalStateTypeId, device->stateValue(temperatureSensorBatteryLevelStateTypeId).toInt() <= 25);
            device->setStateValue(temperatureSensorConnectedStateTypeId, true);
        } else if (device->deviceClassId() == motionDetectorDeviceClassId) {
            // Motion detector
            device->setStateValue(motionDetectorActiveStateTypeId, generateRandomBoolValue());
            device->setStateValue(motionDetectorBatteryLevelStateTypeId, generateBatteryValue(13, 1));
            device->setStateValue(motionDetectorBatteryCriticalStateTypeId, device->stateValue(motionDetectorBatteryLevelStateTypeId).toInt() <= 30);
            device->setStateValue(motionDetectorConnectedStateTypeId, true);
        } else if (device->deviceClassId() == gardenSensorDeviceClassId) {
            // Garden sensor
            device->setStateValue(gardenSensorTemperatureStateTypeId, generateSinValue(-4, 17, 5));
            device->setStateValue(gardenSensorSoilMoistureStateTypeId, generateSinValue(40, 60, 13));
            device->setStateValue(gardenSensorIlluminanceStateTypeId, generateSinValue(0, 80, 2));
            device->setStateValue(gardenSensorBatteryLevelStateTypeId, generateBatteryValue(9, 20));
            device->setStateValue(gardenSensorBatteryCriticalStateTypeId, device->stateValue(gardenSensorBatteryLevelStateTypeId).toDouble() <= 30);
            device->setStateValue(gardenSensorConnectedStateTypeId, true);
        } else if(device->deviceClassId() == netatmoIndoorDeviceClassId) {
            // Netatmo
            device->setStateValue(netatmoIndoorUpdateTimeStateTypeId, QDateTime::currentDateTime().toTime_t());
            device->setStateValue(netatmoIndoorHumidityStateTypeId, generateSinValue(35, 45, 13));
            device->setStateValue(netatmoIndoorTemperatureStateTypeId, generateSinValue(20, 25, 3));
            device->setStateValue(netatmoIndoorPressureStateTypeId, generateSinValue(1003, 1008, 8));
            device->setStateValue(netatmoIndoorNoiseStateTypeId, generateRandomIntValue(40, 80));
            device->setStateValue(netatmoIndoorWifiStrengthStateTypeId, generateRandomIntValue(85, 95));
        } else if (device->deviceClassId() == smartMeterDeviceClassId) {
            device->setStateValue(smartMeterConnectedStateTypeId, true);
            int lastValue = device->property("lastValue").toInt();
            QDateTime lastUpdate = device->property("lastUpdate").toDateTime();
            qlonglong currentPower = generateNoisyRectangle(-2000, 100, 10, 5, lastValue, lastUpdate);
            device->setStateValue(smartMeterCurrentPowerStateTypeId, currentPower);
            device->setProperty("lastValue", lastValue);
            device->setProperty("lastUpdate", lastUpdate);
            if (currentPower < 0) {
                qreal consumptionKWH = 1.0 * currentPower * (1.0 * m_pluginTimer20Seconds->interval() / 1000 / 60 / 60) / 1000;
                device->setStateValue(smartMeterTotalEnergyConsumedStateTypeId, device->stateValue(smartMeterTotalEnergyConsumedStateTypeId).toDouble() - consumptionKWH);
            }
            if (currentPower > 0) {
                qreal consumptionKWH = 1.0 * currentPower * (1.0 * m_pluginTimer20Seconds->interval() / 1000 / 60 / 60) / 1000;
                device->setStateValue(smartMeterTotalEnergyProducedStateTypeId, device->stateValue(smartMeterTotalEnergyProducedStateTypeId).toDouble() + consumptionKWH);
            }
        } else if (device->deviceClassId() == solarPanelDeviceClassId) {
            int lastValue = device->property("lastValue").toInt();
            QDateTime lastUpdate = device->property("lastUpdate").toDateTime();
            qlonglong currentPower = generateNoisyRectangle(0, 2000, 50, 5, lastValue, lastUpdate);
            device->setStateValue(solarPanelCurrentPowerStateTypeId, currentPower);
            device->setProperty("lastValue", lastValue);
            device->setProperty("lastUpdate", lastUpdate);
            qreal consumptionKWH = 1.0 * currentPower * (1.0 * m_pluginTimer20Seconds->interval() / 1000 / 60 / 60) / 1000;
            device->setStateValue(solarPanelTotalEnergyProducedStateTypeId, device->stateValue(solarPanelTotalEnergyProducedStateTypeId).toDouble() + consumptionKWH);
        }

    }
}

void DevicePluginSimulation::onPluginTimer5Minutes()
{
    foreach (Device *device, myDevices()) {
        if(device->deviceClassId() == netatmoIndoorDeviceClassId) {
            // Note: should change between > 1000 co2 < 1000 for showcase, please do not change this behaviour
            int currentValue = device->stateValue(netatmoIndoorCo2StateTypeId).toInt();
            if (currentValue < 1000) {
                device->setStateValue(netatmoIndoorCo2StateTypeId, generateRandomIntValue(1001, 1010));
            } else {
                device->setStateValue(netatmoIndoorCo2StateTypeId, generateRandomIntValue(950, 999));
            }
        }
    }
}

void DevicePluginSimulation::simulationTimerTimeout()
{
    QTimer *t = static_cast<QTimer*>(sender());
    Device *device = m_simulationTimers.key(t);
    if (device->deviceClassId() == garageGateDeviceClassId) {
        if (device->stateValue(garageGateStateStateTypeId).toString() == "opening") {
            device->setStateValue(garageGateIntermediatePositionStateTypeId, false);
            device->setStateValue(garageGateStateStateTypeId, "open");
        }
        if (device->stateValue(garageGateStateStateTypeId).toString() == "closing") {
            device->setStateValue(garageGateIntermediatePositionStateTypeId, false);
            device->setStateValue(garageGateStateStateTypeId, "closed");
        }
    } else if (device->deviceClassId() == extendedAwningDeviceClassId) {
        int currentValue = device->stateValue(extendedAwningPercentageStateTypeId).toInt();
        int targetValue = t->property("targetValue").toInt();
        int newValue = targetValue > currentValue ? qMin(targetValue, currentValue + 5) : qMax(targetValue, currentValue - 5);
        device->setStateValue(extendedAwningPercentageStateTypeId, newValue);
        if (newValue == targetValue) {
            t->stop();
            device->setStateValue(extendedAwningMovingStateTypeId, false);
        }
    } else if (device->deviceClassId() == extendedBlindDeviceClassId) {
        int currentValue = device->stateValue(extendedBlindPercentageStateTypeId).toInt();
        int targetValue = t->property("targetValue").toInt();
        int newValue = targetValue > currentValue ? qMin(targetValue, currentValue + 5) : qMax(targetValue, currentValue - 5);
        device->setStateValue(extendedBlindPercentageStateTypeId, newValue);
        if (newValue == targetValue) {
            t->stop();
            device->setStateValue(extendedBlindMovingStateTypeId, false);
        }
    } else if (device->deviceClassId() == rollerShutterDeviceClassId) {
        int currentValue = device->stateValue(rollerShutterPercentageStateTypeId).toInt();
        int targetValue = t->property("targetValue").toInt();
        int newValue = targetValue > currentValue ? qMin(targetValue, currentValue + 5) : qMax(targetValue, currentValue - 5);
        device->setStateValue(rollerShutterPercentageStateTypeId, newValue);
        if (newValue == targetValue) {
            t->stop();
            device->setStateValue(rollerShutterMovingStateTypeId, false);
        }
    } else if (device->deviceClassId() == fingerPrintSensorDeviceClassId) {
        EventTypeId evt = qrand() % 2 == 0 ? fingerPrintSensorAccessGrantedEventTypeId : fingerPrintSensorAccessDeniedEventTypeId;
        ParamList params;
        if (evt == fingerPrintSensorAccessGrantedEventTypeId) {
            QStringList users = device->stateValue(fingerPrintSensorUsersStateTypeId).toStringList();
            QString user = users.at(qrand() % users.count());
            QSettings settings;
            settings.beginGroup(device->id().toString());
            QStringList fingers = settings.value(user).toStringList();
            params.append(Param(fingerPrintSensorAccessGrantedEventUserIdParamTypeId, user));
            QString finger = fingers.at(qrand() % fingers.count());
            params.append(Param(fingerPrintSensorAccessGrantedEventFingerParamTypeId, finger));
            qCDebug(dcSimulation()) << "Emitting fingerprint accepted for user" << user << "and finger" << finger;
        } else {
            qCDebug(dcSimulation()) << "Emitting fingerprint denied";
        }
        Event event(evt, device->id(), params);
        emitEvent(event);
    }
}
