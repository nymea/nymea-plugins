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

#include "integrationpluginleynew.h"
#include "plugininfo.h"
#include "hardware/radio433/radio433.h"

#include <QDebug>
#include <QStringList>

IntegrationPluginLeynew::IntegrationPluginLeynew()
{
}

void IntegrationPluginLeynew::setupThing(ThingSetupInfo *info)
{
    if (!hardwareManager()->radio433()->available()) {
        return info->finish(Thing::ThingErrorHardwareNotAvailable, QT_TR_NOOP("No 433 MHz radio available on this system."));
    }
}

void IntegrationPluginLeynew::executeAction(ThingActionInfo *info)
{   
    if (!hardwareManager()->radio433()->available()) {
        return info->finish(Thing::ThingErrorHardwareNotAvailable, QT_TR_NOOP("No 433 MHz radio available on this system."));
    }

    Thing *thing = info->thing();
    Action action = info->action();

    QList<int> rawData;
    QByteArray binCode;


    // TODO: find out how the id will be calculated to bin code or make it discoverable
    // =======================================
    // bincode depending on the id
    if (thing->paramValue(rfControllerThingIdParamTypeId) == "0115"){
        binCode.append("001101000001");
    } else if (thing->paramValue(rfControllerThingIdParamTypeId) == "0014") {
        binCode.append("110000010101");
    } else if (thing->paramValue(rfControllerThingIdParamTypeId) == "0008") {
        binCode.append("111101010101");
    } else {
        qCWarning(dcLeynew) << "Could not get id of thing: invalid parameter" << thing->paramValue(rfControllerThingIdParamTypeId);
        return info->finish(Thing::ThingErrorInvalidParameter);
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
        return info->finish(Thing::ThingErrorActionTypeNotFound);
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
        qCDebug(dcLeynew) << "Transmitted" << pluginName() << thing->name();
    }else{
        qCWarning(dcLeynew) << "Could not transmitt" << pluginName() << thing->name();
        return info->finish(Thing::ThingErrorHardwareFailure, QT_TR_NOOP("Error sending data."));
    }

    info->finish(Thing::ThingErrorNoError);
}
