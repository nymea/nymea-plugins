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


#include "integrationpluginbluos.h"
#include "plugininfo.h"
#include "integrations/thing.h"
#include "network/networkaccessmanager.h"


#include <QDebug>
#include <QStringList>
#include <QJsonDocument>
#include <QTimer>


IntegrationPluginBluOS::IntegrationPluginBluOS()
{

}

IntegrationPluginBluOS::~IntegrationPluginBluOS()
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

        m_asyncSetup.insert(bluos, info);
        bluos->getStatus();
        // In case the setup is cancelled before we finish it...
        connect(info, &QObject::destroyed, this, [this, bluos]() {
            m_asyncSetup.remove(bluos);

        });
        return;
    } else {
        return info->finish(Thing::ThingErrorThingClassNotFound);
    }
}

void IntegrationPluginBluOS::postSetupThing(Thing *thing)
{
    Q_UNUSED(thing);

    if (!m_pluginTimer) {
        //m_pluginTimer = hardwareManager()->pluginTimerManager()->registerTimer(2);
        //connect(m_pluginTimer, &PluginTimer::timeout, this, &IntegrationPluginBluOS::onPluginTimer);
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
            QString playbakStatus = action.param(bluosPlayerPlaybackStatusEventPlaybackStatusParamTypeId).value().toString();
            QUuid requestId;
            if (playbakStatus == "Playing") {
                requestId = bluos->play();
            } else if (playbakStatus == "Paused") {
                requestId = bluos->pause();
            } else if (playbakStatus == "Stopped") {
                requestId = bluos->stop();
            }
            m_asyncActions.insert(requestId, info);
            connect(info, &ThingActionInfo::aborted, [this, requestId] {m_asyncActions.remove(requestId);});
        } else if (action.actionTypeId() == bluosPlayerPlayActionTypeId) {
            QUuid requestId = bluos->play();
            m_asyncActions.insert(requestId, info);
            connect(info, &ThingActionInfo::aborted, [this, requestId] {m_asyncActions.remove(requestId);});
        } else if (action.actionTypeId() == bluosPlayerPauseActionTypeId) {
            QUuid requestId = bluos->pause();
            m_asyncActions.insert(requestId, info);
            connect(info, &ThingActionInfo::aborted, [this, requestId] {m_asyncActions.remove(requestId);});
        } else if (action.actionTypeId() == bluosPlayerStopActionTypeId) {
            QUuid requestId = bluos->stop();
            m_asyncActions.insert(requestId, info);
            connect(info, &ThingActionInfo::aborted, [this, requestId] {m_asyncActions.remove(requestId);});
        } else if (action.actionTypeId() == bluosPlayerSkipNextActionTypeId) {
            QUuid requestId = bluos->skip();
            m_asyncActions.insert(requestId, info);
            connect(info, &ThingActionInfo::aborted, [this, requestId] {m_asyncActions.remove(requestId);});
        } else if (action.actionTypeId() == bluosPlayerSkipBackActionTypeId) {
            QUuid requestId = bluos->back();
            m_asyncActions.insert(requestId, info);
            connect(info, &ThingActionInfo::aborted, [this, requestId] {m_asyncActions.remove(requestId);});
        } else if (action.actionTypeId() == bluosPlayerVolumeActionTypeId) {
            uint volume = action.param(bluosPlayerVolumeActionVolumeParamTypeId).value().toUInt();
            QUuid requestId = bluos->setVolume(volume);
            m_asyncActions.insert(requestId, info);
            connect(info, &ThingActionInfo::aborted, [this, requestId] {m_asyncActions.remove(requestId);});
        } else if (action.actionTypeId() == bluosPlayerMuteActionTypeId) {
            bool mute = action.param(bluosPlayerMuteActionMuteParamTypeId).value().toBool();
            QUuid requestId = bluos->setMute(mute);
            m_asyncActions.insert(requestId, info);
            connect(info, &ThingActionInfo::aborted, [this, requestId] {m_asyncActions.remove(requestId);});
        } else if (action.actionTypeId() == bluosPlayerShuffleActionTypeId) {
            bool shuffle = action.param(bluosPlayerShuffleActionShuffleParamTypeId).value().toBool();
            QUuid requestId = bluos->setMute(shuffle);
            m_asyncActions.insert(requestId, info);
            connect(info, &ThingActionInfo::aborted, [this, requestId] {m_asyncActions.remove(requestId);});
        }  else if (action.actionTypeId() == bluosPlayerRepeatActionTypeId) {
            QString repeat = action.param(bluosPlayerRepeatActionRepeatParamTypeId).value().toString();
            QUuid requestId;
            if (repeat == "One") {
                requestId = bluos->setRepeat(BluOS::RepeatMode::One);
            } else if (repeat == "All") {
                requestId = bluos->setRepeat(BluOS::RepeatMode::All);
            } else if (repeat == "None") {
                requestId = bluos->setRepeat(BluOS::RepeatMode::None);
            }
            m_asyncActions.insert(requestId, info);
            connect(info, &ThingActionInfo::aborted, [this, requestId] {m_asyncActions.remove(requestId);});
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
    Q_UNUSED(result)
}

void IntegrationPluginBluOS::browserItem(BrowserItemResult *result)
{
    Q_UNUSED(result)
}

void IntegrationPluginBluOS::executeBrowserItem(BrowserActionInfo *info)
{
    Q_UNUSED(info)
}

void IntegrationPluginBluOS::onConnectionChanged(bool connected)
{
    BluOS *bluos = static_cast<BluOS*>(sender());

    if (m_asyncSetup.contains(bluos)) {
        ThingSetupInfo *info = m_asyncSetup.take(bluos);
        if (connected) {
            m_bluos.insert(info->thing()->id(), bluos);
            info->finish(Thing::ThingErrorNoError);
        } else {
            info->finish(Thing::ThingErrorHardwareNotAvailable);
        }
    } else {
        Thing *thing = myThings().findById(m_bluos.key(bluos));
        if (!thing)
            return;
        thing->setStateValue(bluosPlayerConnectedStateTypeId, connected);
    }
}

void IntegrationPluginBluOS::onStatusResponseReceived(const BluOS::StatusResponse &status)
{
    BluOS *bluos = static_cast<BluOS*>(sender());
    Thing *thing = myThings().findById(m_bluos.key(bluos));
    if (!thing)
        return;
    thing->setStateValue(bluosPlayerArtistStateTypeId, status.Artist);
    thing->setStateValue(bluosPlayerCollectionStateTypeId, status.Album);
    thing->setStateValue(bluosPlayerTitleStateTypeId, status.Name);
    thing->setStateValue(bluosPlayerSourceStateTypeId, status.Service);
    thing->setStateValue(bluosPlayerArtworkStateTypeId, status.ServiceIcon);
    thing->setStateValue(bluosPlayerPlaybackStatusStateTypeId, status.PlaybackState);
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
}

void IntegrationPluginBluOS::onVolumeReceived(int volume, bool mute)
{
    BluOS *bluos = static_cast<BluOS*>(sender());
    Thing *thing = myThings().findById(m_bluos.key(bluos));
    if (!thing)
        return;
    thing->setStateValue(bluosPlayerMuteStateTypeId, mute);
    thing->setStateValue(bluosPlayerVolumeStateTypeId, volume);
}
