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

#include "devicepluginmqttclient.h"
#include "devices/device.h"
#include "plugininfo.h"
#include "network/mqtt/mqttprovider.h"

#include "nymea-mqtt/mqttclient.h"

DevicePluginMqttClient::DevicePluginMqttClient()
{

}

void DevicePluginMqttClient::setupDevice(DeviceSetupInfo *info)
{
    Device *device = info->device();

    MqttClient *client = nullptr;
    if (device->deviceClassId() == internalMqttClientDeviceClassId) {
        client = hardwareManager()->mqttProvider()->createInternalClient(device->id().toString());
    } else if (device->deviceClassId() == mqttClientDeviceClassId){
        client = new MqttClient("nymea-" + device->id().toString().remove(QRegExp("[{}]")).left(8), this);
        client->setUsername(device->paramValue(mqttClientDeviceUsernameParamTypeId).toString());
        client->setPassword(device->paramValue(mqttClientDevicePasswordParamTypeId).toString());
        client->connectToHost(device->paramValue(mqttClientDeviceServerAddressParamTypeId).toString(), device->paramValue(mqttClientDeviceServerPortParamTypeId).toInt());
    }
    m_clients.insert(device, client);

    connect(client, &MqttClient::connected, this, [this, device](){
        subscribe(device);
    });
    connect(client, &MqttClient::subscribeResult, info, [info](quint16 /*packetId*/, const Mqtt::SubscribeReturnCodes returnCodes){
        info->finish(returnCodes.first() == Mqtt::SubscribeReturnCodeFailure ? Device::DeviceErrorHardwareFailure : Device::DeviceErrorNoError);
    });
    connect(client, &MqttClient::publishReceived, this, &DevicePluginMqttClient::publishReceived);
    // In case we're already connected, manually call subscribe now
    if (client->isConnected()) {
        subscribe(device);
    }
}


void DevicePluginMqttClient::executeAction(DeviceActionInfo *info)
{
    Device *device = info->device();
    Action action = info->action();

    ParamTypeId topicParamTypeId = internalMqttClientTriggerActionTopicParamTypeId;
    ParamTypeId payloadParamTypeId = internalMqttClientTriggerActionDataParamTypeId;
    ParamTypeId qosParamTypeId = internalMqttClientTriggerActionQosParamTypeId;
    ParamTypeId retainParamTypeId = internalMqttClientTriggerActionRetainParamTypeId;

    if (device->deviceClassId() == mqttClientDeviceClassId) {
        topicParamTypeId = mqttClientTriggerActionTopicParamTypeId;
        payloadParamTypeId = mqttClientTriggerActionDataParamTypeId;
        qosParamTypeId = mqttClientTriggerActionQosParamTypeId;
        retainParamTypeId = mqttClientTriggerActionRetainParamTypeId;
    }

    MqttClient *client = m_clients.value(device);
    if (!client) {
        qCWarning(dcMqttclient) << "No valid MQTT client for device" << device->name();
        return info->finish(Device::DeviceErrorDeviceNotFound);
    }
    Mqtt::QoS qos = Mqtt::QoS0;
    switch (action.param(qosParamTypeId).value().toInt()) {
    case 0:
        qos = Mqtt::QoS0;
        break;
    case 1:
        qos = Mqtt::QoS1;
        break;
    case 2:
        qos = Mqtt::QoS2;
        break;
    }
    quint16 packetId = client->publish(action.param(topicParamTypeId).value().toString(),
                    action.param(payloadParamTypeId).value().toByteArray(),
                    qos,
                    action.param(retainParamTypeId).value().toBool());

    connect(client, &MqttClient::published, info, [info, packetId](quint16 packetIdResult){
        if (packetId == packetIdResult) {
            info->finish(Device::DeviceErrorNoError);
        }
    });
}

void DevicePluginMqttClient::subscribe(Device *device)
{
    MqttClient *client = m_clients.value(device);
    if (!client) {
        // Device might have been removed
        return;
    }
    if (device->deviceClassId() == internalMqttClientDeviceClassId) {
        client->subscribe(device->paramValue(internalMqttClientDeviceTopicFilterParamTypeId).toString());
    } else {
        client->subscribe(device->paramValue(mqttClientDeviceTopicFilterParamTypeId).toString());
    }
}

void DevicePluginMqttClient::publishReceived(const QString &topic, const QByteArray &payload, bool retained)
{
    qCDebug(dcMqttclient()) << "Publish received" << topic << payload << retained;

    MqttClient* client = static_cast<MqttClient*>(sender());
    Device *device = m_clients.key(client);
    if (!device) {
        qCWarning(dcMqttclient) << "Received a publish message from a client where de don't have a matching device";
        return;
    }

    EventTypeId eventTypeId = internalMqttClientTriggeredEventTypeId;
    ParamTypeId topicParamTypeId = internalMqttClientTriggeredEventTopicParamTypeId;
    ParamTypeId payloadParamTypeId = internalMqttClientTriggeredEventDataParamTypeId;

    if (device->deviceClassId() == mqttClientDeviceClassId) {
        eventTypeId = mqttClientTriggeredEventTypeId;
        topicParamTypeId = mqttClientTriggeredEventTopicParamTypeId;
        payloadParamTypeId = mqttClientTriggeredEventDataParamTypeId;
    }
    emitEvent(Event(eventTypeId, device->id(), ParamList() << Param(topicParamTypeId, topic) << Param(payloadParamTypeId, payload)));
}

void DevicePluginMqttClient::deviceRemoved(Device *device)
{
    qCDebug(dcMqttclient) << device;
    m_clients.take(device)->deleteLater();
}

