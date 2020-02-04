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
            //: Error discovering Denon devices
            info->finish(Thing::ThingErrorHardwareNotAvailable, QT_TR_NOOP("Device discovery is not available."));
            return;
        }

        if (!m_serviceBrowser) {
            m_serviceBrowser = hardwareManager()->zeroConfController()->createServiceBrowser();
            connect(m_serviceBrowser, &ZeroConfServiceBrowser::serviceEntryAdded, this, &IntegrationPluginDenon::onAvahiServiceEntryAdded);
            connect(m_serviceBrowser, &ZeroConfServiceBrowser::serviceEntryRemoved, this, &IntegrationPluginDenon::onAvahiServiceEntryRemoved);
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
<<<<<<< HEAD:denon/integrationplugindenon.cpp
    }

    if (info->thingClassId() == heosThingClassId) {
=======
    } else if (info->deviceClassId() == heosDeviceClassId) {
>>>>>>> tryna to fix discovery:denon/deviceplugindenon.cpp
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

            foreach (const UpnpDeviceDescriptor &upnpDevice, reply->deviceDescriptors()) {
                qCDebug(dcDenon) << "UPnP thing found:" << upnpDevice.modelDescription() << upnpDevice.friendlyName() << upnpDevice.hostAddress().toString() << upnpDevice.modelName() << upnpDevice.manufacturer() << upnpDevice.serialNumber();

                if (upnpDevice.modelName().contains("HEOS", Qt::CaseSensitivity::CaseInsensitive)) {
                    QString serialNumber = upnpDevice.serialNumber();
                    if (serialNumber != "0000001") {
                        // child devices have serial number 0000001
<<<<<<< HEAD:denon/integrationplugindenon.cpp
                        qCDebug(dcDenon) << "UPnP thing found:" << upnpDevice.modelDescription() << upnpDevice.friendlyName() << upnpDevice.hostAddress().toString() << upnpDevice.modelName() << upnpDevice.manufacturer() << upnpDevice.serialNumber();
                        ThingDescriptor descriptor(heosThingClassId, upnpDevice.modelName(), serialNumber);
=======
                        DeviceDescriptor descriptor(heosDeviceClassId, upnpDevice.modelName(), serialNumber);
>>>>>>> tryna to fix discovery:denon/deviceplugindenon.cpp
                        ParamList params;
                        foreach (Thing *existingThing, myThings()) {
                            if (existingThing->paramValue(heosThingSerialNumberParamTypeId).toString().contains(serialNumber, Qt::CaseSensitivity::CaseInsensitive)) {
                                descriptor.setThingId(existingThing->id());
                                break;
                            }
                        }
                        params.append(Param(heosThingModelNameParamTypeId, upnpDevice.modelName()));
                        params.append(Param(heosThingIpParamTypeId, upnpDevice.hostAddress().toString()));
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
        info->finish(Device::DeviceErrorDeviceClassNotFound);
    }
<<<<<<< HEAD:denon/integrationplugindenon.cpp
    info->finish(Thing::ThingErrorThingClassNotFound);
=======
>>>>>>> tryna to fix discovery:denon/deviceplugindenon.cpp
}

void IntegrationPluginDenon::setupThing(ThingSetupInfo *info)
{
    Thing *thing = info->thing();

    if (thing->thingClassId() == AVRX1000ThingClassId) {
        qCDebug(dcDenon) << "Setup Denon device" << thing->paramValue(AVRX1000ThingIpParamTypeId).toString();

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
        connect(info, &QObject::destroyed, this, [this, info, denonConnection]() { m_asyncAvrSetups.remove(denonConnection); });

        denonConnection->connectDevice();
        return;

    } else if (device->deviceClassId() == heosDeviceClassId) {
        qCDebug(dcDenon) << "Setup Denon device" << device->paramValue(heosDeviceIpParamTypeId).toString();

        QHostAddress address(device->paramValue(heosDeviceIpParamTypeId).toString());
        if (address.isNull()) {
            qCWarning(dcDenon) << "Could not parse ip address" << device->paramValue(heosDeviceIpParamTypeId).toString();
            info->finish(Device::DeviceErrorInvalidParameter, QT_TR_NOOP("The given IP address is not valid."));
            return;
        }

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

        m_heos.insert(device->id(), heos);

        m_asyncHeosSetups.insert(heos, info);
        // In case the setup is cancelled before we finish it...
        connect(info, &QObject::destroyed, this, [this, info, heos]() { m_asyncHeosSetups.remove(heos); });

        heos->connectDevice();
        return;
    }

    if (thing->thingClassId() == heosPlayerThingClassId) {
        info->finish(Thing::ThingErrorNoError);
        return;
    }
    info->finish(Thing::ThingErrorThingClassNotFound);
}

void IntegrationPluginDenon::thingRemoved(Thing *thing)
{
    qCDebug(dcDenon) << "Delete " << thing->name();

  AvrConnection *denonConnection = m_avrConnections.take(thing->id());

    if (thing->thingClassId() == AVRX1000ThingClassId) {
        if (m_avrConnections.contains(thing->id())) {
            AvrConnection *denonConnection = m_avrConnections.take(thing->id());
            denonConnection->disconnectDevice();
            denonConnection->deleteLater();
        }
    } else if (thing->thingClassId() == heosThingClassId) {
        if (m_heos.contains(thing->id())) {
            Heos *heos = m_heos.take(thing->id());
            heos->deleteLater();
        }
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
<<<<<<< HEAD:denon/integrationplugindenon.cpp
        return info->finish(Thing::ThingErrorActionTypeNotFound);
    }

    if (thing->thingClassId() == heosPlayerThingClassId) {
=======
        return info->finish(Device::DeviceErrorActionTypeNotFound);
    } else if (device->deviceClassId() == heosPlayerDeviceClassId) {
>>>>>>> tryna to fix discovery:denon/deviceplugindenon.cpp

        Thing *heosThing = myThings().findById(thing->parentId());
        Heos *heos = m_heos.value(heosThing->id());
        int playerId = thing->paramValue(heosPlayerThingPlayerIdParamTypeId).toInt();

        if (action.actionTypeId() == heosPlayerAlertActionTypeId) {
            heos->playUrl(playerId, m_notificationUrl);
            return info->finish(Device::DeviceErrorNoError);
        } else if (action.actionTypeId() == heosPlayerVolumeActionTypeId) {
            int volume = action.param(heosPlayerVolumeActionVolumeParamTypeId).value().toInt();
            heos->setVolume(playerId, volume);
<<<<<<< HEAD:denon/integrationplugindenon.cpp
            return info->finish(Thing::ThingErrorNoError);
        }

        if (action.actionTypeId() == heosPlayerMuteActionTypeId) {
            bool mute = action.param(heosPlayerMuteActionMuteParamTypeId).value().toBool();
            heos->setMute(playerId, mute);
            return info->finish(Thing::ThingErrorNoError);
        }

        if (action.actionTypeId() == heosPlayerPlaybackStatusActionTypeId) {
=======
            return info->finish(Device::DeviceErrorNoError);
        } else if (action.actionTypeId() == heosPlayerMuteActionTypeId) {
            bool mute = action.param(heosPlayerMuteActionMuteParamTypeId).value().toBool();
            heos->setMute(playerId, mute);
            return info->finish(Device::DeviceErrorNoError);
        } else if (action.actionTypeId() == heosPlayerPlaybackStatusActionTypeId) {
>>>>>>> tryna to fix discovery:denon/deviceplugindenon.cpp
            QString playbackStatus = action.param(heosPlayerPlaybackStatusActionPlaybackStatusParamTypeId).value().toString();
            if (playbackStatus == "playing") {
                heos->setPlayerState(playerId, PLAYER_STATE_PLAY);
            } else if (playbackStatus == "stopping") {
                heos->setPlayerState(playerId, PLAYER_STATE_STOP);
            } else if (playbackStatus == "pausing") {
                heos->setPlayerState(playerId, PLAYER_STATE_PAUSE);
            }
<<<<<<< HEAD:denon/integrationplugindenon.cpp
            return info->finish(Thing::ThingErrorNoError);
        }

        if (action.actionTypeId() == heosPlayerShuffleActionTypeId) {
=======
            return info->finish(Device::DeviceErrorNoError);
        } else if (action.actionTypeId() == heosPlayerShuffleActionTypeId) {
>>>>>>> tryna to fix discovery:denon/deviceplugindenon.cpp
            bool shuffle = action.param(heosPlayerShuffleActionShuffleParamTypeId).value().toBool();
            REPEAT_MODE repeatMode = REPEAT_MODE_OFF;
            if (thing->stateValue(heosPlayerRepeatStateTypeId) == "One") {
                repeatMode = REPEAT_MODE_ONE;
            } else if (thing->stateValue(heosPlayerRepeatStateTypeId) == "All") {
                repeatMode = REPEAT_MODE_ALL;
            }
            heos->setPlayMode(playerId, repeatMode, shuffle);
<<<<<<< HEAD:denon/integrationplugindenon.cpp
            return info->finish(Thing::ThingErrorNoError);
        }

        if (action.actionTypeId() == heosPlayerSkipBackActionTypeId) {
            heos->playPrevious(playerId);
            return info->finish(Thing::ThingErrorNoError);
        }

        if (action.actionTypeId() == heosPlayerStopActionTypeId) {
            heos->setPlayerState(playerId, PLAYER_STATE_STOP);
            return info->finish(Thing::ThingErrorNoError);
        }

        if (action.actionTypeId() == heosPlayerPlayActionTypeId) {
            heos->setPlayerState(playerId, PLAYER_STATE_PLAY);
            return info->finish(Thing::ThingErrorNoError);
        }

        if (action.actionTypeId() == heosPlayerPauseActionTypeId) {
            heos->setPlayerState(playerId, PLAYER_STATE_PAUSE);
            return info->finish(Thing::ThingErrorNoError);
        }

        if (action.actionTypeId() == heosPlayerSkipNextActionTypeId) {
            heos->playNext(playerId);
            return info->finish(Thing::ThingErrorNoError);
        }
        return info->finish(Thing::ThingErrorActionTypeNotFound);
    }
    return info->finish(Thing::ThingErrorThingClassNotFound);
=======
            return info->finish(Device::DeviceErrorNoError);
        } else if (action.actionTypeId() == heosPlayerSkipBackActionTypeId) {
            heos->playPrevious(playerId);
            return info->finish(Device::DeviceErrorNoError);
        } else if (action.actionTypeId() == heosPlayerStopActionTypeId) {
            heos->setPlayerState(playerId, PLAYER_STATE_STOP);
            return info->finish(Device::DeviceErrorNoError);
        } else if (action.actionTypeId() == heosPlayerPlayActionTypeId) {
            heos->setPlayerState(playerId, PLAYER_STATE_PLAY);
            return info->finish(Device::DeviceErrorNoError);
        } else if (action.actionTypeId() == heosPlayerPauseActionTypeId) {
            heos->setPlayerState(playerId, PLAYER_STATE_PAUSE);
            return info->finish(Device::DeviceErrorNoError);
        } else if (action.actionTypeId() == heosPlayerSkipNextActionTypeId) {
            heos->playNext(playerId);
            return info->finish(Device::DeviceErrorNoError);
        } else {
            return info->finish(Device::DeviceErrorActionTypeNotFound);
        }
    } else {
        return info->finish(Device::DeviceErrorDeviceClassNotFound);
    }
>>>>>>> tryna to fix discovery:denon/deviceplugindenon.cpp
}

void IntegrationPluginDenon::postSetupThing(Thing *thing)
{
    if (thing->thingClassId() == heosThingClassId) {
        Heos *heos = m_heos.value(thing->id());
        device->setStateValue(heosConnectedStateTypeId, heos->connected());
        heos->getPlayers();
        heos->getGroups();
<<<<<<< HEAD:denon/integrationplugindenon.cpp
    }

    if (thing->thingClassId() == heosPlayerThingClassId) {
        thing->setStateValue(heosPlayerConnectedStateTypeId, true);
        Thing *heosThing = myThings().findById(thing->parentId());
        Heos *heos = m_heos.value(heosThing->id());
        int playerId = thing->paramValue(heosPlayerThingPlayerIdParamTypeId).toInt();

=======
    } else if (device->deviceClassId() == heosPlayerDeviceClassId) {
        device->setStateValue(heosPlayerConnectedStateTypeId, true);
        Device *heosDevice = myDevices().findById(device->parentId());
        Heos *heos = m_heos.value(heosDevice->id());
        int playerId = device->paramValue(heosPlayerDevicePlayerIdParamTypeId).toInt();
>>>>>>> tryna to fix discovery:denon/deviceplugindenon.cpp
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
            Heos *heos = m_heos.value(thing->id());
            heos->getPlayers();
            heos->registerForChangeEvents(true);
<<<<<<< HEAD:denon/integrationplugindenon.cpp
        }

        if (thing->thingClassId() == heosPlayerThingClassId) {
            Thing *heosThing = myThings().findById(thing->parentId());
            Heos *heos = m_heos.value(heosThing->id());
            int playerId = thing->paramValue(heosPlayerThingPlayerIdParamTypeId).toInt();
=======
        } else if (device->deviceClassId() == heosPlayerDeviceClassId) {
            Device *heosDevice = myDevices().findById(device->parentId());
            Heos *heos = m_heos.value(heosDevice->id());
            int playerId = device->paramValue(heosPlayerDevicePlayerIdParamTypeId).toInt();
>>>>>>> tryna to fix discovery:denon/deviceplugindenon.cpp

            heos->getPlayerState(playerId);
            heos->getPlayMode(playerId);
            heos->getVolume(playerId);
            heos->getMute(playerId);
            heos->getNowPlayingMedia(playerId);
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
    Thing *thing  =myThings().findById(m_avrConnections.key(denonConnection));
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
<<<<<<< HEAD:denon/integrationplugindenon.cpp

    Thing *thing = myThings().findById(m_heos.key(heos));
    if (!thing)
        return;

    if (thing->thingClassId() == heosThingClassId) {
        // if the thing is connected
        if (status) {
            // and from the first setup
            if (m_asyncHeosSetups.contains(heos)) {
                ThingSetupInfo *info = m_asyncHeosSetups.take(heos);
                heos->getPlayers();
                info->finish(Thing::ThingErrorNoError);
            }
        }
        thing->setStateValue(heosConnectedStateTypeId, status);
=======
    if (status) {
        // and from the first setup
        if (m_asyncHeosSetups.contains(heos)) {
            DeviceSetupInfo *info = m_asyncHeosSetups.take(heos);
            info->finish(Device::DeviceErrorNoError);
        }
    }

    Device *device = myDevices().findById(m_heos.key(heos));
    if (!device)
        return;

    if (device->deviceClassId() == heosDeviceClassId) {

        device->setStateValue(heosConnectedStateTypeId, status);
>>>>>>> added join/unjoin group:denon/IntegrationPlugindenon.cpp
        // update connection status for all child devices
        foreach (Thing *playerDevice, myThings()) {
            if (playerDevice->thingClassId() == heosPlayerThingClassId) {
                if (playerDevice->parentId() == thing->id()) {
                    playerDevice->setStateValue(heosPlayerConnectedStateTypeId, status);
                }
            }
        }
    }
}

void IntegrationPluginDenon::onHeosPlayersChanged()
{
    Heos *heos = static_cast<Heos *>(sender());
    heos->getPlayers();
}

<<<<<<< HEAD:denon/integrationplugindenon.cpp
void IntegrationPluginDenon::onHeosPlayerDiscovered(HeosPlayer *heosPlayer) {
=======
void IntegrationPluginDenon::onHeosPlayersReceived(QList<HeosPlayer *> heosPlayers) {

>>>>>>> added join/unjoin group:denon/IntegrationPlugindenon.cpp
    Heos *heos = static_cast<Heos *>(sender());

<<<<<<< HEAD:denon/integrationplugindenon.cpp
    Thing *thing = myThings().findById(m_heos.key(heos));

    foreach (Thing *heosPlayerThing, myThings()) {
        if(heosPlayerThing->thingClassId() == heosPlayerThingClassId) {
            if (heosPlayerThing->paramValue(heosPlayerThingPlayerIdParamTypeId).toInt() == heosPlayer->playerId())
                return;
=======
    QList<DeviceDescriptor> heosPlayerDescriptors;
    foreach (HeosPlayer *player, heosPlayers) {
        DeviceDescriptor descriptor(heosPlayerDeviceClassId, player->name(), player->playerModel(), device->id());
        ParamList params;
        if (!myDevices().filterByParam(heosPlayerDevicePlayerIdParamTypeId, player->playerId()).isEmpty()) {
            continue;
>>>>>>> added join/unjoin group:denon/IntegrationPlugindenon.cpp
        }
        params.append(Param(heosPlayerDeviceModelParamTypeId, player->playerModel()));
        params.append(Param(heosPlayerDevicePlayerIdParamTypeId, player->playerId()));
        params.append(Param(heosPlayerDeviceSerialNumberParamTypeId, player->serialNumber()));
        params.append(Param(heosPlayerDeviceVersionParamTypeId, player->playerVersion()));
        descriptor.setParams(params);
        qCDebug(dcDenon) << "Found new heos player" << player->name();
        heosPlayerDescriptors.append(descriptor);
    }
<<<<<<< HEAD:denon/integrationplugindenon.cpp
    QList<ThingDescriptor> heosPlayerDescriptors;
    ThingDescriptor descriptor(heosPlayerThingClassId, heosPlayer->name(), heosPlayer->playerModel(), thing->id());
    ParamList params;
    params.append(Param(heosPlayerThingModelParamTypeId, heosPlayer->playerModel()));
    params.append(Param(heosPlayerThingPlayerIdParamTypeId, heosPlayer->playerId()));
    params.append(Param(heosPlayerThingSerialNumberParamTypeId, heosPlayer->serialNumber()));
    params.append(Param(heosPlayerThingVersionParamTypeId, heosPlayer->playerVersion()));
    descriptor.setParams(params);
    qCDebug(dcDenon) << "Found new heos player" << heosPlayer->name();
    heosPlayerDescriptors.append(descriptor);
    emit autoThingsAppeared(heosPlayerDescriptors);
}

void IntegrationPluginDenon::onHeosPlayStateReceived(int playerId, PLAYER_STATE state)
=======

    //TODO remove devices
    //TODO remove player from player Buffer
    autoDevicesAppeared(heosPlayerDescriptors);
}

void IntegrationPluginDenon::onHeosPlayerInfoRecieved(HeosPlayer *heosPlayer)
{
    qDebug(dcDenon()) << "Heos player info received" << heosPlayer->name() << heosPlayer->playerId() << heosPlayer->groupId();
    m_playerBuffer.insert(heosPlayer->playerId(), heosPlayer);
}

void IntegrationPluginDenon::onHeosPlayStateReceived(int playerId, PLAYER_STATE state)
>>>>>>> added join/unjoin group:denon/IntegrationPlugindenon.cpp
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

<<<<<<< HEAD:denon/integrationplugindenon.cpp
void IntegrationPluginDenon::onHeosNowPlayingMediaStatusReceived(int playerId, SOURCE_ID sourceId, QString artist, QString album, QString song, QString artwork)
{
    foreach(Thing *thing, myThings().filterByParam(heosPlayerThingPlayerIdParamTypeId, playerId)) {
        thing->setStateValue(heosPlayerArtistStateTypeId, artist);
        thing->setStateValue(heosPlayerTitleStateTypeId, song);
        thing->setStateValue(heosPlayerArtworkStateTypeId, artwork);
        thing->setStateValue(heosPlayerCollectionStateTypeId, album);
        QString source;
        switch (sourceId) {
        case SOURCE_ID_PANDORA:
            source = "Pandora";
            break;
        case SOURCE_ID_RHAPSODY:
            source = "Rhapsody";
            break;
        case SOURCE_ID_TUNEIN:
            source = "TuneIn";
            break;
        case SOURCE_ID_SPOTIFY:
            source = "Spotify";
            break;
        case SOURCE_ID_DEEZER:
            source = "Deezer";
            break;
        case SOURCE_ID_NAPSTER:
            source = "Napster";
            break;
        case SOURCE_ID_IHEARTRADIO:
            source = "iHeartRadio";
            break;
        case SOURCE_ID_SIRIUS_XM:
            source = "Sirius XM";
            break;
        case SOURCE_ID_SOUNDCLOUD:
            source = "Soundcloud";
            break;
        case SOURCE_ID_TIDAL:
            source = "Tidal";
            break;
        case SOURCE_ID_FUTURE_SERVICE_1:
            source = "Unknown";
            break;
        case SOURCE_ID_RDIO:
            source = "Rdio";
            break;
        case SOURCE_ID_AMAZON_MUSIC:
            source = "Amazon Music";
            break;
        case SOURCE_ID_FUTURE_SERVICE_2:
            source = "Unknown";
            break;
        case SOURCE_ID_MOODMIX:
            source = "Moodmix";
            break;
        case SOURCE_ID_JUKE:
            source = "Juke";
            break;
        case SOURCE_ID_FUTURE_SERVICE_3:
            source = "Unkown";
            break;
        case SOURCE_ID_QQMUSIC:
            source = "QQMusic";
            break;
        case SOURCE_ID_LOCAL_MEDIA:
            source = "USB Media/DLNA Servers";
            break;
        case SOURCE_ID_HEOS_PLAYLIST:
            source = "HEOS Playlists";
            break;
        case SOURCE_ID_HEOS_HISTORY:
            source = "HEOS History";
            break;
        case SOURCE_ID_HEOS_FAVORITES:
            source = "HEOS Favorites";
            break;
        case SOURCE_ID_HEOS_AUX:
            source = "HEOS aux input";
            break;
        };
        thing->setStateValue(heosPlayerSourceStateTypeId, source);
        break;
    }
=======
void IntegrationPluginDenon::onHeosNowPlayingMediaStatusReceived(int playerId, const QString &sourceId, const QString &artist, const QString &album, const QString &song, const QString &artwork)
{
    Device *device = myDevices().filterByParam(heosPlayerDevicePlayerIdParamTypeId, playerId).first();
    if (!device)
        return;

    device->setStateValue(heosPlayerArtistStateTypeId, artist);
    device->setStateValue(heosPlayerTitleStateTypeId, song);
    device->setStateValue(heosPlayerArtworkStateTypeId, artwork);
    device->setStateValue(heosPlayerCollectionStateTypeId, album);
    device->setStateValue(heosPlayerSourceStateTypeId, sourceId);
>>>>>>> added join/unjoin group:denon/IntegrationPlugindenon.cpp
}


void IntegrationPluginDenon::onHeosMusicSourcesReceived(QList<MusicSourceObject> musicSources)
{
    Heos *heos = static_cast<Heos *>(sender());
    if (m_pendingGetSourcesRequest.contains(heos)) {
        BrowseResult *result = m_pendingGetSourcesRequest.take(heos);
        foreach(MusicSourceObject source, musicSources) {
            MediaBrowserItem item;
            item.setDisplayName(source.name);
            item.setId("source=" + QString::number(source.sourceId));
            item.setExecutable(false);
            item.setBrowsable(true);
            item.setIcon(BrowserItem::BrowserIconMusic);
            if (source.name == "Amazon") {
                item.setMediaIcon(MediaBrowserItem::MediaBrowserIconAmazon);
            } else if (source.name == "Deezer") {
                item.setMediaIcon(MediaBrowserItem::MediaBrowserIconDeezer);
            } else if (source.name == "Napster") {
                item.setMediaIcon(MediaBrowserItem::MediaBrowserIconNapster);
            } else if (source.name == "SoundCloud") {
                item.setMediaIcon(MediaBrowserItem::MediaBrowserIconSoundCloud);
            } else if (source.name == "Tidal") {
                item.setMediaIcon(MediaBrowserItem::MediaBrowserIconTidal);
            } else if (source.name == "TuneIn") {
                item.setMediaIcon(MediaBrowserItem::MediaBrowserIconTuneIn);
            } else if (source.name == "Local Music") {
                item.setMediaIcon(MediaBrowserItem::MediaBrowserIconDisk);
            } else if (source.name == "Playlists") {
                item.setMediaIcon(MediaBrowserItem::MediaBrowserIconPlaylist);
            } else if (source.name == "History") {
                item.setMediaIcon(MediaBrowserItem::MediaBrowserIconRecentlyPlayed);
            } else if (source.name == "AUX Input") {
                item.setMediaIcon(MediaBrowserItem::MediaBrowserIconAux);
            } else if (source.name == "Favorites") {
                item.setIcon(BrowserItem::BrowserIconFavorites);
            } else {
                item.setThumbnail(source.image_url);
            }
            result->addItem(item);
            qDebug(dcDenon()) << "Music source received:" << source.name << source.type << source.sourceId << source.image_url;
        }
        result->finish(Device::DeviceErrorNoError);
    }
}

void IntegrationPluginDenon::onHeosMediaItemsReceived(QList<MediaObject> mediaItems)

void IntegrationPluginDenon::onHeosBrowseRequestReceived(const QString &sourceId, const QString &containerId, QList<MusicSourceObject> musicSources, QList<MediaObject> mediaItems)
{
    QString identifier;
    if (containerId.isEmpty()) {
        identifier = sourceId;
    } else {
        identifier = containerId;
    }
    if (QUrl(identifier).isValid()) {
        identifier = QUrl::fromPercentEncoding(identifier.toUtf8());
    }
}


void IntegrationPluginDenon::onHeosBrowseRequestReceived(QList<MusicSourceObject> musicSources, QList<MediaObject> mediaItems)
{
    Q_UNUSED(musicSources);
    Q_UNUSED(mediaItems);
    if (m_pendingBrowseResult.contains(browseRequest)) {
        BrowseResult *result = m_pendingBrowseResult.take(browseRequest);
        foreach(MediaObject media, mediaItems) {
            MediaBrowserItem item;
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
            if (source.name.contains("Amazon")) {
                item.setMediaIcon(MediaBrowserItem::MediaBrowserIconAmazon);
            } else if (source.name == "Deezer") {
                item.setMediaIcon(MediaBrowserItem::MediaBrowserIconDeezer);
            } else if (source.name == "Napster") {
                item.setMediaIcon(MediaBrowserItem::MediaBrowserIconNapster);
            } else if (source.name == "SoundCloud") {
                item.setMediaIcon(MediaBrowserItem::MediaBrowserIconSoundCloud);
            } else if (source.name == "Tidal") {
                item.setMediaIcon(MediaBrowserItem::MediaBrowserIconTidal);
            } else if (source.name == "TuneIn") {
                item.setMediaIcon(MediaBrowserItem::MediaBrowserIconTuneIn);
            } else if (source.name == "Local Music") {
                item.setMediaIcon(MediaBrowserItem::MediaBrowserIconDisk);
            } else if (source.name == "Playlists") {
                item.setMediaIcon(MediaBrowserItem::MediaBrowserIconPlaylist);
            } else if (source.name == "History") {
                item.setMediaIcon(MediaBrowserItem::MediaBrowserIconRecentlyPlayed);
            } else if (source.name == "AUX Input") {
                item.setMediaIcon(MediaBrowserItem::MediaBrowserIconAux);
            } else if (source.name == "Favorites") {
                item.setIcon(BrowserItem::BrowserIconFavorites);
            } else {
                item.setThumbnail(source.image_url);
            }
            item.setExecutable(false);
            item.setBrowsable(true);
            result->addItem(item);
        }
        result->finish(Device::DeviceErrorNoError);
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
        result->finish(Device::DeviceErrorHardwareFailure);
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

<<<<<<< HEAD:denon/integrationplugindenon.cpp
void IntegrationPluginDenon::onAvahiServiceEntryAdded(const ZeroConfServiceEntry &serviceEntry)
=======
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

void IntegrationPluginDenon::onAvahiServiceEntryAdded(const ZeroConfServiceEntry &serviceEntry)
>>>>>>> added join/unjoin group:denon/IntegrationPlugindenon.cpp
{
    qCDebug(dcDenon()) << "Avahi service entry added:" << serviceEntry;
}

void IntegrationPluginDenon::onAvahiServiceEntryRemoved(const ZeroConfServiceEntry &serviceEntry)
{
    qCDebug(dcDenon()) << "Avahi service entry removed:" << serviceEntry;
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


void IntegrationPluginDenon::browseDevice(BrowseResult *result)
{
    Heos *heos = m_heos.value(result->device()->parentId());
    if (!heos) {
        result->finish(Device::DeviceErrorHardwareNotAvailable);
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
            int pid = result->device()->paramValue(heosPlayerDevicePlayerIdParamTypeId).toInt();
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
                if (browsingPlayer->playerId() == player->playerId()) { //player is the current browsing device
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
            result->finish(Device::DeviceErrorNoError);
        }

    } else if (result->itemId().startsWith("source=")){
        qDebug(dcDenon()) << "Browse source" << result->itemId();
        QString id = result->itemId().remove("source=");
        heos->browseSource(id);
        m_pendingBrowseResult.insert(id, result);
        //connect(result, &QObject::destroyed, this, [this, result->itemId()](){ m_pendingBrowseResult.remove(result->itemId());});

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
    Heos *heos = m_heos.value(result->device()->parentId());
    if (!heos) {
        result->finish(Device::DeviceErrorHardwareNotAvailable);
        return;
    }
    qDebug(dcDenon()) << "Browse item called";
    return;
}

void IntegrationPluginDenon::executeBrowserItem(BrowserActionInfo *info)
{
    Heos *heos = m_heos.value(info->device()->parentId());
    if (!heos) {
        info->finish(Device::DeviceErrorHardwareNotAvailable);
        return;
    }
    BrowserAction action = info->browserAction();
    int playerId = info->device()->paramValue(heosPlayerDevicePlayerIdParamTypeId).toInt();
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

    info->finish(Device::DeviceErrorNoError);
    return;
}

void IntegrationPluginDenon::executeBrowserItemAction(BrowserItemActionInfo *info)
{
    Heos *heos = m_heos.value(info->device()->parentId());
    if (!heos) {
        info->finish(Device::DeviceErrorHardwareNotAvailable);
        return;
    }

    QUrlQuery query(info->browserItemAction().itemId());
    if (info->browserItemAction().actionTypeId() == heosPlayerJoinBrowserItemActionTypeId) {
        if (query.hasQueryItem("player")) {
            QList<int> playerIds;
            playerIds.append(query.queryItemValue("player").toInt());
            playerIds.append(info->device()->paramValue(heosPlayerDevicePlayerIdParamTypeId).toInt());
            heos->setGroup(playerIds);
        } else if(query.hasQueryItem("group")) {

            GroupObject group = m_groupBuffer.value(query.queryItemValue("group").toInt());
            qDebug(dcDenon()) << "Execute browse item action called, Group:" << query.queryItemValue("group").toInt() << group.name;
            QList<int> playerIds;
            foreach(PlayerObject player, group.players) {
                playerIds.append(player.playerId);
            }
            playerIds.append(info->device()->paramValue(heosPlayerDevicePlayerIdParamTypeId).toInt());
            heos->setGroup(playerIds);
        }
    } else if (info->browserItemAction().actionTypeId() == heosPlayerUnjoinBrowserItemActionTypeId) {
        if(query.hasQueryItem("group")) {
            GroupObject group = m_groupBuffer.value(query.queryItemValue("group").toInt());
            QList<int> playerIds;
            foreach(PlayerObject player, group.players) {
                if (player.playerId != info->device()->paramValue(heosPlayerDevicePlayerIdParamTypeId).toInt())
                    playerIds.append(player.playerId);
            }
            heos->setGroup(playerIds);
        }
    }
    info->finish(Device::DeviceErrorNoError);
    return;
}
