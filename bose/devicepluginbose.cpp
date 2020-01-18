/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*
* Copyright 2013 - 2020, nymea GmbH
* Contact: contact@nymea.io
*
* This file is part of nymea.
* This project including source code and documentation is protected by copyright law, and
* remains the property of nymea GmbH. All rights, including reproduction, publication,
* editing and translation, are reserved. The use of this project is subject to the terms of a
* license agreement to be concluded with nymea GmbH in accordance with the terms
* of use of nymea GmbH, available under https://nymea.io/license
*
* GNU Lesser General Public License Usage
* Alternatively, this project may be redistributed and/or modified under the terms of the GNU
* Lesser General Public License as published by the Free Software Foundation; version 3.
* this project is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;
* without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
* See the GNU Lesser General Public License for more details.
*
* You should have received a copy of the GNU Lesser General Public License along with this project.
* If not, see <https://www.gnu.org/licenses/>.
*
* For any further details and any questions please contact us under contact@nymea.io
* or see our FAQ/Licensing Information on https://nymea.io/license/faq
*
* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

#include "devicepluginbose.h"
#include "devices/device.h"
#include "plugininfo.h"
#include "platform/platformzeroconfcontroller.h"
#include "network/zeroconf/zeroconfserviceentry.h"
#include "types/mediabrowseritem.h"

#include <QNetworkRequest>
#include <QNetworkReply>

DevicePluginBose::DevicePluginBose()
{
}

DevicePluginBose::~DevicePluginBose()
{
}

void DevicePluginBose::init()
{
    m_serviceBrowser = hardwareManager()->zeroConfController()->createServiceBrowser("_soundtouch._tcp");
}

void DevicePluginBose::setupDevice(DeviceSetupInfo *info)
{

    if (info->device()->deviceClassId() == soundtouchDeviceClassId) {

        QString ipAddress;
        QString playerId = info->device()->paramValue(soundtouchDevicePlayerIdParamTypeId).toString();

        foreach (const ZeroConfServiceEntry avahiEntry, m_serviceBrowser->serviceEntries()) {
            QString discoveredPlayerId = avahiEntry.hostName().split(".").first();
            if (discoveredPlayerId == playerId) {
                ipAddress = avahiEntry.hostAddress().toString();
                break;
            }
        }

        if (ipAddress.isEmpty()) {
            // Ok, we could not find an ip on zeroconf... Let's try again in a second while setupInfo hasn't timed out.
            qCDebug(dcBose()) << "Device not found via ZeroConf... Waiting for a second for it to appear...";
            QTimer::singleShot(1000, info, [this, info](){
                setupDevice(info);
            });
            return;
        }

        info->device()->setParamValue(soundtouchDeviceIpParamTypeId,ipAddress);
        SoundTouch *soundTouch = new SoundTouch(hardwareManager()->networkManager(), ipAddress, this);
        connect(soundTouch, &SoundTouch::connectionChanged, this, &DevicePluginBose::onConnectionChanged);
        connect(soundTouch, &SoundTouch::infoReceived, this, &DevicePluginBose::onInfoObjectReceived);
        connect(soundTouch, &SoundTouch::nowPlayingReceived, this, &DevicePluginBose::onNowPlayingObjectReceived);
        connect(soundTouch, &SoundTouch::volumeReceived, this, &DevicePluginBose::onVolumeObjectReceived);
        connect(soundTouch, &SoundTouch::sourcesReceived, this, &DevicePluginBose::onSourcesObjectReceived);
        connect(soundTouch, &SoundTouch::bassReceived, this, &DevicePluginBose::onBassObjectReceived);
        connect(soundTouch, &SoundTouch::bassCapabilitiesReceived, this, &DevicePluginBose::onBassCapabilitiesObjectReceived);
        connect(soundTouch, &SoundTouch::zoneReceived, this, &DevicePluginBose::onZoneObjectReceived);
        connect(soundTouch, &SoundTouch::requestExecuted, this, &DevicePluginBose::onRequestExecuted);
        m_soundTouch.insert(info->device(), soundTouch);
        return info->finish(Device::DeviceErrorNoError);

    } else {
        qCWarning(dcBose()) << "DeviceClassId not found" << info->device()->deviceClassId();
        return info->finish(Device::DeviceErrorDeviceClassNotFound);
    }
}

void DevicePluginBose::postSetupDevice(Device *device)
{
    if (device->deviceClassId() == soundtouchDeviceClassId) {
        connect(device, &Device::nameChanged, this, &DevicePluginBose::onDeviceNameChanged);
        SoundTouch *soundTouch = m_soundTouch.value(device);
        soundTouch->getInfo();
        soundTouch->getNowPlaying();
        soundTouch->getVolume();
        soundTouch->getBass();
        soundTouch->getBassCapabilities();
        soundTouch->getZone();
    }

    if (!m_pluginTimer) {
        m_pluginTimer = hardwareManager()->pluginTimerManager()->registerTimer(2);
        connect(m_pluginTimer, &PluginTimer::timeout, this, &DevicePluginBose::onPluginTimer);
    }
}

void DevicePluginBose::deviceRemoved(Device *device)
{
    if (device->deviceClassId() == soundtouchDeviceClassId) {
        SoundTouch *soundTouch = m_soundTouch.take(device);
        soundTouch->deleteLater();
    }

    if (m_pluginTimer && myDevices().isEmpty()) {
        hardwareManager()->pluginTimerManager()->unregisterTimer(m_pluginTimer);
        m_pluginTimer = nullptr;
    }
}

void DevicePluginBose::discoverDevices(DeviceDiscoveryInfo *info)
{
    QTimer::singleShot(5000, info, [this, info](){
        foreach (const ZeroConfServiceEntry avahiEntry, m_serviceBrowser->serviceEntries()) {
            qCDebug(dcBose) << "Zeroconf entry:" << avahiEntry;

            QString playerId = avahiEntry.hostName().split(".").first();
            DeviceDescriptor descriptor(soundtouchDeviceClassId, avahiEntry.name(), avahiEntry.hostAddress().toString());
            ParamList params;

            foreach (Device *existingDevice, myDevices().filterByDeviceClassId(soundtouchDeviceClassId)) {
                if (existingDevice->paramValue(soundtouchDevicePlayerIdParamTypeId).toString() == playerId) {
                    descriptor.setDeviceId(existingDevice->id());
                    break;
                }
            }
            params << Param(soundtouchDeviceIpParamTypeId, avahiEntry.hostAddress().toString());
            params << Param(soundtouchDevicePlayerIdParamTypeId, playerId);
            descriptor.setParams(params);
            info->addDeviceDescriptor(descriptor);
        }
        info->finish(Device::DeviceErrorNoError);
    });
}

void DevicePluginBose::executeAction(DeviceActionInfo *info)
{
    Device *device = info->device();
    Action action = info->action();

    if (device->deviceClassId() == soundtouchDeviceClassId) {
        SoundTouch *soundTouch = m_soundTouch.value(device);

        if (action.actionTypeId() == soundtouchPowerActionTypeId) {
            //bool power = action.param(soundtouchPowerActionPowerParamTypeId).value().toBool();
            QUuid requestId = soundTouch->setKey(KEY_VALUE::KEY_VALUE_POWER); //only toggling possible
            m_pendingActions.insert(requestId, info);
            connect(info, &DeviceActionInfo::aborted, this, [requestId, this] {m_pendingActions.remove(requestId);});

        } else if (action.actionTypeId() == soundtouchMuteActionTypeId) {
            QUuid requestId = soundTouch->setKey(KEY_VALUE::KEY_VALUE_MUTE); //only toggling possible
            m_pendingActions.insert(requestId, info);
            connect(info, &DeviceActionInfo::aborted, this, [requestId, this] {m_pendingActions.remove(requestId);});

        } else if (action.actionTypeId() == soundtouchPlayActionTypeId) {
            QUuid requestId = soundTouch->setKey(KEY_VALUE::KEY_VALUE_PLAY);
            m_pendingActions.insert(requestId, info);
            connect(info, &DeviceActionInfo::aborted, this, [requestId, this] {m_pendingActions.remove(requestId);});

        } else if (action.actionTypeId() == soundtouchPauseActionTypeId) {
            QUuid requestId = soundTouch->setKey(KEY_VALUE::KEY_VALUE_PAUSE);
            m_pendingActions.insert(requestId, info);
            connect(info, &DeviceActionInfo::aborted, this, [requestId, this] {m_pendingActions.remove(requestId);});

        } else if (action.actionTypeId() == soundtouchStopActionTypeId) {
            QUuid requestId = soundTouch->setKey(KEY_VALUE::KEY_VALUE_STOP);
            m_pendingActions.insert(requestId, info);
            connect(info, &DeviceActionInfo::aborted, this, [requestId, this] {m_pendingActions.remove(requestId);});

        } else if (action.actionTypeId() == soundtouchSkipNextActionTypeId) {
            QUuid requestId = soundTouch->setKey(KEY_VALUE::KEY_VALUE_NEXT_TRACK);
            m_pendingActions.insert(requestId, info);
            connect(info, &DeviceActionInfo::aborted, this, [requestId, this] {m_pendingActions.remove(requestId);});

        } else if (action.actionTypeId() == soundtouchSkipBackActionTypeId) {
            QUuid requestId = soundTouch->setKey(KEY_VALUE::KEY_VALUE_PREV_TRACK);
            m_pendingActions.insert(requestId, info);
            connect(info, &DeviceActionInfo::aborted, this, [requestId, this] {m_pendingActions.remove(requestId);});

        } else if (action.actionTypeId() == soundtouchShuffleActionTypeId) {

            QUuid requestId;
            bool shuffle =  action.param(soundtouchShuffleActionShuffleParamTypeId).value().toBool();
            if (shuffle) {
                requestId = soundTouch->setKey(KEY_VALUE::KEY_VALUE_SHUFFLE_ON);
            } else {
                requestId = soundTouch->setKey(KEY_VALUE::KEY_VALUE_SHUFFLE_OFF);
            }
            m_pendingActions.insert(requestId, info);
            connect(info, &DeviceActionInfo::aborted, this, [requestId, this] {m_pendingActions.remove(requestId);});

        } else if (action.actionTypeId() == soundtouchRepeatActionTypeId) {

            QUuid requestId;
            QString repeat =  action.param(soundtouchRepeatActionRepeatParamTypeId).value().toString();
            if (repeat == "None") {
                requestId = soundTouch->setKey(KEY_VALUE::KEY_VALUE_REPEAT_OFF);
            } else if (repeat == "One") {
                requestId = soundTouch->setKey(KEY_VALUE::KEY_VALUE_REPEAT_ONE);
            } else if (repeat == "All") {
                requestId = soundTouch->setKey(KEY_VALUE::KEY_VALUE_REPEAT_ALL);
            }
            m_pendingActions.insert(requestId, info);
            connect(info, &DeviceActionInfo::aborted, this, [requestId, this] {m_pendingActions.remove(requestId);});

        } else if (action.actionTypeId() == soundtouchVolumeActionTypeId) {

            int volume = action.param(soundtouchVolumeActionVolumeParamTypeId).value().toInt();
            QUuid requestId = soundTouch->setVolume(volume);
            m_pendingActions.insert(requestId, info);
            connect(info, &DeviceActionInfo::aborted, this, [requestId, this] {m_pendingActions.remove(requestId);});

        } else if (action.actionTypeId() == soundtouchBassActionTypeId) {

            int bass = action.param(soundtouchBassActionBassParamTypeId).value().toInt();
            QUuid requestId = soundTouch->setBass(bass);
            m_pendingActions.insert(requestId, info);
            connect(info, &DeviceActionInfo::aborted, this, [requestId, this] {m_pendingActions.remove(requestId);});

        } else if (action.actionTypeId() == soundtouchPlaybackStatusActionTypeId) {

            QUuid requestId;
            QString status =  action.param(soundtouchPlaybackStatusActionPlaybackStatusParamTypeId).value().toString();
            if (status == "Playing") {
                soundTouch->setKey(KEY_VALUE::KEY_VALUE_PLAY);
            } else if (status == "Paused") {
                soundTouch->setKey(KEY_VALUE::KEY_VALUE_PAUSE);
            } else if (status == "Stopped") {
                soundTouch->setKey(KEY_VALUE::KEY_VALUE_STOP);
            }
            m_pendingActions.insert(requestId, info);
            connect(info, &DeviceActionInfo::aborted, this, [requestId, this] {m_pendingActions.remove(requestId);});

        } else {
            qCWarning(dcBose()) << "ActionTypeId not found" << action.actionTypeId();
            return info->finish(Device::DeviceErrorActionTypeNotFound);
        }
    } else {
        qCWarning(dcBose()) << "DeviceClassId not found" << device->deviceClassId();
        return info->finish(Device::DeviceErrorDeviceClassNotFound);
    }
}

void DevicePluginBose::browseDevice(BrowseResult *result)
{
    Device *device = result->device();
    if (device->deviceClassId() == soundtouchDeviceClassId) {
        SoundTouch *soundTouch = m_soundTouch.value(device);
        QUuid requestId = soundTouch->getSources();
        m_asyncBrowseResults.insert(requestId, result);
        connect(result, &BrowseResult::aborted, this, [this, requestId]{m_asyncBrowseResults.remove(requestId);});
    }
}

void DevicePluginBose::browserItem(BrowserItemResult *result)
{
    Device *device = result->device();
    if (device->deviceClassId() == soundtouchDeviceClassId) {
        SoundTouch *soundTouch = m_soundTouch.value(device);
        QUuid requestId = soundTouch->getSources();
        m_asyncBrowseItemResults.insert(requestId, result);
        connect(result, &BrowserItemResult::aborted, this, [this, requestId]{m_asyncBrowseItemResults.remove(requestId);});
    }
}

void DevicePluginBose::executeBrowserItem(BrowserActionInfo *info)
{
    Device *device = info->device();
    if (device->deviceClassId() == soundtouchDeviceClassId) {
        SoundTouch *soundTouch = m_soundTouch.value(device);
        SourcesObject sources = m_sourcesObjects.value(device);
        foreach (SourceItemObject source, sources.sourceItems) {
            if (source.source == info->browserAction().itemId()) {
                ContentItemObject contentItem;
                contentItem.source = source.source;
                contentItem.sourceAccount = source.sourceAccount;
                QUuid requestId = soundTouch->setSource(contentItem);
                m_asyncExecuteBrowseItems.insert(requestId, info);
                connect(info, &BrowserActionInfo::aborted, this, [this, requestId]{m_asyncExecuteBrowseItems.remove(requestId);});
                break;
            }
        }
    }
}

void DevicePluginBose::onPluginTimer()
{
    foreach(SoundTouch *soundTouch, m_soundTouch.values()) {
        soundTouch->getInfo();
        soundTouch->getNowPlaying();
        soundTouch->getVolume();
        soundTouch->getBass();
        soundTouch->getZone();
    }
}

void DevicePluginBose::onConnectionChanged(bool connected)
{
    SoundTouch *soundtouch = static_cast<SoundTouch *>(sender());
    Device *device = m_soundTouch.key(soundtouch);
    device->setStateValue(soundtouchConnectedStateTypeId, connected);
}

void DevicePluginBose::onDeviceNameChanged()
{
    Device *device = static_cast<Device*>(sender());
    SoundTouch *soundtouch = m_soundTouch.value(device);
    soundtouch->setName(device->name());
}

void DevicePluginBose::onRequestExecuted(QUuid requestId, bool success)
{
    if (m_pendingActions.contains(requestId)) {
        DeviceActionInfo *info = m_pendingActions.value(requestId);
        if (success) {
            info->finish(Device::DeviceErrorNoError);
        } else {
            info->finish(Device::DeviceErrorHardwareFailure);
        }
    } else if (m_asyncBrowseResults.contains(requestId)) {
        if (!success) {
            BrowseResult *result = m_asyncBrowseResults.take(requestId);
            result->finish(Device::DeviceErrorHardwareFailure);
        }
    } else if (m_asyncExecuteBrowseItems.contains(requestId)) {
        BrowserActionInfo *info = m_asyncExecuteBrowseItems.take(requestId);
        if (success) {
            info->finish(Device::DeviceErrorNoError);
        } else {
            info->finish(Device::DeviceErrorHardwareFailure);
        }
    } else if (m_asyncBrowseItemResults.contains(requestId)) {
        if (!success) {
            BrowserItemResult *result = m_asyncBrowseItemResults.take(requestId);
            result->finish(Device::DeviceErrorHardwareFailure);
        }
    } else {
        //This request was not an action or browse request
    }
}

void DevicePluginBose::onInfoObjectReceived(QUuid requestId, InfoObject infoObject)
{
    Q_UNUSED(requestId);
    SoundTouch *soundtouch = static_cast<SoundTouch *>(sender());
    Device *device = m_soundTouch.key(soundtouch);
    device->setName(infoObject.name);
}

void DevicePluginBose::onNowPlayingObjectReceived(QUuid requestId, NowPlayingObject nowPlaying)
{
    Q_UNUSED(requestId);
    SoundTouch *soundtouch = static_cast<SoundTouch *>(sender());
    Device *device = m_soundTouch.key(soundtouch);

    device->setStateValue(soundtouchPowerStateTypeId, !(nowPlaying.source.toUpper() == "STANDBY"));
    device->setStateValue(soundtouchSourceStateTypeId, nowPlaying.source);
    device->setStateValue(soundtouchTitleStateTypeId, nowPlaying.track);
    device->setStateValue(soundtouchArtistStateTypeId, nowPlaying.artist);
    device->setStateValue(soundtouchCollectionStateTypeId, nowPlaying.album);
    device->setStateValue(soundtouchArtworkStateTypeId, nowPlaying.art.url);
    device->setStateValue(soundtouchShuffleStateTypeId, ( nowPlaying.shuffleSetting == SHUFFLE_STATUS_SHUFFLE_ON ));

    switch (nowPlaying.repeatSettings)  {
    case REPEAT_STATUS_REPEAT_ONE:
        device->setStateValue(soundtouchRepeatStateTypeId, "One");
        break;
    case REPEAT_STATUS_REPEAT_ALL:
        device->setStateValue(soundtouchRepeatStateTypeId, "All");
        break;
    case REPEAT_STATUS_REPEAT_OFF:
        device->setStateValue(soundtouchRepeatStateTypeId, "None");
        break;
    }

    switch (nowPlaying.playStatus) {
    case PLAY_STATUS_PLAY_STATE:
        device->setStateValue(soundtouchPlaybackStatusStateTypeId, "Playing");
        break;
    case PLAY_STATUS_PAUSE_STATE:
    case PLAY_STATUS_BUFFERING_STATE:
        device->setStateValue(soundtouchPlaybackStatusStateTypeId, "Paused");
        break;
    case PLAY_STATUS_STOP_STATE:
        device->setStateValue(soundtouchPlaybackStatusStateTypeId, "Stopped");
        break;
    }
}

void DevicePluginBose::onVolumeObjectReceived(QUuid requestId, VolumeObject volume)
{
    Q_UNUSED(requestId);
    SoundTouch *soundtouch = static_cast<SoundTouch *>(sender());
    Device *device = m_soundTouch.key(soundtouch);
    device->setStateValue(soundtouchVolumeStateTypeId, volume.actualVolume);
    device->setStateValue(soundtouchMuteStateTypeId, volume.muteEnabled);
}

void DevicePluginBose::onSourcesObjectReceived(QUuid requestId, SourcesObject sources)
{
    SoundTouch *soundtouch = static_cast<SoundTouch *>(sender());
    Device *device = m_soundTouch.key(soundtouch);
    m_sourcesObjects.insert(device, sources);

    if (m_asyncBrowseResults.contains(requestId)) {
        BrowseResult *result = m_asyncBrowseResults.value(requestId);
        foreach (SourceItemObject sourceItem, sources.sourceItems) {
            qDebug(dcBose()) << "Source:" << sourceItem.source;
            if (sourceItem.source == "BLUETOOTH") {
                MediaBrowserItem item(sourceItem.source, sourceItem.source, false, true);
                item.setDescription(sourceItem.sourceAccount);
                item.setIcon(BrowserItem::BrowserIcon::BrowserIconMusic);
                item.setMediaIcon(MediaBrowserItem::MediaBrowserIcon::MediaBrowserIconBluetooth);
                result->addItem(item);
            } else if (sourceItem.source == "AUX") {
                MediaBrowserItem item(sourceItem.source, sourceItem.source, false, true);
                item.setDescription(sourceItem.sourceAccount);
                item.setIcon(BrowserItem::BrowserIcon::BrowserIconMusic);
                result->addItem(item);
                item.setMediaIcon(MediaBrowserItem::MediaBrowserIcon::MediaBrowserIconAux);
            } else if ((sourceItem.source == "SPOTIFY") && (sourceItem.status == SOURCE_STATUS_READY)) {
                MediaBrowserItem item(sourceItem.source, sourceItem.source, false, true);
                item.setDescription(sourceItem.sourceAccount);
                item.setIcon(BrowserItem::BrowserIcon::BrowserIconMusic);
                item.setMediaIcon(MediaBrowserItem::MediaBrowserIcon::MediaBrowserIconSpotify);
                result->addItem(item);
            }
        }
        return result->finish(Device::DeviceErrorNoError);
    } else if (m_asyncBrowseItemResults.contains(requestId)) {
        BrowserItemResult *result = m_asyncBrowseItemResults.value(requestId);
        foreach (SourceItemObject sourceItem, sources.sourceItems) {
            if (sourceItem.source == result->itemId()) {
                return result->finish(Device::DeviceErrorNoError);
            }
        }
        return result->finish(Device::DeviceErrorItemNotFound);
    } else {
        qCWarning(dcBose()) << "Received sources without an associated BrowseResult";
    }
}

void DevicePluginBose::onBassObjectReceived(QUuid requestId, BassObject bass)
{
    Q_UNUSED(requestId);
    SoundTouch *soundtouch = static_cast<SoundTouch *>(sender());
    Device *device = m_soundTouch.key(soundtouch);
    device->setStateValue(soundtouchBassStateTypeId, bass.actualBass);
}

void DevicePluginBose::onBassCapabilitiesObjectReceived(QUuid requestId, BassCapabilitiesObject bassCapabilities)
{
    Q_UNUSED(requestId);
    qDebug(dcBose()) << "Bass capabilities (max, min, default):" << bassCapabilities.bassMax << bassCapabilities.bassMin << bassCapabilities.bassDefault;
}

void DevicePluginBose::onGroupObjectReceived(QUuid requestId, GroupObject group)
{
    Q_UNUSED(requestId);
    qDebug(dcBose())  << "Group" << group.name << group.status;
    foreach (RolesObject role, group.roles) {
        qDebug(dcBose()) << "-> member:" << role.groupRole.deviceID;
    }
}

void DevicePluginBose::onZoneObjectReceived(QUuid requestId, ZoneObject zone)
{
    Q_UNUSED(requestId);
    foreach (MemberObject member, zone.members) {
        qDebug(dcBose()) << "-> member:" << member.deviceID;
    }
}
