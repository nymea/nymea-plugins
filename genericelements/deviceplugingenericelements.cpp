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
#include "devicemanager.h"
#include "plugininfo.h"

#include <QDebug>

DevicePluginGenericElements::DevicePluginGenericElements()
{
}

DeviceManager::DeviceSetupStatus DevicePluginGenericElements::setupDevice(Device *device)
{
    // Toggle Button
    if (device->deviceClassId() == toggleButtonDeviceClassId) {
        return DeviceManager::DeviceSetupStatusSuccess;
    }
    // Button
    if (device->deviceClassId() == buttonDeviceClassId) {
        return DeviceManager::DeviceSetupStatusSuccess;
    }
    // ON/OFF Button
    if (device->deviceClassId() == onOffButtonDeviceClassId) {
        return DeviceManager::DeviceSetupStatusSuccess;
    }
    return DeviceManager::DeviceSetupStatusFailure;
}

DeviceManager::DeviceError DevicePluginGenericElements::executeAction(Device *device, const Action &action)
{
    // Toggle Button
    if (device->deviceClassId() == toggleButtonDeviceClassId ) {
        if (action.actionTypeId() == toggleButtonStateActionTypeId) {
            device->setStateValue(toggleButtonStateStateTypeId, !device->stateValue(toggleButtonStateStateTypeId).toBool());
            return DeviceManager::DeviceErrorNoError;
        }
        return DeviceManager::DeviceErrorActionTypeNotFound;
    }
    // Button
    if (device->deviceClassId() == buttonDeviceClassId ) {
        if (action.actionTypeId() == buttonButtonPressActionTypeId) {
            emit emitEvent(Event(buttonButtonPressedEventTypeId, device->id()));
            return DeviceManager::DeviceErrorNoError;
        }
        return DeviceManager::DeviceErrorActionTypeNotFound;
    }
    // ON/OFF Button
    if (device->deviceClassId() == onOffButtonDeviceClassId ) {
        if (action.actionTypeId() == onOffButtonOnActionTypeId) {
            emit emitEvent(Event(onOffButtonOnEventTypeId, device->id()));
            return DeviceManager::DeviceErrorNoError;
        }
        if (action.actionTypeId() == onOffButtonOffActionTypeId) {
            emit emitEvent(Event(onOffButtonOffEventTypeId, device->id()));
            return DeviceManager::DeviceErrorNoError;
        }
        return DeviceManager::DeviceErrorActionTypeNotFound;
    }
    return DeviceManager::DeviceErrorDeviceClassNotFound;
}
