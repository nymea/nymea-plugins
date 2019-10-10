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

void DevicePluginDenon::discoverDevices(DeviceDiscoveryInfo *info)
{
    if (info->deviceClassId() == AVRX1000DeviceClassId) {
        if (!hardwareManager()->zeroConfController()->available() || !hardwareManager()->zeroConfController()->enabled()) {
            info->finish(Device::DeviceErrorHardwareNotAvailable, QT_TR_NOOP("Device discovery is not available."));
            return;
        }

        if (!m_serviceBrowser) {
            m_serviceBrowser = hardwareManager()->zeroConfController()->createServiceBrowser();
            connect(m_serviceBrowser, &ZeroConfServiceBrowser::serviceEntryAdded, this, &DevicePluginDenon::onAvahiServiceEntryAdded);
            connect(m_serviceBrowser, &ZeroConfServiceBrowser::serviceEntryRemoved, this, &DevicePluginDenon::onAvahiServiceEntryRemoved);
        }

        QTimer::singleShot(2000, info, [this, info](){
            QStringList discoveredIds;

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
                    info->addDeviceDescriptor(deviceDescriptor);
                }
            }

            info->finish(Device::DeviceErrorNoError);
        });

        return;
    }

    if (info->deviceClassId() == heosDeviceClassId) {
        /*
        * The HEOS products can be discovered using the UPnP SSDP protocol. Through discovery,
        * the IP address of the HEOS products can be retrieved. Once the IP address is retrieved,
        * a telnet connection to port 1255 can be opened to access the HEOS CLI and control the HEOS system.
        * The HEOS product IP address can also be set statically and manually programmed into the control system.
        * Search target name (ST) in M-SEARCH discovery request is 'urn:schemas-denon-com:device:ACT-Denon:1'.
        */
        UpnpDiscoveryReply *reply = hardwareManager()->upnpDiscovery()->discoverDevices();
        connect(reply, &UpnpDiscoveryReply::finished, info, [this, reply, info](){
            reply->deleteLater();

            if (reply->error() != UpnpDiscoveryReply::UpnpDiscoveryReplyErrorNoError) {
                qCWarning(dcDenon()) << "Upnp discovery error" << reply->error();
                info->finish(Device::DeviceErrorHardwareFailure, QT_TR_NOOP("UPnP discovery failed."));
                return;
            }

            foreach (const UpnpDeviceDescriptor &upnpDevice, reply->deviceDescriptors()) {
                qCDebug(dcDenon) << "UPnP device found:" << upnpDevice.modelDescription() << upnpDevice.friendlyName() << upnpDevice.hostAddress().toString() << upnpDevice.modelName() << upnpDevice.manufacturer() << upnpDevice.serialNumber();

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
                        info->addDeviceDescriptor(descriptor);
                    }
                }
            }
            info->finish(Device::DeviceErrorNoError);
        });
        return;
    }
    info->finish(Device::DeviceErrorDeviceClassNotFound);
}

void DevicePluginDenon::setupDevice(DeviceSetupInfo *info)
{
    Device *device = info->device();

    if (device->deviceClassId() == AVRX1000DeviceClassId) {
        qCDebug(dcDenon) << "Setup Denon device" << device->paramValue(AVRX1000DeviceIpParamTypeId).toString();

        QHostAddress address(device->paramValue(AVRX1000DeviceIpParamTypeId).toString());
        if (address.isNull()) {
            qCWarning(dcDenon) << "Could not parse ip address" << device->paramValue(AVRX1000DeviceIpParamTypeId).toString();
            info->finish(Device::DeviceErrorInvalidParameter, QT_TR_NOOP("The given IP address is not valid."));
            return;
        }

        AvrConnection *denonConnection = new AvrConnection(address, 23, this);
        connect(denonConnection, &AvrConnection::connectionStatusChanged, this, &DevicePluginDenon::onAvrConnectionChanged);
        connect(denonConnection, &AvrConnection::socketErrorOccured, this, &DevicePluginDenon::onAvrSocketError);
        connect(denonConnection, &AvrConnection::channelChanged, this, &DevicePluginDenon::onAvrChannelChanged);
        connect(denonConnection, &AvrConnection::powerChanged, this, &DevicePluginDenon::onAvrPowerChanged);
        connect(denonConnection, &AvrConnection::volumeChanged, this, &DevicePluginDenon::onAvrVolumeChanged);
        connect(denonConnection, &AvrConnection::surroundModeChanged, this, &DevicePluginDenon::onAvrSurroundModeChanged);
        connect(denonConnection, &AvrConnection::muteChanged, this, &DevicePluginDenon::onAvrMuteChanged);

        m_avrConnections.insert(device, denonConnection);
        m_asyncAvrSetups.insert(denonConnection, info);
        // In case the setup is cancelled before we finish it...
        connect(info, &QObject::destroyed, this, [this, info, denonConnection]() { m_asyncAvrSetups.remove(denonConnection); });

        denonConnection->connectDevice();
        return;
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

        m_heos.insert(device, heos);
        m_asyncHeosSetups.insert(heos, info);
        // In case the setup is cancelled before we finish it...
        connect(info, &QObject::destroyed, this, [this, info, heos]() { m_asyncHeosSetups.remove(heos); });

        heos->connectHeos();
        return;
    }

    if (device->deviceClassId() == heosPlayerDeviceClassId) {
        info->finish(Device::DeviceErrorNoError);
        return;
    }
    info->finish(Device::DeviceErrorDeviceClassNotFound);
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

void DevicePluginDenon::executeAction(DeviceActionInfo *info)
{
    Device *device = info->device();
    Action action = info->action();

    qCDebug(dcDenon) << "Execute action" << device->id() << action.id() << action.params();
    if (device->deviceClassId() == AVRX1000DeviceClassId) {
        AvrConnection *avrConnection = m_avrConnections.value(device);

        if (action.actionTypeId() == AVRX1000PowerActionTypeId) {

            bool power = action.param(AVRX1000PowerActionPowerParamTypeId).value().toBool();
            avrConnection->setPower(power);
            return info->finish(Device::DeviceErrorNoError);

        } else if (action.actionTypeId() == AVRX1000VolumeActionTypeId) {

            int vol = action.param(AVRX1000VolumeActionVolumeParamTypeId).value().toInt();
            avrConnection->setVolume(vol);
            return info->finish(Device::DeviceErrorNoError);

        } else if (action.actionTypeId() == AVRX1000ChannelActionTypeId) {

            qCDebug(dcDenon) << "Execute update action" << action.id();
            QByteArray channel = action.param(AVRX1000ChannelActionChannelParamTypeId).value().toByteArray();
            avrConnection->setChannel(channel);
            return info->finish(Device::DeviceErrorNoError);

        } else if (action.actionTypeId() == AVRX1000IncreaseVolumeActionTypeId) {

            avrConnection->increaseVolume();
            return info->finish(Device::DeviceErrorNoError);

        } else if (action.actionTypeId() == AVRX1000DecreaseVolumeActionTypeId) {

            avrConnection->decreaseVolume();
            return info->finish(Device::DeviceErrorNoError);

        } else if (action.actionTypeId() == AVRX1000SurroundModeActionTypeId) {

            QByteArray surroundMode = action.param(AVRX1000SurroundModeActionSurroundModeParamTypeId).value().toByteArray();
            avrConnection->setSurroundMode(surroundMode);
            return info->finish(Device::DeviceErrorNoError);
        }
        return info->finish(Device::DeviceErrorActionTypeNotFound);
    }

    if (device->deviceClassId() == heosPlayerDeviceClassId) {

        Device *heosDevice = myDevices().findById(device->parentId());
        Heos *heos = m_heos.value(heosDevice);
        int playerId = device->paramValue(heosPlayerDevicePlayerIdParamTypeId).toInt();

        if (action.actionTypeId() == heosPlayerVolumeActionTypeId) {
            int volume = action.param(heosPlayerVolumeActionVolumeParamTypeId).value().toInt();
            heos->setVolume(playerId, volume);
            return info->finish(Device::DeviceErrorNoError);
        }

        if (action.actionTypeId() == heosPlayerMuteActionTypeId) {
            bool mute = action.param(heosPlayerMuteActionMuteParamTypeId).value().toBool();
            heos->setMute(playerId, mute);
            return info->finish(Device::DeviceErrorNoError);
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
            return info->finish(Device::DeviceErrorNoError);
        }

        if (action.actionTypeId() == heosPlayerShuffleActionTypeId) {
            bool shuffle = action.param(heosPlayerShuffleActionShuffleParamTypeId).value().toBool();
            REPEAT_MODE repeatMode = REPEAT_MODE_OFF;
            if (device->stateValue(heosPlayerRepeatStateTypeId) == "One") {
                repeatMode = REPEAT_MODE_ONE;
            } else if (device->stateValue(heosPlayerRepeatStateTypeId) == "All") {
                repeatMode = REPEAT_MODE_ALL;
            }
            heos->setPlayMode(playerId, repeatMode, shuffle);
            return info->finish(Device::DeviceErrorNoError);
        }

        if (action.actionTypeId() == heosPlayerSkipBackActionTypeId) {
            heos->playPrevious(playerId);
            return info->finish(Device::DeviceErrorNoError);
        }

        if (action.actionTypeId() == heosPlayerStopActionTypeId) {
            heos->setPlayerState(playerId, PLAYER_STATE_STOP);
            return info->finish(Device::DeviceErrorNoError);
        }

        if (action.actionTypeId() == heosPlayerPlayActionTypeId) {
            heos->setPlayerState(playerId, PLAYER_STATE_PLAY);
            return info->finish(Device::DeviceErrorNoError);
        }

        if (action.actionTypeId() == heosPlayerPauseActionTypeId) {
            heos->setPlayerState(playerId, PLAYER_STATE_PAUSE);
            return info->finish(Device::DeviceErrorNoError);
        }

        if (action.actionTypeId() == heosPlayerSkipNextActionTypeId) {
            heos->playNext(playerId);
            return info->finish(Device::DeviceErrorNoError);
        }
        return info->finish(Device::DeviceErrorActionTypeNotFound);
    }
    return info->finish(Device::DeviceErrorDeviceClassNotFound);
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

    if(!m_pluginTimer) {
        m_pluginTimer = hardwareManager()->pluginTimerManager()->registerTimer(60);
        connect(m_pluginTimer, &PluginTimer::timeout, this, &DevicePluginDenon::onPluginTimer);
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
                DeviceSetupInfo *info = m_asyncAvrSetups.take(denonConnection);
                info->finish(Device::DeviceErrorNoError);
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
            DeviceSetupInfo *info = m_asyncAvrSetups.take(denonConnection);
            qCWarning(dcDenon()) << "Could not add device. The setup failed.";
            info->finish(Device::DeviceErrorHardwareFailure);
            // Delete the connection, the device will not be added and
            // the connection will be created in the next setup
            denonConnection->deleteLater();
            m_avrConnections.remove(device);
        }
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
                DeviceSetupInfo *info = m_asyncHeosSetups.take(heos);
                heos->getPlayers();
                info->finish(Device::DeviceErrorNoError);
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
    autoDevicesAppeared(heosPlayerDescriptors);
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
