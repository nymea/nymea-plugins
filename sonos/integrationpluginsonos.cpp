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

#include "integrationpluginsonos.h"
#include "plugininfo.h"

#include <integrations/thing.h>
#include <types/mediabrowseritem.h>
#include <network/networkaccessmanager.h>
#include <plugintimer.h>

#include <QNetworkRequest>
#include <QNetworkReply>
#include <QUrlQuery>
#include <QJsonDocument>

IntegrationPluginSonos::IntegrationPluginSonos()
{
}


IntegrationPluginSonos::~IntegrationPluginSonos()
{
    if (m_pluginTimer5sec)
        hardwareManager()->pluginTimerManager()->unregisterTimer(m_pluginTimer5sec);
    if (m_pluginTimer60sec)
        hardwareManager()->pluginTimerManager()->unregisterTimer(m_pluginTimer60sec);
}


void IntegrationPluginSonos::setupThing(ThingSetupInfo *info)
{
    Thing *thing = info->thing();

    if (thing->thingClassId() == sonosConnectionThingClassId) {
        Sonos *sonos = nullptr;
        if (m_setupSonosConnections.keys().contains(thing->id())) {
            //Fresh thing setup, has already a fresh access token
            qCDebug(dcSonos()) << "Sonos OAuth setup complete";
            sonos = m_setupSonosConnections.take(thing->id());
            connect(sonos, &Sonos::connectionChanged, this, &IntegrationPluginSonos::onConnectionChanged);
            connect(sonos, &Sonos::householdIdsReceived, this, &IntegrationPluginSonos::onHouseholdIdsReceived);
            connect(sonos, &Sonos::groupsReceived, this, &IntegrationPluginSonos::onGroupsReceived);
            connect(sonos, &Sonos::playBackStatusReceived, this, &IntegrationPluginSonos::onPlayBackStatusReceived);
            connect(sonos, &Sonos::metadataStatusReceived, this, &IntegrationPluginSonos::onMetadataStatusReceived);
            connect(sonos, &Sonos::volumeReceived, this, &IntegrationPluginSonos::onVolumeReceived);
            connect(sonos, &Sonos::actionExecuted, this, &IntegrationPluginSonos::onActionExecuted);
            connect(sonos, &Sonos::authenticationStatusChanged, this, &IntegrationPluginSonos::onAuthenticationStatusChanged);

            connect(sonos, &Sonos::authenticationStatusChanged, info, [info](bool authenticated){
                if (authenticated) {
                    info->finish(Thing::ThingErrorNoError);
                } else {
                    info->finish(Thing::ThingErrorAuthenticationFailure);
                }
            });

            m_sonosConnections.insert(thing, sonos);
            return info->finish(Thing::ThingErrorNoError);
        } else {
            //thing loaded from the thing database, needs a new access token;
            pluginStorage()->beginGroup(thing->id().toString());
            QByteArray refreshToken = pluginStorage()->value("refresh_token").toByteArray();
            pluginStorage()->endGroup();

            if (refreshToken.isEmpty()) {
                info->finish(Thing::ThingErrorAuthenticationFailure);
                return;
            }

            sonos = new Sonos(hardwareManager()->networkManager(), "0a8f6d44-d9d1-4474-bcfa-cfb41f8b66e8", "3095ce48-0c5d-47ce-a1f4-6005c7b8fdb5", this);
            connect(sonos, &Sonos::connectionChanged, this, &IntegrationPluginSonos::onConnectionChanged);
            connect(sonos, &Sonos::householdIdsReceived, this, &IntegrationPluginSonos::onHouseholdIdsReceived);
            connect(sonos, &Sonos::groupsReceived, this, &IntegrationPluginSonos::onGroupsReceived);
            connect(sonos, &Sonos::playBackStatusReceived, this, &IntegrationPluginSonos::onPlayBackStatusReceived);
            connect(sonos, &Sonos::metadataStatusReceived, this, &IntegrationPluginSonos::onMetadataStatusReceived);
            connect(sonos, &Sonos::volumeReceived, this, &IntegrationPluginSonos::onVolumeReceived);
            connect(sonos, &Sonos::actionExecuted, this, &IntegrationPluginSonos::onActionExecuted);
            connect(sonos, &Sonos::authenticationStatusChanged, this, &IntegrationPluginSonos::onAuthenticationStatusChanged);
            connect(sonos, &Sonos::favoritesReceived, this, &IntegrationPluginSonos::onFavoritesReceived);
            sonos->getAccessTokenFromRefreshToken(refreshToken);
            m_sonosConnections.insert(thing, sonos);
            return info->finish(Thing::ThingErrorNoError);
        }
    }

    if (thing->thingClassId() == sonosGroupThingClassId) {
        return info->finish(Thing::ThingErrorNoError);
    }

    qCWarning(dcSonos()) << "Unhandled thing class id in setupDevice" << thing->thingClassId();
}

void IntegrationPluginSonos::startPairing(ThingPairingInfo *info)
{
    if (info->thingClassId() == sonosConnectionThingClassId) {

        Sonos *sonos = new Sonos(hardwareManager()->networkManager(), "0a8f6d44-d9d1-4474-bcfa-cfb41f8b66e8", "3095ce48-0c5d-47ce-a1f4-6005c7b8fdb5", this);
        QUrl url = sonos->getLoginUrl(QUrl("https://127.0.0.1:8888"));
        qCDebug(dcSonos()) << "Sonos url:" << url;
        info->setOAuthUrl(url);
        info->finish(Thing::ThingErrorNoError);
        m_setupSonosConnections.insert(info->thingId(), sonos);
        return;
    }

    qCWarning(dcSonos()) << "Unhandled pairing metod!";
    info->finish(Thing::ThingErrorCreationMethodNotSupported);
}

void IntegrationPluginSonos::confirmPairing(ThingPairingInfo *info, const QString &username, const QString &secret)
{
    Q_UNUSED(username)

    if (info->thingClassId() == sonosConnectionThingClassId) {
        qCDebug(dcSonos()) << "Redirect url is" << secret;
        QUrl url(secret);
        QUrlQuery query(url);
        QByteArray authorizationCode = query.queryItemValue("code").toLocal8Bit();
        //QByteArray state = query.queryItemValue("state").toLocal8Bit();
        //TODO evaluate state if it equals the given state

        Sonos *sonos = m_setupSonosConnections.value(info->thingId());

        if (!sonos) {
            qCWarning(dcSonos()) << "No sonos connection found for thing:" << info->thingName();
            m_setupSonosConnections.remove(info->thingId());
            info->finish(Thing::ThingErrorHardwareFailure);
            return;
        }
        sonos->getAccessTokenFromAuthorizationCode(authorizationCode);
        connect(sonos, &Sonos::authenticationStatusChanged, info, [this, info, sonos](bool authenticated){
            if(!authenticated) {
                qCWarning(dcSonos()) << "Authentication process failed"  << info->thingName();
                m_setupSonosConnections.remove(info->thingId());
                sonos->deleteLater();
                info->finish(Thing::ThingErrorSetupFailed, QT_TR_NOOP("Authentication failed. Please try again."));
                return;
            }
            QByteArray accessToken = sonos->accessToken();
            QByteArray refreshToken = sonos->refreshToken();
            qCDebug(dcSonos()) << "Token:" << accessToken << refreshToken;

            pluginStorage()->beginGroup(info->thingId().toString());
            pluginStorage()->setValue("refresh_token", refreshToken);
            pluginStorage()->endGroup();

            info->finish(Thing::ThingErrorNoError);
        });
        return;
    }
    qCWarning(dcSonos()) << "Invalid thingClassId -> no pairing possible with this device";
    info->finish(Thing::ThingErrorThingClassNotFound);
}

void IntegrationPluginSonos::postSetupThing(Thing *thing)
{
    if (!m_pluginTimer5sec) {
        m_pluginTimer5sec = hardwareManager()->pluginTimerManager()->registerTimer(5);
        connect(m_pluginTimer5sec, &PluginTimer::timeout, this, [this]() {

            foreach (Thing *connectionDevice, myThings().filterByThingClassId(sonosConnectionThingClassId)) {
                Sonos *sonos = m_sonosConnections.value(connectionDevice);
                if (!sonos) {
                    qCWarning(dcSonos()) << "No sonos connection found to" << connectionDevice->name();
                    continue;
                }
                foreach (Thing *groupDevice, myThings().filterByParentId(connectionDevice->id())) {
                    if (groupDevice->thingClassId() == sonosGroupThingClassId) {
                        //get playback status of each group
                        QString groupId = groupDevice->paramValue(sonosGroupThingGroupIdParamTypeId).toString();
                        sonos->getGroupPlaybackStatus(groupId);
                        sonos->getGroupMetadataStatus(groupId);
                        sonos->getGroupVolume(groupId);
                    }
                }
            }
        });
    }

    if (!m_pluginTimer60sec) {
        m_pluginTimer60sec = hardwareManager()->pluginTimerManager()->registerTimer(60);
        connect(m_pluginTimer60sec, &PluginTimer::timeout, this, [this]() {
            foreach (Thing *thing, myThings().filterByThingClassId(sonosConnectionThingClassId)) {
                Sonos *sonos = m_sonosConnections.value(thing);
                if (!sonos) {
                    qCWarning(dcSonos()) << "No sonos connection found to" << thing->name();
                    continue;
                }
                //get groups for each household in order to add or remove groups
                sonos->getHouseholds();
            }
        });
    }

    if (thing->thingClassId() == sonosConnectionThingClassId) {
        Sonos *sonos = m_sonosConnections.value(thing);
        sonos->getHouseholds();
    }

    if (thing->thingClassId() == sonosGroupThingClassId) {
        Thing *parentDevice = myThings().findById(thing->parentId());
        Sonos *sonos = m_sonosConnections.value(parentDevice);
        if (!sonos) {
            return;
        }
        QString groupId = thing->paramValue(sonosGroupThingGroupIdParamTypeId).toString();
        sonos->getGroupPlaybackStatus(groupId);
        sonos->getGroupMetadataStatus(groupId);
        sonos->getGroupVolume(groupId);
    }
}


void IntegrationPluginSonos::startMonitoringAutoThings()
{
    foreach (Thing *thing, myThings()) {
        if (thing->thingClassId() == sonosGroupThingClassId) {
            return;
        }
    }
}

void IntegrationPluginSonos::thingRemoved(Thing *thing)
{
    qCDebug(dcSonos) << "Delete " << thing->name();
    if (myThings().empty()) {
        hardwareManager()->pluginTimerManager()->unregisterTimer(m_pluginTimer5sec);
        hardwareManager()->pluginTimerManager()->unregisterTimer(m_pluginTimer60sec);
        m_pluginTimer5sec = nullptr;
        m_pluginTimer60sec = nullptr;
    }
}


void IntegrationPluginSonos::executeAction(ThingActionInfo *info)
{
    Thing *thing = info->thing();
    Action action = info->action();

    if (thing->thingClassId() == sonosGroupThingClassId) {
        Sonos *sonos = m_sonosConnections.value(myThings().findById(thing->parentId()));
        QString groupId = thing->paramValue(sonosGroupThingGroupIdParamTypeId).toString();

        if (!sonos) {
            qCWarning(dcSonos()) << "Action cannot be executed: Sonos connection not available";
            return info->finish(Thing::ThingErrorHardwareNotAvailable, QT_TR_NOOP("Sonos thing is not available."));
        }

        if (action.actionTypeId()  == sonosGroupPlayActionTypeId) {
            m_pendingActions.insert(sonos->groupPlay(groupId), QPointer<ThingActionInfo>(info));
            return;
        }

        if (action.actionTypeId()  == sonosGroupShuffleActionTypeId) {
            bool shuffle = action.param(sonosGroupShuffleActionShuffleParamTypeId).value().toBool();
            m_pendingActions.insert(sonos->groupSetShuffle(groupId, shuffle), QPointer<ThingActionInfo>(info));
            return;
        }

        if (action.actionTypeId()  == sonosGroupRepeatActionTypeId) {
            if (action.param(sonosGroupRepeatActionRepeatParamTypeId).value().toString() == "None") {
                m_pendingActions.insert(sonos->groupSetRepeat(groupId, Sonos::RepeatModeNone), QPointer<ThingActionInfo>(info));
            } else if (action.param(sonosGroupRepeatActionRepeatParamTypeId).value().toString() == "One") {
                m_pendingActions.insert(sonos->groupSetRepeat(groupId, Sonos::RepeatModeOne), QPointer<ThingActionInfo>(info));
            } else if (action.param(sonosGroupRepeatActionRepeatParamTypeId).value().toString() == "All") {
                m_pendingActions.insert(sonos->groupSetRepeat(groupId, Sonos::RepeatModeAll), QPointer<ThingActionInfo>(info));
            } else {
                return info->finish(Thing::ThingErrorHardwareFailure);
            }
            return;
        }

        if (action.actionTypeId() == sonosGroupPauseActionTypeId) {
            m_pendingActions.insert(sonos->groupPause(groupId), QPointer<ThingActionInfo>(info));
            return;
        }

        if (action.actionTypeId() == sonosGroupStopActionTypeId) {
            m_pendingActions.insert(sonos->groupPause(groupId), QPointer<ThingActionInfo>(info));
            return;
        }

        if (action.actionTypeId() == sonosGroupMuteActionTypeId) {
            bool mute = action.param(sonosGroupMuteActionMuteParamTypeId).value().toBool();
            m_pendingActions.insert(sonos->setGroupMute(groupId, mute), QPointer<ThingActionInfo>(info));
            return;
        }


        if (action.actionTypeId() == sonosGroupVolumeActionTypeId) {
            int volume = action.param(sonosGroupVolumeActionVolumeParamTypeId).value().toInt();
            m_pendingActions.insert(sonos->setGroupVolume(groupId, volume), QPointer<ThingActionInfo>(info));
            return;
        }

        if (action.actionTypeId() == sonosGroupSkipNextActionTypeId) {
            m_pendingActions.insert(sonos->groupSkipToNextTrack(groupId), QPointer<ThingActionInfo>(info));
            return;
        }

        if (action.actionTypeId() == sonosGroupSkipBackActionTypeId) {
            m_pendingActions.insert(sonos->groupSkipToPreviousTrack(groupId), QPointer<ThingActionInfo>(info));
            return;
        }

        if (action.actionTypeId() == sonosGroupPlaybackStatusActionTypeId) {
            QString playbackStatus = action.param(sonosGroupPlaybackStatusActionPlaybackStatusParamTypeId).value().toString();
            if (playbackStatus == "Playing") {
                m_pendingActions.insert(sonos->groupPlay(groupId), QPointer<ThingActionInfo>(info));
            } else if(playbackStatus == "Stopped") {
                m_pendingActions.insert(sonos->groupPause(groupId), QPointer<ThingActionInfo>(info));
            } else if(playbackStatus == "Paused") {
                m_pendingActions.insert(sonos->groupPause(groupId), QPointer<ThingActionInfo>(info));
            }
            return;
        }

        if (action.actionTypeId() == sonosGroupIncreaseVolumeActionTypeId) {
            int volume = qMin(100, thing->stateValue(sonosGroupVolumeStateTypeId).toInt() + 5);
            m_pendingActions.insert(sonos->setGroupVolume(groupId, volume), QPointer<ThingActionInfo>(info));
            return;
        }

        if (action.actionTypeId() == sonosGroupDecreaseVolumeActionTypeId) {
            int volume = qMax(0, thing->stateValue(sonosGroupVolumeStateTypeId).toInt() - 5);
            m_pendingActions.insert(sonos->setGroupVolume(groupId, volume), QPointer<ThingActionInfo>(info));
            return;
        }

        return info->finish(Thing::ThingErrorActionTypeNotFound);
    }
    info->finish(Thing::ThingErrorThingClassNotFound);
}

void IntegrationPluginSonos::browseThing(BrowseResult *result)
{
    Thing *parentDevice = myThings().findById(result->thing()->parentId());
    Sonos *sonosConnection = m_sonosConnections.value(parentDevice);
    if (!sonosConnection) {
        result->finish(Thing::ThingErrorHardwareNotAvailable);
        return;
    }

    qCDebug(dcSonos()) << "Browse Device" << result->itemId();
    QString householdId = result->thing()->paramValue(sonosGroupThingHouseholdIdParamTypeId).toString();
    if (result->itemId().isEmpty()){
        BrowserItem item;
        item.setId(m_browseFavoritesPrefix);
        item.setIcon(BrowserItem::BrowserIconFavorites);
        item.setExecutable(false);
        item.setBrowsable(true);
        item.setDisplayName("Favorites");
        result->addItem(item);
        result->finish(Thing::ThingErrorNoError);
    } else if (result->itemId() == m_browseFavoritesPrefix) {
        QUuid requestId = sonosConnection->getFavorites(householdId);
        m_pendingBrowseResult.insert(requestId, result);
        connect(result, &BrowseResult::aborted, this, [requestId, this](){ m_pendingBrowseResult.remove(requestId); });
    } else {
        //TODO add media browsing
        result->finish(Thing::ThingErrorItemNotFound);
    }
}

void IntegrationPluginSonos::browserItem(BrowserItemResult *result)
{
    Thing *parentDevice = myThings().findById(result->thing()->parentId());
    Sonos *sonosConnection = m_sonosConnections.value(parentDevice);
    if (!sonosConnection) {
        result->finish(Thing::ThingErrorHardwareNotAvailable);
        return;
    }

    qCDebug(dcSonos()) << "Browser Item" << result->itemId();
    QString householdId = result->thing()->paramValue(sonosGroupThingHouseholdIdParamTypeId).toString();
    if (result->itemId().startsWith(m_browseFavoritesPrefix)) {
        QUuid requestId = sonosConnection->getFavorites(householdId);
        m_pendingBrowserItemResult.insert(requestId, result);
        connect(result, &BrowserItemResult::aborted, this, [requestId, this](){ m_pendingBrowserItemResult.remove(requestId); });
    } else {
         //TODO add media browsing
         result->finish(Thing::ThingErrorItemNotFound);
    }
}

void IntegrationPluginSonos::executeBrowserItem(BrowserActionInfo *info)
{
    Thing *parentDevice = myThings().findById(info->thing()->parentId());
    Sonos *sonosConnection = m_sonosConnections.value(parentDevice);
    if (!sonosConnection)
        return;

    QString groupId = info->thing()->paramValue(sonosGroupThingGroupIdParamTypeId).toString();
    if (info->browserAction().itemId().startsWith(m_browseFavoritesPrefix)) {
        QString favoriteId = info->browserAction().itemId().remove(m_browseFavoritesPrefix);
        favoriteId.remove('/');
        QUuid requestId = sonosConnection->loadFavorite(groupId, favoriteId);
        m_pendingBrowserExecution.insert(requestId, info);
        connect(info, &BrowserActionInfo::aborted, this, [requestId, this](){ m_pendingBrowserExecution.remove(requestId); });
    } else {
        //TODO add media browsing
        info->finish(Thing::ThingErrorItemNotFound);
    }
}

void IntegrationPluginSonos::onConnectionChanged(bool connected)
{
    Sonos *sonos = static_cast<Sonos *>(sender());
    Thing *thing = m_sonosConnections.key(sonos);
    if (!thing)
        return;

    thing->setStateValue(sonosConnectionConnectedStateTypeId, connected);

    foreach (Thing *groupDevice, myThings().filterByParentId(thing->id())) {
        groupDevice->setStateValue(sonosGroupConnectedStateTypeId, connected);
    }
}

void IntegrationPluginSonos::onAuthenticationStatusChanged(bool authenticated)
{
    Sonos *sonosConnection = static_cast<Sonos *>(sender());
    Thing *thing = m_sonosConnections.key(sonosConnection);
    if (!thing)
        return;

    thing->setStateValue(sonosConnectionLoggedInStateTypeId, authenticated);
    if (!authenticated) {
        //refresh access token needs to be refreshed
        pluginStorage()->beginGroup(thing->id().toString());
        QByteArray refreshToken = pluginStorage()->value("refresh_token").toByteArray();
        pluginStorage()->endGroup();
        sonosConnection->getAccessTokenFromRefreshToken(refreshToken);
    }
}

void IntegrationPluginSonos::onHouseholdIdsReceived(QList<QString> householdIds)
{
    Sonos *sonos = static_cast<Sonos *>(sender());
    foreach(QString householdId, householdIds) {
        sonos->getGroups(householdId);
        sonos->getPlaylists(householdId);
    }
}

void IntegrationPluginSonos::onFavoritesReceived(QUuid requestId, const QString &householdId, QList<Sonos::FavoriteObject> favorites)
{
    Q_UNUSED(householdId)

    if (m_pendingBrowseResult.contains(requestId)) {
        BrowseResult *result = m_pendingBrowseResult.take(requestId);
        if (!result)
            return;

        foreach(Sonos::FavoriteObject favorite, favorites)  {
            MediaBrowserItem item;
            item.setId(result->itemId() + "/" + favorite.id);
            item.setExecutable(true);
            item.setBrowsable(false);
            if (!favorite.imageUrl.isEmpty()) {
                item.setThumbnail(favorite.imageUrl);
            } else {
                item.setIcon(BrowserItem::BrowserIconFavorites);
            }
            item.setDisplayName(favorite.name);
            item.setDescription(favorite.description);
            result->addItem(item);
            qCDebug(dcSonos()) << "Favorite: " << favorite.name << favorite.description;
        }
        result->finish(Thing::ThingErrorNoError);

    } else if (m_pendingBrowserItemResult.contains(requestId)) {
        BrowserItemResult *result = m_pendingBrowserItemResult.take(requestId);
        if (!result)
            return;
        QString favoriteId = result->itemId().remove(m_browseFavoritesPrefix);
        favoriteId.remove('/');

        foreach(Sonos::FavoriteObject favorite, favorites)  {
            if (favorite.id == favoriteId) {
                MediaBrowserItem item;
                item.setId(result->itemId());
                item.setExecutable(true);
                item.setBrowsable(false);
                if (!favorite.imageUrl.isEmpty()) {
                    item.setThumbnail(favorite.imageUrl);
                } else {
                    item.setIcon(BrowserItem::BrowserIconFavorites);
                }
                item.setDisplayName(favorite.name);
                item.setDescription(favorite.description);
                result->finish(item);
                return;
            }
        }
    }
}

void IntegrationPluginSonos::onPlaylistsReceived(const QString &householdId, QList<Sonos::PlaylistObject> playlists)
{
    Sonos *sonos = static_cast<Sonos *>(sender());

    foreach(Sonos::PlaylistObject playlist, playlists)  {
        qCDebug(dcSonos()) << "Playlist: " << playlist.name << playlist.type << playlist.trackCount;
        sonos->getPlaylist(householdId, playlist.id); //Get the playlist details
    }
}

void IntegrationPluginSonos::onPlaylistSummaryReceived(const QString &householdId, Sonos::PlaylistSummaryObject playlistSummary)
{
    Q_UNUSED(householdId);
    qCDebug(dcSonos()) << "Playlist summary received: " << playlistSummary.name;
    foreach(Sonos::PlaylistTrackObject track, playlistSummary.tracks)  {
        qCDebug(dcSonos()) << "---- Track: " << track.name << track.album << track.artist;
    }
}

void IntegrationPluginSonos::onGroupsReceived(const QString &householdId, QList<Sonos::GroupObject> groupObjects)
{
    Sonos *sonos = static_cast<Sonos *>(sender());
    Thing *parentDevice = m_sonosConnections.key(sonos);
    if (!parentDevice)
        return;

    QList<ThingDescriptor> deviceDescriptors;
    foreach(Sonos::GroupObject groupObject, groupObjects) {
        Thing *groupDevice = myThings().findByParams(ParamList() << Param(sonosGroupThingGroupIdParamTypeId, groupObject.groupId));
        if (groupDevice) {
            if (groupDevice->name() != groupObject.displayName) {
                qCDebug(dcSonos()) << "Updating group name" << groupDevice->name() << "to" << groupObject.displayName;
                groupDevice->setName(groupObject.displayName);
            }
        } else {
            //new thing, add to the system
            ThingDescriptor thingDescriptor(sonosGroupThingClassId, groupObject.displayName, "Sonos Group", parentDevice->id());
            ParamList params;
            params.append(Param(sonosGroupThingGroupIdParamTypeId, groupObject.groupId));
            params.append(Param(sonosGroupThingHouseholdIdParamTypeId, householdId));
            thingDescriptor.setParams(params);
            deviceDescriptors.append(thingDescriptor);
        }
    }

    if (!deviceDescriptors.isEmpty())
        emit autoThingsAppeared(deviceDescriptors);

    //delete auto devices
    foreach(Thing *groupDevice, myThings().filterByParentId(parentDevice->id())) {
        QString groupId = groupDevice->paramValue(sonosGroupThingGroupIdParamTypeId).toString();
        bool deviceRemoved = true;
        foreach (Sonos::GroupObject groupObject, groupObjects) {
            if(groupObject.groupId == groupId) {
                deviceRemoved = false;
            }
        }
        if (deviceRemoved) {
            emit autoThingDisappeared(groupDevice->id());
        }
    }

}

void IntegrationPluginSonos::onPlayBackStatusReceived(const QString &groupId, Sonos::PlayBackObject playBack)
{
    Thing *thing = myThings().findByParams(ParamList() << Param(sonosGroupThingGroupIdParamTypeId, groupId));
    if (!thing)
        return;

    thing->setStateValue(sonosGroupShuffleStateTypeId, playBack.playMode.shuffle);

    if (playBack.playMode.repeatOne) {
        thing->setStateValue(sonosGroupRepeatStateTypeId, "One");
    } else if (playBack.playMode.repeat)  {
        thing->setStateValue(sonosGroupRepeatStateTypeId, "All");
    } else {
        thing->setStateValue(sonosGroupRepeatStateTypeId, "None");
    }

    switch (playBack.playbackState) {
    case Sonos::PlayBackStateIdle:
        thing->setStateValue(sonosGroupPlaybackStatusStateTypeId, "Stopped");
        break;
    case Sonos::PlayBackStatePause:
        thing->setStateValue(sonosGroupPlaybackStatusStateTypeId, "Paused");
        break;
    case Sonos::PlayBackStateBuffering:
    case Sonos::PlayBackStatePlaying:
        thing->setStateValue(sonosGroupPlaybackStatusStateTypeId, "Playing");
        break;
    }
}

void IntegrationPluginSonos::onMetadataStatusReceived(const QString &groupId, Sonos::MetadataStatus metaDataStatus)
{
    Thing *thing = myThings().findByParams(ParamList() << Param(sonosGroupThingGroupIdParamTypeId, groupId));
    if (!thing)
        return;

    thing->setStateValue(sonosGroupTitleStateTypeId, metaDataStatus.currentItem.track.name);
    thing->setStateValue(sonosGroupArtistStateTypeId, metaDataStatus.currentItem.track.artist.name);
    thing->setStateValue(sonosGroupCollectionStateTypeId, metaDataStatus.currentItem.track.album.name);
    if (!metaDataStatus.currentItem.track.imageUrl.isEmpty()){
        thing->setStateValue(sonosGroupArtworkStateTypeId, metaDataStatus.currentItem.track.imageUrl);
    } else {
        thing->setStateValue(sonosGroupArtworkStateTypeId, metaDataStatus.container.imageUrl);
    }
}

void IntegrationPluginSonos::onVolumeReceived(const QString &groupId, Sonos::VolumeObject groupVolume)
{
    Thing *thing = myThings().findByParams(ParamList() << Param(sonosGroupThingGroupIdParamTypeId, groupId));
    if (!thing)
        return;

    thing->setStateValue(sonosGroupVolumeStateTypeId, groupVolume.volume);
    thing->setStateValue(sonosGroupMuteStateTypeId, groupVolume.muted);
}

void IntegrationPluginSonos::onActionExecuted(QUuid sonosActionId, bool success)
{
    if (m_pendingActions.contains(sonosActionId)) {
        QPointer<ThingActionInfo> info = m_pendingActions.value(sonosActionId);
        if (info.isNull()) {
            qCWarning(dcSonos()) << "ThingActionInfo has disappeared. Did it time out?";
            return;
        }
        if (success) {
            info->finish(Thing::ThingErrorNoError);
        } else {
            info->finish(Thing::ThingErrorHardwareFailure);
        }
    }

    if (m_pendingBrowserExecution.contains(sonosActionId)) {
        BrowserActionInfo *info = m_pendingBrowserExecution.value(sonosActionId);
        if (!info) {
            qCWarning(dcSonos()) << "BrowseActionInfo has disappeared. Did it time out?";
            return;
        }

        if (success) {
            info->finish(Thing::ThingErrorNoError);
        } else {
            info->finish(Thing::ThingErrorHardwareFailure);
        }
    }
}
