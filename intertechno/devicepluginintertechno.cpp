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

#include "devicepluginintertechno.h"

#include "devices/device.h"
#include "hardware/radio433/radio433.h"
#include "plugininfo.h"

#include <QDebug>
#include <QStringList>

DevicePluginIntertechno::DevicePluginIntertechno()
{

}


void DevicePluginIntertechno::executeAction(DeviceActionInfo *info)
{
    Device *device = info->device();
    Action action = info->action();

    if (!hardwareManager()->radio433()->available())
        return info->finish(Device::DeviceErrorHardwareNotAvailable, QT_TR_NOOP("No 433MHz radio available on this system."));

    QList<int> rawData;
    QByteArray binCode;

    QString familyCode = device->paramValue(switchDeviceFamilyCodeParamTypeId).toString();

    // =======================================
    // generate bin from family code
    if (familyCode == "A") {
        binCode.append("00000000");
    } else if (familyCode == "B") {
        binCode.append("01000000");
    } else if (familyCode == "C") {
        binCode.append("00010000");
    } else if (familyCode == "D") {
        binCode.append("01010000");
    } else if (familyCode == "E") {
        binCode.append("00000100");
    } else if (familyCode == "F") {
        binCode.append("01000100");
    } else if (familyCode == "G") {
        binCode.append("01000000");
    } else if (familyCode == "H") {
        binCode.append("01010100");
    } else if (familyCode == "I") {
        binCode.append("00000001");
    } else if (familyCode == "J") {
        binCode.append("01000001");
    } else if (familyCode == "K") {
        binCode.append("00010001");
    } else if (familyCode == "L") {
        binCode.append("01010001");
    } else if (familyCode == "M") {
        binCode.append("00000101");
    } else if (familyCode == "N") {
        binCode.append("01000101");
    } else if (familyCode == "O") {
        binCode.append("00010101");
    } else if (familyCode == "P") {
        binCode.append("01010101");
    }

    QString buttonCode = device->paramValue(switchDeviceButtonCodeParamTypeId).toString();

    // =======================================
    // generate bin from button code
    if (buttonCode == "1") {
        binCode.append("00000000");
    } else if (buttonCode == "2") {
        binCode.append("01000000");
    } else if (buttonCode == "3") {
        binCode.append("00010000");
    } else if (buttonCode == "4") {
        binCode.append("01010000");
    } else if (buttonCode == "5") {
        binCode.append("00000100");
    } else if (buttonCode == "6") {
        binCode.append("01000100");
    } else if (buttonCode == "7") {
        binCode.append("01000000");
    } else if (buttonCode == "8") {
        binCode.append("01010100");
    } else if (buttonCode == "9") {
        binCode.append("00000001");
    } else if (buttonCode == "10") {
        binCode.append("01000001");
    } else if (buttonCode == "11") {
        binCode.append("00010001");
    } else if (buttonCode == "12") {
        binCode.append("01010001");
    } else if (buttonCode == "13") {
        binCode.append("00000101");
    } else if (buttonCode == "14") {
        binCode.append("01000101");
    } else if (buttonCode == "15") {
        binCode.append("00010101");
    } else if (buttonCode == "16") {
        binCode.append("01010101");
    }

    if (binCode.length() != 16){
        return info->finish(Device::DeviceErrorInvalidParameter);
    }

    // =======================================
    // add fix nibble (0F)
    binCode.append("0001");

    // =======================================
    // add power nibble
    if (action.param(switchSetPowerActionPowerParamTypeId).value().toBool()) {
        binCode.append("0101");
    } else {
        binCode.append("0100");
    }
    // =======================================
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

    // =======================================
    // send data to hardware resource
    if (hardwareManager()->radio433()->sendData(delay, rawData, 10)) {
        qCDebug(dcIntertechno) << "transmitted" << pluginName() << device->name() << "power: " << action.param(switchSetPowerActionPowerParamTypeId).value().toBool();
    } else {
        qCWarning(dcIntertechno) << "could not transmitt" << pluginName() << device->name() << "power: " << action.param(switchSetPowerActionPowerParamTypeId).value().toBool();
        return info->finish(Device::DeviceErrorHardwareFailure, QT_TR_NOOP("Error sending data."));
    }
    return info->finish(Device::DeviceErrorNoError);
}

void DevicePluginIntertechno::radioData(const QList<int> &rawData)
{
    // filter right here a wrong signal length
    if (rawData.length() != 49) {
        return;
    }
    
    //    QList<Device*> deviceList = deviceManager()->findConfiguredDevices(intertechnoRemoteDeviceClassId);
    //    if (deviceList.isEmpty()) {
    //        return;
    //    }

    int delay = rawData.first()/31;
    QByteArray binCode;
    
    // =======================================
    // average 314
    if (delay > 300 && delay < 400) {
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
            
            //              _
            // if we have  | |___ = 0 -> in 4 delays => 1000
            //                 _
            // if we have  ___| | = 1 -> in 4 delays => 0001
            
            if (div == 1 && divNext == 3) {
                binCode.append('0');
            } else if (div == 3 && divNext == 1) {
                binCode.append('1');
            } else {
                return;
            }
        }
    } else {
        return;
    }

    // =======================================
    // Check nibble 16-19, must be 0001
    if (binCode.mid(16,4) != "0001") {
        return;
    }

    // =======================================
    // Get family code
    QString familyCode;
    bool ok;
    QByteArray familyCodeBin = binCode.left(8);
    int famiyCodeInt = familyCodeBin.toInt(&ok,2);

    if (!ok)
        return;

    switch (famiyCodeInt) {
    case 0b00000000:
        familyCode = "A";
        break;
    case 0b01000000:
        familyCode = "B";
        break;
    case 0b00010000:
        familyCode = "C";
        break;
    case 0b01010000:
        familyCode = "D";
        break;
    case 0b00000100:
        familyCode = "E";
        break;
    case 0b01000100:
        familyCode = "F";
        break;
    case 0b00010100:
        familyCode = "G";
        break;
    case 0b01010100:
        familyCode = "H";
        break;
    case 0b00000001:
        familyCode = "I";
        break;
    case 0b01000001:
        familyCode = "J";
        break;
    case 0b00010001:
        familyCode = "K";
        break;
    case 0b01010001:
        familyCode = "L";
        break;
    case 0b00000101:
        familyCode = "M";
        break;
    case 0b01000101:
        familyCode = "N";
        break;
    case 0b00010101:
        familyCode = "O";
        break;
    case 0b01010101:
        familyCode = "P";
        break;
    default:
        return;
    }

    // =======================================
    // Get button code
    QString buttonCode;
    QByteArray buttonCodeBin = binCode.mid(8,8);
    int buttonCodeInt = buttonCodeBin.toInt(&ok,2);

    if (!ok)
        return;

    switch (buttonCodeInt) {
    case 0b00000000:
        buttonCode = "1";
        break;
    case 0b01000000:
        buttonCode = "2";
        break;
    case 0b00010000:
        buttonCode = "3";
        break;
    case 0b01010000:
        buttonCode = "4";
        break;
    case 0b00000100:
        buttonCode = "5";
        break;
    case 0b01000100:
        buttonCode = "6";
        break;
    case 0b00010100:
        buttonCode = "7";
        break;
    case 0b01010100:
        buttonCode = "8";
        break;
    case 0b00000001:
        buttonCode = "9";
        break;
    case 0b01000001:
        buttonCode = "10";
        break;
    case 0b00010001:
        buttonCode = "11";
        break;
    case 0b01010001:
        buttonCode = "12";
        break;
    case 0b00000101:
        buttonCode = "13";
        break;
    case 0b01000101:
        buttonCode = "14";
        break;
    case 0b00010101:
        buttonCode = "15";
        break;
    case 0b01010101:
        buttonCode = "16";
        break;
    default:
        return;
    }

    // =======================================
    // get power status -> On = 0100, Off = 0001
    bool power;
    if (binCode.right(4).toInt(0,2) == 5) {
        power = true;
    } else if (binCode.right(4).toInt(0,2) == 4) {
        power = false;
    } else {
        return;
    }

    qCDebug(dcIntertechno) << "Intertechno: family code = " << familyCode << "button code =" << buttonCode << power;

}
