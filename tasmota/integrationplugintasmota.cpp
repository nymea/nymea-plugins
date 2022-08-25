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

#include "integrationplugintasmota.h"
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
    {"POWER", sonoff_basicPowerStateTypeId}, // On single channel devices, sometimes there's no index
    {"POWER1", sonoff_basicPowerStateTypeId},
};
static QHash<QString, StateTypeId> sonoff_dimmerPowerStateTypeIds = {
    {"POWER", sonoff_dimmerPowerStateTypeId}, // On single channel devices, sometimes there's no index
    {"POWER1", sonoff_dimmerPowerStateTypeId},
};
static QHash<QString, StateTypeId> sonoff_dualPowerStateTypeIds = {
    {"POWER1", sonoff_dualPowerCH1StateTypeId},
    {"POWER2", sonoff_dualPowerCH2StateTypeId},
};
static QHash<QString, StateTypeId> sonoff_triPowerStateTypeIds = {
    {"POWER1", sonoff_triPowerCH1StateTypeId},
    {"POWER2", sonoff_triPowerCH2StateTypeId},
    {"POWER3", sonoff_triPowerCH3StateTypeId},
};
static QHash<QString, StateTypeId> sonoff_quadPowerStateTypeIds = {
    {"POWER1", sonoff_quadPowerCH1StateTypeId},
    {"POWER2", sonoff_quadPowerCH2StateTypeId},
    {"POWER3", sonoff_quadPowerCH3StateTypeId},
    {"POWER4", sonoff_quadPowerCH4StateTypeId},
};
static QHash<ThingClassId, QHash<QString, StateTypeId>> stateMaps = {
    {sonoff_basicThingClassId, sonoff_basicPowerStateTypeIds},
    {sonoff_dualThingClassId, sonoff_dualPowerStateTypeIds},
    {sonoff_triThingClassId, sonoff_triPowerStateTypeIds},
    {sonoff_quadThingClassId, sonoff_quadPowerStateTypeIds},
    {sonoff_dimmerThingClassId, sonoff_dimmerPowerStateTypeIds}
};

IntegrationPluginTasmota::IntegrationPluginTasmota()
{
    // Helper maps for parent devices (aka sonoff_*)
    m_ipAddressParamTypeMap[sonoff_basicThingClassId] = sonoff_basicThingIpAddressParamTypeId;
    m_ipAddressParamTypeMap[sonoff_dualThingClassId] = sonoff_dualThingIpAddressParamTypeId;
    m_ipAddressParamTypeMap[sonoff_triThingClassId] = sonoff_triThingIpAddressParamTypeId;
    m_ipAddressParamTypeMap[sonoff_quadThingClassId] = sonoff_quadThingIpAddressParamTypeId;
    m_ipAddressParamTypeMap[sonoff_dimmerThingClassId] = sonoff_dimmerThingIpAddressParamTypeId;

    m_attachedDeviceParamTypeIdMap[sonoff_basicThingClassId] << sonoff_basicThingAttachedDeviceCH1ParamTypeId;
    m_attachedDeviceParamTypeIdMap[sonoff_dualThingClassId] << sonoff_dualThingAttachedDeviceCH1ParamTypeId << sonoff_dualThingAttachedDeviceCH2ParamTypeId;
    m_attachedDeviceParamTypeIdMap[sonoff_triThingClassId] << sonoff_triThingAttachedDeviceCH1ParamTypeId << sonoff_triThingAttachedDeviceCH2ParamTypeId << sonoff_triThingAttachedDeviceCH3ParamTypeId;
    m_attachedDeviceParamTypeIdMap[sonoff_quadThingClassId] << sonoff_quadThingAttachedDeviceCH1ParamTypeId << sonoff_quadThingAttachedDeviceCH2ParamTypeId << sonoff_quadThingAttachedDeviceCH3ParamTypeId << sonoff_quadThingAttachedDeviceCH4ParamTypeId;

    // Helper maps for virtual childs (aka tasmota*)
    m_channelParamTypeMap[tasmotaSwitchThingClassId] = tasmotaSwitchThingChannelNameParamTypeId;
    m_channelParamTypeMap[tasmotaLightThingClassId] = tasmotaLightThingChannelNameParamTypeId;
    m_openingChannelParamTypeMap[tasmotaShutterThingClassId] = tasmotaShutterThingOpeningChannelParamTypeId;
    m_closingChannelParamTypeMap[tasmotaShutterThingClassId] = tasmotaShutterThingClosingChannelParamTypeId;
    m_openingChannelParamTypeMap[tasmotaBlindsThingClassId] = tasmotaBlindsThingOpeningChannelParamTypeId;
    m_closingChannelParamTypeMap[tasmotaBlindsThingClassId] = tasmotaBlindsThingClosingChannelParamTypeId;

    m_closableOpenActionTypeMap[tasmotaShutterThingClassId] = tasmotaShutterOpenActionTypeId;
    m_closableCloseActionTypeMap[tasmotaShutterThingClassId] = tasmotaShutterCloseActionTypeId;
    m_closableStopActionTypeMap[tasmotaShutterThingClassId] = tasmotaShutterStopActionTypeId;

    m_closableOpenActionTypeMap[tasmotaBlindsThingClassId] = tasmotaBlindsOpenActionTypeId;
    m_closableCloseActionTypeMap[tasmotaBlindsThingClassId] = tasmotaBlindsCloseActionTypeId;
    m_closableStopActionTypeMap[tasmotaBlindsThingClassId] = tasmotaBlindsStopActionTypeId;
}

IntegrationPluginTasmota::~IntegrationPluginTasmota()
{
}

void IntegrationPluginTasmota::init()
{
}

void IntegrationPluginTasmota::setupThing(ThingSetupInfo *info)
{
    Thing *thing = info->thing();

    if (m_ipAddressParamTypeMap.contains(thing->thingClassId())) {
        ParamTypeId ipAddressParamTypeId = m_ipAddressParamTypeMap.value(thing->thingClassId());

        QHostAddress deviceAddress = QHostAddress(thing->paramValue(ipAddressParamTypeId).toString());
        if (deviceAddress.isNull()) {
            qCWarning(dcTasmota) << "Not a valid IP address given for IP address parameter";
            //: Error setting up thing
            return info->finish(Thing::ThingErrorInvalidParameter, QT_TR_NOOP("The given IP address is not valid."));
        }
        MqttChannel *channel = hardwareManager()->mqttProvider()->createChannel(thing->id().toString().remove(QRegExp("[{}-]")), deviceAddress);
        if (!channel) {
            qCWarning(dcTasmota) << "Failed to create MQTT channel.";
            //: Error setting up thing
            return info->finish(Thing::ThingErrorHardwareFailure, QT_TR_NOOP("Error creating MQTT channel. Please check MQTT server settings."));
        }

        QUrl url(QString("http://%1/cm").arg(deviceAddress.toString()));
        QUrlQuery query;
        QMap<QString, QString> configItems;
        configItems.insert("MqttHost", channel->serverAddress().toString());
        configItems.insert("MqttPort", QString::number(channel->serverPort()));
        configItems.insert("MqttClient", channel->clientId());
        configItems.insert("MqttUser", channel->username());
        configItems.insert("MqttPassword", channel->password());
        configItems.insert("Topic", "sonoff");
        configItems.insert("TelePeriod", thing->setting("telePeriod").toString());
        configItems.insert("FullTopic", channel->topicPrefixList().first() + "/%topic%/");

        QStringList configList;
        foreach (const QString &key, configItems.keys()) {
            configList << key + ' ' + configItems.value(key);
        }
        QString fullCommand = "Backlog " + configList.join(';');
        query.addQueryItem("cmnd", fullCommand.toUtf8().toPercentEncoding());


        url.setQuery(query);
        qCDebug(dcTasmota) << "Configuring Tasmota thing:" << url.toString();
        QNetworkRequest request(url);
        QNetworkReply *reply = hardwareManager()->networkManager()->get(request);
        connect(reply, &QNetworkReply::finished, reply, &QNetworkReply::deleteLater);
        connect(reply, &QNetworkReply::finished, info, [this, info, channel, reply, thing](){
            if (reply->error() != QNetworkReply::NoError) {
                qCDebug(dcTasmota) << "Sonoff thing setup call failed:" << reply->error() << reply->errorString() << reply->readAll();
                hardwareManager()->mqttProvider()->releaseChannel(channel);
                //: Error setting up thing
                info->finish(Thing::ThingErrorSetupFailed, QT_TR_NOOP("Could not connect to Tasmota device."));
                return;
            }
            m_mqttChannels.insert(info->thing(), channel);
            connect(channel, &MqttChannel::clientConnected, this, &IntegrationPluginTasmota::onClientConnected);
            connect(channel, &MqttChannel::clientDisconnected, this, &IntegrationPluginTasmota::onClientDisconnected);
            connect(channel, &MqttChannel::publishReceived, this, &IntegrationPluginTasmota::onPublishReceived);

            qCDebug(dcTasmota) << "Sonoff setup complete";
            info->finish(Thing::ThingErrorNoError);

            connect(thing, &Thing::settingChanged, this, [this, thing](){
                QUrl url(QString("http://%1/cm").arg(thing->paramValue("ipAddress").toString()));
                QUrlQuery query;
                QMap<QString, QString> configItems;
                configItems.insert("TelePeriod", thing->setting("telePeriod").toString());
                url.setQuery(query);
                qCDebug(dcTasmota) << "Configuring Tasmota thing:" << url.toString();
                QNetworkRequest request(url);
                QNetworkReply *reply = hardwareManager()->networkManager()->get(request);
                connect(reply, &QNetworkReply::finished, reply, [reply](){
                    reply->deleteLater();
                    qCInfo(dcTasmota()) << "Configuration on tasmota set with status:" << reply->error() << reply->errorString() << reply->readAll();
                });
            });

            foreach (Thing *child, myThings()) {
                if (child->parentId() == info->thing()->id()) {
                    // Already have child devices... We're done here
                    return;
                }
            }
            qCDebug(dcTasmota) << "Adding Tasmota Switch devices";
            QList<ThingDescriptor> deviceDescriptors;
            for (int i = 0; i < m_attachedDeviceParamTypeIdMap.value(info->thing()->thingClassId()).count(); i++) {
                ThingDescriptor descriptor(tasmotaSwitchThingClassId, info->thing()->name() + " CH" + QString::number(i+1), QString(), info->thing()->id());
                if (m_attachedDeviceParamTypeIdMap.value(info->thing()->thingClassId()).count() == 1) {
                    descriptor.setParams(ParamList() << Param(tasmotaSwitchThingChannelNameParamTypeId, "POWER"));
                } else {
                    descriptor.setParams(ParamList() << Param(tasmotaSwitchThingChannelNameParamTypeId, "POWER" + QString::number(i+1)));
                }
                deviceDescriptors << descriptor;
            }
            emit autoThingsAppeared(deviceDescriptors);

            qCDebug(dcTasmota) << "Adding Tasmota connected devices";
            deviceDescriptors.clear();
            int shutterUpChannel = -1;
            int shutterDownChannel = -1;
            int blindsUpChannel = -1;
            int blindsDownChannel = -1;
            for (int i = 0; i < m_attachedDeviceParamTypeIdMap.value(info->thing()->thingClassId()).count(); i++) {
                ParamTypeId attachedDeviceParamTypeId = m_attachedDeviceParamTypeIdMap.value(info->thing()->thingClassId()).at(i);
                QString deviceType = info->thing()->paramValue(attachedDeviceParamTypeId).toString();
                qCDebug(dcTasmota) << "Connected Device" << i + 1 << deviceType;
                if (deviceType == "Light") {
                    ThingDescriptor descriptor(tasmotaLightThingClassId, info->thing()->name() + " CH" + QString::number(i+1), QString(), info->thing()->id());
                    descriptor.setParentId(info->thing()->id());
                    if (m_attachedDeviceParamTypeIdMap.value(info->thing()->thingClassId()).count() == 1) {
                        descriptor.setParams(ParamList() << Param(tasmotaLightThingChannelNameParamTypeId, "POWER"));
                    } else {
                        descriptor.setParams(ParamList() << Param(tasmotaLightThingChannelNameParamTypeId, "POWER" + QString::number(i+1)));
                    }
                    deviceDescriptors << descriptor;
                } else if (deviceType == "Roller Shutter Up") {
                    shutterUpChannel = i+1;
                } else if (deviceType == "Roller Shutter Down") {
                    shutterDownChannel = i+1;
                } else if (deviceType == "Blinds Up") {
                    blindsUpChannel = i+1;
                } else if (deviceType == "Blinds Down") {
                    blindsDownChannel = i+1;
                }
            }
            if (shutterUpChannel != -1 && shutterDownChannel != -1) {
                qCDebug(dcTasmota) << "Adding Shutter device";
                ThingDescriptor descriptor(tasmotaShutterThingClassId, info->thing()->name() + " Shutter", QString(), info->thing()->id());
                descriptor.setParams(ParamList()
                                     << Param(tasmotaShutterThingOpeningChannelParamTypeId, "POWER" + QString::number(shutterUpChannel))
                                     << Param(tasmotaShutterThingClosingChannelParamTypeId, "POWER" + QString::number(shutterDownChannel)));
                deviceDescriptors << descriptor;
            }
            if (blindsUpChannel != -1 && blindsDownChannel != -1) {
                qCDebug(dcTasmota) << "Adding Blinds device";
                ThingDescriptor descriptor(tasmotaBlindsThingClassId, info->thing()->name() + " Blinds", QString(), info->thing()->id());
                descriptor.setParams(ParamList()
                                     << Param(tasmotaBlindsThingOpeningChannelParamTypeId, "POWER" + QString::number(blindsUpChannel))
                                     << Param(tasmotaBlindsThingClosingChannelParamTypeId, "POWER" + QString::number(blindsDownChannel)));
                deviceDescriptors << descriptor;
            }
            if (!deviceDescriptors.isEmpty()) {
                emit autoThingsAppeared(deviceDescriptors);
            }
        });
        return;
    }

    if (thing->hasState("connected")) {
        Thing* parentDevice = myThings().findById(thing->parentId());
        thing->setStateValue("connected", parentDevice->stateValue("connected"));
        return info->finish(Thing::ThingErrorNoError);
    }

    qCWarning(dcTasmota) << "Unhandled ThingClass in setupDevice" << thing->thingClassId();
}

void IntegrationPluginTasmota::thingRemoved(Thing *thing)
{
    qCDebug(dcTasmota) << "Device removed" << thing->name();
    if (m_mqttChannels.contains(thing)) {
        qCDebug(dcTasmota) << "Releasing MQTT channel";
        MqttChannel* channel = m_mqttChannels.take(thing);
        hardwareManager()->mqttProvider()->releaseChannel(channel);
    }
}

void IntegrationPluginTasmota::executeAction(ThingActionInfo *info)
{
    Thing *thing = info->thing();
    Action action = info->action();

    if (thing->thingClassId() == sonoff_basicThingClassId
            || thing->thingClassId() == sonoff_dualThingClassId
            || thing->thingClassId() == sonoff_triThingClassId
            || thing->thingClassId() == sonoff_quadThingClassId
            || action.actionTypeId() == sonoff_dimmerPowerActionTypeId) {
        MqttChannel *channel = m_mqttChannels.value(thing);
        if (!channel) {
            qCWarning(dcTasmota()) << "No MQTT channel for this thing.";
            info->finish(Thing::ThingErrorHardwareNotAvailable);
            return;
        }
        QString channelName = stateMaps.value(thing->thingClassId()).key(action.actionTypeId());
        QByteArray payload = action.paramValue(action.actionTypeId()).toBool() ? "ON" : "OFF";
        qCDebug(dcTasmota) << "Publishing:" << channel->topicPrefixList().first() + "/sonoff/cmnd/" + channelName << payload;
        channel->publish(channel->topicPrefixList().first() + "/sonoff/cmnd/" + channelName, payload);
        thing->setStateValue(action.actionTypeId(), action.paramValue(action.actionTypeId()));
        info->finish(Thing::ThingErrorNoError);
        return;
    }
    if (action.actionTypeId() == sonoff_dimmerBrightnessActionTypeId) {
        MqttChannel *channel = m_mqttChannels.value(thing);
        if (!channel) {
            qCWarning(dcTasmota()) << "No MQTT channel for this thing:" << thing->name();
            info->finish(Thing::ThingErrorHardwareNotAvailable);
            return;
        }
        QString channelName = stateMaps.value(thing->thingClassId()).key(action.actionTypeId());
        QByteArray payload = QByteArray::number(action.paramValue(action.actionTypeId()).toInt());
        qCDebug(dcTasmota) << "Publishing:" << channel->topicPrefixList().first() + "/sonoff/cmnd/DIMMER1" + channelName << payload;
        channel->publish(channel->topicPrefixList().first() + "/sonoff/cmnd/DIMMER1" + channelName, payload);
        thing->setStateValue(action.actionTypeId(), action.paramValue(action.actionTypeId()));
        info->finish(Thing::ThingErrorNoError);
        return;
    }

    // Legacy (deprecated) connected devices
    if (thing->hasState("power")) {
        Thing *parentDev = myThings().findById(thing->parentId());
        MqttChannel *channel = m_mqttChannels.value(parentDev);
        if (!channel) {
            qCWarning(dcTasmota()) << "No mqtt channel for this thing.";
            return info->finish(Thing::ThingErrorHardwareNotAvailable);
        }
        ParamTypeId channelParamTypeId = m_channelParamTypeMap.value(thing->thingClassId());
        ParamTypeId powerActionParamTypeId = thing->thingClass().actionTypes().findByName("power").paramTypes().findByName("power").id();
        qCDebug(dcTasmota) << "Publishing:" << channel->topicPrefixList().first() + "/sonoff/cmnd/" + thing->paramValue(channelParamTypeId).toString() << (action.param(powerActionParamTypeId).value().toBool() ? "ON" : "OFF");
        channel->publish(channel->topicPrefixList().first() + "/sonoff/cmnd/" + thing->paramValue(channelParamTypeId).toString().toLower(), action.param(powerActionParamTypeId).value().toBool() ? "ON" : "OFF");
        thing->setStateValue("power", action.param(powerActionParamTypeId).value().toBool());
        return info->finish(Thing::ThingErrorNoError);
    }
    if (m_closableStopActionTypeMap.contains(thing->thingClassId())) {
        Thing *parentDev = myThings().findById(thing->parentId());
        MqttChannel *channel = m_mqttChannels.value(parentDev);
        if (!channel) {
            qCWarning(dcTasmota()) << "No mqtt channel for this thing.";
            return info->finish(Thing::ThingErrorHardwareNotAvailable);
        }
        ParamTypeId openingChannelParamTypeId = m_openingChannelParamTypeMap.value(thing->thingClassId());
        ParamTypeId closingChannelParamTypeId = m_closingChannelParamTypeMap.value(thing->thingClassId());
        if (action.actionTypeId() == m_closableOpenActionTypeMap.value(thing->thingClassId())) {
            qCDebug(dcTasmota) << "Publishing:" << channel->topicPrefixList().first() + "/sonoff/cmnd/" + thing->paramValue(closingChannelParamTypeId).toString() << "OFF";
            channel->publish(channel->topicPrefixList().first() + "/sonoff/cmnd/" + thing->paramValue(closingChannelParamTypeId).toString().toLower(), "OFF");
            qCDebug(dcTasmota) << "Publishing:" << channel->topicPrefixList().first() + "/sonoff/cmnd/" + thing->paramValue(openingChannelParamTypeId).toString() << "ON";
            channel->publish(channel->topicPrefixList().first() + "/sonoff/cmnd/" + thing->paramValue(openingChannelParamTypeId).toString().toLower(), "ON");
        } else if (action.actionTypeId() == m_closableCloseActionTypeMap.value(thing->thingClassId())) {
            qCDebug(dcTasmota) << "Publishing:" << channel->topicPrefixList().first() + "/sonoff/cmnd/" + thing->paramValue(openingChannelParamTypeId).toString() << "OFF";
            channel->publish(channel->topicPrefixList().first() + "/sonoff/cmnd/" + thing->paramValue(openingChannelParamTypeId).toString().toLower(), "OFF");
            qCDebug(dcTasmota) << "Publishing:" << channel->topicPrefixList().first() + "/sonoff/cmnd/" + thing->paramValue(closingChannelParamTypeId).toString() << "ON";
            channel->publish(channel->topicPrefixList().first() + "/sonoff/cmnd/" + thing->paramValue(closingChannelParamTypeId).toString().toLower(), "ON");
        } else { // Stop
            qCDebug(dcTasmota) << "Publishing:" << channel->topicPrefixList().first() + "/sonoff/cmnd/" + thing->paramValue(openingChannelParamTypeId).toString() << "OFF";
            channel->publish(channel->topicPrefixList().first() + "/sonoff/cmnd/" + thing->paramValue(openingChannelParamTypeId).toString().toLower(), "OFF");
            qCDebug(dcTasmota) << "Publishing:" << channel->topicPrefixList().first() + "/sonoff/cmnd/" + thing->paramValue(closingChannelParamTypeId).toString() << "OFF";
            channel->publish(channel->topicPrefixList().first() + "/sonoff/cmnd/" + thing->paramValue(closingChannelParamTypeId).toString().toLower(), "OFF");
        }
        return info->finish(Thing::ThingErrorNoError);
    }
    qCWarning(dcTasmota) << "Unhandled execute action call for devie" << thing;
}

void IntegrationPluginTasmota::onClientConnected(MqttChannel *channel)
{
    qCDebug(dcTasmota) << "Sonoff thing connected!";
    Thing *dev = m_mqttChannels.key(channel);
    dev->setStateValue("connected", true);

    foreach (Thing *child, myThings()) {
        if (child->parentId() == dev->id()) {
            child->setStateValue("connected", true);
        }
    }
}

void IntegrationPluginTasmota::onClientDisconnected(MqttChannel *channel)
{
    qCDebug(dcTasmota) << "Sonoff thing disconnected!";
    Thing *dev = m_mqttChannels.key(channel);
    dev->setStateValue("connected", false);

    foreach (Thing *child, myThings()) {
        if (child->parentId() == dev->id()) {
            child->setStateValue("connected", false);
        }
    }
}

void IntegrationPluginTasmota::onPublishReceived(MqttChannel *channel, const QString &topic, const QByteArray &payload)
{
    qCDebug(dcTasmota) << "Publish received from Sonoff thing:" << topic << qUtf8Printable(payload);
    Thing *thing = m_mqttChannels.key(channel);
    if (topic.startsWith(channel->topicPrefixList().first() + "/sonoff/POWER")) {
        QString channelName = topic.split("/").last();

        thing->setStateValue(stateMaps.value(thing->thingClassId()).value(channelName), payload == "ON");

        // Legacy (deprecated) connected things via params
        foreach (Thing *child, myThings().filterByParentId(thing->id())) {
            if (child->paramValue(m_channelParamTypeMap.value(child->thingClassId())).toString() != channelName) {
                continue;
            }
            if (child->hasState("power")) {
                child->setStateValue("power", payload == "ON");
            }
            if (child->thingClassId() == tasmotaSwitchThingClassId) {
                Event event(tasmotaSwitchPressedEventTypeId, child->id());
                emit emitEvent(event);
            }
        }
    }
    if (topic.startsWith(channel->topicPrefixList().first() + "/sonoff/STATE")) {
        QJsonParseError error;
        QJsonDocument jsonDoc = QJsonDocument::fromJson(payload, &error);
        if (error.error != QJsonParseError::NoError) {
            qCWarning(dcTasmota) << "Cannot parse JSON from Tasmota device" << error.errorString();
            return;
        }
        QVariantMap dataMap = jsonDoc.toVariant().toMap();
        thing->setStateValue("signalStrength", dataMap.value("Wifi").toMap().value("RSSI").toInt());

        if (thing->hasState("brightness")) {
            thing->setStateValue("brightness", dataMap.value("Dimmer").toInt());
        }

        // Legacy (deprecated) connected things by params
        foreach (Thing *child, myThings().filterByParentId(thing->id())) {
            if (child->hasState("power")) {
                QString childChannel = child->paramValue(m_channelParamTypeMap.value(child->thingClassId())).toString();
                QString valueString = jsonDoc.toVariant().toMap().value(childChannel).toString();
                child->setStateValue("power", valueString == "ON");
            }
            child->setStateValue("signalStrength", dataMap.value("Wifi").toMap().value("RSSI").toInt());
        }
    }
    if (topic.startsWith(channel->topicPrefixList().first() + "/sonoff/SENSOR")) {
        QJsonParseError error;
        QJsonDocument jsonDoc = QJsonDocument::fromJson(payload, &error);
        if (error.error != QJsonParseError::NoError) {
            qCWarning(dcTasmota) << "Cannot parse JSON from Tasmota device" << error.errorString();
            return;
        }
        QVariantMap dataMap = jsonDoc.toVariant().toMap();

        if (dataMap.contains("ENERGY")) {
            QVariantMap energyMap = dataMap.value("ENERGY").toMap();
            QString channelName = topic.split("/").last();

            // If we received energy meter values but don't have a power meter child yet, create one
            Thing *meter = myThings().filterByParentId(thing->id()).filterByInterface("smartmeterconsumer").findByParams({Param(powerMeterChannelThingChannelNameParamTypeId, channelName)});
            if (!meter) {
                ThingDescriptor descriptor(powerMeterChannelThingClassId, thing->name(), QString(), thing->id());
                descriptor.setParams({Param(powerMeterChannelThingChannelNameParamTypeId, channelName)});
                emit autoThingsAppeared({descriptor});
                return;
            }
            meter->setStateValue("currentPower", energyMap.value("Power").toDouble());
            meter->setStateValue("totalEnergyConsumed", energyMap.value("Total").toDouble());
        }
    }
}

