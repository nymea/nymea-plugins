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

#include "integrationpluginmqttclient.h"
#include "integrations/thing.h"
#include "plugininfo.h"
#include "network/mqtt/mqttprovider.h"

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
        client = new MqttClient(thing->paramValue(mqttClientThingClientIdParamTypeId).toString(), this);
        client->setUsername(thing->paramValue(mqttClientThingUsernameParamTypeId).toString());
        client->setPassword(thing->paramValue(mqttClientThingPasswordParamTypeId).toString());
        QString willTopic = thing->paramValue(mqttClientThingWillTopicParamTypeId).toString();
        if (!willTopic.isEmpty()) {
            client->setWillTopic(willTopic);
            client->setWillMessage(thing->paramValue(mqttClientThingWillMessageParamTypeId).toByteArray());
            client->setWillQoS(static_cast<Mqtt::QoS>(thing->paramValue(mqttClientThingWillQoSParamTypeId).toInt()));
            client->setWillRetain(thing->paramValue(mqttClientThingWillRetainParamTypeId).toBool());
        }
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
    connect(client, &MqttClient::connected, this, [this, thing](){
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
    emitEvent(Event(eventTypeId, thing->id(), ParamList() << Param(topicParamTypeId, topic) << Param(payloadParamTypeId, payload)));
}

void IntegrationPluginMqttClient::thingRemoved(Thing *thing)
{
    qCDebug(dcMqttclient) << thing;
    m_clients.take(thing)->deleteLater();
}

