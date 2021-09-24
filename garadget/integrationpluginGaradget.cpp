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

#include "integrationpluginGaradget.h"
#include "plugininfo.h"

#include <QUrlQuery>
#include <QNetworkReply>
#include <QHostAddress>
#include <QJsonDocument>

#include "hardwaremanager.h"
#include "network/networkaccessmanager.h"
#include "network/mqtt/mqttprovider.h"
#include "network/mqtt/mqttchannel.h"

static QHash<QString, StateTypeId> sonoff_basicPowerStateTypeIds = {
    {"POWER1", sonoff_basicPowerStateTypeId},
};
static QHash<ThingClassId, QHash<QString, StateTypeId>> stateMaps = {
    {sonoff_basicThingClassId, sonoff_basicPowerStateTypeIds},
};

IntegrationPluginGaradget::IntegrationPluginGaradget()
{
    // Helper maps for parent devices (aka sonoff_*)
    m_ipAddressParamTypeMap[sonoff_basicThingClassId] = sonoff_basicThingIpAddressParamTypeId;


    // Helper maps for all devices
    m_connectedStateTypeMap[sonoff_basicThingClassId] = sonoff_basicConnectedStateTypeId;

    m_signalStrengthStateTypeMap[sonoff_basicThingClassId] = sonoff_basicSignalStrengthStateTypeId;
}

IntegrationPluginGaradget::~IntegrationPluginGaradget()
{
}

void IntegrationPluginGaradget::init()
{
}

void IntegrationPluginGaradget::setupThing(ThingSetupInfo *info)
{
    Thing *thing = info->thing();

    if (m_ipAddressParamTypeMap.contains(thing->thingClassId())) {
        ParamTypeId ipAddressParamTypeId = m_ipAddressParamTypeMap.value(thing->thingClassId());

        QHostAddress deviceAddress = QHostAddress(thing->paramValue(ipAddressParamTypeId).toString());
        if (deviceAddress.isNull()) {
            qCWarning(dcGaradget) << "Not a valid IP address given for IP address parameter";
            //: Error setting up thing
            return info->finish(Thing::ThingErrorInvalidParameter, QT_TR_NOOP("The given IP address is not valid."));
        }
        MqttChannel *channel = hardwareManager()->mqttProvider()->createChannel(thing->id().toString().remove(QRegExp("[{}-]")), deviceAddress );
        if (!channel) {
            qCWarning(dcGaradget) << "Failed to create MQTT channel.";
            //: Error setting up thing
            return info->finish(Thing::ThingErrorHardwareFailure, QT_TR_NOOP("Error creating MQTT channel. Please check MQTT server settings."));
        }
// ipaddress is defined here but not sure how to get rid of and still have the mqtt channel. Garadget only works with the broker.

        qCDebug(dcGaradget) << "BEE initial setup instructions:" << deviceAddress.toString() << channel;

        m_mqttChannels.insert(info->thing(), channel);
        connect(channel, &MqttChannel::clientConnected, this, &IntegrationPluginGaradget::onClientConnected);
        connect(channel, &MqttChannel::clientDisconnected, this, &IntegrationPluginGaradget::onClientDisconnected);
        connect(channel, &MqttChannel::publishReceived, this, &IntegrationPluginGaradget::onPublishReceived);

        qCDebug(dcGaradget) << "Garadget setup complete";
        info->finish(Thing::ThingErrorNoError);
        foreach (Thing *child, myThings()) {
            if (child->parentId() == info->thing()->id()) {
                // Already have child devices... We're done here
                return;
            }
        }

// insert to reprogram client Garadget to new device/topic name
        QJsonObject beeobj;
        beeobj.insert("nme", QString(channel->topicPrefixList().first()));
        QJsonDocument beedoc(beeobj);
        QByteArray beedata = beedoc.toJson(QJsonDocument::Compact);
        QString jsonString(beedata);
        qCDebug(dcGaradget) << "Publishing:" << "garadget/garage/set-conf " << beedata;
// next line will not publish to the correct device as topic is 32 characters long and garadget only allows 30 characters.
        channel->publish("garadget/garaged/set-config", beedata);
        channel->publish("garadget/garage/command", "get-config");
// need to develop test for correct modification of nme
        return;
    }

    if (m_connectedStateTypeMap.contains(thing->thingClassId())) {
        Thing* parentDevice = myThings().findById(thing->parentId());
        StateTypeId connectedStateTypeId = m_connectedStateTypeMap.value(thing->thingClassId());
        thing->setStateValue(m_connectedStateTypeMap.value(thing->thingClassId()), parentDevice->stateValue(connectedStateTypeId));
        return info->finish(Thing::ThingErrorNoError);
    }

    qCWarning(dcGaradget) << "Unhandled ThingClass in setupDevice" << thing->thingClassId();
}

void IntegrationPluginGaradget::thingRemoved(Thing *thing)
{
    qCDebug(dcGaradget) << "Device removed" << thing->name();
    if (m_mqttChannels.contains(thing)) {
        qCDebug(dcGaradget) << "Releasing MQTT channel";
        MqttChannel* channel = m_mqttChannels.take(thing);
        hardwareManager()->mqttProvider()->releaseChannel(channel);
    }
}

void IntegrationPluginGaradget::executeAction(ThingActionInfo *info)
{
    Thing *thing = info->thing();
    Action action = info->action();

    if (thing->thingClassId() == sonoff_basicThingClassId) {
        MqttChannel *channel = m_mqttChannels.value(thing);
        if (!channel) {
            qCWarning(dcGaradget()) << "No MQTT channel for this thing.";
            info->finish(Thing::ThingErrorHardwareNotAvailable);
            return;
        }
        QString channelName = stateMaps.value(thing->thingClassId()).key(action.actionTypeId());
        QByteArray payload = action.paramValue(action.actionTypeId()).toBool() ? "ON" : "OFF";
        qCDebug(dcGaradget) << "Publishing:" << channel->topicPrefixList().first() + "/sonoff/cmnd/" + channelName << payload;
        channel->publish(channel->topicPrefixList().first() + "/sonoff/cmnd/" + channelName, payload);
        thing->setStateValue(action.actionTypeId(), action.paramValue(action.actionTypeId()));
        info->finish(Thing::ThingErrorNoError);
        return;
    }

    // Legacy (deprecated) connected devices
    if (m_powerStateTypeMap.contains(thing->thingClassId())) {
        Thing *parentDev = myThings().findById(thing->parentId());
        MqttChannel *channel = m_mqttChannels.value(parentDev);
        if (!channel) {
            qCWarning(dcGaradget()) << "No mqtt channel for this thing.";
            return info->finish(Thing::ThingErrorHardwareNotAvailable);
        }
        ParamTypeId channelParamTypeId = m_channelParamTypeMap.value(thing->thingClassId());
        ParamTypeId powerActionParamTypeId = ParamTypeId(m_powerStateTypeMap.value(thing->thingClassId()).toString());
        qCDebug(dcGaradget) << "Publishing:" << channel->topicPrefixList().first() + "/sonoff/cmnd/" + thing->paramValue(channelParamTypeId).toString() << (action.param(powerActionParamTypeId).value().toBool() ? "ON" : "OFF");
        channel->publish(channel->topicPrefixList().first() + "/sonoff/cmnd/" + thing->paramValue(channelParamTypeId).toString().toLower(), action.param(powerActionParamTypeId).value().toBool() ? "ON" : "OFF");
        thing->setStateValue(m_powerStateTypeMap.value(thing->thingClassId()), action.param(powerActionParamTypeId).value().toBool());
        return info->finish(Thing::ThingErrorNoError);
    }
    if (m_closableStopActionTypeMap.contains(thing->thingClassId())) {
        Thing *parentDev = myThings().findById(thing->parentId());
        MqttChannel *channel = m_mqttChannels.value(parentDev);
        if (!channel) {
            qCWarning(dcGaradget()) << "No mqtt channel for this thing.";
            return info->finish(Thing::ThingErrorHardwareNotAvailable);
        }
        ParamTypeId openingChannelParamTypeId = m_openingChannelParamTypeMap.value(thing->thingClassId());
        ParamTypeId closingChannelParamTypeId = m_closingChannelParamTypeMap.value(thing->thingClassId());
        if (action.actionTypeId() == m_closableOpenActionTypeMap.value(thing->thingClassId())) {
            qCDebug(dcGaradget) << "Publishing:" << channel->topicPrefixList().first() + "/sonoff/cmnd/" + thing->paramValue(closingChannelParamTypeId).toString() << "OFF";
            channel->publish(channel->topicPrefixList().first() + "/sonoff/cmnd/" + thing->paramValue(closingChannelParamTypeId).toString().toLower(), "OFF");
            qCDebug(dcGaradget) << "Publishing:" << channel->topicPrefixList().first() + "/sonoff/cmnd/" + thing->paramValue(openingChannelParamTypeId).toString() << "ON";
            channel->publish(channel->topicPrefixList().first() + "/sonoff/cmnd/" + thing->paramValue(openingChannelParamTypeId).toString().toLower(), "ON");
        } else if (action.actionTypeId() == m_closableCloseActionTypeMap.value(thing->thingClassId())) {
            qCDebug(dcGaradget) << "Publishing:" << channel->topicPrefixList().first() + "/sonoff/cmnd/" + thing->paramValue(openingChannelParamTypeId).toString() << "OFF";
            channel->publish(channel->topicPrefixList().first() + "/sonoff/cmnd/" + thing->paramValue(openingChannelParamTypeId).toString().toLower(), "OFF");
            qCDebug(dcGaradget) << "Publishing:" << channel->topicPrefixList().first() + "/sonoff/cmnd/" + thing->paramValue(closingChannelParamTypeId).toString() << "ON";
            channel->publish(channel->topicPrefixList().first() + "/sonoff/cmnd/" + thing->paramValue(closingChannelParamTypeId).toString().toLower(), "ON");
        } else { // Stop
            qCDebug(dcGaradget) << "Publishing:" << channel->topicPrefixList().first() + "/sonoff/cmnd/" + thing->paramValue(openingChannelParamTypeId).toString() << "OFF";
            channel->publish(channel->topicPrefixList().first() + "/sonoff/cmnd/" + thing->paramValue(openingChannelParamTypeId).toString().toLower(), "OFF");
            qCDebug(dcGaradget) << "Publishing:" << channel->topicPrefixList().first() + "/sonoff/cmnd/" + thing->paramValue(closingChannelParamTypeId).toString() << "OFF";
            channel->publish(channel->topicPrefixList().first() + "/sonoff/cmnd/" + thing->paramValue(closingChannelParamTypeId).toString().toLower(), "OFF");
        }
        return info->finish(Thing::ThingErrorNoError);
    }
    qCWarning(dcGaradget) << "Unhandled execute action call for devie" << thing;
}

void IntegrationPluginGaradget::onClientConnected(MqttChannel *channel)
{
    qCDebug(dcGaradget) << "Sonoff thing connected!";
    Thing *dev = m_mqttChannels.key(channel);
    dev->setStateValue(m_connectedStateTypeMap.value(dev->thingClassId()), true);

    foreach (Thing *child, myThings()) {
        if (child->parentId() == dev->id()) {
            child->setStateValue(m_connectedStateTypeMap.value(child->thingClassId()), true);
        }
    }
}

void IntegrationPluginGaradget::onClientDisconnected(MqttChannel *channel)
{
    qCDebug(dcGaradget) << "Sonoff thing disconnected!";
    Thing *dev = m_mqttChannels.key(channel);
    dev->setStateValue(m_connectedStateTypeMap.value(dev->thingClassId()), false);

    foreach (Thing *child, myThings()) {
        if (child->parentId() == dev->id()) {
            child->setStateValue(m_connectedStateTypeMap.value(child->thingClassId()), false);
        }
    }
}

void IntegrationPluginGaradget::onPublishReceived(MqttChannel *channel, const QString &topic, const QByteArray &payload)
{
    qCDebug(dcGaradget) << "Publish received from Sonoff thing:" << topic << qUtf8Printable(payload);
    Thing *thing = m_mqttChannels.key(channel);
    if (topic.startsWith(channel->topicPrefixList().first() + "/sonoff/POWER")) {
        QString channelName = topic.split("/").last();

        thing->setStateValue(stateMaps.value(thing->thingClassId()).value(channelName), payload == "ON");

        // Legacy (deprecated) connected things via params
        foreach (Thing *child, myThings().filterByParentId(thing->id())) {
            if (child->paramValue(m_channelParamTypeMap.value(child->thingClassId())).toString() != channelName) {
                continue;
            }
            if (m_powerStateTypeMap.contains(child->thingClassId())) {
                child->setStateValue(m_powerStateTypeMap.value(child->thingClassId()), payload == "ON");
            }
        }
    }
    if (topic.startsWith(channel->topicPrefixList().first() + "/sonoff/STATE")) {
        QJsonParseError error;
        QJsonDocument jsonDoc = QJsonDocument::fromJson(payload, &error);
        if (error.error != QJsonParseError::NoError) {
            qCWarning(dcGaradget) << "Cannot parse JSON from Garadget device" << error.errorString();
            return;
        }
        QVariantMap dataMap = jsonDoc.toVariant().toMap();
        thing->setStateValue(m_signalStrengthStateTypeMap.value(thing->thingClassId()), dataMap.value("Wifi").toMap().value("RSSI").toInt());

        if (m_brightnessStateTypeMap.contains(thing->thingClassId())) {
            thing->setStateValue(m_brightnessStateTypeMap.value(thing->thingClassId()), dataMap.value("Dimmer").toInt());
        }

        // Legacy (deprecated) connected things by params
        foreach (Thing *child, myThings().filterByParentId(thing->id())) {
            if (m_powerStateTypeMap.contains(child->thingClassId())) {
                QString childChannel = child->paramValue(m_channelParamTypeMap.value(child->thingClassId())).toString();
                QString valueString = jsonDoc.toVariant().toMap().value(childChannel).toString();
                child->setStateValue(m_powerStateTypeMap.value(child->thingClassId()), valueString == "ON");
            }
            child->setStateValue(m_signalStrengthStateTypeMap.value(child->thingClassId()), dataMap.value("Wifi").toMap().value("RSSI").toInt());
        }
    }
}

