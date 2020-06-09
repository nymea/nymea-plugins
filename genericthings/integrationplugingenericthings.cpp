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

/*!
    \page genericinterfaces.html
    \title Generic interfaces
    \brief Common interfaces to test the rule engine.

    \ingroup plugins
    \ingroup nymea-tests

    The generic interfaces plugin allows you create virtual buttons, which can be connected with a rule. This gives you
    the possibility to execute multiple \l{Action}{Actions} with one signal. Without a rule this generic interfaces are
    useless.

    \chapter Plugin properties
    Following JSON file contains the definition and the description of all available \l{ThingClass}{DeviceClasses}
    and \l{Vendor}{Vendors} of this \l{DevicePlugin}.

    For more details how to read this JSON file please check out the documentation for \l{The plugin JSON File}.

    \quotefile plugins/deviceplugins/genericinterfaces/deviceplugingenericinterfaces.json
*/


#include "integrationplugingenericthings.h"
#include "plugininfo.h"

#include <QDebug>
#include <QtMath>

IntegrationPluginGenericThings::IntegrationPluginGenericThings()
{

}

void IntegrationPluginGenericThings::setupThing(ThingSetupInfo *info)
{
    info->finish(Thing::ThingErrorNoError);
}

void IntegrationPluginGenericThings::executeAction(ThingActionInfo *info)
{
    Thing *thing = info->thing();
    Action action = info->action();

    if (thing->thingClassId() == awningThingClassId) {
        if (action.actionTypeId() == awningOpenActionTypeId) {
            thing->setStateValue(awningStatusStateTypeId, "Opening");
            thing->setStateValue(awningClosingOutputStateTypeId, false);
            thing->setStateValue(awningOpeningOutputStateTypeId, true);
            return info->finish(Thing::ThingErrorNoError);
        }
        if (action.actionTypeId() == awningStopActionTypeId) {
            thing->setStateValue(awningStatusStateTypeId, "Stopped");
            thing->setStateValue(awningOpeningOutputStateTypeId, false);
            thing->setStateValue(awningClosingOutputStateTypeId, false);
            return info->finish(Thing::ThingErrorNoError);
        }
        if (action.actionTypeId() == awningCloseActionTypeId) {
            thing->setStateValue(awningStatusStateTypeId, "Closing");
            thing->setStateValue(awningOpeningOutputStateTypeId, false);
            thing->setStateValue(awningClosingOutputStateTypeId, true);
            return info->finish(Thing::ThingErrorNoError);
        }
        if (action.actionTypeId() == awningOpeningOutputActionTypeId) {
            bool on = action.param(awningOpeningOutputActionOpeningOutputParamTypeId).value().toBool();
            thing->setStateValue(awningOpeningOutputStateTypeId, on);
            if (on) {
                thing->setStateValue(awningStatusStateTypeId, "Opening");
                thing->setStateValue(awningClosingOutputStateTypeId, false);
            } else {
                thing->setStateValue(awningStatusStateTypeId, "Stopped");
            }
            info->finish(Thing::ThingErrorNoError);
        }
        if (action.actionTypeId() == awningClosingOutputActionTypeId) {
            bool on = action.param(awningClosingOutputActionClosingOutputParamTypeId).value().toBool();
            thing->setStateValue(awningClosingOutputStateTypeId, on);
            if (on) {
                thing->setStateValue(awningStatusStateTypeId, "Closing");
                thing->setStateValue(awningOpeningOutputStateTypeId, false);
            } else {
                thing->setStateValue(awningStatusStateTypeId, "Stopped");
            }
            info->finish(Thing::ThingErrorNoError);
        }
        return info->finish(Thing::ThingErrorActionTypeNotFound);
    }

    if (thing->thingClassId() == blindThingClassId ) {
        if (action.actionTypeId() == blindOpenActionTypeId) {
            thing->setStateValue(blindStatusStateTypeId, "Opening");
            thing->setStateValue(blindClosingOutputStateTypeId, false);
            thing->setStateValue(blindOpeningOutputStateTypeId, true);
            return info->finish(Thing::ThingErrorNoError);
        }
        if (action.actionTypeId() == blindStopActionTypeId) {
            thing->setStateValue(blindStatusStateTypeId, "Stopped");
            thing->setStateValue(blindOpeningOutputStateTypeId, false);
            thing->setStateValue(blindClosingOutputStateTypeId, false);
            return info->finish(Thing::ThingErrorNoError);
        }
        if (action.actionTypeId() == blindCloseActionTypeId) {
            thing->setStateValue(blindStatusStateTypeId, "Closing");
            thing->setStateValue(blindOpeningOutputStateTypeId, false);
            thing->setStateValue(blindClosingOutputStateTypeId, true);
            return info->finish(Thing::ThingErrorNoError);
        }
        if (action.actionTypeId() == blindOpeningOutputActionTypeId) {
            bool on = action.param(blindOpeningOutputActionOpeningOutputParamTypeId).value().toBool();
            thing->setStateValue(blindOpeningOutputStateTypeId, on);
            if (on) {
                thing->setStateValue(blindStatusStateTypeId, "Opening");
                thing->setStateValue(blindClosingOutputStateTypeId, false);
            } else {
                thing->setStateValue(blindStatusStateTypeId, "Stopped");
            }
            info->finish(Thing::ThingErrorNoError);
        }
        if (action.actionTypeId() == blindClosingOutputActionTypeId) {
            bool on = action.param(blindClosingOutputActionClosingOutputParamTypeId).value().toBool();
            thing->setStateValue(blindClosingOutputStateTypeId, on);
            if (on) {
                thing->setStateValue(blindStatusStateTypeId, "Closing");
                thing->setStateValue(blindOpeningOutputStateTypeId, false);
            } else {
                thing->setStateValue(blindStatusStateTypeId, "Stopped");
            }
            info->finish(Thing::ThingErrorNoError);
        }
        return info->finish(Thing::ThingErrorActionTypeNotFound);
    }

    if (thing->thingClassId() == shutterThingClassId) {
        if (action.actionTypeId() == shutterOpenActionTypeId) {
            thing->setStateValue(shutterStatusStateTypeId, "Opening");
            thing->setStateValue(shutterClosingOutputStateTypeId, false);
            thing->setStateValue(shutterOpeningOutputStateTypeId, true);
            return info->finish(Thing::ThingErrorNoError);
        }
        if (action.actionTypeId() == shutterStopActionTypeId) {
            thing->setStateValue(shutterStatusStateTypeId, "Stopped");
            thing->setStateValue(shutterOpeningOutputStateTypeId, false);
            thing->setStateValue(shutterClosingOutputStateTypeId, false);
            return info->finish(Thing::ThingErrorNoError);
        }
        if (action.actionTypeId() == shutterCloseActionTypeId) {
            thing->setStateValue(shutterStatusStateTypeId, "Closing");
            thing->setStateValue(shutterOpeningOutputStateTypeId, false);
            thing->setStateValue(shutterClosingOutputStateTypeId, true);
            return info->finish(Thing::ThingErrorNoError);
        }
        if (action.actionTypeId() == shutterOpeningOutputActionTypeId) {
            bool on = action.param(shutterOpeningOutputActionOpeningOutputParamTypeId).value().toBool();
            thing->setStateValue(shutterOpeningOutputStateTypeId, on);
            if (on) {
                thing->setStateValue(shutterStatusStateTypeId, "Opening");
                thing->setStateValue(shutterClosingOutputStateTypeId, false);
            } else {
                thing->setStateValue(shutterStatusStateTypeId, "Stopped");
            }
            info->finish(Thing::ThingErrorNoError);
        }
        if (action.actionTypeId() == shutterClosingOutputActionTypeId) {
            bool on = action.param(shutterClosingOutputActionClosingOutputParamTypeId).value().toBool();
            thing->setStateValue(shutterClosingOutputStateTypeId, on);
            if (on) {
                thing->setStateValue(shutterStatusStateTypeId, "Closing");
                thing->setStateValue(shutterOpeningOutputStateTypeId, false);
            } else {
                thing->setStateValue(shutterStatusStateTypeId, "Stopped");
            }
            info->finish(Thing::ThingErrorNoError);
        }
        return info->finish(Thing::ThingErrorActionTypeNotFound);
    }

    if (thing->thingClassId() == socketThingClassId) {
        if (action.actionTypeId() == socketPowerActionTypeId) {
            thing->setStateValue(socketPowerStateTypeId, action.param(socketPowerActionPowerParamTypeId).value());
            return info->finish(Thing::ThingErrorNoError);
        }
        return info->finish(Thing::ThingErrorActionTypeNotFound);
    }

    if (thing->thingClassId() == lightThingClassId) {
        if (action.actionTypeId() == lightPowerActionTypeId) {
            thing->setStateValue(lightPowerStateTypeId, action.param(lightPowerActionPowerParamTypeId).value());
            return info->finish(Thing::ThingErrorNoError);
        }
        return info->finish(Thing::ThingErrorActionTypeNotFound);
    }

    if (thing->thingClassId() == heatingThingClassId) {
        if (action.actionTypeId() == heatingPowerActionTypeId) {
            thing->setStateValue(heatingPowerStateTypeId, action.param(heatingPowerActionPowerParamTypeId).value());
            return info->finish(Thing::ThingErrorNoError);
        }
        return info->finish(Thing::ThingErrorActionTypeNotFound);
    }

    if (thing->thingClassId() == powerSwitchThingClassId) {
        if (action.actionTypeId() == powerSwitchPowerActionTypeId) {
            thing->setStateValue(powerSwitchPowerStateTypeId, action.param(powerSwitchPowerActionPowerParamTypeId).value());
            info->finish(Thing::ThingErrorNoError);
            return;
        }
    }

    if (thing->thingClassId() == irrigationThingClassId) {
        if (action.actionTypeId() == irrigationPowerActionTypeId) {
            thing->setStateValue(irrigationPowerStateTypeId, action.param(irrigationPowerActionPowerParamTypeId).value());
            info->finish(Thing::ThingErrorNoError);
            return;
        }
    }

    if (thing->thingClassId() == ventilationThingClassId) {
        if (action.actionTypeId() == ventilationPowerActionTypeId) {
            thing->setStateValue(ventilationPowerStateTypeId, action.param(ventilationPowerActionPowerParamTypeId).value());
            info->finish(Thing::ThingErrorNoError);
            return;
        }
    }

    if (thing->thingClassId() == temperatureSensorThingClassId) {
        if (action.actionTypeId() == temperatureSensorInputActionTypeId) {
            double value = info->action().param(temperatureSensorInputActionInputParamTypeId).value().toDouble();
            thing->setStateValue(temperatureSensorInputStateTypeId, value);
            double min = info->thing()->setting(temperatureSensorSettingsMinTempParamTypeId).toDouble();
            double max = info->thing()->setting(temperatureSensorSettingsMaxTempParamTypeId).toDouble();
            double newValue = mapDoubleValue(value, 0, 1, min, max);
            double roundingFactor = qPow(10, info->thing()->setting(temperatureSensorSettingsAccuracyParamTypeId).toInt());
            newValue = qRound(newValue * roundingFactor) / roundingFactor;
            thing->setStateValue(temperatureSensorTemperatureStateTypeId, newValue);
            info->finish(Thing::ThingErrorNoError);
            return;
        }
    }

    if (thing->thingClassId() == humiditySensorThingClassId) {
        if (action.actionTypeId() == humiditySensorInputActionTypeId) {
            double value = info->action().param(humiditySensorInputActionInputParamTypeId).value().toDouble();
            thing->setStateValue(humiditySensorInputStateTypeId, value);
            double min = info->thing()->setting(humiditySensorSettingsMinHumidityParamTypeId).toDouble();
            double max = info->thing()->setting(humiditySensorSettingsMaxHumidityParamTypeId).toDouble();
            double newValue = mapDoubleValue(value, 0, 100, min, max);
            double roundingFactor = qPow(10, info->thing()->setting(humiditySensorSettingsAccuracyParamTypeId).toInt());
            newValue = qRound(newValue * roundingFactor) / roundingFactor;
            thing->setStateValue(humiditySensorHumidityStateTypeId, newValue);
            info->finish(Thing::ThingErrorNoError);
            return;
        }
    }

    if (thing->thingClassId() == moistureSensorThingClassId) {
        if (action.actionTypeId() == moistureSensorInputActionTypeId) {
            double value = info->action().param(moistureSensorInputActionInputParamTypeId).value().toDouble();
            thing->setStateValue(moistureSensorInputStateTypeId, value);
            double min = info->thing()->setting(moistureSensorSettingsMinMoistureParamTypeId).toDouble();
            double max = info->thing()->setting(moistureSensorSettingsMaxMoistureParamTypeId).toDouble();
            double newValue = mapDoubleValue(value, 0, 100, min, max);
            double roundingFactor = qPow(10, info->thing()->setting(moistureSensorSettingsAccuracyParamTypeId).toInt());
            newValue = qRound(newValue * roundingFactor) / roundingFactor;
            thing->setStateValue(moistureSensorMoistureStateTypeId, newValue);
            info->finish(Thing::ThingErrorNoError);
            return;
        }
    }

    return info->finish(Thing::ThingErrorThingClassNotFound);
}

double IntegrationPluginGenericThings::mapDoubleValue(double value, double fromMin, double fromMax, double toMin, double toMax)
{
    double percent = (value - fromMin) / (fromMax - fromMin);
    double toValue = toMin + (toMax - toMin) * percent;
    return toValue;
}
