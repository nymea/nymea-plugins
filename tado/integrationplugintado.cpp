// SPDX-License-Identifier: GPL-3.0-or-later

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*
* Copyright (C) 2013 - 2024, nymea GmbH
* Copyright (C) 2024 - 2025, chargebyte austria GmbH
*
* This file is part of nymea-plugins.
*
* nymea-plugins is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* (at your option) any later version.
*
* nymea-plugins is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
* General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with nymea-plugins. If not, see <https://www.gnu.org/licenses/>.
*
* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

#include "integrationplugintado.h"
#include "plugininfo.h"

#include <integrations/thing.h>
#include <network/networkaccessmanager.h>

#include <QDebug>
#include <QJsonDocument>
#include <QTimer>
#include <QUrlQuery>
#include <QtMath>

namespace {
void finishPendingActions(const QList<ThingActionInfo *> &actions, Thing::ThingError error)
{
    for (ThingActionInfo *info : actions) {
        if (info) {
            info->finish(error);
        }
    }
}
} // namespace

IntegrationPluginTado::IntegrationPluginTado()
{
    m_stateSyncTimer.setInterval(5000);
    connect(&m_stateSyncTimer, &QTimer::timeout, this, &IntegrationPluginTado::syncPendingOverlays);
}

QString IntegrationPluginTado::buildZoneKey(const ThingId &accountThingId, const QString &homeId, const QString &zoneId)
{
    return accountThingId.toString() + ":" + homeId + ":" + zoneId;
}

bool IntegrationPluginTado::overlayStatesEqual(const OverlayState &first, const OverlayState &second)
{
    if (first.deleteOverlay != second.deleteOverlay) {
        return false;
    }
    if (first.deleteOverlay) {
        return true;
    }
    if (first.power != second.power) {
        return false;
    }
    return qAbs(first.temperature - second.temperature) < 0.01;
}

void IntegrationPluginTado::queueOverlayChange(ThingActionInfo *info, const QString &homeId, const QString &zoneId, const OverlayState &desired)
{
    if (!info) {
        return;
    }

    Thing *thing = info->thing();
    if (!thing) {
        return;
    }

    ThingId accountThingId = thing->parentId();
    QString zoneKey = buildZoneKey(accountThingId, homeId, zoneId);
    PendingOverlayChange &pending = m_pendingOverlayChanges[zoneKey];
    pending.accountThingId = accountThingId;
    pending.homeId = homeId;
    pending.zoneId = zoneId;
    pending.desired = desired;
    pending.dirty = true;
    pending.pendingActions.append(info);

    connect(info, &ThingActionInfo::aborted, this, [this, info]() { removePendingAction(info); });

    if (!m_stateSyncTimer.isActive()) {
        m_stateSyncTimer.start();
    }
}

void IntegrationPluginTado::removePendingAction(ThingActionInfo *info)
{
    for (auto it = m_pendingOverlayChanges.begin(); it != m_pendingOverlayChanges.end(); ++it) {
        it->pendingActions.removeAll(info);
    }
    for (auto it = m_pendingRequests.begin(); it != m_pendingRequests.end(); ++it) {
        it->actions.removeAll(info);
    }
}

void IntegrationPluginTado::init() {}

void IntegrationPluginTado::startPairing(ThingPairingInfo *info)
{
    qCDebug(dcTado()) << "Start pairing process ...";

    Tado *tado = new Tado(hardwareManager()->networkManager(), this);
    m_unfinishedTadoAccounts.insert(info->thingId(), tado);

    connect(info, &ThingPairingInfo::aborted, this, [info, tado, this]() {
        qCWarning(dcTado()) << "Thing pairing has been aborted, cleaning up...";
        m_unfinishedTadoAccounts.remove(info->thingId());
        tado->deleteLater();
    });

    connect(tado, &Tado::getLoginUrlFinished, info, [info, tado, this](bool success) {
        if (!success) {
            info->finish(Thing::ThingErrorAuthenticationFailure);
            return;
        }

        connect(info, &ThingPairingInfo::aborted, tado, [info, this]() {
            qCWarning(dcTado()) << "ThingPairingInfo aborted, cleaning up pending setup connection.";
            m_unfinishedTadoAccounts.take(info->thingId())->deleteLater();
        });

        qCDebug(dcTado()) << "Tado server is reachable. Starting the OAuth pairing process using" << tado->loginUrl();
        info->setOAuthUrl(QUrl(tado->loginUrl()));
        info->finish(Thing::ThingErrorNoError);
    });

    tado->getLoginUrl();
}

void IntegrationPluginTado::confirmPairing(ThingPairingInfo *info, const QString &username, const QString &password)
{
    Q_UNUSED(username)

    qCDebug(dcTado()) << "Confirm pairing" << password;
    Tado *tado = m_unfinishedTadoAccounts.value(info->thingId());

    connect(tado, &Tado::connectionError, info, [info](QNetworkReply::NetworkError error) {
        if (error != QNetworkReply::NetworkError::NoError) {
            qCWarning(dcTado()) << "Confirm pairing failed" << error;
            info->finish(Thing::ThingErrorSetupFailed, QT_TR_NOOP("A connection error occurred."));
        }
    });

    connect(tado, &Tado::startAuthenticationFinished, info, [info, tado, this](bool success) {
        if (!success) {
            info->finish(Thing::ThingErrorAuthenticationFailure, QT_TR_NOOP("Failed to authenticate with tado server."));
            return;
        }

        qCDebug(dcTado()) << "Authentication finished successfully.";
        pluginStorage()->beginGroup(info->thingId().toString());
        pluginStorage()->setValue("refreshToken", tado->refreshToken());
        pluginStorage()->endGroup();

        info->finish(Thing::ThingErrorNoError);
    });

    tado->startAuthentication();
}

void IntegrationPluginTado::setupThing(ThingSetupInfo *info)
{
    Thing *thing = info->thing();

    if (thing->thingClassId() == tadoAccountThingClassId) {
        qCDebug(dcTado) << "Setting up Tado account" << thing->name() << thing->params();

        Tado *tado = nullptr;

        if (m_tadoAccounts.contains(thing->id())) {
            qCDebug(dcTado()) << "Setup after reconfigure, cleaning up";
            m_tadoAccounts.take(thing->id())->deleteLater();
        }

        if (m_unfinishedTadoAccounts.contains(thing->id())) {
            qCDebug(dcTado()) << "Using Tado connection from pairing process";
            tado = m_unfinishedTadoAccounts.take(thing->id());
            m_tadoAccounts.insert(thing->id(), tado);
            info->finish(Thing::ThingErrorNoError);

            pluginStorage()->beginGroup(thing->id().toString());
            pluginStorage()->setValue("refreshToken", tado->refreshToken());
            pluginStorage()->endGroup();
        } else {
            // Load refresh token
            pluginStorage()->beginGroup(thing->id().toString());
            QString refreshToken = pluginStorage()->value("refreshToken").toString();
            qCDebug(dcTado()) << "Loaded refresh token" << refreshToken;
            pluginStorage()->endGroup();

            if (refreshToken.isEmpty()) {
                info->finish(Thing::ThingErrorAuthenticationFailure, QT_TR_NOOP("Could not authenticate on the server. Please reconfigure the connection."));
                return;
            }

            tado = new Tado(hardwareManager()->networkManager(), this);
            m_tadoAccounts.insert(thing->id(), tado);
            tado->setRefreshToken(refreshToken);
        }

        // Delete any leftover username password from deprecated password grant flow,
        // storing passwords in plaintext does not correspond to good manners
        pluginStorage()->beginGroup(thing->id().toString());
        pluginStorage()->remove("username");
        pluginStorage()->remove("password");
        pluginStorage()->endGroup();

        connect(info, &ThingSetupInfo::aborted, this, [info, this] {
            if (m_tadoAccounts.contains(info->thing()->id())) {
                m_tadoAccounts.take(info->thing()->id())->deleteLater();
            }
        });

        connect(tado, &Tado::refreshTokenReceived, this, [thing, this](const QString &refreshToken) {
            pluginStorage()->beginGroup(thing->id().toString());
            pluginStorage()->setValue("refreshToken", refreshToken);
            pluginStorage()->endGroup();
        });

        connect(tado, &Tado::accessTokenReceived, info, [info]() {
            qCDebug(dcTado()) << "Token received, account setup successfull";
            info->finish(Thing::ThingErrorNoError);
        });

        connect(tado, &Tado::connectionError, info, [this, info](QNetworkReply::NetworkError error) {
            if (error != QNetworkReply::NetworkError::NoError) {
                if (m_tadoAccounts.contains(info->thing()->id())) {
                    Tado *tado = m_tadoAccounts.take(info->thing()->id());
                    tado->deleteLater();
                }

                qCWarning(dcTado()) << "Connection error during setup:" << error;
                info->finish(Thing::ThingErrorSetupFailed, QT_TR_NOOP("Connection error"));
            }
        });

        connect(tado, &Tado::usernameChanged, this, &IntegrationPluginTado::onUsernameChanged);
        connect(tado, &Tado::authenticationStatusChanged, this, &IntegrationPluginTado::onAuthenticationStatusChanged);
        connect(tado, &Tado::requestExecuted, this, &IntegrationPluginTado::onRequestExecuted);
        connect(tado, &Tado::connectionChanged, this, &IntegrationPluginTado::onConnectionChanged);
        connect(tado, &Tado::homesReceived, this, &IntegrationPluginTado::onHomesReceived);
        connect(tado, &Tado::zonesReceived, this, &IntegrationPluginTado::onZonesReceived);
        connect(tado, &Tado::zoneStateReceived, this, &IntegrationPluginTado::onZoneStateReceived);
        connect(tado, &Tado::overlayReceived, this, &IntegrationPluginTado::onOverlayReceived);

        tado->getAccessToken();

        return;

    } else if (thing->thingClassId() == zoneThingClassId) {
        qCDebug(dcTado) << "Setup Tado zone" << thing->params();
        Thing *parentThing = myThings().findById(thing->parentId());
        if (parentThing->setupComplete()) {
            return info->finish(Thing::ThingErrorNoError);
        } else {
            connect(parentThing, &Thing::setupStatusChanged, info, [parentThing, info] {
                if (parentThing->setupComplete()) {
                    info->finish(Thing::ThingErrorNoError);
                }
            });
        }

    } else {
        qCWarning(dcTado()) << "Unhandled thing class in setupDevice";
        return info->finish(Thing::ThingErrorThingClassNotFound);
    }
}

void IntegrationPluginTado::thingRemoved(Thing *thing)
{
    if (thing->thingClassId() == tadoAccountThingClassId) {
        Tado *tado = m_tadoAccounts.take(thing->id());
        if (tado) {
            tado->deleteLater();
        }

        for (auto it = m_pendingOverlayChanges.begin(); it != m_pendingOverlayChanges.end();) {
            if (it->accountThingId == thing->id()) {
                finishPendingActions(it->pendingActions, Thing::ThingErrorThingNotFound);
                it = m_pendingOverlayChanges.erase(it);
                continue;
            }
            ++it;
        }

        QString accountPrefix = thing->id().toString() + ":";
        for (auto it = m_pendingRequests.begin(); it != m_pendingRequests.end();) {
            if (it->zoneKey.startsWith(accountPrefix)) {
                finishPendingActions(it->actions, Thing::ThingErrorThingNotFound);
                it = m_pendingRequests.erase(it);
                continue;
            }
            ++it;
        }
    } else if (thing->thingClassId() == zoneThingClassId) {
        QString homeId = thing->paramValue(zoneThingHomeIdParamTypeId).toString();
        QString zoneId = thing->paramValue(zoneThingZoneIdParamTypeId).toString();
        QString zoneKey = buildZoneKey(thing->parentId(), homeId, zoneId);
        if (m_pendingOverlayChanges.contains(zoneKey)) {
            PendingOverlayChange pending = m_pendingOverlayChanges.take(zoneKey);
            finishPendingActions(pending.pendingActions, Thing::ThingErrorThingNotFound);
        }
        for (auto it = m_pendingRequests.begin(); it != m_pendingRequests.end();) {
            if (it->zoneKey == zoneKey) {
                finishPendingActions(it->actions, Thing::ThingErrorThingNotFound);
                it = m_pendingRequests.erase(it);
                continue;
            }
            ++it;
        }
    }

    // Clean up storage
    pluginStorage()->remove(thing->id().toString());

    if (myThings().isEmpty()) {
        if (m_pluginTimer) {
            m_pluginTimer->deleteLater();
            m_pluginTimer = nullptr;
        }
        if (m_stateSyncTimer.isActive()) {
            m_stateSyncTimer.stop();
        }
    }
}

void IntegrationPluginTado::postSetupThing(Thing *thing)
{
    if (!m_pluginTimer) {
        m_pluginTimer = hardwareManager()->pluginTimerManager()->registerTimer(10);
        connect(m_pluginTimer, &PluginTimer::timeout, this, &IntegrationPluginTado::onPluginTimer);
    }
    if (!m_stateSyncTimer.isActive()) {
        m_stateSyncTimer.start();
    }

    if (thing->thingClassId() == tadoAccountThingClassId) {
        Tado *tado = m_tadoAccounts.value(thing->id());
        //thing->setStateValue(tadoAccountUserDisplayNameStateTypeId, tado->username());
        thing->setStateValue(tadoAccountLoggedInStateTypeId, true);
        thing->setStateValue(tadoAccountConnectedStateTypeId, true);
        tado->getHomes();

    } else if (thing->thingClassId() == zoneThingClassId) {
        if (m_tadoAccounts.contains(thing->parentId())) {
            Tado *tado = m_tadoAccounts.value(thing->parentId());
            tado->getZoneState(thing->paramValue(zoneThingHomeIdParamTypeId).toString(), thing->paramValue(zoneThingZoneIdParamTypeId).toString());
        }
    }
}

void IntegrationPluginTado::executeAction(ThingActionInfo *info)
{
    Thing *thing = info->thing();
    Action action = info->action();

    if (thing->thingClassId() == zoneThingClassId) {
        if (!m_tadoAccounts.contains(thing->parentId())) {
            info->finish(Thing::ThingErrorThingNotFound);
            return;
        }
        QString homeId = thing->paramValue(zoneThingHomeIdParamTypeId).toString();
        QString zoneId = thing->paramValue(zoneThingZoneIdParamTypeId).toString();
        if (action.actionTypeId() == zoneModeActionTypeId) {
            OverlayState desired;
            QString mode = action.param(zoneModeActionModeParamTypeId).value().toString();
            if (mode == "Tado") {
                desired.deleteOverlay = true;
            } else if (mode == "Off") {
                desired.power = false;
                desired.temperature = thing->stateValue(zoneTargetTemperatureStateTypeId).toDouble();
            } else {
                desired.power = true;
                double targetTemperature = thing->stateValue(zoneTargetTemperatureStateTypeId).toDouble();
                desired.temperature = targetTemperature <= 5.0 ? 5.0 : targetTemperature;
            }
            queueOverlayChange(info, homeId, zoneId, desired);
        } else if (action.actionTypeId() == zoneTargetTemperatureActionTypeId) {
            double temperature = action.param(zoneTargetTemperatureActionTargetTemperatureParamTypeId).value().toDouble();
            OverlayState desired;
            if (temperature <= 0) {
                desired.power = false;
                desired.temperature = 0;
            } else {
                desired.power = true;
                desired.temperature = temperature;
            }
            queueOverlayChange(info, homeId, zoneId, desired);
        } else if (action.actionTypeId() == zonePowerActionTypeId) {
            bool power = action.param(zonePowerActionPowerParamTypeId).value().toBool();
            thing->setStateValue(zonePowerStateTypeId, power); // the actual power set response might be slow
            OverlayState desired;
            double temperature = thing->stateValue(zoneTargetTemperatureStateTypeId).toDouble();
            desired.power = power;
            desired.temperature = power ? temperature : 0;
            queueOverlayChange(info, homeId, zoneId, desired);
        } else {
            qCWarning(dcTado()) << "Execute action, unhandled actionTypeId" << action.actionTypeId();
            info->finish(Thing::ThingErrorActionTypeNotFound);
        }
    } else {
        qCWarning(dcTado()) << "Execute action, unhandled thingClassId" << thing->thingClassId();
        info->finish(Thing::ThingErrorThingClassNotFound);
    }
}

void IntegrationPluginTado::syncPendingOverlays()
{
    if (m_pendingOverlayChanges.isEmpty()) {
        return;
    }

    for (auto it = m_pendingOverlayChanges.begin(); it != m_pendingOverlayChanges.end(); ++it) {
        PendingOverlayChange &pending = it.value();
        if (!pending.dirty) {
            if (!pending.pendingActions.isEmpty()) {
                finishPendingActions(pending.pendingActions, Thing::ThingErrorNoError);
                pending.pendingActions.clear();
            }
            continue;
        }

        if (pending.inFlightValid) {
            continue;
        }

        if (pending.lastSyncedValid && overlayStatesEqual(pending.desired, pending.lastSynced)) {
            pending.dirty = false;
            if (!pending.pendingActions.isEmpty()) {
                finishPendingActions(pending.pendingActions, Thing::ThingErrorNoError);
                pending.pendingActions.clear();
            }
            continue;
        }

        Tado *tado = m_tadoAccounts.value(pending.accountThingId);
        if (!tado) {
            if (!pending.pendingActions.isEmpty()) {
                finishPendingActions(pending.pendingActions, Thing::ThingErrorThingNotFound);
                pending.pendingActions.clear();
            }
            pending.dirty = false;
            continue;
        }

        QUuid requestId;
        if (pending.desired.deleteOverlay) {
            requestId = tado->deleteOverlay(pending.homeId, pending.zoneId);
        } else {
            requestId = tado->setOverlay(pending.homeId, pending.zoneId, pending.desired.power, pending.desired.temperature);
        }

        if (requestId.isNull()) {
            continue;
        }

        PendingRequest request;
        request.zoneKey = it.key();
        request.actions = pending.pendingActions;
        request.sentState = pending.desired;
        pending.pendingActions.clear();
        pending.inFlightValid = true;
        m_pendingRequests.insert(requestId, request);
    }
}

void IntegrationPluginTado::onPluginTimer()
{
    Q_FOREACH (Tado *tado, m_tadoAccounts) {
        ThingId accountThingId = m_tadoAccounts.key(tado);
        if (!tado->authenticated()) {
            tado->getAccessToken();
        } else {
            foreach (Thing *thing, myThings().filterByParentId(accountThingId)) {
                if (thing->thingClassId() == zoneThingClassId) {
                    QString homeId = thing->paramValue(zoneThingHomeIdParamTypeId).toString();
                    QString zoneId = thing->paramValue(zoneThingZoneIdParamTypeId).toString();
                    tado->getZoneState(homeId, zoneId);
                }
            }
        }
    }
}

void IntegrationPluginTado::onConnectionChanged(bool connected)
{
    Tado *tado = static_cast<Tado *>(sender());

    if (m_tadoAccounts.values().contains(tado)) {
        Thing *thing = myThings().findById(m_tadoAccounts.key(tado));
        if (!thing)
            return;
        thing->setStateValue(tadoAccountConnectedStateTypeId, connected);

        if (!connected) {
            foreach (Thing *child, myThings().filterByParentId(thing->id())) {
                if (child->thingClassId() == zoneThingClassId) {
                    child->setStateValue(zoneConnectedStateTypeId, connected);
                }
            }
        }
    }
}

void IntegrationPluginTado::onAuthenticationStatusChanged(bool authenticated)
{
    Tado *tado = static_cast<Tado *>(sender());

    if (m_tadoAccounts.values().contains(tado)) {
        Thing *thing = myThings().findById(m_tadoAccounts.key(tado));
        if (!thing) {
            qCWarning(dcTado()) << "OnAuthenticationChanged no thing found by ID" << m_tadoAccounts.key(tado);
            return;
        }
        thing->setStateValue(tadoAccountLoggedInStateTypeId, authenticated);
        if (!authenticated) {
            foreach (Thing *child, myThings().filterByParentId(thing->id())) {
                if (child->thingClassId() == zoneThingClassId) {
                    child->setStateValue(zoneConnectedStateTypeId, authenticated);
                }
            }
        }
    }
}

void IntegrationPluginTado::onUsernameChanged(const QString &username)
{
    Tado *tado = static_cast<Tado *>(sender());

    if (m_tadoAccounts.values().contains(tado)) {
        Thing *thing = myThings().findById(m_tadoAccounts.key(tado));
        thing->setStateValue(tadoAccountUserDisplayNameStateTypeId, username);
    }
}

void IntegrationPluginTado::onRequestExecuted(QUuid requestId, bool success)
{
    if (!m_pendingRequests.contains(requestId)) {
        return;
    }

    PendingRequest request = m_pendingRequests.take(requestId);
    finishPendingActions(request.actions, success ? Thing::ThingErrorNoError : Thing::ThingErrorHardwareNotAvailable);

    if (!m_pendingOverlayChanges.contains(request.zoneKey)) {
        return;
    }

    PendingOverlayChange &pending = m_pendingOverlayChanges[request.zoneKey];
    pending.inFlightValid = false;
    if (success) {
        pending.lastSynced = request.sentState;
        pending.lastSyncedValid = true;
    }

    if (pending.lastSyncedValid && overlayStatesEqual(pending.desired, pending.lastSynced)) {
        pending.dirty = false;
        if (!pending.pendingActions.isEmpty()) {
            finishPendingActions(pending.pendingActions, Thing::ThingErrorNoError);
            pending.pendingActions.clear();
        }
    } else {
        pending.dirty = true;
    }
}

void IntegrationPluginTado::onHomesReceived(QList<Tado::Home> homes)
{
    qCDebug(dcTado()) << "Homes received";
    Tado *tado = static_cast<Tado *>(sender());
    foreach (Tado::Home home, homes) {
        tado->getZones(home.id);
    }
}

void IntegrationPluginTado::onZonesReceived(const QString &homeId, QList<Tado::Zone> zones)
{
    Tado *tado = static_cast<Tado *>(sender());

    if (m_tadoAccounts.values().contains(tado)) {
        Thing *parentDevice = myThings().findById(m_tadoAccounts.key(tado));
        qCDebug(dcTado()) << "Zones received:" << zones.count() << parentDevice->name();

        ThingDescriptors descriptors;
        foreach (Tado::Zone zone, zones) {
            ThingDescriptor descriptor(zoneThingClassId, zone.name, "Type:" + zone.type, parentDevice->id());
            ParamList params;
            params.append(Param(zoneThingHomeIdParamTypeId, homeId));
            params.append(Param(zoneThingZoneIdParamTypeId, zone.id));
            if (myThings().findByParams(params))
                continue;

            params.append(Param(zoneThingTypeParamTypeId, zone.type));
            descriptor.setParams(params);
            descriptors.append(descriptor);
        }
        emit autoThingsAppeared(descriptors);
    } else {
        qCWarning(dcTado()) << "Tado connection not linked to a thing Id" << m_tadoAccounts.size() << m_tadoAccounts.key(tado).toString();
    }
}

void IntegrationPluginTado::onZoneStateReceived(const QString &homeId, const QString &zoneId, Tado::ZoneState state)
{
    Tado *tado = static_cast<Tado *>(sender());
    ThingId parentId = m_tadoAccounts.key(tado);
    ParamList params;
    params.append(Param(zoneThingHomeIdParamTypeId, homeId));
    params.append(Param(zoneThingZoneIdParamTypeId, zoneId));
    Thing *thing = myThings().filterByParentId(parentId).findByParams(params);
    if (!thing)
        return;

    if (state.overlayIsSet) {
        if (state.overlaySettingPower) {
            thing->setStateValue(zoneModeStateTypeId, "Manual");
        } else {
            thing->setStateValue(zoneModeStateTypeId, "Off");
        }
    } else {
        thing->setStateValue(zoneModeStateTypeId, "Tado");
    }

    thing->setStateValue(zonePowerStateTypeId, (state.heatingPowerPercentage > 0));
    thing->setStateValue(zoneConnectedStateTypeId, state.connected);
    thing->setStateValue(zoneTargetTemperatureStateTypeId, qMax(5.0, state.settingTemperature));
    thing->setStateValue(zoneTemperatureStateTypeId, state.temperature);
    thing->setStateValue(zoneHumidityStateTypeId, state.humidity);
    thing->setStateValue(zoneWindowOpenDetectedStateTypeId, state.windowOpenDetected);
    thing->setStateValue(zoneTadoModeStateTypeId, state.tadoMode);
}

void IntegrationPluginTado::onOverlayReceived(const QString &homeId, const QString &zoneId, const Tado::Overlay &overlay)
{
    Tado *tado = static_cast<Tado *>(sender());
    ThingId parentId = m_tadoAccounts.key(tado);
    ParamList params;
    params.append(Param(zoneThingHomeIdParamTypeId, homeId));
    params.append(Param(zoneThingZoneIdParamTypeId, zoneId));
    Thing *thing = myThings().filterByParentId(parentId).findByParams(params);
    if (!thing)
        return;
    thing->setStateValue(zoneTargetTemperatureStateTypeId, overlay.temperature);

    if (overlay.tadoMode == "MANUAL") {
        if (overlay.power) {
            thing->setStateValue(zoneModeStateTypeId, "Manual");
        } else {
            thing->setStateValue(zoneModeStateTypeId, "Off");
        }
    } else {
        thing->setStateValue(zoneModeStateTypeId, "Tado");
    }
}
