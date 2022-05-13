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

    m_pluginTimer = hardwareManager()->pluginTimerManager()->registerTimer(10);
    connect(m_pluginTimer, &PluginTimer::timeout, this, &IntegrationPluginKodi::onPluginTimer);
}

void IntegrationPluginKodi::setupThing(ThingSetupInfo *info)
{
    Thing *thing = info->thing();
    qCDebug(dcKodi) << "Setup Kodi" << thing->paramValue(kodiThingIpParamTypeId).toString();

    QUuid kodiUuid = thing->paramValue(kodiThingUuidParamTypeId).toUuid();

    // The IP string is optional, we'll try to discover it in any case via zeroconf, however, if it's
    // set in the params, we'll always fall back to that in case we can't find it on zeroconf.

    // The recommended way is to not store an IP in the settings as with DHCP lease times (or IPv6 privacy
    // extension address randomization) an IP might expire eventually and it'll stop working.

    // So actually the params should *only* store the UUID, but we'll support manually entering IP, port and http port
    // for setups that can't use ZeroConf for whatever reason.

    QString ipString = thing->paramValue(kodiThingIpParamTypeId).toString();
    int port = thing->paramValue(kodiThingPortParamTypeId).toInt();
    int httpPort = thing->paramValue(kodiThingHttpPortParamTypeId).toInt();

    if (!kodiUuid.isNull()) {
        foreach (const ZeroConfServiceEntry &entry, m_serviceBrowser->serviceEntries()) {
            if (entry.hostAddress().protocol() == QAbstractSocket::IPv6Protocol && entry.hostAddress().toString().startsWith("fe80")) {
                // We don't support link-local ipv6 addresses yet. skip those entries
                continue;
            }
            QString uuid;
            foreach (const QString &txt, entry.txt()) {
                if (txt.startsWith("uuid")) {
                    uuid = txt.split("=").last();
                    break;
                }
            }

            if (QUuid(uuid) == kodiUuid) {
                ipString = entry.hostAddress().toString();
                port = entry.port();
                break;
            }
        }
        foreach (const ZeroConfServiceEntry avahiEntry, m_httpServiceBrowser->serviceEntries()) {
            QString uuid;
            foreach (const QString &txt, avahiEntry.txt()) {
                if (txt.startsWith("uuid")) {
                    uuid = txt.split("=").last();
                    break;
                }
            }
            if (QUuid(uuid) == kodiUuid) {
                httpPort = avahiEntry.port();
                break;
            }
        }
    }

    if (ipString.isEmpty()) {
        // Ok, we could not find an ip on zeroconf... Let's try again in a second while setupInfo hasn't timed out.
        qCDebug(dcKodi()) << "Device not found via ZeroConf... Waiting for a second for it to appear...";
        QTimer::singleShot(1000, info, [this, info](){
            setupThing(info);
        });
        return;
    }

    qCDebug(dcKodi()).nospace().noquote() << "Connecting to kodi on " << ipString << ":" << port << " (HTTP Port " << httpPort << ")";
    Kodi *kodi= new Kodi(QHostAddress(ipString), port, httpPort, this);

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

        Kodi* kodi = m_kodis.key(thing);

        QNetworkRequest request;
        QHostAddress hostAddr(kodi->hostAddress().toString());
        QString addr;
        if (hostAddr.protocol() == QAbstractSocket::IPv4Protocol) {
            addr = hostAddr.toString();
        } else {
            addr = "[" + hostAddr.toString() + "]";
        }
        QString port = thing->paramValue(kodiThingHttpPortParamTypeId).toString();

        request.setUrl(QUrl(QString("http://%1:%2/jsonrpc").arg(addr).arg(port)));
        qCDebug(dcKodi) << "Prepping file dl" << "http://" + addr + ":" + thing->paramValue(kodiThingPortParamTypeId).toString() + "/jsonrpc";
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
        connect(reply, &QNetworkReply::finished, thing, [thing, reply, addr, port](){
            reply->deleteLater();
            QJsonDocument jsonDoc = QJsonDocument::fromJson(reply->readAll());
            QString fileUrl = "http://" + addr + ":" + port + "/" + jsonDoc.toVariant().toMap().value("result").toMap().value("details").toMap().value("path").toString();
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
    m_kodis.insert(kodi, thing);
    m_asyncSetups.insert(kodi, info);
    connect(info, &QObject::destroyed, this, [this, kodi](){ m_asyncSetups.remove(kodi); });

    kodi->connectKodi();
}

void IntegrationPluginKodi::thingRemoved(Thing *thing)
{
    Kodi *kodi = m_kodis.key(thing);
    m_kodis.remove(kodi);
    qCDebug(dcKodi) << "Delete " << thing->name();
    kodi->deleteLater();
}

void IntegrationPluginKodi::discoverThings(ThingDiscoveryInfo *info)
{
    QTimer::singleShot(5000, info, [this, info](){

        QHash<QString, ThingDescriptor> descriptors;

        foreach (const ZeroConfServiceEntry avahiEntry, m_serviceBrowser->serviceEntries()) {

            QString uuid;
            foreach (const QString &txt, avahiEntry.txt()) {
                if (txt.startsWith("uuid")) {
                    uuid = txt.split("=").last();
                    break;
                }
            }

            if (descriptors.contains(uuid)) {
                // Might appear multiple times, IPv4 and IPv6
                continue;
            }

            qCDebug(dcKodi) << "Zeroconf entry:" << avahiEntry;
            ThingDescriptor descriptor(kodiThingClassId, avahiEntry.name(), avahiEntry.hostName() + " (" + avahiEntry.hostAddress().toString() + ")");
            ParamList params;
            params << Param(kodiThingUuidParamTypeId, uuid);
//            params << Param(kodiThingIpParamTypeId, avahiEntry.hostAddress().toString());
            params << Param(kodiThingPortParamTypeId, avahiEntry.port());
            descriptor.setParams(params);

            Things existing = myThings().filterByParam(kodiThingUuidParamTypeId, uuid);
            if (existing.count() > 0) {
                descriptor.setThingId(existing.first()->id());
            }

            descriptors.insert(uuid, descriptor);
        }

        foreach (const ZeroConfServiceEntry avahiEntry, m_httpServiceBrowser->serviceEntries()) {
            qCDebug(dcKodi) << "Zeroconf http entry:" << avahiEntry;
            QString uuid;
            foreach (const QString &txt, avahiEntry.txt()) {
                if (txt.startsWith("uuid")) {
                    uuid = txt.split("=").last();
                    break;
                }
            }
            if (!descriptors.contains(uuid)) {
                continue;
            }
            qCDebug(dcKodi()) << "Updating http parameter:" << avahiEntry.port();
            ThingDescriptor descriptor = descriptors.value(uuid);
            ParamList params = descriptor.params();
            params << Param(kodiThingHttpPortParamTypeId, avahiEntry.port());
            descriptor.setParams(params);
            descriptors[uuid] = descriptor;
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
    Kodi *kodi = m_kodis.key(thing);

    // check connection state
    if (!kodi->connected()) {
        return info->finish(Thing::ThingErrorHardwareNotAvailable);
    }

    int commandId = -1;
    if (action.actionTypeId() == kodiNotifyActionTypeId) {
        QUrl notificationUrl = thing->setting(kodiSettingsNotificationCustomIconUrlParamTypeId).toUrl();
        QString imageString;
        if (thing->setting(kodiSettingsNotificationUseCustomIconParamTypeId).toBool()) {
            imageString = notificationUrl.toString();
        } else {
            imageString = action.param(kodiNotifyActionTypeParamTypeId).value().toString();
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
    Kodi *kodi = m_kodis.key(result->thing());
    if (!kodi) {
        result->finish(Thing::ThingErrorHardwareNotAvailable);
        return;
    }

    kodi->browse(result);
}

void IntegrationPluginKodi::browserItem(BrowserItemResult *result)
{
    Kodi *kodi = m_kodis.key(result->thing());
    if (!kodi) {
        result->finish(Thing::ThingErrorHardwareNotAvailable);
        return;
    }

    kodi->browserItem(result);
}

void IntegrationPluginKodi::executeBrowserItem(BrowserActionInfo *info)
{
    Kodi *kodi = m_kodis.key(info->thing());
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
    Kodi *kodi = m_kodis.key(info->thing());
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

void IntegrationPluginKodi::onPluginTimer()
{
    foreach (Kodi *kodi, m_kodis.keys()) {
        if (!kodi->connected()) {
            kodi->connectKodi();
            continue;
        }
    }
}

void IntegrationPluginKodi::onConnectionChanged(bool connected)
{
    Kodi *kodi = static_cast<Kodi *>(sender());
    Thing *thing = m_kodis.value(kodi);

    // Finish setup
    ThingSetupInfo *info = m_asyncSetups.value(kodi);
    if (info) {
        if (connected) {
            info->finish(Thing::ThingErrorNoError);
        } else {
            //: Error setting up thing
            m_asyncSetups.take(kodi)->finish(Thing::ThingErrorHardwareFailure, QT_TR_NOOP("This installation of Kodi is too old. Please upgrade your Kodi system."));
        }
    }

    QUrl notificationUrl = thing->setting(kodiSettingsNotificationCustomIconUrlParamTypeId).toUrl();
    QString imageString;
    if (thing->setting(kodiSettingsNotificationUseCustomIconParamTypeId).toBool()) {
        imageString = notificationUrl.toString();
    } else {
        imageString = "info";
    }

    kodi->showNotification("nymea", QT_TR_NOOP("Connected"), 2000, imageString);
    thing->setStateValue(kodiConnectedStateTypeId, kodi->connected());
}

void IntegrationPluginKodi::onStateChanged()
{
    Kodi *kodi = static_cast<Kodi *>(sender());
    Thing *thing = m_kodis.value(kodi);

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
    Thing *thing = m_kodis.value(kodi);
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

