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
#include "plugininfo.h"

#include <QNetworkRequest>
#include <QNetworkReply>

DevicePluginSonos::DevicePluginSonos()
{

}


DevicePluginSonos::~DevicePluginSonos()
{
    hardwareManager()->pluginTimerManager()->unregisterTimer(m_pluginTimer);
}


Device::DeviceSetupStatus DevicePluginSonos::setupDevice(Device *device)
{
    if (!sonos) {

    }

    if (device->deviceClassId() == sonosDeviceClassId) {
        if (!m_sonosSystem->Discover()) {
            return Device::DeviceSetupStatusFailure;
        }
        QString zoneName = device->paramValue(sonosDeviceZoneNameParamTypeId).toString();

        SONOS::ZoneList zones = m_sonosSystem->GetZoneList();

        for(SONOS::ZoneList::const_iterator iz = zones.begin(); iz != zones.end(); ++iz) {

            if (iz->second->GetZoneName().c_str() == zoneName) {
                if (!m_sonosSystem->ConnectZone(iz->second, nullptr, nullptr)) {
                    qCDebug(dcSonos()) << "Failed connecting to zone" << zoneName;
                    return Device::DeviceSetupStatusFailure;
                }
            }
        }
        device->setStateValue(sonosConnectedStateTypeId, true);
    }
    return Device::DeviceSetupStatusSuccess;
}

void DevicePluginSonos::postSetupDevice(Device *device)
{
    if (device->deviceClassId() == sonosDeviceClassId) {
        SONOS::ZonePtr pl = m_sonosSystem->GetConnectedZone();
        uint8_t volume;
        uint8_t mute;
        for (SONOS::Zone::iterator ip = pl->begin(); ip != pl->end(); ++ip) {
            if (!m_sonosSystem->GetPlayer()->GetVolume((*ip)->GetUUID(), &volume)) {
                qWarning(dcSonos()) << "Could not get volume for" << (*ip)->GetHost().c_str();
            } else {
                device->setStateValue(sonosVolumeStateTypeId, volume);
            }

            if (!m_sonosSystem->GetPlayer()->GetMute((*ip)->GetUUID(), &mute)) {
                qWarning(dcSonos()) << "Could not get mute state for" << (*ip)->GetHost().c_str();
            } else {
                device->setStateValue(sonosMuteStateTypeId, mute);
            }
        }

        while(m_sonosSystem->GetPlayer()->TransportPropertyEmpty());

        SONOS::AVTProperty properties = m_sonosSystem->GetPlayer()->GetTransportProperty();

        qDebug(dcSonos()) << "Transport Status" << properties.TransportStatus.c_str();
        qDebug(dcSonos()) << "Transport State" << properties.TransportState.c_str();
        if (QString(properties.TransportState.c_str()) == "PLAYING") {
            device->setStateValue(sonosPlaybackStatusStateTypeId, "Playing");
        } else if (QString(properties.TransportState.c_str()) == "PAUSED") {
            device->setStateValue(sonosPlaybackStatusStateTypeId, "Paused");
        } else if (QString(properties.TransportState.c_str()) == "STOPPED") {
            device->setStateValue(sonosPlaybackStatusStateTypeId, "Stopped");
        }

        qDebug(dcSonos()) << "AVTransport URI" << properties.AVTransportURI.c_str();
        qDebug(dcSonos()) << "AVTransportTitle" << properties.AVTransportURIMetaData->GetValue("dc:title").c_str();
        qDebug(dcSonos()) << "Current Track" << properties.CurrentTrack;
        qDebug(dcSonos()) << "Current Track Duration" <<  properties.CurrentTrackDuration.c_str();
        qDebug(dcSonos()) << "Current Track URI" << properties.CurrentTrackURI.c_str();
        //device->setStateValue(sonosArtworkStateTypeId, properties.CurrentTrackURI.c_str());
        qDebug(dcSonos()) << "Current Track Title" << properties.CurrentTrackMetaData->GetValue("dc:title").c_str();
        device->setStateValue(sonosTitleStateTypeId, properties.CurrentTrackMetaData->GetValue("dc:title").c_str());
        qDebug(dcSonos()) << "Current Track Album" << properties.CurrentTrackMetaData->GetValue("upnp:album").c_str();
        qDebug(dcSonos()) << "Current Track Artist" <<  properties.CurrentTrackMetaData->GetValue("dc:creator").c_str();
        device->setStateValue(sonosArtistStateTypeId, properties.CurrentTrackMetaData->GetValue("dc:creator").c_str());
        qDebug(dcSonos()) << "Current Crossfade Mode" << properties.CurrentCrossfadeMode.c_str();
        qDebug(dcSonos()) << "Current Play Mode" << properties.CurrentPlayMode.c_str();
        qDebug(dcSonos()) << "Current TransportActions" << properties.CurrentTransportActions.c_str();
        qDebug(dcSonos()) << "Number Of Tracks" << properties.NumberOfTracks;
        qDebug(dcSonos()) << "Alarm Running" << properties.r_AlarmRunning.c_str();
        qDebug(dcSonos()) << "Alarm ID Running" << properties.r_AlarmIDRunning.c_str();
        qDebug(dcSonos()) << "Alarm Logged Start Time" << properties.r_AlarmLoggedStartTime.c_str();
        qDebug(dcSonos()) << "AlarmState" << properties.r_AlarmState.c_str();
    }
}

void DevicePluginSonos::deviceRemoved(Device *device)
{
    qCDebug(dcSonos) << "Delete " << device->name();
    if (myDevices().empty()) {
        delete m_sonosSystem;
    }
}

Device::DeviceError DevicePluginSonos::discoverDevices(const DeviceClassId &deviceClassId, const ParamList &params)
{
    Q_UNUSED(params)
    Q_UNUSED(deviceClassId)


    if (!m_sonosSystem->Discover()) {
        return  Device::DeviceErrorDeviceNotFound;
    }

    QList<DeviceDescriptor> descriptors;
    SONOS::ZoneList zones = m_sonosSystem->GetZoneList();

    for (SONOS::ZoneList::const_iterator it = zones.begin(); it != zones.end(); ++it) {
        qDebug(dcSonos()) << "Found zone" << it->second->GetZoneName().c_str();
        DeviceDescriptor descriptor(sonosDeviceClassId, it->second->GetZoneName().c_str());
        ParamList params;
        params << Param(sonosDeviceZoneNameParamTypeId, it->second->GetZoneName().c_str());
        descriptor.setParams(params);
        descriptors << descriptor;
    }
    emit devicesDiscovered(sonosDeviceClassId, descriptors);
    return Device::DeviceErrorAsync;
}

Device::DeviceError DevicePluginSonos::executeAction(Device *device, const Action &action)
{
    Q_UNUSED(action)
    if (device->deviceClassId() == sonosDeviceClassId) {

        if (action.actionTypeId()  == sonosPlayActionTypeId) {
            if (!m_sonosSystem->GetPlayer()->Play()) {
                return Device::DeviceErrorHardwareFailure;
            }
            return Device::DeviceErrorNoError;
        }

        if (action.actionTypeId()  == sonosShuffleActionTypeId) {
            SONOS::PlayMode_t mode = SONOS::PlayMode_t::PlayMode_NORMAL;;
            if (action.param(sonosShuffleActionShuffleParamTypeId).value().toBool()) {
                mode =  SONOS::PlayMode_t::PlayMode_NORMAL;
            }
            if (!m_sonosSystem->GetPlayer()->SetPlayMode(mode)) {
                return Device::DeviceErrorHardwareFailure;
            }
            return Device::DeviceErrorNoError;
        }

        if (action.actionTypeId()  == sonosRepeatActionTypeId) {
            SONOS::PlayMode_t mode;
            if (action.param(sonosRepeatActionRepeatParamTypeId).value().toString() == "None") {
                mode =  SONOS::PlayMode_t::PlayMode_NORMAL;
            } else if (action.param(sonosShuffleActionShuffleParamTypeId).value().toString() == "One") {
                mode =  SONOS::PlayMode_t::PlayMode_REPEAT_ONE;
            } else if (action.param(sonosShuffleActionShuffleParamTypeId).value().toString() == "All") {
                mode =  SONOS::PlayMode_t::PlayMode_REPEAT_ALL;
            } else {
                return Device::DeviceErrorHardwareFailure;
            }

            if (!m_sonosSystem->GetPlayer()->SetPlayMode(mode)) {
                return Device::DeviceErrorHardwareFailure;
            }
            return Device::DeviceErrorNoError;
        }

        if (action.actionTypeId() == sonosPauseActionTypeId) {
            if (!m_sonosSystem->GetPlayer()->Pause()) {
                return Device::DeviceErrorHardwareFailure;
            }
            return Device::DeviceErrorNoError;
        }

        if (action.actionTypeId() == sonosStopActionTypeId) {
            if (!m_sonosSystem->GetPlayer()->Stop()) {
                return Device::DeviceErrorHardwareFailure;
            }
            return Device::DeviceErrorNoError;
        }

        if (action.actionTypeId() == sonosMuteActionTypeId) {
            bool mute = action.param(sonosMuteActionMuteParamTypeId).value().toBool();

            SONOS::ZonePtr pl = m_sonosSystem->GetConnectedZone();
            for (SONOS::Zone::iterator ip = pl->begin(); ip != pl->end(); ++ip) {
                if (!m_sonosSystem->GetPlayer()->SetMute((*ip)->GetUUID(), mute)) {
                    qWarning(dcSonos()) << "Could not set mute state for" << (*ip)->GetHost().c_str();
                    return Device::DeviceErrorHardwareFailure;
                }
            }
            return Device::DeviceErrorNoError;
        }

        if (action.actionTypeId() == sonosSkipNextActionTypeId) {
            if (!m_sonosSystem->GetPlayer()->Next()) {
                return Device::DeviceErrorHardwareFailure;
            }
            return Device::DeviceErrorNoError;
        }

        if (action.actionTypeId() == sonosSkipBackActionTypeId) {
            if(!m_sonosSystem->GetPlayer()->Previous()) {
                return Device::DeviceErrorHardwareFailure;
            }
            return Device::DeviceErrorNoError;
        }

        if (action.actionTypeId() == sonosSkipBackActionTypeId) {
            int volume = action.param(sonosVolumeActionVolumeParamTypeId).value().toInt();

            SONOS::ZonePtr pl = m_sonosSystem->GetConnectedZone();
            for (SONOS::Zone::iterator ip = pl->begin(); ip != pl->end(); ++ip) {
                if (!m_sonosSystem->GetPlayer()->SetVolume((*ip)->GetUUID(), volume)) {
                    qWarning(dcSonos()) << "Could not set volume for" << (*ip)->GetHost().c_str();
                    return Device::DeviceErrorHardwareFailure;
                }
            }
            return Device::DeviceErrorNoError;
        }
        return Device::DeviceErrorActionTypeNotFound;
    }
    return Device::DeviceErrorDeviceClassNotFound;
}

void DevicePluginSonos::onPluginTimer()
{
}

void DevicePluginSonos::handleEventCB(void* handle)
{
    Q_UNUSED(handle);
    /*unsigned char mask = m_sonosSystem->LastEvents();
    if ((mask & SONOS::SVCEvent_TransportChanged))
        qDebug(dcSonos()) << "Event Transport changed";
    if ((mask & SONOS::SVCEvent_AlarmClockChanged))
        qDebug(dcSonos()) << "Alarm clock changed";
    if ((mask & SONOS::SVCEvent_ZGTopologyChanged))
        qDebug(dcSonos()) << "ZG Topology changed";
    if ((mask & SONOS::SVCEvent_ContentDirectoryChanged))
        qDebug(dcSonos()) << "Content directory changed";
    if ((mask & SONOS::SVCEvent_RenderingControlChanged))
        qDebug(dcSonos()) << "Rendering control changed";*/
}
