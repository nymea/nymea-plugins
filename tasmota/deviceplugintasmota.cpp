/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*
* Copyright 2013 - 2020, nymea GmbH
* Contact: contact@nymea.io
*
* This file is part of nymea.
* This project including source code and documentation is protected by
* copyright law, and remains the property of nymea GmbH. All rights, including
* reproduction, publication, editing and translation, are reserved. The use of
* this project is subject to the terms of a license agreement to be concluded
* with nymea GmbH in accordance with the terms of use of nymea GmbH, available
* under https://nymea.io/license
*
* GNU Lesser General Public License Usage
* Alternatively, this project may be redistributed and/or modified under the
* terms of the GNU Lesser General Public License as published by the Free
* Software Foundation; version 3. This project is distributed in the hope that
* it will be useful, but WITHOUT ANY WARRANTY; without even the implied
* warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
* Lesser General Public License for more details.
*
* You should have received a copy of the GNU Lesser General Public License
* along with this project. If not, see <https://www.gnu.org/licenses/>.
*
* For any further details and any questions please contact us under
* contact@nymea.io or see our FAQ/Licensing Information on
* https://nymea.io/license/faq
*
* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

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
    m_ipAddressParamTypeMap[sonoff_triDeviceClassId] = sonoff_triDeviceIpAddressParamTypeId;
    m_ipAddressParamTypeMap[sonoff_quadDeviceClassId] = sonoff_quadDeviceIpAddressParamTypeId;

    m_attachedDeviceParamTypeIdMap[sonoff_basicDeviceClassId] << sonoff_basicDeviceAttachedDeviceCH1ParamTypeId;
    m_attachedDeviceParamTypeIdMap[sonoff_dualDeviceClassId] << sonoff_dualDeviceAttachedDeviceCH1ParamTypeId << sonoff_dualDeviceAttachedDeviceCH2ParamTypeId;
    m_attachedDeviceParamTypeIdMap[sonoff_triDeviceClassId] << sonoff_triDeviceAttachedDeviceCH1ParamTypeId << sonoff_triDeviceAttachedDeviceCH2ParamTypeId << sonoff_triDeviceAttachedDeviceCH3ParamTypeId;
    m_attachedDeviceParamTypeIdMap[sonoff_quadDeviceClassId] << sonoff_quadDeviceAttachedDeviceCH1ParamTypeId << sonoff_quadDeviceAttachedDeviceCH2ParamTypeId << sonoff_quadDeviceAttachedDeviceCH3ParamTypeId << sonoff_quadDeviceAttachedDeviceCH4ParamTypeId;

    // Helper maps for virtual childs (aka tasmota*)
    m_channelParamTypeMap[tasmotaSwitchDeviceClassId] = tasmotaSwitchDeviceChannelNameParamTypeId;
    m_channelParamTypeMap[tasmotaLightDeviceClassId] = tasmotaLightDeviceChannelNameParamTypeId;
    m_openingChannelParamTypeMap[tasmotaShutterDeviceClassId] = tasmotaShutterDeviceOpeningChannelParamTypeId;
    m_closingChannelParamTypeMap[tasmotaShutterDeviceClassId] = tasmotaShutterDeviceClosingChannelParamTypeId;
    m_openingChannelParamTypeMap[tasmotaBlindsDeviceClassId] = tasmotaBlindsDeviceOpeningChannelParamTypeId;
    m_closingChannelParamTypeMap[tasmotaBlindsDeviceClassId] = tasmotaBlindsDeviceClosingChannelParamTypeId;

    m_powerStateTypeMap[tasmotaSwitchDeviceClassId] = tasmotaSwitchPowerStateTypeId;
    m_powerStateTypeMap[tasmotaLightDeviceClassId] = tasmotaLightPowerStateTypeId;

    m_closableOpenActionTypeMap[tasmotaShutterDeviceClassId] = tasmotaShutterOpenActionTypeId;
    m_closableCloseActionTypeMap[tasmotaShutterDeviceClassId] = tasmotaShutterCloseActionTypeId;
    m_closableStopActionTypeMap[tasmotaShutterDeviceClassId] = tasmotaShutterStopActionTypeId;

    m_closableOpenActionTypeMap[tasmotaBlindsDeviceClassId] = tasmotaBlindsOpenActionTypeId;
    m_closableCloseActionTypeMap[tasmotaBlindsDeviceClassId] = tasmotaBlindsCloseActionTypeId;
    m_closableStopActionTypeMap[tasmotaBlindsDeviceClassId] = tasmotaBlindsStopActionTypeId;

    // Helper maps for all devices
    m_connectedStateTypeMap[sonoff_basicDeviceClassId] = sonoff_basicConnectedStateTypeId;
    m_connectedStateTypeMap[sonoff_dualDeviceClassId] = sonoff_dualConnectedStateTypeId;
    m_connectedStateTypeMap[sonoff_triDeviceClassId] = sonoff_triConnectedStateTypeId;
    m_connectedStateTypeMap[sonoff_quadDeviceClassId] = sonoff_quadConnectedStateTypeId;
    m_connectedStateTypeMap[tasmotaSwitchDeviceClassId] = tasmotaSwitchConnectedStateTypeId;
    m_connectedStateTypeMap[tasmotaLightDeviceClassId] = tasmotaLightConnectedStateTypeId;
    m_connectedStateTypeMap[tasmotaShutterDeviceClassId] = tasmotaShutterConnectedStateTypeId;
    m_connectedStateTypeMap[tasmotaBlindsDeviceClassId] = tasmotaBlindsConnectedStateTypeId;
}

DevicePluginTasmota::~DevicePluginTasmota()
{
}

void DevicePluginTasmota::init()
{
}

void DevicePluginTasmota::setupDevice(DeviceSetupInfo *info)
{
    Device *device = info->device();

    if (m_ipAddressParamTypeMap.contains(device->deviceClassId())) {
        ParamTypeId ipAddressParamTypeId = m_ipAddressParamTypeMap.value(device->deviceClassId());

        QHostAddress deviceAddress = QHostAddress(device->paramValue(ipAddressParamTypeId).toString());
        if (deviceAddress.isNull()) {
            qCWarning(dcTasmota) << "Not a valid IP address given for IP address parameter";
            //: Error setting up device
            return info->finish(Device::DeviceErrorInvalidParameter, QT_TR_NOOP("The given IP address is not valid."));
        }
        MqttChannel *channel = hardwareManager()->mqttProvider()->createChannel(device->id().toString().remove(QRegExp("[{}-]")), deviceAddress);
        if (!channel) {
            qCWarning(dcTasmota) << "Failed to create MQTT channel.";
            //: Error setting up device
            return info->finish(Device::DeviceErrorHardwareFailure, QT_TR_NOOP("Error creating MQTT channel. Please check MQTT server settings."));
        }

        QUrl url(QString("http://%1/cm").arg(deviceAddress.toString()));
        QUrlQuery query;
        QMap<QString, QString> configItems;
        configItems.insert("MqttHost", channel->serverAddress().toString());
        configItems.insert("MqttPort", QString::number(channel->serverPort()));
        configItems.insert("MqttClient", channel->clientId());
        configItems.insert("MqttUser", channel->username());
        configItems.insert("MqttPassword", channel->password());
        configItems.insert("Topic", "sonoff");
        configItems.insert("FullTopic", channel->topicPrefixList().first() + "/%topic%/");

        QStringList configList;
        foreach (const QString &key, configItems.keys()) {
            configList << key + ' ' + configItems.value(key);
        }
        QString fullCommand = "Backlog " + configList.join(';');
        query.addQueryItem("cmnd", fullCommand.toUtf8().toPercentEncoding());


        url.setQuery(query);
        qCDebug(dcTasmota) << "Configuring Tasmota device:" << url.toString();
        QNetworkRequest request(url);
        QNetworkReply *reply = hardwareManager()->networkManager()->get(request);
        connect(reply, &QNetworkReply::finished, reply, &QNetworkReply::deleteLater);
        connect(reply, &QNetworkReply::finished, info, [this, info, channel, reply](){
            if (reply->error() != QNetworkReply::NoError) {
                qCDebug(dcTasmota) << "Sonoff device setup call failed:" << reply->error() << reply->errorString() << reply->readAll();
                hardwareManager()->mqttProvider()->releaseChannel(channel);
                //: Error setting up device
                info->finish(Device::DeviceErrorSetupFailed, QT_TR_NOOP("Could not connect to Tasmota device."));
                return;
            }
            m_mqttChannels.insert(info->device(), channel);
            connect(channel, &MqttChannel::clientConnected, this, &DevicePluginTasmota::onClientConnected);
            connect(channel, &MqttChannel::clientDisconnected, this, &DevicePluginTasmota::onClientDisconnected);
            connect(channel, &MqttChannel::publishReceived, this, &DevicePluginTasmota::onPublishReceived);

            qCDebug(dcTasmota) << "Sonoff setup complete";
            info->finish(Device::DeviceErrorNoError);

            foreach (Device *child, myDevices()) {
                if (child->parentId() == info->device()->id()) {
                    // Already have child devices... We're done here
                    return;
                }
            }
            qCDebug(dcTasmota) << "Adding Tasmota Switch devices";
            QList<DeviceDescriptor> deviceDescriptors;
            for (int i = 0; i < m_attachedDeviceParamTypeIdMap.value(info->device()->deviceClassId()).count(); i++) {
                DeviceDescriptor descriptor(tasmotaSwitchDeviceClassId, info->device()->name() + " CH" + QString::number(i+1), QString(), info->device()->id());
                if (m_attachedDeviceParamTypeIdMap.value(info->device()->deviceClassId()).count() == 1) {
                    descriptor.setParams(ParamList() << Param(tasmotaSwitchDeviceChannelNameParamTypeId, "POWER"));
                } else {
                    descriptor.setParams(ParamList() << Param(tasmotaSwitchDeviceChannelNameParamTypeId, "POWER" + QString::number(i+1)));
                }
                deviceDescriptors << descriptor;
            }
            emit autoDevicesAppeared(deviceDescriptors);

            qCDebug(dcTasmota) << "Adding Tasmota connected devices";
            deviceDescriptors.clear();
            int shutterUpChannel = -1;
            int shutterDownChannel = -1;
            int blindsUpChannel = -1;
            int blindsDownChannel = -1;
            for (int i = 0; i < m_attachedDeviceParamTypeIdMap.value(info->device()->deviceClassId()).count(); i++) {
                ParamTypeId attachedDeviceParamTypeId = m_attachedDeviceParamTypeIdMap.value(info->device()->deviceClassId()).at(i);
                QString deviceType = info->device()->paramValue(attachedDeviceParamTypeId).toString();
                qCDebug(dcTasmota) << "Connected Device" << i + 1 << deviceType;
                if (deviceType == "Light") {
                    DeviceDescriptor descriptor(tasmotaLightDeviceClassId, info->device()->name() + " CH" + QString::number(i+1), QString(), info->device()->id());
                    descriptor.setParentDeviceId(info->device()->id());
                    if (m_attachedDeviceParamTypeIdMap.value(info->device()->deviceClassId()).count() == 1) {
                        descriptor.setParams(ParamList() << Param(tasmotaLightDeviceChannelNameParamTypeId, "POWER"));
                    } else {
                        descriptor.setParams(ParamList() << Param(tasmotaLightDeviceChannelNameParamTypeId, "POWER" + QString::number(i+1)));
                    }
                    deviceDescriptors << descriptor;
                } else if (deviceType == "Roller Shutter Up") {
                    shutterUpChannel = i+1;
                } else if (deviceType == "Roller Shutter Down") {
                    shutterDownChannel = i+1;
                } else if (deviceType == "Blinds Up") {
                    blindsUpChannel = i+1;
                } else if (deviceType == "Blinds Down") {
                    blindsDownChannel = i+1;
                }
            }
            if (shutterUpChannel != -1 && shutterDownChannel != -1) {
                qCDebug(dcTasmota) << "Adding Shutter device";
                DeviceDescriptor descriptor(tasmotaShutterDeviceClassId, info->device()->name() + " Shutter", QString(), info->device()->id());
                descriptor.setParams(ParamList()
                                     << Param(tasmotaShutterDeviceOpeningChannelParamTypeId, "POWER" + QString::number(shutterUpChannel))
                                     << Param(tasmotaShutterDeviceClosingChannelParamTypeId, "POWER" + QString::number(shutterDownChannel)));
                deviceDescriptors << descriptor;
            }
            if (blindsUpChannel != -1 && blindsDownChannel != -1) {
                qCDebug(dcTasmota) << "Adding Blinds device";
                DeviceDescriptor descriptor(tasmotaBlindsDeviceClassId, info->device()->name() + " Blinds", QString(), info->device()->id());
                descriptor.setParams(ParamList()
                                     << Param(tasmotaBlindsDeviceOpeningChannelParamTypeId, "POWER" + QString::number(blindsUpChannel))
                                     << Param(tasmotaBlindsDeviceClosingChannelParamTypeId, "POWER" + QString::number(blindsDownChannel)));
                deviceDescriptors << descriptor;
            }
            if (!deviceDescriptors.isEmpty()) {
                emit autoDevicesAppeared(deviceDescriptors);
            }
        });
        return;
    }

    if (m_connectedStateTypeMap.contains(device->deviceClassId())) {
        Device* parentDevice = myDevices().findById(device->parentId());
        StateTypeId connectedStateTypeId = m_connectedStateTypeMap.value(device->deviceClassId());
        device->setStateValue(m_connectedStateTypeMap.value(device->deviceClassId()), parentDevice->stateValue(connectedStateTypeId));
        return info->finish(Device::DeviceErrorNoError);
    }

    qCWarning(dcTasmota) << "Unhandled DeviceClass in setupDevice" << device->deviceClassId();
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

void DevicePluginTasmota::executeAction(DeviceActionInfo *info)
{
    Device *device = info->device();
    Action action = info->action();

    if (m_powerStateTypeMap.contains(device->deviceClassId())) {
        Device *parentDev = myDevices().findById(device->parentId());
        MqttChannel *channel = m_mqttChannels.value(parentDev);
        if (!channel) {
            qCWarning(dcTasmota()) << "No mqtt channel for this device.";
            return info->finish(Device::DeviceErrorHardwareNotAvailable);
        }
        ParamTypeId channelParamTypeId = m_channelParamTypeMap.value(device->deviceClassId());
        ParamTypeId powerActionParamTypeId = ParamTypeId(m_powerStateTypeMap.value(device->deviceClassId()).toString());
        qCDebug(dcTasmota) << "Publishing:" << channel->topicPrefixList().first() + "/sonoff/cmnd/" + device->paramValue(channelParamTypeId).toString() << (action.param(powerActionParamTypeId).value().toBool() ? "ON" : "OFF");
        channel->publish(channel->topicPrefixList().first() + "/sonoff/cmnd/" + device->paramValue(channelParamTypeId).toString().toLower(), action.param(powerActionParamTypeId).value().toBool() ? "ON" : "OFF");
        device->setStateValue(m_powerStateTypeMap.value(device->deviceClassId()), action.param(powerActionParamTypeId).value().toBool());
        return info->finish(Device::DeviceErrorNoError);
    }
    if (m_closableStopActionTypeMap.contains(device->deviceClassId())) {
        Device *parentDev = myDevices().findById(device->parentId());
        MqttChannel *channel = m_mqttChannels.value(parentDev);
        if (!channel) {
            qCWarning(dcTasmota()) << "No mqtt channel for this device.";
            return info->finish(Device::DeviceErrorHardwareNotAvailable);
        }
        ParamTypeId openingChannelParamTypeId = m_openingChannelParamTypeMap.value(device->deviceClassId());
        ParamTypeId closingChannelParamTypeId = m_closingChannelParamTypeMap.value(device->deviceClassId());
        if (action.actionTypeId() == m_closableOpenActionTypeMap.value(device->deviceClassId())) {
            qCDebug(dcTasmota) << "Publishing:" << channel->topicPrefixList().first() + "/sonoff/cmnd/" + device->paramValue(closingChannelParamTypeId).toString() << "OFF";
            channel->publish(channel->topicPrefixList().first() + "/sonoff/cmnd/" + device->paramValue(closingChannelParamTypeId).toString().toLower(), "OFF");
            qCDebug(dcTasmota) << "Publishing:" << channel->topicPrefixList().first() + "/sonoff/cmnd/" + device->paramValue(openingChannelParamTypeId).toString() << "ON";
            channel->publish(channel->topicPrefixList().first() + "/sonoff/cmnd/" + device->paramValue(openingChannelParamTypeId).toString().toLower(), "ON");
        } else if (action.actionTypeId() == m_closableCloseActionTypeMap.value(device->deviceClassId())) {
            qCDebug(dcTasmota) << "Publishing:" << channel->topicPrefixList().first() + "/sonoff/cmnd/" + device->paramValue(openingChannelParamTypeId).toString() << "OFF";
            channel->publish(channel->topicPrefixList().first() + "/sonoff/cmnd/" + device->paramValue(openingChannelParamTypeId).toString().toLower(), "OFF");
            qCDebug(dcTasmota) << "Publishing:" << channel->topicPrefixList().first() + "/sonoff/cmnd/" + device->paramValue(closingChannelParamTypeId).toString() << "ON";
            channel->publish(channel->topicPrefixList().first() + "/sonoff/cmnd/" + device->paramValue(closingChannelParamTypeId).toString().toLower(), "ON");
        } else { // Stop
            qCDebug(dcTasmota) << "Publishing:" << channel->topicPrefixList().first() + "/sonoff/cmnd/" + device->paramValue(openingChannelParamTypeId).toString() << "OFF";
            channel->publish(channel->topicPrefixList().first() + "/sonoff/cmnd/" + device->paramValue(openingChannelParamTypeId).toString().toLower(), "OFF");
            qCDebug(dcTasmota) << "Publishing:" << channel->topicPrefixList().first() + "/sonoff/cmnd/" + device->paramValue(closingChannelParamTypeId).toString() << "OFF";
            channel->publish(channel->topicPrefixList().first() + "/sonoff/cmnd/" + device->paramValue(closingChannelParamTypeId).toString().toLower(), "OFF");
        }
        return info->finish(Device::DeviceErrorNoError);
    }
    qCWarning(dcTasmota) << "Unhandled execute action call for device" << device;
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
        if (topic.startsWith(channel->topicPrefixList().first() + "/sonoff/POWER")) {
            QString channelName = topic.split("/").last();

            foreach (Device *child, myDevices()) {
                if (child->parentId() != dev->id()) {
                    continue;
                }
                if (child->paramValue(m_channelParamTypeMap.value(child->deviceClassId())).toString() != channelName) {
                    continue;
                }
                if (m_powerStateTypeMap.contains(child->deviceClassId())) {
                    child->setStateValue(m_powerStateTypeMap.value(child->deviceClassId()), payload == "ON");
                }
                if (child->deviceClassId() == tasmotaSwitchDeviceClassId) {
                    Event event(tasmotaSwitchPressedEventTypeId, child->id());
                    emit emitEvent(event);
                }
            }
        }
        if (topic.startsWith(channel->topicPrefixList().first() + "/sonoff/STATE")) {
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
                if (m_powerStateTypeMap.contains(child->deviceClassId())) {
                    QString childChannel = child->paramValue(m_channelParamTypeMap.value(child->deviceClassId())).toString();
                    QString valueString = jsonDoc.toVariant().toMap().value(childChannel).toString();
                    child->setStateValue(m_powerStateTypeMap.value(child->deviceClassId()), valueString == "ON");
                }
            }

        }
    }
}

