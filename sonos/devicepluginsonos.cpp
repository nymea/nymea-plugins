/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *                                                                         *
 *  Copyright (C) 2019 Bernhard Trinnes <bernhard.trinnes@nymea.io         *
 *                                                                         *
 *  This file is part of nymea.                                            *
 *                                                                         *
 *  This library is free software; you can redistribute it and/or          *
 *  modify it under the terms of the GNU Lesser General Public             *
 *  License as published by the Free Software Foundation; either           *
 *  version 2.1 of the License, or (at your option) any later version.     *
 *                                                                         *
 *  This library is distributed in the hope that it will be useful,        *
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of         *
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU      *
 *  Lesser General Public License for more details.                        *
 *                                                                         *
 *  You should have received a copy of the GNU Lesser General Public       *
 *  License along with this library; If not, see                           *
 *  <http://www.gnu.org/licenses/>.                                        *
 *                                                                         *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

#include "devicepluginsonos.h"
#include "devices/device.h"
#include "network/networkaccessmanager.h"
#include "plugininfo.h"

#include <QNetworkRequest>
#include <QNetworkReply>
#include <QUrlQuery>
#include <QJsonDocument>

DevicePluginSonos::DevicePluginSonos()
{
}


DevicePluginSonos::~DevicePluginSonos()
{
    if (m_pluginTimer5sec)
        hardwareManager()->pluginTimerManager()->unregisterTimer(m_pluginTimer5sec);
    if (m_pluginTimer60sec)
        hardwareManager()->pluginTimerManager()->unregisterTimer(m_pluginTimer60sec);
}


void DevicePluginSonos::setupDevice(DeviceSetupInfo *info)
{
    Device *device = info->device();

    if (device->deviceClassId() == sonosConnectionDeviceClassId) {
        Sonos *sonos;
        if (m_setupSonosConnections.keys().contains(device->id())) {
            //Fresh device setup, has already a fresh access token
            qCDebug(dcSonos()) << "Sonos OAuth setup complete";
            sonos = m_setupSonosConnections.take(device->id());
            connect(sonos, &Sonos::connectionChanged, this, &DevicePluginSonos::onConnectionChanged);
            connect(sonos, &Sonos::householdIdsReceived, this, &DevicePluginSonos::onHouseholdIdsReceived);
            connect(sonos, &Sonos::groupsReceived, this, &DevicePluginSonos::onGroupsReceived);
            connect(sonos, &Sonos::playBackStatusReceived, this, &DevicePluginSonos::onPlayBackStatusReceived);
            connect(sonos, &Sonos::metadataStatusReceived, this, &DevicePluginSonos::onMetadataStatusReceived);
            connect(sonos, &Sonos::volumeReceived, this, &DevicePluginSonos::onVolumeReceived);
            connect(sonos, &Sonos::actionExecuted, this, &DevicePluginSonos::onActionExecuted);
            connect(sonos, &Sonos::authenticationStatusChanged, this, &DevicePluginSonos::onAuthenticationStatusChanged);

            connect(sonos, &Sonos::authenticationStatusChanged, info, [info](bool authenticated){
                if (authenticated) {
                    info->finish(Device::DeviceErrorNoError);
                } else {
                    info->finish(Device::DeviceErrorAuthenticationFailure);
                }
            });

            m_sonosConnections.insert(device, sonos);
            return info->finish(Device::DeviceErrorNoError);
        } else {
            //device loaded from the device database, needs a new access token;
            pluginStorage()->beginGroup(device->id().toString());
            QByteArray refreshToken = pluginStorage()->value("refresh_token").toByteArray();
            pluginStorage()->endGroup();

            sonos = new Sonos(hardwareManager()->networkManager(), "0a8f6d44-d9d1-4474-bcfa-cfb41f8b66e8", "3095ce48-0c5d-47ce-a1f4-6005c7b8fdb5", this);
            connect(sonos, &Sonos::connectionChanged, this, &DevicePluginSonos::onConnectionChanged);
            connect(sonos, &Sonos::householdIdsReceived, this, &DevicePluginSonos::onHouseholdIdsReceived);
            connect(sonos, &Sonos::groupsReceived, this, &DevicePluginSonos::onGroupsReceived);
            connect(sonos, &Sonos::playBackStatusReceived, this, &DevicePluginSonos::onPlayBackStatusReceived);
            connect(sonos, &Sonos::metadataStatusReceived, this, &DevicePluginSonos::onMetadataStatusReceived);
            connect(sonos, &Sonos::volumeReceived, this, &DevicePluginSonos::onVolumeReceived);
            connect(sonos, &Sonos::actionExecuted, this, &DevicePluginSonos::onActionExecuted);
            connect(sonos, &Sonos::authenticationStatusChanged, this, &DevicePluginSonos::onAuthenticationStatusChanged);
            connect(sonos, &Sonos::favouritesReceived, this, &DevicePluginSonos::onFavouritesReceived);
            sonos->getAccessTokenFromRefreshToken(refreshToken);
            m_sonosConnections.insert(device, sonos);
            return info->finish(Device::DeviceErrorNoError);
        }
    }

    if (device->deviceClassId() == sonosGroupDeviceClassId) {
        return info->finish(Device::DeviceErrorNoError);
    }

    qCWarning(dcSonos()) << "Unhandled device class id in setupDevice" << device->deviceClassId();
}

void DevicePluginSonos::startPairing(DevicePairingInfo *info)
{
    if (info->deviceClassId() == sonosConnectionDeviceClassId) {

        Sonos *sonos = new Sonos(hardwareManager()->networkManager(), "0a8f6d44-d9d1-4474-bcfa-cfb41f8b66e8", "3095ce48-0c5d-47ce-a1f4-6005c7b8fdb5", this);
        QUrl url = sonos->getLoginUrl(QUrl("https://127.0.0.1:8888"));
        qCDebug(dcSonos()) << "Sonos url:" << url;
        info->setOAuthUrl(url);
        info->finish(Device::DeviceErrorNoError);
        m_setupSonosConnections.insert(info->deviceId(), sonos);
        return;
    }

    qCWarning(dcSonos()) << "Unhandled pairing metod!";
    info->finish(Device::DeviceErrorCreationMethodNotSupported);
}

void DevicePluginSonos::confirmPairing(DevicePairingInfo *info, const QString &username, const QString &secret)
{
    Q_UNUSED(username)

    if (info->deviceClassId() == sonosConnectionDeviceClassId) {
        qCDebug(dcSonos()) << "Redirect url is" << secret;
        QUrl url(secret);
        QUrlQuery query(url);
        QByteArray authorizationCode = query.queryItemValue("code").toLocal8Bit();
        QByteArray state = query.queryItemValue("state").toLocal8Bit();
        //TODO evaluate state if it equals the given state

        Sonos *sonos = m_setupSonosConnections.value(info->deviceId());

        if (!sonos) {
            qWarning(dcSonos()) << "No sonos connection found for device:" << info->deviceName();
            m_setupSonosConnections.remove(info->deviceId());
            sonos->deleteLater();
            info->finish(Device::DeviceErrorHardwareFailure);
            return;
        }
        sonos->getAccessTokenFromAuthorizationCode(authorizationCode);
        connect(sonos, &Sonos::authenticationStatusChanged, info, [this, info, sonos](bool authenticated){
            if(!authenticated) {
                qWarning(dcSonos()) << "Authentication process failed"  << info->deviceName();
                m_setupSonosConnections.remove(info->deviceId());
                sonos->deleteLater();
                info->finish(Device::DeviceErrorSetupFailed, QT_TR_NOOP("Authentication failed. Please try again."));
                return;
            }
            QByteArray accessToken = sonos->accessToken();
            QByteArray refreshToken = sonos->refreshToken();
            qCDebug(dcSonos()) << "Token:" << accessToken << refreshToken;

            pluginStorage()->beginGroup(info->deviceId().toString());
            pluginStorage()->setValue("refresh_token", refreshToken);
            pluginStorage()->endGroup();

            info->finish(Device::DeviceErrorNoError);
        });
        return;
    }
    qCWarning(dcSonos()) << "Invalid deviceclassId -> no pairing possible with this device";
    info->finish(Device::DeviceErrorDeviceClassNotFound);
}

void DevicePluginSonos::postSetupDevice(Device *device)
{
    if (!m_pluginTimer5sec) {
        m_pluginTimer5sec = hardwareManager()->pluginTimerManager()->registerTimer(5);
        connect(m_pluginTimer5sec, &PluginTimer::timeout, this, [this]() {

            foreach (Device *connectionDevice, myDevices().filterByDeviceClassId(sonosConnectionDeviceClassId)) {
                Sonos *sonos = m_sonosConnections.value(connectionDevice);
                if (!sonos) {
                    qWarning(dcSonos()) << "No sonos connection found to device" << connectionDevice->name();
                    continue;
                }
                foreach (Device *groupDevice, myDevices().filterByParentDeviceId(connectionDevice->id())) {
                    if (groupDevice->deviceClassId() == sonosGroupDeviceClassId) {
                        //get playback status of each group
                        QString groupId = groupDevice->paramValue(sonosGroupDeviceGroupIdParamTypeId).toString();
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
            foreach (Device *device, myDevices().filterByDeviceClassId(sonosConnectionDeviceClassId)) {
                Sonos *sonos = m_sonosConnections.value(device);
                if (!sonos) {
                    qWarning(dcSonos()) << "No sonos connection found to device" << device->name();
                    continue;
                }
                //get groups for each household in order to add or remove groups
                sonos->getHouseholds();
            }
        });
    }

    if (device->deviceClassId() == sonosConnectionDeviceClassId) {
        Sonos *sonos = m_sonosConnections.value(device);
        sonos->getHouseholds();
    }

    if (device->deviceClassId() == sonosGroupDeviceClassId) {
        Device *parentDevice = myDevices().findById(device->parentId());
        Sonos *sonos = m_sonosConnections.value(parentDevice);
        if (!sonos) {
            return;
        }
        QString groupId = device->paramValue(sonosGroupDeviceGroupIdParamTypeId).toString();
        sonos->getGroupPlaybackStatus(groupId);
        sonos->getGroupMetadataStatus(groupId);
        sonos->getGroupVolume(groupId);
    }
}


void DevicePluginSonos::startMonitoringAutoDevices()
{
    foreach (Device *device, myDevices()) {
        if (device->deviceClassId() == sonosGroupDeviceClassId) {
            return;
        }
    }
}

void DevicePluginSonos::deviceRemoved(Device *device)
{
    qCDebug(dcSonos) << "Delete " << device->name();
    if (myDevices().empty()) {
        hardwareManager()->pluginTimerManager()->unregisterTimer(m_pluginTimer5sec);
        hardwareManager()->pluginTimerManager()->unregisterTimer(m_pluginTimer60sec);
        m_pluginTimer5sec = nullptr;
        m_pluginTimer60sec = nullptr;
    }
}


void DevicePluginSonos::executeAction(DeviceActionInfo *info)
{
    Device *device = info->device();
    Action action = info->action();

    if (device->deviceClassId() == sonosGroupDeviceClassId) {
        Sonos *sonos = m_sonosConnections.value(myDevices().findById(device->parentId()));
        QString groupId = device->paramValue(sonosGroupDeviceGroupIdParamTypeId).toString();

        if (!sonos) {
            qWarning(dcSonos()) << "Action cannot be executed: Sonos connection not available";
            return info->finish(Device::DeviceErrorHardwareNotAvailable, QT_TR_NOOP("Sonos device is not available."));
        }

        if (action.actionTypeId()  == sonosGroupPlayActionTypeId) {
            m_pendingActions.insert(sonos->groupPlay(groupId), QPointer<DeviceActionInfo>(info));
            return;
        }

        if (action.actionTypeId()  == sonosGroupShuffleActionTypeId) {
            bool shuffle = action.param(sonosGroupShuffleActionShuffleParamTypeId).value().toBool();
            m_pendingActions.insert(sonos->groupSetShuffle(groupId, shuffle), QPointer<DeviceActionInfo>(info));
            return;
        }

        if (action.actionTypeId()  == sonosGroupRepeatActionTypeId) {
            if (action.param(sonosGroupRepeatActionRepeatParamTypeId).value().toString() == "None") {
                m_pendingActions.insert(sonos->groupSetRepeat(groupId, Sonos::RepeatModeNone), QPointer<DeviceActionInfo>(info));
            } else if (action.param(sonosGroupRepeatActionRepeatParamTypeId).value().toString() == "One") {
                m_pendingActions.insert(sonos->groupSetRepeat(groupId, Sonos::RepeatModeOne), QPointer<DeviceActionInfo>(info));
            } else if (action.param(sonosGroupRepeatActionRepeatParamTypeId).value().toString() == "All") {
                m_pendingActions.insert(sonos->groupSetRepeat(groupId, Sonos::RepeatModeAll), QPointer<DeviceActionInfo>(info));
            } else {
                return info->finish(Device::DeviceErrorHardwareFailure);
            }
            return;
        }

        if (action.actionTypeId() == sonosGroupPauseActionTypeId) {
            m_pendingActions.insert(sonos->groupPause(groupId), QPointer<DeviceActionInfo>(info));
            return;
        }

        if (action.actionTypeId() == sonosGroupStopActionTypeId) {
            m_pendingActions.insert(sonos->groupPause(groupId), QPointer<DeviceActionInfo>(info));
            return;
        }

        if (action.actionTypeId() == sonosGroupMuteActionTypeId) {
            bool mute = action.param(sonosGroupMuteActionMuteParamTypeId).value().toBool();
            m_pendingActions.insert(sonos->setGroupMute(groupId, mute), QPointer<DeviceActionInfo>(info));
            return;
        }


        if (action.actionTypeId() == sonosGroupVolumeActionTypeId) {
            int volume = action.param(sonosGroupVolumeActionVolumeParamTypeId).value().toInt();
            m_pendingActions.insert(sonos->setGroupVolume(groupId, volume), QPointer<DeviceActionInfo>(info));
            return;
        }

        if (action.actionTypeId() == sonosGroupSkipNextActionTypeId) {
            m_pendingActions.insert(sonos->groupSkipToNextTrack(groupId), QPointer<DeviceActionInfo>(info));
            return;
        }

        if (action.actionTypeId() == sonosGroupSkipBackActionTypeId) {
            m_pendingActions.insert(sonos->groupSkipToPreviousTrack(groupId), QPointer<DeviceActionInfo>(info));
            return;
        }

        if (action.actionTypeId() == sonosGroupPlaybackStatusActionTypeId) {
            QString playbackStatus = action.param(sonosGroupPlaybackStatusActionPlaybackStatusParamTypeId).value().toString();
            if (playbackStatus == "Playing") {
                m_pendingActions.insert(sonos->groupPlay(groupId), QPointer<DeviceActionInfo>(info));
            } else if(playbackStatus == "Stopped") {
                m_pendingActions.insert(sonos->groupPause(groupId), QPointer<DeviceActionInfo>(info));
            } else if(playbackStatus == "Paused") {
                m_pendingActions.insert(sonos->groupPause(groupId), QPointer<DeviceActionInfo>(info));
            }
            return;
        }
        return info->finish(Device::DeviceErrorActionTypeNotFound);
    }
    info->finish(Device::DeviceErrorDeviceClassNotFound);
}

void DevicePluginSonos::browseDevice(BrowseResult *result)
{
    Device *parentDevice = myDevices().findById(result->device()->parentId());
    Sonos *sonosConnection = m_sonosConnections.value(parentDevice);
    if (!sonosConnection)
        return;

    qDebug(dcSonos()) << "Browse Device" << result->itemId();
    QString householdId = result->device()->paramValue(sonosGroupDeviceHouseholdIdParamTypeId).toString();
    if (result->itemId().isEmpty()){
        BrowserItem item;
        item.setId("favorites");
        item.setIcon(BrowserItem::BrowserIconFavorites);
        item.setExecutable(false);
        item.setBrowsable(true);
        item.setDisplayName("Favorites");
        result->addItem(item);
        result->finish(Device::DeviceErrorNoError);
    } else if (result->itemId() == "favorites") {
        sonosConnection->getFavorites(householdId);
        m_pendingBrowseResult.insert(householdId, result);
    } else {
        //TODO add media browsing
        result->finish(Device::DeviceErrorItemNotFound);
    }
}

void DevicePluginSonos::browserItem(BrowserItemResult *result)
{
    Q_UNUSED(result)
}

void DevicePluginSonos::executeBrowserItem(BrowserActionInfo *info)
{
    Device *parentDevice = myDevices().findById(info->device()->parentId());
    Sonos *sonosConnection = m_sonosConnections.value(parentDevice);
    if (!sonosConnection)
        return;

    QString groupId = info->device()->paramValue(sonosGroupDeviceGroupIdParamTypeId).toString();
    QUuid requestId = sonosConnection->loadFavorite(groupId, info->browserAction().itemId());
    m_pendingBrowserExecution.insert(requestId, info);
}

void DevicePluginSonos::executeBrowserItemAction(BrowserItemActionInfo *info)
{
    Q_UNUSED(info)
}

void DevicePluginSonos::onConnectionChanged(bool connected)
{
    Sonos *sonos = static_cast<Sonos *>(sender());
    Device *device = m_sonosConnections.key(sonos);
    if (!device)
        return;
    device->setStateValue(sonosConnectionConnectedStateTypeId, connected);

    foreach (Device *groupDevice, myDevices().filterByParentDeviceId(device->id())) {
        groupDevice->setStateValue(sonosGroupConnectedStateTypeId, connected);
    }
}

void DevicePluginSonos::onAuthenticationStatusChanged(bool authenticated)
{
    Sonos *sonosConnection = static_cast<Sonos *>(sender());
    Device *device = m_sonosConnections.key(sonosConnection);
    if (!device)
        return;

    device->setStateValue(sonosConnectionLoggedInStateTypeId, authenticated);
    if (!authenticated) {
        //refresh access token needs to be refreshed
        pluginStorage()->beginGroup(device->id().toString());
        QByteArray refreshToken = pluginStorage()->value("refresh_token").toByteArray();
        pluginStorage()->endGroup();
        sonosConnection->getAccessTokenFromRefreshToken(refreshToken);
    }
}

void DevicePluginSonos::onHouseholdIdsReceived(QList<QString> householdIds)
{
    Sonos *sonos = static_cast<Sonos *>(sender());
    foreach(QString householdId, householdIds) {
        sonos->getGroups(householdId);
        sonos->getPlaylists(householdId);
    }
}

void DevicePluginSonos::onFavouritesReceived(const QString &householdId, QList<Sonos::FavouriteObject> favourites)
{
    if (m_pendingBrowseResult.contains(householdId)) {
        BrowseResult *result = m_pendingBrowseResult.take(householdId);
        if (!result)
            return;

        foreach(Sonos::FavouriteObject favourite, favourites)  {
            BrowserItem item;
            item.setId(favourite.id);
            item.setExecutable(true);
            item.setBrowsable(false);
            if (!favourite.imageUrl.isEmpty()) {
                 item.setThumbnail(favourite.imageUrl);
            } else {
                item.setIcon(BrowserItem::BrowserIconFavorites);
            }
            item.setDisplayName(favourite.name);
            item.setDescription(favourite.description);
            result->addItem(item);
            qDebug(dcSonos()) << "Favourite: " << favourite.name << favourite.description;
        }
        result->finish(Device::DeviceErrorNoError);
    } else {
        qDebug(dcSonos()) << "Received unhandled favourites list";
    }
}

void DevicePluginSonos::onPlaylistsReceived(const QString &householdId, QList<Sonos::PlaylistObject> playlists)
{
    Sonos *sonos = static_cast<Sonos *>(sender());

    foreach(Sonos::PlaylistObject playlist, playlists)  {
        qDebug(dcSonos()) << "Playlist: " << playlist.name << playlist.type << playlist.trackCount;
        sonos->getPlaylist(householdId, playlist.id); //Get the playlist details
    }
}

void DevicePluginSonos::onPlaylistSummaryReceived(const QString &householdId, Sonos::PlaylistSummaryObject playlistSummary)
{
    Q_UNUSED(householdId);
    qDebug(dcSonos()) << "Playlist summary received: " << playlistSummary.name;
    foreach(Sonos::PlaylistTrackObject track, playlistSummary.tracks)  {
        qDebug(dcSonos()) << "---- Track: " << track.name << track.album << track.artist;
    }
}

void DevicePluginSonos::onGroupsReceived(const QString &householdId, QList<Sonos::GroupObject> groupObjects)
{
    Sonos *sonos = static_cast<Sonos *>(sender());
    Device *parentDevice = m_sonosConnections.key(sonos);
    if (!parentDevice)
        return;

    QList<DeviceDescriptor> deviceDescriptors;
    foreach(Sonos::GroupObject groupObject, groupObjects) {
        Device *groupDevice = myDevices().findByParams(ParamList() << Param(sonosGroupDeviceGroupIdParamTypeId, groupObject.groupId));
        if (groupDevice) {
            if (groupDevice->name() != groupObject.displayName) {
                qDebug(dcSonos()) << "Updating group name" << groupDevice->name() << "to" << groupObject.displayName;
                groupDevice->setName(groupObject.displayName);
            }
        } else {
            //new device, add to the system
            DeviceDescriptor deviceDescriptor(sonosGroupDeviceClassId, groupObject.displayName, "Sonos Group", parentDevice->id());
            ParamList params;
            params.append(Param(sonosGroupDeviceGroupIdParamTypeId, groupObject.groupId));
            params.append(Param(sonosGroupDeviceHouseholdIdParamTypeId, householdId));
            deviceDescriptor.setParams(params);
            deviceDescriptors.append(deviceDescriptor);
        }
    }

    if (!deviceDescriptors.isEmpty())
        emit autoDevicesAppeared(deviceDescriptors);

    //delete auto devices
    foreach(Device *groupDevice, myDevices().filterByParentDeviceId(parentDevice->id())) {
        QString groupId = groupDevice->paramValue(sonosGroupDeviceGroupIdParamTypeId).toString();
        bool deviceRemoved = true;
        foreach (Sonos::GroupObject groupObject, groupObjects) {
            if(groupObject.groupId == groupId) {
                deviceRemoved = false;
            }
        }
        if (deviceRemoved) {
            emit autoDeviceDisappeared(groupDevice->id());
        }
    }

}

void DevicePluginSonos::onPlayBackStatusReceived(const QString &groupId, Sonos::PlayBackObject playBack)
{
    Device *device = myDevices().findByParams(ParamList() << Param(sonosGroupDeviceGroupIdParamTypeId, groupId));
    if (!device)
        return;

    device->setStateValue(sonosGroupShuffleStateTypeId, playBack.playMode.shuffle);

    if (playBack.playMode.repeatOne) {
        device->setStateValue(sonosGroupRepeatStateTypeId, "One");
    } else if (playBack.playMode.repeat)  {
        device->setStateValue(sonosGroupRepeatStateTypeId, "All");
    } else {
        device->setStateValue(sonosGroupRepeatStateTypeId, "None");
    }

    switch (playBack.playbackState) {
    case Sonos::PlayBackStateIdle:
        device->setStateValue(sonosGroupPlaybackStatusStateTypeId, "Stopped");
        break;
    case Sonos::PlayBackStatePause:
        device->setStateValue(sonosGroupPlaybackStatusStateTypeId, "Paused");
        break;
    case Sonos::PlayBackStateBuffering:
    case Sonos::PlayBackStatePlaying:
        device->setStateValue(sonosGroupPlaybackStatusStateTypeId, "Playing");
        break;
    }
}

void DevicePluginSonos::onMetadataStatusReceived(const QString &groupId, Sonos::MetadataStatus metaDataStatus)
{
    Device *device = myDevices().findByParams(ParamList() << Param(sonosGroupDeviceGroupIdParamTypeId, groupId));
    if (!device)
        return;

    device->setStateValue(sonosGroupTitleStateTypeId, metaDataStatus.currentItem.track.name);
    device->setStateValue(sonosGroupArtistStateTypeId, metaDataStatus.currentItem.track.artist.name);
    device->setStateValue(sonosGroupCollectionStateTypeId, metaDataStatus.currentItem.track.album.name);
    if (!metaDataStatus.currentItem.track.imageUrl.isEmpty()){
        device->setStateValue(sonosGroupArtworkStateTypeId, metaDataStatus.currentItem.track.imageUrl);
    } else {
        device->setStateValue(sonosGroupArtworkStateTypeId, metaDataStatus.container.imageUrl);
    }
}

void DevicePluginSonos::onVolumeReceived(const QString &groupId, Sonos::VolumeObject groupVolume)
{
    Device *device = myDevices().findByParams(ParamList() << Param(sonosGroupDeviceGroupIdParamTypeId, groupId));
    if (!device)
        return;

    device->setStateValue(sonosGroupVolumeStateTypeId, groupVolume.volume);
    device->setStateValue(sonosGroupMuteStateTypeId, groupVolume.muted);
}

void DevicePluginSonos::onActionExecuted(QUuid sonosActionId, bool success)
{
    if (m_pendingActions.contains(sonosActionId)) {
        QPointer<DeviceActionInfo> info = m_pendingActions.value(sonosActionId);
        if (info.isNull()) {
            qCWarning(dcSonos()) << "DeviceActionInfo has disappeared. Did it time out?";
            return;
        }
        if (success) {
            info->finish(Device::DeviceErrorNoError);
        } else {
            info->finish(Device::DeviceErrorHardwareFailure);
        }
    }

    if (m_pendingBrowserExecution.contains(sonosActionId)) {
        BrowserActionInfo *info = m_pendingBrowserExecution.value(sonosActionId);
        if (!info) {
            qCWarning(dcSonos()) << "BrowseActionInfo has disappeared. Did it time out?";
            return;
        }

        if (success) {
            info->finish(Device::DeviceErrorNoError);
        } else {
            info->finish(Device::DeviceErrorHardwareFailure);
        }
    }
}
