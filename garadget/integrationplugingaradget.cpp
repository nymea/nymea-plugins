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

    qCDebug(dcGaradget) << "entered setupThing" << thing->paramValue(garadgetThingDeviceNameParamTypeId) ;
    MqttClient *client = nullptr;
    client = hardwareManager()->mqttProvider()->createInternalClient(thing->id().toString());
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


void IntegrationPluginGaradget::postSetupThing(Thing *thing)
{

    if (!m_pluginTimer) {
        uint updatetime = 30;
        m_pluginTimer = hardwareManager()->pluginTimerManager()->registerTimer(updatetime);
        connect(m_pluginTimer, &PluginTimer::timeout, this, [=](){
            foreach (Thing *thing, myThings()) {
                if ((m_lastActivityTimeStamps[thing].msecsTo(QDateTime::currentDateTime()) > 1000 * updatetime) && (thing->stateValue(garadgetConnectedStateTypeId).toBool() == true)) {
                    qCDebug(dcGaradget) << "disconnect garadget";
                    thing->setStateValue(garadgetConnectedStateTypeId, false);
                }
                if (thing->stateValue(garadgetConnectedStateTypeId).toBool() == true) {
                    m_mqttClients.value(thing)->publish("garadget/" + thing->paramValue(garadgetThingDeviceNameParamTypeId).toString() + "/command", "get-status");
                }
            }
        });
        connect(thing, &Thing::settingChanged, this, [=](const ParamTypeId &settingTypeId, const QVariant &value){
            foreach (Thing *thing, myThings()) {
                QJsonObject garadgetobj;

                if ((thing->stateValue(garadgetConnectedStateTypeId).toBool() == true) && (settingTypeId == garadgetSettingsRdtParamTypeId)){
                    garadgetobj.insert("rdt", value.toInt());
                    garadgetobj.insert("rlp", thing->setting(garadgetSettingsRlpParamTypeId).toInt());
                    QJsonDocument garadgetdoc(garadgetobj);
                    QByteArray garadgetdata = garadgetdoc.toJson(QJsonDocument::Compact);
                    QString jsonDoc(garadgetdata);
                    qCDebug(dcGaradget()) << "Changing Configuration" << garadgetdata;
                    m_mqttClients.value(thing)->publish("garadget/" + thing->paramValue(garadgetThingDeviceNameParamTypeId).toString() + "/set-config", garadgetdata);
                }
            }
        });

    }
}



void IntegrationPluginGaradget::thingRemoved(Thing *thing)
{
    qCDebug(dcGaradget) << thing << "Removed";
    m_mqttClients.take(thing)->deleteLater();
    if (m_pluginTimer && myThings().isEmpty()) {
        hardwareManager()->pluginTimerManager()->unregisterTimer(m_pluginTimer);
        m_pluginTimer = nullptr;
    }
}

void IntegrationPluginGaradget::executeAction(ThingActionInfo *info)
{
    Thing *thing = info->thing();
    Action action = info->action();

    QString name = "garadget/" + thing->paramValue(garadgetThingDeviceNameParamTypeId).toString();
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
        if (action.paramValue(garadgetSrtActionSrtParamTypeId).toInt() > -1) {
            actint = action.paramValue( garadgetSrtActionSrtParamTypeId).toInt();
            conftype = "srt";
        } else {
            name = name + "/command";
            act = "get-config";
        }
    }
    if (action.actionTypeId() == garadgetMttActionTypeId) {
        if (action.paramValue( garadgetMttActionMttParamTypeId).toInt() > 0) {
            actint = action.paramValue( garadgetMttActionMttParamTypeId).toInt();
            conftype = "mtt";
        } else {
            name = name + "/command";
            act = "get-config";
        }
    }
    if (action.actionTypeId() == garadgetRltActionTypeId) {
        if (action.paramValue( garadgetRltActionRltParamTypeId).toInt() > 0) {
            actint = action.paramValue(garadgetRltActionRltParamTypeId).toInt();
            conftype = "rlt";
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
    info->finish(Thing::ThingErrorNoError);
    return;
}

void IntegrationPluginGaradget::subscribe(Thing *thing)
{
    MqttClient *client = m_mqttClients.value(thing);
    if (!client) {
        // Device might have been removed
        return;
    }
    client->subscribe("garadget/" + thing->paramValue(garadgetThingDeviceNameParamTypeId).toString() + "/#");
}

void IntegrationPluginGaradget::publishReceived(const QString &topic, const QByteArray &payload, bool retained)
{
//    qCDebug(dcGaradget) << "Received message from topic" << topic << "with msg" << payload << "retain flag" << retained;


    MqttClient* client = static_cast<MqttClient*>(sender());
    Thing *thing = m_mqttClients.key(client);
    if (!thing) {
        qCWarning(dcGaradget) << "Received a publish message from a client but don't have a matching thing" << retained;
        return;
    }
    if (topic.endsWith("/status")) {
        if (thing->stateValue(garadgetConnectedStateTypeId) == false) {
            qCDebug(dcGaradget) << "Setting Garadget to connected" ;
            thing->setStateValue(garadgetConnectedStateTypeId, true);
        }
        m_lastActivityTimeStamps[thing] = QDateTime::currentDateTime();
        QJsonParseError error;
        QJsonDocument jsonDoc = QJsonDocument::fromJson(payload, &error);
        if (error.error != QJsonParseError::NoError) {
            qCWarning(dcGaradget) << "Cannot parse JSON from Garadget device" << error.errorString();
            return;
        }
        QJsonObject jo = jsonDoc.object();
        thing->setStateValue(garadgetSignalStrengthStateTypeId, (99 + jo.value(QString("signal")).toInt()) * 2 );
        thing->setStateValue(garadgetSensorlevelStateTypeId, jo.value(QString("sensor")).toInt());
        thing->setStateValue(garadgetBrightlevelStateTypeId, jo.value(QString("bright")).toInt());
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
        thing->setStateValue(garadgetMttStateTypeId,jo.value(QString("mtt")).toInt());
        thing->setSettingValue(garadgetSettingsRdtParamTypeId,jo.value(QString("rdt")).toInt());
        thing->setSettingValue(garadgetSettingsRlpParamTypeId,jo.value(QString("rlp")).toInt());
        qCDebug(dcGaradget) << "System Configuration" << "srt =" << thing->stateValue(garadgetSrtStateTypeId).toInt() << "rlt =" << thing->stateValue(garadgetRltStateTypeId).toInt()<< "mtt =" << thing->stateValue(garadgetMttStateTypeId).toInt() << "rdt ="  << thing->setting(garadgetSettingsRdtParamTypeId).toUInt() << "rlp =" << thing->setting(garadgetSettingsRlpParamTypeId).toUInt();
    }
    if (topic.endsWith("/set-config")){
        if ( (payload.contains("mqip"))  or (payload.contains("mqpt")) or (payload.contains("nme")) ) {
            thing->setStateValue(garadgetConnectedStateTypeId, false);
            qCDebug(dcGaradget) << "Detected change of Broker msg - set connected to false";
        }
    }
}
