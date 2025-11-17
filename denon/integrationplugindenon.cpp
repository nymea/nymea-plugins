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

    m_serviceBrowser = hardwareManager()->zeroConfController()->createServiceBrowser();
    connect(m_serviceBrowser, &ZeroConfServiceBrowser::serviceEntryAdded, this, [=](const ZeroConfServiceEntry &entry){
        foreach (Thing *thing, myThings().filterByThingClassId(AVRX1000ThingClassId)) {

            if (entry.txt().contains("am=AVRX1000")) {
                QString existingId = thing->paramValue(AVRX1000ThingIdParamTypeId).toString();
                QString discoveredId = entry.name().split("@").first();
                QHostAddress address = entry.hostAddress();
                if (existingId == discoveredId && m_avrConnections.contains(thing->id())) {
                    AvrConnection *avrConnection = m_avrConnections.value(thing->id());
                    avrConnection->setHostAddress(address);
                }
            }
        }
    });
}

void IntegrationPluginDenon::discoverThings(ThingDiscoveryInfo *info)
{
    if (info->thingClassId() == AVRX1000ThingClassId) {

        if (!hardwareManager()->zeroConfController()->available()) {
            qCDebug(dcDenon()) << "Error discovering Denon things. Available:" << hardwareManager()->zeroConfController()->available();
            info->finish(Thing::ThingErrorHardwareNotAvailable, "Thing discovery not possible");
            return;
        }

        QStringList discoveredIds;

        foreach (const ZeroConfServiceEntry &service, m_serviceBrowser->serviceEntries()) {
            qCDebug(dcDenon()) << "mDNS service entry:" << service;
            if (service.txt().contains("am=AVRX1000")) {

                QString id = service.name().split("@").first();
                QString name = service.name().split("@").last();
                QString address = service.hostAddress().toString();
                qCDebug(dcDenon()) << "service discovered" << name << "ID:" << id;
                if (discoveredIds.contains(id))
                    break;
                discoveredIds.append(id);
                ThingDescriptor thingDescriptor(AVRX1000ThingClassId, name, address);
                ParamList params;
                params.append(Param(AVRX1000ThingIdParamTypeId, id));
                thingDescriptor.setParams(params);
                foreach (Thing *existingThing, myThings().filterByThingClassId(AVRX1000ThingClassId)) {
                    if (existingThing->paramValue(AVRX1000ThingIdParamTypeId).toString() == id) {
                        thingDescriptor.setThingId(existingThing->id());
                        break;
                    }
                }
                info->addThingDescriptor(thingDescriptor);
            }
        }
        info->finish(Thing::ThingErrorNoError);

    } else if (info->thingClassId() == heosThingClassId) {
        /*
        * The HEOS products can be discovered using the UPnP SSDP protocol. Through discovery,
        * the IP address of the HEOS products can be retrieved. Once the IP address is retrieved,
        * a telnet connection to port 1255 can be opened to access the HEOS CLI and control the HEOS system.
        * The HEOS product IP address can also be set statically and manually programmed into the control system.
        * Search target name (ST) in M-SEARCH discovery request is 'urn:schemas-denon-com:thing:ACT-Denon:1'.
        */
        if (!hardwareManager()->upnpDiscovery()->available()) {
            qCDebug(dcDenon()) << "UPnP discovery not available";
            info->finish(Thing::ThingErrorHardwareNotAvailable, "UPnP discovery not possible");
            return;
        }
        UpnpDiscoveryReply *reply = hardwareManager()->upnpDiscovery()->discoverDevices();
        connect(reply, &UpnpDiscoveryReply::finished, reply, &UpnpDiscoveryReply::deleteLater);
        connect(reply, &UpnpDiscoveryReply::finished, info, [this, reply, info](){

            if (reply->error() != UpnpDiscoveryReply::UpnpDiscoveryReplyErrorNoError) {
                qCWarning(dcDenon()) << "Upnp discovery error" << reply->error();
                info->finish(Thing::ThingErrorHardwareFailure, QT_TR_NOOP("UPnP discovery failed."));
                return;
            }
            m_heosIpAddresses.clear();
            foreach (const UpnpDeviceDescriptor &upnpThing, reply->deviceDescriptors()) {
                if (upnpThing.modelName().contains("HEOS", Qt::CaseSensitivity::CaseInsensitive) && upnpThing.serialNumber() != "0000001") {
                    // child things have serial number 0000001
                    qCDebug(dcDenon()) << "uPnP thing found:" << upnpThing.modelDescription() << upnpThing.friendlyName() << upnpThing.hostAddress().toString() << upnpThing.modelName() << upnpThing.manufacturer() << upnpThing.serialNumber();

                    m_heosIpAddresses.insert(upnpThing.serialNumber(), upnpThing.hostAddress());
                    ThingDescriptor descriptor(heosThingClassId, upnpThing.modelName(), upnpThing.serialNumber());
                    ParamList params;
                    foreach (Thing *existingThing, myThings()) {
                        if (existingThing->paramValue(heosThingSerialNumberParamTypeId).toString().contains(upnpThing.serialNumber(), Qt::CaseSensitivity::CaseInsensitive)) {
                            descriptor.setThingId(existingThing->id());
                            break;
                        }
                    }
                    params.append(Param(heosThingModelNameParamTypeId, upnpThing.modelName()));
                    params.append(Param(heosThingSerialNumberParamTypeId, upnpThing.serialNumber()));
                    descriptor.setParams(params);
                    info->addThingDescriptor(descriptor);
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

        Q_FOREACH(const QString &serialNumber, m_heosIpAddresses.keys()) {
            if (serialNumber == info->params().paramValue(heosThingSerialNumberParamTypeId).toString()) {
                ThingId thingId = info->thingId();
                Heos *heos = createHeosConnection(m_heosIpAddresses.value(serialNumber));
                m_unfinishedHeosConnections.insert(thingId, heos);
                m_unfinishedHeosPairings.insert(heos, info);
                connect(heos, &Heos::destroyed, this, [this, thingId, heos] {
                    qCDebug(dcDenon()) << "Heos connection deleted, cleaning up";
                    m_unfinishedHeosPairings.remove(heos);
                    m_unfinishedHeosConnections.remove(thingId);
                });
                connect(info, &ThingPairingInfo::aborted, this, [heos] {
                    qCDebug(dcDenon()) << "ThingPairingInfo aborted, deleting heos connection";
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
    }
}

void IntegrationPluginDenon::setupThing(ThingSetupInfo *info)
{
    Thing *thing = info->thing();

    if (thing->thingClassId() == AVRX1000ThingClassId) {
         qCDebug(dcDenon()) << "Setup AVR X1000 thing" << thing->name();

        if (m_avrConnections.contains(thing->id())) {
            qCDebug(dcDenon()) << "Setup after reconfiguration, cleaning up ...";
            m_avrConnections.take(thing->id())->deleteLater();

        }
        QString id = thing->paramValue(AVRX1000ThingIdParamTypeId).toString();
        QHostAddress address = findAvrById(id);
        if (address.isNull()) {
            info->finish(Thing::ThingErrorHardwareNotAvailable);
            return;
        }

        AvrConnection *denonConnection = new AvrConnection(address, 23, this);
        connect(denonConnection, &AvrConnection::connectionStatusChanged, this, &IntegrationPluginDenon::onAvrConnectionChanged);
        connect(denonConnection, &AvrConnection::socketErrorOccured, this, &IntegrationPluginDenon::onAvrSocketError);
        connect(denonConnection, &AvrConnection::commandExecuted, this, &IntegrationPluginDenon::onAvrCommandExecuted);
        connect(denonConnection, &AvrConnection::channelChanged, this, &IntegrationPluginDenon::onAvrChannelChanged);
        connect(denonConnection, &AvrConnection::powerChanged, this, &IntegrationPluginDenon::onAvrPowerChanged);
        connect(denonConnection, &AvrConnection::volumeChanged, this, &IntegrationPluginDenon::onAvrVolumeChanged);
        connect(denonConnection, &AvrConnection::surroundModeChanged, this, &IntegrationPluginDenon::onAvrSurroundModeChanged);
        connect(denonConnection, &AvrConnection::muteChanged, this, &IntegrationPluginDenon::onAvrMuteChanged);
        connect(denonConnection, &AvrConnection::artistChanged, this, &IntegrationPluginDenon::onAvrArtistChanged);
        connect(denonConnection, &AvrConnection::albumChanged, this, &IntegrationPluginDenon::onAvrAlbumChanged);
        connect(denonConnection, &AvrConnection::songChanged, this, &IntegrationPluginDenon::onAvrSongChanged);
        connect(denonConnection, &AvrConnection::playBackModeChanged, this, &IntegrationPluginDenon::onAvrPlayBackModeChanged);
        connect(denonConnection, &AvrConnection::bassLevelChanged, this, &IntegrationPluginDenon::onAvrBassLevelChanged);
        connect(denonConnection, &AvrConnection::trebleLevelChanged, this, &IntegrationPluginDenon::onAvrTrebleLevelChanged);
        connect(denonConnection, &AvrConnection::toneControlEnabledChanged, this, &IntegrationPluginDenon::onAvrToneControlEnabledChanged);

        m_avrConnections.insert(thing->id(), denonConnection);
        m_asyncAvrSetups.insert(denonConnection, info);
        // In case the setup is cancelled before we finish it...
        connect(info, &QObject::destroyed, this, [this, denonConnection]() { m_asyncAvrSetups.remove(denonConnection); });
        connect(info, &ThingSetupInfo::aborted, this, [this, thing] () {
            if (m_avrConnections.contains(thing->id())) {
                AvrConnection *connection = m_avrConnections.take(thing->id());
                connection->deleteLater();
            }
        });
        denonConnection->connectDevice();
        return;
    } else if (thing->thingClassId() == heosThingClassId) {

        qCDebug(dcDenon()) << "Setup Heos connection thing" << thing->name();
        QString serialnumber = thing->paramValue(heosThingSerialNumberParamTypeId).toString();
        if (serialnumber.isEmpty()) {
            qCWarning(dcDenon()) << "Serial number is empty";
            info->finish(Thing::ThingErrorInvalidParameter, QT_TR_NOOP("Serial number is not set"));
            return;
        }

        if (m_heosConnections.contains(thing->id())) {
            qCDebug(dcDenon()) << "Setup after reconfiguration, cleaning up ...";
            m_heosConnections.take(thing->id())->deleteLater();
        }

        if (m_unfinishedHeosConnections.contains(thing->id())) {
            qCDebug(dcDenon()) << "Setup after discovery";
            Heos *heos = m_unfinishedHeosConnections.take(thing->id());
            m_heosConnections.insert(thing->id(), heos);
            info->finish(Thing::ThingErrorNoError);
        } else {
            qCDebug(dcDenon()) << "Starting Heos discovery";
            if (!hardwareManager()->upnpDiscovery()->available()) {
                qCDebug(dcDenon()) << "UPnP discovery not available";
                info->finish(Thing::ThingErrorHardwareNotAvailable, "Discovery not possible");
                return;
            }
            UpnpDiscoveryReply *reply = hardwareManager()->upnpDiscovery()->discoverDevices();
            connect(reply, &UpnpDiscoveryReply::finished, reply, &UpnpDiscoveryReply::deleteLater);
            connect(reply, &UpnpDiscoveryReply::finished, info, [this, reply, info] {
                if (reply->error() != UpnpDiscoveryReply::UpnpDiscoveryReplyErrorNoError) {
                    qCWarning(dcDenon()) << "Upnp discovery error" << reply->error();
                    info->finish(Thing::ThingErrorHardwareFailure, QT_TR_NOOP("Device discovery failed."));
                    return;
                }
                qCDebug(dcDenon()) << "UPnP discovery finished, found" << reply->deviceDescriptors().count() << "uPnP devices";
                Q_FOREACH (const UpnpDeviceDescriptor &upnpThing, reply->deviceDescriptors()) {
                    if (upnpThing.modelName().contains("HEOS", Qt::CaseSensitivity::CaseInsensitive)) {
                        QString serialNumber =  info->thing()->paramValue(heosThingSerialNumberParamTypeId).toString();
                        if (serialNumber == upnpThing.serialNumber()) {
                            ThingId thingId = info->thing()->id();
                            qCDebug(dcDenon()) << "Found Heos device, creating Heos connection";
                            Heos *heos = createHeosConnection(upnpThing.hostAddress());
                            m_heosConnections.insert(thingId, heos);
                            m_asyncHeosSetups.insert(heos, info);
                            // In case the setup is cancelled before we finish it...
                            connect(info, &ThingSetupInfo::aborted, heos, &Heos::deleteLater);
                            connect(heos, &Heos::destroyed, this, [thingId, heos, this] {
                                m_asyncHeosSetups.remove(heos);
                                m_heosConnections.remove(thingId);
                            });
                            heos->connectDevice();
                            return;
                        }
                    }
                }
                qCDebug(dcDenon()) << "Device not found";
                info->finish(Thing::ThingErrorHardwareNotAvailable);
                return;
            });
        }
    } else if (thing->thingClassId() == heosPlayerThingClassId) {

        qCDebug(dcDenon()) << "Setup Heos player" << thing->name();
        Thing *parentThing = myThings().findById(thing->parentId());
        if (!parentThing) {
            qCWarning(dcDenon()) << "Parent thing not found for Heos player" << thing->name();
            return;
        }
        if (parentThing->setupStatus() == Thing::ThingSetupStatusComplete) {
            info->finish(Thing::ThingErrorNoError);
        } else {
            connect(parentThing, &Thing::setupStatusChanged, info, [info, parentThing] {
                if (parentThing->setupStatus() == Thing::ThingSetupStatusComplete) {
                    info->finish(Thing::ThingErrorNoError);
                }
            });
        }
    } else {
        info->finish(Thing::ThingErrorThingClassNotFound);
    }
}

void IntegrationPluginDenon::thingRemoved(Thing *thing)
{
    qCDebug(dcDenon()) << "Delete " << thing->name();

    if (thing->thingClassId() == AVRX1000ThingClassId) {
        if (m_avrConnections.contains(thing->id())) {
            AvrConnection *avrConnection = m_avrConnections.take(thing->id());
            avrConnection->disconnectDevice();
            avrConnection->deleteLater();
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

    qCDebug(dcDenon()) << "Execute action" << thing->id() << action.params();
    if (thing->thingClassId() == AVRX1000ThingClassId) {
        AvrConnection *avrConnection = m_avrConnections.value(thing->id());

        if (action.actionTypeId() == AVRX1000PlayActionTypeId) {
            QUuid commandId = avrConnection->play();
            connect(info, &ThingActionInfo::aborted, [this, commandId] {m_avrPendingActions.remove(commandId);});
            m_avrPendingActions.insert(commandId, info);
        } else if (action.actionTypeId() == AVRX1000PauseActionTypeId) {
            QUuid commandId = avrConnection->pause();
            connect(info, &ThingActionInfo::aborted, [this, commandId] {m_avrPendingActions.remove(commandId);});
            m_avrPendingActions.insert(commandId, info);
        } else if (action.actionTypeId() == AVRX1000StopActionTypeId) {
            QUuid commandId = avrConnection->stop();
            connect(info, &ThingActionInfo::aborted, [this, commandId] {m_avrPendingActions.remove(commandId);});
            m_avrPendingActions.insert(commandId, info);
        } else if (action.actionTypeId() == AVRX1000SkipNextActionTypeId) {
            QUuid commandId = avrConnection->skipNext();
            connect(info, &ThingActionInfo::aborted, [this, commandId] {m_avrPendingActions.remove(commandId);});
            m_avrPendingActions.insert(commandId, info);
        } else if (action.actionTypeId() == AVRX1000SkipBackActionTypeId) {
            QUuid commandId = avrConnection->skipBack();
            connect(info, &ThingActionInfo::aborted, [this, commandId] {m_avrPendingActions.remove(commandId);});
            m_avrPendingActions.insert(commandId, info);
        } else if (action.actionTypeId() == AVRX1000PowerActionTypeId) {
            bool power = action.param(AVRX1000PowerActionPowerParamTypeId).value().toBool();
            QUuid commandId = avrConnection->setPower(power);
            connect(info, &ThingActionInfo::aborted, [this, commandId] {m_avrPendingActions.remove(commandId);});
            m_avrPendingActions.insert(commandId, info);
        } else if (action.actionTypeId() == AVRX1000VolumeActionTypeId) {
            int vol = action.param(AVRX1000VolumeActionVolumeParamTypeId).value().toInt();
            QUuid commandId = avrConnection->setVolume(vol);
            connect(info, &ThingActionInfo::aborted, [this, commandId] {m_avrPendingActions.remove(commandId);});
            m_avrPendingActions.insert(commandId, info);
        } else if (action.actionTypeId() == AVRX1000InputSourceActionTypeId) {
            QByteArray channel = action.param(AVRX1000InputSourceActionInputSourceParamTypeId).value().toByteArray();
            QUuid commandId =  avrConnection->setChannel(channel);
            connect(info, &ThingActionInfo::aborted, [this, commandId] {m_avrPendingActions.remove(commandId);});
            m_avrPendingActions.insert(commandId, info);
        } else if (action.actionTypeId() == AVRX1000IncreaseVolumeActionTypeId) {
            uint step =  action.paramValue(AVRX1000IncreaseVolumeActionStepParamTypeId).toUInt();
            uint currentVolume = thing->stateValue(AVRX1000VolumeStateTypeId).toUInt();
            QUuid commandId = avrConnection->setVolume(qMin<uint>(100, currentVolume + step));
            connect(info, &ThingActionInfo::aborted, [this, commandId] {m_avrPendingActions.remove(commandId);});
            m_avrPendingActions.insert(commandId, info);
        } else if (action.actionTypeId() == AVRX1000DecreaseVolumeActionTypeId) {
            uint step =  action.paramValue(AVRX1000DecreaseVolumeActionStepParamTypeId).toUInt();
            uint currentVolume = thing->stateValue(AVRX1000VolumeStateTypeId).toUInt();
            QUuid commandId = avrConnection->setVolume(qMax<uint>(0, currentVolume - step));
            connect(info, &ThingActionInfo::aborted, [this, commandId] {m_avrPendingActions.remove(commandId);});
            m_avrPendingActions.insert(commandId, info);
        } else if (action.actionTypeId() == AVRX1000SurroundModeActionTypeId) {
            QByteArray surroundMode = action.param(AVRX1000SurroundModeActionSurroundModeParamTypeId).value().toByteArray();
            QUuid commandId = avrConnection->setSurroundMode(surroundMode);
            connect(info, &ThingActionInfo::aborted, [this, commandId] {m_avrPendingActions.remove(commandId);});
            m_avrPendingActions.insert(commandId, info);
        } else if (action.actionTypeId() == AVRX1000MuteActionTypeId) {
            bool mute = action.param(AVRX1000MuteActionMuteParamTypeId).value().toBool();
            QUuid commandId = avrConnection->setMute(mute);
            connect(info, &ThingActionInfo::aborted, [this, commandId] {m_avrPendingActions.remove(commandId);});
            m_avrPendingActions.insert(commandId, info);
        } else if (action.actionTypeId() == AVRX1000RepeatActionTypeId) {
            QString repeatMode = action.param(AVRX1000RepeatActionRepeatParamTypeId).value().toString();
            QUuid commandId;
            if (repeatMode == "One") {
                commandId = avrConnection->setRepeat(AvrConnection::RepeatModeRepeatOne);
            } else if (repeatMode == "All") {
                commandId = avrConnection->setRepeat(AvrConnection::RepeatModeRepeatAll);
            } else {
                commandId = avrConnection->setRepeat(AvrConnection::RepeatModeRepeatNone);
            }
            connect(info, &ThingActionInfo::aborted, [this, commandId] {m_avrPendingActions.remove(commandId);});
            m_avrPendingActions.insert(commandId, info);
        } else if (action.actionTypeId() == AVRX1000ShuffleActionTypeId) {
            bool shuffle = action.param(AVRX1000ShuffleActionShuffleParamTypeId).value().toBool();
            QUuid commandId = avrConnection->setRandom(shuffle);
            connect(info, &ThingActionInfo::aborted, [this, commandId] {m_avrPendingActions.remove(commandId);});
            m_avrPendingActions.insert(commandId, info);
        } else if (action.actionTypeId() == AVRX1000PlaybackStatusActionTypeId) {
            QString playbackStatus = action.param(AVRX1000PlaybackStatusActionPlaybackStatusParamTypeId).value().toString();
            QUuid commandId;
            if (playbackStatus == "Playing") {
                commandId = avrConnection->play();
            } else if (playbackStatus == "Stopped") {
                commandId = avrConnection->stop();
            } else if (playbackStatus == "Paused") {
                commandId = avrConnection->pause();
            } else {
                qCWarning(dcDenon()) << "Unrecognized playback status" << playbackStatus;
                return info->finish(Thing::ThingErrorHardwareFailure, "Unrecognized command");
            }
            connect(info, &ThingActionInfo::aborted, [this, commandId] {m_avrPendingActions.remove(commandId);});
            m_avrPendingActions.insert(commandId, info);
        } else if (action.actionTypeId() == AVRX1000ToneControlActionTypeId) {
            bool enable = action.param(AVRX1000ToneControlActionToneControlParamTypeId).value().toBool();
            QUuid commandId = avrConnection->enableToneControl(enable);
            connect(info, &ThingActionInfo::aborted, [this, commandId] {m_avrPendingActions.remove(commandId);});
            m_avrPendingActions.insert(commandId, info);
        } else if (action.actionTypeId() == AVRX1000BassActionTypeId) {
            int bass = action.param(AVRX1000BassActionBassParamTypeId).value().toInt();
            QUuid commandId = avrConnection->setBassLevel(bass);
            connect(info, &ThingActionInfo::aborted, [this, commandId] {m_avrPendingActions.remove(commandId);});
            m_avrPendingActions.insert(commandId, info);
        } else if (action.actionTypeId() == AVRX1000TrebleActionTypeId) {
            int treble = action.param(AVRX1000TrebleActionTrebleParamTypeId).value().toInt();
            QUuid commandId = avrConnection->setTrebleLevel(treble);
            connect(info, &ThingActionInfo::aborted, [this, commandId] {m_avrPendingActions.remove(commandId);});
            m_avrPendingActions.insert(commandId, info);
        } else {
            qCWarning(dcDenon()) << "ActionType not found" << thing->thingClass().name() << action.actionTypeId() ;
            return info->finish(Thing::ThingErrorActionTypeNotFound);
        }

    } else if (thing->thingClassId() == heosThingClassId) {

        Heos *heos = m_heosConnections.value(thing->id());
        if (action.actionTypeId() == heosRebootActionTypeId) {
            heos->rebootSpeaker();
            return info->finish(Thing::ThingErrorNoError);
        } else {
            qCWarning(dcDenon()) << "ActionType not found" << thing->thingClass().name() << action.actionTypeId() ;
            return info->finish(Thing::ThingErrorActionTypeNotFound);
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
        } else if (action.actionTypeId() == heosPlayerIncreaseVolumeActionTypeId) {
            heos->volumeUp(playerId, action.param(heosPlayerIncreaseVolumeActionStepParamTypeId).value().toInt());
            info->finish(Thing::ThingErrorNoError);
            return;
        } else if (action.actionTypeId() == heosPlayerDecreaseVolumeActionTypeId) {
            heos->volumeUp(playerId, action.param(heosPlayerDecreaseVolumeActionStepParamTypeId).value().toInt());
            info->finish(Thing::ThingErrorNoError);
            return;
        } else {
            qCWarning(dcDenon()) << "ActionType not found" << thing->thingClass().name() << action.actionTypeId() ;
            return info->finish(Thing::ThingErrorActionTypeNotFound);
        }
    } else {
        qCWarning(dcDenon()) << "ThingClass not found" << thing->thingClass().name() << thing->thingClassId() ;
        return info->finish(Thing::ThingErrorThingClassNotFound);
    }
}

void IntegrationPluginDenon::postSetupThing(Thing *thing)
{
    qCDebug(dcDenon()) << "Post setup thing" << thing->name();

    if (thing->thingClassId() == AVRX1000ThingClassId) {
        AvrConnection *avrConnection = m_avrConnections.value(thing->id());
        thing->setStateValue(AVRX1000ConnectedStateTypeId, avrConnection->connected());
        avrConnection->getPower();
        avrConnection->getMute();
        avrConnection->getVolume();
        avrConnection->getChannel();
        avrConnection->getSurroundMode();
        avrConnection->getPlayBackInfo();
        avrConnection->getBassLevel();
        avrConnection->getTrebleLevel();
        avrConnection->getToneControl();

    } else if (thing->thingClassId() == heosThingClassId) {
        Heos *heos = m_heosConnections.value(thing->id());
        thing->setStateValue(heosConnectedStateTypeId, heos->connected());
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
        qCDebug(dcDenon()) << "Creating plugin timer";
        m_pluginTimer = hardwareManager()->pluginTimerManager()->registerTimer(60);
        connect(m_pluginTimer, &PluginTimer::timeout, this, &IntegrationPluginDenon::onPluginTimer);
    }
}


void IntegrationPluginDenon::onPluginTimer()
{
    foreach(AvrConnection *avrConnection, m_avrConnections.values()) {
        if (!avrConnection->connected()) {
            avrConnection->connectDevice();
        }
        Thing *thing = myThings().findById(m_avrConnections.key(avrConnection));
        if (thing->thingClassId() == AVRX1000ThingClassId) {
            avrConnection->getPower();
            avrConnection->getMute();
            avrConnection->getVolume();
            avrConnection->getChannel();
            avrConnection->getSurroundMode();
            avrConnection->getPlayBackInfo();
            avrConnection->getBassLevel();
            avrConnection->getTrebleLevel();
            avrConnection->getToneControl();
        }
    }

    foreach(Thing *thing, myThings().filterByThingClassId(heosThingClassId)) {

        Heos *heos = m_heosConnections.value(thing->id());
        heos->getPlayers();
        heos->registerForChangeEvents(true);
    }
}

void IntegrationPluginDenon::onAvrConnectionChanged(bool status)
{
    AvrConnection *denonConnection = static_cast<AvrConnection *>(sender());

    // if the thing and from the first setup
    if (m_asyncAvrSetups.contains(denonConnection)) {
        // and ist connected
        if (status) {
            ThingSetupInfo *info = m_asyncAvrSetups.take(denonConnection);
            info->thing()->setStateValue(AVRX1000ConnectedStateTypeId, true);
            info->finish(Thing::ThingErrorNoError);
        }
        return;
    }

    Thing *thing = myThings().findById(m_avrConnections.key(denonConnection));
    if (!thing) {
        qCWarning(dcDenon()) << "Could not find a thing associated to this AVR connection";
        return;
    }

    if (thing->thingClassId() == AVRX1000ThingClassId) {
        thing->setStateValue(AVRX1000ConnectedStateTypeId, denonConnection->connected());

        if (!status) {
            QString id = thing->paramValue(AVRX1000ThingIdParamTypeId).toString();
            QHostAddress address = findAvrById(id);
            if (!address.isNull()){
                denonConnection->setHostAddress(address);
            }
        }
    }
}

void IntegrationPluginDenon::onAvrVolumeChanged(int volume)
{
    AvrConnection *denonConnection = static_cast<AvrConnection *>(sender());
    Thing *thing = myThings().findById(m_avrConnections.key(denonConnection));
    if (!thing) {
        qCWarning(dcDenon()) << "Could not find a thing associated to this AVR connection";
        return;
    }

    if (thing->thingClassId() == AVRX1000ThingClassId) {
        thing->setStateValue(AVRX1000VolumeStateTypeId, volume);
    }
}

void IntegrationPluginDenon::onAvrChannelChanged(const QString &channel)
{
    AvrConnection *denonConnection = static_cast<AvrConnection *>(sender());
    Thing *thing = myThings().findById(m_avrConnections.key(denonConnection));
    if (!thing)
        return;

    if (thing->thingClassId() == AVRX1000ThingClassId) {
        thing->setStateValue(AVRX1000InputSourceStateTypeId, channel);
    }
}

void IntegrationPluginDenon::onAvrMuteChanged(bool mute)
{
    AvrConnection *denonConnection = static_cast<AvrConnection *>(sender());
    Thing *thing = myThings().findById(m_avrConnections.key(denonConnection));
    if (!thing) {
        qCWarning(dcDenon()) << "Could not find a thing associated to this AVR connection";
        return;
    }

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

void IntegrationPluginDenon::onAvrSurroundModeChanged(const QString &surroundMode)
{
    AvrConnection *denonConnection = static_cast<AvrConnection *>(sender());
    Thing *thing = myThings().findById(m_avrConnections.key(denonConnection));
    if (!thing){
        qCWarning(dcDenon()) << "Could not find a thing associated to this AVR connection";
        return;
    }
    if (thing->thingClassId() == AVRX1000ThingClassId) {
        thing->setStateValue(AVRX1000SurroundModeStateTypeId, surroundMode);
    }
}

void IntegrationPluginDenon::onAvrSongChanged(const QString &song)
{
    AvrConnection *denonConnection = static_cast<AvrConnection *>(sender());
    Thing *thing = myThings().findById(m_avrConnections.key(denonConnection));
    if (!thing){
        qCWarning(dcDenon()) << "Could not find a thing associated to this AVR connection";
        return;
    }

    if (thing->thingClassId() == AVRX1000ThingClassId) {
        thing->setStateValue(AVRX1000TitleStateTypeId, song);
    }
}

void IntegrationPluginDenon::onAvrArtistChanged(const QString &artist)
{
    AvrConnection *denonConnection = static_cast<AvrConnection *>(sender());
    Thing *thing = myThings().findById(m_avrConnections.key(denonConnection));
    if (!thing){
        qCWarning(dcDenon()) << "Could not find a thing associated to this AVR connection";
        return;
    }

    if (thing->thingClassId() == AVRX1000ThingClassId) {
        thing->setStateValue(AVRX1000ArtistStateTypeId, artist);
    }
}

void IntegrationPluginDenon::onAvrAlbumChanged(const QString &album)
{
    AvrConnection *denonConnection = static_cast<AvrConnection *>(sender());
    Thing *thing = myThings().findById(m_avrConnections.key(denonConnection));
    if (!thing){
        qCWarning(dcDenon()) << "Could not find a thing associated to this AVR connection";
        return;
    }
    if (thing->thingClassId() == AVRX1000ThingClassId) {
        thing->setStateValue(AVRX1000CollectionStateTypeId, album);
    }
}

void IntegrationPluginDenon::onAvrBassLevelChanged(int level)
{
    AvrConnection *denonConnection = static_cast<AvrConnection *>(sender());
    Thing *thing = myThings().findById(m_avrConnections.key(denonConnection));
    if (!thing){
        qCWarning(dcDenon()) << "Could not find a thing associated to this AVR connection";
        return;
    }
    if (thing->thingClassId() == AVRX1000ThingClassId) {
        thing->setStateValue(AVRX1000BassStateTypeId, level);
    }
}

void IntegrationPluginDenon::onAvrTrebleLevelChanged(int level)
{
    AvrConnection *denonConnection = static_cast<AvrConnection *>(sender());
    Thing *thing = myThings().findById(m_avrConnections.key(denonConnection));
    if (!thing){
        qCWarning(dcDenon()) << "Could not find a thing associated to this AVR connection";
        return;
    }
    if (thing->thingClassId() == AVRX1000ThingClassId) {
        thing->setStateValue(AVRX1000TrebleStateTypeId, level);
    }
}

void IntegrationPluginDenon::onAvrToneControlEnabledChanged(bool enabled)
{
    AvrConnection *denonConnection = static_cast<AvrConnection *>(sender());
    Thing *thing = myThings().findById(m_avrConnections.key(denonConnection));
    if (!thing){
        qCWarning(dcDenon()) << "Could not find a thing associated to this AVR connection";
        return;
    }
    if (thing->thingClassId() == AVRX1000ThingClassId) {
        thing->setStateValue(AVRX1000ToneControlStateTypeId, enabled);
    }
}

void IntegrationPluginDenon::onAvrPlayBackModeChanged(AvrConnection::PlayBackMode mode)
{
    AvrConnection *denonConnection = static_cast<AvrConnection *>(sender());
    Thing *thing = myThings().findById(m_avrConnections.key(denonConnection));
    if (!thing){
        qCWarning(dcDenon()) << "Could not find a thing associated to this AVR connection";
        return;
    }

    if (thing->thingClassId() == AVRX1000ThingClassId) {
        switch (mode) {
        case AvrConnection::PlayBackModePlaying:
            thing->setStateValue(AVRX1000PlaybackStatusStateTypeId, "Playing");
            break;
        case AvrConnection::PlayBackModePaused:
            thing->setStateValue(AVRX1000PlaybackStatusStateTypeId, "Paused");
            break;
        case AvrConnection::PlayBackModeStopped:
            thing->setStateValue(AVRX1000PlaybackStatusStateTypeId, "Stopped");
            break;
        }
    }
}


void IntegrationPluginDenon::onAvrSocketError()
{
    AvrConnection *avrConnection = static_cast<AvrConnection *>(sender());

    // Check if setup running for this thing
    if (m_asyncAvrSetups.contains(avrConnection)) {
        ThingSetupInfo *info = m_asyncAvrSetups.take(avrConnection);
        m_avrConnections.remove(info->thing()->id());
        qCWarning(dcDenon()) << "Could not add thing. The setup failed.";
        info->finish(Thing::ThingErrorHardwareFailure);
        // Delete the connection, the thing will not be added and
        // the connection will be created in the next setup
        avrConnection->deleteLater();
    }
}

void IntegrationPluginDenon::onAvrCommandExecuted(const QUuid &commandId, bool success)
{
    if (m_avrPendingActions.contains(commandId)) {
        ThingActionInfo *info = m_avrPendingActions.take(commandId);
        if (success){
            if(info->action().actionTypeId() == AVRX1000PlayActionTypeId) {
                info->thing()->setStateValue(AVRX1000PlaybackStatusStateTypeId, "Playing");
            } else if(info->action().actionTypeId() == AVRX1000PauseActionTypeId) {
                info->thing()->setStateValue(AVRX1000PlaybackStatusStateTypeId, "Paused");
            } else if(info->action().actionTypeId() == AVRX1000StopActionTypeId) {
                info->thing()->setStateValue(AVRX1000PlaybackStatusStateTypeId, "Stopped");
            } else if(info->action().actionTypeId() == AVRX1000PlaybackStatusActionTypeId) {
                info->thing()->setStateValue(AVRX1000PlaybackStatusStateTypeId, info->action().param(AVRX1000PlaybackStatusActionPlaybackStatusParamTypeId).value());
            }
            info->finish(Thing::ThingErrorNoError);

        } else {
            info->finish(Thing::ThingErrorHardwareNotAvailable);
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

    qCDebug(dcDenon()) << "Heos connection changed" << thing->name();
    if (thing->thingClassId() == heosThingClassId) {

        if (pluginStorage()->childGroups().contains(thing->id().toString())) {
            pluginStorage()->beginGroup(thing->id().toString());
            QString username = pluginStorage()->value("username").toString();
            QString password = pluginStorage()->value("password").toString();
            pluginStorage()->endGroup();
            heos->setUserAccount(username, password);
        } else {
            qCWarning(dcDenon()) << "Plugin storage doesn't contain this thingId";
        }

        if (!status) {
            thing->setStateValue(heosLoggedInStateTypeId, false);
            thing->setStateValue(heosUserDisplayNameStateTypeId, "");

            qCDebug(dcDenon()) << "Starting Heos discovery";
            UpnpDiscoveryReply *reply = hardwareManager()->upnpDiscovery()->discoverDevices();
            connect(reply, &UpnpDiscoveryReply::finished, reply, &UpnpDiscoveryReply::deleteLater);
            connect(reply, &UpnpDiscoveryReply::finished, this, &IntegrationPluginDenon::onHeosDiscoveryFinished);
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
    if (heosPlayers.isEmpty()) {
        qCWarning(dcDenon()) << "Received empty player list, this is likely because of a bug in the Heos API, ignoring";
        return;
    }
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
        qCDebug(dcDenon()) << "Found new heos player" << player->name();
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
            qCDebug(dcDenon()) << "Heos player vanished, removing" << thing->name();
            autoThingDisappeared(existingThing->id());
            m_playerBuffer.remove(playerId);
        }
    }
}

void IntegrationPluginDenon::onHeosPlayerInfoRecieved(HeosPlayer *heosPlayer)
{
    qCDebug(dcDenon()) << "Heos player info received" << heosPlayer->name() << heosPlayer->playerId() << heosPlayer->groupId();
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
        thing->setStateValue(heosPlayerShuffleStateTypeId, shuffle);
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
            qCDebug(dcDenon()) << "Music source received:" << source.name << source.type << source.sourceId << source.image_url;
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
            qCDebug(dcDenon()) << "Adding Item" << media.name << media.mediaId << media.containerId << media.mediaType;
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
            qCDebug(dcDenon()) << "Adding Item" << source.name << source.sourceId;
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
        qCWarning(dcDenon()) << "Pending browser result doesnt recognize" << identifier << m_pendingBrowseResult.keys();
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
        qCWarning(dcDenon()) << "Browse error" << errorMessage << errorId;
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

void IntegrationPluginDenon::onHeosDiscoveryFinished()
{
    UpnpDiscoveryReply *reply = static_cast<UpnpDiscoveryReply *>(sender());

    if (reply->error() != UpnpDiscoveryReply::UpnpDiscoveryReplyErrorNoError) {
        qCWarning(dcDenon()) << "Upnp discovery error" << reply->error();
        return;
    }

    Q_FOREACH(const UpnpDeviceDescriptor &upnpThing, reply->deviceDescriptors()) {
        Q_FOREACH(Thing *thing, myThings().filterByThingClassId(heosThingClassId)) {
            QString serialNumber = thing->paramValue(heosThingSerialNumberParamTypeId).toString();
            if (serialNumber == upnpThing.serialNumber()) {
                Heos *heos = m_heosConnections.value(thing->id());
                if (!heos) {
                    qCWarning(dcDenon()) << "On heos discovery, heos connection not found for" << thing->name();
                    return;
                }
                heos->setAddress(upnpThing.hostAddress());
            }
        }
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
        qCDebug(dcDenon()) << "Browse source";
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
            qCDebug(dcDenon()) << "Browse source" << result->itemId();
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
                qCDebug(dcDenon()) << "Adding group item" << player->name();
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
        qCDebug(dcDenon()) << "Browse source" << result->itemId();
        QString id = result->itemId().remove("source=");
        heos->browseSource(id);
        m_pendingBrowseResult.insert(id, result);
        connect(result, &QObject::destroyed, this, [this, id](){ m_pendingBrowseResult.remove(id);});

    } else if (result->itemId().startsWith("container=")){
        qCDebug(dcDenon()) << "Browse container" << result->itemId();
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
    qCDebug(dcDenon()) << "Browse item called" << result->itemId();
    
    result->item().setDisplayName("Test name");
    if (m_mediaObjects.contains(result->itemId())) {
        qCDebug(dcDenon()) << "Media Object found" << m_mediaObjects.value(result->itemId()).name;
        BrowserItem item(result->itemId(), m_mediaObjects.value(result->itemId()).name, false, true);
        result->finish(item);
    } else {
        qCWarning(dcDenon()) << "Media Object not found for itemId" << result->itemId();
        result->finish(Thing::ThingErrorItemNotFound, "Item not found");
    }
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
    qCDebug(dcDenon()) << "Execute browse item called. Player Id:" << playerId << "Item ID" << action.itemId();

    if (m_mediaObjects.contains(action.itemId())) {
        MediaObject media = m_mediaObjects.value(action.itemId());
        if (media.mediaType == MEDIA_TYPE_CONTAINER) {
            heos->addContainerToQueue(playerId, media.sourceId, media.containerId, ADD_CRITERIA_PLAY_NOW);
        } else if (media.mediaType == MEDIA_TYPE_STATION) {
            heos->playStation(playerId, media.sourceId, media.containerId, media.mediaId, media.name);
        }
    } else {
        qCWarning(dcDenon()) << "Media item not found" << action.itemId();
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
            qCDebug(dcDenon()) << "Execute browse item action called, Group:" << query.queryItemValue("group").toInt() << group.name;
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

QHostAddress IntegrationPluginDenon::findAvrById(const QString &id)
{
    foreach (const ZeroConfServiceEntry &service, m_serviceBrowser->serviceEntries()) {
        if (service.txt().contains("am=AVRX1000")) {
            if (service.name().split("@").first() == id) {
                return service.hostAddress();
            }
        }
    }
    return QHostAddress();
}
