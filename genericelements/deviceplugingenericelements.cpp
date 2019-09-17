/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *                                                                         *
 *  Copyright (C) 2015 Simon Stürz <simon.stuerz@guh.io>                   *
 *                                                                         *
 *  This file is part of nymea.                                            *
 *                                                                         *
 *  This library is free software; you can redistribute it and/or          *
 *  modify it under the terms of the GNU Lesser General Public             *
 *  License as published by the Free Software Foundation; either           *
 *  version 2.1 of the License, or (at your option) any later version.     *
 *                                                                         *
 *  This library is distributed in the hope that it will be useful,        *
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of         *
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU      *
 *  Lesser General Public License for more details.                        *
 *                                                                         *
 *  You should have received a copy of the GNU Lesser General Public       *
 *  License along with this library; If not, see                           *
 *  <http://www.gnu.org/licenses/>.                                        *
 *                                                                         *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

#include "deviceplugingenericelements.h"
#include "plugininfo.h"

#include <QDebug>

DevicePluginGenericElements::DevicePluginGenericElements()
{
}

void DevicePluginGenericElements::setupDevice(DeviceSetupInfo *info)
{
    info->finish(Device::DeviceErrorNoError);
}

void DevicePluginGenericElements::executeAction(DeviceActionInfo *info)
{
    Device *device = info->device();
    Action action = info->action();

    // Toggle Button
    if (action.actionTypeId() == toggleButtonStateActionTypeId) {
        device->setStateValue(toggleButtonStateStateTypeId, action.params().paramValue(toggleButtonStateActionStateParamTypeId).toBool());
    }
    // Button
    if (action.actionTypeId() == buttonButtonPressActionTypeId) {
        emit emitEvent(Event(buttonButtonPressedEventTypeId, device->id()));
    }
    // ON/OFF Button
    if (action.actionTypeId() == onOffButtonOnActionTypeId) {
        emit emitEvent(Event(onOffButtonOnEventTypeId, device->id()));
    }
    if (action.actionTypeId() == onOffButtonOffActionTypeId) {
        emit emitEvent(Event(onOffButtonOffEventTypeId, device->id()));
    }
    info->finish(Device::DeviceErrorNoError);
}
