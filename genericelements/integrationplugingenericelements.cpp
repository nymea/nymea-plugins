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

#include "integrationplugingenericelements.h"
#include "plugininfo.h"

#include <QDebug>

IntegrationPluginGenericElements::IntegrationPluginGenericElements()
{
}

void IntegrationPluginGenericElements::setupThing(ThingSetupInfo *info)
{
    info->finish(Thing::ThingErrorNoError);
}

void IntegrationPluginGenericElements::executeAction(ThingActionInfo *info)
{
    Thing *thing = info->thing();
    Action action = info->action();

    // Power Button
    if (thing->thingClassId() == powerButtonThingClassId) {
        if (action.actionTypeId() == powerButtonPowerActionTypeId) {
            thing->setStateValue(powerButtonPowerStateTypeId, action.params().paramValue(powerButtonPowerActionPowerParamTypeId).toBool());
        }
    }

    // Button
    if (thing->thingClassId() == buttonThingClassId) {
        if (action.actionTypeId() == buttonPressActionTypeId) {
            emit emitEvent(Event(buttonPressedEventTypeId, thing->id()));
        }
    }

    // Trigger button
    if (thing->thingClassId() == triggerButtonThingClassId) {
        if (action.actionTypeId() == triggerButtonTriggerActionTypeId) {
            emit emitEvent(Event(triggerButtonPressedEventTypeId, thing->id()));
        }
    }

    // ON/OFF Button
    if (thing->thingClassId() == onOffButtonThingClassId) {
        if (action.actionTypeId() == onOffButtonOnActionTypeId) {
            emit emitEvent(Event(onOffButtonPressedEventTypeId, thing->id(), ParamList() << Param(onOffButtonPressedEventButtonNameParamTypeId, "On")));
        } else if (action.actionTypeId() == onOffButtonOffActionTypeId) {
            emit emitEvent(Event(onOffButtonPressedEventTypeId, thing->id(), ParamList() << Param(onOffButtonPressedEventButtonNameParamTypeId, "Off")));
        }
    }

    info->finish(Thing::ThingErrorNoError);
}
