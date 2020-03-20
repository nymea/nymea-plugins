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

#include "integrationplugindoorbird.h"
#include "plugininfo.h"

#include "platform/platformzeroconfcontroller.h"
#include "network/zeroconf/zeroconfservicebrowser.h"
#include "network/zeroconf/zeroconfserviceentry.h"

#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QHostAddress>
#include <QTimer>

IntegrationPluginDoorbird::IntegrationPluginDoorbird()
{
}


void IntegrationPluginDoorbird::discoverThings(ThingDiscoveryInfo *info)
{
    if (info->thingClassId() == doorBirdThingClassId) {
        ZeroConfServiceBrowser *serviceBrowser = hardwareManager()->zeroConfController()->createServiceBrowser("_axis-video._tcp");
        connect(info, &QObject::destroyed, serviceBrowser, &QObject::deleteLater);

        QTimer::singleShot(5000, this, [this, info, serviceBrowser](){
            foreach (const ZeroConfServiceEntry serviceEntry, serviceBrowser->serviceEntries()) {
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
            serviceBrowser->deleteLater();
            info->finish(Thing::ThingErrorNoError);
        });
        return;
    } else {
        qCWarning(dcDoorBird()) << "Cannot discover for ThingClassId" << info->thingClassId();
        info->finish(Thing::ThingErrorThingNotFound);
    }
}


void IntegrationPluginDoorbird::startPairing(ThingPairingInfo *info)
{
    if (info->thingClassId() == doorBirdThingClassId) {
        info->finish(Thing::ThingErrorNoError, QT_TR_NOOP("Please enter username and password for the DoorBird Thing"));
        return;
    } else {
        qCWarning(dcDoorBird()) << "StartPairing unhandled ThingClassId" << info->thingClassId();
        info->finish(Thing::ThingErrorCreationMethodNotSupported);
    }
}


void IntegrationPluginDoorbird::confirmPairing(ThingPairingInfo *info, const QString &username, const QString &password)
{
    if (info->thingClassId() == doorBirdThingClassId) {
        QHostAddress address = QHostAddress(info->params().paramValue(doorBirdThingAddressParamTypeId).toString());

        Doorbird *doorbird = new Doorbird(address, this);
        connect(doorbird, &Doorbird::deviceConnected, this, &IntegrationPluginDoorbird::onDoorBirdConnected);
        connect(doorbird, &Doorbird::eventReveiced, this, &IntegrationPluginDoorbird::onDoorBirdEvent);
        connect(doorbird, &Doorbird::requestSent, this, &IntegrationPluginDoorbird::onDoorBirdRequestSent);
        connect(doorbird, &Doorbird::sessionIdReceived, this, &IntegrationPluginDoorbird::onSessionIdReceived);
        m_doorbirdConnections.insert(info->thingId(), doorbird);
        m_pendingPairings.insert(doorbird, info);
        doorbird->getSession(username, password);
        connect(info, &ThingPairingInfo::aborted, this, [this, info]{
            if (m_pendingPairings.values().contains(info)) {
                Doorbird *doorbird = m_pendingPairings.key(info);
                m_pendingPairings.remove(doorbird);
                doorbird->deleteLater();
            }
            m_doorbirdConnections.remove(info->thingId());
        });

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
    Thing *thing = info->thing();

    if (thing->thingClassId() == doorBirdThingClassId) {
        QHostAddress address = QHostAddress(thing->paramValue(doorBirdThingAddressParamTypeId).toString());

        if (m_doorbirdConnections.contains(thing->id())) {
            info->finish(Thing::ThingErrorNoError);
        } else {
            pluginStorage()->beginGroup(thing->id().toString());
            QString username = pluginStorage()->value("username").toString();
            QString password = pluginStorage()->value("password").toString();
            pluginStorage()->endGroup();

            qCDebug(dcDoorBird()) << "Thing setup" << thing->name() << username << password;
            Doorbird *doorbird = new Doorbird(address, this);
            connect(doorbird, &Doorbird::deviceConnected, this, &IntegrationPluginDoorbird::onDoorBirdConnected);
            connect(doorbird, &Doorbird::eventReveiced, this, &IntegrationPluginDoorbird::onDoorBirdEvent);
            connect(doorbird, &Doorbird::requestSent, this, &IntegrationPluginDoorbird::onDoorBirdRequestSent);
            connect(doorbird, &Doorbird::sessionIdReceived, this, &IntegrationPluginDoorbird::onSessionIdReceived);
            m_doorbirdConnections.insert(thing->id(), doorbird);
            m_pendingThingSetups.insert(doorbird, info);
            doorbird->getSession(username, password);
            connect(info, &ThingSetupInfo::aborted, this, [thing, doorbird, this] {
                if (!doorbird) {
                    doorbird->deleteLater();
                }
                m_doorbirdConnections.remove(thing->id());
                m_pendingPairings.remove(doorbird);
            });
        }
    } else {
        qCWarning(dcDoorBird()) << "Unhandled Thing class" << info->thing()->thingClass();
        info->finish(Thing::ThingErrorThingClassNotFound);
    }
}


void IntegrationPluginDoorbird::postSetupThing(Thing *thing)
{
    if (thing->thingClassId() == doorBirdThingClassId) {
        thing->setStateValue(doorBirdConnectedStateTypeId, true); //since we checked the connection in the ThingSetup
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
            qCWarning(dcDoorBird()) << "Doorbird object not found" << thing->name();
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
    if (thing->thingClassId() == doorBirdThingClassId) {
        Doorbird *doorbirdConnection = m_doorbirdConnections.take(thing->id());
        doorbirdConnection->deleteLater();
    }
}

void IntegrationPluginDoorbird::onDoorBirdConnected(bool status)
{
    Doorbird *doorbird = static_cast<Doorbird *>(sender());
    Thing *thing = myThings().findById(m_doorbirdConnections.key(doorbird));
    if (!thing)
        return;

    thing->setStateValue(doorBirdConnectedStateTypeId, status);
}

void IntegrationPluginDoorbird::onDoorBirdEvent(Doorbird::EventType eventType, bool status)
{
    Doorbird *doorbird = static_cast<Doorbird *>(sender());
    Thing *thing = myThings().findById(m_doorbirdConnections.key(doorbird));
    if (!thing)
        return;

    switch (eventType) {
    case Doorbird::EventType::Rfid:
        break;
    case Doorbird::EventType::Input:
        break;
    case Doorbird::EventType::Motion:
        thing->setStateValue(doorBirdIsPresentStateTypeId, status);
        if (status) {
            thing->setStateValue(doorBirdLastSeenTimeStateTypeId, QDateTime::currentDateTime().toTime_t());
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
    Doorbird *doorbird = static_cast<Doorbird *>(sender());

    if (m_asyncActions.contains(requestId)) {
        ThingActionInfo* actionInfo = m_asyncActions.take(requestId);
        actionInfo->finish(success ? Thing::ThingErrorNoError : Thing::ThingErrorInvalidParameter);
    }

    if (m_pendingPairings.contains(doorbird) && !success) {
        ThingPairingInfo *info = m_pendingPairings.take(doorbird);
        info->finish(Thing::ThingErrorAuthenticationFailure, tr("Wrong username or password"));
    }

    if (m_pendingThingSetups.contains(doorbird) && !success) {
        ThingSetupInfo *info = m_pendingThingSetups.take(doorbird);
        info->finish(Thing::ThingErrorAuthenticationFailure, tr("Wrong username or password"));
    }
}

void IntegrationPluginDoorbird::onSessionIdReceived(const QString &sessionId)
{
    Q_UNUSED(sessionId);
    Doorbird *doorbird = static_cast<Doorbird *>(sender());

    if (m_pendingPairings.contains(doorbird)) {
        ThingPairingInfo *info = m_pendingPairings.take(doorbird);
        info->finish(Thing::ThingErrorNoError);
    }

    if (m_pendingThingSetups.contains(doorbird)) {
        ThingSetupInfo *info = m_pendingThingSetups.take(doorbird);
        info->finish(Thing::ThingErrorNoError);
    }
}
