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
    \page mqtt.html
    \title Generic MQTT
    \brief Plugin for subscribing and publishing to an MQTT broker.

    \ingroup plugins
    \ingroup nymea-plugins-maker

    This plugin allows to subscribe and publish to MQTT brokers (the nymea internal broker and external ones).

    \note This plugin is ment to be combined with a \l{nymeaserver::Rule}.

    \section3 Example

    A device is configured to publish its state to a MQTT broker. Using this plugin the user can subscribe to
    the same topic on that broker and monitor the device's state.

    Publishing is also supported. This allows use cases such as controlling IoT things via MQTT by publishing
    to topics such devices are subscribed to. Other possibilities are to use nymea as a "translator" between other
    transport layers to MQTT. For instance a sensor might deliver sensor data via Bluetooth to Nymea and using this
    MQTT plugin and a \l{nymeaserver::Rule} nymea can be configured to forward all those sensor values to a MQTT broker.

    \chapter Plugin properties
    The plugin implements the inputtrigger and outputtrigger interfaces, adding parameters for topic and payload to
    the trigger action and triggered event.

    For more details how to read this JSON file please check out the documentation for \l{The plugin JSON File}.

    \quotefile plugins/deviceplugins/mqtt/devicepluginmqtt.json
*/

#include "devicepluginmqtt.h"
#include "plugin/device.h"
#include "plugininfo.h"
#include "network/mqtt/mqttprovider.h"

#include "nymea-mqtt/mqttclient.h"

DevicePluginMqtt::DevicePluginMqtt()
{

}

DeviceManager::DeviceSetupStatus DevicePluginMqtt::setupDevice(Device *device)
{
    MqttClient *client = nullptr;
    if (device->deviceClassId() == internalMqttClientDeviceClassId) {
        client = hardwareManager()->mqttProvider()->createInternalClient(device->id());
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
    connect(client, &MqttClient::subscribeResult, this, [this, device](quint16 packetId, const Mqtt::SubscribeReturnCodes returnCodes){
        Q_UNUSED(packetId)
        emit deviceSetupFinished(device, returnCodes.first() == Mqtt::SubscribeReturnCodeFailure ? DeviceManager::DeviceSetupStatusFailure : DeviceManager::DeviceSetupStatusSuccess);
    });
    connect(client, &MqttClient::publishReceived, this, &DevicePluginMqtt::publishReceived);
    connect(client, &MqttClient::published, this, &DevicePluginMqtt::published);
    // In case we're already connected, manually call subscribe now
    if (client->isConnected()) {
        subscribe(device);
    }

    return DeviceManager::DeviceSetupStatusAsync;
}


DeviceManager::DeviceError DevicePluginMqtt::executeAction(Device *device, const Action &action)
{
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
        return DeviceManager::DeviceErrorDeviceNotFound;
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
    m_pendingPublishes.insert(packetId, action);

    return DeviceManager::DeviceErrorAsync;
}

void DevicePluginMqtt::subscribe(Device *device)
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

void DevicePluginMqtt::publishReceived(const QString &topic, const QByteArray &payload, bool retained)
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

void DevicePluginMqtt::published(quint16 packetId)
{
    if (!m_pendingPublishes.contains(packetId)) {
        return;
    }

    emit actionExecutionFinished(m_pendingPublishes.take(packetId).id(), DeviceManager::DeviceErrorNoError);
}

void DevicePluginMqtt::deviceRemoved(Device *device)
{
    qCDebug(dcMqttclient) << device;
    m_clients.take(device)->deleteLater();
}

