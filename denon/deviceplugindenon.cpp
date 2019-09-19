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
#include "devices/device.h"
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

Device::DeviceError DevicePluginDenon::discoverDevices(const DeviceClassId &deviceClassId, const ParamList &params)
{
    Q_UNUSED(params)

    if (deviceClassId == AVRX1000DeviceClassId) {
        if (!m_serviceBrowser) {
            m_serviceBrowser = hardwareManager()->zeroConfController()->createServiceBrowser();
            connect(m_serviceBrowser, &ZeroConfServiceBrowser::serviceEntryAdded, this, &DevicePluginDenon::onAvahiServiceEntryAdded);
            connect(m_serviceBrowser, &ZeroConfServiceBrowser::serviceEntryRemoved, this, &DevicePluginDenon::onAvahiServiceEntryRemoved);
        }
        QStringList discoveredIds;

        QList<DeviceDescriptor> deviceDescriptors;
        foreach (const ZeroConfServiceEntry &service, m_serviceBrowser->serviceEntries()) {
            if (service.txt().contains("am=AVRX1000")) {

                QString id = service.name().split("@").first();
                QString name = service.name().split("@").last();
                QString address = service.hostAddress().toString();
                qCDebug(dcDenon) << "service discovered" << name << "ID:" << id;
                if (discoveredIds.contains(id))
                    break;
                discoveredIds.append(id);
                DeviceDescriptor deviceDescriptor(AVRX1000DeviceClassId, name, address);
                ParamList params;
                params.append(Param(AVRX1000DeviceIpParamTypeId, address));
                params.append(Param(AVRX1000DeviceIdParamTypeId, id));
                deviceDescriptor.setParams(params);
                foreach (Device *existingDevice, myDevices()) {
                    if (existingDevice->paramValue(AVRX1000DeviceIdParamTypeId).toString() == id) {
                        deviceDescriptor.setDeviceId(existingDevice->id());
                        break;
                    }
                }
                deviceDescriptors.append(deviceDescriptor);
            }
        }

        emit devicesDiscovered(AVRX1000DeviceClassId, deviceDescriptors);
        return Device::DeviceErrorAsync;
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
        return Device::DeviceErrorAsync;
    }
    return Device::DeviceErrorDeviceClassNotFound;
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
            return Device::DeviceSetupStatusFailure;
        }

        AvrConnection *denonConnection = new AvrConnection(address, 23, this);
        connect(denonConnection, &AvrConnection::connectionStatusChanged, this, &DevicePluginDenon::onAvrConnectionChanged);
        connect(denonConnection, &AvrConnection::socketErrorOccured, this, &DevicePluginDenon::onAvrSocketError);
        connect(denonConnection, &AvrConnection::channelChanged, this, &DevicePluginDenon::onAvrChannelChanged);
        connect(denonConnection, &AvrConnection::powerChanged, this, &DevicePluginDenon::onAvrPowerChanged);
        connect(denonConnection, &AvrConnection::volumeChanged, this, &DevicePluginDenon::onAvrVolumeChanged);
        connect(denonConnection, &AvrConnection::surroundModeChanged, this, &DevicePluginDenon::onAvrSurroundModeChanged);
        connect(denonConnection, &AvrConnection::muteChanged, this, &DevicePluginDenon::onAvrMuteChanged);

        m_asyncAvrSetups.append(denonConnection);
        denonConnection->connectDevice();
        m_avrConnections.insert(device, denonConnection);
        return Device::DeviceSetupStatusAsync;
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

        m_asyncHeosSetups.append(heos);
        heos->connectHeos();
        m_heos.insert(device, heos);
        return Device::DeviceSetupStatusAsync;
    }

    if (device->deviceClassId() == heosPlayerDeviceClassId) {
        return Device::DeviceSetupStatusSuccess;
    }
    return Device::DeviceSetupStatusFailure;
}

void DevicePluginDenon::deviceRemoved(Device *device)
{
    qCDebug(dcDenon) << "Delete " << device->name();

    if (device->deviceClassId() == AVRX1000DeviceClassId) {
        AvrConnection *denonConnection = m_avrConnections.value(device);
        m_avrConnections.remove(device);
        denonConnection->disconnectDevice();
        denonConnection->deleteLater();
    }

    if (device->deviceClassId() == heosDeviceClassId) {
        if (m_avrConnections.contains(device)) {
            AvrConnection *denonConnection = m_avrConnections.value(device);
            m_avrConnections.remove(device);
            denonConnection->disconnectDevice();
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
        AvrConnection *avrConnection = m_avrConnections.value(device);

        if (action.actionTypeId() == AVRX1000PowerActionTypeId) {

            bool power = action.param(AVRX1000PowerActionPowerParamTypeId).value().toBool();
            avrConnection->setPower(power);
            return Device::DeviceErrorNoError;

        } else if (action.actionTypeId() == AVRX1000VolumeActionTypeId) {

            int vol = action.param(AVRX1000VolumeActionVolumeParamTypeId).value().toInt();
            avrConnection->setVolume(vol);
            return Device::DeviceErrorNoError;

        } else if (action.actionTypeId() == AVRX1000ChannelActionTypeId) {

            qCDebug(dcDenon) << "Execute update action" << action.id();
            QByteArray channel = action.param(AVRX1000ChannelActionChannelParamTypeId).value().toByteArray();
            avrConnection->setChannel(channel);
            return Device::DeviceErrorNoError;

        } else if (action.actionTypeId() == AVRX1000IncreaseVolumeActionTypeId) {

            avrConnection->increaseVolume();
            return Device::DeviceErrorNoError;

        } else if (action.actionTypeId() == AVRX1000DecreaseVolumeActionTypeId) {

            avrConnection->decreaseVolume();
            return Device::DeviceErrorNoError;

        } else if (action.actionTypeId() == AVRX1000SurroundModeActionTypeId) {

            QByteArray surroundMode = action.param(AVRX1000SurroundModeActionSurroundModeParamTypeId).value().toByteArray();
            avrConnection->setSurroundMode(surroundMode);
            return Device::DeviceErrorNoError;
        }
        return Device::DeviceErrorActionTypeNotFound;
    }

    if (device->deviceClassId() == heosPlayerDeviceClassId) {

        Device *heosDevice = myDevices().findById(device->parentId());
        Heos *heos = m_heos.value(heosDevice);
        int playerId = device->paramValue(heosPlayerDevicePlayerIdParamTypeId).toInt();

        if (action.actionTypeId() == heosPlayerVolumeActionTypeId) {
            int volume = action.param(heosPlayerVolumeActionVolumeParamTypeId).value().toInt();
            heos->setVolume(playerId, volume);
            return Device::DeviceErrorNoError;
        }

        if (action.actionTypeId() == heosPlayerMuteActionTypeId) {
            bool mute = action.param(heosPlayerMuteActionMuteParamTypeId).value().toBool();
            heos->setMute(playerId, mute);
            return Device::DeviceErrorNoError;
        }

        if (action.actionTypeId() == heosPlayerPlaybackStatusActionTypeId) {
            QString playbackStatus = action.param(heosPlayerPlaybackStatusActionPlaybackStatusParamTypeId).value().toString();
            if (playbackStatus == "playing") {
                heos->setPlayerState(playerId, PLAYER_STATE_PLAY);
            } else if (playbackStatus == "stopping") {
                heos->setPlayerState(playerId, PLAYER_STATE_STOP);
            } else if (playbackStatus == "pausing") {
                heos->setPlayerState(playerId, PLAYER_STATE_PAUSE);
            }
            return Device::DeviceErrorNoError;
        }

        if (action.actionTypeId() == heosPlayerShuffleActionTypeId) {
            bool shuffle = action.param(heosPlayerShuffleActionShuffleParamTypeId).value().toBool();
            REPEAT_MODE repeatMode = REPEAT_MODE_OFF;
            if ( device->stateValue(heosPlayerRepeatStateTypeId) == "One") {
                repeatMode = REPEAT_MODE_ONE;
            } else if ( device->stateValue(heosPlayerRepeatStateTypeId) == "All") {
                repeatMode = REPEAT_MODE_ALL;
            }
            heos->setPlayMode(playerId, repeatMode, shuffle);
            return Device::DeviceErrorNoError;
        }

        if (action.actionTypeId() == heosPlayerSkipBackActionTypeId) {
            heos->playPrevious(playerId);
            return Device::DeviceErrorNoError;
        }

        if (action.actionTypeId() == heosPlayerStopActionTypeId) {
            heos->setPlayerState(playerId, PLAYER_STATE_STOP);
            return Device::DeviceErrorNoError;
        }

        if (action.actionTypeId() == heosPlayerPlayActionTypeId) {
            heos->setPlayerState(playerId, PLAYER_STATE_PLAY);
            return Device::DeviceErrorNoError;
        }

        if (action.actionTypeId() == heosPlayerPauseActionTypeId) {
            heos->setPlayerState(playerId, PLAYER_STATE_PAUSE);
            return Device::DeviceErrorNoError;
        }

        if (action.actionTypeId() == heosPlayerSkipNextActionTypeId) {
            heos->playNext(playerId);
            return Device::DeviceErrorNoError;
        }
        return Device::DeviceErrorActionTypeNotFound;
    }
    return Device::DeviceErrorDeviceClassNotFound;
}

void DevicePluginDenon::postSetupDevice(Device *device)
{
    if (device->deviceClassId() == heosDeviceClassId) {

        Heos *heos = m_heos.value(device);
        heos->getPlayers();
    }

    if (device->deviceClassId() == heosPlayerDeviceClassId) {
        device->setStateValue(heosPlayerConnectedStateTypeId, true);
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


void DevicePluginDenon::onPluginTimer()
{
    foreach(AvrConnection *denonConnection, m_avrConnections.values()) {
        if (!denonConnection->connected()) {
            denonConnection->connectDevice();
        }
        Device *device = m_avrConnections.key(denonConnection);
        if (device->deviceClassId() == AVRX1000DeviceClassId) {
            denonConnection->getAllStatus();
        }
    }

    foreach(Device *device, myDevices()) {

        if (device->deviceClassId() == heosDeviceClassId) {
            Heos *heos = m_heos.value(device);
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

void DevicePluginDenon::onAvrConnectionChanged(bool status)
{
    AvrConnection *denonConnection = static_cast<AvrConnection *>(sender());
    Device *device = m_avrConnections.key(denonConnection);
    if (!device)
        return;

    if (device->deviceClassId() == AVRX1000DeviceClassId) {
        // if the device is connected
        if (status) {
            // and from the first setup
            if (m_asyncAvrSetups.contains(denonConnection)) {
                m_asyncAvrSetups.removeAll(denonConnection);

                emit deviceSetupFinished(device, Device::DeviceSetupStatusSuccess);
            }
        }
        device->setStateValue(AVRX1000ConnectedStateTypeId, denonConnection->connected());
    }
}

void DevicePluginDenon::onAvrVolumeChanged(int volume)
{
    AvrConnection *denonConnection = static_cast<AvrConnection *>(sender());
    Device *device = m_avrConnections.key(denonConnection);
    if (!device)
        return;

    if (device->deviceClassId() == AVRX1000DeviceClassId) {
        device->setStateValue(AVRX1000VolumeStateTypeId, volume);
    }
}

void DevicePluginDenon::onAvrChannelChanged(const QByteArray &channel)
{
    AvrConnection *denonConnection = static_cast<AvrConnection *>(sender());
    Device *device = m_avrConnections.key(denonConnection);
    if (!device)
        return;

    if (device->deviceClassId() == AVRX1000DeviceClassId) {
        device->setStateValue(AVRX1000ChannelStateTypeId, channel);
    }
}

void DevicePluginDenon::onAvrMuteChanged(bool mute)
{
    AvrConnection *denonConnection = static_cast<AvrConnection *>(sender());
    Device *device = m_avrConnections.key(denonConnection);
    if (!device)
        return;

    if (device->deviceClassId() == AVRX1000DeviceClassId) {
        device->setStateValue(AVRX1000MuteStateTypeId, mute);
    }
}

void DevicePluginDenon::onAvrPowerChanged(bool power)
{
    AvrConnection *denonConnection = static_cast<AvrConnection *>(sender());
    Device *device = m_avrConnections.key(denonConnection);
    if (!device)
        return;

    if (device->deviceClassId() == AVRX1000DeviceClassId) {
        device->setStateValue(AVRX1000PowerStateTypeId, power);
    }
}

void DevicePluginDenon::onAvrSurroundModeChanged(const QByteArray &surroundMode)
{
    AvrConnection *denonConnection = static_cast<AvrConnection *>(sender());
    Device *device = m_avrConnections.key(denonConnection);
    if (!device)
        return;

    if (device->deviceClassId() == AVRX1000DeviceClassId) {
        device->setStateValue(AVRX1000SurroundModeStateTypeId, surroundMode);
    }
}


void DevicePluginDenon::onAvrSocketError()
{
    AvrConnection *denonConnection = static_cast<AvrConnection *>(sender());
    Device *device = m_avrConnections.key(denonConnection);
    if (!device)
        return;

    if (device->deviceClassId() == AVRX1000DeviceClassId) {

        // Check if setup running for this device
        if (m_asyncAvrSetups.contains(denonConnection)) {
            m_asyncAvrSetups.removeAll(denonConnection);
            qCWarning(dcDenon()) << "Could not add device. The setup failed.";
            emit deviceSetupFinished(device, Device::DeviceSetupStatusFailure);
            // Delete the connection, the device will not be added and
            // the connection will be created in the next setup
            denonConnection->deleteLater();
            m_avrConnections.remove(device);
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
    foreach (const UpnpDeviceDescriptor &upnpDevice, reply->deviceDescriptors()) {

        if (upnpDevice.modelName().contains("HEOS")) {
            QString serialNumber = upnpDevice.serialNumber();
            if (serialNumber != "0000001") {
                // child devices have serial number 0000001
                qCDebug(dcDenon) << "UPnP device found:" << upnpDevice.modelDescription() << upnpDevice.friendlyName() << upnpDevice.hostAddress().toString() << upnpDevice.modelName() << upnpDevice.manufacturer() << upnpDevice.serialNumber();
                DeviceDescriptor descriptor(heosDeviceClassId, upnpDevice.modelName(), serialNumber);
                ParamList params;
                foreach (Device *existingDevice, myDevices()) {
                    if (existingDevice->paramValue(heosDeviceSerialNumberParamTypeId).toString().contains(serialNumber, Qt::CaseSensitivity::CaseInsensitive)) {
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
        qCDebug(dcDenon) << "UPnP device found:" << upnpDevice.modelDescription() << upnpDevice.friendlyName() << upnpDevice.hostAddress().toString() << upnpDevice.modelName() << upnpDevice.manufacturer() << upnpDevice.serialNumber();
    }
    if (!heosDescriptors.isEmpty()) {
        emit devicesDiscovered(heosDeviceClassId, heosDescriptors);
    }
}

void DevicePluginDenon::onHeosConnectionChanged(bool status)
{
    Heos *heos = static_cast<Heos *>(sender());
    heos->registerForChangeEvents(true);
    Device *device = m_heos.key(heos);
    if (!device)
        return;

    if (device->deviceClassId() == heosDeviceClassId) {
        // if the device is connected
        if (status) {
            // and from the first setup
            if (m_asyncHeosSetups.contains(heos)) {
                m_asyncHeosSetups.removeAll(heos);
                heos->getPlayers();
                emit deviceSetupFinished(device, Device::DeviceSetupStatusSuccess);
            }
        }
        device->setStateValue(heosConnectedStateTypeId, status);
        // update connection status for all child devices
        foreach (Device *playerDevice, myDevices()) {
            if (playerDevice->deviceClassId() == heosPlayerDeviceClassId) {
                if (playerDevice->parentId() == device->id()) {
                    playerDevice->setStateValue(heosPlayerConnectedStateTypeId, status);
                }
            }
        }
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

void DevicePluginDenon::onHeosPlayStateReceived(int playerId, PLAYER_STATE state)
{
    foreach(Device *device, myDevices().filterByParam(heosPlayerDevicePlayerIdParamTypeId, playerId)) {
        if (state == PLAYER_STATE_PAUSE) {
            device->setStateValue(heosPlayerPlaybackStatusStateTypeId, "Paused");
        } else if (state == PLAYER_STATE_PLAY) {
            device->setStateValue(heosPlayerPlaybackStatusStateTypeId, "Playing");
        } else if (state == PLAYER_STATE_STOP) {
            device->setStateValue(heosPlayerPlaybackStatusStateTypeId, "Stopped");
        }
        break;
    }
}


void DevicePluginDenon::onHeosRepeatModeReceived(int playerId, REPEAT_MODE repeatMode)
{
    foreach(Device *device, myDevices().filterByParam(heosPlayerDevicePlayerIdParamTypeId, playerId)) {
        if (repeatMode == REPEAT_MODE_ALL) {
            device->setStateValue(heosPlayerRepeatStateTypeId, "All");
        } else  if (repeatMode == REPEAT_MODE_ONE) {
            device->setStateValue(heosPlayerRepeatStateTypeId, "One");
        } else  if (repeatMode == REPEAT_MODE_OFF) {
            device->setStateValue(heosPlayerRepeatStateTypeId, "None");
        }
        break;
    }
}

void DevicePluginDenon::onHeosShuffleModeReceived(int playerId, bool shuffle)
{
    foreach(Device *device, myDevices().filterByParam(heosPlayerDevicePlayerIdParamTypeId, playerId)) {
        device->setStateValue(heosPlayerMuteStateTypeId, shuffle);
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

void DevicePluginDenon::onHeosNowPlayingMediaStatusReceived(int playerId, SOURCE_ID sourceId, QString artist, QString album, QString song, QString artwork)
{
    foreach(Device *device, myDevices().filterByParam(heosPlayerDevicePlayerIdParamTypeId, playerId)) {
        device->setStateValue(heosPlayerArtistStateTypeId, artist);
        device->setStateValue(heosPlayerTitleStateTypeId, song);
        device->setStateValue(heosPlayerArtworkStateTypeId, artwork);
        device->setStateValue(heosPlayerCollectionStateTypeId, album);
        QString source;
        switch (sourceId) {
        case SOURCE_ID_PANDORA:
            source = "Pandora";
            break;
        case SOURCE_ID_RHAPSODY:
            source = "Rhapsody";
            break;
        case SOURCE_ID_TUNEIN:
            source = "TuneIn";
            break;
        case SOURCE_ID_SPOTIFY:
            source = "Spotify";
            break;
        case SOURCE_ID_DEEZER:
            source = "Deezer";
            break;
        case SOURCE_ID_NAPSTER:
            source = "Napster";
            break;
        case SOURCE_ID_IHEARTRADIO:
            source = "iHeartRadio";
            break;
        case SOURCE_ID_SIRIUS_XM:
            source = "Sirius XM";
            break;
        case SOURCE_ID_SOUNDCLOUD:
            source = "Soundcloud";
            break;
        case SOURCE_ID_TIDAL:
            source = "Tidal";
            break;
        case SOURCE_ID_FUTURE_SERVICE_1:
            source = "Unknown";
            break;
        case SOURCE_ID_RDIO:
            source = "Rdio";
            break;
        case SOURCE_ID_AMAZON_MUSIC:
            source = "Amazon Music";
            break;
        case SOURCE_ID_FUTURE_SERVICE_2:
            source = "Unknown";
            break;
        case SOURCE_ID_MOODMIX:
            source = "Moodmix";
            break;
        case SOURCE_ID_JUKE:
            source = "Juke";
            break;
        case SOURCE_ID_FUTURE_SERVICE_3:
            source = "Unkown";
            break;
        case SOURCE_ID_QQMUSIC:
            source = "QQMusic";
            break;
        case SOURCE_ID_LOCAL_MEDIA:
            source = "USB Media/DLNA Servers";
            break;
        case SOURCE_ID_HEOS_PLAYLIST:
            source = "HEOS Playlists";
            break;
        case SOURCE_ID_HEOS_HISTORY:
            source = "HEOS History";
            break;
        case SOURCE_ID_HEOS_FAVORITES:
            source = "HEOS Favorites";
            break;
        case SOURCE_ID_HEOS_AUX:
            source = "HEOS aux input";
            break;
        };
        device->setStateValue(heosPlayerSourceStateTypeId, source);
        break;
    }
}

void DevicePluginDenon::onAvahiServiceEntryAdded(const ZeroConfServiceEntry &serviceEntry)
{
    qCDebug(dcDenon()) << "Avahi service entry added:" << serviceEntry;
}

void DevicePluginDenon::onAvahiServiceEntryRemoved(const ZeroConfServiceEntry &serviceEntry)
{
    qCDebug(dcDenon()) << "Avahi service entry removed:" << serviceEntry;
}
