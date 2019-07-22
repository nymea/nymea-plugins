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

#include "devicepluginunitec.h"
#include "plugininfo.h"
#include "hardwaremanager.h"
#include "hardware/radio433/radio433.h"

#include <QDebug>
#include <QStringList>

DevicePluginUnitec::DevicePluginUnitec()
{
}

Device::DeviceSetupStatus DevicePluginUnitec::setupDevice(Device *device)
{
    if (device->deviceClassId() != switchDeviceClassId) {
        return Device::DeviceSetupStatusFailure;
    }

    foreach (Device* d, myDevices()) {
        if (d->paramValue(switchDeviceChannelParamTypeId).toString() == device->paramValue(switchDeviceChannelParamTypeId).toString()) {
            qCWarning(dcUnitec) << "Unitec switch with channel " << device->paramValue(switchDeviceChannelParamTypeId).toString() << "already added.";
            return Device::DeviceSetupStatusFailure;
        }
    }

    return Device::DeviceSetupStatusSuccess;
}

Device::DeviceError DevicePluginUnitec::executeAction(Device *device, const Action &action)
{   
    if (!hardwareManager()->radio433()->available())
        return Device::DeviceErrorHardwareNotAvailable;

    QList<int> rawData;
    QByteArray binCode;

    if (action.actionTypeId() != switchPowerActionTypeId) {
        return Device::DeviceErrorActionTypeNotFound;
    }

    // Bin codes for buttons
    if (device->paramValue(switchDeviceChannelParamTypeId).toString() == "A" && action.param(switchPowerActionPowerParamTypeId).value().toBool() == true) {
        binCode.append("111011000100111010111111");
    } else if (device->paramValue(switchDeviceChannelParamTypeId).toString() == "A" && action.param(switchPowerActionPowerParamTypeId).value().toBool() == false) {
        binCode.append("111001100110100001011111");
    } else if (device->paramValue(switchDeviceChannelParamTypeId).toString() == "B" && action.param(switchPowerActionPowerParamTypeId).value().toBool() == true) {
        binCode.append("111011000100111010111011");
    } else if (device->paramValue(switchDeviceChannelParamTypeId).toString() == "B" && action.param(switchPowerActionPowerParamTypeId).value().toBool() == false) {
        binCode.append("111000111001100111101011");
    } else if (device->paramValue(switchDeviceChannelParamTypeId).toString() == "C" && action.param(switchPowerActionPowerParamTypeId).value().toBool() == true) {
        binCode.append("111000000011011111000011");
    } else if (device->paramValue(switchDeviceChannelParamTypeId).toString() == "C" && action.param(switchPowerActionPowerParamTypeId).value().toBool() == false) {
        binCode.append("111001100110100001010011");
    } else if (device->paramValue(switchDeviceChannelParamTypeId).toString() == "D" && action.param(switchPowerActionPowerParamTypeId).value().toBool() == true) {
        binCode.append("111001100110100001011101");
    } else if (device->paramValue(switchDeviceChannelParamTypeId).toString() == "D" && action.param(switchPowerActionPowerParamTypeId).value().toBool() == false) {
        binCode.append("111000000011011111001101");
    }

    // =======================================
    //create rawData timings list
    int delay = 500;

    // add sync code
    rawData.append(6);
    rawData.append(14);

    // add the code
    foreach (QChar c, binCode) {
        if(c == '0'){
            rawData.append(2);
            rawData.append(1);
        }else{
            rawData.append(1);
            rawData.append(2);
        }
    }

    // =======================================
    // send data to hardware resource
    if(hardwareManager()->radio433()->sendData(delay, rawData, 10)){
        qCDebug(dcUnitec) << "transmitted" << pluginName() << device->name() << "power: " << action.param(switchPowerActionPowerParamTypeId).value().toBool();
    }else{
        qCWarning(dcUnitec) << "could not transmitt" << pluginName() << device->name() << "power: " << action.param(switchPowerActionPowerParamTypeId).value().toBool();
        return Device::DeviceErrorHardwareNotAvailable;
    }

    return Device::DeviceErrorNoError;
}
