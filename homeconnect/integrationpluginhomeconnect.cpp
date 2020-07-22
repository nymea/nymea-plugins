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

#include "integrationpluginhomeconnect.h"
#include "integrations/integrationplugin.h"
#include "network/networkaccessmanager.h"
#include "plugininfo.h"

#include <QNetworkRequest>
#include <QNetworkReply>
#include <QUrlQuery>
#include <QJsonDocument>

IntegrationPluginHomeConnect::IntegrationPluginHomeConnect()
{
    m_idParamTypeIds.insert(ovenThingClassId, ovenThingIdParamTypeId);
    m_idParamTypeIds.insert(fridgeThingClassId, fridgeThingIdParamTypeId);
    m_idParamTypeIds.insert(dryerThingClassId, dryerThingIdParamTypeId);
    m_idParamTypeIds.insert(coffeMakerThingClassId, coffeMakerThingIdParamTypeId);
    m_idParamTypeIds.insert(dishwasherThingClassId, dishwasherThingIdParamTypeId);
    m_idParamTypeIds.insert(washerThingClassId, washerThingIdParamTypeId);
    m_idParamTypeIds.insert(cookTopThingClassId, cookTopThingIdParamTypeId);
    m_idParamTypeIds.insert(hoodThingClassId, hoodThingIdParamTypeId);
    m_idParamTypeIds.insert(cleaningRobotThingClassId, cleaningRobotThingIdParamTypeId);

    m_connectedStateTypeIds.insert(ovenThingClassId, ovenConnectedStateTypeId);
    m_connectedStateTypeIds.insert(fridgeThingClassId, fridgeConnectedStateTypeId);
    m_connectedStateTypeIds.insert(dryerThingClassId, dryerConnectedStateTypeId);
    m_connectedStateTypeIds.insert(coffeMakerThingClassId, coffeMakerConnectedStateTypeId);
    m_connectedStateTypeIds.insert(dishwasherThingClassId, dishwasherConnectedStateTypeId);
    m_connectedStateTypeIds.insert(washerThingClassId, washerConnectedStateTypeId);
    m_connectedStateTypeIds.insert(cookTopThingClassId, cookTopConnectedStateTypeId);
    m_connectedStateTypeIds.insert(cleaningRobotThingClassId, cleaningRobotConnectedStateTypeId);
    m_connectedStateTypeIds.insert(hoodThingClassId, hoodConnectedStateTypeId);

    m_localControlStateTypeIds.insert(ovenThingClassId, ovenLocalControlStateStateTypeId);
    //m_localControlStateTypeIds.insert(fridgeThingClassId, fridgeLocalControlStateStateTypeId);
    //m_localControlStateTypeIds.insert(dryerThingClassId, dryerLocalControlStateTypeId);
    m_localControlStateTypeIds.insert(coffeMakerThingClassId, coffeMakerLocalControlStateStateTypeId);
    //m_localControlStateTypeIds.insert(dishwasherThingClassId, dishwasherLocalControlStateStateTypeId);
    m_localControlStateTypeIds.insert(washerThingClassId, washerLocalControlStateStateTypeId);
    //m_localControlStateTypeIds.insert(cookTopThingClassId, cookTopLocalControlStateStateTypeId);
    //m_localControlStateTypeIds.insert(cleaningRobotThingClassId, cleaningRobotLocalControlStateStateTypeId);
    //m_localControlStateTypeIds.insert(hoodThingClassId, hoodLocalControlStateStateTypeId);

    m_remoteStartAllowanceStateTypeIds.insert(ovenThingClassId, ovenRemoteStartAllowanceStateStateTypeId);
    //m_remoteStartAllowanceStateTypeIds.insert(fridgeThingClassId, fridgeRemoteStartAllowanceStateStateTypeId);
    m_remoteStartAllowanceStateTypeIds.insert(dryerThingClassId, dryerRemoteStartAllowanceStateStateTypeId);
    m_remoteStartAllowanceStateTypeIds.insert(coffeMakerThingClassId, coffeMakerRemoteStartAllowanceStateStateTypeId);
    m_remoteStartAllowanceStateTypeIds.insert(dishwasherThingClassId, dishwasherRemoteStartAllowanceStateStateTypeId);
    m_remoteStartAllowanceStateTypeIds.insert(washerThingClassId, washerRemoteStartAllowanceStateStateTypeId);
    //m_remoteStartAllowanceStateTypeIds.insert(cookTopThingClassId, cookTopRemoteStartAllowanceStateStateTypeId);
    //m_remoteStartAllowanceStateTypeIds.insert(cleaningRobotThingClassId, cleaningRobotRemoteStartAllowanceStateStateTypeId);
    //m_remoteStartAllowanceStateTypeIds.insert(hoodThingClassId, hoodRemoteStartAllowanceStateStateTypeId);

    m_remoteControlActivationStateTypeIds.insert(ovenThingClassId, ovenRemoteControlActivationStateStateTypeId);
    //m_remoteControlActivationStateTypeIds.insert(fridgeThingClassId, fridgeRemoteControlActivationStateStateTypeId);
    m_remoteControlActivationStateTypeIds.insert(dryerThingClassId, dryerRemoteControlActivationStateStateTypeId);
    //m_remoteControlActivationStateTypeIds.insert(coffeMakerThingClassId, coffeMakerRemoteControlActivationStateStateTypeId);
    m_remoteControlActivationStateTypeIds.insert(dishwasherThingClassId, dishwasherRemoteControlActivationStateStateTypeId);
    m_remoteControlActivationStateTypeIds.insert(washerThingClassId, washerRemoteControlActivationStateStateTypeId);
    //m_remoteControlActivationStateTypeIds.insert(cookTopThingClassId, cookTopRemoteControlActivationStateStateTypeId);
    //m_remoteControlActivationStateTypeIds.insert(cleaningRobotThingClassId, cleaningRobotRemoteControlActivationStateStateTypeId);
    //m_remoteControlActivationStateTypeIds.insert(hoodThingClassId, hoodRemoteControlActivationStateStateTypeId);

    m_doorStateTypeIds.insert(dishwasherThingClassId, dishwasherDoorStateEventTypeId);
    m_doorStateTypeIds.insert(washerThingClassId, washerDoorStateEventTypeId);
    m_doorStateTypeIds.insert(dryerThingClassId, dryerDoorStateEventTypeId);
    m_doorStateTypeIds.insert(ovenThingClassId, ovenDoorStateEventTypeId);

    m_operationStateTypeIds.insert(ovenThingClassId, ovenOperationStateEventTypeId);
    //m_operationStateTypeIds.insert(fridgeThingClassId, fridgeOperationStateEventTypeId);
    m_operationStateTypeIds.insert(dryerThingClassId, dryerOperationStateEventTypeId);
    m_operationStateTypeIds.insert(coffeMakerThingClassId, coffeMakerOperationStateEventTypeId);
    m_operationStateTypeIds.insert(dishwasherThingClassId, dishwasherOperationStateEventTypeId);
    m_operationStateTypeIds.insert(washerThingClassId, washerOperationStateEventTypeId);
    //m_operationStateTypeIds.insert(cookTopThingClassId, cookTopOperationStateEventTypeId);
    //m_operationStateTypeIds.insert(cleaningRobotThingClassId, cleaningRobotOperationStateEventTypeId);
    //m_operationStateTypeIds.insert(hoodThingClassId, hoodOperationStateEventTypeId);
}

void IntegrationPluginHomeConnect::startPairing(ThingPairingInfo *info)
{
    if (info->thingClassId() == homeConnectConnectionThingClassId) {

        bool simulationMode =  configValue(homeConnectPluginSimulationModeParamTypeId).toBool();
        HomeConnect *homeConnect = new HomeConnect(hardwareManager()->networkManager(), "423713AB3EDA5B44BCE6E7B3546C43DADCB27A156C681E30455250637B2213DB", "AE182EA9F1CB99416DFD62CE61BF6DCDB3BB7D4697B58D4499D3792EC9F7412D", simulationMode, this);
        QUrl url = homeConnect->getLoginUrl(QUrl("https://127.0.0.1:8888"), "IdentifyAppliance Monitor Settings");
        qCDebug(dcHomeConnect()) << "HomeConnect url:" << url;
        info->setOAuthUrl(url);
        info->finish(Thing::ThingErrorNoError);
        m_setupHomeConnectConnections.insert(info->thingId(), homeConnect);
    } else {
        qCWarning(dcHomeConnect()) << "Unhandled pairing metod!";
        info->finish(Thing::ThingErrorCreationMethodNotSupported);
    }
}

void IntegrationPluginHomeConnect::confirmPairing(ThingPairingInfo *info, const QString &username, const QString &secret)
{
    Q_UNUSED(username);

    if (info->thingClassId() == homeConnectConnectionThingClassId) {
        qCDebug(dcHomeConnect()) << "Redirect url is" << secret;
        QUrl url(secret);
        QUrlQuery query(url);
        QByteArray authorizationCode = query.queryItemValue("code").toLocal8Bit();
        //QByteArray state = query.queryItemValue("state").toLocal8Bit();
        //TODO evaluate state if it equals the given state

        HomeConnect *homeConnect = m_setupHomeConnectConnections.value(info->thingId());
        if (!homeConnect) {
            qWarning(dcHomeConnect()) << "No HomeConnect connection found for device:"  << info->thingName();
            m_setupHomeConnectConnections.remove(info->thingId());
            homeConnect->deleteLater();
            info->finish(Thing::ThingErrorHardwareFailure);
            return;
        }
        qCDebug(dcHomeConnect()) << "Authorization code" << authorizationCode;
        homeConnect->getAccessTokenFromAuthorizationCode(authorizationCode);
        connect(homeConnect, &HomeConnect::authenticationStatusChanged, this, [info, this](bool authenticated){
            HomeConnect *homeConnect = static_cast<HomeConnect *>(sender());

            if(!authenticated) {
                qWarning(dcHomeConnect()) << "Authentication process failed";
                m_setupHomeConnectConnections.remove(info->thingId());
                homeConnect->deleteLater();
                info->finish(Thing::ThingErrorSetupFailed);
                return;
            }
            QByteArray accessToken = homeConnect->accessToken();
            QByteArray refreshToken = homeConnect->refreshToken();
            qCDebug(dcHomeConnect()) << "Token:" << accessToken << refreshToken;

            pluginStorage()->beginGroup(info->thingId().toString());
            pluginStorage()->setValue("refresh_token", refreshToken);
            pluginStorage()->endGroup();

            info->finish(Thing::ThingErrorNoError);
        });
    } else {
        Q_ASSERT_X(false, "confirmPairing", QString("Unhandled thingClassId: %1").arg(info->thingClassId().toString()).toUtf8());
    }
}


void IntegrationPluginHomeConnect::setupThing(ThingSetupInfo *info)
{
    Thing *thing = info->thing();

    if (thing->thingClassId() == homeConnectConnectionThingClassId) {
        bool simulationMode =  configValue(homeConnectPluginSimulationModeParamTypeId).toBool();
        HomeConnect *homeConnect;
        if (m_setupHomeConnectConnections.keys().contains(thing->id())) {
            //Fresh device setup, has already a fresh access token
            qCDebug(dcHomeConnect()) << "HomeConnect OAuth setup complete";
            homeConnect = m_setupHomeConnectConnections.take(thing->id());
            m_homeConnectConnections.insert(thing, homeConnect);
            info->finish(Thing::ThingErrorNoError);
        } else {
            //device loaded from the device database, needs a new access token;
            pluginStorage()->beginGroup(thing->id().toString());
            QByteArray refreshToken = pluginStorage()->value("refresh_token").toByteArray();
            pluginStorage()->endGroup();
            homeConnect = new HomeConnect(hardwareManager()->networkManager(), "423713AB3EDA5B44BCE6E7B3546C43DADCB27A156C681E30455250637B2213DB", "AE182EA9F1CB99416DFD62CE61BF6DCDB3BB7D4697B58D4499D3792EC9F7412D", simulationMode,  this);
            homeConnect->getAccessTokenFromRefreshToken(refreshToken);
            m_asyncSetup.insert(homeConnect, info);
        }
        connect(homeConnect, &HomeConnect::connectionChanged, this, &IntegrationPluginHomeConnect::onConnectionChanged);
        connect(homeConnect, &HomeConnect::commandExecuted, this, &IntegrationPluginHomeConnect::onRequestExecuted);
        connect(homeConnect, &HomeConnect::authenticationStatusChanged, this, &IntegrationPluginHomeConnect::onAuthenticationStatusChanged);
        connect(homeConnect, &HomeConnect::receivedHomeAppliances, this, &IntegrationPluginHomeConnect::onReceivedHomeAppliances);
        connect(homeConnect, &HomeConnect::receivedStatusList, this, &IntegrationPluginHomeConnect::onReceivedStatusList);
        connect(homeConnect, &HomeConnect::receivedActiveProgram, this, &IntegrationPluginHomeConnect::onReceivedActiveProgram);
        connect(homeConnect, &HomeConnect::receivedSelectedProgram, this, &IntegrationPluginHomeConnect::onReceivedSelectedProgram);
        connect(homeConnect, &HomeConnect::receivedSettings, this, &IntegrationPluginHomeConnect::onReceivedSettings);
        connect(homeConnect, &HomeConnect::receivedEvents, this, &IntegrationPluginHomeConnect::onReceivedEvents);

    } else if ((thing->thingClassId() == dryerThingClassId) ||
               (thing->thingClassId() == fridgeThingClassId) ||
               (thing->thingClassId() == washerThingClassId) ||
               (thing->thingClassId() == dishwasherThingClassId) ||
               (thing->thingClassId() == coffeMakerThingClassId) ||
               (thing->thingClassId() == ovenThingClassId) ||
               (thing->thingClassId() == hoodThingClassId) ||
               (thing->thingClassId() == cleaningRobotThingClassId) ||
               (thing->thingClassId() == cookTopThingClassId)) {
        Thing *parentThing = myThings().findById(thing->parentId());
        if (parentThing->setupComplete()) {
            info->finish(Thing::ThingErrorNoError);
        } else {
            connect(parentThing, &Thing::setupStatusChanged, info, [parentThing, info]{
                if (parentThing->setupComplete()) {
                    info->finish(Thing::ThingErrorNoError);
                }
            });
        }
    } else {
        Q_ASSERT_X(false, "setupThing", QString("Unhandled thingClassId: %1").arg(thing->thingClassId().toString()).toUtf8());
    }
}

void IntegrationPluginHomeConnect::postSetupThing(Thing *thing)
{
    if (!m_pluginTimer5sec) {
        m_pluginTimer5sec = hardwareManager()->pluginTimerManager()->registerTimer(5);
        connect(m_pluginTimer5sec, &PluginTimer::timeout, this, [this]() {

            foreach (Thing *connectionThing, myThings().filterByThingClassId(homeConnectConnectionThingClassId)) {
                HomeConnect *homeConnect = m_homeConnectConnections.value(connectionThing);
                if (!homeConnect) {
                    qWarning(dcHomeConnect()) << "No HomeConnect account found for" << connectionThing->name();
                    continue;
                }
                foreach (Thing *childThing, myThings().filterByParentId(connectionThing->id())) {
                    QString haid = childThing->paramValue(m_idParamTypeIds.value(childThing->thingClassId())).toString();
                    homeConnect->getStatus(haid);
                }
            }
        });
    }

    if (!m_pluginTimer60sec) {
        m_pluginTimer60sec = hardwareManager()->pluginTimerManager()->registerTimer(60);
        connect(m_pluginTimer60sec, &PluginTimer::timeout, this, [this]() {
            foreach (Thing *thing, myThings().filterByThingClassId(homeConnectConnectionThingClassId)) {
                HomeConnect *homeConnect = m_homeConnectConnections.value(thing);
                if (!homeConnect) {
                    qWarning(dcHomeConnect()) << "No HomeConnect account found for" << thing->name();
                    continue;
                }
                homeConnect->getHomeAppliances();
                Q_FOREACH(Thing *childThing, myThings().filterByParentId(thing->id())) {
                    QString haId = thing->paramValue(m_idParamTypeIds.value(childThing->thingClassId())).toString();
                    homeConnect->getSettings(haId);
                    homeConnect->getProgramsActive(haId);
                    homeConnect->getProgramsSelected(haId);
                }
            }
        });
    }

    if (thing->thingClassId() == homeConnectConnectionThingClassId) {
        HomeConnect *homeConnect = m_homeConnectConnections.value(thing);
        homeConnect->getHomeAppliances();
        homeConnect->connectEventStream();
        thing->setStateValue(homeConnectConnectionConnectedStateTypeId, true);
        thing->setStateValue(homeConnectConnectionLoggedInStateTypeId, true);
        //TODO set login username
    } else if ((thing->thingClassId() == dryerThingClassId) ||
               (thing->thingClassId() == fridgeThingClassId) ||
               (thing->thingClassId() == washerThingClassId) ||
               (thing->thingClassId() == dishwasherThingClassId) ||
               (thing->thingClassId() == coffeMakerThingClassId) ||
               (thing->thingClassId() == ovenThingClassId) ||
               (thing->thingClassId() == hoodThingClassId) ||
               (thing->thingClassId() == cleaningRobotThingClassId) ||
               (thing->thingClassId() == cookTopThingClassId)) {
        Thing *parentThing = myThings().findById(thing->parentId());
        if (!parentThing)
            qCWarning(dcHomeConnect()) << "Could not find parent with Id" << thing->parentId().toString();
        HomeConnect *homeConnect = m_homeConnectConnections.value(parentThing);
        QString haId = thing->paramValue(m_idParamTypeIds.value(thing->thingClassId())).toString();
        if (!homeConnect) {
            qCWarning(dcHomeConnect()) << "Could not find HomeConnect connection for thing" << thing->name();
        } else {
            homeConnect->getStatus(haId);
            homeConnect->getSettings(haId);
            homeConnect->getProgramsActive(haId);
            homeConnect->getProgramsSelected(haId);
        }
    } else {
        Q_ASSERT_X(false, "postSetupThing", QString("Unhandled thingClassId: %1").arg(thing->thingClassId().toString()).toUtf8());
    }
}

void IntegrationPluginHomeConnect::executeAction(ThingActionInfo *info)
{
    Thing *thing = info->thing();
    Action action = info->action();
    if (thing->thingClassId() == homeConnectConnectionThingClassId) {
        Q_ASSERT_X(false, "executeAction", QString("Unhandled actionTypeId: %1").arg(action.actionTypeId().toString()).toUtf8());

    } else if (thing->thingClassId() == ovenThingClassId) {
        HomeConnect *homeConnect = m_homeConnectConnections.value(myThings().findById(thing->parentId()));
        QString haid = thing->stateValue(m_idParamTypeIds.value(thing->thingClassId())).toString();
        QUuid requestId;
        if (action.actionTypeId() == ovenPauseActionTypeId) {
            requestId = homeConnect->sendCommand(haid, "BSH.Common.Command.PauseProgram");
        } else if (action.actionTypeId() == ovenResumeActionTypeId) {
            requestId = homeConnect->sendCommand(haid, "BSH.Common.Command.ResumeProgram");
        } else {
            Q_ASSERT_X(false, "executeAction", QString("Unhandled actionTypeId: %1").arg(action.actionTypeId().toString()).toUtf8());
        }
        m_pendingActions.insert(requestId, info);
        connect(info, &ThingActionInfo::aborted, [requestId, this] {
            m_pendingActions.remove(requestId);
        });
    } else {
        Q_ASSERT_X(false, "executeAction", QString("Unhandled thingClassId: %1").arg(thing->thingClassId().toString()).toUtf8());
    }
}

void IntegrationPluginHomeConnect::thingRemoved(Thing *thing)
{
    qCDebug(dcHomeConnect) << "Delete " << thing->name();
    if (thing->thingClassId() == homeConnectConnectionThingClassId) {
        m_homeConnectConnections.take(thing)->deleteLater();
    }

    if (myThings().empty()) {
        if (m_pluginTimer5sec) {
            m_pluginTimer5sec = nullptr;
            hardwareManager()->pluginTimerManager()->unregisterTimer(m_pluginTimer5sec);
        }
        if (m_pluginTimer60sec) {
            hardwareManager()->pluginTimerManager()->unregisterTimer(m_pluginTimer60sec);
            m_pluginTimer60sec = nullptr;
        }
    }
}

void IntegrationPluginHomeConnect::browseThing(BrowseResult *result)
{
    Thing *thing = result->thing();
    qCDebug(dcHomeConnect()) << "Browse thing called " << thing->name();

    HomeConnect *homeConnect = m_homeConnectConnections.value(myThings().findById(thing->parentId()));
    if (!homeConnect)
        return;
    QString haid = thing->stateValue(m_idParamTypeIds.value(thing->thingClassId())).toString();
    homeConnect->getProgramsAvailable(haid);
    connect(homeConnect, &HomeConnect::receivedAvailablePrograms, result, [result, this] (const QString &haId, const QStringList programs) {
        if(result->thing()->paramValue(m_idParamTypeIds.value(result->thing()->id())).toString() == haId) {
            Q_FOREACH(QString program, programs) {
                BrowserItem item;
                item.setExecutable(true);
                item.setDisplayName(program);
                result->addItem(item);
            }
            result->finish(Thing::ThingErrorNoError);
        }
    });
}

void IntegrationPluginHomeConnect::browserItem(BrowserItemResult *result)
{
    Thing *thing = result->thing();
    qCDebug(dcHomeConnect()) << "Browse item called " << thing->name();

    HomeConnect *homeConnect = m_homeConnectConnections.value(myThings().findById(thing->parentId()));
    if (!homeConnect)
        return;
    QString haid = thing->stateValue(m_idParamTypeIds.value(thing->thingClassId())).toString();
    homeConnect->getProgramsAvailable(haid);
    connect(homeConnect, &HomeConnect::receivedAvailablePrograms, result, [result, this] (const QString &haid, const QStringList &programs) {
        if (result->thing()->paramValue(m_idParamTypeIds.value(result->thing()->thingClassId())).toString() == haid) {
            if (programs.contains(result->item().id()))
                result->finish(Thing::ThingErrorNoError);
        }
    });
}

void IntegrationPluginHomeConnect::executeBrowserItem(BrowserActionInfo *info)
{
    Q_UNUSED(info)
    Thing *thing = info->thing();
    qCDebug(dcHomeConnect()) << "Execute browse item called " << thing->name();

    HomeConnect *homeConnect = m_homeConnectConnections.value(myThings().findById(thing->parentId()));
    if (!homeConnect)
        return;
    QString haid = thing->stateValue(m_idParamTypeIds.value(thing->thingClassId())).toString();
    QList<HomeConnect::Option> options;
    //TODO add options like set temperature or start time
    homeConnect->startProgram(haid, info->browserAction().itemId(), options);
}

void IntegrationPluginHomeConnect::onConnectionChanged(bool connected)
{
    HomeConnect *homeConnect = static_cast<HomeConnect *>(sender());
    Thing *thing = m_homeConnectConnections.key(homeConnect);
    if (!thing)
        return;
    thing->setStateValue(homeConnectConnectionConnectedStateTypeId, connected);
    if (!connected) {
        Q_FOREACH(Thing *child, myThings().filterByParentId(thing->id())) {
            child->setStateValue(m_connectedStateTypeIds.value(child->thingClassId()), connected);
        }
    }
}

void IntegrationPluginHomeConnect::onAuthenticationStatusChanged(bool authenticated)
{
    HomeConnect *homeConnectConnection = static_cast<HomeConnect *>(sender());
    if (m_asyncSetup.contains(homeConnectConnection)) {
        ThingSetupInfo *info = m_asyncSetup.take(homeConnectConnection);
        if (authenticated) {
            m_homeConnectConnections.insert(info->thing(), homeConnectConnection);
            info->finish(Thing::ThingErrorNoError);
        } else {
            homeConnectConnection->deleteLater();
            info->finish(Thing::ThingErrorHardwareFailure);
        }
    } else {
        Thing *thing = m_homeConnectConnections.key(homeConnectConnection);
        if (!thing)
            return;

        thing->setStateValue(homeConnectConnectionLoggedInStateTypeId, authenticated);
        if (!authenticated) {
            //refresh access token needs to be refreshed
            pluginStorage()->beginGroup(thing->id().toString());
            QByteArray refreshToken = pluginStorage()->value("refresh_token").toByteArray();
            pluginStorage()->endGroup();
            homeConnectConnection->getAccessTokenFromRefreshToken(refreshToken);
        }
    }
}

void IntegrationPluginHomeConnect::onRequestExecuted(QUuid requestId, bool success)
{
    if (m_pendingActions.contains(requestId)) {
        ThingActionInfo *info = m_pendingActions.value(requestId);
        if (success) {
            info->finish(Thing::ThingErrorNoError);
        } else {
            info->finish(Thing::ThingErrorHardwareNotAvailable);
        }
    }
}

void IntegrationPluginHomeConnect::onReceivedHomeAppliances(const QList<HomeConnect::HomeAppliance> &appliances)
{
    HomeConnect *homeConnectConnection = static_cast<HomeConnect *>(sender());
    Thing *parentThing = m_homeConnectConnections.key(homeConnectConnection);
    if (!parentThing)
        return;

    ThingDescriptors desciptors;
    Q_FOREACH(HomeConnect::HomeAppliance appliance, appliances) {
        ThingClassId thingClassId;

        if (appliance.type.contains("Oven", Qt::CaseInsensitive)) {
            thingClassId = ovenThingClassId;
        } else if (appliance.type.contains("Dishwasher", Qt::CaseInsensitive)) {
            thingClassId = dishwasherThingClassId;
        } else if (appliance.type.contains("Washer", Qt::CaseInsensitive)) {
            thingClassId = washerThingClassId;
        } else if (appliance.type.contains("FidgeFreezer", Qt::CaseInsensitive)) {
            thingClassId = fridgeThingClassId;
        } else if (appliance.type.contains("Refrigerator", Qt::CaseInsensitive)) {
            thingClassId = fridgeThingClassId;
        } else if (appliance.type.contains("Freezer", Qt::CaseInsensitive)) {
            thingClassId = fridgeThingClassId;
        } else if (appliance.type.contains("WineCooler", Qt::CaseInsensitive)) {
            thingClassId = fridgeThingClassId;
        } else if (appliance.type.contains("CoffeeMaker", Qt::CaseInsensitive)) {
            thingClassId = coffeMakerThingClassId;
        } else if (appliance.type.contains("Dryer", Qt::CaseInsensitive)) {
            thingClassId = dryerThingClassId;
        } else if (appliance.type.contains("CookTop", Qt::CaseInsensitive)) {
            thingClassId = cookTopThingClassId;
        } else if (appliance.type.contains("Hood", Qt::CaseInsensitive)) {
            thingClassId = hoodThingClassId;
        } else if (appliance.type.contains("CleaningRobot", Qt::CaseInsensitive)) {
            thingClassId = cleaningRobotThingClassId;
        } else if (appliance.type.contains("CookProcessor", Qt::CaseInsensitive)) {
            thingClassId = cookProcessorThingClassId;
        } else if (appliance.type.contains("WasherDryer", Qt::CaseInsensitive)) {
            thingClassId = washerThingClassId;
            //FIXME add washerdryer thing classid
        } else {
            qCWarning(dcHomeConnect()) << "Unknown thing type" << appliance.type;
            continue;
        }

        if (!myThings().filterByParam(m_idParamTypeIds.value(thingClassId), appliance.homeApplianceId).isEmpty()) {
            Thing * existingThing = myThings().filterByParam(m_idParamTypeIds.value(thingClassId), appliance.homeApplianceId).first();
            existingThing->setStateValue(m_connectedStateTypeIds.value(thingClassId), appliance.connected);
            continue;
        }

        ThingDescriptor descriptor(thingClassId, appliance.name, appliance.brand+" "+appliance.vib, parentThing->id());

        ParamList params;
        params << Param(m_idParamTypeIds.value(thingClassId), appliance.homeApplianceId);
        descriptor.setParams(params);
        desciptors.append(descriptor);
    }
    if (!desciptors.isEmpty())
        emit autoThingsAppeared(desciptors);
}

void IntegrationPluginHomeConnect::onReceivedStatusList(const QString &haId, const QHash<QString, QVariant> &statusList)
{
    HomeConnect *homeConnectConnection = static_cast<HomeConnect *>(sender());
    Thing *parentThing = m_homeConnectConnections.key(homeConnectConnection);
    if (!parentThing)
        return;

    Q_FOREACH(Thing *thing, myThings().filterByParentId(parentThing->id())) {

        if (thing->paramValue(m_idParamTypeIds.value(thing->thingClassId())).toString() == haId) {

            if (statusList.contains("BSH.Common.Status.LocalControlActive")) {
                if (m_localControlStateTypeIds.contains(thing->thingClassId())) {
                    thing->setStateValue(m_localControlStateTypeIds.value(thing->thingClassId()), statusList.value("BSH.Common.Status.LocalControlActive").toBool());
                }
            }
            if (statusList.contains("BSH.Common.Status.RemoteControlActive")) {
                if (m_remoteControlActivationStateTypeIds.contains(thing->thingClassId())) {
                    thing->setStateValue(m_remoteControlActivationStateTypeIds.value(thing->thingClassId()), statusList.value("BSH.Common.Status.RemoteControlActive").toBool());
                }
            }
            if (statusList.contains("BSH.Common.Status.RemoteControlStartAllowed")) {
                if (m_remoteStartAllowanceStateTypeIds.contains(thing->thingClassId())) {
                    thing->setStateValue(m_remoteStartAllowanceStateTypeIds.value(thing->thingClassId()), statusList.value("BSH.Common.Status.RemoteControlStartAllowed").toBool());
                }
            }
            if (statusList.contains("BSH.Common.Status.DoorState")) {
                if (m_doorStateTypeIds.contains(thing->thingClassId())) {
                    thing->setStateValue(m_doorStateTypeIds.value(thing->thingClassId()), statusList.value("BSH.Common.Status.DoorState").toString().split('.').last());
                }
            }
            if (statusList.contains("BSH.Common.Status.OperationState")) {
                if (m_operationStateTypeIds.contains(thing->thingClassId())) {
                    thing->setStateValue(m_operationStateTypeIds.value(thing->thingClassId()), statusList.value("BSH.Common.Status.OperationState").toString().split('.').last());
                }
            }
            break;
        }
    }
}

void IntegrationPluginHomeConnect::onReceivedEvents(const QList<HomeConnect::Event> &events)
{
    Q_FOREACH(HomeConnect::Event event, events) {
        qCDebug(dcHomeConnect()) << "Received event" << event.key << event.uri << event.name;

        if (event.key == "BSH.Common.Root.SelectedProgram") {

        }
        if (event.key == "BSH.Common.Root.SelectedProgram") {

        }
        if (event.key == "BSH.Common.Option.ProgramProgress") {

        }
    /*
     * BSH.Common.Root.SelectedProgram
     * BSH.Common.Root.ActiveProgram
     * Cooking.Oven.Option.SetpointTemperature
     * BSH.Common.Option.Duration
     * Cooking.Oven.Option.FastPreHeat
     * BSH.Common.Option.StartInRelative
     * LaundryCare.Washer.Option.Temperature
     * LaundryCare.Washer.Option.SpinSpeed
     * LaundryCare.Dryer.Option.DryingTarget
     * ConsumerProducts.CoffeeMaker.Option.BeanAmount
     * ConsumerProducts.CoffeeMaker.Option.FillQuantity
     * ConsumerProducts.CoffeeMaker.Option.CoffeeTemperature
     * Cooking.Common.Option.Hood.VentingLevel
     * Cooking.Common.Option.Hood.IntensiveLevel
     * ConsumerProducts.CleaningRobot.Option.ReferenceMapId
     * ConsumerProducts.CleaningRobot.Option.CleaningMode
     * BSH.Common.Option.ElapsedProgramTime
     * BSH.Common.Option.RemainingProgramTime
     * BSH.Common.Option.ProgramProgress
     * ConsumerProducts.CleaningRobot.Option.ProcessPhase
     * BSH.Common.Setting.PowerState
     * BSH.Common.Setting.TemperatureUnit
     * BSH.Common.Setting.LiquidVolumeUnit
     * Cooking.Common.Setting.Lighting
     * Cooking.Common.Setting.LightingBrightness
     * BSH.Common.Setting.AmbientLightEnabled
     * BSH.Common.Setting.AmbientLightBrightness
     * BSH.Common.Setting.AmbientLightColor
     * BSH.Common.Setting.AmbientLightCustomColor
     * Refrigeration.FridgeFreezer.Setting.SetpointTemperatureFreezer
     * Refrigeration.FridgeFreezer.Setting.SetpointTemperatureRefrigerator
     * Refrigeration.Common.Setting.BottleCooler.SetpointTemperature
     * Refrigeration.Common.Setting.ChillerLeft.SetpointTemperature
     * Refrigeration.Common.Setting.ChillerCommon.SetpointTemperature
Refrigeration.Common.Setting.ChillerRight.SetpointTemperature 	NOTIFY
Refrigeration.Common.Setting.WineCompartment.SetpointTemperature 	NOTIFY
Refrigeration.Common.Setting.WineCompartment2.SetpointTemperature 	NOTIFY
Refrigeration.Common.Setting.WineCompartment3.SetpointTemperature 	NOTIFY
Refrigeration.FridgeFreezer.Setting.SuperModeFreezer 	NOTIFY
Refrigeration.FridgeFreezer.Setting.SuperModeRefrigerator 	NOTIFY
Refrigeration.Common.Setting.EcoMode 	NOTIFY
Refrigeration.Common.Setting.SabbathMode 	NOTIFY
Refrigeration.Common.Setting.VacationMode 	NOTIFY
Refrigeration.Common.Setting.FreshMode 	NOTIFY
ConsumerProducts.CleaningRobot.Setting.CurrentMap 	NOTIFY
ConsumerProducts.CleaningRobot.Setting.NameOfMap1 	NOTIFY
ConsumerProducts.CleaningRobot.Setting.NameOfMap2 	NOTIFY
ConsumerProducts.CleaningRobot.Setting.NameOfMap3 	NOTIFY
ConsumerProducts.CleaningRobot.Setting.NameOfMap4 	NOTIFY
ConsumerProducts.CleaningRobot.Setting.NameOfMap5 	NOTIFY
BSH.Common.Status.RemoteControlActive 	STATUS
BSH.Common.Status.RemoteControlStartAllowed 	STATUS
BSH.Common.Status.LocalControlActive 	STATUS
BSH.Common.Status.OperationState 	STATUS
BSH.Common.Status.DoorState 	STATUS
BSH.Common.Status.BatteryLevel 	STATUS
BSH.Common.Status.BatteryChargingState 	STATUS
BSH.Common.Status.ChargingConnection 	STATUS
BSH.Common.Status.Video.CameraState 	STATUS
ConsumerProducts.CleaningRobot.Status.LastSelectedMap 	STATUS
ConsumerProducts.CleaningRobot.Status.DustBoxInserted 	STATUS
ConsumerProducts.CleaningRobot.Status.Lost 	STATUS
ConsumerProducts.CleaningRobot.Status.Lifted 	STATUS
BSH.Common.Event.ProgramAborted
BSH.Common.Event.ProgramFinished
BSH.Common.Event.AlarmClockElapsed 	EVENT
Cooking.Oven.Event.PreheatFinished 	EVENT
ConsumerProducts.CoffeeMaker.Event.BeanContainerEmpty 	EVENT
ConsumerProducts.CoffeeMaker.Event.WaterTankEmpty 	EVENT
ConsumerProducts.CoffeeMaker.Event.DripTrayFull 	EVENT
Refrigeration.FridgeFreezer.Event.DoorAlarmFreezer 	EVENT
Refrigeration.FridgeFreezer.Event.DoorAlarmRefrigerator 	EVENT
Refrigeration.FridgeFreezer.Event.TemperatureAlarmFreezer 	EVENT
ConsumerProducts.CleaningRobot.Event.EmptyDustBoxAndCleanFilter 	EVENT
ConsumerProducts.CleaningRobot.Event.RobotIsStuck
ConsumerProducts.CleaningRobot.Event.DockingStationNotFound
*/
    }
}

void IntegrationPluginHomeConnect::onReceivedActiveProgram(const QString &haId, const QString &key, const QHash<QString, QVariant> &options)
{
    HomeConnect *homeConnectConnection = static_cast<HomeConnect *>(sender());
    Thing *parentThing = m_homeConnectConnections.key(homeConnectConnection);
    if (!parentThing)
        return;

    Q_FOREACH(Thing *thing, myThings().filterByParentId(parentThing->id())) {
        if (thing->paramValue(m_idParamTypeIds.value(thing->thingClassId())).toString() == haId) {
            qCDebug(dcHomeConnect()) << "Received active program" << thing->name() << key << options;
            if (thing->thingClassId() == ovenThingClassId) {
                if (key.contains("Cooking.Oven.Program.HeatingMode")) {
                    thing->setStateValue(ovenActiveProgramStateTypeId, key.split('.').last());
                    thing->setStateValue(ovenTargetTemperatureStateTypeId, options.value("Cooking.Oven.Option.SetpointTemperature").toInt());
                    thing->setStateValue(ovenTargetTemperatureStateTypeId, options.value("BSH.Common.Option.Duration").toInt());
                }
            }
        }
        break;
    }
}

void IntegrationPluginHomeConnect::onReceivedSelectedProgram(const QString &haId, const QString &key, const QHash<QString, QVariant> &options)
{
    HomeConnect *homeConnectConnection = static_cast<HomeConnect *>(sender());
    Thing *parentThing = m_homeConnectConnections.key(homeConnectConnection);
    if (!parentThing)
        return;

    Q_FOREACH(Thing *thing, myThings().filterByParentId(parentThing->id())) {
        if (thing->paramValue(m_idParamTypeIds.value(thing->thingClassId())).toString() == haId) {
            qCDebug(dcHomeConnect()) << "Received active program" << thing->name() << key << options;
            if (thing->thingClassId() == ovenThingClassId) {
                if (key.contains("Cooking.Oven.Program.HeatingMode")) {
                    thing->setStateValue(ovenSelectedProgramStateTypeId, key.split('.').last());
                    thing->setStateValue(ovenTargetTemperatureStateTypeId, options.value("Cooking.Oven.Option.SetpointTemperature").toInt());
                    thing->setStateValue(ovenTargetTemperatureStateTypeId, options.value("BSH.Common.Option.Duration").toInt());
                }
            }
        }
        break;
    }
}

void IntegrationPluginHomeConnect::onReceivedSettings(const QString &haId, const QHash<QString, QVariant> &settings)
{
    HomeConnect *homeConnectConnection = static_cast<HomeConnect *>(sender());
    Thing *parentThing = m_homeConnectConnections.key(homeConnectConnection);
    if (!parentThing)
        return;

    Q_FOREACH(Thing *thing, myThings().filterByParentId(parentThing->id())) {
        if (thing->paramValue(m_idParamTypeIds.value(thing->thingClassId())).toString() == haId) {
            qCDebug(dcHomeConnect()) << "Received setting" << thing->name() << settings;
            if (settings.contains("BSH.Common.Setting.PowerState")) {
                //TODO set power state
            }
            //BSH.Common.Setting.TemperatureUnit
            //BSH.Common.Setting.LiquidVolumeUnit
            //Refrigeration.FridgeFreezer.Setting.SetpointTemperatureRefrigerator
            //Refrigeration.FridgeFreezer.Setting.SetpointTemperatureFreezer
            //Refrigeration.Common.Setting.BottleCooler.SetpointTemperature
            //Refrigeration.Common.Setting.ChillerLeft.SetpointTemperature
            //Refrigeration.Common.Setting.ChillerCommon.SetpointTemperature
            //Refrigeration.Common.Setting.ChillerRight.SetpointTemperature
            //Refrigeration.Common.Setting.WineCompartment.SetpointTemperature
            //Refrigeration.Common.Setting.WineCompartment2.SetpointTemperature
            //Refrigeration.Common.Setting.WineCompartment3.SetpointTemperature
            //Refrigeration.FridgeFreezer.Setting.SuperModeRefrigerator
            //Refrigeration.FridgeFreezer.Setting.SuperModeFreezer
            //Refrigeration.Common.Setting.EcoMode
            //Refrigeration.Common.Setting.SabbathMode
            //Refrigeration.Common.Setting.VacationMode
            //Refrigeration.Common.Setting.FreshMode
            //Cooking.Common.Setting.Lighting
            //Cooking.Common.Setting.LightingBrightness
            //BSH.Common.Setting.AmbientLightEnabled
            //BSH.Common.Setting.AmbientLightBrightness
            //BSH.Common.Setting.AmbientLightColor
            //BSH.Common.Setting.AmbientLightCustomColor
        }
        break;
    }
}
