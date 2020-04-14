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

#include "integrationplugintado.h"
#include "integrations/thing.h"
#include "plugininfo.h"
#include "network/networkaccessmanager.h"

#include <QDebug>
#include <QUrlQuery>
#include <QJsonDocument>

IntegrationPluginTado::IntegrationPluginTado()
{

}

void IntegrationPluginTado::startPairing(ThingPairingInfo *info)
{
    info->finish(Thing::ThingErrorNoError, QT_TR_NOOP("Please enter the login credentials."));
}

void IntegrationPluginTado::confirmPairing(ThingPairingInfo *info, const QString &username, const QString &password)
{
    Tado *tado = new Tado(hardwareManager()->networkManager(), username, this);
    connect(tado, &Tado::tokenReceived, this, &IntegrationPluginTado::onTokenReceived);
    connect(tado, &Tado::authenticationStatusChanged, this, &IntegrationPluginTado::onAuthenticationStatusChanged);
    connect(tado, &Tado::connectionChanged, this, &IntegrationPluginTado::onConnectionChanged);
    connect(tado, &Tado::homesReceived, this, &IntegrationPluginTado::onHomesReceived);
    connect(tado, &Tado::zonesReceived, this, &IntegrationPluginTado::onZonesReceived);
    connect(tado, &Tado::zoneStateReceived, this, &IntegrationPluginTado::onZoneStateReceived);
    connect(tado, &Tado::overlayReceived, this, &IntegrationPluginTado::onOverlayReceived);
    m_unfinishedTadoAccounts.insert(info->thingId(), tado);
    m_unfinishedDevicePairings.insert(info->thingId(), info);
    tado->getToken(password);

    pluginStorage()->beginGroup(info->thingId().toString());
    pluginStorage()->setValue("username", username);
    pluginStorage()->setValue("password", password);
    pluginStorage()->endGroup();
}

void IntegrationPluginTado::setupThing(ThingSetupInfo *info)
{
    Thing *thing = info->thing();

    if (thing->thingClassId() == tadoConnectionThingClassId) {

        qCDebug(dcTado) << "Setup tado connection" << thing->name() << thing->params();
        pluginStorage()->beginGroup(thing->id().toString());
        QString username = pluginStorage()->value("username").toString();
        QString password = pluginStorage()->value("password").toString();
        pluginStorage()->endGroup();

        Tado *tado;
        if (m_unfinishedTadoAccounts.contains(thing->id())) {
            tado = m_unfinishedTadoAccounts.take(thing->id());
            m_tadoAccounts.insert(thing->id(), tado);
            return info->finish(Thing::ThingErrorNoError);
        } else {
            tado = new Tado(hardwareManager()->networkManager(), username, this);
            connect(tado, &Tado::tokenReceived, this, &IntegrationPluginTado::onTokenReceived);
            connect(tado, &Tado::authenticationStatusChanged, this, &IntegrationPluginTado::onAuthenticationStatusChanged);
            connect(tado, &Tado::connectionChanged, this, &IntegrationPluginTado::onConnectionChanged);
            connect(tado, &Tado::homesReceived, this, &IntegrationPluginTado::onHomesReceived);
            connect(tado, &Tado::zonesReceived, this, &IntegrationPluginTado::onZonesReceived);
            connect(tado, &Tado::zoneStateReceived, this, &IntegrationPluginTado::onZoneStateReceived);
            connect(tado, &Tado::overlayReceived, this, &IntegrationPluginTado::onOverlayReceived);
            tado->getToken(password);
            m_tadoAccounts.insert(thing->id(), tado);
            m_asyncDeviceSetup.insert(tado, info);
            return;
        }

    } else if (thing->thingClassId() == zoneThingClassId) {
        qCDebug(dcTado) << "Setup tado thermostat" << thing->params();
        return info->finish(Thing::ThingErrorNoError);
    }
    qCWarning(dcTado()) << "Unhandled thing class in setupDevice";
}

void IntegrationPluginTado::thingRemoved(Thing *thing)
{
    if (thing->thingClassId() == tadoConnectionThingClassId) {
        Tado *tado = m_tadoAccounts.take(thing->id());
        tado->deleteLater();
    }

    if (myThings().isEmpty() && m_pluginTimer) {
        m_pluginTimer->deleteLater();
        m_pluginTimer = nullptr;
    }
}

void IntegrationPluginTado::postSetupThing(Thing *thing)
{
    if (!m_pluginTimer) {
        m_pluginTimer = hardwareManager()->pluginTimerManager()->registerTimer(10);
        connect(m_pluginTimer, &PluginTimer::timeout, this, &IntegrationPluginTado::onPluginTimer);
    }

    if (thing->thingClassId() == tadoConnectionThingClassId) {
        Tado *tado = m_tadoAccounts.value(thing->id());
        thing->setStateValue(tadoConnectionUserDisplayNameStateTypeId, tado->username());
        thing->setStateValue(tadoConnectionLoggedInStateTypeId, true);
        thing->setStateValue(tadoConnectionConnectedStateTypeId, true);
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
        Tado *tado = m_tadoAccounts.value(thing->parentId());
        if (!tado)
            return;
        QString homeId = thing->paramValue(zoneThingHomeIdParamTypeId).toString();
        QString zoneId = thing->paramValue(zoneThingZoneIdParamTypeId).toString();
        if (action.actionTypeId() == zoneModeActionTypeId) {

            if (action.param(zoneModeActionModeParamTypeId).value().toString() == "Tado") {
                tado->deleteOverlay(homeId, zoneId);
            } else if (action.param(zoneModeActionModeParamTypeId).value().toString() == "Off") {
                tado->setOverlay(homeId, zoneId, false, thing->stateValue(zoneTargetTemperatureStateTypeId).toDouble());
            } else {
                if(thing->stateValue(zoneTargetTemperatureStateTypeId).toDouble() <= 5.0) {
                    tado->setOverlay(homeId, zoneId, true, 5);
                } else {
                    tado->setOverlay(homeId, zoneId, true, thing->stateValue(zoneTargetTemperatureStateTypeId).toDouble());
                }
            }
            info->finish(Thing::ThingErrorNoError);
        } else if (action.actionTypeId() == zoneTargetTemperatureActionTypeId) {

            double temperature = action.param(zoneTargetTemperatureActionTargetTemperatureParamTypeId).value().toDouble();
            if (temperature <= 0) {
                QUuid requestId = tado->setOverlay(homeId, zoneId, false, 0);
                m_asyncActions.insert(requestId, info);
            } else {
                tado->setOverlay(homeId, zoneId, true, temperature);
            }
            info->finish(Thing::ThingErrorNoError);
        }
    }
}

void IntegrationPluginTado::onPluginTimer()
{
    foreach (Thing *thing, myThings().filterByThingClassId(zoneThingClassId)) {
        Tado *tado = m_tadoAccounts.value(thing->parentId());
        if (!tado)
            continue;

        QString homeId = thing->paramValue(zoneThingHomeIdParamTypeId).toString();
        QString zoneId = thing->paramValue(zoneThingZoneIdParamTypeId).toString();
        tado->getZoneState(homeId, zoneId);
    }
}

void IntegrationPluginTado::onConnectionChanged(bool connected)
{
    Tado *tado = static_cast<Tado*>(sender());

    if (m_tadoAccounts.values().contains(tado)){
        Thing *thing = myThings().findById(m_tadoAccounts.key(tado));
        thing->setStateValue(tadoConnectionConnectedStateTypeId, connected);

        foreach(Thing *zoneThing, myThings().filterByParentId(thing->id())) {
            zoneThing->setStateValue(zoneConnectedStateTypeId, connected);
        }
    }
}

void IntegrationPluginTado::onAuthenticationStatusChanged(bool authenticated)
{
    Tado *tado = static_cast<Tado*>(sender());

    if (m_unfinishedTadoAccounts.values().contains(tado) && !authenticated){
        ThingId id = m_unfinishedTadoAccounts.key(tado);
        m_unfinishedTadoAccounts.remove(id);
        ThingPairingInfo *info = m_unfinishedDevicePairings.take(id);
        info->finish(Thing::ThingErrorSetupFailed);
    }

    if (m_tadoAccounts.values().contains(tado)){
        Thing *thing = myThings().findById(m_tadoAccounts.key(tado));
        thing->setStateValue(tadoConnectionLoggedInStateTypeId, authenticated);
    }
}

void IntegrationPluginTado::onTokenReceived(Tado::Token token)
{
    Q_UNUSED(token);

    qCDebug(dcTado()) << "Token received";
    Tado *tado = static_cast<Tado*>(sender());

    if (m_asyncDeviceSetup.contains(tado)) {
        ThingSetupInfo *info = m_asyncDeviceSetup.take(tado);
        info->finish(Thing::ThingErrorNoError);
    }

    if (m_unfinishedTadoAccounts.values().contains(tado)) {
        ThingId id = m_unfinishedTadoAccounts.key(tado);
        ThingPairingInfo *info = m_unfinishedDevicePairings.take(id);
        info->finish(Thing::ThingErrorNoError);
    }
}

void IntegrationPluginTado::onHomesReceived(QList<Tado::Home> homes)
{
    qCDebug(dcTado()) << "Homes received";
    Tado *tado = static_cast<Tado*>(sender());
    foreach (Tado::Home home, homes) {
        tado->getZones(home.id);
    }
}

void IntegrationPluginTado::onZonesReceived(const QString &homeId, QList<Tado::Zone> zones)
{
    Tado *tado = static_cast<Tado*>(sender());

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
    Tado *tado = static_cast<Tado*>(sender());
    ThingId parentId = m_tadoAccounts.key(tado);
    ParamList params;
    params.append(Param(zoneThingHomeIdParamTypeId, homeId));
    params.append(Param(zoneThingZoneIdParamTypeId, zoneId));
    Thing *thing = myThings().filterByParentId(parentId).findByParams(params);
    if (!thing)
        return;

    if (state.overlayIsSet)  {
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
    thing->setStateValue(zoneTargetTemperatureStateTypeId, state.settingTemperature);
    thing->setStateValue(zoneTemperatureStateTypeId, state.temperature);
    thing->setStateValue(zoneHumidityStateTypeId, state.humidity);
    thing->setStateValue(zoneWindowOpenStateTypeId, state.windowOpen);
    thing->setStateValue(zoneTadoModeStateTypeId, state.tadoMode);
}

void IntegrationPluginTado::onOverlayReceived(const QString &homeId, const QString &zoneId, const Tado::Overlay &overlay)
{
    Tado *tado = static_cast<Tado*>(sender());
    ThingId parentId = m_tadoAccounts.key(tado);
    ParamList params;
    params.append(Param(zoneThingHomeIdParamTypeId, homeId));
    params.append(Param(zoneThingZoneIdParamTypeId, zoneId));
    Thing *thing = myThings().filterByParentId(parentId).findByParams(params);
    if (!thing)
        return;
    thing->setStateValue(zoneTargetTemperatureStateTypeId, overlay.temperature);

    if (overlay.tadoMode == "MANUAL")  {
        if (overlay.power) {
            thing->setStateValue(zoneModeStateTypeId, "Manual");
        } else {
            thing->setStateValue(zoneModeStateTypeId, "Off");
        }
    } else {
        thing->setStateValue(zoneModeStateTypeId, "Tado");
    }
}
