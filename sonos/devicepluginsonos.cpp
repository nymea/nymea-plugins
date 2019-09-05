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
    hardwareManager()->pluginTimerManager()->unregisterTimer(m_pluginTimer5sec);
    hardwareManager()->pluginTimerManager()->unregisterTimer(m_pluginTimer60sec);
}


Device::DeviceSetupStatus DevicePluginSonos::setupDevice(Device *device)
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
        qCDebug(dcSonos()) << "Sonos OAuth setup complete";
        Sonos *sonos;
        if (m_setupSonosConnections.keys().contains(device->id())) {
            //Fresh device setup, has already a fresh access token
            sonos = m_setupSonosConnections.take(device->id());
        } else {
            //device loaded from the device database, needs a new access token;
            pluginStorage()->beginGroup(device->id().toString());
            QByteArray refreshToken = pluginStorage()->value("refresh_token").toByteArray();
            pluginStorage()->endGroup();

            sonos = new Sonos(hardwareManager()->networkManager(), "b15cbf8c-a39c-47aa-bd93-635a96e9696c", "c086ba71-e562-430b-a52f-867c6482fd11", "", this);
            sonos->getAccessTokenFromRefreshToken(refreshToken);
        }

        connect(sonos, &Sonos::connectionChanged, this, &DevicePluginSonos::onConnectionChanged);
        connect(sonos, &Sonos::householdIdsReceived, this, &DevicePluginSonos::onHouseholdIdsReceived);
        connect(sonos, &Sonos::groupsReceived, this, &DevicePluginSonos::onGroupsReceived);
        connect(sonos, &Sonos::playBackStatusReceived, this, &DevicePluginSonos::onPlayBackStatusReceived);
        connect(sonos, &Sonos::metadataStatusReceived, this, &DevicePluginSonos::onMetadataStatusReceived);
        connect(sonos, &Sonos::volumeReceived, this, &DevicePluginSonos::onVolumeReceived);
        connect(sonos, &Sonos::actionExecuted, this, &DevicePluginSonos::onActionExecuted);
        m_sonosConnections.insert(device, sonos);
    }

    if (device->deviceClassId() == sonosGroupDeviceClassId) {
    }
    return Device::DeviceSetupStatusSuccess;
}

DevicePairingInfo DevicePluginSonos::pairDevice(DevicePairingInfo &devicePairingInfo)
{
    if (devicePairingInfo.deviceClassId() == sonosConnectionDeviceClassId) {

        Sonos *sonos = new Sonos(hardwareManager()->networkManager(), "b15cbf8c-a39c-47aa-bd93-635a96e9696c", "c086ba71-e562-430b-a52f-867c6482fd11", "", this);
        QUrl url = sonos->getLoginUrl(QUrl("https://127.0.0.1:8000"));
        qCDebug(dcSonos()) << "Sonos url:" << url;
        devicePairingInfo.setOAuthUrl(url);
        devicePairingInfo.setStatus(Device::DeviceErrorNoError);
        m_setupSonosConnections.insert(devicePairingInfo.deviceId(), sonos);
        return devicePairingInfo;
    }

    qCWarning(dcSonos()) << "Unhandled pairing metod!";
    devicePairingInfo.setStatus(Device::DeviceErrorCreationMethodNotSupported);
    return devicePairingInfo;
}

DevicePairingInfo DevicePluginSonos::confirmPairing(DevicePairingInfo &devicePairingInfo, const QString &username, const QString &secret)
{
    Q_UNUSED(username);
    qCDebug(dcSonos()) << "Confirm pairing";

    if (devicePairingInfo.deviceClassId() == sonosConnectionDeviceClassId) {
        qCDebug(dcSonos()) << "Secret is" << secret;
        QUrl url(secret);
        QUrlQuery query(url);
        QByteArray accessCode = query.queryItemValue("code").toLocal8Bit();
        qCDebug(dcSonos()) << "Acess code is:" << accessCode;

        Sonos *sonos = m_setupSonosConnections.value(devicePairingInfo.deviceId());

        if (!sonos) {
            m_setupSonosConnections.remove(devicePairingInfo.deviceId());
            sonos->deleteLater();
            devicePairingInfo.setStatus(Device::DeviceErrorHardwareFailure);
            return devicePairingInfo;
        }
        sonos->getAccessTokenFromAuthorizationCode(accessCode);
        connect(sonos, &Sonos::authenticationStatusChanged, this, [devicePairingInfo, this](bool authenticated){
            Sonos *sonos = static_cast<Sonos *>(sender());
            DevicePairingInfo info(devicePairingInfo);
            if(!authenticated) {
                m_setupSonosConnections.remove(info.deviceId());
                sonos->deleteLater();
                info.setStatus(Device::DeviceErrorSetupFailed);
                emit pairingFinished(info);
                return;
            }
            QByteArray accessToken = sonos->accessToken();
            QByteArray refreshToken = sonos->refreshToken();

            pluginStorage()->beginGroup(info.deviceId().toString());
            pluginStorage()->setValue("access_token", accessToken);
            pluginStorage()->setValue("refresh_token", refreshToken);
            pluginStorage()->endGroup();

            info.setStatus(Device::DeviceErrorNoError);
            emit pairingFinished(info);
        });
        devicePairingInfo.setStatus(Device::DeviceErrorAsync);
        return devicePairingInfo;
    }
    qCWarning(dcSonos()) << "Invalid deviceclassId -> no pairing possible with this device";
    devicePairingInfo.setStatus(Device::DeviceErrorHardwareFailure);
    return devicePairingInfo;
}

void DevicePluginSonos::postSetupDevice(Device *device)
{
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


Device::DeviceError DevicePluginSonos::executeAction(Device *device, const Action &action)
{
    if (device->deviceClassId() == sonosGroupDeviceClassId) {
        Sonos *sonos = m_sonosConnections.value(myDevices().findById(device->parentId()));
        QString groupId = device->paramValue(sonosGroupDeviceGroupIdParamTypeId).toString();

        if (!sonos) {
            qWarning(dcSonos()) << "Action cannot be executed: Sonos connection not available";
            return Device::DeviceErrorInvalidParameter;
        }

        if (action.actionTypeId()  == sonosGroupPlayActionTypeId) {
            m_pendingActions.insert(sonos->groupPlay(groupId), action.id());
            return Device::DeviceErrorAsync;
        }

        if (action.actionTypeId()  == sonosGroupShuffleActionTypeId) {
            bool shuffle = action.param(sonosGroupShuffleActionShuffleParamTypeId).value().toBool();
            m_pendingActions.insert(sonos->groupSetShuffle(groupId, shuffle), action.id());
            return Device::DeviceErrorAsync;
        }

        if (action.actionTypeId()  == sonosGroupRepeatActionTypeId) {
            if (action.param(sonosGroupRepeatActionRepeatParamTypeId).value().toString() == "None") {
                m_pendingActions.insert(sonos->groupSetRepeat(groupId, Sonos::RepeatModeNone), action.id());
            } else if (action.param(sonosGroupRepeatActionRepeatParamTypeId).value().toString() == "One") {
                m_pendingActions.insert(sonos->groupSetRepeat(groupId, Sonos::RepeatModeOne), action.id());
            } else if (action.param(sonosGroupRepeatActionRepeatParamTypeId).value().toString() == "All") {
                m_pendingActions.insert(sonos->groupSetRepeat(groupId, Sonos::RepeatModeAll), action.id());
            } else {
                return Device::DeviceErrorHardwareFailure;
            }
            return Device::DeviceErrorAsync;
        }

        if (action.actionTypeId() == sonosGroupPauseActionTypeId) {
            m_pendingActions.insert(sonos->groupPause(groupId), action.id());
            return Device::DeviceErrorAsync;
        }

        if (action.actionTypeId() == sonosGroupStopActionTypeId) {
            m_pendingActions.insert(sonos->groupPause(groupId), action.id());
            return Device::DeviceErrorAsync;
        }

        if (action.actionTypeId() == sonosGroupMuteActionTypeId) {
            bool mute = action.param(sonosGroupMuteActionMuteParamTypeId).value().toBool();
            m_pendingActions.insert(sonos->setGroupMute(groupId, mute), action.id());
            return Device::DeviceErrorAsync;
        }


        if (action.actionTypeId() == sonosGroupVolumeActionTypeId) {
            int volume = action.param(sonosGroupVolumeActionVolumeParamTypeId).value().toInt();
            m_pendingActions.insert(sonos->setGroupVolume(groupId, volume), action.id());
            return Device::DeviceErrorAsync;
        }

        if (action.actionTypeId() == sonosGroupSkipNextActionTypeId) {
            m_pendingActions.insert(sonos->groupSkipToNextTrack(groupId), action.id());
            return Device::DeviceErrorAsync;
        }

        if (action.actionTypeId() == sonosGroupSkipBackActionTypeId) {
            m_pendingActions.insert(sonos->groupSkipToPreviousTrack(groupId), action.id());
            return Device::DeviceErrorAsync;
        }

        if (action.actionTypeId() == sonosGroupPlaybackStatusActionTypeId) {
            QString playbackStatus = action.param(sonosGroupPlaybackStatusActionPlaybackStatusParamTypeId).value().toString();
            if (playbackStatus == "Playing") {
                m_pendingActions.insert(sonos->groupPlay(groupId), action.id());
            } else if(playbackStatus == "Stopped") {
                m_pendingActions.insert(sonos->groupPause(groupId), action.id());
            } else if(playbackStatus == "Paused") {
                m_pendingActions.insert(sonos->groupPause(groupId), action.id());
            }
            return Device::DeviceErrorAsync;
        }
        return Device::DeviceErrorActionTypeNotFound;
    }
    return Device::DeviceErrorDeviceClassNotFound;
}

void DevicePluginSonos::onConnectionChanged(bool connected)
{
    Sonos *sonos = static_cast<Sonos *>(sender());
    Device *device = m_sonosConnections.key(sonos);
    device->setStateValue(sonosConnectionConnectedStateTypeId, connected);

    foreach (Device *groupDevice, myDevices().filterByParentDeviceId(device->id())) {
        groupDevice->setStateValue(sonosGroupConnectedStateTypeId, connected);
    }
}

void DevicePluginSonos::onAuthenticationStatusChanged(bool authenticated)
{
    Sonos *sonosConnection = static_cast<Sonos *>(sender());
    Device *device = m_sonosConnections.key(sonosConnection);
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
        sonos->getFavorites(householdId);
        sonos->getPlaylists(householdId);
    }
}

void DevicePluginSonos::onFavouritesReceived(const QString &householdId, QList<Sonos::FavouriteObject> favourites)
{
    Q_UNUSED(householdId);
    foreach(Sonos::FavouriteObject favourite, favourites)  {
        qDebug(dcSonos()) << "Favourite: " << favourite.name << favourite.description;
    }
}

void DevicePluginSonos::onPlaylistsReceived(const QString &householdId, QList<Sonos::PlaylistObject> playlists)
{
    Sonos *sonos = static_cast<Sonos *>(sender());
    foreach(Sonos::PlaylistObject playlist, playlists)  {
        qDebug(dcSonos()) << "Playlist: " << playlist.name << playlist.type << playlist.trackCount;
        sonos->getPlaylist(householdId, playlist.id);
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
    Q_UNUSED(householdId);
    Sonos *sonos = static_cast<Sonos *>(sender());
    Device *parentDevice = m_sonosConnections.key(sonos);
    if (!parentDevice)
        return;

    QList<DeviceDescriptor> deviceDescriptors;
    foreach(Sonos::GroupObject groupObject, groupObjects) {
        Device *groupDevice = myDevices().findByParams(ParamList() << Param(sonosGroupDeviceGroupIdParamTypeId, groupObject.groupId));
        if (groupDevice) {
            groupDevice->setName(groupObject.displayName);
        } else {
            //new device, add to the system
            DeviceDescriptor deviceDescriptor(sonosGroupDeviceClassId, groupObject.displayName, "Sonos Group", parentDevice->id());
            ParamList params;
            params.append(Param(sonosGroupDeviceGroupIdParamTypeId, groupObject.groupId));
            deviceDescriptor.setParams(params);
            deviceDescriptors.append(deviceDescriptor);
        }
    }

    if (!deviceDescriptors.isEmpty())
        emit autoDevicesAppeared(sonosGroupDeviceClassId, deviceDescriptors);

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
    //device->setStateValue(sonosGroupArtworkStateTypeId, metaDataStatus.currentItem.track.imageUrl);
    device->setStateValue(sonosGroupArtworkStateTypeId, metaDataStatus.container.imageUrl);
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
        ActionId nymeaActionId = m_pendingActions.value(sonosActionId);
        if (success) {
            emit actionExecutionFinished(nymeaActionId, Device::DeviceErrorNoError);
        } else {
            emit actionExecutionFinished(nymeaActionId, Device::DeviceErrorHardwareFailure);
        }
    }
}
