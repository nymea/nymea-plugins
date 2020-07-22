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
#include <QDateTime>

IntegrationPluginHomeConnect::IntegrationPluginHomeConnect()
{
    m_idParamTypeIds.insert(ovenThingClassId, ovenThingIdParamTypeId);
    m_idParamTypeIds.insert(fridgeThingClassId, fridgeThingIdParamTypeId);
    m_idParamTypeIds.insert(dryerThingClassId, dryerThingIdParamTypeId);
    m_idParamTypeIds.insert(coffeeMakerThingClassId, coffeeMakerThingIdParamTypeId);
    m_idParamTypeIds.insert(dishwasherThingClassId, dishwasherThingIdParamTypeId);
    m_idParamTypeIds.insert(washerThingClassId, washerThingIdParamTypeId);
    m_idParamTypeIds.insert(cookTopThingClassId, cookTopThingIdParamTypeId);
    m_idParamTypeIds.insert(hoodThingClassId, hoodThingIdParamTypeId);
    m_idParamTypeIds.insert(cleaningRobotThingClassId, cleaningRobotThingIdParamTypeId);

    m_connectedStateTypeIds.insert(ovenThingClassId, ovenConnectedStateTypeId);
    m_connectedStateTypeIds.insert(fridgeThingClassId, fridgeConnectedStateTypeId);
    m_connectedStateTypeIds.insert(dryerThingClassId, dryerConnectedStateTypeId);
    m_connectedStateTypeIds.insert(coffeeMakerThingClassId, coffeeMakerConnectedStateTypeId);
    m_connectedStateTypeIds.insert(dishwasherThingClassId, dishwasherConnectedStateTypeId);
    m_connectedStateTypeIds.insert(washerThingClassId, washerConnectedStateTypeId);
    m_connectedStateTypeIds.insert(cookTopThingClassId, cookTopConnectedStateTypeId);
    m_connectedStateTypeIds.insert(cleaningRobotThingClassId, cleaningRobotConnectedStateTypeId);
    m_connectedStateTypeIds.insert(hoodThingClassId, hoodConnectedStateTypeId);

    m_localControlStateTypeIds.insert(ovenThingClassId, ovenLocalControlStateStateTypeId);
    //m_localControlStateTypeIds.insert(fridgeThingClassId, fridgeLocalControlStateStateTypeId);
    //m_localControlStateTypeIds.insert(dryerThingClassId, dryerLocalControlStateTypeId);
    m_localControlStateTypeIds.insert(coffeeMakerThingClassId, coffeeMakerLocalControlStateStateTypeId);
    //m_localControlStateTypeIds.insert(dishwasherThingClassId, dishwasherLocalControlStateStateTypeId);
    m_localControlStateTypeIds.insert(washerThingClassId, washerLocalControlStateStateTypeId);
    //m_localControlStateTypeIds.insert(cookTopThingClassId, cookTopLocalControlStateStateTypeId);
    //m_localControlStateTypeIds.insert(cleaningRobotThingClassId, cleaningRobotLocalControlStateStateTypeId);
    //m_localControlStateTypeIds.insert(hoodThingClassId, hoodLocalControlStateStateTypeId);

    m_remoteStartAllowanceStateTypeIds.insert(ovenThingClassId, ovenRemoteStartAllowanceStateStateTypeId);
    //m_remoteStartAllowanceStateTypeIds.insert(fridgeThingClassId, fridgeRemoteStartAllowanceStateStateTypeId);
    m_remoteStartAllowanceStateTypeIds.insert(dryerThingClassId, dryerRemoteStartAllowanceStateStateTypeId);
    m_remoteStartAllowanceStateTypeIds.insert(coffeeMakerThingClassId, coffeeMakerRemoteStartAllowanceStateStateTypeId);
    m_remoteStartAllowanceStateTypeIds.insert(dishwasherThingClassId, dishwasherRemoteStartAllowanceStateStateTypeId);
    m_remoteStartAllowanceStateTypeIds.insert(washerThingClassId, washerRemoteStartAllowanceStateStateTypeId);
    //m_remoteStartAllowanceStateTypeIds.insert(cookTopThingClassId, cookTopRemoteStartAllowanceStateStateTypeId);
    //m_remoteStartAllowanceStateTypeIds.insert(cleaningRobotThingClassId, cleaningRobotRemoteStartAllowanceStateStateTypeId);
    //m_remoteStartAllowanceStateTypeIds.insert(hoodThingClassId, hoodRemoteStartAllowanceStateStateTypeId);

    m_remoteControlActivationStateTypeIds.insert(ovenThingClassId, ovenRemoteControlActivationStateStateTypeId);
    //m_remoteControlActivationStateTypeIds.insert(fridgeThingClassId, fridgeRemoteControlActivationStateStateTypeId);
    m_remoteControlActivationStateTypeIds.insert(dryerThingClassId, dryerRemoteControlActivationStateStateTypeId);
    //m_remoteControlActivationStateTypeIds.insert(coffeeMakerThingClassId, coffeeMakerRemoteControlActivationStateStateTypeId);
    m_remoteControlActivationStateTypeIds.insert(dishwasherThingClassId, dishwasherRemoteControlActivationStateStateTypeId);
    m_remoteControlActivationStateTypeIds.insert(washerThingClassId, washerRemoteControlActivationStateStateTypeId);
    //m_remoteControlActivationStateTypeIds.insert(cookTopThingClassId, cookTopRemoteControlActivationStateStateTypeId);
    //m_remoteControlActivationStateTypeIds.insert(cleaningRobotThingClassId, cleaningRobotRemoteControlActivationStateStateTypeId);
    //m_remoteControlActivationStateTypeIds.insert(hoodThingClassId, hoodRemoteControlActivationStateStateTypeId);

    m_doorStateTypeIds.insert(dishwasherThingClassId, dishwasherDoorStateStateTypeId);
    m_doorStateTypeIds.insert(washerThingClassId, washerDoorStateStateTypeId);
    m_doorStateTypeIds.insert(dryerThingClassId, dryerDoorStateStateTypeId);
    m_doorStateTypeIds.insert(ovenThingClassId, ovenDoorStateStateTypeId);

    m_operationStateTypeIds.insert(ovenThingClassId, ovenOperationStateStateTypeId);
    m_operationStateTypeIds.insert(dryerThingClassId, dryerOperationStateStateTypeId);
    m_operationStateTypeIds.insert(coffeeMakerThingClassId, coffeeMakerOperationStateStateTypeId);
    m_operationStateTypeIds.insert(dishwasherThingClassId, dishwasherOperationStateStateTypeId);
    m_operationStateTypeIds.insert(washerThingClassId, washerOperationStateStateTypeId);

    m_selectedProgramStateTypeIds.insert(ovenThingClassId, ovenSelectedProgramStateTypeId);
    m_selectedProgramStateTypeIds.insert(dryerThingClassId, dryerSelectedProgramStateTypeId);
    m_selectedProgramStateTypeIds.insert(coffeeMakerThingClassId, coffeeMakerSelectedProgramStateTypeId);
    m_selectedProgramStateTypeIds.insert(dishwasherThingClassId, dishwasherSelectedProgramStateTypeId);
    m_selectedProgramStateTypeIds.insert(washerThingClassId, washerSelectedProgramStateTypeId);

    m_progressStateTypeIds.insert(ovenThingClassId, ovenProgressStateTypeId);
    m_progressStateTypeIds.insert(dryerThingClassId, dryerProgressStateTypeId);
    m_progressStateTypeIds.insert(dishwasherThingClassId, dishwasherProgressStateTypeId);
    m_progressStateTypeIds.insert(washerThingClassId, washerProgressStateTypeId);
    m_progressStateTypeIds.insert(coffeeMakerThingClassId, coffeeMakerProgressStateTypeId);

    m_endTimerStateTypeIds.insert(ovenThingClassId, ovenEndTimeStateTypeId);

    m_pauseActionTypeIds.insert(ovenThingClassId, ovenPauseActionTypeId);
    m_resumeActionTypeIds.insert(ovenThingClassId, ovenResumeActionTypeId);

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
        connect(homeConnect, &HomeConnect::receivedSelectedProgram, this, &IntegrationPluginHomeConnect::onReceivedSelectedProgram);
        connect(homeConnect, &HomeConnect::receivedSettings, this, &IntegrationPluginHomeConnect::onReceivedSettings);
        connect(homeConnect, &HomeConnect::receivedEvents, this, &IntegrationPluginHomeConnect::onReceivedEvents);

    } else if ((thing->thingClassId() == dryerThingClassId) ||
               (thing->thingClassId() == fridgeThingClassId) ||
               (thing->thingClassId() == washerThingClassId) ||
               (thing->thingClassId() == dishwasherThingClassId) ||
               (thing->thingClassId() == coffeeMakerThingClassId) ||
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
    /*  if (!m_pluginTimer5sec) {
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
    }*/

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
                    QString haId = childThing->paramValue(m_idParamTypeIds.value(childThing->thingClassId())).toString();
                    homeConnect->getStatus(haId);
                    homeConnect->getSettings(haId);
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
               (thing->thingClassId() == coffeeMakerThingClassId) ||
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
    HomeConnect *homeConnect = m_homeConnectConnections.value(myThings().findById(thing->parentId()));
    if (!homeConnect) {
        return info->finish(Thing::ThingErrorHardwareNotAvailable);
    }
    QString haid = thing->paramValue(m_idParamTypeIds.value(thing->thingClassId())).toString();

    if (m_pauseActionTypeIds.values().contains(action.actionTypeId())) {
        QUuid requestId;
        requestId = homeConnect->sendCommand(haid, "BSH.Common.Command.PauseProgram");
        m_pendingActions.insert(requestId, info);
        connect(info, &ThingActionInfo::aborted, [requestId, this] {
            m_pendingActions.remove(requestId);
        });
    } else if (m_resumeActionTypeIds.values().contains(action.actionTypeId())) {
        QUuid requestId;
        requestId = homeConnect->sendCommand(haid, "BSH.Common.Command.ResumeProgram");
        m_pendingActions.insert(requestId, info);
        connect(info, &ThingActionInfo::aborted, [requestId, this] {
            m_pendingActions.remove(requestId);
        });
    } else if (thing->thingClassId() == ovenThingClassId) {
        //set temperature
    } else if (thing->thingClassId() == coffeeMakerThingClassId) {

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
    QString haid = thing->paramValue(m_idParamTypeIds.value(thing->thingClassId())).toString();
    homeConnect->getProgramsAvailable(haid);
    connect(homeConnect, &HomeConnect::receivedAvailablePrograms, result, [result, this] (const QString &haId, const QStringList programs) {
        if(result->thing()->paramValue(m_idParamTypeIds.value(result->thing()->thingClassId())).toString() == haId) {
            Q_FOREACH(QString program, programs) {
                BrowserItem item;
                item.setExecutable(true);
                item.setDisplayName(program.split('.').last());
                item.setId(program);
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
    QString haid = thing->paramValue(m_idParamTypeIds.value(thing->thingClassId())).toString();
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
    QString haid = thing->paramValue(m_idParamTypeIds.value(thing->thingClassId())).toString();
    QList<HomeConnect::Option> options;
    //TODO add options like set temperature or start time
    homeConnect->startProgram(haid, info->browserAction().itemId(), options);
}

void IntegrationPluginHomeConnect::parseKey(Thing *thing, const QString &key, const QVariant &value)
{
    qCDebug(dcHomeConnect()) << thing->name() << key.split('.').last() << value;
    // PROGRAM CHANGES
    if (key == "BSH.Common.Root.SelectedProgram") {
        if (m_selectedProgramStateTypeIds.contains(thing->thingClassId())) {
            thing->setStateValue(m_selectedProgramStateTypeIds.value(thing->thingClassId()), value.toString().split('.').last());
        }
    } else if (key == "BSH.Common.Root.ActiveProgram") {

        // Option Changes
    } else if (key == "Cooking.Oven.Option.SetpointTemperature") {
        thing->setStateValue(ovenTargetTemperatureStateTypeId, value);
    } else if (key == "BSH.Common.Option.Duration") {
        thing->setStateValue(ovenDurationStateTypeId, value);
    } else if (key == "Cooking.Oven.Option.FastPreHeat") {
    } else if (key == "BSH.Common.Option.StartInRelative") {
    } else if (key == "LaundryCare.Washer.Option.Temperature") {
        thing->setStateValue(washerTemperatureStateTypeId, value);
    } else if (key == "LaundryCare.Washer.Option.SpinSpeed") {
        thing->setStateValue(washerSpinSpeedStateTypeId, value);
    } else if (key == "LaundryCare.Dryer.Option.DryingTarget") {
    } else if (key == "ConsumerProducts.CoffeeMaker.Option.BeanAmount") {

    } else if (key == "ConsumerProducts.CoffeeMaker.Option.FillQuantity") {
    } else if (key == "ConsumerProducts.CoffeeMaker.Option.CoffeeTemperature") {
    } else if (key == "Cooking.Common.Option.Hood.VentingLevel") {
    } else if (key == "Cooking.Common.Option.Hood.IntensiveLevel") {
    } else if (key == "ConsumerProducts.CleaningRobot.Option.ReferenceMapId") {
    } else if (key == "ConsumerProducts.CleaningRobot.Option.CleaningMode") {

        // Program Progress Changes
    } else if (key == "BSH.Common.Option.ElapsedProgramTime") {
    } else if (key == "BSH.Common.Option.RemainingProgramTime") {
        QString time = QDateTime::fromMSecsSinceEpoch(QDateTime::currentMSecsSinceEpoch()+(value.toInt()*1000)).time().toString();
        thing->setStateValue(m_endTimerStateTypeIds.value(thing->thingClassId()), time);
    } else if (key == "BSH.Common.Option.ProgramProgress") {
        if (m_progressStateTypeIds.contains(thing->thingClassId())) {
            thing->setStateValue(m_progressStateTypeIds.value(thing->thingClassId()), value);
        }
    } else if (key == "ConsumerProducts.CleaningRobot.Option.ProcessPhase") {
    } else if (key == "BSH.Common.Status.OperationState") {
        if (m_operationStateTypeIds.contains(thing->thingClassId())) {
            thing->setStateValue(m_operationStateTypeIds.value(thing->thingClassId()), value.toString().split('.').last());
        }
        // Program Progress Events
    } else if (key == "BSH.Common.Event.ProgramAborted") {
    } else if (key == "BSH.Common.Event.ProgramFinished") {
    } else if (key == "BSH.Common.Event.AlarmClockElapsed") {
    } else if (key == "Cooking.Oven.Event.PreheatFinished") {

        // Home Appliance State Changes
    } else if (key == "BSH.Common.Setting.PowerState") {
    } else if (key == "BSH.Common.Status.RemoteControlActive") {
        if (m_remoteControlActivationStateTypeIds.contains(thing->thingClassId())) {
            thing->setStateValue(m_remoteControlActivationStateTypeIds.value(thing->thingClassId()), value);
        }
    } else if (key == "BSH.Common.Status.RemoteControlStartAllowed") {
        if (m_remoteStartAllowanceStateTypeIds.contains(thing->thingClassId())) {
            thing->setStateValue(m_remoteStartAllowanceStateTypeIds.value(thing->thingClassId()), value);
        }
    } else if (key == "BSH.Common.Status.LocalControlActive") {
        if (m_localControlStateTypeIds.contains(thing->thingClassId())) {
            thing->setStateValue(m_localControlStateTypeIds.value(thing->thingClassId()), value);
        }
    } else if (key == "BSH.Common.Status.DoorState") {
        if (m_doorStateTypeIds.contains(thing->thingClassId())) {
            thing->setStateValue(m_doorStateTypeIds.value(thing->thingClassId()), value.toString().split('.').last());
        }

        // Home Appliance Events
    } else if (key == "ConsumerProducts.CoffeeMaker.Event.BeanContainerEmpty") {
    } else if (key == "ConsumerProducts.CoffeeMaker.Event.WaterTankEmpty") {
    } else if (key == "ConsumerProducts.CoffeeMaker.Event.DripTrayFull") {
    } else if (key == "Refrigeration.FridgeFreezer.Event.DoorAlarmFreezer") {
    } else if (key == "Refrigeration.FridgeFreezer.Event.DoorAlarmRefrigerator") {
    } else if (key == "Refrigeration.FridgeFreezer.Event.TemperatureAlarmFreezer") {
    } else if (key == "ConsumerProducts.CleaningRobot.Event.EmptyDustBoxAndCleanFilter") {
    } else if (key == "ConsumerProducts.CleaningRobot.Event.RobotIsStuck") {
    } else if (key == "ConsumerProducts.CleaningRobot.Event.DockingStationNotFound") {
        // UNDOKUMENTED
    } else if (key == "Cooking.Oven.Status.CurrentCavityTemperature") {
        thing->setStateValue(ovenCurrentTemperatureStateTypeId, value);
    } else {
        qCWarning(dcHomeConnect()) << "Parse key: unknown key" << key;
    }
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
            thingClassId = coffeeMakerThingClassId;
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
            Q_FOREACH(QString key, statusList.keys()) {
                parseKey(thing, key, statusList.value(key));
            }
            break;
        }
    }
}

void IntegrationPluginHomeConnect::onReceivedEvents(HomeConnect::EventType eventType, const QString &haId, const QList<HomeConnect::Event> &events)
{

    HomeConnect *homeConnectConnection = static_cast<HomeConnect *>(sender());
    Thing *parentThing = m_homeConnectConnections.key(homeConnectConnection);
    if (!parentThing)
        return;

    Q_FOREACH(Thing *thing, myThings().filterByParentId(parentThing->id())) {

        if (thing->paramValue(m_idParamTypeIds.value(thing->thingClassId())).toString() == haId) {

            switch (eventType) {
            case HomeConnect::EventTypeKeepAlive:
                break;
            case HomeConnect::EventTypeConnected:
                thing->setStateValue(m_connectedStateTypeIds.value(thing->thingClassId()), true);
                break;
            case HomeConnect::EventTypeDisconnected:
                thing->setStateValue(m_connectedStateTypeIds.value(thing->thingClassId()), false);
                break;
            case HomeConnect::EventTypeStatus:
            case HomeConnect::EventTypeEvent:
            case HomeConnect::EventTypeNotify: {
                Q_FOREACH(HomeConnect::Event event, events) {
                    parseKey(thing, event.key, event.value);
                }
            } break;
            case HomeConnect::EventTypePaired: {
                //TODO add device
            } break;
            case HomeConnect::EventTypeDepaired: {
                //TODO remove device
            } break;
            }
            break;
        }
    }
}

void IntegrationPluginHomeConnect::onReceivedSelectedProgram(const QString &haId, const QString &key, const QHash<QString, QVariant> &options)
{
    Q_UNUSED(options)
    HomeConnect *homeConnectConnection = static_cast<HomeConnect *>(sender());
    Thing *parentThing = m_homeConnectConnections.key(homeConnectConnection);
    if (!parentThing)
        return;

    Q_FOREACH(Thing *thing, myThings().filterByParentId(parentThing->id())) {
        if (thing->paramValue(m_idParamTypeIds.value(thing->thingClassId())).toString() == haId) {
            if (thing->thingClassId() == ovenThingClassId) {
                if (key.contains("Cooking.Oven.Program.HeatingMode")) {
                    thing->setStateValue(ovenSelectedProgramStateTypeId, key.split('.').last());
                }
            } else if (thing->thingClassId() == washerThingClassId) {
                if (key.contains("LaundryCare.Washer.Program")) {
                    thing->setStateValue(washerSelectedProgramStateTypeId, key.split('.').last());
                }
            } else if (thing->thingClassId() == dishwasherThingClassId) {
                if (key.contains("Dishcare.Dishwasher.Program")) {
                    thing->setStateValue(dishwasherSelectedProgramStateTypeId, key.split('.').last());
                }
            } else if (thing->thingClassId() == dryerThingClassId) {
                if (key.contains("LaundryCare.Dryer.Program")) {
                    thing->setStateValue(dryerSelectedProgramStateTypeId, key.split('.').last());
                }
            } else if (thing->thingClassId() == coffeeMakerThingClassId) {
                if (key.contains("ConsumerProducts.CoffeeMaker.Program")) {
                    thing->setStateValue(coffeeMakerSelectedProgramStateTypeId, key.split('.').last());
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
