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
    m_connectedStateTypesMap[shellySwitchDeviceClassId] = shellySwitchConnectedStateTypeId;
    m_connectedStateTypesMap[shellyGenericDeviceClassId] = shellyGenericConnectedStateTypeId;
    m_connectedStateTypesMap[shellyLightDeviceClassId] = shellyLightConnectedStateTypeId;

    m_powerActionTypesMap[shellyGenericPowerActionTypeId] = shellyGenericDeviceClassId;
    m_powerActionTypesMap[shellyLightPowerActionTypeId] = shellyLightDeviceClassId;

    m_powerActionParamTypesMap[shellyGenericPowerActionTypeId] = shellyGenericPowerActionPowerParamTypeId;
    m_powerActionParamTypesMap[shellyLightPowerActionTypeId] = shellyLightPowerActionPowerParamTypeId;

    m_powerStateTypeMap[shellyGenericDeviceClassId] = shellyGenericPowerStateTypeId;
    m_powerStateTypeMap[shellyLightDeviceClassId] = shellyLightPowerStateTypeId;
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
        QRegExp namePattern;
        if (info->deviceClassId() == shelly1DeviceClassId) {
            namePattern = QRegExp("^shelly(1|1pm|plug|plug-s)-[0-9A-Z]+$");
        }
        if (!entry.name().contains(namePattern)) {
            continue;
        }

        DeviceDescriptor descriptor(shelly1DeviceClassId, entry.name(), entry.hostAddress().toString());
        ParamList params;
        params << Param(shelly1DeviceIdParamTypeId, entry.name());
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

    if (device->deviceClassId() == shelly1DeviceClassId) {
        setupShellyGateway(info);
        return;
    }

    setupShellyChild(info);
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

    if (action.actionTypeId() == shelly1RebootActionTypeId) {
        QUrl url;
        url.setScheme("http");
        url.setHost(getIP(info->device()));
        url.setPath("/reboot");
        url.setUserName(device->paramValue(shelly1DeviceUsernameParamTypeId).toString());
        url.setPassword(device->paramValue(shelly1DevicePasswordParamTypeId).toString());
        QNetworkReply *reply = hardwareManager()->networkManager()->get(QNetworkRequest(url));
        connect(reply, &QNetworkReply::finished, &QNetworkReply::deleteLater);
        connect(reply, &QNetworkReply::finished, info, [info, reply](){
            info->finish(reply->error() == QNetworkReply::NoError ? Device::DeviceErrorNoError : Device::DeviceErrorHardwareFailure);
        });
        return;
    }

    if (m_powerActionTypesMap.contains(action.actionTypeId())) {
        Device *parentDevice = myDevices().findById(device->parentId());
        MqttChannel *channel = m_mqttChannels.value(parentDevice);
        QString shellyId = parentDevice->paramValue(shelly1DeviceIdParamTypeId).toString();
        ParamTypeId powerParamTypeId = m_powerActionParamTypesMap.value(action.actionTypeId());
        bool on = action.param(powerParamTypeId).value().toBool();
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
    device->setStateValue(shelly1ConnectedStateTypeId, true);

    foreach (Device *child, myDevices().filterByParentDeviceId(device->id())) {
        child->setStateValue(m_connectedStateTypesMap[child->deviceClassId()], true);
    }
}

void DevicePluginShelly::onClientDisconnected(MqttChannel *channel)
{
    Device *device = m_mqttChannels.key(channel);
    if (!device) {
        qCWarning(dcShelly()) << "Received a client disconnect for a device we don't know!";
        return;
    }
    device->setStateValue(shelly1ConnectedStateTypeId, false);

    foreach (Device *child, myDevices().filterByParentDeviceId(device->id())) {
        child->setStateValue(m_connectedStateTypesMap[child->deviceClassId()], false);
    }
}

void DevicePluginShelly::onPublishReceived(MqttChannel *channel, const QString &topic, const QByteArray &payload)
{
    Device *device = m_mqttChannels.key(channel);
    if (!device) {
        qCWarning(dcShelly()) << "Received a publish message for a device we don't know!";
        return;
    }

    QString shellyId = device->paramValue(shelly1DeviceIdParamTypeId).toString();
    if (topic == "shellies/" + shellyId + "/input/0") {
        // "1" or "0"
        // Emit event button pressed
        bool on = payload == "1";
        foreach (Device *child, myDevices().filterByParentDeviceId(device->id())) {
            if (child->deviceClassId() == shellySwitchDeviceClassId) {
                if (child->stateValue(shellySwitchPowerStateTypeId).toBool() != on) {
                    child->setStateValue(shellySwitchPowerStateTypeId, on);
                    emit emitEvent(Event(shellySwitchPressedEventTypeId, child->id()));
                }
            }
        }
    }

    if (topic == "shellies/" + shellyId + "/relay/0") {
        bool on = payload == "on";

        foreach (Device *child, myDevices().filterByParentDeviceId(device->id())) {
            if (m_powerStateTypeMap.contains(child->deviceClassId())) {
                child->setStateValue(m_powerStateTypeMap.value(child->deviceClassId()), on);
            }
        }
    }
    qCDebug(dcShelly()) << "Publish received from" << device->name() << topic << payload;
}

void DevicePluginShelly::setupShellyGateway(DeviceSetupInfo *info)
{
    QString shellyId = info->device()->paramValue(shelly1DeviceIdParamTypeId).toString();
    ZeroConfServiceEntry zeroConfEntry;
    foreach (const ZeroConfServiceEntry &entry, m_zeroconfBrowser->serviceEntries()) {
        if (entry.name() == shellyId) {
            zeroConfEntry = entry;
        }
    }
    QHostAddress address;
    pluginStorage()->beginGroup(info->device()->id().toString());
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
    url.setUserName(info->device()->paramValue(shelly1DeviceUsernameParamTypeId).toString());
    url.setPassword(info->device()->paramValue(shelly1DevicePasswordParamTypeId).toString());

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
    connect(reply, &QNetworkReply::finished, info, [this, info, reply, channel, address](){
        if (reply->error() != QNetworkReply::NoError) {
            hardwareManager()->mqttProvider()->releaseChannel(channel);
            qCWarning(dcShelly()) << "Error fetching device settings" << reply->error() << reply->errorString();
            if (reply->error() == QNetworkReply::AuthenticationRequiredError) {
                info->finish(Device::DeviceErrorAuthenticationFailure, QT_TR_NOOP("Username and password not set correctly."));
            } else {
                info->finish(Device::DeviceErrorHardwareFailure, QT_TR_NOOP("Error connecting to Shelly device."));
            }
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

        DeviceDescriptors autoChilds;

        // Always create the switch device if we don't have one yet
        if (myDevices().filterByParentDeviceId(info->device()->id()).filterByDeviceClassId(shellySwitchDeviceClassId).isEmpty()) {
            DeviceDescriptor switchChild(shellySwitchDeviceClassId, "Shelly switch", QString(), info->device()->id());
            autoChilds.append(switchChild);
        }

        // Add connected devices as configured in params
        if (info->device()->paramValue(shelly1DeviceConnectedDeviceParamTypeId).toString() == "Generic") {
            if (myDevices().filterByParentDeviceId(info->device()->id()).filterByDeviceClassId(shellyGenericDeviceClassId).isEmpty()) {
                DeviceDescriptor genericChild(shellyGenericDeviceClassId, "Shelly connected device", QString(), info->device()->id());
                autoChilds.append(genericChild);
            }
        }
        if (info->device()->paramValue(shelly1DeviceConnectedDeviceParamTypeId).toString() == "Light") {
            if (myDevices().filterByParentDeviceId(info->device()->id()).filterByDeviceClassId(shellyLightDeviceClassId).isEmpty()) {
                DeviceDescriptor genericChild(shellyLightDeviceClassId, "Shelly connected light", QString(), info->device()->id());
                autoChilds.append(genericChild);
            }
        }

        info->finish(Device::DeviceErrorNoError);

        emit autoDevicesAppeared(autoChilds);

        // Make sure authentication is enalbed if the user wants it
        QString username = info->device()->paramValue(shelly1DeviceUsernameParamTypeId).toString();
        QString password = info->device()->paramValue(shelly1DevicePasswordParamTypeId).toString();
        if (!username.isEmpty()) {
            QUrl url;
            url.setScheme("http");
            url.setHost(address.toString());
            url.setPort(80);
            url.setPath("/settings/login");
            url.setUserName(username);
            url.setPassword(password);

            QUrlQuery query;
            query.addQueryItem("username", username);
            query.addQueryItem("password", password);
            query.addQueryItem("enabled", "true");

            url.setQuery(query);

            QNetworkRequest request(url);
            qCDebug(dcShelly()) << "Enabling auth" << username << password;
            QNetworkReply *reply = hardwareManager()->networkManager()->get(request);
            connect(reply, &QNetworkReply::finished, reply, &QNetworkReply::deleteLater);
        }
    });
}

void DevicePluginShelly::setupShellyChild(DeviceSetupInfo *info)
{
    Device *device = info->device();

    // Connect to settings changes to store them to the device
    connect(info->device(), &Device::settingChanged, this, [this, device](const ParamTypeId &paramTypeId, const QVariant &value){
        Device *parentDevice = myDevices().findById(device->parentId());
        pluginStorage()->beginGroup(parentDevice->id().toString());
        QString address = pluginStorage()->value("cachedAddress").toString();
        pluginStorage()->endGroup();

        QUrl url;
        url.setScheme("http");
        url.setHost(address);
        url.setPort(80);
        url.setPath("/settings/relay/0");
        url.setUserName(parentDevice->paramValue(shelly1DeviceUsernameParamTypeId).toString());
        url.setPassword(parentDevice->paramValue(shelly1DevicePasswordParamTypeId).toString());

        QUrlQuery query;
        if (paramTypeId == shellySwitchSettingsButtonTypeParamTypeId) {
            query.addQueryItem("btn_type", value.toString());
        }
        if (paramTypeId == shellySwitchSettingsInvertButtonParamTypeId) {
            query.addQueryItem("btn_reverse", value.toBool() ? "1" : "0");
        }
        if (paramTypeId == shellyGenericSettingsDefaultStateParamTypeId || paramTypeId == shellyLightSettingsDefaultStateParamTypeId) {
            query.addQueryItem("default_state", value.toString());
        }

        url.setQuery(query);

        QNetworkReply *reply = hardwareManager()->networkManager()->get(QNetworkRequest(url));
        connect(reply, &QNetworkReply::finished, reply, &QNetworkReply::deleteLater);
    });

    info->finish(Device::DeviceErrorNoError);
}

QString DevicePluginShelly::getIP(Device *device) const
{
    Device *d = device;
    if (!device->parentId().isNull()) {
        d = myDevices().findById(device->parentId());
    }
    pluginStorage()->beginGroup(d->id().toString());
    QString ip = pluginStorage()->value("cachedAddress").toString();
    pluginStorage()->endGroup();
    return ip;
}
