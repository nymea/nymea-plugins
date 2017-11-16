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
    \page elro.html
    \title Elro
    \brief Plugin for all Elro 433 MHz based devices.

    \ingroup plugins
    \ingroup guh-plugins

    This plugin allows to controll RF 433 MHz actors an receive remote signals from \l{http://www.elroshop.eu/}{Elro}
    devices.

    \chapter Plugin properties
    Following JSON file contains the definition and the description of all available \l{DeviceClass}{DeviceClasses}
    and \l{Vendor}{Vendors} of this \l{DevicePlugin}.

    For more details how to read this JSON file please check out the documentation for \l{The plugin JSON File}.

    \quotefile plugins/deviceplugins/elro/devicepluginelro.json
*/

#include "devicepluginelro.h"
#include "devicemanager.h"
#include "plugininfo.h"
#include "hardware/radio433/radio433.h"


#include <QDebug>
#include <QStringList>

DevicePluginElro::DevicePluginElro()
{
}

DeviceManager::DeviceError DevicePluginElro::executeAction(Device *device, const Action &action)
{
    if (!hardwareManager()->radio433()->available())
        return DeviceManager::DeviceErrorHardwareNotAvailable;

    if (action.actionTypeId() != elroSocketPowerActionTypeId)
        return DeviceManager::DeviceErrorActionTypeNotFound;

    QList<int> rawData;
    QByteArray binCode;

    // create the bincode
    // channels
    if (device->paramValue(elroSocketChan1ParamTypeId).toBool()) {
        binCode.append("00");
    } else {
        binCode.append("01");
    }
    if (device->paramValue(elroSocketChan2ParamTypeId).toBool()) {
        binCode.append("00");
    } else {
        binCode.append("01");
    }
    if (device->paramValue(elroSocketChan3ParamTypeId).toBool()) {
        binCode.append("00");
    }else{
        binCode.append("01");
    }
    if(device->paramValue(elroSocketChan4ParamTypeId).toBool()){
        binCode.append("00");
    } else {
        binCode.append("01");
    }
    if (device->paramValue(elroSocketChan5ParamTypeId).toBool()) {
        binCode.append("00");
    } else {
        binCode.append("01");
    }

    // Buttons
    if (device->paramValue(elroSocketAParamTypeId).toBool()) {
        binCode.append("00");
    } else {
        binCode.append("01");
    }
    if (device->paramValue(elroSocketBParamTypeId).toBool()) {
        binCode.append("00");
    } else {
        binCode.append("01");
    }
    if (device->paramValue(elroSocketCParamTypeId).toBool()) {
        binCode.append("00");
    } else {
        binCode.append("01");
    }
    if (device->paramValue(elroSocketDParamTypeId).toBool()) {
        binCode.append("00");
    } else {
        binCode.append("01");
    }
    if (device->paramValue(elroSocketEParamTypeId).toBool()) {
        binCode.append("00");
    } else {
        binCode.append("01");
    }

    // Power
    if (action.param(elroSocketPowerParamTypeId).value().toBool()) {
        binCode.append("0001");
    } else {
        binCode.append("0100");
    }

    //create rawData timings list
    int delay = 350;

    // sync signal
    rawData.append(1);
    rawData.append(31);

    // add the code
    foreach (QChar c, binCode) {
        if (c == '0') {
            rawData.append(1);
            rawData.append(3);
        } else {
            rawData.append(3);
            rawData.append(1);
        }
    }

    // send data to hardware resource
    if (hardwareManager()->radio433()->sendData(delay, rawData, 10)) {
        qCDebug(dcElro) << "Transmitted" << pluginName() << device->name() << "power: " << action.param(elroSocketPowerParamTypeId).value().toBool();
    } else {
        qCWarning(dcElro) << "Could not transmitt" << pluginName() << device->name() << "power: " << action.param(elroSocketPowerParamTypeId).value().toBool();
        return DeviceManager::DeviceErrorHardwareNotAvailable;
    }

    return DeviceManager::DeviceErrorNoError;
}

void DevicePluginElro::radioData(const QList<int> &rawData)
{    
    // filter right here a wrong signal length
    if (rawData.length() != 49) {
        return;
    }

    int delay = rawData.first()/31;
    QByteArray binCode;
    
    // average 314
    if (delay > 290 && delay < 400) {
        // go trough all 48 timings (without sync signal)
        for (int i = 1; i <= 48; i+=2 ) {
            int div;
            int divNext;
            
            // if short
            if (rawData.at(i) <= 700) {
                div = 1;
            } else {
                div = 3;
            }
            // if long
            if (rawData.at(i+1) < 700) {
                divNext = 1;
            } else {
                divNext = 3;
            }

             //      _
             //     | |___   = 0 -> in 4 delays => 1000
             //         _
             //     ___| |   = 1 -> in 4 delays => 0001

            if (div == 1 && divNext == 3) {
                binCode.append('0');
            } else if(div == 3 && divNext == 1) {
                binCode.append('1');
            } else {
                return;
            }
        }
    } else {
        return;
    }

    qCDebug(dcElro) << "Understands this protocol: " << binCode;

    if (binCode.left(20) == "00000100000000000001") {
        if (binCode.right(4) == "0100") {
            qCDebug(dcElro) << "Motion Detector OFF";
        } else {
            qCDebug(dcElro) << "Motion Detector ON";
        }
    }

    // get the channel of the remote signal (5 channels, true=1, false=0)
    QList<bool> group;
    for (int i = 1; i < 10; i+=2) {
        if (binCode.at(i-1) == '0' && binCode.at(i) == '1') {
            group << false;
        } else if(binCode.at(i-1) == '0' && binCode.at(i) == '0') {
            group << true;
        } else {
            return;
        }
    }
    
    // get the button letter
    QString button;
    QByteArray buttonCode = binCode.mid(10,10);

    if (buttonCode == "0001010101") {
        button = "A";
    } else if (buttonCode == "0100010101") {
        button = "B";
    } else if (buttonCode == "0101000101") {
        button = "C";
    } else if(buttonCode == "0101010001") {
        button = "D";
    } else if(buttonCode == "0101010100") {
        button = "E";
    } else {
        return;
    }

    // get power status -> On = 0100, Off = 0001
    bool power;
    if (binCode.right(4).toInt(0,2) == 1) {
        power = true;
    } else if(binCode.right(4).toInt(0,2) == 4) {
        power = false;
    } else {
        return;
    }

    qCDebug(dcElro) << group << buttonCode << power;
}
