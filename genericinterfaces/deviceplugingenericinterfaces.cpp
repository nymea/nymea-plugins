
/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *                                                                         *
 *  Copyright (C) 2019 Bernhard Trinnes <bernhard.trinnes@nymea.io         *
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
    Following JSON file contains the definition and the description of all available \l{DeviceClass}{DeviceClasses}
    and \l{Vendor}{Vendors} of this \l{DevicePlugin}.

    For more details how to read this JSON file please check out the documentation for \l{The plugin JSON File}.

    \quotefile plugins/deviceplugins/genericinterfaces/deviceplugingenericinterfaces.json
*/


#include "deviceplugingenericinterfaces.h"
#include "devices/devicemanager.h"
#include "plugininfo.h"

#include <QDebug>

DevicePluginGenericInterfaces::DevicePluginGenericInterfaces()
{
}

void DevicePluginGenericInterfaces::setupDevice(DeviceSetupInfo *info)
{
    info->finish(Device::DeviceErrorNoError);
}

void DevicePluginGenericInterfaces::executeAction(DeviceActionInfo *info)
{
    Device *device = info->device();
    Action action = info->action();

    if (device->deviceClassId() == awningDeviceClassId) {
        if (action.actionTypeId() == awningOpenActionTypeId) {
            device->setStateValue(awningStatusStateTypeId, "Opening");
            device->setStateValue(awningClosingOutputStateTypeId, false);
            device->setStateValue(awningOpeningOutputStateTypeId, true);
            return info->finish(Device::DeviceErrorNoError);
        }
        if (action.actionTypeId() == awningStopActionTypeId) {
            device->setStateValue(awningStatusStateTypeId, "Stopped");
            device->setStateValue(awningOpeningOutputStateTypeId, false);
            device->setStateValue(awningClosingOutputStateTypeId, false);
            return info->finish(Device::DeviceErrorNoError);
        }
        if (action.actionTypeId() == awningCloseActionTypeId) {
            device->setStateValue(awningStatusStateTypeId, "Closing");
            device->setStateValue(awningOpeningOutputStateTypeId, false);
            device->setStateValue(awningClosingOutputStateTypeId, true);
            return info->finish(Device::DeviceErrorNoError);
        }
        return info->finish(Device::DeviceErrorActionTypeNotFound);
    }

    if (device->deviceClassId() == blindDeviceClassId ) {
        if (action.actionTypeId() == blindOpenActionTypeId) {
            device->setStateValue(blindStatusStateTypeId, "Opening");
            device->setStateValue(blindClosingOutputStateTypeId, false);
            device->setStateValue(blindOpeningOutputStateTypeId, true);
            return info->finish(Device::DeviceErrorNoError);
        }
        if (action.actionTypeId() == blindStopActionTypeId) {
            device->setStateValue(blindStatusStateTypeId, "Stopped");
            device->setStateValue(blindOpeningOutputStateTypeId, false);
            device->setStateValue(blindClosingOutputStateTypeId, false);
            return info->finish(Device::DeviceErrorNoError);
        }
        if (action.actionTypeId() == blindCloseActionTypeId) {
            device->setStateValue(blindStatusStateTypeId, "Closing");
            device->setStateValue(blindOpeningOutputStateTypeId, false);
            device->setStateValue(blindClosingOutputStateTypeId, true);
            return info->finish(Device::DeviceErrorNoError);
        }
        return info->finish(Device::DeviceErrorActionTypeNotFound);
    }

    if (device->deviceClassId() == shutterDeviceClassId) {
        if (action.actionTypeId() == shutterOpenActionTypeId) {
            device->setStateValue(shutterStatusStateTypeId, "Opening");
            device->setStateValue(shutterClosingOutputStateTypeId, false);
            device->setStateValue(shutterOpeningOutputStateTypeId, true);
            return info->finish(Device::DeviceErrorNoError);
        }
        if (action.actionTypeId() == shutterStopActionTypeId) {
            device->setStateValue(shutterStatusStateTypeId, "Stopped");
            device->setStateValue(shutterOpeningOutputStateTypeId, false);
            device->setStateValue(shutterClosingOutputStateTypeId, false);
            return info->finish(Device::DeviceErrorNoError);
        }
        if (action.actionTypeId() == shutterCloseActionTypeId) {
            device->setStateValue(shutterStatusStateTypeId, "Closing");
            device->setStateValue(shutterOpeningOutputStateTypeId, false);
            device->setStateValue(shutterClosingOutputStateTypeId, true);
            return info->finish(Device::DeviceErrorNoError);
        }
        return info->finish(Device::DeviceErrorActionTypeNotFound);
    }

    if (device->deviceClassId() == socketDeviceClassId) {
        if (action.actionTypeId() == socketPowerActionTypeId) {
            device->setStateValue(socketPowerStateTypeId, action.param(socketPowerActionPowerParamTypeId).value());
            return info->finish(Device::DeviceErrorNoError);
        }
        return info->finish(Device::DeviceErrorActionTypeNotFound);
    }

    if (device->deviceClassId() == lightDeviceClassId) {
        if (action.actionTypeId() == lightPowerActionTypeId) {
            device->setStateValue(lightPowerStateTypeId, action.param(lightPowerActionPowerParamTypeId).value());
            return info->finish(Device::DeviceErrorNoError);
        }
        return info->finish(Device::DeviceErrorActionTypeNotFound);
    }

    if (device->deviceClassId() == heatingDeviceClassId) {
        if (action.actionTypeId() == heatingPowerActionTypeId) {
            device->setStateValue(heatingPowerStateTypeId, action.param(heatingPowerActionPowerParamTypeId).value());
            return info->finish(Device::DeviceErrorNoError);
        }
        return info->finish(Device::DeviceErrorActionTypeNotFound);
    }
    return info->finish(Device::DeviceErrorDeviceClassNotFound);
}
