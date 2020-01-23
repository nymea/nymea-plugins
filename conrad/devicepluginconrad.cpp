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

#include "devicepluginconrad.h"

#include "devices/device.h"
#include "plugininfo.h"
#include "hardware/radio433/radio433.h"

#include <QDebug>
#include <QStringList>


DevicePluginConrad::DevicePluginConrad(): DevicePlugin()
{

}

void DevicePluginConrad::setupDevice(DeviceSetupInfo *info)
{
    if (info->device()->deviceClassId() == conradShutterDeviceClassId) {
        info->finish(Device::DeviceErrorNoError);
        return;
    }

    info->finish(Device::DeviceErrorDeviceClassNotFound);
}

void DevicePluginConrad::executeAction(DeviceActionInfo *info)
{
    Device *device = info->device();
    Action action = info->action();

    if (!hardwareManager()->radio433()->available()) {
        info->finish(Device::DeviceErrorHardwareNotAvailable);
        return;
    }

    QList<int> rawData;
    QByteArray binCode;

    int repetitions = 10;

    if (action.actionTypeId() == conradShutterUpActionTypeId) {
        binCode = "10101000";
    } else if (action.actionTypeId() == conradShutterDownActionTypeId) {
        binCode = "10100000";
    } else if (action.actionTypeId() == conradShutterSyncActionTypeId) {
        binCode = "10100000";
        repetitions = 20;
    } else {
        info->finish(Device::DeviceErrorActionTypeNotFound);
        return;
    }

    // append ID
    binCode.append("100101010110011000000001");

    //QByteArray remoteId = "100101010110011000000001";
    //    QByteArray motionDetectorId = "100100100101101101101010";
    //QByteArray wallSwitchId = "000001001101000010110110";
    //    QByteArray randomID     = "100010101010111010101010";



    // =======================================
    //create rawData timings list
    int delay = 650;

    // sync signal
    rawData.append(1);
    rawData.append(10);

    // add the code
    foreach (QChar c, binCode) {
        if(c == '0'){
            rawData.append(1);
            rawData.append(2);
        }
        if(c == '1'){
            rawData.append(2);
            rawData.append(1);
        }
    }

    // =======================================
    // send data to driver
    if(hardwareManager()->radio433()->sendData(delay, rawData, repetitions)){
        qCDebug(dcConrad) << "Transmitted successfully" << device->name() << action.actionTypeId();
    }else{
        qCWarning(dcConrad) << "Could not transmitt" << pluginName() << device->name() << action.actionTypeId();
        info->finish(Device::DeviceErrorHardwareNotAvailable);
        return;
    }
    info->finish(Device::DeviceErrorNoError);
}

void DevicePluginConrad::radioData(const QList<int> &rawData)
{
    // filter right here a wrong signal length
    if(rawData.length() != 65){
        return;
    }

    qCDebug(dcConrad) << rawData;

    int delay = rawData.first()/10;
    QByteArray binCode;

    // =======================================
    // average 650
    if(delay > 600 && delay < 750){
        // go trough all 64 timings (without sync signal)
        for(int i = 1; i <= 64; i+=2 ){
            int div;
            int divNext;

            // if short
            if(rawData.at(i) <= 900){
                div = 1;
            }else{
                div = 2;
            }
            // if long
            if(rawData.at(i+1) < 900){
                divNext = 1;
            }else{
                divNext = 2;
            }

            //              _
            // if we have  | |__ = 0 -> in 4 delays => 100
            //              __
            // if we have  |  |_ = 1 -> in 4 delays => 110

            if(div == 1 && divNext == 2){
                binCode.append('0');
            }else if(div == 2 && divNext == 1){
                binCode.append('1');
            }else{
                return;
            }
        }
    }else{
        return;
    }

    qCDebug(dcConrad) << binCode.left(binCode.length() - 24) << "  ID = " << binCode.right(24);
}
