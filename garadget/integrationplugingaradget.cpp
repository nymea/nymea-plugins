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

#include "integrationplugingaradget.h"
#include "integrations/thing.h"
#include "plugininfo.h"

#include <QJsonDocument>

#include "network/mqtt/mqttprovider.h"
#include <mqttclient.h>


void IntegrationPluginGaradget::setupThing(ThingSetupInfo *info)
{
    Thing *thing = info->thing();

    QString device = thing->paramValue(garadgetThingTopicFilterParamTypeId).toString();
    if (!(device.startsWith( "garadget/"))) {
        if (device.contains("/")) {
            return info->finish(Thing::ThingErrorInvalidParameter, QT_TR_NOOP(QString ("The given deviceName of is not valid.")));
        }
        thing->setParamValue(garadgetThingTopicFilterParamTypeId,"garadget/" + device + "/#" );
/* do not delete
        thing->setParamValue(garadgetThingTopicFilterParamTypeId,"garadget/" + device + "/config" );
        qCDebug(dcGaradget) << "Setting up to reconfig deviceName to " << thing->paramValue(garadgetThingTopicFilterParamTypeId).toString();
    } else {
        if (device.endsWith("/config")) {
            qCDebug(dcGaradget) << "incoming device defintion" << device;
            thing->setParamValue(garadgetThingTopicFilterParamTypeId,"garadget/" + device + "/#" );
            qCDebug(dcGaradget) << "Setting up to for operation with deviceName" << thing->paramValue(garadgetThingTopicFilterParamTypeId).toString();
        }
*/
    }

    qCDebug(dcGaradget) << "entered setupThing" << thing->paramValue(garadgetThingTopicFilterParamTypeId);
    MqttClient *client = nullptr;
    if (thing->thingClassId() == garadgetThingClassId) {
        client = hardwareManager()->mqttProvider()->createInternalClient(thing->id().toString());
    }
    m_mqttClients.insert(thing, client);

    connect(client, &MqttClient::connected, this, [this, thing](){
        subscribe(thing);
    });
    connect(client, &MqttClient::subscribeResult, info, [info](quint16 /*packetId*/, const Mqtt::SubscribeReturnCodes returnCodes){
        info->finish(returnCodes.first() == Mqtt::SubscribeReturnCodeFailure ? Thing::ThingErrorHardwareFailure : Thing::ThingErrorNoError);
    });
    connect(client, &MqttClient::publishReceived, this, &IntegrationPluginGaradget::publishReceived);
    // In case we're already connected, manually call subscribe now
    if (client->isConnected()) {
        qCDebug(dcGaradget) << "entered is Connected" << client;
        subscribe(thing);
    }
}

void IntegrationPluginGaradget::thingRemoved(Thing *thing)
{
    qCDebug(dcGaradget) << thing;
    m_mqttClients.take(thing)->deleteLater();
}

void IntegrationPluginGaradget::executeAction(ThingActionInfo *info)
{
    Thing *thing = info->thing();
    Action action = info->action();

    qCDebug(dcGaradget) << "executeAction function" << (thing->thingClassId());

    QString name = thing->paramValue(garadgetThingTopicFilterParamTypeId).toString();
    if (name.endsWith("/config")) {
        qCDebug(dcGaradget) << "Device" << name << " not configured yet";
        qCDebug(dcGaradget) << "NME value is" << thing->stateValue(garadgetOpeningOutputStateTypeId).toString();
        return;
    }
    if (name.endsWith("/#")) {
        name.chop(2);
    }
    name = name + "/command";
//  These net 2 never get setup in current configuration. Not clear what to do to set them up.
    ParamTypeId qosParamTypeId = garadgetTriggerActionQosParamTypeId;
    ParamTypeId retainParamTypeId = garadgetTriggerActionRetainParamTypeId;

    MqttClient *client = m_mqttClients.value(thing);
    if (!client) {
        qCWarning(dcGaradget) << "No valid MQTT client for thing" << thing->name();
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
    qCDebug(dcGaradget) << "publish" << name << action.actionTypeId() << qos;
    QString act = "";
    QByteArray actarray;
    if (action.actionTypeId() == garadgetOpenActionTypeId) {
        act = "open";
    }
    if (action.actionTypeId() == garadgetCloseActionTypeId) {
        act = "close";
    }
    if (action.actionTypeId() == garadgetStopActionTypeId) {
        act = "stop";
    }
    actarray = act.toUtf8();
    qCDebug(dcGaradget) << "Publishing:" << name << act;
    quint16 packetId = client->publish(name, actarray, qos, action.param(retainParamTypeId).value().toBool());
    connect(client, &MqttClient::published, info, [info, packetId](quint16 packetIdResult){
        if (packetId == packetIdResult) {
            info->finish(Thing::ThingErrorNoError);
        }
    });
    return;
}

void IntegrationPluginGaradget::subscribe(Thing *thing)
{
    MqttClient *client = m_mqttClients.value(thing);
    if (!client) {
        // Device might have been removed
        return;
    }
    if (thing->thingClassId() == garadgetThingClassId) {
        client->subscribe(thing->paramValue(garadgetThingTopicFilterParamTypeId).toString());
    }
}

void IntegrationPluginGaradget::publishReceived(const QString &topic, const QByteArray &payload, bool retained)
{
    qCDebug(dcGaradget) << "Publish received" << topic << payload << retained;


    MqttClient* client = static_cast<MqttClient*>(sender());
    Thing *thing = m_mqttClients.key(client);
    if (!thing) {
        qCWarning(dcGaradget) << "Received a publish message from a client where de don't have a matching thing";
        return;
    }
    if (topic.endsWith("/status")) {
        thing->setStateValue(garadgetConnectedStateTypeId, true);
        QJsonParseError error;
        QJsonDocument jsonDoc = QJsonDocument::fromJson(payload, &error);
        if (error.error != QJsonParseError::NoError) {
            qCWarning(dcGaradget) << "Cannot parse JSON from Garadget device" << error.errorString();
            return;
        }
        QJsonObject jo = jsonDoc.object();
        if (jo.value(QString("status")).toString().contains(QString("stopped"))) {
            thing->setStateValue(garadgetStateStateTypeId, "intermediate");
            qCDebug(dcGaradget) << "Garadget is" << jo.value(QString("status")).toString() ;
            thing->setStateValue(garadgetPercentageStateTypeId,50);
        } else {
            thing->setStateValue(garadgetStateStateTypeId, jo.value(QString("status")).toString());
            qCDebug(dcGaradget) << "Garadget is" << jo.value(QString("status")).toString() ;

            if (jo.value(QString("status")).toString().contains(QString("opening"))) {

            } else {
                if (jo.value(QString("status")).toString().contains(QString("open"))) {
                    thing->setStateValue(garadgetPercentageStateTypeId,0);
                    qCDebug(dcGaradget) << "Garadget Door Open" ;
                }
            }
            if (jo.value(QString("status")).toString().contains(QString("closed"))) {
                thing->setStateValue(garadgetPercentageStateTypeId, 100);
                qCDebug(dcGaradget) << "Garadget Door Closed " ;
            }
        }
    }
    if (topic.endsWith("/config")) {
        QJsonParseError error;
        QJsonDocument jsonDoc = QJsonDocument::fromJson(payload, &error);
        if (error.error != QJsonParseError::NoError) {
            qCWarning(dcGaradget) << "Cannot parse JSON from Garadget device" << error.errorString();
            return;
        }
        QJsonObject jo = jsonDoc.object();
        qCDebug(dcGaradget) << "id=" << jo.value(QString("id")).toString() << "nme=" << jo.value(QString("nme")).toString();
        if (!(jo.value(QString("id")).toString() == jo.value(QString("nme")).toString())) {
            // Michael, would like to cause a nme and mqtt configuration messaged to be published to the device
            // then need to establish different connect maybe due to change in topic name

            qCDebug(dcGaradget) << "System to change deviceName to ID and restrict device to mqtt only";

        }
    }
}
