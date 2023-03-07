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

#include "integrationpluginkodi.h"
#include "integrations/thing.h"
#include "plugininfo.h"
#include "network/upnp/upnpdiscovery.h"
#include "platform/platformzeroconfcontroller.h"
#include "network/zeroconf/zeroconfservicebrowser.h"
#include "network/zeroconf/zeroconfserviceentry.h"
#include "network/networkaccessmanager.h"

#include <QNetworkRequest>
#include <QNetworkReply>

IntegrationPluginKodi::IntegrationPluginKodi()
{

}

IntegrationPluginKodi::~IntegrationPluginKodi()
{
    hardwareManager()->pluginTimerManager()->unregisterTimer(m_pluginTimer);
    delete m_serviceBrowser;
    delete m_httpServiceBrowser;
}

void IntegrationPluginKodi::init()
{
    m_serviceBrowser = hardwareManager()->zeroConfController()->createServiceBrowser("_xbmc-jsonrpc._tcp");
    m_httpServiceBrowser = hardwareManager()->zeroConfController()->createServiceBrowser("_http._tcp");

    connect(m_serviceBrowser, &ZeroConfServiceBrowser::serviceEntryAdded, this, [=](const ZeroConfServiceEntry &entry){
        QUuid uuid = QUuid(entry.txt("uuid"));
        foreach (Thing *thing, myThings()) {
            if (thing->paramValue(kodiThingUuidParamTypeId).toUuid() == uuid) {
                qCDebug(dcKodi()) << "Kodi" << thing->name() << "appeared on ZeroConf";
                Kodi *kodi = m_kodis.value(thing);
                kodi->setHostAddress(entry.hostAddress());
                kodi->setPort(entry.port());
                if (!kodi->connected()) {
                    kodi->connectKodi();
                }
            }
        }
    });
}

void IntegrationPluginKodi::setupThing(ThingSetupInfo *info)
{
    Thing *thing = info->thing();
    qCDebug(dcKodi) << "Setthing up Kodi" << thing->paramValue(kodiThingUuidParamTypeId).toString();

    KodiHostInfo hostInfo = resolve(thing);

    if (info->isInitialSetup()) {
        if (hostInfo.address.isNull()) {
            info->finish(Thing::ThingErrorHardwareNotAvailable, QT_TR_NOOP("Unable to find Kodi in the network."));
            return;
        }
    }

    if (m_kodis.contains(thing)) {
        delete m_kodis.take(thing);
    }

    qCDebug(dcKodi()).nospace().noquote() << "Connecting to kodi on " << hostInfo.address.toString() << ":" << hostInfo.rpcPort << " (HTTP Port " << hostInfo.httpPort << ")";
    Kodi *kodi= new Kodi(hostInfo.address, hostInfo.rpcPort, hostInfo.httpPort, thing);
    m_kodis.insert(thing, kodi);

    if (info->isInitialSetup()) {
        connect(kodi, &Kodi::connectionStatusChanged, info, [info](bool connected){
            if (connected) {
                info->finish(Thing::ThingErrorNoError);
            }
        });
    } else {
        info->finish(Thing::ThingErrorNoError);
    }

    connect(kodi, &Kodi::connectionStatusChanged, this, &IntegrationPluginKodi::onConnectionChanged);
    connect(kodi, &Kodi::stateChanged, this, &IntegrationPluginKodi::onStateChanged);
    connect(kodi, &Kodi::actionExecuted, this, &IntegrationPluginKodi::onActionExecuted);
    connect(kodi, &Kodi::playbackStatusChanged, this, &IntegrationPluginKodi::onPlaybackStatusChanged);
    connect(kodi, &Kodi::browserItemExecuted, this, &IntegrationPluginKodi::onBrowserItemExecuted);
    connect(kodi, &Kodi::browserItemActionExecuted, this, &IntegrationPluginKodi::onBrowserItemActionExecuted);

    connect(kodi, &Kodi::activePlayerChanged, thing, [thing](const QString &playerType){
        thing->setStateValue(kodiPlayerTypeStateTypeId, playerType);
    });
    connect(kodi, &Kodi::mediaMetadataChanged, thing, [this, thing](const QString &title, const QString &artist, const QString &collection, const QString &artwork){
        thing->setStateValue(kodiTitleStateTypeId, title);
        thing->setStateValue(kodiArtistStateTypeId, artist);
        thing->setStateValue(kodiCollectionStateTypeId, collection);

        Kodi* kodi = m_kodis.value(thing);

        QNetworkRequest request;
        QHostAddress hostAddr(kodi->hostAddress().toString());
        QString addr;
        if (hostAddr.protocol() == QAbstractSocket::IPv4Protocol) {
            addr = hostAddr.toString();
        } else {
            addr = "[" + hostAddr.toString() + "]";
        }

        uint httpPort = kodi->httpPort();
        request.setUrl(QUrl(QString("http://%1:%2/jsonrpc").arg(addr).arg(httpPort)));
        qCDebug(dcKodi) << "Prepping file dl" << request.url().toString();
        request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
        QVariantMap map;
        map.insert("jsonrpc", "2.0");
        map.insert("method", "Files.PrepareDownload");
        map.insert("id", QString::number(123));
        QVariantMap params;
        params.insert("path", artwork);
        map.insert("params", params);
        QJsonDocument jsonDoc = QJsonDocument::fromVariant(map);
        QNetworkReply *reply = hardwareManager()->networkManager()->post(request, jsonDoc.toJson(QJsonDocument::Compact));
        connect(reply, &QNetworkReply::finished, thing, [thing, reply, addr, httpPort](){
            reply->deleteLater();
            QJsonDocument jsonDoc = QJsonDocument::fromJson(reply->readAll());
            QString fileUrl = QString("http://%1:%2/%3").arg(addr).arg(httpPort).arg(jsonDoc.toVariant().toMap().value("result").toMap().value("details").toMap().value("path").toString());
            qCDebug(dcKodi()) << "DL result:" << jsonDoc.toJson();
            qCDebug(dcKodi()) << "Resolved url:" << fileUrl;
            thing->setStateValue(kodiArtworkStateTypeId, fileUrl);
        });

    });

    connect(kodi, &Kodi::shuffleChanged, thing, [thing](bool shuffle){
        thing->setStateValue(kodiShuffleStateTypeId, shuffle);
    });
    connect(kodi, &Kodi::repeatChanged, thing, [thing](const QString &repeat){
        if (repeat == "one") {
            thing->setStateValue(kodiRepeatStateTypeId, "One");
        } else if (repeat == "all") {
            thing->setStateValue(kodiRepeatStateTypeId, "All");
        } else {
            thing->setStateValue(kodiRepeatStateTypeId, "None");
        }
    });

    if (!kodi->hostAddress().isNull()) {
        kodi->connectKodi();
    }
}

void IntegrationPluginKodi::postSetupThing(Thing */*thing*/)
{
    if (!m_pluginTimer) {
        m_pluginTimer = hardwareManager()->pluginTimerManager()->registerTimer(10);
        connect(m_pluginTimer, &PluginTimer::timeout, this, [this](){
            foreach (Thing *thing, myThings()){
                Kodi *kodi = m_kodis.value(thing);
                if (!kodi->connected()) {
                    KodiHostInfo hostInfo = resolve(thing);
                    kodi->setHostAddress(hostInfo.address);
                    kodi->setPort(hostInfo.rpcPort);
                    kodi->setHttpPort(hostInfo.httpPort);
                    kodi->connectKodi();
                    continue;
                }
            }
        });
    }
}

void IntegrationPluginKodi::thingRemoved(Thing *thing)
{
    m_kodis.remove(thing);

    if (myThings().isEmpty()) {
        hardwareManager()->pluginTimerManager()->unregisterTimer(m_pluginTimer);
        m_pluginTimer = nullptr;
    }
}

void IntegrationPluginKodi::discoverThings(ThingDiscoveryInfo *info)
{
    QTimer::singleShot(5000, info, [this, info](){

        QHash<QUuid, ThingDescriptor> descriptors;

        foreach (const ZeroConfServiceEntry avahiEntry, m_serviceBrowser->serviceEntries()) {

            QUuid uuid = avahiEntry.txt("uuid");
            if (descriptors.contains(uuid)) {
                // Might appear multiple times, IPv4 and IPv6
                continue;
            }

            qCDebug(dcKodi) << "Zeroconf entry:" << avahiEntry;
            ThingDescriptor descriptor(kodiThingClassId, avahiEntry.name(), avahiEntry.hostName() + " (" + avahiEntry.hostAddress().toString() + ")");
            ParamList params;
            params << Param(kodiThingUuidParamTypeId, uuid);
            descriptor.setParams(params);

            Things existing = myThings().filterByParam(kodiThingUuidParamTypeId, uuid);
            if (existing.count() > 0) {
                descriptor.setThingId(existing.first()->id());
            }

            descriptors.insert(uuid, descriptor);
        }

        foreach (const ThingDescriptor &d, descriptors.values()) {
            qCDebug(dcKodi()) << "Returning descritpor:" << d.params();
        }
        info->addThingDescriptors(descriptors.values());
        info->finish(Thing::ThingErrorNoError);
    });
}

void IntegrationPluginKodi::executeAction(ThingActionInfo *info)
{
    Action action = info->action();
    Thing *thing = info->thing();
    Kodi *kodi = m_kodis.value(thing);

    // check connection state
    if (!kodi->connected()) {
        return info->finish(Thing::ThingErrorHardwareNotAvailable);
    }

    int commandId = -1;
    if (action.actionTypeId() == kodiNotifyActionTypeId) {
        QString notificationType = action.param(kodiNotifyActionTypeParamTypeId).value().toString();
        QUrl notificationUrl = thing->setting(kodiSettingsNotificationCustomIconUrlParamTypeId).toUrl();
        QString imageString;
        if (notificationType == "icon") {
            if (!notificationUrl.isEmpty() && notificationUrl.isValid()) {
                imageString = notificationUrl.toString();
            } else {
                // No valid icon url configured. Let's fallback to info
                imageString = "info";
            }
        } else {
            // info, warning, error
            imageString = notificationType;
        }

        commandId = kodi->showNotification(
                    action.param(kodiNotifyActionTitleParamTypeId).value().toString(),
                    action.param(kodiNotifyActionBodyParamTypeId).value().toString(),
                    thing->setting(kodiSettingsNotificationDurationParamTypeId).toUInt(),
                    imageString);
    } else if (action.actionTypeId() == kodiVolumeActionTypeId) {
        commandId = kodi->setVolume(action.param(kodiVolumeActionVolumeParamTypeId).value().toInt());
    } else if (action.actionTypeId() == kodiMuteActionTypeId) {
        commandId = kodi->setMuted(action.param(kodiMuteActionMuteParamTypeId).value().toBool());
    } else if (action.actionTypeId() == kodiNavigateActionTypeId) {
        commandId = kodi->navigate(action.param(kodiNavigateActionToParamTypeId).value().toString());
    } else if (action.actionTypeId() == kodiSystemActionTypeId) {
        commandId = kodi->systemCommand(action.param(kodiSystemActionSystemCommandParamTypeId).value().toString());
    } else if(action.actionTypeId() == kodiSkipBackActionTypeId) {
        commandId = kodi->navigate("skipprevious");
    } else if(action.actionTypeId() == kodiFastRewindActionTypeId) {
        commandId = kodi->navigate("rewind");
    } else if(action.actionTypeId() == kodiStopActionTypeId) {
        commandId = kodi->navigate("stop");
    } else if(action.actionTypeId() == kodiPlayActionTypeId) {
        commandId = kodi->navigate("play");
    } else if(action.actionTypeId() == kodiPauseActionTypeId) {
        commandId = kodi->navigate("pause");
    } else if(action.actionTypeId() == kodiFastForwardActionTypeId) {
        commandId = kodi->navigate("fastforward");
    } else if(action.actionTypeId() == kodiSkipNextActionTypeId) {
        commandId = kodi->navigate("skipnext");
    } else if (action.actionTypeId() == kodiShuffleActionTypeId) {
        commandId = kodi->setShuffle(action.param(kodiShuffleActionShuffleParamTypeId).value().toBool());
    } else if (action.actionTypeId() == kodiIncreaseVolumeActionTypeId) {
        commandId = kodi->setVolume(qMin(100, thing->stateValue(kodiVolumeStateTypeId).toInt() + 5));
    } else if (action.actionTypeId() == kodiDecreaseVolumeActionTypeId) {
        commandId = kodi->setVolume(qMax(0, thing->stateValue(kodiVolumeStateTypeId).toInt() - 5));
    } else if (action.actionTypeId() == kodiRepeatActionTypeId) {
        QString repeat = action.param(kodiRepeatActionRepeatParamTypeId).value().toString();
        if (repeat == "One") {
            commandId = kodi->setRepeat("one");
        } else if (repeat == "All") {
            commandId = kodi->setRepeat("all");
        } else {
            commandId = kodi->setRepeat("off");
        }
    } else {
        qWarning(dcKodi()) << "Unhandled action type" << action.actionTypeId();
        return info->finish(Thing::ThingErrorActionTypeNotFound);
    }

    m_pendingActions.insert(commandId, info);
    connect(info, &QObject::destroyed, this, [this, commandId](){ m_pendingActions.remove(commandId); });
}

void IntegrationPluginKodi::browseThing(BrowseResult *result)
{
    Kodi *kodi = m_kodis.value(result->thing());
    if (!kodi) {
        result->finish(Thing::ThingErrorHardwareNotAvailable);
        return;
    }

    kodi->browse(result);
}

void IntegrationPluginKodi::browserItem(BrowserItemResult *result)
{
    Kodi *kodi = m_kodis.value(result->thing());
    if (!kodi) {
        result->finish(Thing::ThingErrorHardwareNotAvailable);
        return;
    }

    kodi->browserItem(result);
}

void IntegrationPluginKodi::executeBrowserItem(BrowserActionInfo *info)
{
    Kodi *kodi = m_kodis.value(info->thing());
    if (!kodi) {
        info->finish(Thing::ThingErrorHardwareNotAvailable);
        return;
    }

    int id = kodi->launchBrowserItem(info->browserAction().itemId());
    if (id == -1) {
        return info->finish(Thing::ThingErrorHardwareFailure);
    }
    m_pendingBrowserActions.insert(id, info);
    connect(info, &QObject::destroyed, this, [this, id](){ m_pendingBrowserActions.remove(id); });
}

void IntegrationPluginKodi::executeBrowserItemAction(BrowserItemActionInfo *info)
{
    Kodi *kodi = m_kodis.value(info->thing());
    if (!kodi) {
        return info->finish(Thing::ThingErrorHardwareNotAvailable);
    }

    int id = kodi->executeBrowserItemAction(info->browserItemAction().itemId(), info->browserItemAction().actionTypeId());
    if (id == -1) {
        return info->finish(Thing::ThingErrorHardwareFailure);
    }
    m_pendingBrowserItemActions.insert(id, info);
    connect(info, &QObject::destroyed, this, [this, id](){ m_pendingBrowserItemActions.remove(id); });
}

IntegrationPluginKodi::KodiHostInfo IntegrationPluginKodi::resolve(Thing *thing)
{
    QUuid uuid = thing->paramValue(kodiThingUuidParamTypeId).toUuid();

    KodiHostInfo ret;

    foreach (const ZeroConfServiceEntry &entry, m_serviceBrowser->serviceEntries()) {
        //Disabling IPv6 for now... it seems to be to unreliable
        if (entry.hostAddress().protocol() == QAbstractSocket::IPv6Protocol) {
            continue;
        }

        QUuid entryUuid = entry.txt("uuid");
        if (entryUuid != uuid) {
            continue;
        }

        ret.address = entry.hostAddress();
        ret.rpcPort = entry.port();

        foreach (const ZeroConfServiceEntry &httpEntry, m_httpServiceBrowser->serviceEntries()) {
            if (QUuid(httpEntry.txt("uuid")) == uuid) {
                ret.httpPort = httpEntry.port();
                break;
            }
        }
        break;
    }

    if (ret.address.isNull()) {
        if (pluginStorage()->childGroups().contains(thing->id().toString())) {
            pluginStorage()->beginGroup(thing->id().toString());
            ret.address = pluginStorage()->value("address").toString();
            ret.rpcPort = pluginStorage()->value("rpcPort").toUInt();
            ret.httpPort = pluginStorage()->value("httpPort").toUInt();
            pluginStorage()->endGroup();
            qCDebug(dcKodi()) << "Kodi not found on ZeroConf. Using cached info:" << ret.address.toString() << ret.rpcPort << ret.httpPort;
        } else {
            qCDebug(dcKodi()) << "Kodi not found on ZeroConf and no cached info availble";
        }
    } else {
        qCDebug(dcKodi()) << "Kodie address resolved:" << ret.address.toString();
    }
    return ret;
}

void IntegrationPluginKodi::onConnectionChanged(bool connected)
{
    qCDebug(dcKodi()) << "Connection status changed:" << connected;
    Kodi *kodi = static_cast<Kodi *>(sender());
    Thing *thing = m_kodis.key(kodi);

    thing->setStateValue(kodiConnectedStateTypeId, connected);


    if (connected) {
        pluginStorage()->beginGroup(thing->id().toString());
        pluginStorage()->setValue("address", kodi->hostAddress().toString());
        pluginStorage()->setValue("rpcPort", kodi->port());
        pluginStorage()->setValue("httpPort", kodi->httpPort());
        pluginStorage()->endGroup();

        QString imageString;
        QUrl notificationUrl = thing->setting(kodiSettingsNotificationCustomIconUrlParamTypeId).toUrl();
        if (!notificationUrl.isEmpty() && notificationUrl.isValid()) {
            imageString = notificationUrl.toString();
        } else {
            imageString = "info";
        }
        kodi->showNotification("nymea", "Connected", 2000, imageString);
    }
}

void IntegrationPluginKodi::onStateChanged()
{
    Kodi *kodi = static_cast<Kodi *>(sender());
    Thing *thing = m_kodis.key(kodi);

    // set thing state values
    thing->setStateValue(kodiVolumeStateTypeId, kodi->volume());
    thing->setStateValue(kodiMuteStateTypeId, kodi->muted());
}

void IntegrationPluginKodi::onActionExecuted(int actionId, bool success)
{
    if (!m_pendingActions.contains(actionId)) {
        return;
    }
    m_pendingActions.take(actionId)->finish(success ? Thing::ThingErrorNoError : Thing::ThingErrorInvalidParameter);
}

void IntegrationPluginKodi::onBrowserItemExecuted(int actionId, bool success)
{
    if (!m_pendingBrowserActions.contains(actionId)) {
        return;
    }
    m_pendingBrowserActions.take(actionId)->finish(success ? Thing::ThingErrorNoError : Thing::ThingErrorInvalidParameter);
}

void IntegrationPluginKodi::onBrowserItemActionExecuted(int actionId, bool success)
{
    if (!m_pendingBrowserItemActions.contains(actionId)) {
        return;
    }
    m_pendingBrowserItemActions.take(actionId)->finish(success ? Thing::ThingErrorNoError : Thing::ThingErrorHardwareFailure);
}

void IntegrationPluginKodi::onPlaybackStatusChanged(const QString &playbackStatus)
{
    Kodi *kodi = static_cast<Kodi *>(sender());
    Thing *thing = m_kodis.key(kodi);
    thing->setStateValue(kodiPlaybackStatusStateTypeId, playbackStatus);
    // legacy events
    if (playbackStatus == "Playing") {
        emit emitEvent(Event(kodiOnPlayerPlayEventTypeId, thing->id()));
    } else if (playbackStatus == "Paused") {
        emit emitEvent(Event(kodiOnPlayerPauseEventTypeId, thing->id()));
    } else {
        emit emitEvent(Event(kodiOnPlayerStopEventTypeId, thing->id()));
    }
}

