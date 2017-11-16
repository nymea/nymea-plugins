/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *                                                                         *
 *  Copyright (C) 2015 Simon Stürz <simon.stuerz@guh.io>                   *
 *                                                                         *
 *  This file is part of guh.                                              *
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
    \page leynew.html
    \title Leynew
    \brief Plugin for the Leynew RF 433 MHz led controller.

    \ingroup plugins
    \ingroup guh-plugins

    This plugin allows to controll RF 433 MHz actors an receive remote signals from \l{http://www.leynew.com/en/}{Leynew}
    devices.

    Currently only following device is supported:

    \l{http://www.leynew.com/en/productview.asp?id=589}{http://www.leynew.com/en/productview.asp?id=589}

    \l{http://image.en.09635.com/2012-8/15/RF-Controller-Aluminum-Version-LN-CON-RF20B-H-3CH-LV-316.jpg}{http://image.en.09635.com/2012-8/15/RF-Controller-Aluminum-Version-LN-CON-RF20B-H-3CH-LV-316.jpg}

    \chapter Plugin properties
    Following JSON file contains the definition and the description of all available \l{DeviceClass}{DeviceClasses}
    and \l{Vendor}{Vendors} of this \l{DevicePlugin}.

    For more details how to read this JSON file please check out the documentation for \l{The plugin JSON File}.

    \quotefile plugins/deviceplugins/leynew/devicepluginleynew.json
*/

#include "devicepluginleynew.h"
#include "devicemanager.h"
#include "plugininfo.h"
#include "hardware/radio433/radio433.h"

#include <QDebug>
#include <QStringList>

DevicePluginLeynew::DevicePluginLeynew()
{
}

DeviceManager::DeviceSetupStatus DevicePluginLeynew::setupDevice(Device *device)
{
    Q_UNUSED(device);

    return DeviceManager::DeviceSetupStatusSuccess;
}

DeviceManager::DeviceError DevicePluginLeynew::executeAction(Device *device, const Action &action)
{   
    if (!hardwareManager()->radio433()->available()) {
        return DeviceManager::DeviceErrorHardwareNotAvailable;
    }

    if (device->deviceClassId() != rfControllerDeviceClassId) {
        return DeviceManager::DeviceErrorDeviceClassNotFound;
    }

    QList<int> rawData;
    QByteArray binCode;


    // TODO: find out how the id will be calculated to bin code or make it discoverable
    // =======================================
    // bincode depending on the id
    if (device->paramValue(rfControllerIdParamTypeId) == "0115"){
        binCode.append("001101000001");
    } else if (device->paramValue(rfControllerIdParamTypeId) == "0014") {
        binCode.append("110000010101");
    } else if (device->paramValue(rfControllerIdParamTypeId) == "0008") {
        binCode.append("111101010101");
    } else {
        qCWarning(dcLeynew) << "Could not get id of device: invalid parameter" << device->paramValue(rfControllerIdParamTypeId);
        return DeviceManager::DeviceErrorInvalidParameter;
    }

    int repetitions = 12;
    // =======================================
    // bincode depending on the action
    if (action.actionTypeId() == rfControllerBrightnessUpActionTypeId) {
        binCode.append("000000000011");
        repetitions = 8;
    } else if (action.actionTypeId() == rfControllerBrightnessDownActionTypeId) {
        binCode.append("000000001100");
        repetitions = 8;
    } else if (action.actionTypeId() == rfControllerPowerActionTypeId) {
        binCode.append("000011000000");
    } else if (action.actionTypeId() == rfControllerRedActionTypeId) {
        binCode.append("000000001111");
    } else if (action.actionTypeId() == rfControllerGreenActionTypeId) {
        binCode.append("000000110011");
    } else if (action.actionTypeId() == rfControllerBlueActionTypeId) {
        binCode.append("000011000011");
    } else if (action.actionTypeId() == rfControllerWhiteActionTypeId) {
        binCode.append("000000111100");
    } else if (action.actionTypeId() == rfControllerOrangeActionTypeId) {
        binCode.append("000011001100");
    } else if (action.actionTypeId() == rfControllerYellowActionTypeId) {
        binCode.append("000011110000");
    } else if (action.actionTypeId() == rfControllerCyanActionTypeId) {
        binCode.append("001100000011");
    } else if (action.actionTypeId() == rfControllerPurpleActionTypeId) {
        binCode.append("110000000011");
    } else if (action.actionTypeId() == rfControllerPlayPauseActionTypeId) {
        binCode.append("000000110000");
    } else if (action.actionTypeId() == rfControllerSpeedUpActionTypeId) {
        binCode.append("001100110000");
        repetitions = 8;
    } else if (action.actionTypeId() == rfControllerSpeedDownActionTypeId) {
        binCode.append("110000000000");
        repetitions = 8;
    } else if (action.actionTypeId() == rfControllerAutoActionTypeId) {
        binCode.append("001100001100");
    } else if (action.actionTypeId() == rfControllerFlashActionTypeId) {
        binCode.append("110011000000");
    } else if (action.actionTypeId() == rfControllerJump3ActionTypeId) {
        binCode.append("111100001100");
    } else if (action.actionTypeId() == rfControllerJump7ActionTypeId) {
        binCode.append("001111000000");
    } else if (action.actionTypeId() == rfControllerFade3ActionTypeId) {
        binCode.append("110000110000");
    } else if (action.actionTypeId() == rfControllerFade7ActionTypeId) {
        binCode.append("001100000000");
    } else {
        return DeviceManager::DeviceErrorActionTypeNotFound;
    }

    // =======================================
    //create rawData timings list
    int delay = 50;

    // sync signal (starting with ON)
    rawData.append(3);
    rawData.append(90);

    // add the code
    foreach (QChar c, binCode) {
        if(c == '0'){
            //   _
            //  | |_ _
            rawData.append(3);
            rawData.append(9);
        }else{
            //   _ _
            //  |   |_
            rawData.append(9);
            rawData.append(3);
        }
    }

    // =======================================
    // send data to hardware resource
    if(hardwareManager()->radio433()->sendData(delay, rawData, repetitions)){
        qCDebug(dcLeynew) << "Transmitted" << pluginName() << device->name() << action.id();
    }else{
        qCWarning(dcLeynew) << "Could not transmitt" << pluginName() << device->name() << action.id();
        return DeviceManager::DeviceErrorHardwareNotAvailable;
    }
    return DeviceManager::DeviceErrorNoError;
}
