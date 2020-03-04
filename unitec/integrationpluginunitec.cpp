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

#include "integrationpluginunitec.h"
#include "plugininfo.h"
#include "hardwaremanager.h"
#include "hardware/radio433/radio433.h"

#include <QDebug>
#include <QStringList>

IntegrationPluginUnitec::IntegrationPluginUnitec()
{
}

void IntegrationPluginUnitec::setupThing(ThingSetupInfo *info)
{
    if (!hardwareManager()->radio433()->available())
        return info->finish(Thing::ThingErrorHardwareNotAvailable, QT_TR_NOOP("No 433 MHz radio available on this system."));

    Thing *thing = info->thing();

    foreach (Thing* d, myThings()) {
        if (d->paramValue(switchThingChannelParamTypeId).toString() == thing->paramValue(switchThingChannelParamTypeId).toString()) {
            qCWarning(dcUnitec) << "Unitec switch with channel " << thing->paramValue(switchThingChannelParamTypeId).toString() << "already added.";
            return info->finish(Thing::ThingErrorHardwareNotAvailable, QT_TR_NOOP("There is already a unitec switch on this channel"));
        }
    }

    return info->finish(Thing::ThingErrorNoError);
}

void IntegrationPluginUnitec::executeAction(ThingActionInfo *info)
{   
    if (!hardwareManager()->radio433()->available())
        return info->finish(Thing::ThingErrorHardwareNotAvailable, QT_TR_NOOP("No 433 MHz radio available on this system."));

    Thing *thing = info->thing();
    Action action = info->action();

    QList<int> rawData;
    QByteArray binCode;

    // Bin codes for buttons
    if (thing->paramValue(switchThingChannelParamTypeId).toString() == "A" && action.param(switchPowerActionPowerParamTypeId).value().toBool() == true) {
        binCode.append("111011000100111010111111");
    } else if (thing->paramValue(switchThingChannelParamTypeId).toString() == "A" && action.param(switchPowerActionPowerParamTypeId).value().toBool() == false) {
        binCode.append("111001100110100001011111");
    } else if (thing->paramValue(switchThingChannelParamTypeId).toString() == "B" && action.param(switchPowerActionPowerParamTypeId).value().toBool() == true) {
        binCode.append("111011000100111010111011");
    } else if (thing->paramValue(switchThingChannelParamTypeId).toString() == "B" && action.param(switchPowerActionPowerParamTypeId).value().toBool() == false) {
        binCode.append("111000111001100111101011");
    } else if (thing->paramValue(switchThingChannelParamTypeId).toString() == "C" && action.param(switchPowerActionPowerParamTypeId).value().toBool() == true) {
        binCode.append("111000000011011111000011");
    } else if (thing->paramValue(switchThingChannelParamTypeId).toString() == "C" && action.param(switchPowerActionPowerParamTypeId).value().toBool() == false) {
        binCode.append("111001100110100001010011");
    } else if (thing->paramValue(switchThingChannelParamTypeId).toString() == "D" && action.param(switchPowerActionPowerParamTypeId).value().toBool() == true) {
        binCode.append("111001100110100001011101");
    } else if (thing->paramValue(switchThingChannelParamTypeId).toString() == "D" && action.param(switchPowerActionPowerParamTypeId).value().toBool() == false) {
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
        qCDebug(dcUnitec) << "transmitted" << pluginName() << thing->name() << "power: " << action.param(switchPowerActionPowerParamTypeId).value().toBool();
    }else{
        qCWarning(dcUnitec) << "could not transmitt" << pluginName() << thing->name() << "power: " << action.param(switchPowerActionPowerParamTypeId).value().toBool();
        return info->finish(Thing::ThingErrorHardwareFailure, QT_TR_NOOP("Error sending data."));
    }

    return info->finish(Thing::ThingErrorNoError);
}
