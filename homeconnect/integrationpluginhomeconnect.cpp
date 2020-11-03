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
    m_localControlStateTypeIds.insert(dryerThingClassId, dryerLocalControlStateStateTypeId);
    m_localControlStateTypeIds.insert(coffeeMakerThingClassId, coffeeMakerLocalControlStateStateTypeId);
    m_localControlStateTypeIds.insert(washerThingClassId, washerLocalControlStateStateTypeId);

    m_remoteStartAllowanceStateTypeIds.insert(ovenThingClassId, ovenRemoteStartAllowanceStateStateTypeId);
    m_remoteStartAllowanceStateTypeIds.insert(dryerThingClassId, dryerRemoteStartAllowanceStateStateTypeId);
    m_remoteStartAllowanceStateTypeIds.insert(coffeeMakerThingClassId, coffeeMakerRemoteStartAllowanceStateStateTypeId);
    m_remoteStartAllowanceStateTypeIds.insert(dishwasherThingClassId, dishwasherRemoteStartAllowanceStateStateTypeId);
    m_remoteStartAllowanceStateTypeIds.insert(washerThingClassId, washerRemoteStartAllowanceStateStateTypeId);

    m_remoteControlActivationStateTypeIds.insert(ovenThingClassId, ovenRemoteControlActivationStateStateTypeId);
    m_remoteControlActivationStateTypeIds.insert(dryerThingClassId, dryerRemoteControlActivationStateStateTypeId);
    m_remoteControlActivationStateTypeIds.insert(dishwasherThingClassId, dishwasherRemoteControlActivationStateStateTypeId);
    m_remoteControlActivationStateTypeIds.insert(washerThingClassId, washerRemoteControlActivationStateStateTypeId);

    m_doorStateTypeIds.insert(dishwasherThingClassId, dishwasherClosedStateTypeId);
    m_doorStateTypeIds.insert(washerThingClassId, washerClosedStateTypeId);
    m_doorStateTypeIds.insert(dryerThingClassId, dryerClosedStateTypeId);
    m_doorStateTypeIds.insert(ovenThingClassId, ovenClosedStateTypeId);
    m_doorStateTypeIds.insert(coffeeMakerThingClassId, coffeeMakerClosedStateTypeId);
    m_doorStateTypeIds.insert(fridgeThingClassId, fridgeClosedStateTypeId);

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
    m_endTimerStateTypeIds.insert(washerThingClassId, washerEndTimeStateTypeId);
    m_endTimerStateTypeIds.insert(dryerThingClassId, dryerEndTimeStateTypeId);
    m_endTimerStateTypeIds.insert(dishwasherThingClassId, dishwasherEndTimeStateTypeId);

    m_startActionTypeIds.insert(washerThingClassId, washerStartActionTypeId);
    m_startActionTypeIds.insert(dryerThingClassId, dryerStartActionTypeId);
    m_startActionTypeIds.insert(dishwasherThingClassId, dishwasherStartActionTypeId);
    m_startActionTypeIds.insert(coffeeMakerThingClassId, coffeeMakerStartActionTypeId);

    m_stopActionTypeIds.insert(washerThingClassId, washerStopActionTypeId);
    m_stopActionTypeIds.insert(dryerThingClassId, dryerStopActionTypeId);
    m_stopActionTypeIds.insert(dishwasherThingClassId, dishwasherStopActionTypeId);
    m_stopActionTypeIds.insert(coffeeMakerThingClassId, coffeeMakerStopActionTypeId);

    m_programFinishedEventTypeIds.insert(ovenThingClassId, ovenProgramFinishedEventTypeId);
    m_programFinishedEventTypeIds.insert(dryerThingClassId, dryerProgramFinishedEventTypeId);
    m_programFinishedEventTypeIds.insert(cookTopThingClassId, cookTopProgramFinishedEventTypeId);
    m_programFinishedEventTypeIds.insert(coffeeMakerThingClassId, coffeeMakerProgramFinishedEventTypeId);
    m_programFinishedEventTypeIds.insert(hoodThingClassId, hoodProgramFinishedEventTypeId);
    m_programFinishedEventTypeIds.insert(washerThingClassId, washerProgramFinishedEventTypeId);
    m_programFinishedEventTypeIds.insert(dishwasherThingClassId, dishwasherProgramFinishedEventTypeId);
    m_programFinishedEventTypeIds.insert(cleaningRobotThingClassId, cleaningRobotProgramFinishedEventTypeId);

    m_coffeeStrengthTypes.insert("ConsumerProducts.CoffeeMaker.EnumType.BeanAmount.VeryMild", "Very mild");
    m_coffeeStrengthTypes.insert("ConsumerProducts.CoffeeMaker.EnumType.BeanAmount.Mild", "Mild");
    m_coffeeStrengthTypes.insert("ConsumerProducts.CoffeeMaker.EnumType.BeanAmount.MildPlus", "Mild +");
    m_coffeeStrengthTypes.insert("ConsumerProducts.CoffeeMaker.EnumType.BeanAmount.Normal", "Normal");
    m_coffeeStrengthTypes.insert("ConsumerProducts.CoffeeMaker.EnumType.BeanAmount.NormalPlug", "Normal +");
    m_coffeeStrengthTypes.insert("ConsumerProducts.CoffeeMaker.EnumType.BeanAmount.Strong", "Strong");
    m_coffeeStrengthTypes.insert("ConsumerProducts.CoffeeMaker.EnumType.BeanAmount.StrongPlus", "Strong +");
    m_coffeeStrengthTypes.insert("ConsumerProducts.CoffeeMaker.EnumType.BeanAmount.VeryStrong", "Very strong");
    m_coffeeStrengthTypes.insert("ConsumerProducts.CoffeeMaker.EnumType.BeanAmount.VeryStrongPlus", "Very strong +");
    m_coffeeStrengthTypes.insert("ConsumerProducts.CoffeeMaker.EnumType.BeanAmount.ExtraStrong", "Extra strong");
    m_coffeeStrengthTypes.insert("ConsumerProducts.CoffeeMaker.EnumType.BeanAmount.DoubleShot", "Double shot");
    m_coffeeStrengthTypes.insert("ConsumerProducts.CoffeeMaker.EnumType.BeanAmount.DoubleShotPlus", "Double shot +");
    m_coffeeStrengthTypes.insert("ConsumerProducts.CoffeeMaker.EnumType.BeanAmount.DoubleShotPlusPlus", "Double shot ++");
    m_coffeeStrengthTypes.insert("ConsumerProducts.CoffeeMaker.EnumType.BeanAmount.TribleShot", "Trible shot");
    m_coffeeStrengthTypes.insert("ConsumerProducts.CoffeeMaker.EnumType.BeanAmount.TribleShotPlus", "Trible shot +");
    m_coffeeStrengthTypes.insert("ConsumerProducts.CoffeeMaker.EnumType.BeanAmount.CoffeeGround", "Coffee ground");
}

void IntegrationPluginHomeConnect::startPairing(ThingPairingInfo *info)
{
    if (info->thingClassId() == homeConnectAccountThingClassId) {

        bool simulationMode =  configValue(homeConnectPluginSimulationModeParamTypeId).toBool();
        bool controlEnabled =  configValue(homeConnectPluginControlEnabledParamTypeId).toBool();
        QByteArray clientKey = configValue(homeConnectPluginCustomClientKeyParamTypeId).toByteArray();
        QByteArray clientSecret = configValue(homeConnectPluginCustomClientSecretParamTypeId).toByteArray();
        if (clientKey.isEmpty() || clientSecret.isEmpty()) {
            clientKey = apiKeyStorage()->requestKey("homeconnect").data("clientKey");
            clientSecret = apiKeyStorage()->requestKey("homeconnect").data("clientSecret");
        }
        if (clientKey.isEmpty() || clientSecret.isEmpty()) {
            info->finish(Thing::ThingErrorAuthenticationFailure, tr("Client key and/or seceret is not available."));
            return;
        }
        HomeConnect *homeConnect = new HomeConnect(hardwareManager()->networkManager(), clientKey, clientSecret, simulationMode, this);
        QString scope = "IdentifyAppliance Monitor Settings Dishwasher Washer Dryer WasherDryer Refrigerator Freezer WineCooler CoffeeMaker Hood CookProcessor";
        if (controlEnabled)
            scope.append(" Control");
        QUrl url = homeConnect->getLoginUrl(QUrl("https://127.0.0.1:8888"), scope);
        qCDebug(dcHomeConnect()) << "HomeConnect url:" << url;
        m_setupHomeConnectConnections.insert(info->thingId(), homeConnect);
        info->setOAuthUrl(url);
        info->finish(Thing::ThingErrorNoError);
    } else {
        qCWarning(dcHomeConnect()) << "Unhandled pairing metod!";
        info->finish(Thing::ThingErrorCreationMethodNotSupported);
    }
}

void IntegrationPluginHomeConnect::confirmPairing(ThingPairingInfo *info, const QString &username, const QString &secret)
{
    Q_UNUSED(username);

    if (info->thingClassId() == homeConnectAccountThingClassId) {
        qCDebug(dcHomeConnect()) << "Redirect url is" << secret;
        QUrl url(secret);
        QUrlQuery query(url);
        QByteArray authorizationCode = query.queryItemValue("code").toLocal8Bit();

        HomeConnect *homeConnect = m_setupHomeConnectConnections.value(info->thingId());
        if (!homeConnect) {
            qWarning(dcHomeConnect()) << "No HomeConnect connection found for device:"  << info->thingName();
            m_setupHomeConnectConnections.remove(info->thingId());
            info->finish(Thing::ThingErrorHardwareFailure);
            return;
        }
        qCDebug(dcHomeConnect()) << "Authorization code" << authorizationCode;
        homeConnect->getAccessTokenFromAuthorizationCode(authorizationCode);
        connect(homeConnect, &HomeConnect::receivedRefreshToken, info, [info, this](const QByteArray &refreshToken){
            qCDebug(dcHomeConnect()) << "Token:" << refreshToken;

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

    if (thing->thingClassId() == homeConnectAccountThingClassId) {
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
            if (refreshToken.isEmpty()) {
                info->finish(Thing::ThingErrorAuthenticationFailure, tr("Refresh token is not available."));
                return;
            }
            QByteArray clientKey = configValue(homeConnectPluginCustomClientKeyParamTypeId).toByteArray();
            QByteArray clientSecret = configValue(homeConnectPluginCustomClientSecretParamTypeId).toByteArray();
            if (clientKey.isEmpty() || clientSecret.isEmpty()) {
                clientKey = apiKeyStorage()->requestKey("homeconnect").data("clientKey");
                clientSecret = apiKeyStorage()->requestKey("homeconnect").data("clientSecret");
            }
            if (clientKey.isEmpty() || clientSecret.isEmpty()) {
                info->finish(Thing::ThingErrorAuthenticationFailure, tr("Client key and/or seceret is not available."));
                return;
            }
            homeConnect = new HomeConnect(hardwareManager()->networkManager(), clientKey, clientSecret, simulationMode,  this);
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

    } else if (m_idParamTypeIds.contains(thing->thingClassId())) {
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
    if (!m_pluginTimer15min) {
        m_pluginTimer15min = hardwareManager()->pluginTimerManager()->registerTimer(60*15);
        connect(m_pluginTimer15min, &PluginTimer::timeout, this, [this]() {
            Q_FOREACH (Thing *thing, myThings().filterByThingClassId(homeConnectAccountThingClassId)) {
                HomeConnect *homeConnect = m_homeConnectConnections.value(thing);
                if (!homeConnect) {
                    qWarning(dcHomeConnect()) << "No HomeConnect account found for" << thing->name();
                    continue;
                }
                homeConnect->getHomeAppliances();
                Q_FOREACH (Thing *childThing, myThings().filterByParentId(thing->id())) {
                    QString haId = childThing->paramValue(m_idParamTypeIds.value(childThing->thingClassId())).toString();
                    homeConnect->getStatus(haId);
                    homeConnect->getSettings(haId);
                    homeConnect->getProgramsSelected(haId);
                }
            }
        });
    }

    if (thing->thingClassId() == homeConnectAccountThingClassId) {
        HomeConnect *homeConnect = m_homeConnectConnections.value(thing);
        homeConnect->getHomeAppliances();
        homeConnect->connectEventStream();
        thing->setStateValue(homeConnectAccountConnectedStateTypeId, true);
        thing->setStateValue(homeConnectAccountLoggedInStateTypeId, true);
        //TBD Set user name
    } else if (m_idParamTypeIds.contains(thing->thingClassId())) {
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

    if (m_stopActionTypeIds.values().contains(action.actionTypeId())) {
        QUuid requestId;
        requestId = homeConnect->stopProgram(haid);
        m_pendingActions.insert(requestId, info);
        connect(info, &ThingActionInfo::aborted, [requestId, this] {
            m_pendingActions.remove(requestId);
        });
    } else if (thing->thingClassId() == ovenThingClassId) {
        //Oven control is only allowed with an additional agreement with home connect
    } else if (thing->thingClassId() == coffeeMakerThingClassId) {
        if (action.actionTypeId() == coffeeMakerTemperatureActionTypeId) {
            QUuid requestId;
            QList<HomeConnect::Option> options;
            HomeConnect::Option coffeeTemperature;
            coffeeTemperature.key = "ConsumerProducts.CoffeeMaker.Option.CoffeeTemperature";
            if (action.param(coffeeMakerTemperatureActionTemperatureParamTypeId).value().toString() == "Normal") {
                coffeeTemperature.value = "ConsumerProducts.CoffeeMaker.EnumType.CoffeeTemperature.90C";
            } else if (action.param(coffeeMakerTemperatureActionTemperatureParamTypeId).value().toString() == "High") {
                coffeeTemperature.value = "ConsumerProducts.CoffeeMaker.EnumType.CoffeeTemperature.94C";
            } else if (action.param(coffeeMakerTemperatureActionTemperatureParamTypeId).value().toString() == "Very high") {
                coffeeTemperature.value = "ConsumerProducts.CoffeeMaker.EnumType.CoffeeTemperature.95C";
            }
            options.append(coffeeTemperature);
            requestId = homeConnect->setSelectedProgramOptions(haid, options);
            m_pendingActions.insert(requestId, info);
            connect(info, &ThingActionInfo::aborted, [requestId, this] {
                m_pendingActions.remove(requestId);
            });

        } else if (action.actionTypeId() == coffeeMakerStrengthActionTypeId) {
            QUuid requestId;
            QList<HomeConnect::Option> options;
            HomeConnect::Option beanAmount;
            beanAmount.key = "ConsumerProducts.CoffeeMaker.Option.BeanAmount";
            QString coffeeStrength = action.param(coffeeMakerStrengthActionStrengthParamTypeId).value().toString();
            if (m_coffeeStrengthTypes.values().contains(coffeeStrength)) {
                beanAmount.value = m_coffeeStrengthTypes.key(coffeeStrength);
            } else {
                qCWarning(dcHomeConnect()) << "Unhandled coffee strength action param" << coffeeStrength;
            }
            options.append(beanAmount);
            requestId = homeConnect->setSelectedProgramOptions(haid, options);
            m_pendingActions.insert(requestId, info);
            connect(info, &ThingActionInfo::aborted, [requestId, this] {
                m_pendingActions.remove(requestId);
            });

        } else if (action.actionTypeId() == coffeeMakerFillQuantityActionTypeId) {
            QUuid requestId;
            QList<HomeConnect::Option> options;
            HomeConnect::Option fillQuantity;
            fillQuantity.key = "ConsumerProducts.CoffeeMaker.Option.FillQuantity";
            fillQuantity.unit = "ml";
            fillQuantity.value = qRound(action.param(coffeeMakerFillQuantityActionFillQuantityParamTypeId).value().toInt()/10.00)*10;
            options.append(fillQuantity);
            requestId = homeConnect->setSelectedProgramOptions(haid, options);
            m_pendingActions.insert(requestId, info);
            connect(info, &ThingActionInfo::aborted, [requestId, this] {
                m_pendingActions.remove(requestId);
            });
        } else if (action.actionTypeId() == coffeeMakerStartActionTypeId) {
            if (!m_selectedProgram.contains(thing)) {
                homeConnect->getProgramsSelected(haid);
                return info->finish(Thing::ThingErrorMissingParameter, tr("Please select a program first"));
            }
            QUuid requestId;
            requestId = homeConnect->startProgram(haid, m_selectedProgram.value(thing), QList<HomeConnect::Option>());
            m_pendingActions.insert(requestId, info);
            connect(info, &ThingActionInfo::aborted, [requestId, this] {
                m_pendingActions.remove(requestId);
            });
        }
    } else if (thing->thingClassId() == fridgeThingClassId) {
        //Fridge control is only allowed with an additional agreement with home connect
    } else if (thing->thingClassId() == dishwasherThingClassId) {
        if (action.actionTypeId() == dishwasherStartActionTypeId) {
            if (!m_selectedProgram.contains(thing)) {
                homeConnect->getProgramsSelected(haid);
                return info->finish(Thing::ThingErrorMissingParameter, tr("Please select a program first"));
            }
            QUuid requestId;
            HomeConnect::Option startTime;
            startTime.key = "BSH.Common.Option.StartInRelative";
            startTime.unit = "seconds";
            startTime.value = action.param(dishwasherStartActionStartTimeParamTypeId).value().toInt() * 60;
            requestId = homeConnect->startProgram(haid, m_selectedProgram.value(thing), QList<HomeConnect::Option>() << startTime);
            m_pendingActions.insert(requestId, info);
            connect(info, &ThingActionInfo::aborted, [requestId, this] {
                m_pendingActions.remove(requestId);
            });
        } else {
            Q_ASSERT_X(false, "executeAction", QString("Unhandled actionTypeId: %1").arg(action.actionTypeId().toString()).toUtf8());
        }
    } else if (thing->thingClassId() == washerThingClassId) {
        if (action.actionTypeId() == washerStartActionTypeId) {

            if (!m_selectedProgram.contains(thing)) {
                homeConnect->getProgramsSelected(haid);
                return info->finish(Thing::ThingErrorMissingParameter, tr("Please select a program first"));
            }
            QUuid requestId;
            requestId = homeConnect->startProgram(haid, m_selectedProgram.value(thing), QList<HomeConnect::Option>());
            m_pendingActions.insert(requestId, info);
            connect(info, &ThingActionInfo::aborted, [requestId, this] {
                m_pendingActions.remove(requestId);
            });
        }
    } else if (thing->thingClassId() == dryerThingClassId) {
        if (action.actionTypeId() == dryerStartActionTypeId) {
            if (!m_selectedProgram.contains(thing)) {
                homeConnect->getProgramsSelected(haid);
                return info->finish(Thing::ThingErrorMissingParameter, tr("Please select a program first"));
            }
            QUuid requestId;
            requestId = homeConnect->startProgram(haid, m_selectedProgram.value(thing), QList<HomeConnect::Option>());
            m_pendingActions.insert(requestId, info);
            connect(info, &ThingActionInfo::aborted, [requestId, this] {
                m_pendingActions.remove(requestId);
            });
        } else if (action.actionTypeId() == dryerDryingTargetActionTypeId) {
            QUuid requestId;
            QList<HomeConnect::Option> options;
            HomeConnect::Option dryingTarget;
            dryingTarget.key = "LaundryCare.Dryer.Option.DryingTarget";
            QString target = action.param(dryerDryingTargetActionDryingTargetParamTypeId).value().toString();
            if (target == "Iron dry") {
                dryingTarget.value = "LaundryCare.Dryer.EnumType.DryingTarget.IronDry";
            } else if (target == "Cupboard dry") {
                dryingTarget.value = "LaundryCare.Dryer.EnumType.DryingTarget.CupboardDry";
            } else if (target == "Cupboard dry plus") {
                dryingTarget.value = "LaundryCare.Dryer.EnumType.DryingTarget.CupboardDryPlus";
            }
            options.append(dryingTarget);
            requestId = homeConnect->setSelectedProgramOptions(haid, options);
            m_pendingActions.insert(requestId, info);
            connect(info, &ThingActionInfo::aborted, [requestId, this] {
                m_pendingActions.remove(requestId);
            });
        }
    } else if (thing->thingClassId() == cleaningRobotThingClassId) {
        //Home Connect: Program support is planned to be released in 2020.
    } else if (thing->thingClassId() == cookTopThingClassId) {
        //Home Connect: Program support is planned to be released in 2020.
    } else if (thing->thingClassId() == hoodThingClassId) {
        //Home Connect: Program support is planned to be released in 2020.
    } else {
        Q_ASSERT_X(false, "executeAction", QString("Unhandled thingClassId: %1").arg(thing->thingClassId().toString()).toUtf8());
    }
}

void IntegrationPluginHomeConnect::thingRemoved(Thing *thing)
{
    qCDebug(dcHomeConnect) << "Delete " << thing->name();
    if (thing->thingClassId() == homeConnectAccountThingClassId) {
        m_homeConnectConnections.take(thing)->deleteLater();
    } else {
        m_selectedProgram.remove(thing);
    }

    if (myThings().empty()) {
        if (m_pluginTimer15min) {
            hardwareManager()->pluginTimerManager()->unregisterTimer(m_pluginTimer15min);
            m_pluginTimer15min = nullptr;
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
            if (programs.contains(result->item().id())) {
                result->item().setDisplayName(result->item().id());
                result->finish(Thing::ThingErrorNoError);
            }
        }
    });
}

void IntegrationPluginHomeConnect::executeBrowserItem(BrowserActionInfo *info)
{
    Thing *thing = info->thing();
    qCDebug(dcHomeConnect()) << "Execute browse item called " << thing->name();

    HomeConnect *homeConnect = m_homeConnectConnections.value(myThings().findById(thing->parentId()));
    if (!homeConnect)
        return;
    QString haid = thing->paramValue(m_idParamTypeIds.value(thing->thingClassId())).toString();
    QUuid requestId = homeConnect->selectProgram(haid, info->browserAction().itemId(), QList<HomeConnect::Option> ());
    m_selectedProgram.insert(thing, info->browserAction().itemId());

    connect(homeConnect, &HomeConnect::commandExecuted, info, [requestId, info] (const QUuid &commandId, bool success) {
        if (requestId == commandId) {
            if (success) {
                info->finish(Thing::ThingErrorNoError);
            } else {
                info->finish(Thing::ThingErrorHardwareNotAvailable);
            }
        }
    });
}

void IntegrationPluginHomeConnect::parseKey(Thing *thing, const QString &key, const QVariant &value)
{
    qCDebug(dcHomeConnect()) << thing->name() << key.split('.').last() << value;

    if (key.contains(".Setting.")){
        parseSettingKey(thing, key, value);
        return;
    }
    // PROGRAM CHANGES
    if (key == "BSH.Common.Root.SelectedProgram") {
        if (m_selectedProgramStateTypeIds.contains(thing->thingClassId())) {
            thing->setStateValue(m_selectedProgramStateTypeIds.value(thing->thingClassId()), value.toString().split('.').last());
        }
        m_selectedProgram.insert(thing, value.toString());
    } else if (key == "BSH.Common.Root.ActiveProgram") {
        // Option Changes
    } else if (key == "Cooking.Oven.Option.SetpointTemperature") {
        thing->setStateValue(ovenTargetTemperatureStateTypeId, value);
    } else if (key == "BSH.Common.Option.Duration") {
        thing->setStateValue(ovenDurationStateTypeId, value);
        //} else if (key == "Cooking.Oven.Option.FastPreHeat") {
        //} else if (key == "BSH.Common.Option.StartInRelative") {
    } else if (key == "LaundryCare.Washer.Option.Temperature") {
        thing->setStateValue(washerTemperatureStateTypeId, value.toString().split('.').last()); // Cold, 20, 40, 60°C
    } else if (key == "LaundryCare.Washer.Option.SpinSpeed") {
        thing->setStateValue(washerSpinSpeedStateTypeId, value.toString().split('.').last()); // Off, 400, 600, 800
    } else if (key == "LaundryCare.Dryer.Option.DryingTarget") {
        if (value.toString() == "LaundryCare.Dryer.EnumType.DryingTarget.IronDry") {
            thing->setStateValue(dryerDryingTargetStateTypeId, "Iron dry");
        } else if (value.toString() == "LaundryCare.Dryer.EnumType.DryingTarget.CupboardDry") {
            thing->setStateValue(dryerDryingTargetStateTypeId, "Cupboard dry");
        } else if (value.toString() == "LaundryCare.Dryer.EnumType.DryingTarget.CupboardDryPlus") {
            thing->setStateValue(dryerDryingTargetStateTypeId, "Cupboard dry plus");
        }
    } else if (key == "ConsumerProducts.CoffeeMaker.Option.BeanAmount") {
        QString beanAmount = value.toString();
        if (m_coffeeStrengthTypes.contains(beanAmount)) {
            thing->setStateValue(coffeeMakerStrengthStateTypeId, m_coffeeStrengthTypes.value(beanAmount));
        } else {
            qCWarning(dcHomeConnect()) << "Unhandled bean amount" << beanAmount;
        }
    } else if (key == "ConsumerProducts.CoffeeMaker.Option.FillQuantity") {
        thing->setStateValue(coffeeMakerFillQuantityStateTypeId, value);
    } else if (key == "ConsumerProducts.CoffeeMaker.Option.CoffeeTemperature") {
        QString temperature = value.toString().split(".").last();
        if (temperature == "88C" ||
                temperature == "90C" ||
                temperature == "92C") {
            thing->setStateValue(coffeeMakerTemperatureStateTypeId, "Normal");
        } else if (temperature == "92C" ||
                   temperature == "94C") {
            thing->setStateValue(coffeeMakerTemperatureStateTypeId, "High");
        } else if (temperature == "95C" ||
                   temperature == "96C") {
            thing->setStateValue(coffeeMakerTemperatureStateTypeId, "Very high");
        } else {
            qCDebug(dcHomeConnect()) << "Unkown Coffee temperature string" << temperature;
        }
    } else if (key == "Cooking.Common.Option.Hood.VentingLevel") {
        thing->setStateValue(hoodVentingLevelStateTypeId, value);
        //} else if (key == "Cooking.Common.Option.Hood.IntensiveLevel") {
        //} else if (key == "ConsumerProducts.CleaningRobot.Option.ReferenceMapId") {
        //} else if (key == "ConsumerProducts.CleaningRobot.Option.CleaningMode") {

        // Program Progress Changes
        //} else if (key == "BSH.Common.Option.ElapsedProgramTime") {
    } else if (key == "BSH.Common.Option.RemainingProgramTime") {
        QString time = QDateTime::fromMSecsSinceEpoch(QDateTime::currentMSecsSinceEpoch()+(value.toInt()*1000)).time().toString();
        thing->setStateValue(m_endTimerStateTypeIds.value(thing->thingClassId()), time);
    } else if (key == "BSH.Common.Option.ProgramProgress") {
        if (m_progressStateTypeIds.contains(thing->thingClassId())) {
            thing->setStateValue(m_progressStateTypeIds.value(thing->thingClassId()), value);
        }
        //} else if (key == "ConsumerProducts.CleaningRobot.Option.ProcessPhase") {
    } else if (key == "BSH.Common.Status.OperationState") {
        if (m_operationStateTypeIds.contains(thing->thingClassId())) {
            QString operationState = value.toString();
            if (operationState == "BSH.Common.EnumType.OperationState.Inactive") {
                thing->setStateValue(m_operationStateTypeIds.value(thing->thingClassId()), "Inactive");
            } else if (operationState == "BSH.Common.EnumType.OperationState.Ready") {
                thing->setStateValue(m_operationStateTypeIds.value(thing->thingClassId()), "Ready");
            } else if (operationState == "BSH.Common.EnumType.OperationState.DelayedStart") {
                thing->setStateValue(m_operationStateTypeIds.value(thing->thingClassId()), "Delayed start");
            } else if (operationState == "BSH.Common.EnumType.OperationState.Run") {
                thing->setStateValue(m_operationStateTypeIds.value(thing->thingClassId()), "Run");
            } else if (operationState == "BSH.Common.EnumType.OperationState.Pause") {
                thing->setStateValue(m_operationStateTypeIds.value(thing->thingClassId()), "Pause");
            } else if (operationState == "BSH.Common.EnumType.OperationState.ActionRequired") {
                thing->setStateValue(m_operationStateTypeIds.value(thing->thingClassId()), "Action required");
            } else if (operationState == "BSH.Common.EnumType.OperationState.Finished") {
                thing->setStateValue(m_operationStateTypeIds.value(thing->thingClassId()), "Finished");
            } else if (operationState == "BSH.Common.EnumType.OperationState.Error") {
                thing->setStateValue(m_operationStateTypeIds.value(thing->thingClassId()), "Error");
            } else if (operationState == "BSH.Common.EnumType.OperationState.Aborting") {
                thing->setStateValue(m_operationStateTypeIds.value(thing->thingClassId()), "Aborting");
            }
        }
        if (value.toString().split('.').last().contains("Finished")) {
            //apparently the finished event is not emitted by HomeConnect so this will hopefully do the trick
            if (m_programFinishedEventTypeIds.contains(thing->thingClassId())) {
                emitEvent(Event(m_programFinishedEventTypeIds.value(thing->thingClassId()), thing->id()));
            }
            if (m_progressStateTypeIds.contains(thing->thingClassId())) {
                thing->setStateValue(m_progressStateTypeIds.value(thing->thingClassId()), 0);
            }
        }
        // Program Progress Events
    } else if (key == "BSH.Common.Event.ProgramAborted") {
        if (m_programFinishedEventTypeIds.contains(thing->thingClassId())) {
            emitEvent(Event(m_programFinishedEventTypeIds.value(thing->thingClassId()), thing->id()));
        }
        if (m_progressStateTypeIds.contains(thing->thingClassId())) {
            thing->setStateValue(m_progressStateTypeIds.value(thing->thingClassId()), 0);
        }
    } else if (key == "BSH.Common.Event.ProgramFinished") {
        if (m_programFinishedEventTypeIds.contains(thing->thingClassId())) {
            emitEvent(Event(m_programFinishedEventTypeIds.value(thing->thingClassId()), thing->id()));
        }
        if (m_progressStateTypeIds.contains(thing->thingClassId())) {
            thing->setStateValue(m_progressStateTypeIds.value(thing->thingClassId()), 0);
        }
        //} else if (key == "BSH.Common.Event.AlarmClockElapsed") {
    } else if (key == "Cooking.Oven.Event.PreheatFinished") {
        emitEvent(Event(ovenPreheatFinishedEventTypeId, thing->id()));
        // Home Appliance State Changes
        //} else if (key == "BSH.Common.Setting.PowerState") {
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
            thing->setStateValue(m_doorStateTypeIds.value(thing->thingClassId()), value.toString().split('.').last() != "Open");
        }

        // Home Appliance Events
    } else if (key == "ConsumerProducts.CoffeeMaker.Event.BeanContainerEmpty") {
        emitEvent(Event(coffeeMakerBeanContainerEmptyEventTypeId, thing->id()));
    } else if (key == "ConsumerProducts.CoffeeMaker.Event.WaterTankEmpty") {
        emitEvent(Event(coffeeMakerWaterTankEmptyEventTypeId, thing->id()));
    } else if (key == "ConsumerProducts.CoffeeMaker.Event.DripTrayFull") {
        emitEvent(Event(coffeeMakerDripTrayFullEventTypeId, thing->id()));
    } else if (key == "Refrigeration.FridgeFreezer.Event.DoorAlarmFreezer") {;
        emitEvent(Event(fridgeDoorAlarmFreezerEventTypeId, thing->id()));
    } else if (key == "Refrigeration.FridgeFreezer.Event.DoorAlarmRefrigerator") {
        emitEvent(Event(fridgeDoorAlarmRefrigeratorEventTypeId, thing->id()));
    } else if (key == "Refrigeration.FridgeFreezer.Event.TemperatureAlarmFreezer") {
        emitEvent(Event(fridgeTemperatureAlarmFreezerEventTypeId, thing->id()));
    } else if (key == "ConsumerProducts.CleaningRobot.Event.EmptyDustBoxAndCleanFilter") {
        emitEvent(Event(cleaningRobotEmptyDustBoxAndCleanFilterEventTypeId, thing->id()));
    } else if (key == "ConsumerProducts.CleaningRobot.Event.RobotIsStuck") {
        emitEvent(Event(cleaningRobotRobotIsStuckEventTypeId, thing->id()));
    } else if (key == "ConsumerProducts.CleaningRobot.Event.DockingStationNotFound") {
        emitEvent(Event(cleaningRobotDockingStationNotFoundEventTypeId, thing->id()));

        // UNDOCUMENTED
    } else if (key == "Cooking.Oven.Status.CurrentCavityTemperature") {
        thing->setStateValue(ovenCurrentTemperatureStateTypeId, value);
    } else {
        qCWarning(dcHomeConnect()) << "Parse key: unknown key" << key;
    }
}

void IntegrationPluginHomeConnect::parseSettingKey(Thing *thing, const QString &key, const QVariant &value)
{
    if (key.contains("Refrigeration.FridgeFreezer.Setting.SetpointTemperatureRefrigerator")) {
        thing->setStateValue(fridgeFridgeTemperatureSettingStateTypeId, value);
    } else if (key.contains("Refrigeration.FridgeFreezer.Setting.SetpointTemperatureFreezer")) {
        thing->setStateValue(fridgeFreezerTemperatureStateTypeId, value);
        // For future improvements
        //} else if (key.contains("BSH.Common.Setting.PowerState")) {
        //} else if (key.contains("BSH.Common.Setting.TemperatureUnit")) {
        //} else if (key.contains("BSH.Common.Setting.LiquidVolumeUnit")) {
        //} else if (key.contains("Refrigeration.Common.Setting.BottleCooler.SetpointTemperature")) {
        //} else if (key.contains("Refrigeration.Common.Setting.ChillerLeft.SetpointTemperature")) {
        //} else if (key.contains("Refrigeration.Common.Setting.ChillerCommon.SetpointTemperature")) {
        //} else if (key.contains("Refrigeration.Common.Setting.ChillerRight.SetpointTemperature")) {
        //} else if (key.contains("Refrigeration.Common.Setting.WineCompartment.SetpointTemperature")) {
        //} else if (key.contains("Refrigeration.Common.Setting.WineCompartment2.SetpointTemperature")) {
        //} else if (key.contains("Refrigeration.Common.Setting.WineCompartment3.SetpointTemperature")) {
        //} else if (key.contains("Refrigeration.FridgeFreezer.Setting.SuperModeRefrigerator")) {
        //} else if (key.contains("Refrigeration.FridgeFreezer.Setting.SuperModeFreezer")) {
        //} else if (key.contains("Refrigeration.Common.Setting.EcoMode")) {
        //} else if (key.contains("Refrigeration.Common.Setting.SabbathMode")) {
        //} else if (key.contains("Refrigeration.Common.Setting.VacationMode")) {
        //} else if (key.contains("Refrigeration.Common.Setting.FreshMode")) {
        //} else if (key.contains("Cooking.Common.Setting.Lighting")) {
        //} else if (key.contains("Cooking.Common.Setting.LightingBrightness")) {
        //} else if (key.contains("BSH.Common.Setting.AmbientLightEnabled")) {
        //} else if (key.contains("BSH.Common.Setting.AmbientLightBrightness")) {
        //} else if (key.contains("BSH.Common.Setting.AmbientLightColor")) {
        //} else if (key.contains("BSH.Common.Setting.AmbientLightCustomColor")) {
    }
}

bool IntegrationPluginHomeConnect::checkIfActionIsPossible(ThingActionInfo *info)
{
    Thing *thing = info->thing();

    if (m_connectedStateTypeIds.contains(thing->thingClassId())) {
        if(!thing->stateValue(m_connectedStateTypeIds.value(thing->thingClassId())).toBool()) {
            info->finish(Thing::ThingErrorHardwareNotAvailable, tr("Appliance ist not connected."));
            return false;
        }
    }
    if (m_remoteStartAllowanceStateTypeIds.contains(thing->thingClassId())) {
        if (!thing->stateValue(m_remoteStartAllowanceStateTypeIds.value(thing->thingClassId())).toBool()) {
            info->finish(Thing::ThingErrorHardwareNotAvailable, tr("Remote start is not activated."));
            return false;
        }
    }

    if (m_remoteControlActivationStateTypeIds.contains(thing->thingClassId())) {
        if (!thing->stateValue(m_remoteControlActivationStateTypeIds.value(thing->thingClassId())).toBool()) {
            info->finish(Thing::ThingErrorHardwareNotAvailable, tr("Remote control is not activated."));
            return false;
        }
    }

    if (m_doorStateTypeIds.contains(thing->thingClassId())) {
        if (!thing->stateValue(m_doorStateTypeIds.value(thing->thingClassId())).toBool()) {
            info->finish(Thing::ThingErrorHardwareNotAvailable, tr("Door is not closed."));
            return false;
        }
    }

    if (m_operationStateTypeIds.contains(thing->thingClassId())) {
        if (thing->stateValue(m_operationStateTypeIds.value(thing->thingClassId())).toString() != "Ready") {
            info->finish(Thing::ThingErrorHardwareNotAvailable, tr("Appliance not ready."));
            return false;
        }
    }
    return true;
}

void IntegrationPluginHomeConnect::onConnectionChanged(bool connected)
{
    HomeConnect *homeConnect = static_cast<HomeConnect *>(sender());
    Thing *thing = m_homeConnectConnections.key(homeConnect);
    if (!thing)
        return;
    thing->setStateValue(homeConnectAccountConnectedStateTypeId, connected);
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

        thing->setStateValue(homeConnectAccountLoggedInStateTypeId, authenticated);
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
            //To improve add washerdryer thing classid
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
            qCDebug(dcHomeConnect()) << "Received status list device" << thing->name();
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
            qCDebug(dcHomeConnect()) << "Received selected program" << key << "device" << thing->name();
            if (m_selectedProgramStateTypeIds.contains(thing->thingClassId())) {
                thing->setStateValue(m_selectedProgramStateTypeIds.value(thing->thingClassId()), key.split('.').last());
            }
            m_selectedProgram.insert(thing, key);
            break;
        }
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
            Q_FOREACH(QString setting, settings.keys()) {
                parseSettingKey(thing, setting, settings.value(setting));
            }
            break;
        }
    }
}
