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

#include "integrationpluginstreamunlimited.h"
#include "plugininfo.h"
#include "streamunlimiteddevice.h"

#include <platform/platformzeroconfcontroller.h>
#include <network/zeroconf/zeroconfservicebrowser.h>
#include <network/networkaccessmanager.h>
#include <types/browseritem.h>

#include <QDebug>

IntegrationPluginStreamUnlimited::IntegrationPluginStreamUnlimited()
{
}

void IntegrationPluginStreamUnlimited::init()
{
    m_zeroConfBrowser = hardwareManager()->zeroConfController()->createServiceBrowser("_sues800device._tcp");
    connect(m_zeroConfBrowser, &ZeroConfServiceBrowser::serviceEntryAdded, this, [=](const ZeroConfServiceEntry &entry){
        foreach (Thing *thing, m_devices.keys()) {
            QString thingId = thing->paramValue(streamSDKdevBoardThingIdParamTypeId).toString();
            if (entry.protocol() == QAbstractSocket::IPv4Protocol) {

                foreach (const QString &txtEntry, entry.txt()) {
                    QStringList parts = txtEntry.split('=');
                    if (parts.length() == 2 && parts.first() == "uuid" && parts.last() == thingId) {
                        StreamUnlimitedDevice *device = m_devices.value(thing);
                        if (device->connectionStatus() != StreamUnlimitedDevice::ConnectionStatusConnected) {
                            device->setHost(entry.hostAddress(), entry.port());
                        }
                    }
                }
            }
        }
    });
}

void IntegrationPluginStreamUnlimited::discoverThings(ThingDiscoveryInfo *info)
{
    qCDebug(dcStreamUnlimited()) << "Discovery";
    foreach (const ZeroConfServiceEntry &entry, m_zeroConfBrowser->serviceEntries()) {
        // Disabling IPv6 for now, it seems to be not really reliable with those devices
        if (entry.serviceType() != "_sues800device._tcp" || entry.protocol() != QAbstractSocket::IPv4Protocol) {
            continue;
        }
        qCDebug(dcStreamUnlimited) << "Zeroconf entry:" << entry;
        QString id;
        QString name;
        foreach (const QString &txt, entry.txt()) {
            QString field = txt.split("=").first();
            QString value = txt.split("=").last();
            if (field == "uuid") {
                id = value;
            } else if (field == "name") {
                name = value;
            }
        }
        QHostAddress address = entry.hostAddress();

        ParamList params;
        params << Param(streamSDKdevBoardThingIdParamTypeId, id);

        ThingDescriptor descriptor(streamSDKdevBoardThingClassId, name, address.toString());
        descriptor.setParams(params);

        foreach (Thing *thing, myThings()) {
            if (thing->paramValue(streamSDKdevBoardThingIdParamTypeId).toString() == id) {
                descriptor.setThingId(thing->id());
                break;
            }
        }

        info->addThingDescriptor(descriptor);
    }

    info->finish(Thing::ThingErrorNoError);
}

void IntegrationPluginStreamUnlimited::setupThing(ThingSetupInfo *info)
{
    Thing *thing = info->thing();

    QString id = thing->paramValue(streamSDKdevBoardThingIdParamTypeId).toString();

    StreamUnlimitedDevice *device = new StreamUnlimitedDevice(hardwareManager()->networkManager(), this);
    m_devices.insert(thing, device);

    ZeroConfServiceEntry entry;
    foreach (const ZeroConfServiceEntry &e, m_zeroConfBrowser->serviceEntries()) {
        if (entry.serviceType() != "_sues800device._tcp" && entry.protocol() == QAbstractSocket::IPv4Protocol) {
            foreach (const QString &txtEntry, entry.txt()) {
                QStringList parts = txtEntry.split('=');
                if (parts.length() == 2 && parts.first() == "uuid" && parts.last() == id) {
                    entry = e;
                }
            }
        }
    }
    if (entry.isValid()) {
        device->setHost(entry.hostAddress(), entry.port());
    } else if (pluginStorage()->childGroups().contains(id)) {
        pluginStorage()->beginGroup(id);
        QHostAddress address(pluginStorage()->value("address").toString());
        int port = pluginStorage()->value("port").toInt();
        pluginStorage()->endGroup();
        device->setHost(address, port);
    }

    connect(device, &StreamUnlimitedDevice::connectionStatusChanged, thing, [=](StreamUnlimitedDevice::ConnectionStatus status){
        thing->setStateValue(streamSDKdevBoardConnectedStateTypeId, status == StreamUnlimitedDevice::ConnectionStatusConnected);
        // Cache address information on successful connect
        if (status == StreamUnlimitedDevice::ConnectionStatusConnected) {
            pluginStorage()->beginGroup(id);
            pluginStorage()->setValue("address", device->address().toString());
            pluginStorage()->setValue("port", device->port());
            pluginStorage()->endGroup();
        }
    });
    connect(device, &StreamUnlimitedDevice::playbackStatusChanged, thing, [thing](StreamUnlimitedDevice::PlayStatus state){
        QHash<StreamUnlimitedDevice::PlayStatus, QString> stateMap;
        stateMap.insert(StreamUnlimitedDevice::PlayStatusStopped, "Stopped");
        stateMap.insert(StreamUnlimitedDevice::PlayStatusPaused, "Paused");
        stateMap.insert(StreamUnlimitedDevice::PlayStatusPlaying, "Playing");
        thing->setStateValue(streamSDKdevBoardPlaybackStatusStateTypeId, stateMap.value(state));
    });
    connect(device, &StreamUnlimitedDevice::volumeChanged, thing, [thing](uint volume){
        thing->setStateValue(streamSDKdevBoardVolumeStateTypeId, volume);
    });
    connect(device, &StreamUnlimitedDevice::muteChanged, thing, [thing](bool mute){
        thing->setStateValue(streamSDKdevBoardMuteStateTypeId, mute);
    });
    connect(device, &StreamUnlimitedDevice::titleChanged, thing, [thing](const QString &title){
        thing->setStateValue(streamSDKdevBoardTitleStateTypeId, title);
    });
    connect(device, &StreamUnlimitedDevice::artistChanged, thing, [thing](const QString &artist){
        thing->setStateValue(streamSDKdevBoardArtistStateTypeId, artist);
    });
    connect(device, &StreamUnlimitedDevice::albumChanged, thing, [thing](const QString &album){
        thing->setStateValue(streamSDKdevBoardCollectionStateTypeId, album);
    });
    connect(device, &StreamUnlimitedDevice::artworkChanged, thing, [thing](const QString &artwork){
        thing->setStateValue(streamSDKdevBoardArtworkStateTypeId, artwork);
    });
    connect(device, &StreamUnlimitedDevice::shuffleChanged, thing, [thing](bool shuffle){
        thing->setStateValue(streamSDKdevBoardShuffleStateTypeId, shuffle);
    });
    connect(device, &StreamUnlimitedDevice::repeatChanged, thing, [thing](StreamUnlimitedDevice::Repeat repeat){
        QHash<StreamUnlimitedDevice::Repeat, QString> stateMap;
        stateMap.insert(StreamUnlimitedDevice::RepeatNone, "None");
        stateMap.insert(StreamUnlimitedDevice::RepeatOne, "One");
        stateMap.insert(StreamUnlimitedDevice::RepeatAll, "All");
        thing->setStateValue(streamSDKdevBoardRepeatStateTypeId, stateMap.value(repeat));
    });

    info->finish(Thing::ThingErrorNoError);
}

void IntegrationPluginStreamUnlimited::executeAction(ThingActionInfo *info)
{
    StreamUnlimitedDevice *device = m_devices.value(info->thing());
    int commandId = -1;

    if (info->action().actionTypeId() == streamSDKdevBoardVolumeActionTypeId) {
        commandId = device->setVolume(info->action().param(streamSDKdevBoardVolumeActionVolumeParamTypeId).value().toUInt());
    }
    if (info->action().actionTypeId() == streamSDKdevBoardMuteActionTypeId) {
        commandId = device->setMute(info->action().param(streamSDKdevBoardMuteActionMuteParamTypeId).value().toBool());
    }
    if (info->action().actionTypeId() == streamSDKdevBoardRepeatActionTypeId) {
        QString repeatString = info->action().param(streamSDKdevBoardRepeatActionRepeatParamTypeId).value().toString();
        qCDebug(dcStreamUnlimited()) << "Repeat action:" << repeatString;
        QHash<StreamUnlimitedDevice::Repeat, QString> stateMap;
        stateMap.insert(StreamUnlimitedDevice::RepeatNone, "None");
        stateMap.insert(StreamUnlimitedDevice::RepeatOne, "One");
        stateMap.insert(StreamUnlimitedDevice::RepeatAll, "All");
        commandId = device->setRepeat(stateMap.key(repeatString));
    }
    if (info->action().actionTypeId() == streamSDKdevBoardShuffleActionTypeId) {
        commandId = device->setShuffle(info->action().param(streamSDKdevBoardShuffleActionShuffleParamTypeId).value().toBool());
    }

    if (info->action().actionTypeId() == streamSDKdevBoardPlayActionTypeId) {
        commandId = device->play();
    }
    if (info->action().actionTypeId() == streamSDKdevBoardPauseActionTypeId) {
        commandId = device->pause();
    }
    if (info->action().actionTypeId() == streamSDKdevBoardStopActionTypeId) {
        commandId = device->stop();
    }
    if (info->action().actionTypeId() == streamSDKdevBoardSkipBackActionTypeId) {
        commandId = device->skipBack();
    }
    if (info->action().actionTypeId() == streamSDKdevBoardSkipNextActionTypeId) {
        commandId = device->skipNext();
    }
    if (info->action().actionTypeId() == streamSDKdevBoardStorePresetActionTypeId) {
        uint presetId = info->action().param(streamSDKdevBoardStorePresetActionIndexParamTypeId).value().toUInt();
        commandId = device->storePreset(presetId);
    }
    if (info->action().actionTypeId() == streamSDKdevBoardLoadPresetActionTypeId) {
        uint presetId = info->action().param(streamSDKdevBoardLoadPresetActionIndexParamTypeId).value().toUInt();
        commandId = device->loadPreset(presetId);
    }

    connect(device, &StreamUnlimitedDevice::commandCompleted, info, [=](int replyCommandId, bool success){
        if (replyCommandId == commandId) {
            info->finish(success ? Thing::ThingErrorNoError : Thing::ThingErrorHardwareFailure);
        }
    });
}

void IntegrationPluginStreamUnlimited::thingRemoved(Thing *thing)
{
    m_devices.take(thing)->deleteLater();
}

void IntegrationPluginStreamUnlimited::browseThing(BrowseResult *result)
{
    StreamUnlimitedDevice *device = m_devices.value(result->thing());

    if (device->language() != result->locale()) {
        qCDebug(dcStreamUnlimited()) << "Setting language on device:" << result->locale();

        int commandId = device->setLocaleOnBoard(result->locale());
        connect(device, &StreamUnlimitedDevice::commandCompleted, result, [=](int replyId){
            if (replyId != commandId) {
                return;
            }

            // We'll call browsing in any case, regardless if setLocale succeeded or not
            browseThingInternal(result);
        });

    } else {
        browseThingInternal(result);
    }
}

void IntegrationPluginStreamUnlimited::executeBrowserItem(BrowserActionInfo *info)
{
    StreamUnlimitedDevice *device = m_devices.value(info->thing());
    int commandId = device->playBrowserItem(info->browserAction().itemId());
    connect(device, &StreamUnlimitedDevice::commandCompleted, info, [=](int replyId, bool success){
        if (replyId != commandId) {
            return;
        }
        if (!success) {
            info->finish(Thing::ThingErrorHardwareFailure);
            return;
        }

        info->finish(Thing::ThingErrorNoError);
    });
}

void IntegrationPluginStreamUnlimited::browserItem(BrowserItemResult *result)
{
    StreamUnlimitedDevice *device = m_devices.value(result->thing());

    if (device->language() != result->locale()) {
        qCDebug(dcStreamUnlimited()) << "Setting locale on board:" << result->locale();

        int commandId = device->setLocaleOnBoard(result->locale());
        connect(device, &StreamUnlimitedDevice::commandCompleted, result, [=](int replyId){
            if (replyId != commandId) {
                return;
            }

            // We'll call browsing in any case, regardless if setLocale succeeded or not
            browserItemInternal(result);
        });

    } else {
        browserItemInternal(result);
    }
}

void IntegrationPluginStreamUnlimited::browseThingInternal(BrowseResult *result)
{
    StreamUnlimitedDevice *device = m_devices.value(result->thing());

    int commandId = device->browseDevice(result->itemId());
    connect(device, &StreamUnlimitedDevice::browseResults, result, [=](int replyId, bool success, const BrowserItems &items){
        if (replyId != commandId) {
            return;
        }
        if (!success) {
            result->finish(Thing::ThingErrorHardwareFailure);
            return;
        }

        result->addItems(items);
        result->finish(Thing::ThingErrorNoError);
    });
}

void IntegrationPluginStreamUnlimited::browserItemInternal(BrowserItemResult *result)
{
    StreamUnlimitedDevice *device = m_devices.value(result->thing());

    int commandId = device->browserItem(result->itemId());
    connect(device, &StreamUnlimitedDevice::browserItemResult, result, [=](int replyId, bool success, const BrowserItem &item){
        if (replyId != commandId) {
            return;
        }
        if (!success) {
            result->finish(Thing::ThingErrorHardwareFailure);
            return;
        }

        result->finish(item);
    });
}
