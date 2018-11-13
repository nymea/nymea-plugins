/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *                                                                         *
 *  Copyright (C) 2017 Bernhard Trinnes <bernhard.trinnes@guh.io>          *
 *  Copyright (C) 2018 Simon Stürz <simon.stuerz@guh.io>                   *
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

#include "deviceplugindoorbird.h"
#include "plugininfo.h"

#include <network/networkaccessmanager.h>
#include <network/upnp/upnpdiscovery.h>


DevicePluginDoorbird::DevicePluginDoorbird()
{

}

DevicePluginDoorbird::~DevicePluginDoorbird()
{

}

DeviceManager::DeviceError DevicePluginDoorbird::discoverDevices(const DeviceClassId &deviceClassId, const ParamList &params)
{
    Q_UNUSED(deviceClassId)
    Q_UNUSED(params)

    UpnpDiscoveryReply *reply = hardwareManager()->upnpDiscovery()->discoverDevices();

    connect(reply, &UpnpDiscoveryReply::finished, this, [reply]() {
        reply->deleteLater();
        qCDebug(dcDoorbird) << "UPnP discovery reply:" << reply->error();
    });

    return DeviceManager::DeviceErrorAsync;
}

void DevicePluginDoorbird::init()
{

}

DeviceManager::DeviceSetupStatus DevicePluginDoorbird::setupDevice(Device *device)
{
    Q_UNUSED(device)
    return DeviceManager::DeviceSetupStatusSuccess;
}

DeviceManager::DeviceError DevicePluginDoorbird::executeAction(Device *device, const Action &action)
{
    if (action.actionTypeId() == doorbirdUnlatchActionTypeId) {
        QNetworkRequest request(QString("http://%1/bha-api/open-door.cgi?r=1").arg(device->paramValue(doorbirdDeviceAddressParamTypeId).toString()));
        QNetworkReply *reply = hardwareManager()->networkManager()->get(request);
        connect(reply, &QNetworkReply::finished, this, [reply](){
            reply->deleteLater();
            qDebug() << "Network reply finished:" << reply->error() << reply->errorString();
        });
    }
    return DeviceManager::DeviceErrorDeviceClassNotFound;
}
