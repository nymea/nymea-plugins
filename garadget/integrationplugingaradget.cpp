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

    pluginStorage()->beginGroup(thing->id().toString());
    pluginStorage()->setValue("lastWillAvailable", false);
    pluginStorage()->endGroup();

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
        subscribe(thing);
    }
}


void IntegrationPluginGaradget::postSetupThing(Thing *thing)
{

    if (!m_pluginTimer) {
        int updatetime = 30;
        int lwtupdatetime = 300 / updatetime;
        m_pluginTimer = hardwareManager()->pluginTimerManager()->registerTimer(updatetime);
        connect(m_pluginTimer, &PluginTimer::timeout, this, [=](){
            m_statuscounter[thing] += 1;
            foreach (Thing *thing, myThings()) {
                pluginStorage()->beginGroup(thing->id().toString());
                bool lastWillAvailable = pluginStorage()->value("lastWillAvailable").toBool();
                pluginStorage()->endGroup();
                if ((lastWillAvailable == false) && (m_lastActivityTimeStamps[thing].msecsTo(QDateTime::currentDateTime()) > 2000 * updatetime) && (thing->stateValue(garadgetConnectedStateTypeId).toBool() == true)) {
                    qCDebug(dcGaradget) << "disconnect device" << thing->paramValue(garadgetThingDeviceNameParamTypeId).toString();
                    thing->setStateValue(garadgetConnectedStateTypeId, false);
                }
                if ( ((lastWillAvailable == false) && (thing->stateValue(garadgetConnectedStateTypeId).toBool() == true)) || m_statuscounter[thing] > lwtupdatetime) {
                    m_mqttClients.value(thing)->publish("garadget/" + thing->paramValue(garadgetThingDeviceNameParamTypeId).toString() + "/command", "get-status");
                }
            }
            if (m_statuscounter[thing] > lwtupdatetime) {
                m_statuscounter[thing] = 1;
            }
        });
        connect(thing, &Thing::settingChanged, this, [=](const ParamTypeId &settingTypeId){
            foreach (Thing *thing, myThings()) {
                QJsonObject garadgetobj;

                if ((thing->stateValue(garadgetConnectedStateTypeId).toBool() == true) && (settingTypeId == garadgetSettingsRdtParamTypeId)){
                    garadgetobj.insert("rdt", thing->setting(garadgetSettingsRdtParamTypeId).toInt());
                    garadgetobj.insert("rlp", thing->setting(garadgetSettingsRlpParamTypeId).toInt());
                    garadgetobj.insert("rlt",thing->setting(garadgetSettingsRltParamTypeId).toInt());
                    garadgetobj.insert("mtt", thing->setting(garadgetSettingsMttParamTypeId).toInt());
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
    qCDebug(dcGaradget) << "device " << thing->paramValue(garadgetThingDeviceNameParamTypeId).toString() << "Removed";
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
    if (conftype != "") {
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
    MqttClient* client = static_cast<MqttClient*>(sender());
    Thing *thing = m_mqttClients.key(client);
    if (!thing) {
        qCWarning(dcGaradget) << "Received a publish message from a client but don't have a matching thing" << retained;
        return;
    }
    if (topic.endsWith("/status")) {
        if (thing->stateValue(garadgetConnectedStateTypeId) == false) {
            qCDebug(dcGaradget) << "Setting" << thing->paramValue(garadgetThingDeviceNameParamTypeId).toString() << "to Online" ;
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
        thing->setStateValue(garadgetSignalStrengthStateTypeId, (90 + jo.value(QString("signal")).toInt()) * 2.4 );
        thing->setStateValue(garadgetSensorlevelStateTypeId, jo.value(QString("sensor")).toInt());
        thing->setStateValue(garadgetBrightlevelStateTypeId, jo.value(QString("bright")).toInt());
        if (jo.value(QString("status")).toString().contains(QString("stopped"))) {
            thing->setStateValue(garadgetStateStateTypeId, "intermediate");
            qCDebug(dcGaradget) << thing->paramValue(garadgetThingDeviceNameParamTypeId).toString() << "is" << jo.value(QString("status")).toString() ;
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
        thing->setSettingValue(garadgetSettingsRltParamTypeId,jo.value(QString("rlt")).toInt());
        thing->setSettingValue(garadgetSettingsMttParamTypeId,jo.value(QString("mtt")).toInt());
        thing->setSettingValue(garadgetSettingsRdtParamTypeId,jo.value(QString("rdt")).toInt());
        thing->setSettingValue(garadgetSettingsRlpParamTypeId,jo.value(QString("rlp")).toInt());
        qCDebug(dcGaradget) << thing->paramValue(garadgetThingDeviceNameParamTypeId).toString() << "System Configuration" << "srt =" << thing->stateValue(garadgetSrtStateTypeId).toInt() << "rlt =" << thing->setting(garadgetSettingsRltParamTypeId).toInt()<< "mtt =" << thing->setting(garadgetSettingsMttParamTypeId).toInt() << "rdt ="  << thing->setting(garadgetSettingsRdtParamTypeId).toUInt() << "rlp =" << thing->setting(garadgetSettingsRlpParamTypeId).toUInt();
    }
    if (topic.endsWith("/set-config")){
        if ( (payload.contains("mqip"))  or (payload.contains("mqpt")) or (payload.contains("nme")) ) {
            thing->setStateValue(garadgetConnectedStateTypeId, false);
            qCDebug(dcGaradget) << thing->paramValue(garadgetThingDeviceNameParamTypeId).toString() << "Detected change of Broker msg - set connected to false";
        }
    }
    if (topic.endsWith("/LWT")){
        if (payload.contains("Online") or payload.contains("Offline")) {
            qCDebug(dcGaradget()) << "Setting support for LWT";
            pluginStorage()->beginGroup(thing->id().toString());
            pluginStorage()->setValue("lastWillAvailable", true);
            pluginStorage()->endGroup();
        }
        if (payload.contains("Online")) {
            if (thing->stateValue(garadgetConnectedStateTypeId) == false) {
                qCDebug(dcGaradget) << "Setting" << thing->paramValue(garadgetThingDeviceNameParamTypeId).toString() << "to Online" ;
                thing->setStateValue(garadgetConnectedStateTypeId, true);
            }
        }
        if (payload.contains("Offline")) {
            if (thing->stateValue(garadgetConnectedStateTypeId) == true) {
                qCDebug(dcGaradget) << "Setting" << thing->paramValue(garadgetThingDeviceNameParamTypeId).toString() << "to Offline" ;
                thing->setStateValue(garadgetConnectedStateTypeId, false);
            }
        }
    }
}
