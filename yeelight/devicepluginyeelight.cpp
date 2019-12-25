/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *                                                                         *
 *  Copyright (C) 2020 Bernhard Trinnes <bernhard.trinnes@nymea.io>        *
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

#include "devicepluginyeelight.h"

#include "devices/device.h"
#include "types/param.h"
#include "plugininfo.h"
#include "network/upnp/upnpdiscovery.h"
#include "network/upnp/upnpdiscoveryreply.h"

#include <QDebug>
#include <QColor>

DevicePluginYeelight::DevicePluginYeelight()
{

}

void DevicePluginYeelight::discoverDevices(DeviceDiscoveryInfo *info)
{
    if (info->deviceClassId() == colorBulbDeviceClassId) {

        DiscoveryJob *discovery = new DiscoveryJob();
        m_discoveries.insert(info, discovery);

        qCDebug(dcYeelight()) << "Starting UPnP discovery...";
        UpnpDiscoveryReply *upnpReply = hardwareManager()->upnpDiscovery()->discoverDevices("wifi_bulb");
        discovery->upnpReply = upnpReply;
        // Always clean up the upnp discovery
        connect(upnpReply, &UpnpDiscoveryReply::finished, upnpReply, &UpnpDiscoveryReply::deleteLater);

        // Process results if info is still around
        connect(upnpReply, &UpnpDiscoveryReply::finished, info, [this, upnpReply, discovery](){

            // Indicate we're done...
            discovery->upnpReply = nullptr;

            if (upnpReply->error() != UpnpDiscoveryReply::UpnpDiscoveryReplyErrorNoError) {
                qCWarning(dcYeelight()) << "Upnp discovery error" << upnpReply->error();
            }

            foreach (const UpnpDeviceDescriptor &upnpDevice, upnpReply->deviceDescriptors()) {
                if (upnpDevice.modelDescription().contains("color")) {
                    DeviceDescriptor descriptor(bridgeDeviceClassId, "Yeelight Color Bulb", upnpDevice.hostAddress().toString());
                    ParamList params;
                    QString bridgeId = upnpDevice.serialNumber().toLower();
                    foreach (Device *existingDevice, myDevices()) {
                        if (existingDevice->paramValue(bridgeDeviceIdParamTypeId).toString() == bridgeId) {
                            descriptor.setDeviceId(existingDevice->id());
                            break;
                        }
                    }
                    params.append(Param(bridgeDeviceHostParamTypeId, upnpDevice.hostAddress().toString()));
                    params.append(Param(bridgeDeviceIdParamTypeId, bridgeId));
                    descriptor.setParams(params);
                    qCDebug(dcYeelight()) << "UPnP: Found Hue bridge:" << bridgeId;
                    discovery->results.append(descriptor);
                }
            }
            //finishDiscovery(discovery);
        });

        info->finish(Device::DeviceErrorNoError);
    }
}

void DevicePluginYeelight::setupDevice(DeviceSetupInfo *info)
{
    Device *device = info->device();
    if (device->deviceClassId() == colorBulbDeviceClassId) {
        QHostAddress *address = device->paramValue(colorBulbDeviceAddressParamTypeId).toString();
        int port;
        Yeelight *yeelight = new Yeelight(hardwareManager(), address, port, this);
        info->finish(Device::DeviceErrorNoError);
    }
}

void DevicePluginYeelight::executeAction(DeviceActionInfo *info)
{
    Device *device = info->device();
    Action action = info->action();

    if (device->deviceClassId() == colorBulbDeviceClassId) {

       // if (action.actionTypeId() == ) {
        //}
    }
}

void DevicePluginYeelight::deviceRemoved(Device *device)
{

}
