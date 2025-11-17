// SPDX-License-Identifier: GPL-3.0-or-later

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*
* Copyright (C) 2013 - 2024, nymea GmbH
* Copyright (C) 2024 - 2025, chargebyte austria GmbH
*
* This file is part of nymea-plugins.
*
* nymea-plugins is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* (at your option) any later version.
*
* nymea-plugins is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
* General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with nymea-plugins. If not, see <https://www.gnu.org/licenses/>.
*
* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

#include "integrationpluginsimpleheatpump.h"
#include "plugininfo.h"

IntegrationPluginSimpleHeatpump::IntegrationPluginSimpleHeatpump()
{

}

void IntegrationPluginSimpleHeatpump::init()
{

}

void IntegrationPluginSimpleHeatpump::discoverThings(ThingDiscoveryInfo *info)
{
    // Check if GPIOs are available on this platform
    if (!Gpio::isAvailable()) {
        qCWarning(dcSimpleHeatPump()) << "There are no GPIOs available on this plattform";
        //: Error discovering GPIOs
        return info->finish(Thing::ThingErrorHardwareNotAvailable, QT_TR_NOOP("No GPIOs available on this system."));
    }

    // FIXME: provide once system configurations are loaded

    info->finish(Thing::ThingErrorNoError);
}

void IntegrationPluginSimpleHeatpump::setupThing(ThingSetupInfo *info)
{
    Thing *thing = info->thing();
    qCDebug(dcSimpleHeatPump()) << "Setup" << thing->name() << thing->params();

    if (thing->thingClassId() == simpleHeatPumpInterfaceThingClassId) {
        // Check if GPIOs are available on this platform
        if (!Gpio::isAvailable()) {
            qCWarning(dcSimpleHeatPump()) << "There are no GPIOs available on this plattform";
            //: Error set up GPIOs
            return info->finish(Thing::ThingErrorHardwareNotAvailable, QT_TR_NOOP("No GPIOs found on this system."));
        }

        int gpioNumber = thing->paramValue(simpleHeatPumpInterfaceThingGpioNumberParamTypeId).toInt();
        bool initialValue = thing->stateValue(simpleHeatPumpInterfacePowerStateTypeId).toBool();

        if (gpioNumber < 0) {
            qCWarning(dcSimpleHeatPump()) << "Invalid GPIO number" << gpioNumber;
            info->finish(Thing::ThingErrorInvalidParameter);
            return;
        }

        Gpio *gpio = new Gpio(gpioNumber, this);
        if (!gpio->exportGpio()) {
            qCWarning(dcSimpleHeatPump()) << "Could not export gpio" << gpioNumber;
            gpio->deleteLater();
            return info->finish(Thing::ThingErrorHardwareFailure, QT_TR_NOOP("Failed to set up the GPIO interface."));
        }

        if (!gpio->setDirection(Gpio::DirectionOutput)) {
            qCWarning(dcSimpleHeatPump()) << "Failed to configure gpio" << gpioNumber << "as output";
            gpio->deleteLater();
            return info->finish(Thing::ThingErrorHardwareFailure, QT_TR_NOOP("Failed to set up the GPIO interface."));
        }

        // Set the pin to the initial value
        if (!gpio->setValue(initialValue ? Gpio::ValueHigh : Gpio::ValueLow)) {
            qCWarning(dcSimpleHeatPump()) << "Failed to set initially value" << initialValue << "for gpio" << gpioNumber;
            gpio->deleteLater();
            return info->finish(Thing::ThingErrorHardwareFailure, QT_TR_NOOP("Failed to set up the GPIO interface."));
        }

        m_gpios.insert(thing, gpio);
        info->finish(Thing::ThingErrorNoError);
    }
}

void IntegrationPluginSimpleHeatpump::postSetupThing(Thing *thing)
{
    Q_UNUSED(thing)
}

void IntegrationPluginSimpleHeatpump::thingRemoved(Thing *thing)
{
    if (m_gpios.contains(thing)) {
        delete m_gpios.take(thing);
    }
}

void IntegrationPluginSimpleHeatpump::executeAction(ThingActionInfo *info)
{
    Thing *thing = info->thing();

    if (thing->thingClassId() == simpleHeatPumpInterfaceThingClassId) {
        Gpio *gpio = m_gpios.value(info->thing());
        if (!gpio) {
            qCWarning(dcSimpleHeatPump()) << "Failed to execute action. There is no GPIO available for" << thing;
            info->finish(Thing::ThingErrorHardwareNotAvailable);
            return;
        }

        if (info->action().actionTypeId() == simpleHeatPumpInterfacePowerActionTypeId) {
            bool heatPumpEnabled = info->action().paramValue(simpleHeatPumpInterfacePowerActionPowerParamTypeId).toBool();
            if (!gpio->setValue(heatPumpEnabled ? Gpio::ValueHigh : Gpio::ValueLow)) {
                qCWarning(dcSimpleHeatPump()) << "Failed to set the power for" << thing << "to" << heatPumpEnabled;
                info->finish(Thing::ThingErrorHardwareFailure);
                return;
            }

            thing->setStateValue(simpleHeatPumpInterfacePowerStateTypeId, heatPumpEnabled);
            info->finish(Thing::ThingErrorNoError);
        }
    }
}
