/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *                                                                         *
 *  Copyright (C) 2015 Simon Stuerz <simon.stuerz@nymea.io>                *
 *  Copyright (C) 2019 Bernhard Trinnes <bernhard.trinnes@nymea.io>        *
 *                                                                         *
 *  This file is part of nymea.                                            *
 *                                                                         *
 *  nymea is free software: you can redistribute it and/or modify          *
 *  it under the terms of the GNU General Public License as published by   *
 *  the Free Software Foundation, version 2 of the License.                *
 *                                                                         *
 *  nymea is distributed in the hope that it will be useful,               *
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of         *
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the           *
 *  GNU General Public License for more details.                           *
 *                                                                         *
 *  You should have received a copy of the GNU General Public License      *
 *  along with nymea. If not, see <http://www.gnu.org/licenses/>.          *
 *                                                                         *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */


#include "devicepluginwallbe.h"
#include "plugininfo.h"

#include "devices/devicemanager.h"
#include "devices/device.h"
#include "types/param.h"

#include <QDebug>
#include <QStringList>
#include <QColor>
#include <QJsonDocument>
#include <QNetworkInterface>


DevicePluginWallbe::DevicePluginWallbe()
{
}

void DevicePluginWallbe::setupDevice(DeviceSetupInfo *info)
{
    Device *device = info->device();
    qCDebug(dcWallbe) << "Setting up a new device:" << device->params();

    QHostAddress address(device->paramValue(wallbeEcoDeviceIpParamTypeId).toString());

    if (address.isNull()){
        qCWarning(dcWallbe) << "Address is Null";
        info->finish(Device::DeviceErrorSetupFailed);
        return;
    }

    // TCP connction to target device
    WallBe *wallbe = new WallBe(address, 502, this);
    m_connections.insert(device, wallbe);

    info->finish(Device::DeviceErrorNoError);
    return;
}

void DevicePluginWallbe::discoverDevices(DeviceDiscoveryInfo *info)
{
    if (info->deviceClassId() != wallbeEcoDeviceClassId){
        info->finish(Device::DeviceErrorDeviceClassNotFound);
        return;
    }

    Discover *discover = new Discover(QStringList("-xO" "-p 502"));
    connect(discover, &Discover::devicesDiscovered, this, [info, this](QList<Host> hosts){
        foreach (Host host, hosts) {
            DeviceDescriptor descriptor;
            descriptor.setTitle(host.hostName());
            ParamList params;
            params.append(Param(wallbeEcoDeviceIpParamTypeId, host.address()));
            params.append(Param(wallbeEcoDeviceMacParamTypeId, host.macAddress()));
            descriptor.setParams(params);
            info->addDeviceDescriptor(descriptor);
        }
        info->finish(Device::DeviceErrorNoError);
    });
    return; //Async setup method
}

void DevicePluginWallbe::postSetupDevice(Device *device)
{
    qCDebug(dcWallbe) << "post Setup Device";
    update(device);

    if (!m_pluginTimer) {
        m_pluginTimer = hardwareManager()->pluginTimerManager()->registerTimer(120);
        connect(m_pluginTimer, &PluginTimer::timeout, this, [this] {
            foreach(Device *device, m_connections.keys()) {
                update(device);
            }
        });
    }
}

void DevicePluginWallbe::executeAction(DeviceActionInfo *info)
{
    Device *device = info->device();
    Action action = info->action();

    WallBe *wallbe = m_connections.value(device);
    if (!wallbe) {
        info->finish(Device::DeviceErrorHardwareFailure);
        return;
    }

    if (device->deviceClassId() == wallbeEcoDeviceClassId){

        // check if this is the "set power" action
        if (action.actionTypeId() == wallbeEcoPowerActionTypeId) {

            // get the param value of the charging action
            bool charging =  action.param(wallbeEcoPowerActionPowerParamTypeId).value().toBool();
            qCDebug(dcWallbe) << "start Charging button" << device->name() << "set power to" << charging;
            wallbe->setChargingStatus(charging);
            // Set the "power" state
            device->setStateValue(wallbeEcoPowerStateTypeId, charging);
            info->finish(Device::DeviceErrorNoError);
            return;

        } else if(action.actionTypeId() == wallbeEcoChargeCurrentActionTypeId){

            uint16_t current = action.param(wallbeEcoChargeCurrentEventChargeCurrentParamTypeId).value().toInt();
            qCDebug(dcWallbe) << "Charging power set to" << current;
            wallbe->setChargingCurrent(current);
            device->setStateValue(wallbeEcoChargeCurrentStateTypeId, current);
            info->finish(Device::DeviceErrorNoError);
            return;
        }
        info->finish(Device::DeviceErrorActionTypeNotFound);
        return;
    }
    info->finish(Device::DeviceErrorDeviceClassNotFound);
}


void DevicePluginWallbe::deviceRemoved(Device *device)
{
    m_address.removeOne(QHostAddress(device->paramValue(wallbeEcoDeviceIpParamTypeId).toString()));
    WallBe *wallbe = m_connections.take(device);
    if (wallbe) {
        qCDebug(dcWallbe) << "Remove device" << device->name();
        wallbe->deleteLater();
    }

    if (myDevices().isEmpty()) {
        hardwareManager()->pluginTimerManager()->unregisterTimer(m_pluginTimer);
        m_pluginTimer = nullptr;
    }
}


void DevicePluginWallbe::update(Device *device)
{    
    WallBe * wallbe = m_connections.value(device);
    if(!wallbe->isAvailable())
        return;

    device->setStateValue(wallbeEcoConnectedStateTypeId, true);

    //EV state - 16 bit ASCII (8bit)
    switch (wallbe->getEvStatus()) {
    case 65:
        device->setStateValue(wallbeEcoEvStatusStateTypeId, "A - No Car plugged in");
        break;
    case 66:
        device->setStateValue(wallbeEcoEvStatusStateTypeId, "B - Supply equipment not yet ready");
        break;
    case 67:
        device->setStateValue(wallbeEcoEvStatusStateTypeId, "C - Ready to charge");
        break;
    case 68:
        device->setStateValue(wallbeEcoEvStatusStateTypeId, "D - Ready to charge, ventilation needed");
        break;
    case 69:
        device->setStateValue(wallbeEcoEvStatusStateTypeId, "E - Short circuit detected");
        break;
    case 70:
        device->setStateValue(wallbeEcoEvStatusStateTypeId, "F - Supply equipment not available");
        break;
    default:
        device->setStateValue(wallbeEcoEvStatusStateTypeId, "F - Supply equipment not available");
    }

    qCDebug(dcWallbe) << "EV State:" << device->stateValue(wallbeEcoEvStatusStateTypeId).toString();

    // Extract Input Register 102 - load time - 32bit integer
    device->setStateValue(wallbeEcoChargeTimeStateTypeId, wallbe->getChargingTime());

    // Read the charge current state
    device->setStateValue(wallbeEcoChargeCurrentStateTypeId, wallbe->getChargingCurrent());
    device->setStateValue(wallbeEcoPowerStateTypeId,  wallbe->getChargingStatus());
}
