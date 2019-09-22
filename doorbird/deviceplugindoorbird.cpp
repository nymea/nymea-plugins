/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *                                                                         *
 *  Copyright (C) 2019 Bernhard Trinnes <bernhard.trinnes@nymea.io>        *
 *  Copyright (C) 2019 Michael Zanetti <michael.zanetti@nymea.io>          *
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

#include "platform/platformzeroconfcontroller.h"
#include "network/zeroconf/zeroconfservicebrowser.h"
#include "network/zeroconf/zeroconfserviceentry.h"

#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QHostAddress>
#include <QTimer>

DevicePluginDoorbird::DevicePluginDoorbird()
{
}

DevicePluginDoorbird::~DevicePluginDoorbird()
{

}

Device::DeviceError DevicePluginDoorbird::discoverDevices(const DeviceClassId &deviceClassId, const ParamList &params)
{
    Q_UNUSED(params)
    if (deviceClassId == doorBirdDeviceClassId) {
        ZeroConfServiceBrowser *serviceBrowser = hardwareManager()->zeroConfController()->createServiceBrowser();
        QTimer::singleShot(5000, this, [this, serviceBrowser](){
            QList<DeviceDescriptor> deviceDescriptors;
            foreach (const ZeroConfServiceEntry serviceEntry, serviceBrowser->serviceEntries()) {
                if (serviceEntry.hostName().startsWith("bha-")) {
                    qCDebug(dcDoorBird) << "Found DoorBird device";
                    DeviceDescriptor deviceDescriptor(doorBirdDeviceClassId, serviceEntry.name(), serviceEntry.hostAddress().toString());
                    ParamList params;
                    //TODO add rediscovery
                    params.append(Param(doorBirdDeviceSerialnumberParamTypeId, serviceEntry.hostName()));
                    params.append(Param(doorBirdDeviceAddressParamTypeId, serviceEntry.hostAddress().toString()));
                    deviceDescriptor.setParams(params);
                    deviceDescriptors.append(deviceDescriptor);
                }
            }
            emit devicesDiscovered(doorBirdDeviceClassId, deviceDescriptors);
            serviceBrowser->deleteLater();
        });
        return Device::DeviceErrorAsync;
    }
    return Device::DeviceErrorDeviceClassNotFound;
}

DevicePairingInfo DevicePluginDoorbird::pairDevice(DevicePairingInfo &devicePairingInfo)
{
    qCDebug(dcDoorBird()) << "PairDevice:" << devicePairingInfo.deviceClassId();

    devicePairingInfo.setStatus(Device::DeviceErrorNoError);
    devicePairingInfo.setMessage(tr("Please enter username and password for your Doorbird device."));
    return devicePairingInfo;
}

DevicePairingInfo DevicePluginDoorbird::confirmPairing(DevicePairingInfo &devicePairingInfo, const QString &username, const QString &secret)
{
    qCDebug(dcDoorBird()) << "confirm pairing called";

    pluginStorage()->beginGroup(devicePairingInfo.deviceId().toString());
    pluginStorage()->setValue("username", username);
    pluginStorage()->setValue("password", secret);
    pluginStorage()->endGroup();

    devicePairingInfo.setStatus(Device::DeviceErrorNoError);

    return devicePairingInfo;
}


Device::DeviceSetupStatus DevicePluginDoorbird::setupDevice(Device *device)
{
    if (device->deviceClassId() == doorBirdDeviceClassId) {
        QHostAddress address = QHostAddress(device->paramValue(doorBirdDeviceAddressParamTypeId).toString());

        pluginStorage()->beginGroup(device->id().toString());
        QString username = pluginStorage()->value("username").toString();
        QString password = pluginStorage()->value("password").toString();
        pluginStorage()->endGroup();

        Doorbird *doorbird = new Doorbird(address, username, password, this);
        doorbird->connectToEventMonitor();
        m_doorbirdConnections.insert(device, doorbird);
        return Device::DeviceSetupStatusSuccess;
    }
    return Device::DeviceSetupStatusFailure;
}

Device::DeviceError DevicePluginDoorbird::executeAction(Device *device, const Action &action)
{
    if (device->deviceClassId() == doorBirdDeviceClassId) {
        Doorbird *doorbird = m_doorbirdConnections.value(device);
        if (!doorbird)
            return Device::DeviceErrorDeviceNotFound;

        if (action.actionTypeId() == doorBirdOpenDoorActionTypeId) {
            //Todo get action param
            doorbird->openDoor(1);
            return Device::DeviceErrorNoError;
        }
        if (action.actionTypeId() == doorBirdLightOnActionTypeId) {
            doorbird->lightOn();
            return Device::DeviceErrorNoError;
        }
    }
    return Device::DeviceErrorDeviceClassNotFound;
}

void DevicePluginDoorbird::deviceRemoved(Device *device)
{
    if (device->deviceClassId() == doorBirdDeviceClassId) {
        Doorbird *doorbirdConnection = m_doorbirdConnections.take(device);
        doorbirdConnection->deleteLater();
    }
}
