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

#include "integrationpluginbose.h"
#include "integrations/thing.h"
#include "plugininfo.h"
#include "platform/platformzeroconfcontroller.h"
#include "network/zeroconf/zeroconfserviceentry.h"
#include "types/mediabrowseritem.h"

#include <QNetworkRequest>
#include <QNetworkReply>

IntegrationPluginBose::IntegrationPluginBose()
{
}

IntegrationPluginBose::~IntegrationPluginBose()
{
}

void IntegrationPluginBose::init()
{
    m_serviceBrowser = hardwareManager()->zeroConfController()->createServiceBrowser("_soundtouch._tcp");

    updateConsumerKey();

    connect(this, &IntegrationPlugin::configValueChanged, this, &IntegrationPluginBose::updateConsumerKey);
    connect(apiKeyStorage(), &ApiKeyStorage::keyAdded, this, &IntegrationPluginBose::updateConsumerKey);
    connect(apiKeyStorage(), &ApiKeyStorage::keyUpdated, this, &IntegrationPluginBose::updateConsumerKey);
}

void IntegrationPluginBose::setupThing(ThingSetupInfo *info)
{
    if (info->thing()->thingClassId() == soundtouchThingClassId) {

        QString ipAddress;
        QString playerId = info->thing()->paramValue(soundtouchThingPlayerIdParamTypeId).toString();

        foreach (const ZeroConfServiceEntry avahiEntry, m_serviceBrowser->serviceEntries()) {
            QString discoveredPlayerId = avahiEntry.hostName().split(".").first();
            if (discoveredPlayerId == playerId) {
                ipAddress = avahiEntry.hostAddress().toString();
                break;
            }
        }

        if (ipAddress.isEmpty()) {
            // Ok, we could not find an ip on zeroconf... Let's try again in a second while setupInfo hasn't timed out.
            qCDebug(dcBose()) << "Device not found via ZeroConf... Waiting for a second for it to appear...";
            QTimer::singleShot(1000, info, [this, info](){
                setupThing(info);
            });
            return;
        }

        info->thing()->setParamValue(soundtouchThingIpParamTypeId,ipAddress);
        SoundTouch *soundTouch = new SoundTouch(hardwareManager()->networkManager(), ipAddress, this);
        connect(soundTouch, &SoundTouch::connectionChanged, this, &IntegrationPluginBose::onConnectionChanged);
        connect(soundTouch, &SoundTouch::infoReceived, this, &IntegrationPluginBose::onInfoObjectReceived);
        connect(soundTouch, &SoundTouch::nowPlayingReceived, this, &IntegrationPluginBose::onNowPlayingObjectReceived);
        connect(soundTouch, &SoundTouch::volumeReceived, this, &IntegrationPluginBose::onVolumeObjectReceived);
        connect(soundTouch, &SoundTouch::sourcesReceived, this, &IntegrationPluginBose::onSourcesObjectReceived);
        connect(soundTouch, &SoundTouch::bassReceived, this, &IntegrationPluginBose::onBassObjectReceived);
        connect(soundTouch, &SoundTouch::bassCapabilitiesReceived, this, &IntegrationPluginBose::onBassCapabilitiesObjectReceived);
        connect(soundTouch, &SoundTouch::zoneReceived, this, &IntegrationPluginBose::onZoneObjectReceived);
        connect(soundTouch, &SoundTouch::requestExecuted, this, &IntegrationPluginBose::onRequestExecuted);
        connect(soundTouch, &SoundTouch::presetsReceived, this, &IntegrationPluginBose::onPresetsReceived);
        m_soundTouch.insert(info->thing(), soundTouch);
        return info->finish(Thing::ThingErrorNoError);

    } else {
        qCWarning(dcBose()) << "ThingClassId not found" << info->thing()->thingClassId();
        return info->finish(Thing::ThingErrorThingClassNotFound);
    }
}

void IntegrationPluginBose::postSetupThing(Thing *thing)
{
    if (thing->thingClassId() == soundtouchThingClassId) {
        connect(thing, &Thing::nameChanged, this, &IntegrationPluginBose::onDeviceNameChanged);
        SoundTouch *soundTouch = m_soundTouch.value(thing);
        soundTouch->getInfo();
        soundTouch->getNowPlaying();
        soundTouch->getVolume();
        soundTouch->getBass();
        soundTouch->getBassCapabilities();
        soundTouch->getZone();
    }

    if (!m_pluginTimer) {
        m_pluginTimer = hardwareManager()->pluginTimerManager()->registerTimer(2);
        connect(m_pluginTimer, &PluginTimer::timeout, this, &IntegrationPluginBose::onPluginTimer);
    }
}

void IntegrationPluginBose::thingRemoved(Thing *thing)
{
    if (thing->thingClassId() == soundtouchThingClassId) {
        SoundTouch *soundTouch = m_soundTouch.take(thing);
        soundTouch->deleteLater();
    }

    if (m_pluginTimer && myThings().isEmpty()) {
        hardwareManager()->pluginTimerManager()->unregisterTimer(m_pluginTimer);
        m_pluginTimer = nullptr;
    }
}

void IntegrationPluginBose::discoverThings(ThingDiscoveryInfo *info)
{
    QTimer::singleShot(5000, info, [this, info](){
        foreach (const ZeroConfServiceEntry avahiEntry, m_serviceBrowser->serviceEntries()) {
            qCDebug(dcBose) << "Zeroconf entry:" << avahiEntry;

            QString playerId = avahiEntry.hostName().split(".").first();
            ThingDescriptor descriptor(soundtouchThingClassId, avahiEntry.name(), avahiEntry.hostAddress().toString());
            ParamList params;

            foreach (Thing *existingThing, myThings().filterByThingClassId(soundtouchThingClassId)) {
                if (existingThing->paramValue(soundtouchThingPlayerIdParamTypeId).toString() == playerId) {
                    descriptor.setThingId(existingThing->id());
                    break;
                }
            }
            params << Param(soundtouchThingIpParamTypeId, avahiEntry.hostAddress().toString());
            params << Param(soundtouchThingPlayerIdParamTypeId, playerId);
            descriptor.setParams(params);
            info->addThingDescriptor(descriptor);
        }
        info->finish(Thing::ThingErrorNoError);
    });
}

void IntegrationPluginBose::executeAction(ThingActionInfo *info)
{
    Thing *thing = info->thing();
    Action action = info->action();

    if (thing->thingClassId() == soundtouchThingClassId) {
        SoundTouch *soundTouch = m_soundTouch.value(thing);

        if (action.actionTypeId() == soundtouchPowerActionTypeId) {
            //bool power = action.param(soundtouchPowerActionPowerParamTypeId).value().toBool();
            QUuid requestId = soundTouch->setKey(KEY_VALUE::KEY_VALUE_POWER, true); //only toggling possible
            m_pendingActions.insert(requestId, info);
            connect(info, &ThingActionInfo::aborted, this, [requestId, this] {m_pendingActions.remove(requestId);});

        } else if (action.actionTypeId() == soundtouchMuteActionTypeId) {
            QUuid requestId = soundTouch->setKey(KEY_VALUE::KEY_VALUE_MUTE, true); //only toggling possible
            m_pendingActions.insert(requestId, info);
            connect(info, &ThingActionInfo::aborted, this, [requestId, this] {m_pendingActions.remove(requestId);});

        } else if (action.actionTypeId() == soundtouchPlayActionTypeId) {
            QUuid requestId = soundTouch->setKey(KEY_VALUE::KEY_VALUE_PLAY, true);
            m_pendingActions.insert(requestId, info);
            connect(info, &ThingActionInfo::aborted, this, [requestId, this] {m_pendingActions.remove(requestId);});

        } else if (action.actionTypeId() == soundtouchPauseActionTypeId) {
            QUuid requestId = soundTouch->setKey(KEY_VALUE::KEY_VALUE_PAUSE, true);
            m_pendingActions.insert(requestId, info);
            connect(info, &ThingActionInfo::aborted, this, [requestId, this] {m_pendingActions.remove(requestId);});

        } else if (action.actionTypeId() == soundtouchStopActionTypeId) {
            QUuid requestId = soundTouch->setKey(KEY_VALUE::KEY_VALUE_STOP, true);
            m_pendingActions.insert(requestId, info);
            connect(info, &ThingActionInfo::aborted, this, [requestId, this] {m_pendingActions.remove(requestId);});

        } else if (action.actionTypeId() == soundtouchSkipNextActionTypeId) {
            QUuid requestId = soundTouch->setKey(KEY_VALUE::KEY_VALUE_NEXT_TRACK, true);
            m_pendingActions.insert(requestId, info);
            connect(info, &ThingActionInfo::aborted, this, [requestId, this] {m_pendingActions.remove(requestId);});

        } else if (action.actionTypeId() == soundtouchSkipBackActionTypeId) {
            QUuid requestId = soundTouch->setKey(KEY_VALUE::KEY_VALUE_PREV_TRACK, true);
            m_pendingActions.insert(requestId, info);
            connect(info, &ThingActionInfo::aborted, this, [requestId, this] {m_pendingActions.remove(requestId);});

        } else if (action.actionTypeId() == soundtouchShuffleActionTypeId) {

            QUuid requestId;
            bool shuffle =  action.param(soundtouchShuffleActionShuffleParamTypeId).value().toBool();
            if (shuffle) {
                requestId = soundTouch->setKey(KEY_VALUE::KEY_VALUE_SHUFFLE_ON, true);
            } else {
                requestId = soundTouch->setKey(KEY_VALUE::KEY_VALUE_SHUFFLE_OFF, true);
            }
            m_pendingActions.insert(requestId, info);
            connect(info, &ThingActionInfo::aborted, this, [requestId, this] {m_pendingActions.remove(requestId);});

        } else if (action.actionTypeId() == soundtouchRepeatActionTypeId) {

            QUuid requestId;
            QString repeat =  action.param(soundtouchRepeatActionRepeatParamTypeId).value().toString();
            if (repeat == "None") {
                requestId = soundTouch->setKey(KEY_VALUE::KEY_VALUE_REPEAT_OFF, true);
            } else if (repeat == "One") {
                requestId = soundTouch->setKey(KEY_VALUE::KEY_VALUE_REPEAT_ONE, true);
            } else if (repeat == "All") {
                requestId = soundTouch->setKey(KEY_VALUE::KEY_VALUE_REPEAT_ALL, true);
            }
            m_pendingActions.insert(requestId, info);
            connect(info, &ThingActionInfo::aborted, this, [requestId, this] {m_pendingActions.remove(requestId);});

        } else if (action.actionTypeId() == soundtouchVolumeActionTypeId) {

            int volume = action.param(soundtouchVolumeActionVolumeParamTypeId).value().toInt();
            QUuid requestId = soundTouch->setVolume(volume);
            m_pendingActions.insert(requestId, info);
            connect(info, &ThingActionInfo::aborted, this, [requestId, this] {m_pendingActions.remove(requestId);});

        } else if (action.actionTypeId() == soundtouchBassActionTypeId) {

            int bass = action.param(soundtouchBassActionBassParamTypeId).value().toInt();
            QUuid requestId = soundTouch->setBass(bass);
            m_pendingActions.insert(requestId, info);
            connect(info, &ThingActionInfo::aborted, this, [requestId, this] {m_pendingActions.remove(requestId);});

        } else if (action.actionTypeId() == soundtouchPlaybackStatusActionTypeId) {

            QUuid requestId;
            QString status =  action.param(soundtouchPlaybackStatusActionPlaybackStatusParamTypeId).value().toString();
            if (status == "Playing") {
                requestId = soundTouch->setKey(KEY_VALUE::KEY_VALUE_PLAY, true);
            } else if (status == "Paused") {
                requestId = soundTouch->setKey(KEY_VALUE::KEY_VALUE_PAUSE, true);
            } else if (status == "Stopped") {
                requestId = soundTouch->setKey(KEY_VALUE::KEY_VALUE_STOP, true);
            }
            m_pendingActions.insert(requestId, info);
            connect(info, &ThingActionInfo::aborted, this, [requestId, this] {m_pendingActions.remove(requestId);});

        } else if (action.actionTypeId() == soundtouchSavePresetActionTypeId) {

            QUuid requestId;
            QString preset =  action.param(soundtouchSavePresetActionPresetNumberParamTypeId).value().toString();
            if (preset.contains("1")) {
                requestId = soundTouch->setKey(KEY_VALUE::KEY_VALUE_PRESET_1, true);
            } else if (preset.contains("2")) {
                requestId = soundTouch->setKey(KEY_VALUE::KEY_VALUE_PRESET_2, true);
            } else if (preset.contains("3")) {
                requestId = soundTouch->setKey(KEY_VALUE::KEY_VALUE_PRESET_3, true);
            } else if (preset.contains("4")) {
                requestId = soundTouch->setKey(KEY_VALUE::KEY_VALUE_PRESET_4, true);
            } else if (preset.contains("5")) {
                requestId = soundTouch->setKey(KEY_VALUE::KEY_VALUE_PRESET_5, true);
            } else if (preset.contains("6")) {
                requestId = soundTouch->setKey(KEY_VALUE::KEY_VALUE_PRESET_6, true);
            } else {
                qCWarning(dcBose()) << "Unhandled preset number: " << preset;
                info->finish(Thing::ThingErrorInvalidParameter);
                return;
            }
            m_pendingActions.insert(requestId, info);
            connect(info, &ThingActionInfo::aborted, this, [requestId, this] {m_pendingActions.remove(requestId);});
        } else if (action.actionTypeId() == soundtouchAlertActionTypeId) {
            if (m_consumerKey.isEmpty()) {
                return info->finish(Thing::ThingErrorHardwareFailure, tr("No consumer key available."));
            }
            QUrl alertUrl;
            QString alertSound = action.param(soundtouchAlertActionSoundParamTypeId).value().toString();
            if (alertSound == "Doorbell") {
                alertUrl = configValue(bosePluginDoorbellSoundUrlParamTypeId).toString();
            } else if (alertSound == "Notification") {
                alertUrl = configValue(bosePluginNotificationSoundUrlParamTypeId).toString();
            }
            if (!alertUrl.isValid()) {
                return info->finish(Thing::ThingErrorHardwareFailure, tr("Sound URL is not valid."));
            }
            PlayInfoObject playInfo;
            playInfo.url = alertUrl.toString();
            playInfo.appKey = m_consumerKey;
            playInfo.services = "nymea";
            playInfo.volume = action.param(soundtouchAlertActionVolumeParamTypeId).value().toInt();
            playInfo.reason = action.param(soundtouchAlertActionMessageParamTypeId).value().toString();
            QUuid requestId = soundTouch->setSpeaker(playInfo);
            m_pendingActions.insert(requestId, info);
            connect(info, &ThingActionInfo::aborted, this, [requestId, this] {m_pendingActions.remove(requestId);});
        } else {
            qCWarning(dcBose()) << "ActionTypeId not found" << action.actionTypeId();
            return info->finish(Thing::ThingErrorActionTypeNotFound);
        }
    } else {
        qCWarning(dcBose()) << "ThingClassId not found" << thing->thingClassId();
        return info->finish(Thing::ThingErrorThingClassNotFound);
    }
}

void IntegrationPluginBose::browseThing(BrowseResult *result)
{
    Thing *thing = result->thing();
    if (thing->thingClassId() == soundtouchThingClassId) {
        SoundTouch *soundTouch = m_soundTouch.value(thing);
        if (result->itemId() == "presets") {
            QUuid requestId = soundTouch->getPresets();
            m_asyncBrowseResults.insert(requestId, result);
            connect(result, &BrowseResult::aborted, this, [this, requestId]{m_asyncBrowseResults.remove(requestId);});
        } else {
            BrowserItem presetItem("presets", "Presets", true, false);
            presetItem.setIcon(BrowserItem::BrowserIcon::BrowserIconFavorites);
            QUuid requestId = soundTouch->getSources();
            result->addItem(presetItem);
            m_asyncBrowseResults.insert(requestId, result);
            connect(result, &BrowseResult::aborted, this, [this, requestId]{m_asyncBrowseResults.remove(requestId);});
        }
    }
}

void IntegrationPluginBose::browserItem(BrowserItemResult *result)
{
    Thing *thing = result->thing();
    if (thing->thingClassId() == soundtouchThingClassId) {
        SoundTouch *soundTouch = m_soundTouch.value(thing);

        if (result->itemId() == "presets") {
            QUuid requestId = soundTouch->getPresets();
            m_asyncBrowseItemResults.insert(requestId, result);
            connect(result, &BrowserItemResult::aborted, this, [this, requestId]{m_asyncBrowseItemResults.remove(requestId);});
        } else {
            BrowserItem presetItem("presets", "Presets", true, false);
            presetItem.setIcon(BrowserItem::BrowserIcon::BrowserIconFavorites);
            QUuid requestId = soundTouch->getSources();
            m_asyncBrowseItemResults.insert(requestId, result);
            connect(result, &BrowserItemResult::aborted, this, [this, requestId]{m_asyncBrowseItemResults.remove(requestId);});
        }
    }
}

void IntegrationPluginBose::executeBrowserItem(BrowserActionInfo *info)
{
    Thing *thing = info->thing();
    if (thing->thingClassId() == soundtouchThingClassId) {
        SoundTouch *soundTouch = m_soundTouch.value(thing);
        if (info->browserAction().itemId().startsWith("presets")) {
            QUuid requestId;
            int number = info->browserAction().itemId().split("&").last().toInt();
            if (number == 1) {
                requestId = soundTouch->setKey(KEY_VALUE::KEY_VALUE_PRESET_1, false);
            } else if (number == 2) {
                requestId = soundTouch->setKey(KEY_VALUE::KEY_VALUE_PRESET_2, false);
            } else if (number == 3) {
                requestId = soundTouch->setKey(KEY_VALUE::KEY_VALUE_PRESET_3, false);
            } else if (number == 4) {
                requestId = soundTouch->setKey(KEY_VALUE::KEY_VALUE_PRESET_4, false);
            } else if (number == 5) {
                requestId = soundTouch->setKey(KEY_VALUE::KEY_VALUE_PRESET_5, false);
            } else if (number == 6) {
                requestId = soundTouch->setKey(KEY_VALUE::KEY_VALUE_PRESET_6, false);
            } else {
                qCWarning(dcBose()) << "Unhandled preset number: " << number;
                info->finish(Thing::ThingErrorInvalidParameter);
                return;
            }
            m_asyncExecuteBrowseItems.insert(requestId, info);
            connect(info, &BrowserActionInfo::aborted, this, [this, requestId]{m_asyncExecuteBrowseItems.remove(requestId);});
        } else {
            SourcesObject sources = m_sourcesObjects.value(thing);
            foreach (SourceItemObject source, sources.sourceItems) {
                if (source.source == info->browserAction().itemId()) {
                    ContentItemObject contentItem;
                    contentItem.source = source.source;
                    contentItem.sourceAccount = source.sourceAccount;
                    QUuid requestId = soundTouch->setSource(contentItem);
                    m_asyncExecuteBrowseItems.insert(requestId, info);
                    connect(info, &BrowserActionInfo::aborted, this, [this, requestId]{m_asyncExecuteBrowseItems.remove(requestId);});
                    break;
                }
            }
        }
    }
}

void IntegrationPluginBose::updateConsumerKey()
{
    QString key = configValue(bosePluginCustomConsumerKeyParamTypeId).toString();
    if (!key.isEmpty()) {
        m_consumerKey = key;
        return;
    }
    key = apiKeyStorage()->requestKey("bose").data("consumerKey");
    if (!key.isEmpty()) {
        m_consumerKey = key;
        return;
    }
    qCWarning(dcBose()) << "No API key set.";
    qCWarning(dcBose()) << "Either install an API key pacakge (nymea-apikeysprovider-plugin-*) or provide a key in the plugin settings.";
}

void IntegrationPluginBose::onPluginTimer()
{
    foreach(SoundTouch *soundTouch, m_soundTouch.values()) {
        soundTouch->getInfo();
        soundTouch->getNowPlaying();
        soundTouch->getVolume();
        soundTouch->getBass();
        soundTouch->getZone();
    }
}

void IntegrationPluginBose::onConnectionChanged(bool connected)
{
    SoundTouch *soundtouch = static_cast<SoundTouch *>(sender());
    Thing *thing = m_soundTouch.key(soundtouch);
    thing->setStateValue(soundtouchConnectedStateTypeId, connected);
}

void IntegrationPluginBose::onDeviceNameChanged()
{
    Thing *thing = static_cast<Thing*>(sender());
    SoundTouch *soundtouch = m_soundTouch.value(thing);
    soundtouch->setName(thing->name());
}

void IntegrationPluginBose::onRequestExecuted(QUuid requestId, bool success)
{
    if (m_pendingActions.contains(requestId)) {
        ThingActionInfo *info = m_pendingActions.value(requestId);
        if (success) {
            info->finish(Thing::ThingErrorNoError);
        } else {
            info->finish(Thing::ThingErrorHardwareFailure);
        }
    } else if (m_asyncBrowseResults.contains(requestId)) {
        if (!success) {
            BrowseResult *result = m_asyncBrowseResults.take(requestId);
            result->finish(Thing::ThingErrorHardwareFailure);
        }
    } else if (m_asyncExecuteBrowseItems.contains(requestId)) {
        BrowserActionInfo *info = m_asyncExecuteBrowseItems.take(requestId);
        if (success) {
            info->finish(Thing::ThingErrorNoError);
        } else {
            info->finish(Thing::ThingErrorHardwareFailure);
        }
    } else if (m_asyncBrowseItemResults.contains(requestId)) {
        if (!success) {
            BrowserItemResult *result = m_asyncBrowseItemResults.take(requestId);
            result->finish(Thing::ThingErrorHardwareFailure);
        }
    } else {
        //This request was not an action or browse request
    }
}

void IntegrationPluginBose::onInfoObjectReceived(QUuid requestId, InfoObject infoObject)
{
    Q_UNUSED(requestId);
    SoundTouch *soundtouch = static_cast<SoundTouch *>(sender());
    Thing *thing = m_soundTouch.key(soundtouch);
    thing->setName(infoObject.name);
}

void IntegrationPluginBose::onNowPlayingObjectReceived(QUuid requestId, NowPlayingObject nowPlaying)
{
    Q_UNUSED(requestId);
    SoundTouch *soundtouch = static_cast<SoundTouch *>(sender());
    Thing *thing = m_soundTouch.key(soundtouch);

    thing->setStateValue(soundtouchPowerStateTypeId, !(nowPlaying.source.toUpper() == "STANDBY"));
    thing->setStateValue(soundtouchSourceStateTypeId, nowPlaying.source);
    thing->setStateValue(soundtouchTitleStateTypeId, nowPlaying.track);
    thing->setStateValue(soundtouchArtistStateTypeId, nowPlaying.artist);
    thing->setStateValue(soundtouchCollectionStateTypeId, nowPlaying.album);
    thing->setStateValue(soundtouchArtworkStateTypeId, nowPlaying.art.url);
    thing->setStateValue(soundtouchShuffleStateTypeId, ( nowPlaying.shuffleSetting == SHUFFLE_STATUS_SHUFFLE_ON ));

    switch (nowPlaying.repeatSettings)  {
    case REPEAT_STATUS_REPEAT_ONE:
        thing->setStateValue(soundtouchRepeatStateTypeId, "One");
        break;
    case REPEAT_STATUS_REPEAT_ALL:
        thing->setStateValue(soundtouchRepeatStateTypeId, "All");
        break;
    case REPEAT_STATUS_REPEAT_OFF:
        thing->setStateValue(soundtouchRepeatStateTypeId, "None");
        break;
    }

    switch (nowPlaying.playStatus) {
    case PLAY_STATUS_PLAY_STATE:
        thing->setStateValue(soundtouchPlaybackStatusStateTypeId, "Playing");
        break;
    case PLAY_STATUS_PAUSE_STATE:
    case PLAY_STATUS_BUFFERING_STATE:
        thing->setStateValue(soundtouchPlaybackStatusStateTypeId, "Paused");
        break;
    case PLAY_STATUS_STOP_STATE:
        thing->setStateValue(soundtouchPlaybackStatusStateTypeId, "Stopped");
        break;
    }
}

void IntegrationPluginBose::onVolumeObjectReceived(QUuid requestId, VolumeObject volume)
{
    Q_UNUSED(requestId);
    SoundTouch *soundtouch = static_cast<SoundTouch *>(sender());
    Thing *thing = m_soundTouch.key(soundtouch);
    thing->setStateValue(soundtouchVolumeStateTypeId, volume.actualVolume);
    thing->setStateValue(soundtouchMuteStateTypeId, volume.muteEnabled);
}

void IntegrationPluginBose::onSourcesObjectReceived(QUuid requestId, SourcesObject sources)
{
    SoundTouch *soundtouch = static_cast<SoundTouch *>(sender());
    Thing *thing = m_soundTouch.key(soundtouch);
    m_sourcesObjects.insert(thing, sources);

    if (m_asyncBrowseResults.contains(requestId)) {
        BrowseResult *result = m_asyncBrowseResults.value(requestId);
        foreach (SourceItemObject sourceItem, sources.sourceItems) {
            qDebug(dcBose()) << "Source:" << sourceItem.source;
            if (sourceItem.source == "BLUETOOTH") {
                MediaBrowserItem item(sourceItem.source, sourceItem.source, false, true);
                item.setDescription(sourceItem.sourceAccount);
                item.setIcon(BrowserItem::BrowserIcon::BrowserIconMusic);
                item.setMediaIcon(MediaBrowserItem::MediaBrowserIcon::MediaBrowserIconBluetooth);
                result->addItem(item);
            } else if (sourceItem.source == "AUX") {
                MediaBrowserItem item(sourceItem.source, sourceItem.source, false, true);
                item.setDescription(sourceItem.sourceAccount);
                item.setIcon(BrowserItem::BrowserIcon::BrowserIconMusic);
                result->addItem(item);
                item.setMediaIcon(MediaBrowserItem::MediaBrowserIcon::MediaBrowserIconAux);
            } else if ((sourceItem.source == "SPOTIFY") && (sourceItem.status == SOURCE_STATUS_READY)) {
                MediaBrowserItem item(sourceItem.source, sourceItem.source, false, true);
                item.setDescription(sourceItem.sourceAccount);
                item.setIcon(BrowserItem::BrowserIcon::BrowserIconMusic);
                item.setMediaIcon(MediaBrowserItem::MediaBrowserIcon::MediaBrowserIconSpotify);
                result->addItem(item);
            }
        }
        return result->finish(Thing::ThingErrorNoError);
    } else if (m_asyncBrowseItemResults.contains(requestId)) {
        BrowserItemResult *result = m_asyncBrowseItemResults.value(requestId);
        foreach (SourceItemObject sourceItem, sources.sourceItems) {
            if (sourceItem.source == result->itemId()) {
                return result->finish(Thing::ThingErrorNoError);
            }
        }
        return result->finish(Thing::ThingErrorItemNotFound);
    } else {
        qCWarning(dcBose()) << "Received sources without an associated BrowseResult";
    }
}

void IntegrationPluginBose::onBassObjectReceived(QUuid requestId, BassObject bass)
{
    Q_UNUSED(requestId);
    SoundTouch *soundtouch = static_cast<SoundTouch *>(sender());
    Thing *thing = m_soundTouch.key(soundtouch);
    thing->setStateValue(soundtouchBassStateTypeId, bass.actualBass);
}

void IntegrationPluginBose::onBassCapabilitiesObjectReceived(QUuid requestId, BassCapabilitiesObject bassCapabilities)
{
    Q_UNUSED(requestId);
    qDebug(dcBose()) << "Bass capabilities (max, min, default):" << bassCapabilities.bassMax << bassCapabilities.bassMin << bassCapabilities.bassDefault;
}

void IntegrationPluginBose::onGroupObjectReceived(QUuid requestId, GroupObject group)
{
    Q_UNUSED(requestId);
    qDebug(dcBose())  << "Group" << group.name << group.status;
    foreach (RolesObject role, group.roles) {
        qDebug(dcBose()) << "-> member:" << role.groupRole.deviceID;
    }
}

void IntegrationPluginBose::onZoneObjectReceived(QUuid requestId, ZoneObject zone)
{
    Q_UNUSED(requestId);
    foreach (MemberObject member, zone.members) {
        qDebug(dcBose()) << "-> member:" << member.deviceID;
    }
}

void IntegrationPluginBose::onPresetsReceived(QUuid requestId, QList<PresetObject> presets)
{
    if (m_asyncBrowseResults.contains(requestId)) {
        BrowseResult *result = m_asyncBrowseResults.take(requestId);
        foreach (PresetObject preset, presets) {
            BrowserItem item("presets&"+QString::number(preset.presetId), QString("Preset %1").arg(QString::number(preset.presetId)), false, true);
            item.setDescription(preset.ContentItem.source+" "+preset.ContentItem.itemName);
            item.setThumbnail(preset.ContentItem.containerArt);
            result->addItem(item);
        }
        result->finish(Thing::ThingErrorNoError);
    }

    if (m_asyncBrowseItemResults.contains(requestId)) {
        BrowserItemResult *result = m_asyncBrowseItemResults.value(requestId);
        foreach (PresetObject preset, presets) {
            if (preset.presetId == result->itemId().split("&").last().toInt()) {
                return result->finish(Thing::ThingErrorNoError);
            }
        }
        return result->finish(Thing::ThingErrorItemNotFound);
    }
}
