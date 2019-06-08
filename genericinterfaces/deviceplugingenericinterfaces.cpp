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
#include "devicemanager.h"
#include "plugininfo.h"

#include <QDebug>

DevicePluginGenericInterfaces::DevicePluginGenericInterfaces()
{
}

DeviceManager::DeviceSetupStatus DevicePluginGenericInterfaces::setupDevice(Device *device)
{
    if (device->deviceClassId() == awningDeviceClassId) {
        return DeviceManager::DeviceSetupStatusSuccess;
    }

    if (device->deviceClassId() == blindDeviceClassId) {
        return DeviceManager::DeviceSetupStatusSuccess;
    }

    if (device->deviceClassId() == shutterDeviceClassId) {
        return DeviceManager::DeviceSetupStatusSuccess;
    }

    if (device->deviceClassId() == socketDeviceClassId) {
        return DeviceManager::DeviceSetupStatusSuccess;
    }

    if (device->deviceClassId() == lightDeviceClassId) {
        return DeviceManager::DeviceSetupStatusSuccess;
    }

    if (device->deviceClassId() == heatingDeviceClassId) {
        return DeviceManager::DeviceSetupStatusSuccess;
    }
    return DeviceManager::DeviceSetupStatusFailure;
}

DeviceManager::DeviceError DevicePluginGenericInterfaces::executeAction(Device *device, const Action &action)
{
    if (device->deviceClassId() == awningDeviceClassId) {
        if (action.actionTypeId() == awningOpenActionTypeId) {
            return DeviceManager::DeviceErrorNoError;
        }
        if (action.actionTypeId() == awningStopActionTypeId) {
            return DeviceManager::DeviceErrorNoError;
        }
        if (action.actionTypeId() == awningCloseActionTypeId) {
            return DeviceManager::DeviceErrorNoError;
        }
        return DeviceManager::DeviceErrorActionTypeNotFound;
    }

    if (device->deviceClassId() == blindDeviceClassId ) {
        if (action.actionTypeId() == blindOpenActionTypeId) {
            return DeviceManager::DeviceErrorNoError;
        }
        if (action.actionTypeId() == blindStopActionTypeId) {
            return DeviceManager::DeviceErrorNoError;
        }
        if (action.actionTypeId() == blindCloseActionTypeId) {
            return DeviceManager::DeviceErrorNoError;
        }
        return DeviceManager::DeviceErrorActionTypeNotFound;
    }

    if (device->deviceClassId() == shutterDeviceClassId) {
        if (action.actionTypeId() == shutterOpenActionTypeId) {
            return DeviceManager::DeviceErrorNoError;
        }
        if (action.actionTypeId() == shutterStopActionTypeId) {
            return DeviceManager::DeviceErrorNoError;
        }
        if (action.actionTypeId() == shutterCloseActionTypeId) {
            return DeviceManager::DeviceErrorNoError;
        }
        return DeviceManager::DeviceErrorActionTypeNotFound;
    }

    if (device->deviceClassId() == awningDeviceClassId) {
        if (action.actionTypeId() == awningOpenActionTypeId) {
            return DeviceManager::DeviceErrorNoError;
        }
        if (action.actionTypeId() == awningStopActionTypeId) {
            return DeviceManager::DeviceErrorNoError;
        }
        if (action.actionTypeId() == awningCloseActionTypeId) {
            return DeviceManager::DeviceErrorNoError;
        }
        return DeviceManager::DeviceErrorActionTypeNotFound;
    }

    if (device->deviceClassId() == socketDeviceClassId) {
        if (action.actionTypeId() == socketPowerActionTypeId) {
            return DeviceManager::DeviceErrorNoError;
        }
        return DeviceManager::DeviceErrorActionTypeNotFound;
    }

    if (device->deviceClassId() == lightDeviceClassId) {
        if (action.actionTypeId() == lightPowerActionTypeId) {
            return DeviceManager::DeviceErrorNoError;
        }
        return DeviceManager::DeviceErrorActionTypeNotFound;
    }

    if (device->deviceClassId() == heatingDeviceClassId) {
        if (action.actionTypeId() == heatingPowerActionTypeId) {
            return DeviceManager::DeviceErrorNoError;
        }
        return DeviceManager::DeviceErrorActionTypeNotFound;
    }
    return DeviceManager::DeviceErrorDeviceClassNotFound;
}
