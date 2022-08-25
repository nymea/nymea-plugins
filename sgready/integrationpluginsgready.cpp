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

#include "integrationpluginsgready.h"
#include "plugininfo.h"

IntegrationPluginSgReady::IntegrationPluginSgReady()
{

}

void IntegrationPluginSgReady::init()
{
    // TODO: Load possible system configurations for gpio pairs depending on well knwon platforms
}

void IntegrationPluginSgReady::discoverThings(ThingDiscoveryInfo *info)
{
    // Check if GPIOs are available on this platform
    if (!Gpio::isAvailable()) {
        qCWarning(dcSgReady()) << "There are no GPIOs available on this plattform";
        //: Error discovering SG ready GPIOs
        return info->finish(Thing::ThingErrorHardwareNotAvailable, QT_TR_NOOP("No GPIOs available on this system."));
    }

    // FIXME: provide once system configurations are loaded

    info->finish(Thing::ThingErrorNoError);
}

void IntegrationPluginSgReady::setupThing(ThingSetupInfo *info)
{
    Thing *thing = info->thing();
    qCDebug(dcSgReady()) << "Setup" << thing->name() << thing->params();

    // Check if GPIOs are available on this platform
    if (!Gpio::isAvailable()) {
        qCWarning(dcSgReady()) << "There are no GPIOs available on this plattform";
        //: Error discovering SG ready GPIOs
        return info->finish(Thing::ThingErrorHardwareNotAvailable, QT_TR_NOOP("No GPIOs found on this system."));
    }

    if (thing->thingClassId() == sgReadyInterfaceThingClassId) {
        if (m_sgReadyInterfaces.contains(thing)) {
            // Rreconfigure...
            SgReadyInterface *interface = m_sgReadyInterfaces.take(thing);
            delete interface;
            // Continue with normal setup...
        }

        int gpioNumber1 = thing->paramValue(sgReadyInterfaceThingGpioNumber1ParamTypeId).toUInt();
        int gpioNumber2 = thing->paramValue(sgReadyInterfaceThingGpioNumber2ParamTypeId).toUInt();
        bool gpioEnabled1 = thing->stateValue(sgReadyInterfaceGpio1StateStateTypeId).toBool();
        bool gpioEnabled2 = thing->stateValue(sgReadyInterfaceGpio2StateStateTypeId).toBool();

        SgReadyInterface *sgReadyInterface = new SgReadyInterface(gpioNumber1, gpioNumber2, this);
        if (!sgReadyInterface->setup(gpioEnabled1, gpioEnabled2)) {
            qCWarning(dcSgReady()) << "Setup" << thing << "failed because the GPIO could not be set up correctly.";
            //: Error message if SG ready GPIOs setup failed
            info->finish(Thing::ThingErrorHardwareFailure, QT_TR_NOOP("Failed to set up the GPIO hardware interface."));
            return;
        }

        // Reflect the SG states
        connect(sgReadyInterface, &SgReadyInterface::sgReadyModeChanged, this, [thing, sgReadyInterface](SgReadyInterface::SgReadyMode mode){
            Q_UNUSED(mode)
            thing->setStateValue(sgReadyInterfaceGpio1StateStateTypeId, sgReadyInterface->gpio1()->value() == Gpio::ValueHigh);
            thing->setStateValue(sgReadyInterfaceGpio2StateStateTypeId, sgReadyInterface->gpio2()->value() == Gpio::ValueHigh);
        });

        m_sgReadyInterfaces.insert(thing, sgReadyInterface);
        info->finish(Thing::ThingErrorNoError);
        return;
    }
}

void IntegrationPluginSgReady::postSetupThing(Thing *thing)
{
    Q_UNUSED(thing)
}

void IntegrationPluginSgReady::thingRemoved(Thing *thing)
{
    if (m_sgReadyInterfaces.contains(thing)) {
        SgReadyInterface *sgReadyInterface = m_sgReadyInterfaces.take(thing);
        delete sgReadyInterface;
    }
}

void IntegrationPluginSgReady::executeAction(ThingActionInfo *info)
{
    Thing *thing = info->thing();
    if (thing->thingClassId() == sgReadyInterfaceThingClassId) {

        SgReadyInterface *sgReadyInterface = m_sgReadyInterfaces.value(thing);
        if (!sgReadyInterface || !sgReadyInterface->isValid()) {
            qCWarning(dcSgReady()) << "Failed to execute action. There is no interface available for" << thing;
            info->finish(Thing::ThingErrorHardwareNotAvailable);
            return;
        }

        // FIXME: the modes have timing constrains we need to take care off.

        if (info->action().actionTypeId() == sgReadyInterfaceSgReadyModeActionTypeId) {
            QString sgReadyModeString = info->action().paramValue(sgReadyInterfaceSgReadyModeActionSgReadyModeParamTypeId).toString();
            qCDebug(dcSgReady()) << "Set SG ready mode from" << thing << "to" << sgReadyModeString;
            SgReadyInterface::SgReadyMode mode;
            if (sgReadyModeString == "Off") {
                mode = SgReadyInterface::SgReadyModeOff;
            } else if (sgReadyModeString == "Low") {
                mode = SgReadyInterface::SgReadyModeLow;
            } else if (sgReadyModeString == "High") {
                mode = SgReadyInterface::SgReadyModeHigh;
            } else {
                mode = SgReadyInterface::SgReadyModeStandard;
            }

            if (!sgReadyInterface->setSgReadyMode(mode)) {
                qCWarning(dcSgReady()) << "Failed to set the sg ready mode on" << thing << "to" << sgReadyModeString;
                info->finish(Thing::ThingErrorHardwareFailure);
                return;
            }

            info->finish(Thing::ThingErrorNoError);
        }
    }
}

