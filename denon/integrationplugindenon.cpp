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

#include "integrationplugindenon.h"
#include "plugininfo.h"
#include "integrations/thing.h"
#include "network/networkaccessmanager.h"
#include "network/upnp/upnpdiscovery.h"
#include "network/upnp/upnpdiscoveryreply.h"
#include "platform/platformzeroconfcontroller.h"
#include "network/zeroconf/zeroconfservicebrowser.h"
#include "types/mediabrowseritem.h"

#include <QDebug>
#include <QStringList>
#include <QJsonDocument>
#include <QTimer>
#include <QUrl>
#include <QUrlQuery>

IntegrationPluginDenon::IntegrationPluginDenon()
{
}

void IntegrationPluginDenon::init()
{
    m_notificationUrl = QUrl(configValue(denonPluginNotificationUrlParamTypeId).toString());
    connect(this, &IntegrationPluginDenon::configValueChanged, this, &IntegrationPluginDenon::onPluginConfigurationChanged);
}

void IntegrationPluginDenon::discoverThings(ThingDiscoveryInfo *info)
{
    if (info->thingClassId() == AVRX1000ThingClassId) {
        if (!hardwareManager()->zeroConfController()->available() || !hardwareManager()->zeroConfController()->enabled()) {
            //: Error discovering Denon things
            info->finish(Thing::ThingErrorHardwareNotAvailable, QT_TR_NOOP("Thing discovery is not available."));
            return;
        }

        if (!m_serviceBrowser) {
            m_serviceBrowser = hardwareManager()->zeroConfController()->createServiceBrowser();;
        }

        QTimer::singleShot(2000, info, [this, info](){
            QStringList discoveredIds;

            foreach (const ZeroConfServiceEntry &service, m_serviceBrowser->serviceEntries()) {
                if (service.txt().contains("am=AVRX1000")) {

                    QString id = service.name().split("@").first();
                    QString name = service.name().split("@").last();
                    QString address = service.hostAddress().toString();
                    qCDebug(dcDenon) << "service discovered" << name << "ID:" << id;
                    if (discoveredIds.contains(id))
                        break;
                    discoveredIds.append(id);
                    ThingDescriptor thingDescriptor(AVRX1000ThingClassId, name, address);
                    ParamList params;
                    params.append(Param(AVRX1000ThingIpParamTypeId, address));
                    params.append(Param(AVRX1000ThingIdParamTypeId, id));
                    thingDescriptor.setParams(params);
                    foreach (Thing *existingThing, myThings()) {
                        if (existingThing->paramValue(AVRX1000ThingIdParamTypeId).toString() == id) {
                            thingDescriptor.setThingId(existingThing->id());
                            break;
                        }
                    }
                    info->addThingDescriptor(thingDescriptor);
                }
            }
            info->finish(Thing::ThingErrorNoError);
        });
        return;
    } else if (info->thingClassId() == heosThingClassId) {
        /*
        * The HEOS products can be discovered using the UPnP SSDP protocol. Through discovery,
        * the IP address of the HEOS products can be retrieved. Once the IP address is retrieved,
        * a telnet connection to port 1255 can be opened to access the HEOS CLI and control the HEOS system.
        * The HEOS product IP address can also be set statically and manually programmed into the control system.
        * Search target name (ST) in M-SEARCH discovery request is 'urn:schemas-denon-com:thing:ACT-Denon:1'.
        */
        UpnpDiscoveryReply *reply = hardwareManager()->upnpDiscovery()->discoverDevices();
        connect(reply, &UpnpDiscoveryReply::finished, info, [this, reply, info](){
            reply->deleteLater();

            if (reply->error() != UpnpDiscoveryReply::UpnpDiscoveryReplyErrorNoError) {
                qCWarning(dcDenon()) << "Upnp discovery error" << reply->error();
                info->finish(Thing::ThingErrorHardwareFailure, QT_TR_NOOP("UPnP discovery failed."));
                return;
            }

            foreach (const UpnpDeviceDescriptor &upnpThing, reply->deviceDescriptors()) {
                qCDebug(dcDenon) << "UPnP thing found:" << upnpThing.modelDescription() << upnpThing.friendlyName() << upnpThing.hostAddress().toString() << upnpThing.modelName() << upnpThing.manufacturer() << upnpThing.serialNumber();

                if (upnpThing.modelName().contains("HEOS", Qt::CaseSensitivity::CaseInsensitive)) {
                    QString serialNumber = upnpThing.serialNumber();
                    if (serialNumber != "0000001") {
                        // child things have serial number 0000001
                        ThingDescriptor descriptor(heosThingClassId, upnpThing.modelName(), serialNumber);
                        ParamList params;
                        foreach (Thing *existingThing, myThings()) {
                            if (existingThing->paramValue(heosThingSerialNumberParamTypeId).toString().contains(serialNumber, Qt::CaseSensitivity::CaseInsensitive)) {
                                descriptor.setThingId(existingThing->id());
                                break;
                            }
                        }
                        params.append(Param(heosThingModelNameParamTypeId, upnpThing.modelName()));
                        params.append(Param(heosThingIpParamTypeId, upnpThing.hostAddress().toString()));
                        params.append(Param(heosThingSerialNumberParamTypeId, serialNumber));
                        descriptor.setParams(params);
                        info->addThingDescriptor(descriptor);
                    }
                }
            }
            info->finish(Thing::ThingErrorNoError);
        });
        return;
    } else {
        info->finish(Thing::ThingErrorThingClassNotFound);
    }
}

void IntegrationPluginDenon::startPairing(ThingPairingInfo *info)
{
    info->finish(Thing::ThingErrorNoError, QT_TR_NOOP("Please enter your HEOS account credentials. Leave empty if you doesn't have any. Some features like music browsing won't be available."));
}

void IntegrationPluginDenon::confirmPairing(ThingPairingInfo *info, const QString &username, const QString &password)
{

    if (info->thingClassId() == heosThingClassId) {

        if (username.isEmpty()) { //thing connection will be setup without an user account
            return info->finish(Thing::ThingErrorNoError);
        }

        QHostAddress address(info->params().paramValue(heosThingIpParamTypeId).toString());
        Heos *heos = createHeosConnection(address);
        m_unfinishedHeosConnections.insert(info->thingId(), heos);
        m_unfinishedHeosPairings.insert(heos, info);
        connect(info, &ThingPairingInfo::aborted, this, [heos, this] {
            m_unfinishedHeosPairings.remove(heos);
            heos->deleteLater();
        });
        heos->connectDevice();
        heos->setUserAccount(username, password);

        pluginStorage()->beginGroup(info->thingId().toString());
        pluginStorage()->setValue("username", username);
        pluginStorage()->setValue("password", password);
        pluginStorage()->endGroup();
    }
}

void IntegrationPluginDenon::setupThing(ThingSetupInfo *info)
{
    Thing *thing = info->thing();

    if (thing->thingClassId() == AVRX1000ThingClassId) {
        qCDebug(dcDenon) << "Setup Denon thing" << thing->paramValue(AVRX1000ThingIpParamTypeId).toString();

        QHostAddress address(thing->paramValue(AVRX1000ThingIpParamTypeId).toString());
        if (address.isNull()) {
            qCWarning(dcDenon) << "Could not parse ip address" << thing->paramValue(AVRX1000ThingIpParamTypeId).toString();
            info->finish(Thing::ThingErrorInvalidParameter, QT_TR_NOOP("The given IP address is not valid."));
            return;
        }

        AvrConnection *denonConnection = new AvrConnection(address, 23, this);
        connect(denonConnection, &AvrConnection::connectionStatusChanged, this, &IntegrationPluginDenon::onAvrConnectionChanged);
        connect(denonConnection, &AvrConnection::socketErrorOccured, this, &IntegrationPluginDenon::onAvrSocketError);
        connect(denonConnection, &AvrConnection::channelChanged, this, &IntegrationPluginDenon::onAvrChannelChanged);
        connect(denonConnection, &AvrConnection::powerChanged, this, &IntegrationPluginDenon::onAvrPowerChanged);
        connect(denonConnection, &AvrConnection::volumeChanged, this, &IntegrationPluginDenon::onAvrVolumeChanged);
        connect(denonConnection, &AvrConnection::surroundModeChanged, this, &IntegrationPluginDenon::onAvrSurroundModeChanged);
        connect(denonConnection, &AvrConnection::muteChanged, this, &IntegrationPluginDenon::onAvrMuteChanged);

        m_avrConnections.insert(thing->id(), denonConnection);
        m_asyncAvrSetups.insert(denonConnection, info);
        // In case the setup is cancelled before we finish it...
        connect(info, &QObject::destroyed, this, [this, denonConnection]() { m_asyncAvrSetups.remove(denonConnection); });
        denonConnection->connectDevice();
        return;
    } else if (thing->thingClassId() == heosThingClassId) {
        qCDebug(dcDenon) << "Setup Denon thing" << thing->paramValue(heosThingIpParamTypeId).toString();

        QHostAddress address(thing->paramValue(heosThingIpParamTypeId).toString());
        if (address.isNull()) {
            qCWarning(dcDenon) << "Could not parse ip address" << thing->paramValue(heosThingIpParamTypeId).toString();
            info->finish(Thing::ThingErrorInvalidParameter, QT_TR_NOOP("The given IP address is not valid."));
            return;
        }

        Heos *heos;
        if (m_unfinishedHeosConnections.contains(thing->id())) {
            heos = m_unfinishedHeosConnections.take(thing->id());
            m_heosConnections.insert(thing->id(), heos);
            info->finish(Thing::ThingErrorNoError);
        } else {
            heos = createHeosConnection(address);
            m_heosConnections.insert(thing->id(), heos);
            m_asyncHeosSetups.insert(heos, info);
            // In case the setup is cancelled before we finish it...
            connect(info, &QObject::destroyed, this, [=]() {m_asyncHeosSetups.remove(heos);});
            heos->connectDevice();
        }
        return;
    } else if (thing->thingClassId() == heosPlayerThingClassId) {
        info->finish(Thing::ThingErrorNoError);
        return;
    } else {
        info->finish(Thing::ThingErrorThingClassNotFound);
    }
}

void IntegrationPluginDenon::thingRemoved(Thing *thing)
{
    qCDebug(dcDenon) << "Delete " << thing->name();

    if (thing->thingClassId() == AVRX1000ThingClassId) {
        if (m_avrConnections.contains(thing->id())) {
            AvrConnection *denonConnection = m_avrConnections.take(thing->id());
            denonConnection->disconnectDevice();
            denonConnection->deleteLater();
        }
    } else if (thing->thingClassId() == heosThingClassId) {
        if (m_heosConnections.contains(thing->id())) {
            Heos *heos = m_heosConnections.take(thing->id());
            heos->deleteLater();
        }
        pluginStorage()->remove(thing->id().toString());
    }

    if (myThings().empty()) {
        hardwareManager()->pluginTimerManager()->unregisterTimer(m_pluginTimer);
        m_pluginTimer = nullptr;
    }
}

void IntegrationPluginDenon::executeAction(ThingActionInfo *info)
{
    Thing *thing = info->thing();
    Action action = info->action();

    qCDebug(dcDenon) << "Execute action" << thing->id() << action.id() << action.params();
    if (thing->thingClassId() == AVRX1000ThingClassId) {
        AvrConnection *avrConnection = m_avrConnections.value(thing->id());

        if (action.actionTypeId() == AVRX1000PowerActionTypeId) {

            bool power = action.param(AVRX1000PowerActionPowerParamTypeId).value().toBool();
            avrConnection->setPower(power);
            return info->finish(Thing::ThingErrorNoError);

        } else if (action.actionTypeId() == AVRX1000VolumeActionTypeId) {

            int vol = action.param(AVRX1000VolumeActionVolumeParamTypeId).value().toInt();
            avrConnection->setVolume(vol);
            return info->finish(Thing::ThingErrorNoError);

        } else if (action.actionTypeId() == AVRX1000ChannelActionTypeId) {

            qCDebug(dcDenon) << "Execute update action" << action.id();
            QByteArray channel = action.param(AVRX1000ChannelActionChannelParamTypeId).value().toByteArray();
            avrConnection->setChannel(channel);
            return info->finish(Thing::ThingErrorNoError);

        } else if (action.actionTypeId() == AVRX1000IncreaseVolumeActionTypeId) {

            avrConnection->increaseVolume();
            return info->finish(Thing::ThingErrorNoError);

        } else if (action.actionTypeId() == AVRX1000DecreaseVolumeActionTypeId) {

            avrConnection->decreaseVolume();
            return info->finish(Thing::ThingErrorNoError);

        } else if (action.actionTypeId() == AVRX1000SurroundModeActionTypeId) {

            QByteArray surroundMode = action.param(AVRX1000SurroundModeActionSurroundModeParamTypeId).value().toByteArray();
            avrConnection->setSurroundMode(surroundMode);
            return info->finish(Thing::ThingErrorNoError);
        }
        return info->finish(Thing::ThingErrorActionTypeNotFound);
    } else if (thing->thingClassId() == heosThingClassId) {

        Heos *heos = m_heosConnections.value(thing->id());
        if (action.actionTypeId() == heosRebootActionTypeId) {
            heos->rebootSpeaker();
            return info->finish(Thing::ThingErrorNoError);
        }
    } else if (thing->thingClassId() == heosPlayerThingClassId) {

        Thing *heosThing = myThings().findById(thing->parentId());
        Heos *heos = m_heosConnections.value(heosThing->id());
        int playerId = thing->paramValue(heosPlayerThingPlayerIdParamTypeId).toInt();

        if (action.actionTypeId() == heosPlayerAlertActionTypeId) {
            heos->playUrl(playerId, m_notificationUrl);
            return info->finish(Thing::ThingErrorNoError);
        } else if (action.actionTypeId() == heosPlayerVolumeActionTypeId) {
            int volume = action.param(heosPlayerVolumeActionVolumeParamTypeId).value().toInt();
            heos->setVolume(playerId, volume);
            return info->finish(Thing::ThingErrorNoError);
        } else if (action.actionTypeId() == heosPlayerMuteActionTypeId) {
            bool mute = action.param(heosPlayerMuteActionMuteParamTypeId).value().toBool();
            heos->setMute(playerId, mute);
            return info->finish(Thing::ThingErrorNoError);
        } else if (action.actionTypeId() == heosPlayerPlaybackStatusActionTypeId) {
            QString playbackStatus = action.param(heosPlayerPlaybackStatusActionPlaybackStatusParamTypeId).value().toString();
            if (playbackStatus == "playing") {
                heos->setPlayerState(playerId, PLAYER_STATE_PLAY);
            } else if (playbackStatus == "stopping") {
                heos->setPlayerState(playerId, PLAYER_STATE_STOP);
            } else if (playbackStatus == "pausing") {
                heos->setPlayerState(playerId, PLAYER_STATE_PAUSE);
            }
            return info->finish(Thing::ThingErrorNoError);
        } else if (action.actionTypeId() == heosPlayerShuffleActionTypeId) {
            bool shuffle = action.param(heosPlayerShuffleActionShuffleParamTypeId).value().toBool();
            REPEAT_MODE repeatMode = REPEAT_MODE_OFF;
            if (thing->stateValue(heosPlayerRepeatStateTypeId) == "One") {
                repeatMode = REPEAT_MODE_ONE;
            } else if (thing->stateValue(heosPlayerRepeatStateTypeId) == "All") {
                repeatMode = REPEAT_MODE_ALL;
            }
            heos->setPlayMode(playerId, repeatMode, shuffle);
            return info->finish(Thing::ThingErrorNoError);
        } else if (action.actionTypeId() == heosPlayerSkipBackActionTypeId) {
            heos->playPrevious(playerId);
            return info->finish(Thing::ThingErrorNoError);
        } else if (action.actionTypeId() == heosPlayerStopActionTypeId) {
            heos->setPlayerState(playerId, PLAYER_STATE_STOP);
            return info->finish(Thing::ThingErrorNoError);
        } else if (action.actionTypeId() == heosPlayerPlayActionTypeId) {
            heos->setPlayerState(playerId, PLAYER_STATE_PLAY);
            return info->finish(Thing::ThingErrorNoError);
        } else if (action.actionTypeId() == heosPlayerPauseActionTypeId) {
            heos->setPlayerState(playerId, PLAYER_STATE_PAUSE);
            return info->finish(Thing::ThingErrorNoError);
        } else if (action.actionTypeId() == heosPlayerSkipNextActionTypeId) {
            heos->playNext(playerId);
            return info->finish(Thing::ThingErrorNoError);
        } else {
            return info->finish(Thing::ThingErrorActionTypeNotFound);
        }
    } else {
        return info->finish(Thing::ThingErrorThingClassNotFound);
    }
}

void IntegrationPluginDenon::postSetupThing(Thing *thing)
{
    if (thing->thingClassId() == heosThingClassId) {
        Heos *heos = m_heosConnections.value(thing->id());
        thing->setStateValue(heosConnectedStateTypeId, heos->connected());
        if (pluginStorage()->childGroups().contains(thing->id().toString())) {
            pluginStorage()->beginGroup(thing->id().toString());
            QString username = pluginStorage()->value("username").toString();
            QString password = pluginStorage()->value("password").toString();
            pluginStorage()->endGroup();
            heos->setUserAccount(username, password);
        } else {
            qCWarning(dcDenon()) << "Plugin storage doesn't contain this deviceId";
        }
        heos->getPlayers();
        heos->getGroups();

    } else if (thing->thingClassId() == heosPlayerThingClassId) {
        thing->setStateValue(heosPlayerConnectedStateTypeId, true);
        Thing *heosThing = myThings().findById(thing->parentId());
        Heos *heos = m_heosConnections.value(heosThing->id());
        int playerId = thing->paramValue(heosPlayerThingPlayerIdParamTypeId).toInt();
        heos->getPlayerState(playerId);
        heos->getPlayMode(playerId);
        heos->getVolume(playerId);
        heos->getMute(playerId);
        heos->getNowPlayingMedia(playerId);
    }

    if (!m_pluginTimer) {
        m_pluginTimer = hardwareManager()->pluginTimerManager()->registerTimer(60);
        connect(m_pluginTimer, &PluginTimer::timeout, this, &IntegrationPluginDenon::onPluginTimer);
    }
}


void IntegrationPluginDenon::onPluginTimer()
{
    foreach(AvrConnection *denonConnection, m_avrConnections.values()) {
        if (!denonConnection->connected()) {
            denonConnection->connectDevice();
        }
        Thing *thing = myThings().findById(m_avrConnections.key(denonConnection));
        if (thing->thingClassId() == AVRX1000ThingClassId) {
            denonConnection->getAllStatus();
        }
    }

    foreach(Thing *thing, myThings()) {

        if (thing->thingClassId() == heosThingClassId) {
            Heos *heos = m_heosConnections.value(thing->id());
            heos->getPlayers();
            heos->registerForChangeEvents(true);
        }
    }
}

void IntegrationPluginDenon::onAvrConnectionChanged(bool status)
{
    AvrConnection *denonConnection = static_cast<AvrConnection *>(sender());
    Thing *thing = myThings().findById(m_avrConnections.key(denonConnection));
    if (!thing)
        return;

    if (thing->thingClassId() == AVRX1000ThingClassId) {
        // if the thing is connected
        if (status) {
            // and from the first setup
            if (m_asyncAvrSetups.contains(denonConnection)) {
                ThingSetupInfo *info = m_asyncAvrSetups.take(denonConnection);
                info->finish(Thing::ThingErrorNoError);
            }
        }
        thing->setStateValue(AVRX1000ConnectedStateTypeId, denonConnection->connected());
    }
}

void IntegrationPluginDenon::onAvrVolumeChanged(int volume)
{
    AvrConnection *denonConnection = static_cast<AvrConnection *>(sender());
    Thing *thing = myThings().findById(m_avrConnections.key(denonConnection));
    if (!thing)
        return;

    if (thing->thingClassId() == AVRX1000ThingClassId) {
        thing->setStateValue(AVRX1000VolumeStateTypeId, volume);
    }
}

void IntegrationPluginDenon::onAvrChannelChanged(const QByteArray &channel)
{
    AvrConnection *denonConnection = static_cast<AvrConnection *>(sender());
    Thing *thing = myThings().findById(m_avrConnections.key(denonConnection));
    if (!thing)
        return;

    if (thing->thingClassId() == AVRX1000ThingClassId) {
        thing->setStateValue(AVRX1000ChannelStateTypeId, channel);
    }
}

void IntegrationPluginDenon::onAvrMuteChanged(bool mute)
{
    AvrConnection *denonConnection = static_cast<AvrConnection *>(sender());
    Thing *thing = myThings().findById(m_avrConnections.key(denonConnection));
    if (!thing)
        return;

    if (thing->thingClassId() == AVRX1000ThingClassId) {
        thing->setStateValue(AVRX1000MuteStateTypeId, mute);
    }
}

void IntegrationPluginDenon::onAvrPowerChanged(bool power)
{
    AvrConnection *denonConnection = static_cast<AvrConnection *>(sender());
    Thing *thing = myThings().findById(m_avrConnections.key(denonConnection));
    if (!thing)
        return;

    if (thing->thingClassId() == AVRX1000ThingClassId) {
        thing->setStateValue(AVRX1000PowerStateTypeId, power);
    }
}

void IntegrationPluginDenon::onAvrSurroundModeChanged(const QByteArray &surroundMode)
{
    AvrConnection *denonConnection = static_cast<AvrConnection *>(sender());
    Thing *thing = myThings().findById(m_avrConnections.key(denonConnection));
    if (!thing)
        return;

    if (thing->thingClassId() == AVRX1000ThingClassId) {
        thing->setStateValue(AVRX1000SurroundModeStateTypeId, surroundMode);
    }
}


void IntegrationPluginDenon::onAvrSocketError()
{
    AvrConnection *denonConnection = static_cast<AvrConnection *>(sender());
    Thing *thing = myThings().findById(m_avrConnections.key(denonConnection));
    if (!thing)
        return;

    if (thing->thingClassId() == AVRX1000ThingClassId) {

        // Check if setup running for this thing
        if (m_asyncAvrSetups.contains(denonConnection)) {
            ThingSetupInfo *info = m_asyncAvrSetups.take(denonConnection);
            qCWarning(dcDenon()) << "Could not add thing. The setup failed.";
            info->finish(Thing::ThingErrorHardwareFailure);
            // Delete the connection, the thing will not be added and
            // the connection will be created in the next setup
            denonConnection->deleteLater();
            m_avrConnections.remove(thing->id());
        }
    }
}

void IntegrationPluginDenon::onHeosConnectionChanged(bool status)
{
    Heos *heos = static_cast<Heos *>(sender());
    heos->registerForChangeEvents(true);
    if (status) {
        if (m_asyncHeosSetups.contains(heos)) {
            ThingSetupInfo *info = m_asyncHeosSetups.take(heos);
            info->finish(Thing::ThingErrorNoError);
        }
    }

    Thing *thing = myThings().findById(m_heosConnections.key(heos));
    if (!thing)
        return;

    if (thing->thingClassId() == heosThingClassId) {

        if (pluginStorage()->childGroups().contains(thing->id().toString())) {
            pluginStorage()->beginGroup(thing->id().toString());
            QString username = pluginStorage()->value("username").toString();
            QString password = pluginStorage()->value("password").toString();
            pluginStorage()->endGroup();
            heos->setUserAccount(username, password);
        } else {
            qCWarning(dcDenon()) << "Plugin storage doesn't contain this deviceId";
        }

        if (!status) {
            thing->setStateValue(heosLoggedInStateTypeId, false);
            thing->setStateValue(heosUserDisplayNameStateTypeId, "");
        }
        thing->setStateValue(heosConnectedStateTypeId, status);
        // update connection status for all child things
        foreach (Thing *playerThing, myThings().filterByParentId(thing->id())) {
            if (playerThing->thingClassId() == heosPlayerThingClassId) {
                playerThing->setStateValue(heosPlayerConnectedStateTypeId, status);
            }
        }
    }
}

void IntegrationPluginDenon::onHeosPlayersChanged()
{
    Heos *heos = static_cast<Heos *>(sender());
    heos->getPlayers();
}

void IntegrationPluginDenon::onHeosPlayersReceived(QList<HeosPlayer *> heosPlayers) {

    Heos *heos = static_cast<Heos *>(sender());
    Thing *thing = myThings().findById(m_heosConnections.key(heos));
    if (!thing) {
        return;
    }

    QList<ThingDescriptor> heosPlayerDescriptors;
    foreach (HeosPlayer *player, heosPlayers) {
        ThingDescriptor descriptor(heosPlayerThingClassId, player->name(), player->playerModel(), thing->id());
        ParamList params;
        if (!myThings().filterByParam(heosPlayerThingPlayerIdParamTypeId, player->playerId()).isEmpty()) {
            continue;
        }
        params.append(Param(heosPlayerThingModelParamTypeId, player->playerModel()));
        params.append(Param(heosPlayerThingPlayerIdParamTypeId, player->playerId()));
        params.append(Param(heosPlayerThingSerialNumberParamTypeId, player->serialNumber()));
        params.append(Param(heosPlayerThingVersionParamTypeId, player->playerVersion()));
        descriptor.setParams(params);
        qCDebug(dcDenon) << "Found new heos player" << player->name();
        heosPlayerDescriptors.append(descriptor);
    }
    if (!heosPlayerDescriptors.isEmpty())
        autoThingsAppeared(heosPlayerDescriptors);

    foreach(Thing *existingThing, myThings().filterByParentId(thing->id())) {
        bool playerAvailable = false;
        int playerId = existingThing->paramValue(heosPlayerThingPlayerIdParamTypeId).toInt();
        foreach (HeosPlayer *player, heosPlayers) {
            if (player->playerId() == playerId) {
                playerAvailable = true;
                break;
            }
        }
        if (!playerAvailable) {
            autoThingDisappeared(existingThing->id());
            m_playerBuffer.remove(playerId);
        }
    }
}

void IntegrationPluginDenon::onHeosPlayerInfoRecieved(HeosPlayer *heosPlayer)
{
    qDebug(dcDenon()) << "Heos player info received" << heosPlayer->name() << heosPlayer->playerId() << heosPlayer->groupId();
    m_playerBuffer.insert(heosPlayer->playerId(), heosPlayer);
}

void IntegrationPluginDenon::onHeosPlayStateReceived(int playerId, PLAYER_STATE state)
{
    foreach(Thing *thing, myThings().filterByParam(heosPlayerThingPlayerIdParamTypeId, playerId)) {
        if (state == PLAYER_STATE_PAUSE) {
            thing->setStateValue(heosPlayerPlaybackStatusStateTypeId, "Paused");
        } else if (state == PLAYER_STATE_PLAY) {
            thing->setStateValue(heosPlayerPlaybackStatusStateTypeId, "Playing");
        } else if (state == PLAYER_STATE_STOP) {
            thing->setStateValue(heosPlayerPlaybackStatusStateTypeId, "Stopped");
        }
        break;
    }
}

void IntegrationPluginDenon::onHeosRepeatModeReceived(int playerId, REPEAT_MODE repeatMode)
{
    foreach(Thing *thing, myThings().filterByParam(heosPlayerThingPlayerIdParamTypeId, playerId)) {
        if (repeatMode == REPEAT_MODE_ALL) {
            thing->setStateValue(heosPlayerRepeatStateTypeId, "All");
        } else  if (repeatMode == REPEAT_MODE_ONE) {
            thing->setStateValue(heosPlayerRepeatStateTypeId, "One");
        } else  if (repeatMode == REPEAT_MODE_OFF) {
            thing->setStateValue(heosPlayerRepeatStateTypeId, "None");
        }
        break;
    }
}

void IntegrationPluginDenon::onHeosShuffleModeReceived(int playerId, bool shuffle)
{
    foreach(Thing *thing, myThings().filterByParam(heosPlayerThingPlayerIdParamTypeId, playerId)) {
        thing->setStateValue(heosPlayerMuteStateTypeId, shuffle);
        break;
    }
}

void IntegrationPluginDenon::onHeosMuteStatusReceived(int playerId, bool mute)
{
    foreach(Thing *thing, myThings().filterByParam(heosPlayerThingPlayerIdParamTypeId, playerId)) {
        thing->setStateValue(heosPlayerMuteStateTypeId, mute);
        break;
    }
}

void IntegrationPluginDenon::onHeosVolumeStatusReceived(int playerId, int volume)
{
    foreach(Thing *thing, myThings().filterByParam(heosPlayerThingPlayerIdParamTypeId, playerId)) {
        thing->setStateValue(heosPlayerVolumeStateTypeId, volume);
        break;
    }
}

void IntegrationPluginDenon::onHeosNowPlayingMediaStatusReceived(int playerId, const QString &sourceId, const QString &artist, const QString &album, const QString &song, const QString &artwork)
{
    Thing *thing = myThings().filterByParam(heosPlayerThingPlayerIdParamTypeId, playerId).first();
    if (!thing)
        return;

    thing->setStateValue(heosPlayerArtistStateTypeId, artist);
    thing->setStateValue(heosPlayerTitleStateTypeId, song);
    thing->setStateValue(heosPlayerArtworkStateTypeId, artwork);
    thing->setStateValue(heosPlayerCollectionStateTypeId, album);
    thing->setStateValue(heosPlayerSourceStateTypeId, sourceId);
}

void IntegrationPluginDenon::onHeosMusicSourcesReceived(quint32 sequenceNumber, QList<MusicSourceObject> musicSources)
{
    Q_UNUSED(sequenceNumber)
    Heos *heos = static_cast<Heos *>(sender());
    Thing *thing = myThings().findById(m_heosConnections.key(heos));
    if (!thing) {
        return;
    }
    bool loggedIn = thing->stateValue(heosLoggedInStateTypeId).toBool();
    if (m_pendingGetSourcesRequest.contains(heos)) {
        BrowseResult *result = m_pendingGetSourcesRequest.take(heos);
        foreach(MusicSourceObject source, musicSources) {
            MediaBrowserItem item;
            item.setDisplayName(source.name);
            item.setId("source=" + QString::number(source.sourceId));
            item.setExecutable(false);
            item.setBrowsable(source.available);
            if (!source.available) {
                item.setDescription(tr("Service is not available"));
            } else {
                item.setDescription(source.serviceUsername);
            }
            item.setIcon(BrowserItem::BrowserIconMusic);
            if (source.name == "Amazon") {
                item.setMediaIcon(MediaBrowserItem::MediaBrowserIconAmazon);
                //result->addItem(item);
            } else if (source.name == "Deezer") {
                item.setMediaIcon(MediaBrowserItem::MediaBrowserIconDeezer);
                //result->addItem(item);
            } else if (source.name == "Napster") {
                item.setMediaIcon(MediaBrowserItem::MediaBrowserIconNapster);
                //result->addItem(item);
            } else if (source.name == "SoundCloud") {
                item.setMediaIcon(MediaBrowserItem::MediaBrowserIconSoundCloud);
                //result->addItem(item);
            } else if (source.name == "Tidal") {
                item.setMediaIcon(MediaBrowserItem::MediaBrowserIconTidal);
                //result->addItem(item);
            } else if (source.name == "TuneIn") {
                item.setMediaIcon(MediaBrowserItem::MediaBrowserIconTuneIn);
                item.setBrowsable(true);
                item.setDescription(source.serviceUsername);
                result->addItem(item);
            } else if (source.name == "Local Music") {
                item.setMediaIcon(MediaBrowserItem::MediaBrowserIconDisk);
                //result->addItem(item);
            } else if (source.name == "Playlists") {
                item.setMediaIcon(MediaBrowserItem::MediaBrowserIconPlaylist);
            } else if (source.name == "History") {
                item.setMediaIcon(MediaBrowserItem::MediaBrowserIconRecentlyPlayed);
                item.setBrowsable(loggedIn);
                if (!loggedIn) {
                    item.setDescription("Login required");
                } else {
                    item.setDescription(source.serviceUsername);
                }
                result->addItem(item);
            } else if (source.name == "AUX Input") {
                item.setMediaIcon(MediaBrowserItem::MediaBrowserIconAux);
                //result->addItem(item);
            } else if (source.name == "Favorites") {
                item.setIcon(BrowserItem::BrowserIconFavorites);
                item.setBrowsable(loggedIn);
                if (!loggedIn) {
                    item.setDescription("Login required");
                } else {
                    item.setDescription(source.serviceUsername);
                }
                result->addItem(item);
            } else {
                item.setThumbnail(source.image_url);
            }
            qDebug(dcDenon()) << "Music source received:" << source.name << source.type << source.sourceId << source.image_url;
        }
        result->finish(Thing::ThingErrorNoError);
    }
}

void IntegrationPluginDenon::onHeosBrowseRequestReceived(quint32 sequenceNumber, const QString &sourceId, const QString &containerId, QList<MusicSourceObject> musicSources, QList<MediaObject> mediaItems)
{
    Q_UNUSED(sequenceNumber)
    Heos *heos = static_cast<Heos *>(sender());
    Thing *thing = myThings().findById(m_heosConnections.key(heos));
    if (!thing) {
        return;
    }
    bool loggedIn = thing->stateValue(heosLoggedInStateTypeId).toBool();

    QString identifier;
    if (containerId.isEmpty()) {
        identifier = sourceId;
    } else {
        identifier = containerId;
    }
    if (QUrl(identifier).isValid()) {
        identifier = QUrl::fromPercentEncoding(identifier.toUtf8());
    }

    if (m_pendingBrowseResult.contains(identifier)) {
        BrowseResult *result = m_pendingBrowseResult.take(identifier);
        foreach(MediaObject media, mediaItems) {
            MediaBrowserItem item;
            item.setIcon(BrowserItem::BrowserIconMusic);
            qDebug(dcDenon()) << "Adding Item" << media.name << media.mediaId << media.containerId << media.mediaType;
            item.setDisplayName(media.name);
            if (media.mediaType == MEDIA_TYPE_CONTAINER) {
                item.setId("container=" + media.containerId + "&" + sourceId);
            } else {
                item.setId(media.mediaId);
            }
            item.setThumbnail(media.imageUrl);
            item.setExecutable(media.isPlayable);
            item.setBrowsable(media.isContainer);
            //item.setActionTypeIds();
            m_mediaObjects.insert(item.id(), media);
            result->addItem(item);
        }
        foreach(MusicSourceObject source, musicSources) {
            MediaBrowserItem item;
            item.setDisplayName(source.name);
            qDebug(dcDenon()) << "Adding Item" << source.name << source.sourceId;
            item.setId("source=" + QString::number(source.sourceId));
            item.setIcon(BrowserItem::BrowserIconMusic);
            item.setExecutable(false);
            item.setBrowsable(true);
            if (source.name.contains("Amazon")) {
                item.setMediaIcon(MediaBrowserItem::MediaBrowserIconAmazon);
                //result->addItem(item);
            } else if (source.name == "Deezer") {
                item.setMediaIcon(MediaBrowserItem::MediaBrowserIconDeezer);
                //result->addItem(item);
            } else if (source.name == "Napster") {
                item.setMediaIcon(MediaBrowserItem::MediaBrowserIconNapster);
                //result->addItem(item);
            } else if (source.name == "SoundCloud") {
                item.setMediaIcon(MediaBrowserItem::MediaBrowserIconSoundCloud);
                //result->addItem(item);
            } else if (source.name == "Tidal") {
                item.setMediaIcon(MediaBrowserItem::MediaBrowserIconTidal);
                //result->addItem(item);
            } else if (source.name == "TuneIn") {
                item.setMediaIcon(MediaBrowserItem::MediaBrowserIconTuneIn);
                result->addItem(item);
            } else if (source.name == "Local Music") {
                item.setMediaIcon(MediaBrowserItem::MediaBrowserIconDisk);
                //result->addItem(item);
            } else if (source.name == "Playlists") {
                item.setMediaIcon(MediaBrowserItem::MediaBrowserIconPlaylist);
                //result->addItem(item);
            } else if (source.name == "History") {
                item.setMediaIcon(MediaBrowserItem::MediaBrowserIconRecentlyPlayed);
                item.setBrowsable(loggedIn);
                if (!loggedIn) {
                    item.setDescription("Login required");
                }
            } else if (source.name == "AUX Input") {
                item.setMediaIcon(MediaBrowserItem::MediaBrowserIconAux);
                //result->addItem(item);
            } else if (source.name == "Favorites") {
                item.setIcon(BrowserItem::BrowserIconFavorites);
                item.setBrowsable(loggedIn);
                if (!loggedIn) {
                    item.setDescription("Login required");
                }
                result->addItem(item);
            } else {
                item.setThumbnail(source.image_url);
            }
        }
        result->finish(Thing::ThingErrorNoError);
    } else {
        qWarning(dcDenon()) << "Pending browser result doesnt recognize" << identifier << m_pendingBrowseResult.keys();
    }
}

void IntegrationPluginDenon::onHeosBrowseErrorReceived(const QString &sourceId, const QString &containerId, int errorId, const QString &errorMessage)
{
    QString identifier;
    if (containerId.isEmpty()) {
        identifier = sourceId;
    } else {
        identifier = containerId;
    }

    if (m_pendingBrowseResult.contains(identifier)) {
        BrowseResult *result = m_pendingBrowseResult.take(identifier);
        qWarning(dcDenon) << "Browse error" << errorMessage << errorId;
        result->finish(Thing::ThingErrorHardwareFailure, errorMessage);
    }
}

void IntegrationPluginDenon::onHeosPlayerNowPlayingChanged(int playerId)
{
    Heos *heos = static_cast<Heos *>(sender());
    heos->getNowPlayingMedia(playerId);
}

void IntegrationPluginDenon::onHeosPlayerQueueChanged(int playerId)
{
    Heos *heos = static_cast<Heos *>(sender());
    heos->getNowPlayingMedia(playerId);
}

void IntegrationPluginDenon::onHeosGroupsReceived(QList<GroupObject> groups)
{
    m_groupBuffer.clear();
    foreach(GroupObject group, groups) {
        m_groupBuffer.insert(group.groupId, group);
    }
}

void IntegrationPluginDenon::onHeosGroupsChanged()
{
    Heos *heos = static_cast<Heos *>(sender());
    heos->getGroups();
    heos->getPlayers();
}

void IntegrationPluginDenon::onHeosUserChanged(bool signedIn, const QString &userName)
{
    Q_UNUSED(userName)
    Heos *heos = static_cast<Heos *>(sender());

    //This is to check if the credentials are correct
    if (m_unfinishedHeosPairings.contains(heos)) {
        ThingPairingInfo *info = m_unfinishedHeosPairings.take(heos);
        if (signedIn) {
            info->finish(Thing::ThingErrorNoError);
        } else {
            info->finish(Thing::ThingErrorAuthenticationFailure, tr("Wrong username or password"));
            m_unfinishedHeosConnections.remove(info->thingId());
            heos->deleteLater();
        }
    } else if (m_heosConnections.values().contains(heos)) {
        Thing *thing = myThings().findById(m_heosConnections.key(heos));
        thing->setStateValue(heosLoggedInStateTypeId, signedIn);
        thing->setStateValue(heosUserDisplayNameStateTypeId, userName);
    } else {
        qCDebug(dcDenon()) << "Unhandled user changed event" << signedIn << userName;
    }
}

void IntegrationPluginDenon::onPluginConfigurationChanged(const ParamTypeId &paramTypeId, const QVariant &value)
{
    qCDebug(dcDenon()) << "Plugin configuration changed";

    // Check advanced mode
    if (paramTypeId == denonPluginNotificationUrlParamTypeId) {
        qCDebug(dcDenon()) << "Advanced mode" << (value.toBool() ? "enabled." : "disabled.");
        m_notificationUrl = value.toUrl();
    }
}

void IntegrationPluginDenon::browseThing(BrowseResult *result)
{
    Heos *heos = m_heosConnections.value(result->thing()->parentId());
    if (!heos) {
        result->finish(Thing::ThingErrorHardwareNotAvailable);
        return;
    }

    if (result->itemId().isEmpty()) {
        qDebug(dcDenon()) << "Browse source";
        MediaBrowserItem item;
        item.setId("type=group");
        item.setIcon(BrowserItem::BrowserIcon::BrowserIconPackage);
        item.setBrowsable(true);
        item.setExecutable(false);
        item.setDisplayName("Groups");
        result->addItem(item);
        heos->getMusicSources();
        m_pendingGetSourcesRequest.insert(heos, result);
        connect(result, &QObject::destroyed, this, [this, heos](){m_pendingGetSourcesRequest.remove(heos);});
    }

    QUrlQuery itemQuery(result->itemId());
    if (itemQuery.queryItemValue("type") == "group"){
        if (itemQuery.hasQueryItem("group")) {
            //TBD list players in groups
        } else {
            qDebug(dcDenon()) << "Browse source" << result->itemId();
            int pid = result->thing()->paramValue(heosPlayerThingPlayerIdParamTypeId).toInt();
            HeosPlayer *browsingPlayer = m_playerBuffer.value(pid);
            foreach (GroupObject group, m_groupBuffer) {
                MediaBrowserItem item;
                item.setBrowsable(false);
                item.setExecutable(false);
                item.setMediaIcon(MediaBrowserItem::MediaBrowserIconNone);
                item.setIcon(BrowserItem::BrowserIconPackage);
                item.setDisplayName(group.name);
                item.setId(result->itemId() + "&" + "group=" + QString::number(group.groupId));
                // if player is already part of the group set action type id to unjoin
                if (browsingPlayer->groupId() == group.groupId) {
                    item.setActionTypeIds(QList<ActionTypeId>() << heosPlayerUnjoinBrowserItemActionTypeId);
                } else {
                    item.setActionTypeIds(QList<ActionTypeId>() << heosPlayerJoinBrowserItemActionTypeId);
                }
                result->addItem(item);
            }

            foreach (HeosPlayer *player, m_playerBuffer.values()) {
                qDebug(dcDenon) << "Adding group item" << player->name();
                if (browsingPlayer->playerId() == player->playerId()) { //player is the current browsing thing
                    continue;
                }
                if (player->groupId() != -1) { // Dont display players that are already assigned to a group
                    continue;
                }
                MediaBrowserItem item;
                item.setBrowsable(false);
                item.setExecutable(false);
                item.setMediaIcon(MediaBrowserItem::MediaBrowserIconMusicLibrary);
                item.setIcon(BrowserItem::BrowserIconFile);
                item.setDisplayName(player->name());
                item.setId(result->itemId() + "&player=" + QString::number(player->playerId()));
                item.setActionTypeIds(QList<ActionTypeId>() << heosPlayerJoinBrowserItemActionTypeId);
                result->addItem(item);

            }
            result->finish(Thing::ThingErrorNoError);
        }

    } else if (result->itemId().startsWith("source=")){
        qDebug(dcDenon()) << "Browse source" << result->itemId();
        QString id = result->itemId().remove("source=");
        heos->browseSource(id);
        m_pendingBrowseResult.insert(id, result);
        connect(result, &QObject::destroyed, this, [this, id](){ m_pendingBrowseResult.remove(id);});

    } else if (result->itemId().startsWith("container=")){
        qDebug(dcDenon()) << "Browse container" << result->itemId();
        QStringList values = result->itemId().split("&");
        if (values.length() == 2) {
            QString id = values[0].remove("container=");
            heos->browseSourceContainers(values[1], id);
            // URL encoding is needed because some container ids are a URL and their encoding varies.
            if (QUrl(id).isValid()) {
                id = QUrl::fromPercentEncoding(id.toUtf8());
            }
            m_pendingBrowseResult.insert(id, result);
            connect(result, &QObject::destroyed, this, [this, id](){ m_pendingBrowseResult.remove(id);});
        }
    }
}

void IntegrationPluginDenon::browserItem(BrowserItemResult *result)
{
    Heos *heos = m_heosConnections.value(result->thing()->parentId());
    if (!heos) {
        result->finish(Thing::ThingErrorHardwareNotAvailable);
        return;
    }
    qDebug(dcDenon()) << "Browse item called" << result->itemId();
    
    BrowserItem item(result->itemId());
    return result->finish(Thing::ThingErrorNoError);
}

void IntegrationPluginDenon::executeBrowserItem(BrowserActionInfo *info)
{
    Heos *heos = m_heosConnections.value(info->thing()->parentId());
    if (!heos) {
        info->finish(Thing::ThingErrorHardwareNotAvailable);
        return;
    }
    BrowserAction action = info->browserAction();
    int playerId = info->thing()->paramValue(heosPlayerThingPlayerIdParamTypeId).toInt();
    qDebug(dcDenon()) << "Execute browse item called. Player Id:" << playerId << "Item ID" << action.itemId();

    if (m_mediaObjects.contains(action.itemId())) {
        MediaObject media = m_mediaObjects.value(action.itemId());
        if (media.mediaType == MEDIA_TYPE_CONTAINER) {
            heos->addContainerToQueue(playerId, media.sourceId, media.containerId, ADD_CRITERIA_PLAY_NOW);
        } else if (media.mediaType == MEDIA_TYPE_STATION) {
            heos->playStation(playerId, media.sourceId, media.containerId, media.mediaId, media.name);
        }
    } else {
        qWarning(dcDenon()) << "Media item not found" << action.itemId();
    }

    info->finish(Thing::ThingErrorNoError);
    return;
}

void IntegrationPluginDenon::executeBrowserItemAction(BrowserItemActionInfo *info)
{
    Heos *heos = m_heosConnections.value(info->thing()->parentId());
    if (!heos) {
        info->finish(Thing::ThingErrorHardwareNotAvailable);
        return;
    }

    QUrlQuery query(info->browserItemAction().itemId());
    if (info->browserItemAction().actionTypeId() == heosPlayerJoinBrowserItemActionTypeId) {
        if (query.hasQueryItem("player")) {
            QList<int> playerIds;
            playerIds.append(query.queryItemValue("player").toInt());
            playerIds.append(info->thing()->paramValue(heosPlayerThingPlayerIdParamTypeId).toInt());
            heos->setGroup(playerIds);
        } else if(query.hasQueryItem("group")) {

            GroupObject group = m_groupBuffer.value(query.queryItemValue("group").toInt());
            qDebug(dcDenon()) << "Execute browse item action called, Group:" << query.queryItemValue("group").toInt() << group.name;
            QList<int> playerIds;
            foreach(PlayerObject player, group.players) {
                playerIds.append(player.playerId);
            }
            playerIds.append(info->thing()->paramValue(heosPlayerThingPlayerIdParamTypeId).toInt());
            heos->setGroup(playerIds);
        }
    } else if (info->browserItemAction().actionTypeId() == heosPlayerUnjoinBrowserItemActionTypeId) {
        if(query.hasQueryItem("group")) {
            GroupObject group = m_groupBuffer.value(query.queryItemValue("group").toInt());
            QList<int> playerIds;
            foreach(PlayerObject player, group.players) {
                if (player.playerId != info->thing()->paramValue(heosPlayerThingPlayerIdParamTypeId).toInt())
                    playerIds.append(player.playerId);
            }
            heos->setGroup(playerIds);
        }
    }
    info->finish(Thing::ThingErrorNoError);
    return;
}

Heos *IntegrationPluginDenon::createHeosConnection(const QHostAddress &address)
{
    Heos *heos = new Heos(address, this);
    connect(heos, &Heos::connectionStatusChanged, this, &IntegrationPluginDenon::onHeosConnectionChanged);
    connect(heos, &Heos::playersChanged, this, &IntegrationPluginDenon::onHeosPlayersChanged);
    connect(heos, &Heos::playersRecieved, this, &IntegrationPluginDenon::onHeosPlayersReceived);
    connect(heos, &Heos::playerInfoRecieved, this, &IntegrationPluginDenon::onHeosPlayerInfoRecieved);
    connect(heos, &Heos::playerPlayStateReceived, this, &IntegrationPluginDenon::onHeosPlayStateReceived);
    connect(heos, &Heos::playerRepeatModeReceived, this, &IntegrationPluginDenon::onHeosRepeatModeReceived);
    connect(heos, &Heos::playerShuffleModeReceived, this, &IntegrationPluginDenon::onHeosShuffleModeReceived);
    connect(heos, &Heos::playerMuteStatusReceived, this, &IntegrationPluginDenon::onHeosMuteStatusReceived);
    connect(heos, &Heos::playerVolumeReceived, this, &IntegrationPluginDenon::onHeosVolumeStatusReceived);
    connect(heos, &Heos::nowPlayingMediaStatusReceived, this, &IntegrationPluginDenon::onHeosNowPlayingMediaStatusReceived);
    connect(heos, &Heos::playerNowPlayingChanged, this, &IntegrationPluginDenon::onHeosPlayerNowPlayingChanged);
    connect(heos, &Heos::musicSourcesReceived, this, &IntegrationPluginDenon::onHeosMusicSourcesReceived);
    connect(heos, &Heos::browseRequestReceived, this, &IntegrationPluginDenon::onHeosBrowseRequestReceived);
    connect(heos, &Heos::browseErrorReceived, this, &IntegrationPluginDenon::onHeosBrowseErrorReceived);
    connect(heos, &Heos::playerQueueChanged, this, &IntegrationPluginDenon::onHeosPlayerQueueChanged);
    connect(heos, &Heos::groupsReceived, this, &IntegrationPluginDenon::onHeosGroupsReceived);
    connect(heos, &Heos::groupsChanged, this, &IntegrationPluginDenon::onHeosGroupsChanged);
    connect(heos, &Heos::userChanged, this, &IntegrationPluginDenon::onHeosUserChanged);
    return heos;
}
