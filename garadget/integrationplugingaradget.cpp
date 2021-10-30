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

    QString device = thing->paramValue(garadgetThingDeviceNameParamTypeId).toString();
    if (!(device.startsWith( "garadget/"))) {
        if (device.contains("/")) {
            return info->finish(Thing::ThingErrorInvalidParameter, QT_TR_NOOP(QString ("The given deviceName is not valid - no /s allowed")));
        }
        thing->setParamValue(garadgetThingDeviceNameParamTypeId,"garadget/" + device + "/#" );
    }

    qCDebug(dcGaradget) << "entered setupThing" << thing->paramValue(garadgetThingDeviceNameParamTypeId);
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
    qCDebug(dcGaradget) << thing << "Removed";
    m_mqttClients.take(thing)->deleteLater();
}

void IntegrationPluginGaradget::executeAction(ThingActionInfo *info)
{
    Thing *thing = info->thing();
    Action action = info->action();

    QString name = thing->paramValue(garadgetThingDeviceNameParamTypeId).toString();
    if (name.endsWith("/#")) {
        name.chop(2);
    }
    MqttClient *client = m_mqttClients.value(thing);
    if (!client) {
        qCWarning(dcGaradget) << "No valid MQTT client for thing" << thing->name();
        return info->finish(Thing::ThingErrorThingNotFound);
    }
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
    if (act != "" ) {
        name = name + "/command";
    }
    QString conftype = "";
    int actint = 0;
    if (action.actionTypeId() == garadgetSrtActionTypeId) {
        if ( (action.paramValue(garadgetSrtActionSrtParamTypeId).toInt() > -1) and (action.paramValue(garadgetSrtActionSrtParamTypeId).toInt() < 81)) {
            actint = action.paramValue( garadgetSrtActionSrtParamTypeId).toInt();
            conftype = "srt";
        } else {
            name = name + "/command";
            act = "get-config";
        }
    }
    if (action.actionTypeId() == garadgetMttActionTypeId) {
        if ( (action.paramValue( garadgetMttActionMttParamTypeId).toInt() > 0) and (action.paramValue( garadgetMttActionMttParamTypeId).toInt() < 121) ){
            actint = action.paramValue( garadgetMttActionMttParamTypeId).toInt() * 1000;
            conftype = "mtt";
        } else {
            name = name + "/command";
            act = "get-config";
        }
    }
    if (action.actionTypeId() == garadgetRltActionTypeId) {
        if ( (action.paramValue( garadgetRltActionRltParamTypeId).toInt() > 9) and (action.paramValue( garadgetRltActionRltParamTypeId).toInt() < 2001) ){
            actint = action.paramValue(garadgetRltActionRltParamTypeId).toInt() * 1000;
            conftype = "mtt";
        } else {
            name = name + "/command";
            act = "get-config";
        }
    }
    if (conftype != "") {
        qCDebug(dcGaradget) << "Processing config change" << conftype << "to" << action.paramValue( garadgetSrtActionSrtParamTypeId).toInt();
        name = name + "/set-config";
        QJsonObject garadgetobj;
        garadgetobj.insert(conftype, actint);
        QJsonDocument garadgetdoc(garadgetobj);
        QByteArray garadgetdata = garadgetdoc.toJson(QJsonDocument::Compact);
        QString jsonDoc(garadgetdata);
        act = garadgetdata;
    }
    if (act != "" ) {
        actarray = act.toUtf8();
        qCDebug(dcGaradget) << "Publishing:" << name << act;
        quint16 packetId = client->publish(name, actarray, Mqtt::QoS1, false);
        connect(client, &MqttClient::published, info, [info, packetId](quint16 packetIdResult){
            if (packetId == packetIdResult) {
                info->finish(Thing::ThingErrorNoError);
            }
        });
    }
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
        client->subscribe(thing->paramValue(garadgetThingDeviceNameParamTypeId).toString());
    }
}

void IntegrationPluginGaradget::publishReceived(const QString &topic, const QByteArray &payload, bool retained)
{
    qCDebug(dcGaradget) << "Received message from topic" << topic << "with msg" << payload << "retain flag" << retained;


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
        } else {
            thing->setStateValue(garadgetStateStateTypeId, jo.value(QString("status")).toString());
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
        thing->setStateValue(garadgetSrtStateTypeId,jo.value(QString("srt")).toInt());
        thing->setStateValue(garadgetRltStateTypeId,jo.value(QString("rlt")).toInt());
        thing->setStateValue(garadgetMttStateTypeId,jo.value(QString("mtt")).toInt()/1000);
        qCDebug(dcGaradget) << "System Configuration" << "srt =" << thing->stateValue(garadgetSrtStateTypeId).toInt() << "rlt =" << thing->stateValue(garadgetRltStateTypeId).toInt()<< "mtt =" << thing->stateValue(garadgetMttStateTypeId).toInt() * 1000;
    }
}
