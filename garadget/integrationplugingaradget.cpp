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
#include "plugininfo.h"

#include <QUrlQuery>
#include <QNetworkReply>
#include <QHostAddress>
#include <QJsonDocument>

#include "hardwaremanager.h"
#include "network/networkaccessmanager.h"
#include "network/mqtt/mqttprovider.h"
#include "network/mqtt/mqttchannel.h"

void IntegrationPluginGaradget::setupThing(ThingSetupInfo *info)
{
    Thing *thing = info->thing();

    qCDebug(dcGaradget) << "entered setupThing";
    QHostAddress deviceAddress = QHostAddress(thing->paramValue(garadgetThingIpAddressParamTypeId).toString());
    // ipaddress is defined here but not sure how to get rid of and still have the mqtt channel. Garadget only works with the broker.
    // checking on possibility of http on garadget.

    if (deviceAddress.isNull()) {
        qCWarning(dcGaradget) << "Not a valid IP address given for IP address parameter";
        //: Error setting up thing
        return info->finish(Thing::ThingErrorInvalidParameter, QT_TR_NOOP("The given IP address is not valid."));
    }

    // Note from Michael: Does the clientId really need to be "garage"? Note that client ids must be unique on a single
    // right now changing nme over the broker is the method to change the topic to garadget/uuid that nymea likes.
    // but if you delete the device from nymea, the topic is lost and you have to go to the device Wifi AP to fix.
    // Painful unless you monitor mosquitto_sub to fix by mosquitto_pub unless you tell me a better way to restore to a known value.
    // So we will see the path to follow based on the Garadget response.
    MqttChannel *channel = hardwareManager()->mqttProvider()->createChannel(thing->id().toString().remove(QRegExp("[{}-]")), deviceAddress );
    if (!channel) {
        qCWarning(dcGaradget) << "Failed to create MQTT channel.";
        //: Error setting up thing
        return info->finish(Thing::ThingErrorHardwareFailure, QT_TR_NOOP("Error creating MQTT channel. Please check MQTT server settings."));
    }

    qCDebug(dcGaradget) << "initial setup instructions:" << deviceAddress.toString() << channel;

/*
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

    QNetworkRequest request(url);
    QNetworkReply *reply = hardwareManager()->networkManager()->get(request);
    connect(reply, &QNetworkReply::finished, reply, &QNetworkReply::deleteLater);
    connect(reply, &QNetworkReply::finished, info, [this, info, channel, reply](){
        if (reply->error() != QNetworkReply::NoError) {
            qCDebug(dcTasmota) << "Sonoff thing setup call failed:" << reply->error() << reply->errorString() << reply->readAll();
            hardwareManager()->mqttProvider()->releaseChannel(channel);
              //: Error setting up thing
            info->finish(Thing::ThingErrorSetupFailed, QT_TR_NOOP("Could not connect to Tasmota device."));
            return;
         }
*/
    m_mqttChannels.insert(thing, channel);
    connect(channel, &MqttChannel::clientConnected, this, &IntegrationPluginGaradget::onClientConnected);
    connect(channel, &MqttChannel::clientDisconnected, this, &IntegrationPluginGaradget::onClientDisconnected);
    connect(channel, &MqttChannel::publishReceived, this, &IntegrationPluginGaradget::onPublishReceived);

// insert to reprogram client Garadget to new device/topic name    
// trick till we figure out the http setup or whatever
    QJsonObject garadgetobj;
    garadgetobj.insert("nme", QString(channel->topicPrefixList().first()));
    QJsonDocument garadgetdoc(garadgetobj);
    QByteArray garadgetdata = garadgetdoc.toJson(QJsonDocument::Compact);
    QString jsonString(garadgetdata);
    qCDebug(dcGaradget) << "Setting Garadget topic to:" <<  channel->topicPrefixList().first() << garadgetdata;
    channel->publish("garadget/garage/set-config", garadgetdata);
    qCDebug(dcGaradget) << "Garadget setup complete";


    info->finish(Thing::ThingErrorNoError);
}

void IntegrationPluginGaradget::thingRemoved(Thing *thing)
{
    qCDebug(dcGaradget) << "Device removed" << thing->name();
    qCDebug(dcGaradget) << "Releasing MQTT channel";
    MqttChannel* channel = m_mqttChannels.take(thing);
    hardwareManager()->mqttProvider()->releaseChannel(channel);
}

void IntegrationPluginGaradget::executeAction(ThingActionInfo *info)
{
    Thing *thing = info->thing();
    Action action = info->action();

    qCDebug(dcGaradget) << "executeAction function" << (thing->thingClassId());

    MqttChannel *channel = m_mqttChannels.value(info->thing());
    if (!channel) {
        qCWarning(dcGaradget()) << "No mqtt channel for this thing.";
        return info->finish(Thing::ThingErrorHardwareNotAvailable);
    }


    if (action.actionTypeId() == garadgetOpenActionTypeId) {
        qCDebug(dcGaradget) << "Publishing:" << "garadget/" + channel->topicPrefixList().first() + "/command" << "open";
        channel->publish("garadget/" + channel->topicPrefixList().first() + "/command", "open");
//            qCDebug(dcGaradget) << "OpenAction stuff:" << action.actionTypeId() << action.paramValue(action.actionTypeId());
//            qCDebug(dcGaradget) << "OpenMapAction stuff:" << action.paramValue(action.actionTypeId()) << stateOpeningMaps.value(thing->thingClassId()).key(action.actionTypeId());
//            thing->setStateValue(action.actionTypeId(), action.paramValue(action.actionTypeId()));
        info->finish(Thing::ThingErrorNoError);
        return;
    }
    if (action.actionTypeId() == garadgetCloseActionTypeId) {
        qCDebug(dcGaradget) << "Publishing:" << "garadget/" + channel->topicPrefixList().first() + "/command" << "close";
        channel->publish("garadget/" + channel->topicPrefixList().first() + "/command", "close");
//            qCDebug(dcGaradget) << "CloseAction stuff:" << thing << thing->thingClassId() << thing->thingClassId();
//            qCDebug(dcGaradget) << "CloseAction newparameter" << thing->paramValue(stateStateMAPS) ;
//            thing->setStateValue(stateStateMAPS.value(thing->thingClassId()).key(action.actionTypeId()),"closed");
//            qCDebug(dcGaradget) << "CloseMapAction stuff:" << stateStateMAPS.value(thing->thingClassId()).key(action.actionTypeId());
//            thing->setStateValue(action.actionTypeId(), action.paramValue(action.actionTypeId()));
        info->finish(Thing::ThingErrorNoError);
        return;
    }
    if (action.actionTypeId() == garadgetStopActionTypeId) {
        qCDebug(dcGaradget) << "Publishing:" << "garadget/" + channel->topicPrefixList().first() + "/command" << "stop";
        channel->publish("garadget/" + channel->topicPrefixList().first() + "/command", "stop");
//            qCDebug(dcGaradget) << "StopAction stuff:" << action.actionTypeId() << action.paramValue(action.actionTypeId());
//            qCDebug(dcGaradget) << "StopMapAction stuff:" << action.paramValue(action.actionTypeId()) << stateMovingMaps.value(thing->thingClassId()).key(action.actionTypeId());
//            thing->setStateValue(action.actionTypeId(), action.paramValue(action.actionTypeId()));
        info->finish(Thing::ThingErrorNoError);
        return;
    }

    qCWarning(dcGaradget) << "Unhandled execute action call for garadget thing" << thing;
}

void IntegrationPluginGaradget::onClientConnected(MqttChannel *channel)
{
    qCDebug(dcGaradget) << "Garadget thing connected!";
    Thing *dev = m_mqttChannels.key(channel);
    dev->setStateValue(garadgetConnectedStateTypeId, true);
}

void IntegrationPluginGaradget::onClientDisconnected(MqttChannel *channel)
{
    qCDebug(dcGaradget) << "Garadget thing disconnected!";
    Thing *dev = m_mqttChannels.key(channel);
    dev->setStateValue(garadgetConnectedStateTypeId, false);
}

void IntegrationPluginGaradget::onPublishReceived(MqttChannel *channel, const QString &topic, const QByteArray &payload)
{
    qCDebug(dcGaradget) << "Publish received from garadget thing:" << topic << qUtf8Printable(payload);
    Thing *thing = m_mqttChannels.key(channel);

    if (topic.startsWith("garadget/" + channel->topicPrefixList().first() + "/status")) {
        QJsonParseError error;
        QJsonDocument jsonDoc = QJsonDocument::fromJson(payload, &error);
        if (error.error != QJsonParseError::NoError) {
            qCWarning(dcGaradget) << "Cannot parse JSON from Garadget device" << error.errorString();
            return;
        }
        QJsonObject jo = jsonDoc.object();
        if (jo.value(QString("status")).toString().contains(QString("stopped"))) {
            thing->setStateValue(garadgetStateStateTypeId, "intermediate");
            qCDebug(dcGaradget) << "Garadget status value " << "intermediate" ;
            thing->setStateValue(garadgetClosingOutputStateTypeId, false);
// this set the states of Percentage state based on the status message from Garadget to show door in intermediate state
            thing->setStateValue(garadgetPercentageStateTypeId,50);
        } else {
            thing->setStateValue(garadgetStateStateTypeId, jo.value(QString("status")).toString());
            qCDebug(dcGaradget) << "Garadget status value " << jo.value(QString("status")).toString() ;

// these set the states of Opening & Closing Output states based on the status message from Garadget
// Percentage State set not as slick as your timer method but it does work
            if (jo.value(QString("status")).toString().contains(QString("opening"))) {
                thing->setStateValue(garadgetOpeningOutputStateTypeId, true);
                qCDebug(dcGaradget) << "Garadget OpeningOutput " << "true" ;
            } else {
                if (jo.value(QString("status")).toString().contains(QString("open"))) {
                    thing->setStateValue(garadgetOpeningOutputStateTypeId, false);
                    thing->setStateValue(garadgetPercentageStateTypeId,0);
                    qCDebug(dcGaradget) << "Garadget OpeningOutput " << "false" ;
                }
            }
            if (jo.value(QString("status")).toString().contains(QString("closing"))) {
                thing->setStateValue(garadgetClosingOutputStateTypeId, true);
                qCDebug(dcGaradget) << "Garadget ClosingOutput " << "true" ;
            }
            if (jo.value(QString("status")).toString().contains(QString("closed"))) {
                thing->setStateValue(garadgetClosingOutputStateTypeId, false);
                thing->setStateValue(garadgetPercentageStateTypeId, 100);
                qCDebug(dcGaradget) << "Garadget CloseingOutput " << "false" ;
            }
        }
    }

}
