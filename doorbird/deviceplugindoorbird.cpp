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


void DevicePluginDoorbird::discoverDevices(DeviceDiscoveryInfo *info)
{
    if (info->deviceClassId() == doorBirdDeviceClassId) {
        ZeroConfServiceBrowser *serviceBrowser = hardwareManager()->zeroConfController()->createServiceBrowser("_axis-video._tcp");
        connect(info, &QObject::destroyed, serviceBrowser, &QObject::deleteLater);

        QTimer::singleShot(5000, this, [this, info, serviceBrowser](){
            foreach (const ZeroConfServiceEntry serviceEntry, serviceBrowser->serviceEntries()) {
                if (serviceEntry.hostName().startsWith("bha-")) {
                    qCDebug(dcDoorBird) << "Found DoorBird device, name: " << serviceEntry.name() << "\n   host address:" << serviceEntry.hostAddress().toString() << "\n    text:" << serviceEntry.txt() << serviceEntry.protocol() << serviceEntry.serviceType();
                    DeviceDescriptor deviceDescriptor(doorBirdDeviceClassId, serviceEntry.name(), serviceEntry.hostAddress().toString());
                    ParamList params;
                    QString macAddress = serviceEntry.txt().first();
                    if (!myDevices().filterByParam(doorBirdDeviceSerialnumberParamTypeId, macAddress).isEmpty()) {
                        Device *existingDevice = myDevices().filterByParam(doorBirdDeviceSerialnumberParamTypeId, serviceEntry.hostName()).first();
                        deviceDescriptor.setDeviceId(existingDevice->id());
                    }
                    params.append(Param(doorBirdDeviceSerialnumberParamTypeId, macAddress));
                    params.append(Param(doorBirdDeviceAddressParamTypeId, serviceEntry.hostAddress().toString()));
                    deviceDescriptor.setParams(params);
                    info->addDeviceDescriptor(deviceDescriptor);
                }
            }
            serviceBrowser->deleteLater();
            info->finish(Device::DeviceErrorNoError);
        });
        return;
    }
    qCWarning(dcDoorBird()) << "Cannot discover for deviceClassId" << info->deviceClassId();
    info->finish(Device::DeviceErrorDeviceNotFound);
}


void DevicePluginDoorbird::startPairing(DevicePairingInfo *info)
{

    if (info->deviceClassId() == doorBirdDeviceClassId) {
        qCDebug(dcDoorBird()) << "User and password. Login is \"user\" and \"password\".";
        info->finish(Device::DeviceErrorNoError, QT_TR_NOOP("Please enter the user credentials"));
        return;
    }
    info->finish(Device::DeviceErrorCreationMethodNotSupported);
}


void DevicePluginDoorbird::confirmPairing(DevicePairingInfo *info, const QString &username, const QString &secret)
{
    if (info->deviceClassId() == doorBirdDeviceClassId) {
        qCDebug(dcDoorBird()) << "confirm pairing called";

        pluginStorage()->beginGroup(info->deviceId().toString());
        pluginStorage()->setValue("username", username);
        pluginStorage()->setValue("password", secret);
        pluginStorage()->endGroup();

        info->finish(Device::DeviceErrorNoError);
        return;
    }
    info->finish(Device::DeviceErrorDeviceClassNotFound);
    return;
}


void DevicePluginDoorbird::setupDevice(DeviceSetupInfo *info)
{
    Device *device = info->device();

    if (device->deviceClassId() == doorBirdDeviceClassId) {
        QHostAddress address = QHostAddress(device->paramValue(doorBirdDeviceAddressParamTypeId).toString());

        pluginStorage()->beginGroup(device->id().toString());
        QString username = pluginStorage()->value("username").toString();
        QString password = pluginStorage()->value("password").toString();
        pluginStorage()->endGroup();

        qCDebug(dcDoorBird()) << "Device setup" << device->name() << username << password;
        Doorbird *doorbird = new Doorbird(address, username, password, this);
        connect(doorbird, &Doorbird::deviceConnected, this, &DevicePluginDoorbird::onDoorBirdConnected);
        connect(doorbird, &Doorbird::eventReveiced, this, &DevicePluginDoorbird::onDoorBirdEvent);
        connect(doorbird, &Doorbird::requestSent, this, &DevicePluginDoorbird::onDoorBirdRequestSent);
        connect(doorbird, &Doorbird::liveImageReceived, this, &DevicePluginDoorbird::onImageReceived);
        doorbird->connectToEventMonitor();
        m_doorbirdConnections.insert(device, doorbird);
        info->finish(Device::DeviceErrorNoError);
        return;
    }
    qCWarning(dcDoorBird()) << "Unhandled device class" << info->device()->deviceClass();
    info->finish(Device::DeviceErrorDeviceClassNotFound);
}


void DevicePluginDoorbird::postSetupDevice(Device *device)
{
    if (device->deviceClassId() == doorBirdDeviceClassId) {
        Doorbird *doorbird =  m_doorbirdConnections.value(device);
        doorbird->infoRequest();
        doorbird->listFavorites();
        doorbird->listSchedules();
    }
}


void DevicePluginDoorbird::executeAction(DeviceActionInfo *info)
{
    Device *device = info->device();
    Action action = info->action();

    if (device->deviceClassId() == doorBirdDeviceClassId) {
        Doorbird *doorbird = m_doorbirdConnections.value(device);
        if (!doorbird) {
            info->finish(Device::DeviceErrorHardwareFailure);
            return;
        }
        if (action.actionTypeId() == doorBirdOpenDoorActionTypeId) {
            int number = action.param(doorBirdOpenDoorActionNumberParamTypeId).value().toInt();
            doorbird->openDoor(number);
            info->finish(Device::DeviceErrorNoError);
            return;
        } else if (action.actionTypeId() == doorBirdLightOnActionTypeId) {
            doorbird->lightOn();
            info->finish(Device::DeviceErrorNoError);
            return;
        } else if (action.actionTypeId() == doorBirdRestartActionTypeId) {
            doorbird->restart();
            info->finish(Device::DeviceErrorNoError);
            return;
        }
        info->finish(Device::DeviceErrorActionTypeNotFound);
        return;
    }
    info->finish(Device::DeviceErrorDeviceClassNotFound);
}

void DevicePluginDoorbird::deviceRemoved(Device *device)
{
    if (device->deviceClassId() == doorBirdDeviceClassId) {
        Doorbird *doorbirdConnection = m_doorbirdConnections.take(device);
        doorbirdConnection->deleteLater();
    }
}

void DevicePluginDoorbird::onDoorBirdConnected(bool status)
{
    Doorbird *doorbird = static_cast<Doorbird *>(sender());
    Device *device = m_doorbirdConnections.key(doorbird);
    if (!device)
        return;

    device->setStateValue(doorBirdConnectedStateTypeId, status);
}

void DevicePluginDoorbird::onDoorBirdEvent(Doorbird::EventType eventType, bool status)
{
    Doorbird *doorbird = static_cast<Doorbird *>(sender());
    Device *device = m_doorbirdConnections.key(doorbird);
    if (!device)
        return;

    switch (eventType) {
    case Doorbird::EventType::Rfid:
        break;
    case Doorbird::EventType::Input:
        break;
    case Doorbird::EventType::Motion:
        device->setStateValue(doorBirdIsPresentStateTypeId, status);
        if (status) {
            doorbird->liveImageRequest();
        }
        break;
    case Doorbird::EventType::Doorbell:
        if (status) {
            emit emitEvent(Event(doorBirdDoorbellPressedEventTypeId ,device->id()));
        }
        break;
    }
}

void DevicePluginDoorbird::onDoorBirdRequestSent(QUuid requestId, bool success)
{
    Doorbird *doorbird = static_cast<Doorbird *>(sender());
    Device *device = m_doorbirdConnections.key(doorbird);
    if (!device)
        return;

    if (!m_asyncActions.contains(requestId))
        return;

    DeviceActionInfo* actionInfo = m_asyncActions.take(requestId);
    actionInfo->finish(success ? Device::DeviceErrorNoError : Device::DeviceErrorInvalidParameter);
}

void DevicePluginDoorbird::onImageReceived(QImage image)
{
    Q_UNUSED(image);
    Doorbird *doorbird = static_cast<Doorbird *>(sender());
    Device *device = m_doorbirdConnections.key(doorbird);
    if (!device)
        return;
    //TODO add QR code detection
    Event event;
    event.setDeviceId(device->id());
    event.setEventTypeId(doorBirdQrCodeDetectedEventTypeId);
    event.setParams(ParamList() << Param(doorBirdQrCodeDetectedEventDataParamTypeId, "image received"));
    emit emitEvent(event);
}
