/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*
* Copyright 2013 - 2022, nymea GmbH
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

#include "integrationpluginespuino.h"

#include "integrations/integrationplugin.h"

#include "plugininfo.h"

#include "network/networkaccessmanager.h"
#include "network/mqtt/mqttprovider.h"
#include "network/mqtt/mqttchannel.h"
#include "network/zeroconf/zeroconfservicebrowser.h"
#include "platform/platformzeroconfcontroller.h"

#include <mqttclient.h>

#include <QWebSocket>
#include <QJsonDocument>
#include <QNetworkReply>
#include <QUrlQuery>

void IntegrationPluginEspuino::init()
{
    m_zeroConfBrowser = hardwareManager()->zeroConfController()->createServiceBrowser("_http._tcp");
}

void IntegrationPluginEspuino::discoverThings(ThingDiscoveryInfo *info)
{
    foreach (const ZeroConfServiceEntry &entry, m_zeroConfBrowser->serviceEntries()) {
        QRegExp match("espuino.*");
        if (!match.exactMatch(entry.name())) {
            continue;
        }

        qCDebug(dcESPuino()) << "Found device:" << entry;
        ThingDescriptor descriptor(info->thingClassId(), entry.hostName(), entry.hostAddress().toString());
        ParamList params;
        params << Param(espuinoThingHostnameParamTypeId, entry.hostName());
        descriptor.setParams(params);

        Things existingThings = myThings().filterByParam(espuinoThingHostnameParamTypeId, entry.hostName());
        if (existingThings.count() == 1) {
            qCDebug(dcESPuino()) << "This device already exists in the system!";
            descriptor.setThingId(existingThings.first()->id());
        }

        info->addThingDescriptor(descriptor);
    }

    info->finish(Thing::ThingErrorNoError);
}

void IntegrationPluginEspuino::setupThing(ThingSetupInfo *info)
{
    Thing *thing = info->thing();

    MqttChannel *channel;
    if (myThings().findByParams(ParamList() << Param(espuinoThingHostnameParamTypeId, thing->paramValue(espuinoThingHostnameParamTypeId).toString())) == nullptr){
        qCDebug(dcESPuino) << "Creating MQTT channel for new device.";

        channel = hardwareManager()->mqttProvider()->createChannel(QHostAddress(getHost(thing)), {"Cmnd/ESPuino", "State/ESPuino"});
        if (!channel) {
            qCWarning(dcESPuino) << "Failed to create MQTT channel.";
            info->finish(Thing::ThingErrorHardwareFailure, QT_TR_NOOP("Error creating MQTT channel. Please check MQTT server settings."));
            return;
        }

        qCInfo(dcESPuino) << "Reconfiguring MQTT settings via Websocket.";
        QWebSocket *ws = new QWebSocket(QString(), QWebSocketProtocol::VersionLatest, info);
        connect(ws, &QWebSocket::connected, info, [channel, ws](){
            QJsonDocument jsonRequest{QJsonObject
            {
                {"mqtt", QJsonObject{{"mqttEnable", "1"},
                                     {"mqttClientId", channel->clientId()},
                                     {"mqttServer", channel->serverAddress().toString()},
                                     {"mqttUser", channel->username()},
                                     {"mqttPwd", channel->password()},
                                     {"mqttPort", QString::number(channel->serverPort())}}}
            }};
            ws->sendTextMessage(jsonRequest.toJson(QJsonDocument::Compact));
        });
        connect(ws, &QWebSocket::textMessageReceived, info, [this, info, thing, channel, ws](const QString &message){
            ws->close();

            QJsonParseError parseError;
            QJsonDocument jsonDoc = QJsonDocument::fromJson(message.toUtf8(), &parseError);
            if (parseError.error != QJsonParseError::NoError) {
                qCWarning(dcESPuino()) << "Json parse error:" << parseError.error << parseError.errorString() << "Received:" << message;
                info->finish(Thing::ThingErrorSetupFailed, QT_TR_NOOP("Failed to configure MQTT via Websocket."));
                hardwareManager()->mqttProvider()->releaseChannel(channel);
                return;
            }
            if (jsonDoc.object().value("status").toString() != "ok") {
                qCWarning(dcESPuino()) << "Failed to configure MQTT via websocket. Received:" << message;
                info->finish(Thing::ThingErrorSetupFailed, QT_TR_NOOP("Failed to configure MQTT via Websocket."));
                hardwareManager()->mqttProvider()->releaseChannel(channel);
                return;
            }

            pluginStorage()->beginGroup(thing->id().toString());
            pluginStorage()->setValue("clientId", channel->clientId());
            pluginStorage()->setValue("username", channel->username());
            pluginStorage()->setValue("password", channel->password());
            pluginStorage()->endGroup();

            qCInfo(dcESPuino) << "Restarting box to apply new MQTT config.";
            QUrl url(QString("http://%1/restart").arg(getHost(thing)));
            QNetworkRequest request(url);
            QNetworkReply *reply = hardwareManager()->networkManager()->get(request);
            connect(reply, &QNetworkReply::finished, reply, &QNetworkReply::deleteLater);

            info->finish(Thing::ThingErrorNoError);
        });
        QUrl url(QString("ws://%1/ws").arg(getHost(thing)));
        ws->open(url);
    } else {
        qCDebug(dcESPuino) << "Creating MQTT channel for existing device.";
        pluginStorage()->beginGroup(thing->id().toString());
        QString clientId = pluginStorage()->value("clientId").toString();
        QString username = pluginStorage()->value("username").toString();
        QString password = pluginStorage()->value("password").toString();
        pluginStorage()->endGroup();
        channel = hardwareManager()->mqttProvider()->createChannel(clientId, username, password, QHostAddress(getHost(thing)), {"Cmnd/ESPuino", "State/ESPuino"});
        if (!channel) {
            qCWarning(dcESPuino) << "Failed to create MQTT channel.";
            info->finish(Thing::ThingErrorHardwareFailure, QT_TR_NOOP("Error creating MQTT channel. Please check MQTT server settings."));
            return;
        }
        info->finish(Thing::ThingErrorNoError);
    }

    m_mqttChannels.insert(thing, channel);
    connect(channel, &MqttChannel::clientConnected, this, &IntegrationPluginEspuino::onClientConnected);
    connect(channel, &MqttChannel::clientDisconnected, this, &IntegrationPluginEspuino::onClientDisconnected);
    connect(channel, &MqttChannel::publishReceived, this, &IntegrationPluginEspuino::onPublishReceived);
}

void IntegrationPluginEspuino::thingRemoved(Thing *thing)
{
    qCDebug(dcESPuino) << "Device removed" << thing->name();
    if (m_mqttChannels.contains(thing)) {
        qCDebug(dcESPuino) << "Releasing MQTT channel";
        MqttChannel* channel = m_mqttChannels.take(thing);
        hardwareManager()->mqttProvider()->releaseChannel(channel);
    }
}

void IntegrationPluginEspuino::onClientConnected(MqttChannel *channel)
{
    Thing *thing = m_mqttChannels.key(channel);
    qCDebug(dcESPuino) << "Thing connected" << thing->name();
    if (!thing) {
        qCWarning(dcESPuino) << "Received a MQTT connected message from a client but don't have a matching thing";
        return;
    }
    thing->setStateValue(espuinoConnectedStateTypeId, true);
}

void IntegrationPluginEspuino::onClientDisconnected(MqttChannel *channel)
{
    Thing *thing = m_mqttChannels.key(channel);
    qCDebug(dcESPuino) << "Thing disconnected" << thing->name();
    if (!thing) {
        qCWarning(dcESPuino) << "Received a MQTT disconnected message from a client but don't have a matching thing";
        return;
    }
    thing->setStateValue(espuinoConnectedStateTypeId, false);
}

void IntegrationPluginEspuino::onPublishReceived(MqttChannel *channel, const QString &topic, const QByteArray &payload)
{
    qCDebug(dcESPuino) << "Publish received" << topic << payload;

    Thing *thing = m_mqttChannels.key(channel);
    if (!thing) {
        qCWarning(dcESPuino) << "Received a publish message from a client but don't have a matching thing";
        return;
    }

    if (topic == "State/ESPuino/State") {
        thing->setStateValue(espuinoConnectedStateTypeId, payload == "Online");
    } else if (topic == "State/ESPuino/Playmode") {
        thing->setStateValue(espuinoPlaybackStatusStateTypeId, payload == "0" ? "Stopped" : "Playing");

        QString playmode = "None";
        if (payload == "0") {
            playmode = "None";
        } else if (payload == "1") {
            playmode = "Single track";
        } else if (payload == "2") {
            playmode = "Single track (loop)";
        } else if (payload == "12") {
            playmode = "Single track of a directory (random). Followed by sleep.";
        } else if (payload == "3") {
            playmode = "Audiobook";
        } else if (payload == "4") {
            playmode = "Audiobook (loop)";
        } else if (payload == "5") {
            playmode = "All tracks of a directory (sorted alph.)";
        } else if (payload == "6") {
            playmode = "All tracks of a directory (random)";
        } else if (payload == "7") {
            playmode = "All tracks of a directory (sorted alph., loop)";
        } else if (payload == "9") {
            playmode = "All tracks of a directory (random, loop)";
        } else if (payload == "8") {
            playmode = "Webradio";
        } else if (payload == "11") {
            playmode = "List (files from SD and/or webstreams) from local .m3u-File";
        } else if (payload == "10") {
            playmode = "Busy";
        } else {
            qCWarning(dcESPuino) << "Unknown playmode received" << payload;
        }
        thing->setStateValue(espuinoPlaymodeStateTypeId, playmode);
    } else if (topic == "State/ESPuino/Loudness") {
        bool ok;
        int volume = payload.toInt(&ok);
        if (ok) {
            thing->setStateValue(espuinoVolumeStateTypeId, volume);
        } else {
            qCWarning(dcESPuino) << "Failed to read numeric volume value" << payload;
            thing->setStateValue(espuinoVolumeStateTypeId, 0);
        }
    } else if (topic == "State/ESPuino/Track") {
        thing->setStateValue(espuinoTitleStateTypeId, payload);
    } else if (topic == "State/ESPuino/CoverChanged") {
        thing->setStateValue(espuinoArtworkStateTypeId, QString("http://%1/cover?%2").arg(getHost(thing)).arg(QDateTime::currentMSecsSinceEpoch()));
    } else if (topic == "State/ESPuino/LedBrightness") {
        bool ok;
        int brightness = payload.toInt(&ok);
        if (ok) {
            thing->setStateValue(espuinoBrightnessStateTypeId, brightness);
        } else {
            qCWarning(dcESPuino) << "Failed to read numeric brightness value" << payload;
            thing->setStateValue(espuinoBrightnessStateTypeId, 0);
        }
    } else if (topic == "State/ESPuino/RepeatMode") {
        if (payload == "3") {
            thing->setStateValue(espuinoRepeatStateTypeId, "All");
        } if (payload == "1") {
            thing->setStateValue(espuinoRepeatStateTypeId, "One");
        } else {
            thing->setStateValue(espuinoRepeatStateTypeId, "None");
        }
    } else if (topic == "State/ESPuino/WifiRssi") {
        bool ok;
        int rssi = payload.toInt(&ok);
        if (ok) {
            thing->setStateValue(espuinoSignalStrengthStateTypeId, qMin(100, qMax(0, (rssi + 100) * 2)));
        } else {
            thing->setStateValue(espuinoSignalStrengthStateTypeId, 0);
        }
    } else if (topic == "State/ESPuino/LockControl") {
        thing->setStateValue(espuinoChildLockStateTypeId, payload == "ON");
    } else if (topic == "State/ESPuino/SleepTimer") {
        if (payload == "EOP") {
            thing->setStateValue(espuinoSleepmodeStateTypeId, "End of playlist");
        } else if (payload == "EOT") {
            thing->setStateValue(espuinoSleepmodeStateTypeId, "End of track");
        } else if (payload == "EO5T") {
            thing->setStateValue(espuinoSleepmodeStateTypeId, "End of five tracks");
        } else if (payload == "0") {
            thing->setStateValue(espuinoSleepmodeStateTypeId, "None");
        } else {
            bool ok;
            int timer = payload.toInt(&ok);
            if (ok) {
                thing->setStateValue(espuinoSleepmodeStateTypeId, "Timer");
                thing->setStateValue(espuinoSleeptimerStateTypeId, timer);
            } else {
                qCWarning(dcESPuino) << "Failed to read numeric sleep timer value" << payload;
                thing->setStateValue(espuinoSleepmodeStateTypeId, "None");
            }
        }
    } else if (topic == "State/ESPuino/Battery") {
        bool ok;
        float battery = payload.toFloat(&ok);
        if (ok) {
            thing->setStateValue(espuinoBatteryLevelStateTypeId, battery);
            thing->setStateValue(espuinoBatteryCriticalStateTypeId, battery < 5.0f);
        } else {
            qCWarning(dcESPuino) << "Failed to read numeric battery level value" << payload;
            thing->setStateValue(espuinoBatteryLevelStateTypeId, 0);
            thing->setStateValue(espuinoBatteryCriticalStateTypeId, false);
        }
    }

    // Finish pending action.
    QPointer<ThingActionInfo> thingActionInfo = m_pendingActions.take(topic);
    if (!thingActionInfo.isNull()) {
        thingActionInfo->finish(Thing::ThingErrorNoError);
    }
}

void IntegrationPluginEspuino::executeAction(ThingActionInfo *info)
{
    Thing *thing = info->thing();
    Action action = info->action();

    MqttChannel *channel = m_mqttChannels.value(thing);
    if (!channel) {
        qCWarning(dcESPuino) << "No valid MQTT channel for thing" << thing->name();
        return info->finish(Thing::ThingErrorThingNotFound);
    }

    // See: https://github.com/biologist79/ESPuino#mqtt-topics-and-their-ranges
    QString topic;
    QByteArray payload;
    if (action.actionTypeId() == espuinoVolumeActionTypeId) {
        topic = "Cmnd/ESPuino/Loudness";
        payload = QByteArray::number(action.param(espuinoVolumeActionVolumeParamTypeId).value().toInt());
        m_pendingActions.insert("State/ESPuino/Loudness", info);
    } else if (action.actionTypeId() == espuinoIncreaseVolumeActionTypeId) {
        topic = "Cmnd/ESPuino/Loudness";
        payload = QByteArray::number(thing->stateValue(espuinoVolumeStateTypeId).toInt() + 1);
        m_pendingActions.insert("State/ESPuino/Loudness", info);
    } else if (action.actionTypeId() == espuinoDecreaseVolumeActionTypeId) {
        topic = "Cmnd/ESPuino/Loudness";
        payload = QByteArray::number(thing->stateValue(espuinoVolumeStateTypeId).toInt() - 1);
        m_pendingActions.insert("State/ESPuino/Loudness", info);
    } else if (action.actionTypeId() == espuinoStopActionTypeId) {
        topic = "Cmnd/ESPuino/TrackControl";
        payload = "1";
        m_pendingActions.insert("State/ESPuino/TrackControl", info);
    } else if (action.actionTypeId() == espuinoSkipNextActionTypeId) {
        topic = "Cmnd/ESPuino/TrackControl";
        payload = "4";
        m_pendingActions.insert("State/ESPuino/TrackControl", info);
    } else if (action.actionTypeId() == espuinoSkipBackActionTypeId) {
        topic = "Cmnd/ESPuino/TrackControl";
        payload = "5";
        m_pendingActions.insert("State/ESPuino/TrackControl", info);
    } else if (action.actionTypeId() == espuinoPlayActionTypeId) {
        topic = "Cmnd/ESPuino/TrackControl";
        payload = "3";
        m_pendingActions.insert("State/ESPuino/TrackControl", info);
    } else if (action.actionTypeId() == espuinoPauseActionTypeId) {
        topic = "Cmnd/ESPuino/TrackControl";
        payload = "3";
        m_pendingActions.insert("State/ESPuino/TrackControl", info);
    } else if (action.actionTypeId() == espuinoBrightnessActionTypeId) {
        topic = "Cmnd/ESPuino/LedBrightness";
        payload = QByteArray::number(action.param(espuinoBrightnessActionBrightnessParamTypeId).value().toInt());
        m_pendingActions.insert("State/ESPuino/LedBrightness", info);
    } else if (action.actionTypeId() == espuinoRepeatActionTypeId) {
        topic = "Cmnd/ESPuino/RepeatMode";
        QString repeat = action.param(espuinoRepeatActionRepeatParamTypeId).value().toString();
        if (repeat == "One") {
            payload = "1";
        } else if (repeat == "All") {
            payload = "3";
        } else {
            payload = "0";
        }
        m_pendingActions.insert("State/ESPuino/RepeatMode", info);
    } else if (action.actionTypeId() == espuinoChildLockActionTypeId) {
        topic = "Cmnd/ESPuino/LockControls";
        payload = action.param(espuinoChildLockActionChildLockParamTypeId).value().toBool() ? "ON" : "OFF";
        m_pendingActions.insert("State/ESPuino/LockControls", info);
    } else if (action.actionTypeId() == espuinoSleepmodeActionTypeId) {
        topic = "Cmnd/ESPuino/SleepTimer";
        QString sleepmode = action.param(espuinoSleepmodeActionSleepmodeParamTypeId).value().toString();
        if (sleepmode == "None") {
            payload = "0";
        } else if (sleepmode == "End of playlist") {
            payload = "EOP";
        } else if (sleepmode == "End of track") {
            payload = "EOT";
        } else if (sleepmode == "End of five tracks") {
            payload = "EO5T";
        } else {
            payload = QByteArray::number(thing->stateValue(espuinoSleeptimerStateTypeId).toInt());
        }
        m_pendingActions.insert("State/ESPuino/SleepTimer", info);
    } else if (action.actionTypeId() == espuinoSleeptimerActionTypeId) {
        thing->setStateValue(espuinoSleeptimerStateTypeId, action.param(espuinoSleeptimerActionSleeptimerParamTypeId).value().toUInt());
        info->finish(Thing::ThingErrorNoError);
    }

    if (!topic.isEmpty()) {
        qCDebug(dcESPuino) << "Publishing:" << topic << payload;
        channel->publish(topic, payload);
    }
    return;
}

void IntegrationPluginEspuino::browseThing(BrowseResult *result)
{
    QUrlQuery id(result->itemId());
    browseThing(result, id.queryItemValue("path"));
}

void IntegrationPluginEspuino::browseThing(BrowseResult *result, const QString &path)
{
    QUrl url(QString("http://%1/explorer?path=%2").arg(getHost(result->thing())).arg(path.isEmpty() ? "/" : path));
    QNetworkRequest request(url);
    QNetworkReply *reply = hardwareManager()->networkManager()->get(request);
    connect(reply, &QNetworkReply::finished, reply, &QNetworkReply::deleteLater);
    connect(reply, &QNetworkReply::finished, result, [result, reply, path, this]() {
        if (reply->error() != QNetworkReply::NoError) {
            qCWarning(dcESPuino()) << "Error fetching paths";
            result->finish(Thing::ThingErrorHardwareNotAvailable);
            return;
        }
        QByteArray data = reply->readAll();
        QJsonParseError error;
        QJsonDocument jsonDoc = QJsonDocument::fromJson(data, &error);
        if (error.error != QJsonParseError::NoError) {
            qCWarning(dcESPuino()) << "Error parsing json" << data;
            result->finish(Thing::ThingErrorHardwareFailure);
            return;
        }

        //qCDebug(dcESPuino()) << "Reply:" << qUtf8Printable(jsonDoc.toJson());
        QVariantList variantList = jsonDoc.toVariant().toList();
        foreach (const QVariant &element, variantList) {
            QVariantMap variantMap = element.toMap();
            QUrlQuery id;
            id.addQueryItem("name", variantMap.value("name").toString());
            id.addQueryItem("path", path + "/" + variantMap.value("name").toString());
            if (variantMap.value("dir").toBool()) {
                id.addQueryItem("playmode", QString::number(5));
                id.addQueryItem("type", "dir");
            } else if (variantMap.value("name").toString().contains(QRegExp("\\.(:?mp3|ogg|wav|wma|acc|m4a|flac)$", Qt::CaseInsensitive))) {
                id.addQueryItem("playmode", QString::number(1));
                id.addQueryItem("type", "audiofile");
            } else if (variantMap.value("name").toString().endsWith(".m3u", Qt::CaseInsensitive)) {
                id.addQueryItem("playmode", QString::number(11));
                id.addQueryItem("type", "playlist");
            }
            result->addItem(browserItemFromQuery(id));
        }
        result->finish(Thing::ThingErrorNoError);
    });
}

void IntegrationPluginEspuino::browserItem(BrowserItemResult *result)
{
    QUrlQuery id(result->itemId());
    result->finish(browserItemFromQuery(id));
}

BrowserItem IntegrationPluginEspuino::browserItemFromQuery(const QUrlQuery &id)
{
    BrowserItem item;
    item.setDisplayName(id.queryItemValue("name"));
    if (id.queryItemValue("type") == "dir") {
        item.setId(id.toString());
        item.setIcon(BrowserItem::BrowserIconFolder);
        item.setBrowsable(true);
        item.setActionTypeIds({espuinoPlayAllBrowserItemActionTypeId});
    } else if (id.queryItemValue("type") == "audiofile") {
        item.setId(id.toString());
        item.setIcon(BrowserItem::BrowserIconMusic);
        item.setExecutable(true);
    } else if (id.queryItemValue("type") == "playlist") {
        item.setId(id.toString());
        item.setIcon(BrowserItem::BrowserIconDocument);
        item.setExecutable(true);
    } else {
        item.setId(id.toString());
        item.setIcon(BrowserItem::BrowserIconFile);
    }
    return item;
}

void IntegrationPluginEspuino::IntegrationPluginEspuino::executeBrowserItem(BrowserActionInfo *info)
{
    Thing *thing = info->thing();
    BrowserAction action = info->browserAction();

    QUrl url(QString("http://%1/exploreraudio?%2").arg(getHost(thing)).arg(action.itemId()));
    qCInfo(dcESPuino) << "Starting playback" << url.toString();
    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    QNetworkReply *reply = hardwareManager()->networkManager()->post(request, QByteArray());
    connect(reply, &QNetworkReply::finished, reply, &QNetworkReply::deleteLater);
    connect(reply, &QNetworkReply::finished, this, [reply]() {
       if (reply->error() != QNetworkReply::NoError) {
            qCWarning(dcESPuino()) << "Fail to execute play action";
       }
    });
}

void IntegrationPluginEspuino::executeBrowserItemAction(BrowserItemActionInfo *info)
{
    Thing *thing = info->thing();
    BrowserItemAction action = info->browserItemAction();

    if (action.actionTypeId() == espuinoPlayAllBrowserItemActionTypeId) {
        QUrl url(QString("http://%1/exploreraudio?%2").arg(getHost(thing)).arg(action.itemId()));
        qCInfo(dcESPuino) << "Starting playback" << url.toString();
        QNetworkRequest request(url);
        request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
        QNetworkReply *reply = hardwareManager()->networkManager()->post(request, QByteArray());
        connect(reply, &QNetworkReply::finished, reply, &QNetworkReply::deleteLater);
        connect(reply, &QNetworkReply::finished, this, [reply]() {
           if (reply->error() != QNetworkReply::NoError) {
                qCWarning(dcESPuino()) << "Fail to execute play action";
           }
        });
    }
}

QString IntegrationPluginEspuino::getHost(Thing *thing) const
{
    QString hostName = thing->paramValue(espuinoThingHostnameParamTypeId).toString();
    ZeroConfServiceEntry zeroConfEntry;
    foreach (const ZeroConfServiceEntry &entry, m_zeroConfBrowser->serviceEntries()) {
        if (hostName == entry.hostName()) {
            zeroConfEntry = entry;
        }
    }
    QString host;
    pluginStorage()->beginGroup(thing->id().toString());
    if (zeroConfEntry.isValid()) {
        host = zeroConfEntry.hostAddress().toString();
        pluginStorage()->setValue("cachedAddress", host);
    } else if (pluginStorage()->contains("cachedAddress")){
        host = pluginStorage()->value("cachedAddress").toString();
    } else {
        qCWarning(dcESPuino()) << "Unable to determine IP address for:" << hostName;
    }
    pluginStorage()->endGroup();

    return host;
}
