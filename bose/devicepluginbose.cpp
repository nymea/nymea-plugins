/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *                                                                         *
 *  Copyright (C) 2019 Bernhard Trinnes <bernhard.trinnes@nymea.io
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

#include "devicepluginbose.h"
#include "devices/device.h"
#include "plugininfo.h"
#include "platform/platformzeroconfcontroller.h"
#include "network/zeroconf/zeroconfservicebrowser.h"
#include "network/zeroconf/zeroconfserviceentry.h"

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
}

Device::DeviceSetupStatus DevicePluginBose::setupDevice(Device *device)
{
    if (!m_pluginTimer) {
        m_pluginTimer = hardwareManager()->pluginTimerManager()->registerTimer(2);
        connect(m_pluginTimer, &PluginTimer::timeout, this, &DevicePluginBose::onPluginTimer);
    }

    if (device->deviceClassId() == soundtouchDeviceClassId) {

        connect(device, &Device::nameChanged, this, &DevicePluginBose::onDeviceNameChanged);

        QString ipAddress = device->paramValue(soundtouchDeviceIpParamTypeId).toString();
        SoundTouch *soundTouch = new SoundTouch(hardwareManager()->networkManager(), ipAddress, this);
        connect(soundTouch, &SoundTouch::connectionChanged, this, &DevicePluginBose::onConnectionChanged);

        connect(soundTouch, &SoundTouch::infoReceived, this, &DevicePluginBose::onInfoObjectReceived);
        connect(soundTouch, &SoundTouch::nowPlayingReceived, this, &DevicePluginBose::onNowPlayingObjectReceived);
        connect(soundTouch, &SoundTouch::volumeReceived, this, &DevicePluginBose::onVolumeObjectReceived);
        connect(soundTouch, &SoundTouch::sourcesReceived, this, &DevicePluginBose::onSourcesObjectReceived);
        connect(soundTouch, &SoundTouch::bassReceived, this, &DevicePluginBose::onBassObjectReceived);
        connect(soundTouch, &SoundTouch::bassCapabilitiesReceived, this, &DevicePluginBose::onBassCapabilitiesObjectReceived);
        connect(soundTouch, &SoundTouch::zoneReceived, this, &DevicePluginBose::onZoneObjectReceived);

        soundTouch->getInfo();
        soundTouch->getNowPlaying();
        soundTouch->getVolume();
        soundTouch->getSources();
        soundTouch->getBass();
        soundTouch->getBassCapabilities();
        soundTouch->getZone();

        m_soundTouch.insert(device, soundTouch);

        return Device::DeviceSetupStatusSuccess;
    }
    return Device::DeviceSetupStatusFailure;
}

void DevicePluginBose::deviceRemoved(Device *device)
{
    if (device->deviceClassId() == soundtouchDeviceClassId) {
        SoundTouch *soundTouch = m_soundTouch.take(device);
        soundTouch->deleteLater();
    }

    if (m_pluginTimer && myDevices().isEmpty()) {
        hardwareManager()->pluginTimerManager()->unregisterTimer(m_pluginTimer);
    }
}

Device::DeviceError DevicePluginBose::discoverDevices(const DeviceClassId &deviceClassId, const ParamList &params)
{
    Q_UNUSED(params)
    Q_UNUSED(deviceClassId)

    ZeroConfServiceBrowser *serviceBrowser = hardwareManager()->zeroConfController()->createServiceBrowser("_soundtouch._tcp");
    QTimer::singleShot(5000, this, [this, serviceBrowser](){
        QList<DeviceDescriptor> descriptors;
        foreach (const ZeroConfServiceEntry avahiEntry, serviceBrowser->serviceEntries()) {
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
            descriptors << descriptor;
        }
        emit devicesDiscovered(soundtouchDeviceClassId, descriptors);
        serviceBrowser->deleteLater();
    });

    return Device::DeviceErrorAsync;
}

Device::DeviceError DevicePluginBose::executeAction(Device *device, const Action &action)
{
    if (device->deviceClassId() == soundtouchDeviceClassId) {
        SoundTouch *soundTouch = m_soundTouch.value(device);

        if (action.actionTypeId() == soundtouchPowerActionTypeId) {
            //bool power = action.param(soundtouchPowerActionPowerParamTypeId).value().toBool();
            soundTouch->setKey(KEY_VALUE::KEY_VALUE_POWER); //only toggling possible
            return Device::DeviceErrorNoError;
        }
        if (action.actionTypeId() == soundtouchMuteActionTypeId) {
            soundTouch->setKey(KEY_VALUE::KEY_VALUE_MUTE);
            return Device::DeviceErrorNoError;
        }
        if (action.actionTypeId() == soundtouchPlayActionTypeId) {
            soundTouch->setKey(KEY_VALUE::KEY_VALUE_PLAY);
            return Device::DeviceErrorNoError;
        }
        if (action.actionTypeId() == soundtouchPauseActionTypeId) {
            soundTouch->setKey(KEY_VALUE::KEY_VALUE_PAUSE);
            return Device::DeviceErrorNoError;
        }
        if (action.actionTypeId() == soundtouchStopActionTypeId) {
            soundTouch->setKey(KEY_VALUE::KEY_VALUE_STOP);
            return Device::DeviceErrorNoError;
        }
        if (action.actionTypeId() == soundtouchSkipNextActionTypeId) {
            soundTouch->setKey(KEY_VALUE::KEY_VALUE_NEXT_TRACK);
            return Device::DeviceErrorNoError;
        }
        if (action.actionTypeId() == soundtouchSkipBackActionTypeId) {
            soundTouch->setKey(KEY_VALUE::KEY_VALUE_PREV_TRACK);
            return Device::DeviceErrorNoError;
        }

        if (action.actionTypeId() == soundtouchShuffleActionTypeId) {

            bool shuffle =  action.param(soundtouchShuffleActionShuffleParamTypeId).value().toBool();
            if (shuffle) {
                soundTouch->setKey(KEY_VALUE::KEY_VALUE_SHUFFLE_ON);
            } else {
                soundTouch->setKey(KEY_VALUE::KEY_VALUE_SHUFFLE_OFF);
            }
            return Device::DeviceErrorNoError;
        }

        if (action.actionTypeId() == soundtouchRepeatActionTypeId) {

            QString repeat =  action.param(soundtouchRepeatActionRepeatParamTypeId).value().toString();
            if (repeat == "None") {
                soundTouch->setKey(KEY_VALUE::KEY_VALUE_REPEAT_OFF);
            } else if (repeat == "One") {
                soundTouch->setKey(KEY_VALUE::KEY_VALUE_REPEAT_ONE);
            } else if (repeat == "All") {
                soundTouch->setKey(KEY_VALUE::KEY_VALUE_REPEAT_ALL);
            }
            return Device::DeviceErrorNoError;
        }

        if (action.actionTypeId() == soundtouchVolumeActionTypeId) {
            int volume = action.param(soundtouchVolumeActionVolumeParamTypeId).value().toInt();
            soundTouch->setVolume(volume);
            return Device::DeviceErrorNoError;
        }

        if (action.actionTypeId() == soundtouchBassActionTypeId) {
            int bass = action.param(soundtouchBassActionBassParamTypeId).value().toInt();
            soundTouch->setBass(bass);
            return Device::DeviceErrorNoError;
        }

        if (action.actionTypeId() == soundtouchPlaybackStatusActionTypeId) {
            QString status =  action.param(soundtouchPlaybackStatusActionPlaybackStatusParamTypeId).value().toString();
            if (status == "Playing") {
                soundTouch->setKey(KEY_VALUE::KEY_VALUE_PLAY);
            } else if (status == "Paused") {
                soundTouch->setKey(KEY_VALUE::KEY_VALUE_PAUSE);
            } else if (status == "Stopped") {
                soundTouch->setKey(KEY_VALUE::KEY_VALUE_STOP);
            }
            return Device::DeviceErrorNoError;
        }

        return Device::DeviceErrorActionTypeNotFound;
    }
    return Device::DeviceErrorDeviceClassNotFound;
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

void DevicePluginBose::onInfoObjectReceived(InfoObject infoObject)
{
    SoundTouch *soundtouch = static_cast<SoundTouch *>(sender());
    Device *device = m_soundTouch.key(soundtouch);
    device->setName(infoObject.name);
}

void DevicePluginBose::onNowPlayingObjectReceived(NowPlayingObject nowPlaying)
{
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

void DevicePluginBose::onVolumeObjectReceived(VolumeObject volume)
{
    SoundTouch *soundtouch = static_cast<SoundTouch *>(sender());
    Device *device = m_soundTouch.key(soundtouch);
    device->setStateValue(soundtouchVolumeStateTypeId, volume.actualVolume);
    device->setStateValue(soundtouchMuteStateTypeId, volume.muteEnabled);
}

void DevicePluginBose::onSourcesObjectReceived(SourcesObject sources)
{
    foreach (SourceItemObject sourceItem, sources.sourceItems) {
        qDebug(dcBose()) << "Source:" << sources.deviceId << sourceItem.source << sourceItem.displayName;
    }
}

void DevicePluginBose::onBassObjectReceived(BassObject bass)
{
    SoundTouch *soundtouch = static_cast<SoundTouch *>(sender());
    Device *device = m_soundTouch.key(soundtouch);
    device->setStateValue(soundtouchBassStateTypeId, bass.actualBass);
}

void DevicePluginBose::onBassCapabilitiesObjectReceived(BassCapabilitiesObject bassCapabilities)
{
    qDebug(dcBose()) << "Bass capabilities (max, min, default):" << bassCapabilities.bassMax << bassCapabilities.bassMin << bassCapabilities.bassDefault;
}

void DevicePluginBose::onGroupObjectReceived(GroupObject group)
{
    qDebug(dcBose())  << "Group" << group.name << group.status;
    foreach (RolesObject role, group.roles) {
        qDebug(dcBose()) << "-> member:" << role.groupRole.deviceID;
    }
}

void DevicePluginBose::onZoneObjectReceived(ZoneObject zone)
{
    qDebug(dcBose())  << "Zone master" << zone.deviceID;
    foreach (MemberObject member, zone.members) {
        qDebug(dcBose()) << "-> member:" << member.deviceID;
    }
}
