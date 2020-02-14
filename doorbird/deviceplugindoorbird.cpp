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
                    QString macAddress;
                    if (serviceEntry.txt().length() == 0) {
                        qCWarning(dcDoorBird()) << "Discovery failed, service entry missing";
                        continue;
                    }

                    if (serviceEntry.txt().first().split("=").length() == 2) {
                        macAddress = serviceEntry.txt().first().split("=").last();
                    } else {
                        qCWarning(dcDoorBird()) << "Could not parse MAC Address" << serviceEntry.txt().first();
                        continue;
                    }
                    if (!myDevices().filterByParam(doorBirdDeviceSerialnumberParamTypeId, macAddress).isEmpty()) {
                        Device *existingDevice = myDevices().filterByParam(doorBirdDeviceSerialnumberParamTypeId, macAddress).first();
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
    } else {
        qCWarning(dcDoorBird()) << "Cannot discover for deviceClassId" << info->deviceClassId();
        info->finish(Device::DeviceErrorDeviceNotFound);
    }
}


void DevicePluginDoorbird::startPairing(DevicePairingInfo *info)
{
    if (info->deviceClassId() == doorBirdDeviceClassId) {
        info->finish(Device::DeviceErrorNoError, QT_TR_NOOP("Please enter username and password for the DoorBird device"));
        return;
    } else {
        qCWarning(dcDoorBird()) << "StartPairing unhandled deviceClassId" << info->deviceClassId();
        info->finish(Device::DeviceErrorCreationMethodNotSupported);
    }
}


void DevicePluginDoorbird::confirmPairing(DevicePairingInfo *info, const QString &username, const QString &password)
{
    if (info->deviceClassId() == doorBirdDeviceClassId) {
        QHostAddress address = QHostAddress(info->params().paramValue(doorBirdDeviceAddressParamTypeId).toString());

        Doorbird *doorbird = new Doorbird(address, this);
        connect(doorbird, &Doorbird::deviceConnected, this, &DevicePluginDoorbird::onDoorBirdConnected);
        connect(doorbird, &Doorbird::eventReveiced, this, &DevicePluginDoorbird::onDoorBirdEvent);
        connect(doorbird, &Doorbird::requestSent, this, &DevicePluginDoorbird::onDoorBirdRequestSent);
        connect(doorbird, &Doorbird::sessionIdReceived, this, &DevicePluginDoorbird::onSessionIdReceived);
        m_doorbirdConnections.insert(info->deviceId(), doorbird);
        m_pendingPairings.insert(doorbird, info);
        doorbird->getSession(username, password);
        connect(info, &DevicePairingInfo::aborted, this, [this, info]{
            if (m_pendingPairings.values().contains(info)) {
                Doorbird *doorbird = m_pendingPairings.key(info);
                m_pendingPairings.remove(doorbird);
                doorbird->deleteLater();
            }
            m_doorbirdConnections.remove(info->deviceId());
        });

        pluginStorage()->beginGroup(info->deviceId().toString());
        pluginStorage()->setValue("username", username);
        pluginStorage()->setValue("password", password);
        pluginStorage()->endGroup();
    } else {
        qCWarning(dcDoorBird()) << "Confirm pairing DeviceClassNotFound" << info->deviceClassId();
        info->finish(Device::DeviceErrorDeviceClassNotFound);
    }
}


void DevicePluginDoorbird::setupDevice(DeviceSetupInfo *info)
{
    Device *device = info->device();

    if (device->deviceClassId() == doorBirdDeviceClassId) {
        QHostAddress address = QHostAddress(device->paramValue(doorBirdDeviceAddressParamTypeId).toString());

        if (m_doorbirdConnections.contains(device->id())) {
            info->finish(Device::DeviceErrorNoError);
        } else {
            pluginStorage()->beginGroup(device->id().toString());
            QString username = pluginStorage()->value("username").toString();
            QString password = pluginStorage()->value("password").toString();
            pluginStorage()->endGroup();

            qCDebug(dcDoorBird()) << "Device setup" << device->name() << username << password;
            Doorbird *doorbird = new Doorbird(address, this);
            connect(doorbird, &Doorbird::deviceConnected, this, &DevicePluginDoorbird::onDoorBirdConnected);
            connect(doorbird, &Doorbird::eventReveiced, this, &DevicePluginDoorbird::onDoorBirdEvent);
            connect(doorbird, &Doorbird::requestSent, this, &DevicePluginDoorbird::onDoorBirdRequestSent);
            connect(doorbird, &Doorbird::sessionIdReceived, this, &DevicePluginDoorbird::onSessionIdReceived);
            m_doorbirdConnections.insert(device->id(), doorbird);
            m_pendingDeviceSetups.insert(doorbird, info);
            doorbird->getSession(username, password);
            connect(info, &DeviceSetupInfo::aborted, this, [device, doorbird, this] {
                if (!doorbird) {
                    doorbird->deleteLater();
                }
                m_doorbirdConnections.remove(device->id());
                m_pendingPairings.remove(doorbird);
            });
        }
    } else {
        qCWarning(dcDoorBird()) << "Unhandled device class" << info->device()->deviceClass();
        info->finish(Device::DeviceErrorDeviceClassNotFound);
    }
}


void DevicePluginDoorbird::postSetupDevice(Device *device)
{
    if (device->deviceClassId() == doorBirdDeviceClassId) {
        device->setStateValue(doorBirdConnectedStateTypeId, true); //since we checked the connection in the deviceSetup
        Doorbird *doorbird =  m_doorbirdConnections.value(device->id());
        doorbird->connectToEventMonitor();
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
        Doorbird *doorbird = m_doorbirdConnections.value(device->id());
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
        } else {
            qCWarning(dcDoorBird()) << "Unhandled ActionTypeId:" << action.actionTypeId();
            info->finish(Device::DeviceErrorActionTypeNotFound);
        }
    } else {
        qCWarning(dcDoorBird()) << "Execute action, unhandled device class" << device->deviceClass();
        info->finish(Device::DeviceErrorDeviceClassNotFound);
    }
}

void DevicePluginDoorbird::deviceRemoved(Device *device)
{
    if (device->deviceClassId() == doorBirdDeviceClassId) {
        Doorbird *doorbirdConnection = m_doorbirdConnections.take(device->id());
        doorbirdConnection->deleteLater();
    }
}

void DevicePluginDoorbird::onDoorBirdConnected(bool status)
{
    Doorbird *doorbird = static_cast<Doorbird *>(sender());
    Device *device = myDevices().findById(m_doorbirdConnections.key(doorbird));
    if (!device)
        return;

    device->setStateValue(doorBirdConnectedStateTypeId, status);
}

void DevicePluginDoorbird::onDoorBirdEvent(Doorbird::EventType eventType, bool status)
{
    Doorbird *doorbird = static_cast<Doorbird *>(sender());
    Device *device = myDevices().findById(m_doorbirdConnections.key(doorbird));
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
            device->setStateValue(doorBirdLastSeenTimeStateTypeId, QDateTime::currentDateTime().toTime_t());
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

    if (m_asyncActions.contains(requestId)) {
        DeviceActionInfo* actionInfo = m_asyncActions.take(requestId);
        actionInfo->finish(success ? Device::DeviceErrorNoError : Device::DeviceErrorInvalidParameter);
    }

    if (m_pendingPairings.contains(doorbird) && !success) {
        DevicePairingInfo *info = m_pendingPairings.take(doorbird);
        info->finish(Device::DeviceErrorAuthenticationFailure, tr("Wrong username or password"));
    }

    if (m_pendingDeviceSetups.contains(doorbird) && !success) {
        DeviceSetupInfo *info = m_pendingDeviceSetups.take(doorbird);
        info->finish(Device::DeviceErrorAuthenticationFailure, tr("Wrong username or password"));
    }
}

void DevicePluginDoorbird::onSessionIdReceived(const QString &sessionId)
{
    Q_UNUSED(sessionId);
    Doorbird *doorbird = static_cast<Doorbird *>(sender());

    if (m_pendingPairings.contains(doorbird)) {
        DevicePairingInfo *info = m_pendingPairings.take(doorbird);
        info->finish(Device::DeviceErrorNoError);
    }

    if (m_pendingDeviceSetups.contains(doorbird)) {
        DeviceSetupInfo *info = m_pendingDeviceSetups.take(doorbird);
        info->finish(Device::DeviceErrorNoError);
    }
}
