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
#include <QTimer>

IntegrationPluginTado::IntegrationPluginTado()
{

}

void IntegrationPluginTado::startPairing(ThingPairingInfo *info)
{
    qCDebug(dcTado()) << "Start pairing process, checking the internet connection ...";
    NetworkAccessManager *network = hardwareManager()->networkManager();
    QNetworkReply *reply = network->get(QNetworkRequest(QUrl("https://my.tado.com/api/v2")));
    connect(reply, &QNetworkReply::finished, reply, &QNetworkReply::deleteLater);
    connect(reply, &QNetworkReply::finished, info, [reply, info] {

        if (reply->error() == QNetworkReply::NetworkError::HostNotFoundError) {
            qCWarning(dcTado()) << "Tado server is not reachable, likely because of a missing internet connection.";
            info->finish(Thing::ThingErrorHardwareNotAvailable, QT_TR_NOOP("Tado server is not reachable."));
        } else {
            qCDebug(dcTado()) << "Internet connection available";
            info->finish(Thing::ThingErrorNoError, QT_TR_NOOP("Please enter the login credentials for your Tado account."));
        }
    });
}

void IntegrationPluginTado::confirmPairing(ThingPairingInfo *info, const QString &username, const QString &password)
{
    qCDebug(dcTado()) << "Confirm pairing" << username << "Network manager available" << hardwareManager()->networkManager()->available();
    Tado *tado = new Tado(hardwareManager()->networkManager(), username, this);
    m_unfinishedTadoAccounts.insert(info->thingId(), tado);

    connect(info, &ThingPairingInfo::aborted, this, [info, tado, this]() {
        qCWarning(dcTado()) << "Thing pairing has been aborted, going to clean-up";
        m_unfinishedTadoAccounts.remove(info->thingId());
        tado->deleteLater();
    });

    connect(tado, &Tado::connectionError, info, [info] (QNetworkReply::NetworkError error){

        if (error == QNetworkReply::NetworkError::ProtocolInvalidOperationError) {
            qCWarning(dcTado()) << "Confirm pairing failed, wrong username or password";
            info->finish(Thing::ThingErrorSetupFailed, QT_TR_NOOP("Wrong username or password."));
        } else if (error != QNetworkReply::NetworkError::NoError){
            qCWarning(dcTado()) << "Confirm pairing failed" << error;
            info->finish(Thing::ThingErrorSetupFailed, QT_TR_NOOP("Connection error"));
        }
        // info->finish(success) will be called after the token has been received
    });

    connect(tado, &Tado::apiCredentialsReceived, info, [info, password, tado] (bool success) {
        if (success) {
            tado->getToken(password);
        } else {
            info->finish(Thing::ThingErrorAuthenticationFailure, QT_TR_NOOP("Client credentials not found, the plug-in version might be outdated."));
        }
    });

    connect(tado, &Tado::tokenReceived, info, [this, info, username, password](Tado::Token token) {
        Q_UNUSED(token)

        pluginStorage()->beginGroup(info->thingId().toString());
        pluginStorage()->setValue("username", username);
        pluginStorage()->setValue("password", password);
        pluginStorage()->endGroup();

        info->finish(Thing::ThingErrorNoError);
    });
    tado->getApiCredentials();
}

void IntegrationPluginTado::setupThing(ThingSetupInfo *info)
{
    Thing *thing = info->thing();

    if (thing->thingClassId() == tadoAccountThingClassId) {

        qCDebug(dcTado) << "Setup Tado account" << thing->name() << thing->params();
        Tado *tado;

        if (m_tadoAccounts.contains(thing->id())) {
            qCDebug(dcTado()) << "Setup after reconfigure, cleaning up";
            m_tadoAccounts.take(thing->id())->deleteLater();
        }

        if (m_unfinishedTadoAccounts.contains(thing->id())) {
            qCDebug(dcTado()) << "Using Tado connection from pairing process";
            tado = m_unfinishedTadoAccounts.take(thing->id());
            m_tadoAccounts.insert(thing->id(), tado);
            info->finish(Thing::ThingErrorNoError);
        } else {
            pluginStorage()->beginGroup(thing->id().toString());
            QString username = pluginStorage()->value("username").toString();
            QString password = pluginStorage()->value("password").toString();
            pluginStorage()->endGroup();

            tado = new Tado(hardwareManager()->networkManager(), username, this);
            m_tadoAccounts.insert(thing->id(), tado);
            connect(info, &ThingSetupInfo::aborted, [info, this] {
                if (m_tadoAccounts.contains(info->thing()->id())) {
                    Tado *tado = m_tadoAccounts.take(info->thing()->id());
                    tado->deleteLater();
                }
            });

            connect(tado, &Tado::apiCredentialsReceived, info, [password, tado] {
                tado->getToken(password);
            });

            connect(tado, &Tado::tokenReceived, info, [ info](Tado::Token token) {
                Q_UNUSED(token)

                qCDebug(dcTado()) << "Token received, account setup successfull";
                info->finish(Thing::ThingErrorNoError);
            });

            connect(tado, &Tado::connectionError, info, [this, info] (QNetworkReply::NetworkError error) {

                if (error != QNetworkReply::NetworkError::NoError){

                    if (m_tadoAccounts.contains(info->thing()->id())) {
                        Tado *tado = m_tadoAccounts.take(info->thing()->id());
                        tado->deleteLater();
                    }
                    if (error == QNetworkReply::NetworkError::ProtocolInvalidOperationError) {
                        qCWarning(dcTado()) << "Confirm pairing failed, wrong username or password";
                        info->finish(Thing::ThingErrorSetupFailed, QT_TR_NOOP("Wrong username or password."));
                    } else {
                        qCWarning(dcTado()) << "Confirm pairing failed" << error;
                        info->finish(Thing::ThingErrorSetupFailed, QT_TR_NOOP("Connection error"));
                    }
                }
            });
            tado->getApiCredentials();
        }
        connect(tado, &Tado::authenticationStatusChanged, this, &IntegrationPluginTado::onAuthenticationStatusChanged);
        connect(tado, &Tado::requestExecuted, this, &IntegrationPluginTado::onRequestExecuted);
        connect(tado, &Tado::connectionChanged, this, &IntegrationPluginTado::onConnectionChanged);
        connect(tado, &Tado::homesReceived, this, &IntegrationPluginTado::onHomesReceived);
        connect(tado, &Tado::zonesReceived, this, &IntegrationPluginTado::onZonesReceived);
        connect(tado, &Tado::zoneStateReceived, this, &IntegrationPluginTado::onZoneStateReceived);
        connect(tado, &Tado::overlayReceived, this, &IntegrationPluginTado::onOverlayReceived);
        return;

    } else if (thing->thingClassId() == zoneThingClassId) {
        qCDebug(dcTado) << "Setup Tado zone" << thing->params();
        Thing *parentThing = myThings().findById(thing->parentId());
        if(parentThing->setupComplete()) {
            return info->finish(Thing::ThingErrorNoError);
        } else {
            connect(parentThing, &Thing::setupStatusChanged, info, [parentThing, info]{
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

    if (thing->thingClassId() == tadoAccountThingClassId) {
        Tado *tado = m_tadoAccounts.value(thing->id());
        thing->setStateValue(tadoAccountUserDisplayNameStateTypeId, tado->username());
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
        Tado *tado = m_tadoAccounts.value(thing->parentId());
        if (!tado) {
            info->finish(Thing::ThingErrorThingNotFound);
            return;
        }
        QString homeId = thing->paramValue(zoneThingHomeIdParamTypeId).toString();
        QString zoneId = thing->paramValue(zoneThingZoneIdParamTypeId).toString();
        if (action.actionTypeId() == zoneModeActionTypeId) {
            QUuid requestId;
            if (action.param(zoneModeActionModeParamTypeId).value().toString() == "Tado") {
                requestId = tado->deleteOverlay(homeId, zoneId);
            } else if (action.param(zoneModeActionModeParamTypeId).value().toString() == "Off") {
                requestId = tado->setOverlay(homeId, zoneId, false, thing->stateValue(zoneTargetTemperatureStateTypeId).toDouble());
            } else {
                if(thing->stateValue(zoneTargetTemperatureStateTypeId).toDouble() <= 5.0) {
                    requestId =  tado->setOverlay(homeId, zoneId, true, 5);
                } else {
                    requestId = tado->setOverlay(homeId, zoneId, true, thing->stateValue(zoneTargetTemperatureStateTypeId).toDouble());
                }
            }
            m_asyncActions.insert(requestId, info);
            connect(info, &ThingActionInfo::aborted, [requestId, this] {m_asyncActions.remove(requestId);});
        } else if (action.actionTypeId() == zoneTargetTemperatureActionTypeId) {

            double temperature = action.param(zoneTargetTemperatureActionTargetTemperatureParamTypeId).value().toDouble();
            QUuid requestId;
            if (temperature <= 0) {
                requestId = tado->setOverlay(homeId, zoneId, false, 0);
            } else {
                requestId = tado->setOverlay(homeId, zoneId, true, temperature);
            }
            m_asyncActions.insert(requestId, info);
            connect(info, &ThingActionInfo::aborted, [requestId, this] {m_asyncActions.remove(requestId);});
        } else if (action.actionTypeId() == zonePowerActionTypeId) {
            bool power = action.param(zonePowerActionPowerParamTypeId).value().toBool();
            thing->setStateValue(zonePowerStateTypeId, power); // the actual power set response might be slow
            QUuid requestId;
            double temperature = thing->stateValue(zoneTargetTemperatureStateTypeId).toDouble();
            if (!power) {
                requestId = tado->setOverlay(homeId, zoneId, false, 0);
            } else {
                requestId = tado->setOverlay(homeId, zoneId, true, temperature);
            }
            m_asyncActions.insert(requestId, info);
            connect(info, &ThingActionInfo::aborted, [requestId, this] {m_asyncActions.remove(requestId);});
        } else {
            qCWarning(dcTado()) << "Execute action, unhandled actionTypeId" << action.actionTypeId();
            info->finish(Thing::ThingErrorActionTypeNotFound);
        }
    } else {
        qCWarning(dcTado()) << "Execute action, unhandled thingClassId" << thing->thingClassId();
        info->finish(Thing::ThingErrorThingClassNotFound);
    }
}

void IntegrationPluginTado::onPluginTimer()
{
    Q_FOREACH(Tado *tado, m_tadoAccounts){
        ThingId accountThingId = m_tadoAccounts.key(tado);
        if (!tado->authenticated()) {
            pluginStorage()->beginGroup(accountThingId.toString());
            QString password = pluginStorage()->value("password").toString();
            pluginStorage()->endGroup();
            tado->getToken(password);
        } else {
            Q_FOREACH(Thing *thing, myThings().filterByParentId(accountThingId)) {
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
    Tado *tado = static_cast<Tado*>(sender());

    if (m_tadoAccounts.values().contains(tado)){
        Thing *thing = myThings().findById(m_tadoAccounts.key(tado));
        if (!thing)
            return;
        thing->setStateValue(tadoAccountConnectedStateTypeId, connected);

        if (!connected) {
            Q_FOREACH(Thing *child, myThings().filterByParentId(thing->id())) {
                if (child->thingClassId() == zoneThingClassId) {
                    child->setStateValue(zoneConnectedStateTypeId, connected);
                }
            }
        }
    }
}

void IntegrationPluginTado::onAuthenticationStatusChanged(bool authenticated)
{
    Tado *tado = static_cast<Tado*>(sender());

    if (m_tadoAccounts.values().contains(tado)){
        Thing *thing = myThings().findById(m_tadoAccounts.key(tado));
        if (!thing){
            qCWarning(dcTado()) << "OnAuthenticationChanged no thing found by ID" << m_tadoAccounts.key(tado);
            return;
        }
        thing->setStateValue(tadoAccountLoggedInStateTypeId, authenticated);
        if (!authenticated) {
            Q_FOREACH(Thing *child, myThings().filterByParentId(thing->id())) {
                if (child->thingClassId() == zoneThingClassId) {
                    child->setStateValue(zoneConnectedStateTypeId, authenticated);
                }
            }
        }
    }
}

void IntegrationPluginTado::onRequestExecuted(QUuid requestId, bool success)
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
