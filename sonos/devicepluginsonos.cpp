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
    if (!m_pluginTimer) {

    }
    if (device->deviceClassId() == sonosConnectionDeviceClassId) {

        Sonos *sonos = new Sonos("0a8f6d44-d9d1-4474-bcfa-cfb41f8b66e8", this);
        pluginStorage()->beginGroup(device->id().toString());
        QString username = pluginStorage()->value("username").toString();
        QString password = pluginStorage()->value("password").toString();
        pluginStorage()->endGroup();

        sonos->authenticate(username, password);

        m_sonosConnections.insert(device->id(), sonos);
        connect(sonos, &Sonos::authenticationFinished, this, [this, sonos](bool success){
            if (success) {
            } else {
                qCWarning(dcSonos()) << "Cannot authenticate to Sonos api";
            }
        });

    }

    if (device->deviceClassId() == sonosGroupDeviceClassId) {

        //set parent ID
    }
    return Device::DeviceSetupStatusSuccess;
}

void DevicePluginSonos::postSetupDevice(Device *device)
{
    if (device->deviceClassId() == sonosConnectionDeviceClassId) {

    }

    if (device->deviceClassId() == sonosGroupDeviceClassId) {

    }
}

void DevicePluginSonos::deviceRemoved(Device *device)
{
    qCDebug(dcSonos) << "Delete " << device->name();
    if (myDevices().empty()) {
    }
}


Device::DeviceError DevicePluginSonos::executeAction(Device *device, const Action &action)
{
    Q_UNUSED(action)
    if (device->deviceClassId() == sonosGroupDeviceClassId) {
        Sonos *sonos = m_sonosConnections.value(device->parentId());
        int groupId = device->paramValue(sonosGroupDe)
        if (!sonos)
            return Device::DeviceErrorInvalidParameter;

        if (action.actionTypeId()  == sonosGroupPlayActionTypeId) {
            sonos->play();
            return Device::DeviceErrorAsync;
        }

        if (action.actionTypeId()  == sonosGroupShuffleActionTypeId) {

            return Device::DeviceErrorAsync;
        }

        if (action.actionTypeId()  == sonosGroupRepeatActionTypeId) {

            if (action.param(sonosGroupRepeatActionRepeatParamTypeId).value().toString() == "None") {

            } else if (action.param(sonosGroupShuffleActionShuffleParamTypeId).value().toString() == "One") {

            } else if (action.param(sonosGroupShuffleActionShuffleParamTypeId).value().toString() == "All") {

            } else {
                return Device::DeviceErrorHardwareFailure;
            }


            return Device::DeviceErrorAsync;
        }

        if (action.actionTypeId() == sonosGroupPauseActionTypeId) {
            sonos->pause();
            return Device::DeviceErrorNoError;
        }

        if (action.actionTypeId() == sonosGroupStopActionTypeId) {
            sonos->stop();
            return Device::DeviceErrorNoError;
        }

        if (action.actionTypeId() == sonosGroupMuteActionTypeId) {
            bool mute = action.param(sonosGroupMuteActionMuteParamTypeId).value().toBool();

           sonos->setGroupMute()
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
