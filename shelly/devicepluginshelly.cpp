/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *                                                                         *
 *  Copyright (C) 2019 Michael Zanetti <michael.zanetti@nymea.io>          *
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

#include "devicepluginshelly.h"
#include "plugininfo.h"

#include <QUrlQuery>
#include <QNetworkReply>
#include <QHostAddress>
#include <QJsonDocument>

#include "hardwaremanager.h"
#include "network/networkaccessmanager.h"
#include "network/mqtt/mqttprovider.h"
#include "network/mqtt/mqttchannel.h"

#include "network/zeroconf/zeroconfservicebrowser.h"
#include "platform/platformzeroconfcontroller.h"

DevicePluginShelly::DevicePluginShelly()
{
}

DevicePluginShelly::~DevicePluginShelly()
{
}

void DevicePluginShelly::init()
{
    m_zeroconfBrowser = hardwareManager()->zeroConfController()->createServiceBrowser("_http._tcp");
}

void DevicePluginShelly::discoverDevices(DeviceDiscoveryInfo *info)
{
    foreach (const ZeroConfServiceEntry &entry, m_zeroconfBrowser->serviceEntries()) {
//        qCDebug(dcShelly()) << "Have entry" << entry;
        QRegExp namePattern("^shelly[1-2]-[0-9A-Z]+$");
        if (!entry.name().contains(namePattern)) {
            continue;
        }

        DeviceDescriptor descriptor(shellyOneDeviceClassId, entry.name(), entry.hostAddress().toString());
        ParamList params;
        params << Param(shellyOneDeviceIdParamTypeId, entry.name());
        descriptor.setParams(params);

        Device *existingDevice = myDevices().findByParams(params);
        if (existingDevice) {
            descriptor.setDeviceId(existingDevice->id());
        }

        info->addDeviceDescriptor(descriptor);
        qCDebug(dcShelly()) << "Found shelly device!" << entry;

    }

    info->finish(Device::DeviceErrorNoError);
}

void DevicePluginShelly::setupDevice(DeviceSetupInfo *info)
{
    Device *device = info->device();

    if (device->deviceClassId() == shellyOneDeviceClassId) {
        QString shellyId = device->paramValue(shellyOneDeviceIdParamTypeId).toString();
        ZeroConfServiceEntry zeroConfEntry;
        foreach (const ZeroConfServiceEntry &entry, m_zeroconfBrowser->serviceEntries()) {
            if (entry.name() == shellyId) {
                zeroConfEntry = entry;
            }
        }
        QHostAddress address;
        pluginStorage()->beginGroup(device->id().toString());
        if (zeroConfEntry.isValid()) {
            address = zeroConfEntry.hostAddress().toString();
            pluginStorage()->setValue("cachedAddress", address.toString());
        } else {
            qCWarning(dcShelly()) << "Could not find Shelly device on zeroconf. Trying cached address.";
            address = pluginStorage()->value("cachedAddress").toString();
        }
        pluginStorage()->endGroup();

        if (address.isNull()) {
            qCWarning(dcShelly()) << "Unable to determine Shelly's network address. Failed to set up device.";
            info->finish(Device::DeviceErrorHardwareNotAvailable, QT_TR_NOOP("Unable to find the device in the network."));
            return;
        }

        MqttChannel *channel = hardwareManager()->mqttProvider()->createChannel(shellyId, QHostAddress(address), {"shellies"});
        if (!channel) {
            qCWarning(dcShelly()) << "Failed to create MQTT channel.";
            return info->finish(Device::DeviceErrorHardwareFailure, QT_TR_NOOP("Error creating MQTT channel. Please check MQTT server settings."));
        }

        QUrl url;
        url.setScheme("http");
        url.setHost(address.toString());
        url.setPort(80);
        url.setPath("/settings");

        QUrlQuery query;
        query.addQueryItem("mqtt_server", channel->serverAddress().toString() + ":" + QString::number(channel->serverPort()));
        query.addQueryItem("mqtt_user", channel->username());
        query.addQueryItem("mqtt_pass", channel->password());
        query.addQueryItem("mqtt_enable", "true");

        url.setQuery(query);

        QNetworkRequest request(url);
        qCDebug(dcShelly()) << "Connecting to" << url.toString();
        QNetworkReply *reply = hardwareManager()->networkManager()->get(request);
        connect(reply, &QNetworkReply::finished, reply, &QNetworkReply::deleteLater);
        connect(info, &DeviceSetupInfo::aborted, channel, [this, channel](){
            hardwareManager()->mqttProvider()->releaseChannel(channel);
        });
        connect(reply, &QNetworkReply::finished, info, [this, info, reply, channel](){
            if (reply->error() != QNetworkReply::NoError) {
                qCWarning(dcShelly()) << "Error fetching device settings" << reply->error() << reply->errorString();
                info->finish(Device::DeviceErrorHardwareFailure, QT_TR_NOOP("Error connecting to Shelly device."));
                hardwareManager()->mqttProvider()->releaseChannel(channel);
                return;
            }
            QByteArray data = reply->readAll();
            QJsonParseError error;
            QJsonDocument jsonDoc = QJsonDocument::fromJson(data, &error);
            if (error.error != QJsonParseError::NoError) {
                qCWarning(dcShelly()) << "Error parsing settings reply" << error.errorString() << "\n" << data;
                info->finish(Device::DeviceErrorHardwareFailure, QT_TR_NOOP("Unexpected data received from Shelly device."));
                hardwareManager()->mqttProvider()->releaseChannel(channel);
                return;
            }
            qCDebug(dcShelly()) << "Settings data" << qUtf8Printable(jsonDoc.toJson(QJsonDocument::Indented));

            m_mqttChannels.insert(info->device(), channel);
            connect(channel, &MqttChannel::clientConnected, this, &DevicePluginShelly::onClientConnected);
            connect(channel, &MqttChannel::clientDisconnected, this, &DevicePluginShelly::onClientDisconnected);
            connect(channel, &MqttChannel::publishReceived, this, &DevicePluginShelly::onPublishReceived);

            info->finish(Device::DeviceErrorNoError);
        });
        return;
    }

    qCWarning(dcShelly) << "Unhandled DeviceClass in setupDevice" << device->deviceClassId();
}

void DevicePluginShelly::deviceRemoved(Device *device)
{
    if (m_mqttChannels.contains(device)) {
        hardwareManager()->mqttProvider()->releaseChannel(m_mqttChannels.take(device));
    }
    qCDebug(dcShelly()) << "Device removed" << device->name();
}

void DevicePluginShelly::executeAction(DeviceActionInfo *info)
{
    Device *device = info->device();
    Action action = info->action();

    if (action.actionTypeId() == shellyOnePowerActionTypeId) {
        MqttChannel *channel = m_mqttChannels.value(device);
        QString shellyId = device->paramValue(shellyOneDeviceIdParamTypeId).toString();
        bool on = action.param(shellyOnePowerActionPowerParamTypeId).value().toBool();
        channel->publish("shellies/" + shellyId + "/relay/0/command", on ? "on" : "off");
        info->finish(Device::DeviceErrorNoError);
        return;
    }

    qCWarning(dcShelly()) << "Unhandled execute action call for device" << device;
}

void DevicePluginShelly::onClientConnected(MqttChannel *channel)
{
    Device *device = m_mqttChannels.key(channel);
    if (!device) {
        qCWarning(dcShelly()) << "Received a client connect for a device we don't know!";
        return;
    }
    device->setStateValue(shellyOneConnectedStateTypeId, true);
}

void DevicePluginShelly::onClientDisconnected(MqttChannel *channel)
{
    Device *device = m_mqttChannels.key(channel);
    if (!device) {
        qCWarning(dcShelly()) << "Received a client disconnect for a device we don't know!";
        return;
    }
    device->setStateValue(shellyOneConnectedStateTypeId, false);
}

void DevicePluginShelly::onPublishReceived(MqttChannel *channel, const QString &topic, const QByteArray &payload)
{
    Device *device = m_mqttChannels.key(channel);
    if (!device) {
        qCWarning(dcShelly()) << "Received a publish message for a device we don't know!";
        return;
    }

    QString shellyId = device->paramValue(shellyOneDeviceIdParamTypeId).toString();
    if (topic == "shellies/" + shellyId + "/input/0") {
        // "1" or "0"
        // Emit event button pressed
    }

    if (topic == "shellies/" + shellyId + "/relay/0") {
        bool on = payload == "on";
        device->setStateValue(shellyOnePowerStateTypeId, on);
    }
    qCDebug(dcShelly()) << "Publish received from" << device->name() << topic << payload;

}
