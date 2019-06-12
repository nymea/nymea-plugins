/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *                                                                         *
 *  Copyright (C) 2015 Simon Stürz <simon.stuerz@nymea.io>                 *
 *  Copyright (C) 2019 Bernhard Trinnes <bernhard.trinnes@nymea.io>        *
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

/*!
    \page denon.html
    \title Denon
    \brief Plugin for Denon AV and Heos Devices

    \ingroup plugins
    \ingroup nymea-plugins

    This plug-in supports the
    \l {http://www.denon.de/de/product/hometheater/avreceivers/avrx1000}{Denon AV Amplifier AVR-X1000}

    \chapter Plugin properties
    Following JSON file contains the definition and the description of all available \l{DeviceClass}{DeviceClasses}
    and \l{Vendor}{Vendors} of this \l{DevicePlugin}.

    For more details how to read this JSON file please check out the documentation for \l{The plugin JSON File}.

    \quotefile plugins/deviceplugins/denon/deviceplugindenon.json
*/

#include "deviceplugindenon.h"
#include "plugininfo.h"
#include "plugin/device.h"
#include "network/networkaccessmanager.h"
#include "network/upnp/upnpdiscovery.h"
#include "network/upnp/upnpdiscoveryreply.h"
#include "platform/platformzeroconfcontroller.h"
#include "network/zeroconf/zeroconfservicebrowser.h"

#include <QDebug>
#include <QStringList>
#include <QJsonDocument>
#include <QTimer>

DevicePluginDenon::DevicePluginDenon()
{
}

DeviceManager::DeviceError DevicePluginDenon::discoverDevices(const DeviceClassId &deviceClassId, const ParamList &params)
{
    Q_UNUSED(params)

    if (deviceClassId == AVRX1000DeviceClassId) {

        UpnpDiscoveryReply *reply = hardwareManager()->upnpDiscovery()->discoverDevices("urn:schemas-upnp-org:device:MediaRenderer:1", "nymea", 7000);
        connect(reply, &UpnpDiscoveryReply::finished, this, &DevicePluginDenon::onUpnpDiscoveryFinished);
        return DeviceManager::DeviceErrorAsync;
    }

    if (deviceClassId == heosDeviceClassId) {
        /*
        * The HEOS products can be discovered using the UPnP SSDP protocol. Through discovery,
        * the IP address of the HEOS products can be retrieved. Once the IP address is retrieved,
        * a telnet connection to port 1255 can be opened to access the HEOS CLI and control the HEOS system.
        * The HEOS product IP address can also be set statically and manually programmed into the control system.
        * Search target name (ST) in M-SEARCH discovery request is 'urn:schemas-denon-com:device:ACT-Denon:1'.
        */
        UpnpDiscoveryReply *reply = hardwareManager()->upnpDiscovery()->discoverDevices();
        connect(reply, &UpnpDiscoveryReply::finished, this, &DevicePluginDenon::onUpnpDiscoveryFinished);
        return DeviceManager::DeviceErrorAsync;
    }
    return DeviceManager::DeviceErrorDeviceClassNotFound;
}

Device::DeviceSetupStatus DevicePluginDenon::setupDevice(Device *device)
{
    if(!m_pluginTimer) {
        m_pluginTimer = hardwareManager()->pluginTimerManager()->registerTimer(60);
        connect(m_pluginTimer, &PluginTimer::timeout, this, &DevicePluginDenon::onPluginTimer);
    }

    if (device->deviceClassId() == AVRX1000DeviceClassId) {
        qCDebug(dcDenon) << "Setup Denon device" << device->paramValue(AVRX1000DeviceIpParamTypeId).toString();

        QHostAddress address(device->paramValue(AVRX1000DeviceIpParamTypeId).toString());
        if (address.isNull()) {
            qCWarning(dcDenon) << "Could not parse ip address" << device->paramValue(AVRX1000DeviceIpParamTypeId).toString();
            return DeviceManager::DeviceSetupStatusFailure;
        }

        DenonConnection *denonConnection = new DenonConnection(address, 23, this);
        connect(denonConnection, &DenonConnection::connectionStatusChanged, this, &DevicePluginDenon::onAVRConnectionChanged);
        connect(denonConnection, &DenonConnection::socketErrorOccured, this, &DevicePluginDenon::onAVRSocketError);
        connect(denonConnection, &DenonConnection::dataReady, this, &DevicePluginDenon::onAVRDataReceived);

        m_asyncSetups.append(denonConnection);
        denonConnection->connectDenon();
        m_denonConnections.insert(device, denonConnection);
        return DeviceManager::DeviceSetupStatusAsync;
    }

    if (device->deviceClassId() == heosDeviceClassId) {
        qCDebug(dcDenon) << "Setup Denon device" << device->paramValue(heosDeviceIpParamTypeId).toString();

        QHostAddress address(device->paramValue(heosDeviceIpParamTypeId).toString());
        Heos *heos = new Heos(address, this);
        connect(heos, &Heos::connectionStatusChanged, this, &DevicePluginDenon::onHeosConnectionChanged);
        connect(heos, &Heos::playerDiscovered, this, &DevicePluginDenon::onHeosPlayerDiscovered);
        connect(heos, &Heos::playStateReceived, this, &DevicePluginDenon::onHeosPlayStateReceived);
        connect(heos, &Heos::repeatModeReceived, this, &DevicePluginDenon::onHeosRepeatModeReceived);
        connect(heos, &Heos::shuffleModeReceived, this, &DevicePluginDenon::onHeosShuffleModeReceived);
        connect(heos, &Heos::muteStatusReceived, this, &DevicePluginDenon::onHeosMuteStatusReceived);
        connect(heos, &Heos::volumeStatusReceived, this, &DevicePluginDenon::onHeosVolumeStatusReceived);
        connect(heos, &Heos::nowPlayingMediaStatusReceived, this, &DevicePluginDenon::onHeosNowPlayingMediaStatusReceived);

        heos->connectHeos();
        m_heos.insert(device, heos);
        return DeviceManager::DeviceSetupStatusAsync;
    }

    if (device->deviceClassId() == heosPlayerDeviceClassId) {
        return DeviceManager::DeviceSetupStatusSuccess;
    }
    return DeviceManager::DeviceSetupStatusFailure;
}

void DevicePluginDenon::deviceRemoved(Device *device)
{
    qCDebug(dcDenon) << "Delete " << device->name();

    if (device->deviceClassId() == AVRX1000DeviceClassId) {
        DenonConnection *denonConnection = m_denonConnections.value(device);
        m_denonConnections.remove(device);
        denonConnection->disconnectDenon();
        denonConnection->deleteLater();
    }

    if (device->deviceClassId() == heosDeviceClassId) {
        if (m_denonConnections.contains(device)) {
            DenonConnection *denonConnection = m_denonConnections.value(device);
            m_denonConnections.remove(device);
            denonConnection->disconnectDenon();
            denonConnection->deleteLater();
        }
    }

    if (myDevices().empty()) {
        hardwareManager()->pluginTimerManager()->unregisterTimer(m_pluginTimer);
    }
}

Device::DeviceError DevicePluginDenon::executeAction(Device *device, const Action &action)
{
    qCDebug(dcDenon) << "Execute action" << device->id() << action.id() << action.params();
    if (device->deviceClassId() == AVRX1000DeviceClassId) {
        DenonConnection *denonConnection = m_denonConnections.value(device);

        if (action.actionTypeId() == AVRX1000PowerActionTypeId) {

            qCDebug(dcDenon) << "set power action" << action.id();
            qCDebug(dcDenon) << "power: " << action.param(AVRX1000PowerActionPowerParamTypeId).value().Bool;

            if (action.param(AVRX1000PowerActionPowerParamTypeId).value().toBool() == true) {
                QByteArray cmd = "PWON\r";
                qCDebug(dcDenon) << "Execute power: " << action.id() << cmd;
                denonConnection->sendData(cmd);
            } else {
                QByteArray cmd = "PWSTANDBY\r";
                qCDebug(dcDenon) << "Execute power: " << action.id() << cmd;
                denonConnection->sendData(cmd);
            }

            return Device::DeviceErrorNoError;

        } else if (action.actionTypeId() == AVRX1000VolumeActionTypeId) {

            QByteArray vol = action.param(AVRX1000VolumeActionVolumeParamTypeId).value().toByteArray();
            QByteArray cmd = "MV" + vol + "\r";

            qCDebug(dcDenon) << "Execute volume" << action.id() << cmd;
            denonConnection->sendData(cmd);
            return DeviceManager::DeviceErrorNoError;

        } else if (action.actionTypeId() == AVRX1000ChannelActionTypeId) {

            qCDebug(dcDenon) << "Execute update action" << action.id();
            QByteArray channel = action.param(AVRX1000ChannelActionChannelParamTypeId).value().toByteArray();
            QByteArray cmd = "SI" + channel + "\r";

            qCDebug(dcDenon) << "Change to channel:" << cmd;
            denonConnection->sendData(cmd);
            return DeviceManager::DeviceErrorNoError;

        } else if (action.actionTypeId() == AVRX1000IncreaseVolumeActionTypeId) {
            QByteArray cmd = "MVUP\r";
            qCDebug(dcDenon) << "Execute volume increase" << action.id() << cmd;
            denonConnection->sendData(cmd);
            return DeviceManager::DeviceErrorNoError;

        } else if (action.actionTypeId() == AVRX1000DecreaseVolumeActionTypeId) {
            QByteArray cmd = "MVDOWN\r";
            qCDebug(dcDenon) << "Execute volume decrease" << action.id() << cmd;
            denonConnection->sendData(cmd);
            return DeviceManager::DeviceErrorNoError;
        }
        return DeviceManager::DeviceErrorActionTypeNotFound;
    }

    if (device->deviceClassId() == heosPlayerDeviceClassId) {

        Device *heosDevice = myDevices().findById(device->parentId());
        Heos *heos = m_heos.value(heosDevice);
        int playerId = device->paramValue(heosPlayerDevicePlayerIdParamTypeId).toInt();

        if (action.actionTypeId() == heosPlayerVolumeActionTypeId) {
            int volume = action.param(heosPlayerVolumeActionVolumeParamTypeId).value().toInt();
            heos->setVolume(playerId, volume);
            return DeviceManager::DeviceErrorNoError;
        }

        if (action.actionTypeId() == heosPlayerMuteActionTypeId) {
            bool mute = action.param(heosPlayerMuteActionMuteParamTypeId).value().toBool();
            heos->setMute(playerId, mute);
            return DeviceManager::DeviceErrorNoError;
        }

        if (action.actionTypeId() == heosPlayerPlaybackStatusActionTypeId) {
            QString playbackStatus = action.param(heosPlayerPlaybackStatusActionPlaybackStatusParamTypeId).value().toString();
            if (playbackStatus == "playing") {
                heos->setPlayerState(playerId, Heos::HeosPlayerState::Play);
            } else if (playbackStatus == "stopping") {
                heos->setPlayerState(playerId, Heos::HeosPlayerState::Stop);
            } else if (playbackStatus == "pausing") {
                heos->setPlayerState(playerId, Heos::HeosPlayerState::Pause);
            }
            return DeviceManager::DeviceErrorNoError;
        }

        if (action.actionTypeId() == heosPlayerShuffleActionTypeId) {
            bool shuffle = action.param(heosPlayerShuffleActionShuffleParamTypeId).value().toBool();
            Heos::HeosRepeatMode repeatMode;
            repeatMode = Heos::HeosRepeatMode::Off;
            heos->setPlayMode(playerId, repeatMode, shuffle);

            return DeviceManager::DeviceErrorNoError;
        }

        if (action.actionTypeId() == heosPlayerSkipBackActionTypeId) {
            heos->playPrevious(playerId);
            return DeviceManager::DeviceErrorNoError;
        }

        if (action.actionTypeId() == heosPlayerFastRewindActionTypeId) {

            return DeviceManager::DeviceErrorActionTypeNotFound;
        }

        if (action.actionTypeId() == heosPlayerStopActionTypeId) {
            heos->setPlayerState(playerId, Heos::HeosPlayerState::Stop);
            return DeviceManager::DeviceErrorNoError;
        }

        if (action.actionTypeId() == heosPlayerPlayActionTypeId) {
            heos->setPlayerState(playerId, Heos::HeosPlayerState::Play);
            return DeviceManager::DeviceErrorNoError;
        }

        if (action.actionTypeId() == heosPlayerPauseActionTypeId) {

            heos->setPlayerState(playerId, Heos::HeosPlayerState::Pause);
            return DeviceManager::DeviceErrorNoError;
        }

        if (action.actionTypeId() == heosPlayerFastForwardActionTypeId) {

            return DeviceManager::DeviceErrorActionTypeNotFound;
        }

        if (action.actionTypeId() == heosPlayerSkipNextActionTypeId) {
            heos->playNext(playerId);
            return DeviceManager::DeviceErrorNoError;
        }
        return DeviceManager::DeviceErrorActionTypeNotFound;
    }
    return Device::DeviceErrorDeviceClassNotFound;
}

void DevicePluginDenon::postSetupDevice(Device *device)
{
    if (device->deviceClassId() == heosDeviceClassId) {

        Heos *heos = m_heos.value(device);
        heos->getPlayers();
        device->setStateValue(heosConnectedStateTypeId, heos->connected());
    }

    if (device->deviceClassId() == heosPlayerDeviceClassId) {

        Device *heosDevice = myDevices().findById(device->parentId());
        Heos *heos = m_heos.value(heosDevice);
        int playerId = device->paramValue(heosPlayerDevicePlayerIdParamTypeId).toInt();
        heos->getPlayerState(playerId);
        heos->getPlayMode(playerId);
        heos->getVolume(playerId);
        heos->getMute(playerId);
        heos->getNowPlayingMedia(playerId);
        device->setStateValue(heosPlayerConnectedStateTypeId, heos->connected());
    }
}


void DevicePluginDenon::onPluginTimer()
{
    foreach(DenonConnection *denonConnection, m_denonConnections.values()) {
        if (!denonConnection->connected()) {
            denonConnection->connectDenon();
        }
        Device *device = m_denonConnections.key(denonConnection);
        if (device->deviceClassId() == AVRX1000DeviceClassId) {
            denonConnection->sendData("PW?\rSI?\rMV?\r");
        }
    }

    foreach(Device *device, myDevices()) {

        if (device->deviceClassId() == heosDeviceClassId) {
            Heos *heos = m_heos.value(device);
            if (!heos->connected()) {
                heos->connectHeos();
            }
            device->setStateValue(heosConnectedStateTypeId, heos->connected());
            heos->getPlayers();
            heos->registerForChangeEvents(true);
        }

        if (device->deviceClassId() == heosPlayerDeviceClassId) {
            Device *heosDevice = myDevices().findById(device->parentId());
            Heos *heos = m_heos.value(heosDevice);
            int playerId = device->paramValue(heosPlayerDevicePlayerIdParamTypeId).toInt();

            heos->getPlayerState(playerId);
            heos->getPlayMode(playerId);
            heos->getVolume(playerId);
            heos->getMute(playerId);
            heos->getNowPlayingMedia(playerId);
        }
    }
}

void DevicePluginDenon::onAVRConnectionChanged()
{
    DenonConnection *denonConnection = static_cast<DenonConnection *>(sender());
    Device *device = m_denonConnections.key(denonConnection);

    if (device->deviceClassId() == AVRX1000DeviceClassId) {
        // if the device is connected
        if (denonConnection->connected()) {
            // and from the first setup
            if (m_asyncSetups.contains(denonConnection)) {
                m_asyncSetups.removeAll(denonConnection);
                denonConnection->sendData("PW?\rSI?\rMV?\r");
                emit deviceSetupFinished(device, DeviceManager::DeviceSetupStatusSuccess);
            }
        }
        device->setStateValue(AVRX1000ConnectedStateTypeId, denonConnection->connected());
    }
}

void DevicePluginDenon::onAVRDataReceived(const QByteArray &data)
{
    DenonConnection *denonConnection = static_cast<DenonConnection *>(sender());
    Device *device = m_denonConnections.key(denonConnection);
    qCDebug(dcDenon) << "Data received" << data;

    if (device->deviceClassId() == AVRX1000DeviceClassId) {
        if (data.contains("MV") && !data.contains("MAX")){
            int index = data.indexOf("MV");
            int vol = data.mid(index+2, 2).toInt();

            qCDebug(dcDenon) << "Update volume:" << vol;
            device->setStateValue(AVRX1000VolumeStateTypeId, vol);
        }

        if (data.contains("SI")) {
            QString cmd;
            if (data.contains("TUNER")) {
                cmd = "TUNER";
            } else if (data.contains("DVD")) {
                cmd = "DVD";
            } else if (data.contains("BD")) {
                cmd = "BD";
            } else if (data.contains("TV")) {
                cmd = "TV";
            } else if (data.contains("SAT/CBL")) {
                cmd = "SAT/CBL";
            } else if (data.contains("MPLAY")) {
                cmd = "MPLAY";
            } else if (data.contains("GAME")) {
                cmd = "GAME";
            } else if (data.contains("AUX1")) {
                cmd = "AUX1";
            } else if (data.contains("NET")) {
                cmd = "NET";
            } else if (data.contains("PANDORA")) {
                cmd = "PANDORA";
            } else if (data.contains("SIRIUSXM")) {
                cmd = "SIRIUSXM";
            } else if (data.contains("SPOTIFY")) {
                cmd = "SPOTIFY";
            } else if (data.contains("FLICKR")) {
                cmd = "FLICKR";
            } else if (data.contains("FAVORITES")) {
                cmd = "FAVORITES";
            } else if (data.contains("IRADIO")) {
                cmd = "IRADIO";
            } else if (data.contains("SERVER")) {
                cmd = "SERVER";
            } else if (data.contains("USB/IPOD")) {
                cmd = "USB/IPOD";
            } else if (data.contains("IPD")) {
                cmd = "IPD";
            } else if (data.contains("IRP")) {
                cmd = "IRP";
            } else if (data.contains("FVP")) {
                cmd = "FVP";
            }
            qCDebug(dcDenon) << "Update channel:" << cmd;
            device->setStateValue(AVRX1000ChannelStateTypeId, cmd);
        }

        if (data.contains("PWON")) {
            qCDebug(dcDenon) << "Update power on";
            device->setStateValue(AVRX1000PowerStateTypeId, true);
        } else if (data.contains("PWSTANDBY")) {
            qCDebug(dcDenon) << "Update power off";
            device->setStateValue(AVRX1000PowerStateTypeId, false);
        }
    }
}


void DevicePluginDenon::onAVRSocketError()
{
    DenonConnection *denonConnection = static_cast<DenonConnection *>(sender());
    Device *device = m_denonConnections.key(denonConnection);
    if (device->deviceClassId() == AVRX1000DeviceClassId) {

        // Check if setup running for this device
        if (m_asyncSetups.contains(denonConnection)) {
            m_asyncSetups.removeAll(denonConnection);
            qCWarning(dcDenon()) << "Could not add device. The setup failed.";
            emit deviceSetupFinished(device, DeviceManager::DeviceSetupStatusFailure);
            // Delete the connection, the device will not be added and
            // the connection will be created in the next setup
            denonConnection->deleteLater();
            m_denonConnections.remove(device);
        }
    }
}

void DevicePluginDenon::onUpnpDiscoveryFinished()
{
    qCDebug(dcDenon()) << "Upnp discovery finished";
    UpnpDiscoveryReply *reply = static_cast<UpnpDiscoveryReply *>(sender());
    if (reply->error() != UpnpDiscoveryReply::UpnpDiscoveryReplyErrorNoError) {
        qCWarning(dcDenon()) << "Upnp discovery error" << reply->error();
    }
    reply->deleteLater();

    if (reply->deviceDescriptors().isEmpty()) {
        qCDebug(dcDenon) << "No UPnP device found.";
        return;
    }

    QList<DeviceDescriptor> heosDescriptors;
    QList<DeviceDescriptor> avrDescriptors;
    foreach (const UpnpDeviceDescriptor &upnpDevice, reply->deviceDescriptors()) {

        if (upnpDevice.modelName().contains("HEOS")) {
            QString serialNumber = upnpDevice.serialNumber();
            if (serialNumber != "0000001") {
                // child devices have serial number 0000001
                qCDebug(dcDenon) << "UPnP device found:" << upnpDevice.modelDescription() << upnpDevice.friendlyName() << upnpDevice.hostAddress().toString() << upnpDevice.modelName() << upnpDevice.manufacturer() << upnpDevice.serialNumber();
                DeviceDescriptor descriptor(heosDeviceClassId, upnpDevice.modelName(), serialNumber);
                ParamList params;
                foreach (Device *existingDevice, myDevices()) {
                    if (existingDevice->paramValue(heosDeviceSerialNumberParamTypeId).toString() == serialNumber) {
                        descriptor.setDeviceId(existingDevice->id());
                        break;
                    }
                }
                params.append(Param(heosDeviceModelNameParamTypeId, upnpDevice.modelName()));
                params.append(Param(heosDeviceIpParamTypeId, upnpDevice.hostAddress().toString()));
                params.append(Param(heosDeviceSerialNumberParamTypeId, serialNumber));
                descriptor.setParams(params);
                heosDescriptors.append(descriptor);
            }
        }
        //if (upnpDevice.modelName().contains("")) {
            qCDebug(dcDenon) << "UPnP device found:" << upnpDevice.modelDescription() << upnpDevice.friendlyName() << upnpDevice.hostAddress().toString() << upnpDevice.modelName() << upnpDevice.manufacturer() << upnpDevice.serialNumber();
        //}
    }
    if (!heosDescriptors.isEmpty()) {
        emit devicesDiscovered(heosDeviceClassId, heosDescriptors);
    }
    if (!avrDescriptors.isEmpty()) {
        emit devicesDiscovered(AVRX1000DeviceClassId, avrDescriptors);
    }
}

void DevicePluginDenon::onHeosConnectionChanged()
{
    Heos *heos = static_cast<Heos *>(sender());
    heos->registerForChangeEvents(true);
    Device *device = m_heos.key(heos);
    if (!device->setupComplete() && heos->connected()) {
        emit deviceSetupFinished(device, DeviceManager::DeviceSetupStatusSuccess);
    }
}

void DevicePluginDenon::onHeosPlayerDiscovered(HeosPlayer *heosPlayer) {

    Heos *heos = static_cast<Heos *>(sender());
    Device *device = m_heos.key(heos);

    foreach (Device *heosPlayerDevice, myDevices()) {
        if(heosPlayerDevice->deviceClassId() == heosPlayerDeviceClassId) {
            if (heosPlayerDevice->paramValue(heosPlayerDevicePlayerIdParamTypeId).toInt() == heosPlayer->playerId())
                return;
        }
    }
    QList<DeviceDescriptor> heosPlayerDescriptors;
    DeviceDescriptor descriptor(heosPlayerDeviceClassId, heosPlayer->name(), heosPlayer->playerModel(), device->id());
    ParamList params;
    params.append(Param(heosPlayerDeviceModelParamTypeId, heosPlayer->playerModel()));
    params.append(Param(heosPlayerDevicePlayerIdParamTypeId, heosPlayer->playerId()));
    params.append(Param(heosPlayerDeviceSerialNumberParamTypeId, heosPlayer->serialNumber()));
    params.append(Param(heosPlayerDeviceVersionParamTypeId, heosPlayer->playerVersion()));
    descriptor.setParams(params);
    qCDebug(dcDenon) << "Found new heos player" << heosPlayer->name();
    heosPlayerDescriptors.append(descriptor);
    autoDevicesAppeared(heosPlayerDeviceClassId,  heosPlayerDescriptors);
}

void DevicePluginDenon::onHeosPlayStateReceived(int playerId, Heos::HeosPlayerState state)
{
    foreach(Device *device, myDevices().filterByParam(heosPlayerDevicePlayerIdParamTypeId, playerId)) {
        if (state == Heos::HeosPlayerState::Pause) {
            device->setStateValue(heosPlayerPlaybackStatusStateTypeId, "Paused");
        } else if (state == Heos::HeosPlayerState::Play) {
            device->setStateValue(heosPlayerPlaybackStatusStateTypeId, "Playing");
        } else if (state == Heos::HeosPlayerState::Stop) {
            device->setStateValue(heosPlayerPlaybackStatusStateTypeId, "Stopped");
        }
        break;
    }
}


void DevicePluginDenon::onHeosRepeatModeReceived(int playerId, Heos::HeosRepeatMode repeatMode)
{
    foreach(Device *device, myDevices().filterByParam(heosPlayerDevicePlayerIdParamTypeId, playerId)) {
        if (repeatMode == Heos::HeosRepeatMode::All) {
            device->setStateValue(heosPlayerRepeatStateTypeId, "All");
        } else  if (repeatMode == Heos::HeosRepeatMode::One) {
            device->setStateValue(heosPlayerRepeatStateTypeId, "One");
        } else  if (repeatMode == Heos::HeosRepeatMode::Off) {
            device->setStateValue(heosPlayerRepeatStateTypeId, "None");
        }
        break;
    }
}

void DevicePluginDenon::onHeosShuffleModeReceived(int playerId, bool shuffle)
{
    foreach(Device *device, myDevices().filterByParam(heosPlayerDevicePlayerIdParamTypeId, playerId)) {
        if (shuffle) {
            device->setStateValue(heosPlayerMuteStateTypeId, true);
        } else {
            device->setStateValue(heosPlayerMuteStateTypeId, false);
        }
        break;
    }
}

void DevicePluginDenon::onHeosMuteStatusReceived(int playerId, bool mute)
{
    foreach(Device *device, myDevices().filterByParam(heosPlayerDevicePlayerIdParamTypeId, playerId)) {
        device->setStateValue(heosPlayerMuteStateTypeId, mute);
        break;
    }
}

void DevicePluginDenon::onHeosVolumeStatusReceived(int playerId, int volume)
{
    foreach(Device *device, myDevices().filterByParam(heosPlayerDevicePlayerIdParamTypeId, playerId)) {
        device->setStateValue(heosPlayerVolumeStateTypeId, volume);
        break;
    }
}

void DevicePluginDenon::onHeosNowPlayingMediaStatusReceived(int playerId, QString source, QString artist, QString album, QString song, QString artwork)
{
    foreach(Device *device, myDevices().filterByParam(heosPlayerDevicePlayerIdParamTypeId, playerId)) {
        device->setStateValue(heosPlayerArtistStateTypeId, artist);
        device->setStateValue(heosPlayerTitleStateTypeId, song);
        device->setStateValue(heosPlayerArtworkStateTypeId, artwork);
        device->setStateValue(heosPlayerCollectionStateTypeId, album);
        device->setStateValue(heosPlayerSourceStateTypeId, source);
        break;
    }
}
