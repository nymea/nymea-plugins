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
