// SPDX-License-Identifier: GPL-3.0-or-later

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*
* Copyright (C) 2013 - 2024, nymea GmbH
* Copyright (C) 2024 - 2025, chargebyte austria GmbH
*
* This file is part of nymea-plugins.
*
* nymea-plugins is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* (at your option) any later version.
*
* nymea-plugins is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
* General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with nymea-plugins. If not, see <https://www.gnu.org/licenses/>.
*
* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

#include "integrationplugindoorbird.h"
#include "plugininfo.h"

#include <platform/platformzeroconfcontroller.h>
#include <network/zeroconf/zeroconfserviceentry.h>

#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QHostAddress>
#include <QTimer>

IntegrationPluginDoorbird::IntegrationPluginDoorbird()
{
}

void IntegrationPluginDoorbird::init()
{
    m_serviceBrowser = hardwareManager()->zeroConfController()->createServiceBrowser("_axis-video._tcp");
}

void IntegrationPluginDoorbird::discoverThings(ThingDiscoveryInfo *info)
{
    if (info->thingClassId() == doorBirdThingClassId) {

        QTimer::singleShot(5000, info, [this, info](){
            foreach (const ZeroConfServiceEntry serviceEntry, m_serviceBrowser->serviceEntries()) {
                if (serviceEntry.hostName().startsWith("bha-")) {
                    qCDebug(dcDoorBird) << "Found DoorBird Thing, name: " << serviceEntry.name() << "\n   host address:" << serviceEntry.hostAddress().toString() << "\n    text:" << serviceEntry.txt() << serviceEntry.protocol() << serviceEntry.serviceType();
                    ThingDescriptor ThingDescriptor(doorBirdThingClassId, serviceEntry.name(), serviceEntry.hostAddress().toString());
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
                    if (!myThings().filterByParam(doorBirdThingSerialnumberParamTypeId, macAddress).isEmpty()) {
                        Thing *existingThing = myThings().filterByParam(doorBirdThingSerialnumberParamTypeId, macAddress).first();
                        ThingDescriptor.setThingId(existingThing->id());
                    }
                    params.append(Param(doorBirdThingSerialnumberParamTypeId, macAddress));
                    params.append(Param(doorBirdThingAddressParamTypeId, serviceEntry.hostAddress().toString()));
                    ThingDescriptor.setParams(params);
                    info->addThingDescriptor(ThingDescriptor);
                }
            }
            info->finish(Thing::ThingErrorNoError);
        });
        return;
    } else {
        qCWarning(dcDoorBird()) << "Cannot discover for ThingClassId" << info->thingClassId();
        info->finish(Thing::ThingErrorThingClassNotFound);
    }
}


void IntegrationPluginDoorbird::startPairing(ThingPairingInfo *info)
{
    qCDebug(dcDoorBird()) << "Start pairing";
    if (info->thingClassId() == doorBirdThingClassId) {
        info->finish(Thing::ThingErrorNoError, QT_TR_NOOP("Please enter username and password for the DoorBird Thing"));
        return;
    } else {
        qCWarning(dcDoorBird()) << "StartPairing unhandled ThingClassId" << info->thingClassId();
        info->finish(Thing::ThingErrorThingClassNotFound);
    }
}


void IntegrationPluginDoorbird::confirmPairing(ThingPairingInfo *info, const QString &username, const QString &password)
{
    qCDebug(dcDoorBird()) << "Confirm pairing";
    if (info->thingClassId() == doorBirdThingClassId) {
        QHostAddress address = QHostAddress(info->params().paramValue(doorBirdThingAddressParamTypeId).toString());

        Doorbird *doorbird = new Doorbird(address, this);
        doorbird->getSession(username, password);
        connect(doorbird, &Doorbird::sessionIdReceived, info, [info, doorbird, this] {
            qCDebug(dcDoorBird()) << "Session id received, pairing successfull";
            m_pairingConnections.insert(info->thingId(), doorbird);
            info->finish(Thing::ThingErrorNoError);
        });
        connect(info, &ThingPairingInfo::aborted, doorbird, &Doorbird::deleteLater);

        pluginStorage()->beginGroup(info->thingId().toString());
        pluginStorage()->setValue("username", username);
        pluginStorage()->setValue("password", password);
        pluginStorage()->endGroup();
    } else {
        qCWarning(dcDoorBird()) << "Confirm pairing ThingClassNotFound" << info->thingClassId();
        info->finish(Thing::ThingErrorThingClassNotFound);
    }
}


void IntegrationPluginDoorbird::setupThing(ThingSetupInfo *info)
{
    qCDebug(dcDoorBird()) << "Setup thing" << info->thing()->name();
    Thing *thing = info->thing();

    if (thing->thingClassId() == doorBirdThingClassId) {

        if (m_doorbirdConnections.contains(thing->id())) {
            qCDebug(dcDoorBird()) << "Reconfigure, cleaning up";
            Doorbird *doorbird = m_doorbirdConnections.take(thing->id());
            if (doorbird)
                doorbird->deleteLater();
        }

        Doorbird *doorbird;
        if (m_pairingConnections.contains(thing->id())) {
            qCDebug(dcDoorBird()) << "Setup after pairing process, not creating new connection";
            doorbird = m_pairingConnections.value(thing->id());
            m_doorbirdConnections.insert(thing->id(), doorbird);
            info->finish(Thing::ThingErrorNoError);
        } else {
            QHostAddress address = QHostAddress(thing->paramValue(doorBirdThingAddressParamTypeId).toString());

            pluginStorage()->beginGroup(thing->id().toString());
            QString username = pluginStorage()->value("username").toString();
            QString password = pluginStorage()->value("password").toString();
            pluginStorage()->endGroup();

            qCDebug(dcDoorBird()) << "Creating new Doorbird conection" << username << password.left(4)+"******";
            doorbird = new Doorbird(address, this);

            connect(doorbird, &Doorbird::sessionIdReceived, info, [info, thing, doorbird, this] {
                 qCDebug(dcDoorBird()) << "Session id received, thing setup successfull";
                m_doorbirdConnections.insert(thing->id(), doorbird);
                info->finish(Thing::ThingErrorNoError);
            });
            doorbird->getSession(username, password);
            connect(info, &ThingSetupInfo::aborted, doorbird, &Doorbird::deleteLater);
        }
        connect(doorbird, &Doorbird::deviceConnected, this, &IntegrationPluginDoorbird::onDoorBirdConnected);
        connect(doorbird, &Doorbird::eventReveiced, this, &IntegrationPluginDoorbird::onDoorBirdEvent);
        connect(doorbird, &Doorbird::requestSent, this, &IntegrationPluginDoorbird::onDoorBirdRequestSent);
    } else {
        qCWarning(dcDoorBird()) << "Unhandled Thing class" << info->thing()->thingClass();
        info->finish(Thing::ThingErrorThingClassNotFound);
    }
}


void IntegrationPluginDoorbird::postSetupThing(Thing *thing)
{
    qCDebug(dcDoorBird()) << "Post setup thing" << thing->name();
    if (thing->thingClassId() == doorBirdThingClassId) {
        thing->setStateValue(doorBirdConnectedStateTypeId, true); //since we checked the connection inside ThingSetup
        Doorbird *doorbird =  m_doorbirdConnections.value(thing->id());
        doorbird->connectToEventMonitor();
        doorbird->infoRequest();
        doorbird->listFavorites();
        doorbird->listSchedules();
    }
}


void IntegrationPluginDoorbird::executeAction(ThingActionInfo *info)
{
    Thing *thing = info->thing();
    Action action = info->action();

    if (thing->thingClassId() == doorBirdThingClassId) {
        Doorbird *doorbird = m_doorbirdConnections.value(thing->id());
        if (!doorbird) {
            qCWarning(dcDoorBird()) << "Doorbird connection not found" << thing->name();
            info->finish(Thing::ThingErrorHardwareFailure);
            return;
        }
        if (action.actionTypeId() == doorBirdOpenDoorActionTypeId) {
            int number = action.param(doorBirdOpenDoorActionNumberParamTypeId).value().toInt();
            QUuid requestId = doorbird->openDoor(number);
            m_asyncActions.insert(requestId, info);
            connect(info, &ThingActionInfo::aborted, this, [requestId, this] {m_asyncActions.remove(requestId);});
            return;
        } else if (action.actionTypeId() == doorBirdLightOnActionTypeId) {
            QUuid requestId = doorbird->lightOn();
            m_asyncActions.insert(requestId, info);
            connect(info, &ThingActionInfo::aborted, this, [requestId, this] {m_asyncActions.remove(requestId);});
            return;
        } else if (action.actionTypeId() == doorBirdRestartActionTypeId) {
            QUuid requestId = doorbird->restart();
            m_asyncActions.insert(requestId, info);
            connect(info, &ThingActionInfo::aborted, this, [requestId, this] {m_asyncActions.remove(requestId);});
            return;
        } else {
            qCWarning(dcDoorBird()) << "Unhandled ActionTypeId:" << action.actionTypeId();
            info->finish(Thing::ThingErrorActionTypeNotFound);
        }
    } else {
        qCWarning(dcDoorBird()) << "Execute action, unhandled Thing class" << thing->thingClass();
        info->finish(Thing::ThingErrorThingClassNotFound);
    }
}

void IntegrationPluginDoorbird::thingRemoved(Thing *thing)
{
    qCDebug(dcDoorBird()) << "Removing thing" << thing->name();
    if (thing->thingClassId() == doorBirdThingClassId) {
        Doorbird *doorbirdConnection = m_doorbirdConnections.take(thing->id());
        doorbirdConnection->deleteLater();
    }
}

void IntegrationPluginDoorbird::onDoorBirdConnected(bool status)
{
    Doorbird *doorbird = static_cast<Doorbird *>(sender());
    Thing *thing = myThings().findById(m_doorbirdConnections.key(doorbird));
    if (!thing) {
        qCWarning(dcDoorBird()) << "Doorbird connection status changed, associated thing not found";
        return;
    }

    thing->setStateValue(doorBirdConnectedStateTypeId, status);
}

void IntegrationPluginDoorbird::onDoorBirdEvent(Doorbird::EventType eventType, bool status)
{
    Doorbird *doorbird = static_cast<Doorbird *>(sender());
    Thing *thing = myThings().findById(m_doorbirdConnections.key(doorbird));
    if (!thing) {
        qCWarning(dcDoorBird()) << "Doorbird event received, associated thing not found";
        return;
    }

    switch (eventType) {
    case Doorbird::EventType::Rfid:
        qCDebug(dcDoorBird()) << "RFID event received, this is not yet handled";
        break;
    case Doorbird::EventType::Input:
        qCDebug(dcDoorBird()) << "Input event received, this is not yet handled";
        break;
    case Doorbird::EventType::Motion:
        thing->setStateValue(doorBirdIsPresentStateTypeId, status);
        if (status) {
            thing->setStateValue(doorBirdLastSeenTimeStateTypeId, QDateTime::currentDateTime().toSecsSinceEpoch());
        }
        break;
    case Doorbird::EventType::Doorbell:
        if (status) {
            emit emitEvent(Event(doorBirdDoorbellPressedEventTypeId ,thing->id()));
        }
        break;
    }
}

void IntegrationPluginDoorbird::onDoorBirdRequestSent(QUuid requestId, bool success)
{
    if (m_asyncActions.contains(requestId)) {
        ThingActionInfo* actionInfo = m_asyncActions.take(requestId);
        actionInfo->finish(success ? Thing::ThingErrorNoError : Thing::ThingErrorInvalidParameter);
    }
}
