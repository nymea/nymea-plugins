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

#include "integrationpluginsimulation.h"
#include "plugininfo.h"

#include <QtMath>
#include <QColor>
#include <QDateTime>
#include <QSettings>

IntegrationPluginSimulation::IntegrationPluginSimulation()
{

}

IntegrationPluginSimulation::~IntegrationPluginSimulation()
{
    hardwareManager()->pluginTimerManager()->unregisterTimer(m_pluginTimer20Seconds);
    hardwareManager()->pluginTimerManager()->unregisterTimer(m_pluginTimer5Min);
}

void IntegrationPluginSimulation::init()
{
    // Seed the random generator with current time
    qsrand(QDateTime::currentMSecsSinceEpoch() / 1000);

    // Change some values every 20 seconds
    m_pluginTimer20Seconds = hardwareManager()->pluginTimerManager()->registerTimer(20);
    connect(m_pluginTimer20Seconds, &PluginTimer::timeout, this, &IntegrationPluginSimulation::onPluginTimer20Seconds);

    // Change some values every 5 min
    m_pluginTimer5Min = hardwareManager()->pluginTimerManager()->registerTimer(300);
    connect(m_pluginTimer5Min, &PluginTimer::timeout, this, &IntegrationPluginSimulation::onPluginTimer5Minutes);
}

void IntegrationPluginSimulation::setupThing(ThingSetupInfo *info)
{
    Thing *thing = info->thing();
    qCDebug(dcSimulation()) << "Set up thing" << thing->name();
    if (thing->thingClassId() == garageGateThingClassId ||
            thing->thingClassId() == extendedAwningThingClassId ||
            thing->thingClassId() == extendedBlindThingClassId ||
            thing->thingClassId() == venetianBlindThingClassId ||
            thing->thingClassId() == rollerShutterThingClassId ||
            thing->thingClassId() == fingerPrintSensorThingClassId ||
            thing->thingClassId() == barcodeScannerThingClassId ||
            thing->thingClassId() == contactSensorThingClassId ||
            thing->thingClassId() == waterSensorThingClassId) {
        m_simulationTimers.insert(thing, new QTimer(thing));
        connect(m_simulationTimers[thing], &QTimer::timeout, this, &IntegrationPluginSimulation::simulationTimerTimeout);
    }
    if (thing->thingClassId() == fingerPrintSensorThingClassId && thing->stateValue(fingerPrintSensorUsersStateTypeId).toStringList().count() > 0) {
        m_simulationTimers.value(thing)->start(10000);
    }
    if (thing->thingClassId() == barcodeScannerThingClassId) {
        m_simulationTimers.value(thing)->start(10000);
    }
    if (thing->thingClassId() == thermostatThingClassId) {
        QTimer *t = new QTimer(thing);
        connect(t, &QTimer::timeout, thing, [thing](){
            double targetTemp = thing->stateValue(thermostatTargetTemperatureStateTypeId).toDouble();
            double currentTemp = thing->stateValue(thermostatTemperatureStateTypeId).toDouble();
            bool heatingOn = thing->stateValue(thermostatHeatingOnStateTypeId).toBool();
            bool coolingOn = thing->stateValue(thermostatCoolingOnStateTypeId).toBool();
            bool boost = thing->stateValue(thermostatBoostStateTypeId).toBool();

            // When we're heating, temp increases slowly until it's up on par with target temp
            if (heatingOn) {
                double diff = targetTemp - currentTemp;
                currentTemp += 0.005 + diff * (boost ? 0.2 : 0.1);
                if (currentTemp >= targetTemp) {
                    thing->setStateValue(thermostatHeatingOnStateTypeId, false);
                }
            } else {
                // Decrease 1% per interval to simulate drop of temperature (assuming it's cold outside)
                currentTemp = currentTemp * 0.995;

                // Start heating when we're more than 2 degrees lower than what we should be
                if (currentTemp < targetTemp - 2) {
                    thing->setStateValue(thermostatHeatingOnStateTypeId, true);
                }
            }

            if (coolingOn) {
                double diff = targetTemp - currentTemp;
                currentTemp += diff * 0.1;
                if (currentTemp <= targetTemp) {
                    thing->setStateValue(thermostatCoolingOnStateTypeId, false);
                }
            } else {
                if (currentTemp > targetTemp + 2) {
                    thing->setStateValue(thermostatCoolingOnStateTypeId, true);
                }
            }

            thing->setStateValue(thermostatTemperatureStateTypeId, currentTemp);
        });
        t->start(10000);
    }

    if (thing->thingClassId() == contactSensorThingClassId) {
        m_simulationTimers.value(thing)->start(10000);
    }
    if (thing->thingClassId() == waterSensorThingClassId) {
        m_simulationTimers.value(thing)->start(10000);
    }
    info->finish(Thing::ThingErrorNoError);
}

void IntegrationPluginSimulation::thingRemoved(Thing *thing)
{
    // Clean up any timers we may have for this thing
    if (m_simulationTimers.contains(thing)) {
        QTimer *t = m_simulationTimers.take(thing);
        t->stop();
        t->deleteLater();
    }
}

void IntegrationPluginSimulation::executeAction(ThingActionInfo *info)
{
    Thing *thing = info->thing();
    Action action = info->action();
    // Check the ThingClassId for "Simple Button"
    if (thing->thingClassId() == simpleButtonThingClassId ) {

        // check if this is the "press" action
        if (action.actionTypeId() == simpleButtonTriggerActionTypeId) {

            // Emit the "button pressed" event
            qCDebug(dcSimulation()) << "Emit button pressed event for" << thing->name();
            Event event(simpleButtonPressedEventTypeId, thing->id());
            emit emitEvent(event);

            return info->finish(Thing::ThingErrorNoError);
        }
        return info->finish(Thing::ThingErrorActionTypeNotFound);
    }

    // Check the ThingClassId for "Alternative Button"
    if (thing->thingClassId() == alternativeButtonThingClassId) {

        // check if this is the "set power" action
        if (action.actionTypeId() == alternativeButtonPowerActionTypeId) {

            // get the param value
            Param powerParam = action.param(alternativeButtonPowerActionPowerParamTypeId);
            bool power = powerParam.value().toBool();

            qCDebug(dcSimulation()) << "Set power" << power << "for button" << thing->name();

            // Set the "power" state
            thing->setStateValue(alternativeButtonPowerStateTypeId, power);

            return info->finish(Thing::ThingErrorNoError);
        }
        return info->finish(Thing::ThingErrorActionTypeNotFound);
    }

    if (thing->thingClassId() == heatingThingClassId) {

        // check if this is the "set power" action
        if (action.actionTypeId() == heatingPowerActionTypeId) {

            // get the param value
            Param powerParam = action.param(heatingPowerActionPowerParamTypeId);
            bool power = powerParam.value().toBool();
            qCDebug(dcSimulation()) << "Set power" << power << "for heating device" << thing->name();
            thing->setStateValue(heatingPowerStateTypeId, power);
            return info->finish(Thing::ThingErrorNoError);

        } else if (action.actionTypeId() == heatingPercentageActionTypeId) {

            // get the param value
            Param percentageParam = action.param(heatingPercentageActionPercentageParamTypeId);
            int percentage = percentageParam.value().toInt();

            qCDebug(dcSimulation()) << "Set target temperature percentage" << percentage << "for heating device" << thing->name();

            thing->setStateValue(heatingPercentageStateTypeId, percentage);
            return info->finish(Thing::ThingErrorNoError);
        }
        return info->finish(Thing::ThingErrorActionTypeNotFound);
    }

    if (thing->thingClassId() == thermostatThingClassId) {
        if (action.actionTypeId() == thermostatBoostActionTypeId) {
            bool boost = action.param(thermostatBoostActionBoostParamTypeId).value().toBool();
            qCDebug(dcSimulation()) << "Set boost" << boost << "for thermostat device" << thing->name();
            thing->setStateValue(thermostatBoostStateTypeId, boost);
            QTimer *t = new QTimer(thing);
            t->setInterval(5 * 60 * 1000);
            t->setSingleShot(true);
            connect(t, &QTimer::timeout, t, &QTimer::deleteLater);
            connect(t, &QTimer::timeout, thing, [thing](){
                thing->setStateValue(thermostatBoostStateTypeId, false);
            });
            return info->finish(Thing::ThingErrorNoError);
        }
        if (action.actionTypeId() == thermostatTargetTemperatureActionTypeId) {
            double targetTemp = action.param(thermostatTargetTemperatureActionTargetTemperatureParamTypeId).value().toDouble();
            qCDebug(dcSimulation()) << "Set targetTemp" << targetTemp << "for thermostat device" << thing->name();
            thing->setStateValue(thermostatTargetTemperatureStateTypeId, targetTemp);
            return info->finish(Thing::ThingErrorNoError);
        }
    }

    if (thing->thingClassId() == evChargerThingClassId){

        if (action.actionTypeId() == evChargerPowerActionTypeId){
            // get the param value
            Param powerParam = action.param(evChargerPowerActionPowerParamTypeId);
            bool power = powerParam.value().toBool();

            qCDebug(dcSimulation()) << "Set power" << power << "for heating device" << thing->name();

            thing->setStateValue(evChargerPowerStateTypeId, power);
            return info->finish(Thing::ThingErrorNoError);

        } else if(action.actionTypeId() == evChargerMaxChargingCurrentActionTypeId){
            // get the param value
            Param maxChargeParam = action.param(evChargerMaxChargingCurrentActionMaxChargingCurrentParamTypeId);
            double maxCharge = maxChargeParam.value().toDouble();
            qCDebug(dcSimulation()) << "Set maximum charging current to" << maxCharge << "for EV Charger device" << thing->name();
            thing->setStateValue(evChargerMaxChargingCurrentStateTypeId, maxCharge);
            return info->finish(Thing::ThingErrorNoError);
        }
        return info->finish(Thing::ThingErrorActionTypeNotFound);
    }

    if(thing->thingClassId() == socketThingClassId){

        if(action.actionTypeId() == socketPowerActionTypeId){
            // get the param value
            Param powerParam = action.param(socketPowerActionPowerParamTypeId);
            bool power = powerParam.value().toBool();
            // Set the "power" state
            qCDebug(dcSimulation()) << "Set power" << power << "for socket device" << thing->name();
            thing->setStateValue(socketPowerStateTypeId, power);
            return info->finish(Thing::ThingErrorNoError);
        }
        return info->finish(Thing::ThingErrorActionTypeNotFound);
    }

    if(thing->thingClassId() == colorBulbThingClassId){

        if(action.actionTypeId() == colorBulbBrightnessActionTypeId){
            int brightness = action.param(colorBulbBrightnessActionBrightnessParamTypeId).value().toInt();
            qCDebug(dcSimulation()) << "Set brightness" << brightness << "for color bulb" << thing->name();
            thing->setStateValue(colorBulbBrightnessStateTypeId, brightness);
            return info->finish(Thing::ThingErrorNoError);

        } else if (action.actionTypeId() == colorBulbColorTemperatureActionTypeId){
            int temperature = action.param(colorBulbColorTemperatureActionColorTemperatureParamTypeId).value().toInt();
            qCDebug(dcSimulation()) << "Set color temperature" << temperature << "for color bulb" << thing->name();
            thing->setStateValue(colorBulbColorTemperatureStateTypeId, temperature);
            return info->finish(Thing::ThingErrorNoError);

        } else if (action.actionTypeId() == colorBulbColorActionTypeId) {
            QColor color = action.param(colorBulbColorActionColorParamTypeId).value().value<QColor>();
            qCDebug(dcSimulation()) << "Set color" << color << "for color bulb" << thing->name();
            thing->setStateValue(colorBulbColorStateTypeId, color);
            return info->finish(Thing::ThingErrorNoError);

        } else if (action.actionTypeId() == colorBulbPowerActionTypeId) {
            bool power = action.param(colorBulbPowerActionPowerParamTypeId).value().toBool();
            qCDebug(dcSimulation()) << "Set power" << power << "for color bulb" << thing->name();
            thing->setStateValue(colorBulbPowerStateTypeId, power);
            return info->finish(Thing::ThingErrorNoError);
        }

        return info->finish(Thing::ThingErrorActionTypeNotFound);
    }

    if (thing->thingClassId() == heatingRodThingClassId) {

        if (action.actionTypeId() == heatingRodPowerActionTypeId) {
            bool power = action.param(heatingRodPowerActionPowerParamTypeId).value().toBool();
            qCDebug(dcSimulation()) << "Set power" << power << "for heating rod" << thing->name();
            thing->setStateValue(heatingRodPowerStateTypeId, power);
            return info->finish(Thing::ThingErrorNoError);
        } else if (action.actionTypeId() == heatingRodPercentageActionTypeId) {
            int percentage = action.param(heatingRodPercentageActionPercentageParamTypeId).value().toInt();
            qCDebug(dcSimulation()) << "Set percentage" << percentage << "for heating rod" << thing->name();
            thing->setStateValue(heatingRodPercentageStateTypeId, percentage);
            return info->finish(Thing::ThingErrorNoError);
        }

        return info->finish(Thing::ThingErrorActionTypeNotFound);
    }

    if (thing->thingClassId() == batteryThingClassId) {
        if (action.actionTypeId() == batteryMaxChargingActionTypeId) {
            int maxCharging = action.param(batteryMaxChargingActionMaxChargingParamTypeId).value().toInt();
            thing->setStateValue(batteryMaxChargingStateTypeId, maxCharging);
            qCDebug(dcSimulation()) << "Set max charging power" << maxCharging << "for battery" << thing->name();
            thing->setStateValue(batteryChargingStateTypeId, true);
            return info->finish(Thing::ThingErrorNoError);
        }
        return info->finish(Thing::ThingErrorActionTypeNotFound);
    }

    if (thing->thingClassId() == waterValveThingClassId) {
        if (action.actionTypeId() == waterValvePowerActionTypeId) {
            bool power = action.param(waterValvePowerActionPowerParamTypeId).value().toBool();
            thing->setStateValue(waterValvePowerStateTypeId, power);
            return info->finish(Thing::ThingErrorNoError);
        }
        return info->finish(Thing::ThingErrorActionTypeNotFound);
    }

    if (thing->thingClassId() == garageGateThingClassId) {
        if (action.actionTypeId() == garageGateOpenActionTypeId) {
            if (thing->stateValue(garageGateStateStateTypeId).toString() == "opening") {
                qCDebug(dcSimulation()) << "Garage gate already opening.";
                return info->finish(Thing::ThingErrorNoError);
            }
            if (thing->stateValue(garageGateStateStateTypeId).toString() == "open" &&
                    !thing->stateValue(garageGateIntermediatePositionStateTypeId).toBool()) {
                qCDebug(dcSimulation()) << "Garage gate already open.";
                return info->finish(Thing::ThingErrorNoError);
            }
            thing->setStateValue(garageGateStateStateTypeId, "opening");
            thing->setStateValue(garageGateIntermediatePositionStateTypeId, true);
            m_simulationTimers.value(thing)->start(5000);
            return info->finish(Thing::ThingErrorNoError);
        }
        if (action.actionTypeId() == garageGateCloseActionTypeId) {
            if (thing->stateValue(garageGateStateStateTypeId).toString() == "closing") {
                qCDebug(dcSimulation()) << "Garage gate already closing.";
                return info->finish(Thing::ThingErrorNoError);
            }
            if (thing->stateValue(garageGateStateStateTypeId).toString() == "closed" &&
                    !thing->stateValue(garageGateIntermediatePositionStateTypeId).toBool()) {
                qCDebug(dcSimulation()) << "Garage gate already closed.";
                return info->finish(Thing::ThingErrorNoError);
            }
            thing->setStateValue(garageGateStateStateTypeId, "closing");
            thing->setStateValue(garageGateIntermediatePositionStateTypeId, true);
            m_simulationTimers.value(thing)->start(5000);
            return info->finish(Thing::ThingErrorNoError);
        }
        if (action.actionTypeId() == garageGateStopActionTypeId) {
            if (thing->stateValue(garageGateStateStateTypeId).toString() == "opening" ||
                    thing->stateValue(garageGateStateStateTypeId).toString() == "closing") {
                thing->setStateValue(garageGateStateStateTypeId, "open");
                return info->finish(Thing::ThingErrorNoError);
            }
            qCDebug(dcSimulation()) << "Garage gate not moving";
            return info->finish(Thing::ThingErrorNoError);
        }
        if (action.actionTypeId() == garageGatePowerActionTypeId) {
            bool power = action.param(garageGatePowerActionPowerParamTypeId).value().toBool();
            thing->setStateValue(garageGatePowerStateTypeId, power);
            return info->finish(Thing::ThingErrorNoError);
        }
    }

    if (thing->thingClassId() == rollerShutterThingClassId) {
        if (action.actionTypeId() == rollerShutterOpenActionTypeId) {
            qCDebug(dcSimulation()) << "Opening roller shutter";
            m_simulationTimers.value(thing)->setProperty("targetValue", 0);
            m_simulationTimers.value(thing)->start(500);
            thing->setStateValue(rollerShutterMovingStateTypeId, true);
            return info->finish(Thing::ThingErrorNoError);
        }
        if (action.actionTypeId() == rollerShutterCloseActionTypeId) {
            qCDebug(dcSimulation()) << "Closing roller shutter";
            m_simulationTimers.value(thing)->setProperty("targetValue", 100);
            m_simulationTimers.value(thing)->start(500);
            thing->setStateValue(rollerShutterMovingStateTypeId, true);
            return info->finish(Thing::ThingErrorNoError);
        }
        if (action.actionTypeId() == rollerShutterStopActionTypeId) {
            qCDebug(dcSimulation()) << "Stopping roller shutter";
            m_simulationTimers.value(thing)->stop();
            thing->setStateValue(rollerShutterMovingStateTypeId, false);
            return info->finish(Thing::ThingErrorNoError);
        }
        if (action.actionTypeId() == rollerShutterPercentageActionTypeId) {
            qCDebug(dcSimulation()) << "Setting awning to" << action.param(rollerShutterPercentageActionPercentageParamTypeId);
            m_simulationTimers.value(thing)->setProperty("targetValue", action.param(rollerShutterPercentageActionPercentageParamTypeId).value());
            m_simulationTimers.value(thing)->start(500);
            thing->setStateValue(rollerShutterMovingStateTypeId, true);
            return info->finish(Thing::ThingErrorNoError);
        }
    }

    if (thing->thingClassId() == extendedAwningThingClassId) {
        if (action.actionTypeId() == extendedAwningOpenActionTypeId) {
            qCDebug(dcSimulation()) << "Opening awning";
            m_simulationTimers.value(thing)->setProperty("targetValue", 100);
            m_simulationTimers.value(thing)->start(500);
            thing->setStateValue(extendedAwningMovingStateTypeId, true);
            return info->finish(Thing::ThingErrorNoError);
        }
        if (action.actionTypeId() == extendedAwningCloseActionTypeId) {
            qCDebug(dcSimulation()) << "Closing awning";
            m_simulationTimers.value(thing)->setProperty("targetValue", 0);
            m_simulationTimers.value(thing)->start(500);
            thing->setStateValue(extendedAwningMovingStateTypeId, true);
            return info->finish(Thing::ThingErrorNoError);
        }
        if (action.actionTypeId() == extendedAwningStopActionTypeId) {
            qCDebug(dcSimulation()) << "Stopping awning";
            m_simulationTimers.value(thing)->stop();
            thing->setStateValue(extendedAwningMovingStateTypeId, false);
            return info->finish(Thing::ThingErrorNoError);
        }
        if (action.actionTypeId() == extendedAwningPercentageActionTypeId) {
            qCDebug(dcSimulation()) << "Setting awning to" << action.param(extendedAwningPercentageActionPercentageParamTypeId);
            m_simulationTimers.value(thing)->setProperty("targetValue", action.param(extendedAwningPercentageActionPercentageParamTypeId).value());
            m_simulationTimers.value(thing)->start(500);
            thing->setStateValue(extendedAwningMovingStateTypeId, true);
            return info->finish(Thing::ThingErrorNoError);
        }
    }

    if (thing->thingClassId() == fingerPrintSensorThingClassId) {
        if (action.actionTypeId() == fingerPrintSensorAddUserActionTypeId) {
            QStringList users = thing->stateValue(fingerPrintSensorUsersStateTypeId).toStringList();
            QString username = action.param(fingerPrintSensorAddUserActionUserIdParamTypeId).value().toString();
            QString finger = action.param(fingerPrintSensorAddUserActionFingerParamTypeId).value().toString();
            QSettings settings;
            settings.beginGroup(thing->id().toString());
            QStringList usedFingers = settings.value(username).toStringList();
            if (users.contains(username) && usedFingers.contains(finger)) {
                return info->finish(Thing::ThingErrorDuplicateUuid);
            }
            QTimer::singleShot(5000, info, [this, info, thing, username, finger]() {
                if (username.toLower().trimmed() == "john") {
                    info->finish(Thing::ThingErrorHardwareFailure, QT_TR_NOOP("Fingerprint could not be scanned. Please try again."));
                } else {
                    info->finish(Thing::ThingErrorNoError);
                    QStringList users = thing->stateValue(fingerPrintSensorUsersStateTypeId).toStringList();
                    if (!users.contains(username)) {
                        users.append(username);
                        thing->setStateValue(fingerPrintSensorUsersStateTypeId, users);
                        m_simulationTimers.value(thing)->start(10000);
                    }

                    QSettings settings;
                    settings.beginGroup(thing->id().toString());
                    QStringList usedFingers = settings.value(username).toStringList();
                    usedFingers.append(finger);
                    settings.setValue(username, usedFingers);
                    settings.endGroup();
                }
            });
            return;
        }
        if (action.actionTypeId() == fingerPrintSensorRemoveUserActionTypeId) {
            QStringList users = thing->stateValue(fingerPrintSensorUsersStateTypeId).toStringList();
            QString username = action.params().first().value().toString();
            if (!users.contains(username)) {
                return info->finish(Thing::ThingErrorInvalidParameter);
            }
            users.removeAll(username);
            thing->setStateValue(fingerPrintSensorUsersStateTypeId, users);
            if (users.count() == 0) {
                m_simulationTimers.value(thing)->stop();
            }
            return info->finish(Thing::ThingErrorNoError);
        }
    }

    if (thing->thingClassId() == simpleBlindThingClassId) {
        if (action.actionTypeId() == simpleBlindOpenActionTypeId) {
            qCDebug(dcSimulation()) << "Opening simple blind";
            return info->finish(Thing::ThingErrorNoError);
        }
        if (action.actionTypeId() == simpleBlindCloseActionTypeId) {
            qCDebug(dcSimulation()) << "Closing simple blind";
            return info->finish(Thing::ThingErrorNoError);
        }
        if (action.actionTypeId() == simpleBlindStopActionTypeId) {
            qCDebug(dcSimulation()) << "Stopping simple blind";
            return info->finish(Thing::ThingErrorNoError);
        }
    }

    if (thing->thingClassId() == extendedBlindThingClassId) {
        if (action.actionTypeId() == extendedBlindOpenActionTypeId) {
            qCDebug(dcSimulation()) << "Opening extended blind";
            m_simulationTimers.value(thing)->setProperty("targetValue", 0);
            m_simulationTimers.value(thing)->start(500);
            thing->setStateValue(extendedBlindMovingStateTypeId, true);
            return info->finish(Thing::ThingErrorNoError);
        }
        if (action.actionTypeId() == extendedBlindCloseActionTypeId) {
            qCDebug(dcSimulation()) << "Closing extended blind";
            m_simulationTimers.value(thing)->setProperty("targetValue", 100);
            m_simulationTimers.value(thing)->start(500);
            thing->setStateValue(extendedBlindMovingStateTypeId, true);
            return info->finish(Thing::ThingErrorNoError);
        }
        if (action.actionTypeId() == extendedBlindStopActionTypeId) {
            qCDebug(dcSimulation()) << "Stopping extended blind";
            m_simulationTimers.value(thing)->stop();
            thing->setStateValue(extendedBlindMovingStateTypeId, false);
            return info->finish(Thing::ThingErrorNoError);
        }
        if (action.actionTypeId() == extendedBlindPercentageActionTypeId) {
            qCDebug(dcSimulation()) << "Setting extended blind to" << action.param(extendedBlindPercentageActionPercentageParamTypeId);
            m_simulationTimers.value(thing)->setProperty("targetValue", action.param(extendedBlindPercentageActionPercentageParamTypeId).value());
            m_simulationTimers.value(thing)->start(500);
            thing->setStateValue(extendedBlindMovingStateTypeId, true);
            return info->finish(Thing::ThingErrorNoError);
        }
    }

    if (thing->thingClassId() == venetianBlindThingClassId) {
        if (action.actionTypeId() == venetianBlindOpenActionTypeId) {
            qCDebug(dcSimulation()) << "Opening venetian blind";
            m_simulationTimers.value(thing)->setProperty("targetPosition", 0);
            m_simulationTimers.value(thing)->start(500);
            thing->setStateValue(venetianBlindMovingStateTypeId, true);
            return info->finish(Thing::ThingErrorNoError);
        }
        if (action.actionTypeId() == venetianBlindCloseActionTypeId) {
            qCDebug(dcSimulation()) << "Closing venetian blind";
            m_simulationTimers.value(thing)->setProperty("targetPosition", 100);
            m_simulationTimers.value(thing)->start(500);
            thing->setStateValue(venetianBlindMovingStateTypeId, true);
            return info->finish(Thing::ThingErrorNoError);
        }
        if (action.actionTypeId() == venetianBlindStopActionTypeId) {
            qCDebug(dcSimulation()) << "Stopping venetian blind";
            m_simulationTimers.value(thing)->stop();
            m_simulationTimers.value(thing)->setProperty("targetPosition", thing->stateValue(venetianBlindPercentageStateTypeId).toInt());
            m_simulationTimers.value(thing)->setProperty("targetAngle", thing->stateValue(venetianBlindAngleStateTypeId).toInt());
            thing->setStateValue(venetianBlindMovingStateTypeId, false);
            return info->finish(Thing::ThingErrorNoError);
        }
        if (action.actionTypeId() == venetianBlindPercentageActionTypeId) {
            qCDebug(dcSimulation()) << "Setting venetian blind position to" << action.param(venetianBlindPercentageActionPercentageParamTypeId);
            m_simulationTimers.value(thing)->setProperty("targetPosition", action.param(venetianBlindPercentageActionPercentageParamTypeId).value());
            m_simulationTimers.value(thing)->start(500);
            thing->setStateValue(venetianBlindMovingStateTypeId, true);
            return info->finish(Thing::ThingErrorNoError);
        }
        if (action.actionTypeId() == venetianBlindAngleActionTypeId) {
            qCDebug(dcSimulation()) << "Setting venetian blind angle to" << action.param(venetianBlindAngleActionAngleParamTypeId);
            m_simulationTimers.value(thing)->setProperty("targetAngle", action.param(venetianBlindAngleActionAngleParamTypeId).value());
            m_simulationTimers.value(thing)->start(500);
            thing->setStateValue(venetianBlindMovingStateTypeId, true);
            return info->finish(Thing::ThingErrorNoError);
        }
    }

    qCWarning(dcSimulation()) << "Unhandled thing class" << thing->thingClassId() << "for" << thing->name();
}

int IntegrationPluginSimulation::generateRandomIntValue(int min, int max)
{
    int value = ((qrand() % ((max + 1) - min)) + min);
    // qCDebug(dcSimulation()) << "Generateed random int value: [" << min << ", " << max << "] -->" << value;
    return value;
}

double IntegrationPluginSimulation::generateRandomDoubleValue(double min, double max)
{
    double value = generateRandomIntValue(static_cast<int>(min * 10), static_cast<int>(max * 10)) / 10.0;
    // qCDebug(dcSimulation()) << "Generated random double value: [" << min << ", " << max << "] -->" << value;
    return value;
}

bool IntegrationPluginSimulation::generateRandomBoolValue()
{
    bool value = static_cast<bool>(generateRandomIntValue(0, 1));
    // qCDebug(dcSimulation()) << "Generated random bool value:" << value;
    return value;
}

qreal IntegrationPluginSimulation::generateSinValue(int min, int max, int hourOffset, int decimals)
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

qreal IntegrationPluginSimulation::generateBatteryValue(int chargeStartHour, int chargeDurationInMinutes)
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

qreal IntegrationPluginSimulation::generateNoisyRectangle(int min, int max, int maxNoise, int stablePeriodInMinutes, int &lastValue, QDateTime &lastChangeTimestamp)
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

void IntegrationPluginSimulation::onPluginTimer20Seconds()
{
    foreach (Thing *thing, myThings()) {
        if (thing->thingClassId() == temperatureSensorThingClassId) {
            // Temperature sensor
            thing->setStateValue(temperatureSensorTemperatureStateTypeId, generateSinValue(18, 23, 8));
            thing->setStateValue(temperatureSensorHumidityStateTypeId, generateSinValue(40, 55, 20));
            thing->setStateValue(temperatureSensorBatteryLevelStateTypeId, generateBatteryValue(8, 10));
            thing->setStateValue(temperatureSensorBatteryCriticalStateTypeId, thing->stateValue(temperatureSensorBatteryLevelStateTypeId).toInt() <= 25);
            thing->setStateValue(temperatureSensorConnectedStateTypeId, true);
        } else if (thing->thingClassId() == motionDetectorThingClassId) {
            // Motion detector
            thing->setStateValue(motionDetectorIsPresentStateTypeId, generateRandomBoolValue());
            thing->setStateValue(motionDetectorBatteryLevelStateTypeId, generateBatteryValue(13, 1));
            thing->setStateValue(motionDetectorBatteryCriticalStateTypeId, thing->stateValue(motionDetectorBatteryLevelStateTypeId).toInt() <= 30);
            thing->setStateValue(motionDetectorConnectedStateTypeId, true);
        } else if (thing->thingClassId() == waterSensorThingClassId) {
            thing->setStateValue(waterSensorWaterDetectedStateTypeId, generateRandomBoolValue());
        } else if (thing->thingClassId() == gardenSensorThingClassId) {
            // Garden sensor
            thing->setStateValue(gardenSensorTemperatureStateTypeId, generateSinValue(-4, 17, 5));
            thing->setStateValue(gardenSensorSoilMoistureStateTypeId, generateSinValue(40, 60, 13));
            thing->setStateValue(gardenSensorIlluminanceStateTypeId, generateSinValue(0, 80, 2));
            thing->setStateValue(gardenSensorBatteryLevelStateTypeId, generateBatteryValue(9, 20));
            thing->setStateValue(gardenSensorBatteryCriticalStateTypeId, thing->stateValue(gardenSensorBatteryLevelStateTypeId).toDouble() <= 30);
            thing->setStateValue(gardenSensorConnectedStateTypeId, true);
        } else if(thing->thingClassId() == netatmoIndoorThingClassId) {
            // Netatmo
            thing->setStateValue(netatmoIndoorUpdateTimeStateTypeId, QDateTime::currentDateTime().toTime_t());
            thing->setStateValue(netatmoIndoorHumidityStateTypeId, generateSinValue(35, 45, 13));
            thing->setStateValue(netatmoIndoorTemperatureStateTypeId, generateSinValue(20, 25, 3));
            thing->setStateValue(netatmoIndoorPressureStateTypeId, generateSinValue(1003, 1008, 8));
            thing->setStateValue(netatmoIndoorNoiseStateTypeId, generateRandomIntValue(40, 80));
            thing->setStateValue(netatmoIndoorWifiStrengthStateTypeId, generateRandomIntValue(85, 95));
        } else if (thing->thingClassId() == smartMeterThingClassId) {
            thing->setStateValue(smartMeterConnectedStateTypeId, true);
            int lastValue = thing->property("lastValue").toInt();
            QDateTime lastUpdate = thing->property("lastUpdate").toDateTime();
            qlonglong currentPower = generateNoisyRectangle(-2000, 100, 10, 5, lastValue, lastUpdate);
            thing->setStateValue(smartMeterCurrentPowerStateTypeId, currentPower);
            thing->setProperty("lastValue", lastValue);
            thing->setProperty("lastUpdate", lastUpdate);
            if (currentPower < 0) {
                qreal consumptionKWH = 1.0 * currentPower * (1.0 * m_pluginTimer20Seconds->interval() / 1000 / 60 / 60) / 1000;
                thing->setStateValue(smartMeterTotalEnergyConsumedStateTypeId, thing->stateValue(smartMeterTotalEnergyConsumedStateTypeId).toDouble() - consumptionKWH);
            }
            if (currentPower > 0) {
                qreal consumptionKWH = 1.0 * currentPower * (1.0 * m_pluginTimer20Seconds->interval() / 1000 / 60 / 60) / 1000;
                thing->setStateValue(smartMeterTotalEnergyProducedStateTypeId, thing->stateValue(smartMeterTotalEnergyProducedStateTypeId).toDouble() + consumptionKWH);
            }
        } else if (thing->thingClassId() == solarPanelThingClassId) {
            int lastValue = thing->property("lastValue").toInt();
            QDateTime lastUpdate = thing->property("lastUpdate").toDateTime();
            qlonglong currentPower = generateNoisyRectangle(0, 2000, 50, 5, lastValue, lastUpdate);
            thing->setStateValue(solarPanelCurrentPowerStateTypeId, currentPower);
            thing->setProperty("lastValue", lastValue);
            thing->setProperty("lastUpdate", lastUpdate);
            qreal consumptionKWH = 1.0 * currentPower * (1.0 * m_pluginTimer20Seconds->interval() / 1000 / 60 / 60) / 1000;
            thing->setStateValue(solarPanelTotalEnergyProducedStateTypeId, thing->stateValue(solarPanelTotalEnergyProducedStateTypeId).toDouble() + consumptionKWH);
        }

    }
}

void IntegrationPluginSimulation::onPluginTimer5Minutes()
{
    foreach (Thing *thing, myThings()) {
        if(thing->thingClassId() == netatmoIndoorThingClassId) {
            // Note: should change between > 1000 co2 < 1000 for showcase, please do not change this behaviour
            int currentValue = thing->stateValue(netatmoIndoorCo2StateTypeId).toInt();
            if (currentValue < 1000) {
                thing->setStateValue(netatmoIndoorCo2StateTypeId, generateRandomIntValue(1001, 1010));
            } else {
                thing->setStateValue(netatmoIndoorCo2StateTypeId, generateRandomIntValue(950, 999));
            }
        }
    }
}

void IntegrationPluginSimulation::simulationTimerTimeout()
{
    QTimer *t = static_cast<QTimer*>(sender());
    Thing *thing = m_simulationTimers.key(t);
    if (thing->thingClassId() == garageGateThingClassId) {
        if (thing->stateValue(garageGateStateStateTypeId).toString() == "opening") {
            thing->setStateValue(garageGateIntermediatePositionStateTypeId, false);
            thing->setStateValue(garageGateStateStateTypeId, "open");
        }
        if (thing->stateValue(garageGateStateStateTypeId).toString() == "closing") {
            thing->setStateValue(garageGateIntermediatePositionStateTypeId, false);
            thing->setStateValue(garageGateStateStateTypeId, "closed");
        }
    } else if (thing->thingClassId() == extendedAwningThingClassId) {
        int currentValue = thing->stateValue(extendedAwningPercentageStateTypeId).toInt();
        int targetValue = t->property("targetValue").toInt();
        int newValue = targetValue > currentValue ? qMin(targetValue, currentValue + 5) : qMax(targetValue, currentValue - 5);
        thing->setStateValue(extendedAwningPercentageStateTypeId, newValue);
        if (newValue == targetValue) {
            t->stop();
            thing->setStateValue(extendedAwningMovingStateTypeId, false);
        }
    } else if (thing->thingClassId() == extendedBlindThingClassId) {
        int currentValue = thing->stateValue(extendedBlindPercentageStateTypeId).toInt();
        int targetValue = t->property("targetValue").toInt();
        int newValue = targetValue > currentValue ? qMin(targetValue, currentValue + 5) : qMax(targetValue, currentValue - 5);
        thing->setStateValue(extendedBlindPercentageStateTypeId, newValue);
        if (newValue == targetValue) {
            t->stop();
            thing->setStateValue(extendedBlindMovingStateTypeId, false);
        }
    } else if (thing->thingClassId() == venetianBlindThingClassId) {
        int targetPosition = t->property("targetPosition").toInt();
        int targetAngle = t->property("targetAngle").toInt();

        int currentPosition = thing->stateValue(venetianBlindPercentageStateTypeId).toInt();
        int currentAngle = thing->stateValue(venetianBlindAngleStateTypeId).toInt();

        int newPosition = targetPosition > currentPosition ? qMin(targetPosition, currentPosition + 5) : qMax(targetPosition, currentPosition - 5);
        thing->setStateValue(venetianBlindPercentageStateTypeId, newPosition);

        int newAngle = targetAngle > currentAngle ? qMin(targetAngle, currentAngle + 5) : qMax(targetAngle, currentAngle - 5);
        thing->setStateValue(venetianBlindAngleStateTypeId, newAngle);

        if (newPosition == targetPosition && newAngle == targetAngle) {
            t->stop();
            thing->setStateValue(venetianBlindMovingStateTypeId, false);
        }
    } else if (thing->thingClassId() == rollerShutterThingClassId) {
        int currentValue = thing->stateValue(rollerShutterPercentageStateTypeId).toInt();
        int targetValue = t->property("targetValue").toInt();
        int newValue = targetValue > currentValue ? qMin(targetValue, currentValue + 5) : qMax(targetValue, currentValue - 5);
        thing->setStateValue(rollerShutterPercentageStateTypeId, newValue);
        if (newValue == targetValue) {
            t->stop();
            thing->setStateValue(rollerShutterMovingStateTypeId, false);
        }
    } else if (thing->thingClassId() == fingerPrintSensorThingClassId) {
        EventTypeId evt = qrand() % 2 == 0 ? fingerPrintSensorAccessGrantedEventTypeId : fingerPrintSensorAccessDeniedEventTypeId;
        ParamList params;
        if (evt == fingerPrintSensorAccessGrantedEventTypeId) {
            QStringList users = thing->stateValue(fingerPrintSensorUsersStateTypeId).toStringList();
            QString user = users.at(qrand() % users.count());
            QSettings settings;
            settings.beginGroup(thing->id().toString());
            QStringList fingers = settings.value(user).toStringList();
            params.append(Param(fingerPrintSensorAccessGrantedEventUserIdParamTypeId, user));
            QString finger = fingers.at(qrand() % fingers.count());
            params.append(Param(fingerPrintSensorAccessGrantedEventFingerParamTypeId, finger));
            qCDebug(dcSimulation()) << "Emitting fingerprint accepted for user" << user << "and finger" << finger;
        } else {
            qCDebug(dcSimulation()) << "Emitting fingerprint denied";
        }
        Event event(evt, thing->id(), params);
        emitEvent(event);
    } else if (thing->thingClassId() == thermostatThingClassId) {
        thing->setStateValue(thermostatBoostStateTypeId, false);
        t->stop();
    } else if (thing->thingClassId() == barcodeScannerThingClassId) {
        QString code;
        int codeIndex = thing->property("codeIndex").toInt();
        switch (codeIndex) {
        case 0:
            code = "12345";
            thing->setProperty("codeIndex", 1);
            break;
        case 1:
            code = "23456";
            thing->setProperty("codeIndex", 2);
            break;
        default:
            code = "34567";
            thing->setProperty("codeIndex", 0);
            break;
        }

        ParamList params = ParamList() << Param(barcodeScannerCodeScannedEventContentParamTypeId, code);
        Event event(barcodeScannerCodeScannedEventTypeId, thing->id(), params);
        emit emitEvent(event);
    } else if (thing->thingClassId() == contactSensorThingClassId) {
       thing->setStateValue(contactSensorClosedStateTypeId, !thing->stateValue(contactSensorClosedStateTypeId).toBool());
       thing->setStateValue(contactSensorBatteryLevelStateTypeId, thing->stateValue(contactSensorBatteryLevelStateTypeId).toInt()-1);

       if (thing->stateValue(contactSensorBatteryLevelStateTypeId).toInt() == 0) {
           thing->setStateValue(contactSensorBatteryLevelStateTypeId, 100);
           thing->setStateValue(contactSensorBatteryCriticalStateTypeId, false);
       } else if (thing->stateValue(contactSensorBatteryLevelStateTypeId).toInt() <= 20) {
           thing->setStateValue(contactSensorBatteryCriticalStateTypeId, true);
       } else {
           thing->setStateValue(contactSensorBatteryCriticalStateTypeId, false);
       }
    } else if (thing->thingClassId() == waterSensorThingClassId) {
        bool wet = qrand() > (RAND_MAX / 2);
        thing->setStateValue(waterSensorWaterDetectedStateTypeId, wet);
    }
}
