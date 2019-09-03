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
    hardwareManager()->pluginTimerManager()->unregisterTimer(m_pluginTimer);
}


Device::DeviceSetupStatus DevicePluginSonos::setupDevice(Device *device)
{
    if (!m_pluginTimer) {
        hardwareManager()->pluginTimerManager()->registerTimer(10);
        connect(m_pluginTimer, &PluginTimer::timeout, this, &DevicePluginSonos::onPluginTimer);
    }

    if(!m_tokenRefreshTimer) {
        m_tokenRefreshTimer = new QTimer(this);
        m_tokenRefreshTimer->setSingleShot(false);
        connect(m_tokenRefreshTimer, &QTimer::timeout, this, &DevicePluginSonos::onRefreshTimeout);
    }

    if (device->deviceClassId() == sonosConnectionDeviceClassId) {
        qCDebug(dcSonos()) << "Sonos OAuth setup complete";

        pluginStorage()->beginGroup(device->id().toString());
        QByteArray accessToken = pluginStorage()->value("access_token").toByteArray();
        pluginStorage()->endGroup();

        Sonos *sonos = new Sonos(hardwareManager()->networkManager(), accessToken, this);
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
        QString clientId = "b15cbf8c-a39c-47aa-bd93-635a96e9696c";
        QString clientSecret = "c086ba71-e562-430b-a52f-867c6482fd11";

        QUrl url("https://api.sonos.com/login/v3/oauth");
        QUrlQuery queryParams;
        queryParams.addQueryItem("client_id", clientId);
        queryParams.addQueryItem("redirect_uri", "https://127.0.0.1:8888");
        queryParams.addQueryItem("response_type", "code");
        queryParams.addQueryItem("scope", "playback-control-all");
        queryParams.addQueryItem("state", QUuid::createUuid().toString());
        url.setQuery(queryParams);

        qCDebug(dcSonos()) << "Sonos url:" << url;

        devicePairingInfo.setOAuthUrl(url);
        devicePairingInfo.setStatus(Device::DeviceErrorNoError);
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
        qCDebug(dcSonos()) << "Acess code is:" << query.queryItemValue("code");

        QString accessCode = query.queryItemValue("code");

        // Obtaining access token
        url = QUrl("https://api.sonos.com/login/v3/oauth/access");
        query.clear();
        query.addQueryItem("grant_type", "authorization_code");
        query.addQueryItem("code", accessCode);
        query.addQueryItem("redirect_uri", "https%3A%2F%2F127.0.0.1%3A8888");
        url.setQuery(query);

        QNetworkRequest request(url);
        request.setHeader(QNetworkRequest::ContentTypeHeader, "application/x-www-form-urlencoded;charset=utf-8");

        QByteArray clientId = "b15cbf8c-a39c-47aa-bd93-635a96e9696c";
        QByteArray clientSecret = "c086ba71-e562-430b-a52f-867c6482fd11";

        QByteArray auth = QByteArray(clientId + ':' + clientSecret).toBase64(QByteArray::Base64Encoding | QByteArray::KeepTrailingEquals);
        request.setRawHeader("Authorization", QString("Basic %1").arg(QString(auth)).toUtf8());

        QNetworkReply *reply = hardwareManager()->networkManager()->post(request, QByteArray());
        connect(reply, &QNetworkReply::finished, this, [this, reply, devicePairingInfo](){
            reply->deleteLater();
            DevicePairingInfo info(devicePairingInfo);

            QJsonDocument jsonDoc = QJsonDocument::fromJson(reply->readAll());
            qCDebug(dcSonos()) << "Sonos accessToken reply:" << this << reply->error() << reply->errorString() << jsonDoc.toJson();
            if(!jsonDoc.toVariant().toMap().contains("access_token") || !jsonDoc.toVariant().toMap().contains("refresh_token") ) {
                info.setStatus(Device::DeviceErrorSetupFailed);
                emit pairingFinished(info);
                return;
            }
            qCDebug(dcSonos()) << "Access token:" << jsonDoc.toVariant().toMap().value("access_token").toString();
            QByteArray accessToken = jsonDoc.toVariant().toMap().value("access_token").toByteArray();

            qCDebug(dcSonos()) << "Refresh token:" << jsonDoc.toVariant().toMap().value("refresh_token").toString();
            QByteArray refreshToken = jsonDoc.toVariant().toMap().value("refresh_token").toByteArray();

            pluginStorage()->beginGroup(info.deviceId().toString());
            pluginStorage()->setValue("access_token", accessToken);
            pluginStorage()->setValue("refresh_token", refreshToken);
            pluginStorage()->endGroup();

            if (jsonDoc.toVariant().toMap().contains("expires_in")) {
                int expireTime = jsonDoc.toVariant().toMap().value("expires_in").toInt();
                qCDebug(dcSonos()) << "expires at" << QDateTime::currentDateTime().addSecs(expireTime).toString();
                //m_tokenRefreshTimer->start((expireTime - 20) * 1000);
                //TODO
            }

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
        hardwareManager()->pluginTimerManager()->unregisterTimer(m_pluginTimer);
        m_pluginTimer = nullptr;
    }
}


Device::DeviceError DevicePluginSonos::executeAction(Device *device, const Action &action)
{
    if (device->deviceClassId() == sonosGroupDeviceClassId) {
        Sonos *sonos = m_sonosConnections.value(device);
        QByteArray groupId = device->paramValue(sonosGroupDeviceGroupIdParamTypeId).toByteArray();

        if (!sonos)
            return Device::DeviceErrorInvalidParameter;

        if (action.actionTypeId()  == sonosGroupPlayActionTypeId) {
            sonos->groupPlay(groupId);
            return Device::DeviceErrorAsync;
        }

        if (action.actionTypeId()  == sonosGroupShuffleActionTypeId) {
            bool shuffle = action.param(sonosGroupShuffleActionShuffleParamTypeId).value().toBool();
            sonos->groupSetShuffle(groupId, shuffle);
            return Device::DeviceErrorAsync;
        }

        if (action.actionTypeId()  == sonosGroupRepeatActionTypeId) {

            if (action.param(sonosGroupRepeatActionRepeatParamTypeId).value().toString() == "None") {
                sonos->groupSetRepeat(groupId, Sonos::RepeatModeNone);
            } else if (action.param(sonosGroupShuffleActionShuffleParamTypeId).value().toString() == "One") {
                sonos->groupSetRepeat(groupId, Sonos::RepeatModeOne);
            } else if (action.param(sonosGroupShuffleActionShuffleParamTypeId).value().toString() == "All") {
                sonos->groupSetRepeat(groupId, Sonos::RepeatModeAll);
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
                sonos->groupPlay(groupId);
            } else if(playbackStatus == "Stopped") {
                sonos->groupPause(groupId);
            } else if(playbackStatus == "Paused") {
                sonos->groupPause(groupId);
            }
            return Device::DeviceErrorAsync;
        }
        return Device::DeviceErrorActionTypeNotFound;
    }
    return Device::DeviceErrorDeviceClassNotFound;
}

void DevicePluginSonos::onPluginTimer()
{    
    foreach (Device *device, myDevices().filterByDeviceClassId(sonosConnectionDeviceClassId)) {

        //get groups for each household in order to add or remove groups
        Sonos *sonos = m_sonosConnections.value(device);
        if (!sonos)
            continue;
        sonos->getHouseholds();

        foreach (Device *groupDevice, myDevices().filterByParentDeviceId(device->id())) {
            if (device->deviceClassId() == sonosGroupDeviceClassId) {
                //get playback status of each group
                QByteArray groupId = groupDevice->paramValue(sonosGroupDeviceGroupIdParamTypeId).toByteArray();
                sonos->getGroupPlaybackStatus(groupId);
                sonos->getGroupMetadataStatus(groupId);
                sonos->getGroupVolume(groupId);
            }
        }
    }
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

void DevicePluginSonos::onRefreshTimeout()
{
    qCDebug(dcSonos) << "Refresh authentication token";

    QUrlQuery query;
    query.addQueryItem("grant_type", "refresh_token");
    query.addQueryItem("refresh_token", m_sonosConnectionRefreshToken);

    QUrl url("https://api.sonos.com/login/v3/oauth");
    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/x-www-form-urlencoded; charset=UTF-8");
    QNetworkReply *reply = hardwareManager()->networkManager()->post(request, QByteArray());
    connect(reply, &QNetworkReply::finished, this, [this, reply](){
        reply->deleteLater();

        QJsonDocument jsonDoc = QJsonDocument::fromJson(reply->readAll());
        qCDebug(dcSonos()) << "Sonos accessToken reply:" << this << reply->error() << reply->errorString() << jsonDoc.toJson();
        if(!jsonDoc.toVariant().toMap().contains("access_token")) {
            return;
        }
        qCDebug(dcSonos()) << "Access token:" << jsonDoc.toVariant().toMap().value("access_token").toString();
        m_sonosConnectionAccessToken = jsonDoc.toVariant().toMap().value("access_token").toByteArray();

        if (jsonDoc.toVariant().toMap().contains("expires_in")) {
            int expireTime = jsonDoc.toVariant().toMap().value("expires_in").toInt();
            qCDebug(dcSonos()) << "expires at" << QDateTime::currentDateTime().addSecs(expireTime).toString();
            m_tokenRefreshTimer->start((expireTime - 20) * 1000);
        }
    });
}

void DevicePluginSonos::onHouseholdIdsReceived(QList<QString> householdIds)
{
    Sonos *sonos = static_cast<Sonos *>(sender());
    foreach(QString householdId, householdIds) {
        sonos->getGroups(householdId);
    }
}

void DevicePluginSonos::onGroupsReceived(QList<Sonos::GroupObject> groupObjects)
{
    Sonos *sonos = static_cast<Sonos *>(sender());
    Device *parentDevice = m_sonosConnections.key(sonos);
    if (!parentDevice)
        return;

    QList<DeviceDescriptor> deviceDescriptors;
    foreach(Sonos::GroupObject groupObject, groupObjects) {
        if (!myDevices().filterByParam(sonosGroupDeviceGroupIdParamTypeId, groupObject.groupId).isEmpty()) {
            continue;
        }
        DeviceDescriptor deviceDescriptor(sonosGroupDeviceClassId, groupObject.displayName, "Sonos Group", parentDevice->id());
        ParamList params;
        params.append(Param(sonosGroupDeviceGroupIdParamTypeId, groupObject.groupId));
        deviceDescriptor.setParams(params);
        deviceDescriptors.append(deviceDescriptor);
    }

    if (!deviceDescriptors.isEmpty())
        emit autoDevicesAppeared(sonosGroupDeviceClassId, deviceDescriptors);
}

void DevicePluginSonos::onPlayBackStatusReceived(const QString &groupId, Sonos::PlayBackObject playBack)
{
    qDebug(dcSonos()) << "Playback status received" << playBack.playbackState;
    Device *device = myDevices().findByParams(ParamList() << Param(sonosGroupDeviceGroupIdParamTypeId, groupId));
    if (!device)
        return;

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

void DevicePluginSonos::onVolumeReceived(const QString &groupId, Sonos::GroupVolumeObject groupVolume)
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
