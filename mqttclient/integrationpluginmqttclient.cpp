// SPDX-License-Identifier: GPL-3.0-or-later

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*
* Copyright (C) 2013 - 2024, nymea GmbH
* Copyright (C) 2024 - 2025, chargebyte austria GmbH
*
* This file is part of nymea-plugins.
*
* nymea-plugins is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* (at your option) any later version.
*
* nymea-plugins is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
* General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with nymea-plugins. If not, see <https://www.gnu.org/licenses/>.
*
* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

#include "integrationpluginmqttclient.h"
#include "plugininfo.h"

#include <integrations/thing.h>
#include <network/mqtt/mqttprovider.h>

#include <mqttclient.h>

IntegrationPluginMqttClient::IntegrationPluginMqttClient()
{

}

void IntegrationPluginMqttClient::setupThing(ThingSetupInfo *info)
{
    Thing *thing = info->thing();

    MqttClient *client = nullptr;
    if (m_clients.contains(thing)) {
        delete m_clients.take(thing);
    }
    if (thing->thingClassId() == internalMqttClientThingClassId) {
        client = hardwareManager()->mqttProvider()->createInternalClient(thing->id().toString());
        if (!client) {
            info->finish(Thing::ThingErrorHardwareNotAvailable, QT_TR_NOOP("Unable to to connect to internal MQTT broker. Verify the MQTT broker is set up and running."));
            return;
        }

    } else if (thing->thingClassId() == mqttClientThingClassId){
        client = new MqttClient(thing->paramValue(mqttClientThingClientIdParamTypeId).toString(), thing);
        client->setUsername(thing->paramValue(mqttClientThingUsernameParamTypeId).toString());
        client->setPassword(thing->paramValue(mqttClientThingPasswordParamTypeId).toString());
        client->setKeepAlive(thing->paramValue(mqttClientThingKeepAliveParamTypeId).toUInt());
        QString willTopic = thing->paramValue(mqttClientThingWillTopicParamTypeId).toString();
        if (!willTopic.isEmpty()) {
            client->setWillTopic(willTopic);
            client->setWillMessage(thing->paramValue(mqttClientThingWillMessageParamTypeId).toByteArray());
            client->setWillQoS(static_cast<Mqtt::QoS>(thing->paramValue(mqttClientThingWillQoSParamTypeId).toInt()));
            client->setWillRetain(thing->paramValue(mqttClientThingWillRetainParamTypeId).toBool());
        }
        qCDebug(dcMqttclient()) << "Connecting to" << thing->paramValue(mqttClientThingServerAddressParamTypeId).toString() << thing->paramValue(mqttClientThingServerPortParamTypeId).toInt() << "SSL:" << thing->paramValue(mqttClientThingUseSslParamTypeId).toBool() << "user:" << thing->paramValue(mqttClientThingUsernameParamTypeId).toString() << "Client ID:" << thing->paramValue(mqttClientThingClientIdParamTypeId).toString();
        client->connectToHost(thing->paramValue(mqttClientThingServerAddressParamTypeId).toString(),
                              thing->paramValue(mqttClientThingServerPortParamTypeId).toInt(),
                              true,
                              thing->paramValue(mqttClientThingUseSslParamTypeId).toBool());
    }
    m_clients.insert(thing, client);

    connect(client, &MqttClient::error, info, [info](QAbstractSocket::SocketError socketError){
        qCWarning(dcMqttclient()) << "An error happened during setup:" << socketError;
        info->finish(Thing::ThingErrorHardwareFailure, QT_TR_NOOP("An error happened connecting to the MQTT broker. Please make sure the login credentials are correct and your user has apprpriate permissions to subscribe to the given topic filter."));
    });
    connect(client, &MqttClient::connected, thing, [this, thing](){
        subscribe(thing);
    });
    connect(client, &MqttClient::subscribeResult, info, [info](quint16 /*packetId*/, const Mqtt::SubscribeReturnCodes returnCodes){
        info->finish(returnCodes.first() == Mqtt::SubscribeReturnCodeFailure ? Thing::ThingErrorHardwareFailure : Thing::ThingErrorNoError);
    });
    connect(client, &MqttClient::publishReceived, this, &IntegrationPluginMqttClient::publishReceived);
    // In case we're already connected, manually call subscribe now
    if (client->isConnected()) {
        subscribe(thing);
    }
}


void IntegrationPluginMqttClient::executeAction(ThingActionInfo *info)
{
    Thing *thing = info->thing();
    Action action = info->action();

    ParamTypeId topicParamTypeId = internalMqttClientTriggerActionTopicParamTypeId;
    ParamTypeId payloadParamTypeId = internalMqttClientTriggerActionDataParamTypeId;
    ParamTypeId qosParamTypeId = internalMqttClientTriggerActionQosParamTypeId;
    ParamTypeId retainParamTypeId = internalMqttClientTriggerActionRetainParamTypeId;

    if (thing->thingClassId() == mqttClientThingClassId) {
        topicParamTypeId = mqttClientTriggerActionTopicParamTypeId;
        payloadParamTypeId = mqttClientTriggerActionDataParamTypeId;
        qosParamTypeId = mqttClientTriggerActionQosParamTypeId;
        retainParamTypeId = mqttClientTriggerActionRetainParamTypeId;
    }

    MqttClient *client = m_clients.value(thing);
    if (!client) {
        qCWarning(dcMqttclient) << "No valid MQTT client for thing" << thing->name();
        return info->finish(Thing::ThingErrorThingNotFound);
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
            info->finish(Thing::ThingErrorNoError);
        }
    });
}

void IntegrationPluginMqttClient::subscribe(Thing *thing)
{
    MqttClient *client = m_clients.value(thing);
    if (!client) {
        // Device might have been removed
        return;
    }
    if (thing->thingClassId() == internalMqttClientThingClassId) {
        client->subscribe(thing->paramValue(internalMqttClientThingTopicFilterParamTypeId).toString());
    } else {
        client->subscribe(thing->paramValue(mqttClientThingTopicFilterParamTypeId).toString());
    }
}

void IntegrationPluginMqttClient::publishReceived(const QString &topic, const QByteArray &payload, bool retained)
{
    qCDebug(dcMqttclient()) << "Publish received" << topic << payload << retained;

    MqttClient* client = static_cast<MqttClient*>(sender());
    Thing *thing = m_clients.key(client);
    if (!thing) {
        qCWarning(dcMqttclient) << "Received a publish message from a client where de don't have a matching thing";
        return;
    }

    EventTypeId eventTypeId = internalMqttClientTriggeredEventTypeId;
    ParamTypeId topicParamTypeId = internalMqttClientTriggeredEventTopicParamTypeId;
    ParamTypeId payloadParamTypeId = internalMqttClientTriggeredEventDataParamTypeId;

    if (thing->thingClassId() == mqttClientThingClassId) {
        eventTypeId = mqttClientTriggeredEventTypeId;
        topicParamTypeId = mqttClientTriggeredEventTopicParamTypeId;
        payloadParamTypeId = mqttClientTriggeredEventDataParamTypeId;
    }

    emit emitEvent(Event(eventTypeId, thing->id(), ParamList() << Param(topicParamTypeId, topic) << Param(payloadParamTypeId, payload)));
}

void IntegrationPluginMqttClient::thingRemoved(Thing *thing)
{
    qCDebug(dcMqttclient) << thing;
    m_clients.take(thing)->deleteLater();
}

