/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*
* Copyright 2013 - 2021, nymea GmbH
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
        bool initialValue = thing->stateValue(simpleHeatPumpInterfaceBoostStateTypeId).toBool();

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

        if (info->action().actionTypeId() == simpleHeatPumpInterfaceBoostActionTypeId) {
            bool boostEnabled = info->action().paramValue(simpleHeatPumpInterfaceBoostActionBoostParamTypeId).toBool();
            if (!gpio->setValue(boostEnabled ? Gpio::ValueHigh : Gpio::ValueLow)) {
                qCWarning(dcSimpleHeatPump()) << "Failed to set the boost mode for" << thing << "to" << boostEnabled;
                info->finish(Thing::ThingErrorHardwareFailure);
                return;
            }

            thing->setStateValue(simpleHeatPumpInterfaceBoostStateTypeId, boostEnabled);
            info->finish(Thing::ThingErrorNoError);
        }
    }
}
