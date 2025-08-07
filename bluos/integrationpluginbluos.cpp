/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*
* Copyright 2013 - 2025, nymea GmbH
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


#include "integrationpluginbluos.h"
#include "plugininfo.h"

#include <integrations/thing.h>
#include <types/mediabrowseritem.h>
#include <types/browseritem.h>

#include <QDebug>
#include <QStringList>
#include <QJsonDocument>
#include <QTimer>
#include <QtGlobal>


IntegrationPluginBluOS::IntegrationPluginBluOS()
{

}

void IntegrationPluginBluOS::init()
{
    m_serviceBrowser = hardwareManager()->zeroConfController()->createServiceBrowser("_musc._tcp");
}

void IntegrationPluginBluOS::discoverThings(ThingDiscoveryInfo *info)
{
    QTimer::singleShot(5000, info, [this, info](){
        foreach (const ZeroConfServiceEntry avahiEntry, m_serviceBrowser->serviceEntries()) {
            qCDebug(dcBluOS()) << "Zeroconf entry:" << avahiEntry;

            QString playerId = avahiEntry.hostName().split(".").first();
            ThingDescriptor descriptor(bluosPlayerThingClassId, avahiEntry.name(), avahiEntry.hostAddress().toString());
            ParamList params;

            foreach (Thing *existingDevice, myThings().filterByThingClassId(bluosPlayerThingClassId)) {
                if (existingDevice->paramValue(bluosPlayerThingSerialNumberParamTypeId).toString() == playerId) {
                    descriptor.setThingId(existingDevice->id());
                    break;
                }
            }
            params << Param(bluosPlayerThingAddressParamTypeId, avahiEntry.hostAddress().toString());
            params << Param(bluosPlayerThingPortParamTypeId, avahiEntry.port());
            params << Param(bluosPlayerThingSerialNumberParamTypeId, playerId);
            descriptor.setParams(params);
            info->addThingDescriptor(descriptor);
        }
        info->finish(Thing::ThingErrorNoError);
    });
}

void IntegrationPluginBluOS::setupThing(ThingSetupInfo *info)
{
    Thing *thing = info->thing();

    if (thing->thingClassId() == bluosPlayerThingClassId) {
        qCDebug(dcBluOS()) << "Setup BluOS device" << thing->paramValue(bluosPlayerThingAddressParamTypeId).toString();

        QHostAddress address(thing->paramValue(bluosPlayerThingAddressParamTypeId).toString());
        int port = thing->paramValue(bluosPlayerThingPortParamTypeId).toInt();
        BluOS *bluos = new BluOS(hardwareManager()->networkManager() , address, port, this);
        connect(bluos, &BluOS::connectionChanged, this, &IntegrationPluginBluOS::onConnectionChanged);
        connect(bluos, &BluOS::statusReceived, this, &IntegrationPluginBluOS::onStatusResponseReceived);
        connect(bluos, &BluOS::actionExecuted, this, &IntegrationPluginBluOS::onActionExecuted);
        connect(bluos, &BluOS::volumeReceived, this, &IntegrationPluginBluOS::onVolumeReceived);
        connect(bluos, &BluOS::presetsReceived, this, &IntegrationPluginBluOS::onPresetsReceived);
        connect(bluos, &BluOS::sourcesReceived, this, &IntegrationPluginBluOS::onSourcesReceived);
        connect(bluos, &BluOS::shuffleStateReceived, this, &IntegrationPluginBluOS::onShuffleStateReceived);
        connect(bluos, &BluOS::repeatModeReceived, this, &IntegrationPluginBluOS::onRepeatModeReceived);

        m_asyncSetup.insert(bluos, info);
        bluos->getStatus();
        // In case the setup is cancelled before we finish it...
        connect(info, &ThingSetupInfo::aborted, this, [this, bluos] {
            m_asyncSetup.remove(bluos);
            bluos->deleteLater();
        });
        return;
    } else {
        return info->finish(Thing::ThingErrorThingClassNotFound);
    }
}

void IntegrationPluginBluOS::postSetupThing(Thing *thing)
{
    Q_UNUSED(thing)

    if (!m_pluginTimer) {
        m_pluginTimer = hardwareManager()->pluginTimerManager()->registerTimer(10);
        connect(m_pluginTimer, &PluginTimer::timeout, [this] {
            foreach(BluOS *bluos, m_bluos) {
                bluos->getStatus();
            }
        });
    }
}

void IntegrationPluginBluOS::thingRemoved(Thing *thing)
{
    if (thing->thingClassId() == bluosPlayerThingClassId) {
        BluOS *bluos = m_bluos.take(thing->id());
        bluos->deleteLater();
    } else {
        qCWarning(dcBluOS()) << "Things removed, unhandled thing class id";
    }
}

void IntegrationPluginBluOS::executeAction(ThingActionInfo *info)
{
    Thing *thing = info->thing();
    Action action = info->action();

    if (thing->thingClassId() == bluosPlayerThingClassId) {
        BluOS *bluos = m_bluos.value(thing->id());
        if (!bluos) {
            return info->finish(Thing::ThingErrorHardwareFailure);
        }
        if (action.actionTypeId() == bluosPlayerPlaybackStatusActionTypeId) {
            QString playbakStatus = action.param(bluosPlayerPlaybackStatusActionPlaybackStatusParamTypeId).value().toString();
            QUuid requestId;
            if (playbakStatus == "Playing") {
                requestId = bluos->play();
            } else if (playbakStatus == "Paused") {
                requestId = bluos->pause();
            } else if (playbakStatus == "Stopped") {
                requestId = bluos->stop();
            } else {
                qCWarning(dcBluOS()) << "Unhandled Playback mode";
            }
            m_asyncActions.insert(requestId, info);
            connect(info, &ThingActionInfo::aborted, this, [this, requestId] {m_asyncActions.remove(requestId);});
        } else if (action.actionTypeId() == bluosPlayerPlayActionTypeId) {
            QUuid requestId = bluos->play();
            m_asyncActions.insert(requestId, info);
            connect(info, &ThingActionInfo::aborted, this, [this, requestId] {m_asyncActions.remove(requestId);});
        } else if (action.actionTypeId() == bluosPlayerPauseActionTypeId) {
            QUuid requestId = bluos->pause();
            m_asyncActions.insert(requestId, info);
            connect(info, &ThingActionInfo::aborted, this, [this, requestId] {m_asyncActions.remove(requestId);});
        } else if (action.actionTypeId() == bluosPlayerStopActionTypeId) {
            QUuid requestId = bluos->stop();
            m_asyncActions.insert(requestId, info);
            connect(info, &ThingActionInfo::aborted, this, [this, requestId] {m_asyncActions.remove(requestId);});
        } else if (action.actionTypeId() == bluosPlayerSkipNextActionTypeId) {
            QUuid requestId = bluos->skip();
            m_asyncActions.insert(requestId, info);
            connect(info, &ThingActionInfo::aborted, this, [this, requestId] {m_asyncActions.remove(requestId);});
        } else if (action.actionTypeId() == bluosPlayerSkipBackActionTypeId) {
            QUuid requestId = bluos->back();
            m_asyncActions.insert(requestId, info);
            connect(info, &ThingActionInfo::aborted, this, [this, requestId] {m_asyncActions.remove(requestId);});
        } else if (action.actionTypeId() == bluosPlayerVolumeActionTypeId) {
            uint volume = action.param(bluosPlayerVolumeActionVolumeParamTypeId).value().toUInt();
            QUuid requestId = bluos->setVolume(volume);
            m_asyncActions.insert(requestId, info);
            connect(info, &ThingActionInfo::aborted, this, [this, requestId] {m_asyncActions.remove(requestId);});
        } else if (action.actionTypeId() == bluosPlayerMuteActionTypeId) {
            bool mute = action.param(bluosPlayerMuteActionMuteParamTypeId).value().toBool();
            QUuid requestId = bluos->setMute(mute);
            m_asyncActions.insert(requestId, info);
            connect(info, &ThingActionInfo::aborted, this, [this, requestId] {m_asyncActions.remove(requestId);});
        } else if (action.actionTypeId() == bluosPlayerShuffleActionTypeId) {
            bool shuffle = action.param(bluosPlayerShuffleActionShuffleParamTypeId).value().toBool();
            QUuid requestId = bluos->setShuffle(shuffle);
            m_asyncActions.insert(requestId, info);
            connect(info, &ThingActionInfo::aborted, this, [this, requestId] {m_asyncActions.remove(requestId);});
        }  else if (action.actionTypeId() == bluosPlayerRepeatActionTypeId) {
            QString repeat = action.param(bluosPlayerRepeatActionRepeatParamTypeId).value().toString();
            QUuid requestId;
            if (repeat == "One") {
                requestId = bluos->setRepeat(BluOS::RepeatMode::One);
            } else if (repeat == "All") {
                requestId = bluos->setRepeat(BluOS::RepeatMode::All);
            } else if (repeat == "None") {
                requestId = bluos->setRepeat(BluOS::RepeatMode::None);
            } else {
                qCWarning(dcBluOS()) << "Unhandled Repeat Mode";
            }
            m_asyncActions.insert(requestId, info);
            connect(info, &ThingActionInfo::aborted, this, [this, requestId] {m_asyncActions.remove(requestId);});
        } else if (action.actionTypeId() == bluosPlayerIncreaseVolumeActionTypeId) {
            uint step = action.param(bluosPlayerIncreaseVolumeActionStepParamTypeId).value().toUInt();
            QUuid requestId = bluos->setVolume(qMin<uint>(100, thing->stateValue(bluosPlayerVolumeStateTypeId).toUInt() + step));
            m_asyncActions.insert(requestId, info);
            connect(info, &ThingActionInfo::aborted, this, [this, requestId] {m_asyncActions.remove(requestId);});
        } else if (action.actionTypeId() == bluosPlayerDecreaseVolumeActionTypeId) {
            uint step = action.param(bluosPlayerDecreaseVolumeActionStepParamTypeId).value().toUInt();
            QUuid requestId = bluos->setVolume(qMax<uint>(0, thing->stateValue(bluosPlayerVolumeStateTypeId).toUInt() - step));
            m_asyncActions.insert(requestId, info);
            connect(info, &ThingActionInfo::aborted, this, [this, requestId] {m_asyncActions.remove(requestId);});
        } else {
            qCWarning(dcBluOS()) << "Execute Action, unhandled action type id" << action.actionTypeId();
            return info->finish(Thing::ThingErrorThingClassNotFound);
        }
    } else {
        qCWarning(dcBluOS()) << "Execute Action, unhandled thing class id" << thing->thingClassId();
        return info->finish(Thing::ThingErrorThingClassNotFound);
    }
}

void IntegrationPluginBluOS::browseThing(BrowseResult *result)
{
    Thing *thing = result->thing();
    if (thing->thingClassId() == bluosPlayerThingClassId) {
        BluOS *bluos = m_bluos.value(thing->id());
        if (!bluos) {
            qCWarning(dcBluOS()) << "Could not find any BluOS object that belongs to" << thing->name();
            result->finish(Thing::ThingErrorHardwareNotAvailable, "BluOS connection not properly initialized");
            return;
        }
        if (result->itemId() == "presets") {
            QUuid requestId = bluos->listPresets();
            m_asyncBrowseResults.insert(requestId, result);
            connect(result, &BrowseResult::aborted, this, [this, requestId]{m_asyncBrowseResults.remove(requestId);});
        } else if (result->itemId() == "grouping") {
            foreach (const ZeroConfServiceEntry avahiEntry, m_serviceBrowser->serviceEntries()) {
                qCDebug(dcBluOS()) << "Zeroconf entry:" << avahiEntry;

                QString playerId = avahiEntry.hostName().split(".").first();
                if (thing->paramValue(bluosPlayerThingSerialNumberParamTypeId).toString() == playerId) {
                    continue;
                }
                MediaBrowserItem groupingItem("grouping&"+avahiEntry.hostAddress().toString()+"&"+avahiEntry.port(), avahiEntry.name(), true, false);
                groupingItem.setDescription(avahiEntry.hostAddress().toString());
                groupingItem.setMediaIcon(MediaBrowserItem::MediaBrowserIconNetwork);
                groupingItem.setIcon(BrowserItem::BrowserIconMusic);
                result->addItem(groupingItem);
            }
        } else if (result->itemId().isEmpty()) {
            MediaBrowserItem presetItem("presets", "Presets", true, false);
            presetItem.setIcon(BrowserItem::BrowserIcon::BrowserIconFavorites);
            presetItem.setMediaIcon(MediaBrowserItem::MediaBrowserIconMusicLibrary);
            result->addItem(presetItem);

            // MediaBrowserItem groupingItem("grouping", "Grouping", true, false);
            // groupingItem.setIcon(BrowserItem::BrowserIcon::BrowserIconApplication);
            // groupingItem.setMediaIcon(MediaBrowserItem::MediaBrowserIconNetwork);
            // result->addItem(groupingItem);

            QUuid requestId = bluos->getSources();
            m_asyncBrowseResults.insert(requestId, result);
            connect(result, &BrowseResult::aborted, this, [this, requestId]{m_asyncBrowseResults.remove(requestId);});
        } else {
            QUuid requestId = bluos->browseSource(result->itemId());
            m_asyncBrowseResults.insert(requestId, result);
            connect(result, &BrowseResult::aborted, this, [this, requestId]{m_asyncBrowseResults.remove(requestId);});
        }
    }
}

void IntegrationPluginBluOS::browserItem(BrowserItemResult *result)
{
    Thing *thing = result->thing();
    if (thing->thingClassId() == bluosPlayerThingClassId) {
        BluOS *bluos = m_bluos.value(thing->id());
        if (!bluos) {
            qCWarning(dcBluOS()) << "Could not find any BluOS object that belongs to" << thing->name();
            return;
        }
        if (result->itemId() == "presets") {
            QUuid requestId = bluos->listPresets();
            m_asyncBrowseItemResults.insert(requestId, result);
            connect(result, &BrowserItemResult::aborted, this, [this, requestId]{m_asyncBrowseItemResults.remove(requestId);});
        } else {
            BrowserItem presetItem("presets", "Presets", true, false);
            presetItem.setIcon(BrowserItem::BrowserIcon::BrowserIconFavorites);
            QUuid requestId = bluos->getSources();
            m_asyncBrowseItemResults.insert(requestId, result);
            connect(result, &BrowserItemResult::aborted, this, [this, requestId]{m_asyncBrowseItemResults.remove(requestId);});
        }
    }
}

void IntegrationPluginBluOS::executeBrowserItem(BrowserActionInfo *info)
{
    Thing *thing = info->thing();
    if (thing->thingClassId() == bluosPlayerThingClassId) {
        BluOS *bluos = m_bluos.value(thing->id());
        if (!bluos) {
            qCWarning(dcBluOS()) << "Could not find any BluOS object that belongs to" << thing->name();
            return;
        }

        if (info->browserAction().itemId().startsWith("presets")) {
            QUuid requestId;
            int presetId = info->browserAction().itemId().split("&").last().toInt();
            requestId = bluos->loadPreset(presetId);
            m_asyncExecuteBrowseItems.insert(requestId, info);
            connect(info, &BrowserActionInfo::aborted, this, [this, requestId]{m_asyncExecuteBrowseItems.remove(requestId);});
        } else  if (info->browserAction().itemId().startsWith("grouping")) {
            //TODO Grouping
            //Test devices are required
        } else {
            //TODO Sources
            //Test services are required
        }
    }
}

void IntegrationPluginBluOS::onConnectionChanged(bool connected)
{
    BluOS *bluos = static_cast<BluOS*>(sender());

    if (m_asyncSetup.contains(bluos)) {
        ThingSetupInfo *info = m_asyncSetup.take(bluos);
        if (connected) {
            m_bluos.insert(info->thing()->id(), bluos);
            info->thing()->setStateValue(bluosPlayerConnectedStateTypeId, true);
            info->finish(Thing::ThingErrorNoError);
        } else {
            bluos->deleteLater();
            info->finish(Thing::ThingErrorSetupFailed);
        }
    } else {
        Thing *thing = myThings().findById(m_bluos.key(bluos));
        if (!thing) {
            qCWarning(dcBluOS()) << "Could not find any Thing that belongs to the BluOS object";
            return;
        }
        thing->setStateValue(bluosPlayerConnectedStateTypeId, connected);
    }
}

void IntegrationPluginBluOS::onStatusResponseReceived(const BluOS::StatusResponse &status)
{
    BluOS *bluos = static_cast<BluOS*>(sender());
    Thing *thing = myThings().findById(m_bluos.key(bluos));
    if (!thing){
        qCWarning(dcBluOS()) << "Could not find any Thing that belongs to this BluOS object";
        return;
    }
    thing->setStateValue(bluosPlayerArtistStateTypeId, status.Artist);
    thing->setStateValue(bluosPlayerCollectionStateTypeId, status.Album);
    thing->setStateValue(bluosPlayerTitleStateTypeId, status.Title);
    thing->setStateValue(bluosPlayerSourceStateTypeId, status.Service);
    thing->setStateValue(bluosPlayerArtworkStateTypeId, status.Image);
    switch (status.State) {
    case BluOS::PlaybackState::Playing:
    case BluOS::PlaybackState::Streaming:
        thing->setStateValue(bluosPlayerPlaybackStatusStateTypeId, "Playing");
        break;
    case BluOS::PlaybackState::Paused:
        thing->setStateValue(bluosPlayerPlaybackStatusStateTypeId, "Paused");
        break;
    case BluOS::PlaybackState::Stopped:
        thing->setStateValue(bluosPlayerPlaybackStatusStateTypeId, "Stopped");
        break;
    default:
        thing->setStateValue(bluosPlayerPlaybackStatusStateTypeId, "Stopped");
        break;
    }

    thing->setStateValue(bluosPlayerMuteStateTypeId, status.Mute);
    thing->setStateValue(bluosPlayerVolumeStateTypeId, status.Volume);
    thing->setStateValue(bluosPlayerShuffleStateTypeId, status.Shuffle);
    switch (status.Repeat) {
    case BluOS::RepeatMode::All:
        thing->setStateValue(bluosPlayerRepeatStateTypeId, "All");
        break;
    case BluOS::RepeatMode::One:
        thing->setStateValue(bluosPlayerRepeatStateTypeId, "One");
        break;
    case BluOS::RepeatMode::None:
        thing->setStateValue(bluosPlayerRepeatStateTypeId, "None");
        break;
    }
    thing->setStateValue(bluosPlayerGroupStateTypeId, status.Group);
}

void IntegrationPluginBluOS::onActionExecuted(QUuid requestId, bool success)
{
    if (m_asyncActions.contains(requestId)) {
        ThingActionInfo *info = m_asyncActions.take(requestId);
        if (success) {
            info->finish(Thing::ThingErrorNoError);
        } else {
            info->finish(Thing::ThingErrorHardwareNotAvailable);
        }
    }
    if (m_asyncExecuteBrowseItems.contains(requestId)) {
        BrowserActionInfo *info = m_asyncExecuteBrowseItems.take(requestId);
        if (success) {
            info->finish(Thing::ThingErrorNoError);
        } else {
            info->finish(Thing::ThingErrorHardwareFailure);
        }
        emit m_pluginTimer->timeout(); // get a status update
    }
}

void IntegrationPluginBluOS::onVolumeReceived(int volume, bool mute)
{
    BluOS *bluos = static_cast<BluOS*>(sender());
    Thing *thing = myThings().findById(m_bluos.key(bluos));
    if (!thing){
        qCWarning(dcBluOS()) << "Could not find any Thing that belongs to this BluOS object";
        return;
    }
    thing->setStateValue(bluosPlayerMuteStateTypeId, mute);
    thing->setStateValue(bluosPlayerVolumeStateTypeId, volume);
}

void IntegrationPluginBluOS::onShuffleStateReceived(bool state)
{
    BluOS *bluos = static_cast<BluOS*>(sender());
    Thing *thing = myThings().findById(m_bluos.key(bluos));
    if (!thing)
        return;
   thing->setStateValue(bluosPlayerShuffleStateTypeId, state);
}

void IntegrationPluginBluOS::onRepeatModeReceived(BluOS::RepeatMode mode)
{
    BluOS *bluos = static_cast<BluOS*>(sender());
    Thing *thing = myThings().findById(m_bluos.key(bluos));
    if (!thing){
        qCWarning(dcBluOS()) << "Could not find any Thing that belongs to this BluOS object";
        return;
    }
    switch (mode) {
    case BluOS::RepeatMode::All:
        thing->setStateValue(bluosPlayerRepeatStateTypeId, "All");
        break;
    case BluOS::RepeatMode::One:
        thing->setStateValue(bluosPlayerRepeatStateTypeId, "One");
        break;
    case BluOS::RepeatMode::None:
        thing->setStateValue(bluosPlayerRepeatStateTypeId, "None");
        break;
    }

}

void IntegrationPluginBluOS::onPresetsReceived(QUuid requestId, const QList<BluOS::Preset> &presets)
{
    BluOS *bluos = static_cast<BluOS*>(sender());
    Thing *thing = myThings().findById(m_bluos.key(bluos));

    if (m_asyncBrowseResults.contains(requestId)) {
        BrowseResult *result = m_asyncBrowseResults.take(requestId);
        if (!thing) {
            qCWarning(dcBluOS()) << "Could not find any Thing that belongs to this browse result";
            result->finish(Thing::ThingErrorHardwareNotAvailable);
            return;
        }
        foreach(BluOS::Preset preset, presets) {
            qCDebug(dcBluOS()) << "Preset added" << preset.Name << preset.Id << preset.Url;
            BrowserItem item("presets&"+QString::number(preset.Id), preset.Name, false, true);
            item.setIcon(BrowserItem::BrowserIcon::BrowserIconFavorites);
            result->addItem(item);
        }
        result->finish(Thing::ThingErrorNoError);
    }
    if (m_asyncBrowseItemResults.contains(requestId)) {
        BrowserItemResult *result = m_asyncBrowseItemResults.take(requestId);
        result->finish(Thing::ThingErrorItemNotFound);
        //For future browsing features
    }
}

void IntegrationPluginBluOS::onSourcesReceived(QUuid requestId, const QList<BluOS::Source> &sources)
{
    BluOS *bluos = static_cast<BluOS*>(sender());
    Thing *thing = myThings().findById(m_bluos.key(bluos));

    if (m_asyncBrowseResults.contains(requestId)) {
        BrowseResult *result = m_asyncBrowseResults.take(requestId);
        if (!thing) {
            qCWarning(dcBluOS()) << "Could not find any Thing that belongs to this browse result";
            result->finish(Thing::ThingErrorHardwareNotAvailable);
            return;
        }
        foreach(BluOS::Source source, sources) {
            qCDebug(dcBluOS()) << "Source added" << source.Text << source.BrowseKey << source.Type;
            MediaBrowserItem item;
            item.setDisplayName(source.Text);
            if (source.BrowseKey.isEmpty()) {
                item.setBrowsable(false);
                item.setExecutable(true);
                item.setId(source.Text);
            } else {
                item.setBrowsable(true);
                item.setExecutable(false);
                item.setId(source.BrowseKey);
            }
            item.setIcon(BrowserItem::BrowserIconMusic);
            if (source.Text == "Bluetooth") {
                item.setMediaIcon(MediaBrowserItem::MediaBrowserIconBluetooth);
                //result->addItem(item);
            } else if (source.Text == "Spotify") {
                item.setExecutable(false);
                item.setBrowsable(false);
                item.setMediaIcon(MediaBrowserItem::MediaBrowserIconSpotify);
                item.setDescription("Open the Spotify App for browsing");
                result->addItem(item);
            } else if (source.Text == "TuneIn") {
                item.setMediaIcon(MediaBrowserItem::MediaBrowserIconTuneIn);
                result->addItem(item);
            } else if (source.Text.contains("Aux")) {
                item.setMediaIcon(MediaBrowserItem::MediaBrowserIconAux);
                result->addItem(item);
            } else if (source.Text == "Radio Paradise") {
                //item.setMediaIcon(MediaBrowserItem::MediaBrowserIconRadioParadise);
                //result->addItem(item);
                //Needs testing before continuing
            }
        }
        result->finish(Thing::ThingErrorNoError);
    }

    if (m_asyncBrowseItemResults.contains(requestId)) {
        BrowserItemResult *result = m_asyncBrowseItemResults.take(requestId);
        result->finish(Thing::ThingErrorItemNotFound);
        //For future browsing features
    }
}

void IntegrationPluginBluOS::onBrowseResultReceived(QUuid requestId, const QList<BluOS::Source> &sources)
{
    BluOS *bluos = static_cast<BluOS*>(sender());
    Thing *thing = myThings().findById(m_bluos.key(bluos));

    if (m_asyncBrowseResults.contains(requestId)) {
        BrowseResult *result = m_asyncBrowseResults.take(requestId);

        if (!thing) {
            qCWarning(dcBluOS()) << "Could not find any Thing that belongs to this browse result";
            result->finish(Thing::ThingErrorHardwareNotAvailable);
            return;
        }
        foreach(BluOS::Source source, sources) {
            qCDebug(dcBluOS()) << "Source added" << source.Text << source.BrowseKey << source.Type;
            MediaBrowserItem item;
            item.setDisplayName(source.Text);
            if (source.BrowseKey.isEmpty()) {
                item.setBrowsable(false);
                item.setExecutable(true);
                item.setId(source.Text);
            } else {
                item.setBrowsable(true);
                item.setExecutable(false);
                item.setId(source.BrowseKey);
            }
            item.setIcon(BrowserItem::BrowserIconMusic);
            result->addItem(item);
        }
        result->finish(Thing::ThingErrorNoError);
    }
}
