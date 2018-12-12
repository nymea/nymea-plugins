/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *                                                                         *
 *  Copyright (C) 2018 Michael Zanetti <michael.zanetti@nymea.io>          *
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
    \page tasmota.html
    \title Tasmota (Sonoff) devices
    \brief Plugin for Tasmota devices (Sonoff devices flashed with Tasmota and others)

    \ingroup plugins
    \ingroup nymea-plugins-maker

    This plugin allows to make use of Sonoff-Tasmota devices via the nymea internal MQTT broker.
    There is no external MQTT broker needed.

    Note that Sonoff devices must be flashed with the Tasmota sofware and connected to the WiFi network in order to
    work with this plugin.

    See https://github.com/arendst/Sonoff-Tasmota/wiki for a list of all supported devices and instructions on how to
    install Tasmota on those.

    After flashing Tasmota to a Sonoff device and connecting it to WiFi, it can be added to nymea. The only required
    thing is the IP address to the device. This plugin will create a new isoloated MQTT channel on the nymea internal
    MQTT broker and provision login details to the Tasmota device via HTTP. Once that is successful, the Tasmota device
    will connect to the MQTT broker and appear as connected in nymea.

    \chapter Plugin properties
    When adding a Tasmota device it will add a new Gateway type device representing the Tasmota device itself. In addition
    to that a power switch device will appear which can be used to control the switches in the Tasmota device. Upon
    device setup, the user can optionally select the type of the connected hardware, (e.g. a light) which causes this
    plugin to create a light device in the system which also controls the switches inside the Tasmota device and nicely
    integrates with the nymea:ux for the given device type.

    For more details how to read this JSON file please check out the documentation for \l{The plugin JSON File}.

    \quotefile plugins/deviceplugins/tasmota/deviceplugintasmota.json
*/

#include "deviceplugintasmota.h"
#include "plugininfo.h"

#include <QUrlQuery>
#include <QNetworkReply>
#include <QHostAddress>
#include <QJsonDocument>

#include "hardwaremanager.h"
#include "network/networkaccessmanager.h"
#include "network/mqtt/mqttprovider.h"
#include "network/mqtt/mqttchannel.h"

DevicePluginTasmota::DevicePluginTasmota()
{
    // Helper maps for parent devices (aka sonoff_*)
    m_ipAddressParamTypeMap[sonoff_basicDeviceClassId] = sonoff_basicDeviceIpAddressParamTypeId;
    m_ipAddressParamTypeMap[sonoff_dualDeviceClassId] = sonoff_dualDeviceIpAddressParamTypeId;
    m_ipAddressParamTypeMap[sonoff_quadDeviceClassId] = sonoff_quadDeviceIpAddressParamTypeId;

    m_attachedDeviceParamTypeIdMap[sonoff_basicDeviceClassId] << sonoff_basicDeviceAttachedDeviceCH1ParamTypeId;
    m_attachedDeviceParamTypeIdMap[sonoff_dualDeviceClassId] << sonoff_dualDeviceAttachedDeviceCH1ParamTypeId << sonoff_dualDeviceAttachedDeviceCH2ParamTypeId;
    m_attachedDeviceParamTypeIdMap[sonoff_quadDeviceClassId] << sonoff_quadDeviceAttachedDeviceCH1ParamTypeId << sonoff_quadDeviceAttachedDeviceCH2ParamTypeId << sonoff_quadDeviceAttachedDeviceCH3ParamTypeId << sonoff_quadDeviceAttachedDeviceCH4ParamTypeId;

    // Helper maps for virtual childs (aka tasmota*)
    m_channelParamTypeMap[tasmotaSwitchDeviceClassId] = tasmotaSwitchDeviceChannelNameParamTypeId;
    m_channelParamTypeMap[tasmotaLightDeviceClassId] = tasmotaLightDeviceChannelNameParamTypeId;

    m_powerStateTypeMap[tasmotaSwitchDeviceClassId] = tasmotaSwitchPowerStateTypeId;
    m_powerStateTypeMap[tasmotaLightDeviceClassId] = tasmotaLightPowerStateTypeId;

    // Helper maps for all devices
    m_connectedStateTypeMap[sonoff_basicDeviceClassId] = sonoff_basicConnectedStateTypeId;
    m_connectedStateTypeMap[sonoff_dualDeviceClassId] = sonoff_dualConnectedStateTypeId;
    m_connectedStateTypeMap[sonoff_quadDeviceClassId] = sonoff_quadConnectedStateTypeId;
    m_connectedStateTypeMap[tasmotaSwitchDeviceClassId] = tasmotaSwitchConnectedStateTypeId;
    m_connectedStateTypeMap[tasmotaLightDeviceClassId] = tasmotaLightConnectedStateTypeId;
}

DevicePluginTasmota::~DevicePluginTasmota()
{
}

void DevicePluginTasmota::init()
{
}

DeviceManager::DeviceSetupStatus DevicePluginTasmota::setupDevice(Device *device)
{
    if (m_ipAddressParamTypeMap.contains(device->deviceClassId())) {
        ParamTypeId ipAddressParamTypeId = m_ipAddressParamTypeMap.value(device->deviceClassId());

        QHostAddress deviceAddress = QHostAddress(device->paramValue(ipAddressParamTypeId).toString());
        if (deviceAddress.isNull()) {
            qCWarning(dcTasmota) << "Not a valid IP address given for IP address parameter";
            return DeviceManager::DeviceSetupStatusFailure;
        }
        MqttChannel *channel = hardwareManager()->mqttProvider()->createChannel(device->id(), deviceAddress);
        if (!channel) {
            qCWarning(dcTasmota) << "Failed to create MQTT channel.";
            return DeviceManager::DeviceSetupStatusFailure;
        }

        QUrl url("http://10.10.10.90/sv");
        QUrlQuery query;
        query.addQueryItem("w", "2%2C1");
        query.addQueryItem("mh", channel->serverAddress().toString());
        query.addQueryItem("ml", QString::number(channel->serverPort()));
        query.addQueryItem("mc", channel->clientId());
        query.addQueryItem("mu", channel->username());
        query.addQueryItem("mp", channel->password());
        query.addQueryItem("mt", "sonoff");
        query.addQueryItem("mf", channel->topicPrefix() + "/%topic%/");
        url.setQuery(query);
        QNetworkRequest request(url);
        QNetworkReply *reply = hardwareManager()->networkManager()->get(request);
        connect(reply, &QNetworkReply::finished, this, [this, device, channel, reply](){
            reply->deleteLater();
            if (reply->error() != QNetworkReply::NoError) {
                qCDebug(dcTasmota) << "Sonoff device setup call failed:" << reply->error() << reply->errorString() << reply->readAll();
                hardwareManager()->mqttProvider()->releaseChannel(channel);
                emit deviceSetupFinished(device, DeviceManager::DeviceSetupStatusFailure);
                return;
            }
            m_mqttChannels.insert(device, channel);
            connect(channel, &MqttChannel::clientConnected, this, &DevicePluginTasmota::onClientConnected);
            connect(channel, &MqttChannel::clientDisconnected, this, &DevicePluginTasmota::onClientDisconnected);
            connect(channel, &MqttChannel::publishReceived, this, &DevicePluginTasmota::onPublishReceived);

            qCDebug(dcTasmota) << "Sonoff setup complete";
            emit deviceSetupFinished(device, DeviceManager::DeviceSetupStatusSuccess);

            foreach (Device *child, myDevices()) {
                if (child->parentId() == device->id()) {
                    // Already have child devices... We're done here
                    return;
                }
            }
            qCDebug(dcTasmota) << "Adding Tasmota Switch devices";
            QList<DeviceDescriptor> deviceDescriptors;
            for (int i = 0; i < m_attachedDeviceParamTypeIdMap.value(device->deviceClassId()).count(); i++) {
                DeviceDescriptor descriptor(tasmotaSwitchDeviceClassId, device->name() + " CH" + QString::number(i+1), QString(), device->id());
                if (m_attachedDeviceParamTypeIdMap.value(device->deviceClassId()).count() == 1) {
                    descriptor.setParams(ParamList() << Param(tasmotaSwitchDeviceChannelNameParamTypeId, "POWER"));
                } else {
                    descriptor.setParams(ParamList() << Param(tasmotaSwitchDeviceChannelNameParamTypeId, "POWER" + QString::number(i+1)));
                }
                deviceDescriptors << descriptor;
            }
            emit autoDevicesAppeared(tasmotaSwitchDeviceClassId, deviceDescriptors);

            qCDebug(dcTasmota) << "Adding Tasmota connected devices";
            deviceDescriptors.clear();
            for (int i = 0; i < m_attachedDeviceParamTypeIdMap.value(device->deviceClassId()).count(); i++) {
                ParamTypeId attachedDeviceParamTypeId = m_attachedDeviceParamTypeIdMap.value(device->deviceClassId()).at(i);
                if (device->paramValue(attachedDeviceParamTypeId).toString() == "Light") {
                    DeviceDescriptor descriptor(tasmotaLightDeviceClassId, device->name() + " CH" + QString::number(i+1), QString(), device->id());
                    descriptor.setParentDeviceId(device->id());
                    if (m_attachedDeviceParamTypeIdMap.value(device->deviceClassId()).count() == 1) {
                        descriptor.setParams(ParamList() << Param(tasmotaLightDeviceChannelNameParamTypeId, "POWER"));
                    } else {
                        descriptor.setParams(ParamList() << Param(tasmotaLightDeviceChannelNameParamTypeId, "POWER" + QString::number(i+1)));
                    }
                    deviceDescriptors << descriptor;
                }
            }
            if (!deviceDescriptors.isEmpty()) {
                emit autoDevicesAppeared(tasmotaLightDeviceClassId, deviceDescriptors);
            }
        });
        return DeviceManager::DeviceSetupStatusAsync;
    }

    if (m_connectedStateTypeMap.contains(device->deviceClassId())) {
        Device* parentDevice = myDevices().findById(device->parentId());
        StateTypeId connectedStateTypeId = m_connectedStateTypeMap.value(device->deviceClassId());
        device->setStateValue(m_connectedStateTypeMap.value(device->deviceClassId()), parentDevice->stateValue(connectedStateTypeId));
        return DeviceManager::DeviceSetupStatusSuccess;
    }

    qCWarning(dcTasmota) << "Unhandled DeviceClass in setupDevice" << device->deviceClassId();
    return DeviceManager::DeviceSetupStatusFailure;
}

void DevicePluginTasmota::deviceRemoved(Device *device)
{
    qCDebug(dcTasmota) << "Device removed" << device->name();
    if (m_mqttChannels.contains(device)) {
        qCDebug(dcTasmota) << "Releasing MQTT channel";
        MqttChannel* channel = m_mqttChannels.take(device);
        hardwareManager()->mqttProvider()->releaseChannel(channel);
    }
}

DeviceManager::DeviceError DevicePluginTasmota::executeAction(Device *device, const Action &action)
{
    if (m_powerStateTypeMap.contains(device->deviceClassId())) {
        Device *parentDev = myDevices().findById(device->parentId());
        MqttChannel *channel = m_mqttChannels.value(parentDev);
        ParamTypeId channelParamTypeId = m_channelParamTypeMap.value(device->deviceClassId());
        ParamTypeId powerActionParamTypeId = ParamTypeId(m_powerStateTypeMap.value(device->deviceClassId()).toString());
        qCDebug(dcTasmota) << "Publishing:" << channel->topicPrefix() + "/sonoff/cmnd/" + device->paramValue(channelParamTypeId).toString() << (action.param(powerActionParamTypeId).value().toBool() ? "ON" : "OFF");
        channel->publish(channel->topicPrefix() + "/sonoff/cmnd/" + device->paramValue(channelParamTypeId).toString().toLower(), action.param(powerActionParamTypeId).value().toBool() ? "ON" : "OFF");
        device->setStateValue(m_powerStateTypeMap.value(device->deviceClassId()), action.param(powerActionParamTypeId).value().toBool());
        return DeviceManager::DeviceErrorNoError;
    }
    return DeviceManager::DeviceErrorDeviceClassNotFound;
}

void DevicePluginTasmota::onClientConnected(MqttChannel *channel)
{
    qCDebug(dcTasmota) << "Sonoff device connected!";
    Device *dev = m_mqttChannels.key(channel);
    dev->setStateValue(m_connectedStateTypeMap.value(dev->deviceClassId()), true);

    foreach (Device *child, myDevices()) {
        if (child->parentId() == dev->id()) {
            child->setStateValue(m_connectedStateTypeMap.value(child->deviceClassId()), true);
        }
    }
}

void DevicePluginTasmota::onClientDisconnected(MqttChannel *channel)
{
    qCDebug(dcTasmota) << "Sonoff device disconnected!";
    Device *dev = m_mqttChannels.key(channel);
    dev->setStateValue(m_connectedStateTypeMap.value(dev->deviceClassId()), false);

    foreach (Device *child, myDevices()) {
        if (child->parentId() == dev->id()) {
            child->setStateValue(m_connectedStateTypeMap.value(child->deviceClassId()), false);
        }
    }
}

void DevicePluginTasmota::onPublishReceived(MqttChannel *channel, const QString &topic, const QByteArray &payload)
{
    qCDebug(dcTasmota) << "Publish received from Sonoff device:" << topic << payload;
    Device *dev = m_mqttChannels.key(channel);
    if (m_ipAddressParamTypeMap.contains(dev->deviceClassId())) {
        if (topic.startsWith(channel->topicPrefix() + "/sonoff/POWER")) {
            QString channelName = topic.split("/").last();

            foreach (Device *child, myDevices()) {
                if (child->parentId() != dev->id()) {
                    continue;
                }
                if (child->paramValue(m_channelParamTypeMap.value(child->deviceClassId())).toString() != channelName) {
                    continue;
                }
                child->setStateValue(m_powerStateTypeMap.value(child->deviceClassId()), payload == "ON");
            }
        }
        if (topic.startsWith(channel->topicPrefix() + "/sonoff/STATE")) {
            QJsonParseError error;
            QJsonDocument jsonDoc = QJsonDocument::fromJson(payload, &error);
            if (error.error != QJsonParseError::NoError) {
                qCWarning(dcTasmota) << "Cannot parse JSON from Tasmota device" << error.errorString();
                return;
            }
            foreach (Device *child, myDevices()) {
                if (child->parentId() != dev->id()) {
                    continue;
                }
                QString childChannel = child->paramValue(m_channelParamTypeMap.value(child->deviceClassId())).toString();
                QString valueString = jsonDoc.toVariant().toMap().value(childChannel).toString();
                child->setStateValue(m_powerStateTypeMap.value(child->deviceClassId()), valueString == "ON");
            }

        }
    }
}

