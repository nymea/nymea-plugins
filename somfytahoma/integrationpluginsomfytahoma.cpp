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

#include "integrationpluginsomfytahoma.h"

#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>

#include "network/networkaccessmanager.h"

#include "plugininfo.h"
#include "somfytahomarequests.h"

void IntegrationPluginSomfyTahoma::startPairing(ThingPairingInfo *info)
{
    info->finish(Thing::ThingErrorNoError, QT_TR_NOOP("Please enter the login credentials for Somfy Tahoma."));
}

void IntegrationPluginSomfyTahoma::confirmPairing(ThingPairingInfo *info, const QString &username, const QString &password)
{
    SomfyTahomaRequest *request = createSomfyTahomaLoginRequest(hardwareManager()->networkManager(), username, password, this);
    connect(request, &SomfyTahomaRequest::error, info, [info](){
        info->finish(Thing::ThingErrorAuthenticationFailure, QT_TR_NOOP("Failed to login to Somfy Tahoma."));
    });
    connect(request, &SomfyTahomaRequest::finished, info, [this, info, username, password](const QVariant &/*result*/){
        pluginStorage()->beginGroup(info->thingId().toString());
        pluginStorage()->setValue("username", username);
        pluginStorage()->setValue("password", password);
        pluginStorage()->endGroup();
        info->finish(Thing::ThingErrorNoError);
    });
}

void IntegrationPluginSomfyTahoma::setupThing(ThingSetupInfo *info)
{
    if (info->thing()->thingClassId() == tahomaThingClassId) {
        SomfyTahomaRequest *request = createLoginRequestWithStoredCredentials(info->thing());
        connect(request, &SomfyTahomaRequest::error, info, [info](){
            info->finish(Thing::ThingErrorHardwareFailure, QT_TR_NOOP("Failed to login to Somfy Tahoma."));
        });
        connect(request, &SomfyTahomaRequest::finished, info, [this, info](const QVariant &/*result*/){
            QUuid accountId = info->thing()->id();
            SomfyTahomaRequest *request = createSomfyTahomaGetRequest(hardwareManager()->networkManager(), "/setup", this);
            connect(request, &SomfyTahomaRequest::finished, this, [this, accountId](const QVariant &result){
                QList<ThingDescriptor> unknownDevices;
                foreach (const QVariant &gatewayVariant, result.toMap()["gateways"].toList()) {
                    QVariantMap gatewayMap = gatewayVariant.toMap();
                    QString gatewayId = gatewayMap.value("gatewayId").toString();
                    Thing *thing = myThings().findByParams(ParamList() << Param(gatewayThingGatewayIdParamTypeId, gatewayId));
                    if (thing) {
                        qCDebug(dcSomfyTahoma()) << "Found existing gateway:" << gatewayId;
                    } else {
                        qCInfo(dcSomfyTahoma()) << "Found new gateway:" << gatewayId;
                        ThingDescriptor descriptor(gatewayThingClassId, "TaHoma Gateway", QString(), accountId);
                        descriptor.setParams(ParamList() << Param(gatewayThingGatewayIdParamTypeId, gatewayId));
                        unknownDevices.append(descriptor);
                    }
                }
                foreach (const QVariant &deviceVariant, result.toMap()["devices"].toList()) {
                    QVariantMap deviceMap = deviceVariant.toMap();
                    QString type = deviceMap.value("uiClass").toString();
                    QString deviceUrl = deviceMap.value("deviceURL").toString();
                    QString label = deviceMap.value("label").toString();
                    if (type == QStringLiteral("RollerShutter")) {
                        Thing *thing = myThings().findByParams(ParamList() << Param(rollershutterThingDeviceUrlParamTypeId, deviceUrl));
                        if (thing) {
                            qCDebug(dcSomfyTahoma()) << "Found existing roller shutter:" << label << deviceUrl;
                        } else {
                            qCInfo(dcSomfyTahoma()) << "Found new roller shutter:" << label << deviceUrl;
                            ThingDescriptor descriptor(rollershutterThingClassId, label, QString(), accountId);
                            descriptor.setParams(ParamList() << Param(rollershutterThingDeviceUrlParamTypeId, deviceUrl));
                            unknownDevices.append(descriptor);
                        }
                    } else if (type == QStringLiteral("ExteriorVenetianBlind")) {
                        Thing *thing = myThings().findByParams(ParamList() << Param(venetianblindThingDeviceUrlParamTypeId, deviceUrl));
                        if (thing) {
                            qCDebug(dcSomfyTahoma()) << "Found existing venetian blind:" << label << deviceUrl;
                        } else {
                            qCInfo(dcSomfyTahoma()) << "Found new venetian blind:" << label << deviceUrl;
                            ThingDescriptor descriptor(venetianblindThingClassId, label, QString(), accountId);
                            descriptor.setParams(ParamList() << Param(venetianblindThingDeviceUrlParamTypeId, deviceUrl));
                            unknownDevices.append(descriptor);
                        }
                    } else if (type == QStringLiteral("GarageDoor")) {
                        Thing *thing = myThings().findByParams(ParamList() << Param(garagedoorThingDeviceUrlParamTypeId, deviceUrl));
                        if (thing) {
                            qCDebug(dcSomfyTahoma()) << "Found existing garage door:" << label << deviceUrl;
                        } else {
                            qCInfo(dcSomfyTahoma()) << "Found new garage door:" << label << deviceUrl;
                            ThingDescriptor descriptor(garagedoorThingClassId, label, QString(), accountId);
                            descriptor.setParams(ParamList() << Param(garagedoorThingDeviceUrlParamTypeId, deviceUrl));
                            unknownDevices.append(descriptor);
                        }
                    } else if (type == QStringLiteral("Awning")) {
                        Thing *thing = myThings().findByParams(ParamList() << Param(awningThingDeviceUrlParamTypeId, deviceUrl));
                        if (thing) {
                            qCDebug(dcSomfyTahoma()) << "Found existing awning:" << label << deviceUrl;
                        } else {
                            qCInfo(dcSomfyTahoma()) << "Found new awning:" << label << deviceUrl;
                            ThingDescriptor descriptor(awningThingClassId, label, QString(), accountId);
                            descriptor.setParams(ParamList() << Param(awningThingDeviceUrlParamTypeId, deviceUrl));
                            unknownDevices.append(descriptor);
                        }
                    } else if (type == QStringLiteral("Light") && (deviceUrl.startsWith("io"))) {
                        Thing *thing = myThings().findByParams(ParamList() << Param(lightThingDeviceUrlParamTypeId, deviceUrl));
                        if (thing) {
                            qCDebug(dcSomfyTahoma()) << "Found existing light:" << label << deviceUrl;
                        } else {
                            qCInfo(dcSomfyTahoma()) << "Found new light:" << label << deviceUrl;
                            ThingDescriptor descriptor(lightThingClassId, label, QString(), accountId);
                            descriptor.setParams(ParamList() << Param(lightThingDeviceUrlParamTypeId, deviceUrl));
                            unknownDevices.append(descriptor);
                        }
                    } else if (type == QStringLiteral("ProtocolGateway") ||
                               type == QStringLiteral("Alarm") ||
                               (type == QStringLiteral("Light") && deviceUrl.startsWith("hue"))) {
                        continue;
                    } else {
                        qCInfo(dcSomfyTahoma()) << "Found unsupperted Somfy device:" << label << type << deviceUrl;
                    }
                }
                if (!unknownDevices.isEmpty()) {
                    emit autoThingsAppeared(unknownDevices);
                }
            });
            info->finish(Thing::ThingErrorNoError);
        });
    }

    else if (info->thing()->thingClassId() == gatewayThingClassId ||
             info->thing()->thingClassId() == rollershutterThingClassId ||
             info->thing()->thingClassId() == venetianblindThingClassId ||
             info->thing()->thingClassId() == garagedoorThingClassId ||
             info->thing()->thingClassId() == awningThingClassId ||
             info->thing()->thingClassId() == lightThingClassId) {
        info->finish(Thing::ThingErrorNoError);
    }
}

void IntegrationPluginSomfyTahoma::postSetupThing(Thing *thing)
{
    if (thing->thingClassId() == tahomaThingClassId) {
        pluginStorage()->beginGroup(thing->id().toString());
        thing->setStateValue(tahomaUserDisplayNameStateTypeId, pluginStorage()->value("username"));
        pluginStorage()->endGroup();

        refreshAccount(thing);
    }

    // Set parent of all devices to the respective gateway. We create all devices in setup() of the account.
    // But we don't have the ThingIds of the gateways, because they're created in setup() as well.
    QUrl deviceUrl;
    if (thing->thingClassId() == rollershutterThingClassId) {
        deviceUrl = QUrl(thing->paramValue(rollershutterThingDeviceUrlParamTypeId).toString());
    } else if (thing->thingClassId() == venetianblindThingClassId) {
        deviceUrl = QUrl(thing->paramValue(venetianblindThingDeviceUrlParamTypeId).toString());
    } else if (thing->thingClassId() == garagedoorThingClassId) {
        deviceUrl = QUrl(thing->paramValue(garagedoorThingDeviceUrlParamTypeId).toString());
    } else if (thing->thingClassId() == awningThingClassId) {
        deviceUrl = QUrl(thing->paramValue(awningThingDeviceUrlParamTypeId).toString());
    } else if (thing->thingClassId() == lightThingClassId) {
        deviceUrl = QUrl(thing->paramValue(lightThingDeviceUrlParamTypeId).toString());
    }
    if (!deviceUrl.isEmpty()) {
        Thing *gateway = myThings().findByParams(ParamList() << Param(gatewayThingGatewayIdParamTypeId, deviceUrl.host()));
        if (gateway) {
            thing->setParentId(gateway->parentId());
        } else {
            qCWarning(dcSomfyTahoma()) << "Couldn't find gateway for thing" << thing;
        }
    }
}

void IntegrationPluginSomfyTahoma::refreshAccount(Thing *thing)
{
    // Ensure that even't polling doesn't interfere the refreshing.
    if (m_eventPollTimer.contains(thing)) {
        hardwareManager()->pluginTimerManager()->unregisterTimer(m_eventPollTimer[thing]);
    }

    SomfyTahomaRequest *setupRequest = createSomfyTahomaGetRequest(hardwareManager()->networkManager(), "/setup", this);
    connect(setupRequest, &SomfyTahomaRequest::error, this, [this, thing](){
        markDisconnected(thing);
    });
    connect(setupRequest, &SomfyTahomaRequest::finished, this, [this, thing](const QVariant &result){
        thing->setStateValue(tahomaLoggedInStateTypeId, true);
        thing->setStateValue(tahomaConnectedStateTypeId, true);
        foreach (const QVariant &gatewayVariant, result.toMap()["gateways"].toList()) {
            QVariantMap gatewayMap = gatewayVariant.toMap();
            QString gatewayId = gatewayMap.value("gatewayId").toString();
            Thing *thing = myThings().findByParams(ParamList() << Param(gatewayThingGatewayIdParamTypeId, gatewayId));
            if (thing) {
                qCDebug(dcSomfyTahoma()) << "Setting initial state for gateway:" << gatewayId;
                thing->setStateValue(gatewayConnectedStateTypeId, gatewayMap["connectivity"].toMap()["status"] == "OK");
                pluginStorage()->beginGroup(thing->id().toString());
                pluginStorage()->setValue("connected", gatewayMap["connectivity"].toMap()["status"] == "OK");
                pluginStorage()->endGroup();
            }
        }
        foreach (const QVariant &deviceVariant, result.toMap()["devices"].toList()) {
            updateThingStates(deviceVariant.toMap()["deviceURL"].toString(), deviceVariant.toMap()["states"].toList());
        }
    });

    SomfyTahomaRequest *eventRegistrationRequest = createSomfyTahomaPostRequest(hardwareManager()->networkManager(), "/events/register", "application/json", QByteArray(), this);
    connect(eventRegistrationRequest, &SomfyTahomaRequest::error, this, [this, thing](){
        qCWarning(dcSomfyTahoma()) << "Failed to register for events.";
        markDisconnected(thing);
    });
    connect(eventRegistrationRequest, &SomfyTahomaRequest::finished, this, [this, thing](const QVariant &result){
        thing->setStateValue(tahomaConnectedStateTypeId, true);
        QString eventListenerId = result.toMap()["id"].toString();
        m_eventPollTimer[thing] = hardwareManager()->pluginTimerManager()->registerTimer(2 /*sec*/);
        connect(m_eventPollTimer[thing], &PluginTimer::timeout, thing, [this, thing, eventListenerId](){
            SomfyTahomaRequest *eventFetchRequest = createSomfyTahomaEventFetchRequest(hardwareManager()->networkManager(), eventListenerId, this);
            connect(eventFetchRequest, &SomfyTahomaRequest::error, thing, [this, thing](QNetworkReply::NetworkError error){
                markDisconnected(thing);
                if (error == QNetworkReply::AuthenticationRequiredError) {
                    qCInfo(dcSomfyTahoma()) << "Failed to fetch events: Authentication expired, reauthenticating";
                    SomfyTahomaRequest *request = createLoginRequestWithStoredCredentials(thing);
                    connect(request, &SomfyTahomaRequest::error, this, [this, thing](){
                        // This is a fatal error. The user needs to reconfigure the account to provide new credentials.
                        qCWarning(dcSomfyTahoma()) << "Failed to reauthenticate";
                        hardwareManager()->pluginTimerManager()->unregisterTimer(m_eventPollTimer[thing]);
                        m_eventPollTimer.remove(thing);
                    });
                    connect(request, &SomfyTahomaRequest::finished, this, [this, thing](const QVariant &/*result*/){
                        qCInfo(dcSomfyTahoma()) << "Reauthentication successful";
                        refreshAccount(thing);
                    });
                } else {
                    qCWarning(dcSomfyTahoma()) << "Failed to fetch events:" << error;
                }
            });
            connect(eventFetchRequest, &SomfyTahomaRequest::finished, thing, [this, thing](const QVariant &result){
                thing->setStateValue(tahomaConnectedStateTypeId, true);
                restoreChildConnectedState(thing);
                if (!result.toList().empty()) {
                    qCDebug(dcSomfyTahoma()) << "Got events:" << qUtf8Printable(QJsonDocument::fromVariant(result).toJson());
                }
                handleEvents(result.toList());
            });
        });
    });
}

void IntegrationPluginSomfyTahoma::thingRemoved(Thing *thing)
{
    m_eventPollTimer.remove(thing);
}

void IntegrationPluginSomfyTahoma::handleEvents(const QVariantList &eventList)
{
    Thing *thing;
    foreach (const QVariant &eventVariant, eventList) {
        QVariantMap eventMap = eventVariant.toMap();
        if (eventMap["name"] == "DeviceStateChangedEvent") {
            updateThingStates(eventMap["deviceURL"].toString(), eventMap["deviceStates"].toList());
        } else if (eventMap["name"] == "ExecutionRegisteredEvent") {
            QList<Thing *> things;
            foreach (const QVariant &action, eventMap["actions"].toList()) {
                thing = myThings().findByParams(ParamList() << Param(rollershutterThingDeviceUrlParamTypeId, action.toMap()["deviceURL"]));
                if (thing) {
                    thing->setStateValue(rollershutterMovingStateTypeId, true);
                    things.append(thing);
                    continue;
                }
                thing = myThings().findByParams(ParamList() << Param(venetianblindThingDeviceUrlParamTypeId, action.toMap()["deviceURL"]));
                if (thing) {
                    thing->setStateValue(venetianblindMovingStateTypeId, true);
                    things.append(thing);
                }
                thing = myThings().findByParams(ParamList() << Param(garagedoorThingDeviceUrlParamTypeId, action.toMap()["deviceURL"]));
                if (thing) {
                    thing->setStateValue(garagedoorMovingStateTypeId, true);
                    things.append(thing);
                }
                thing = myThings().findByParams(ParamList() << Param(awningThingDeviceUrlParamTypeId, action.toMap()["deviceURL"]));
                if (thing) {
                    thing->setStateValue(awningMovingStateTypeId, true);
                    things.append(thing);
                    continue;
                }
            }
            qCDebug(dcSomfyTahoma()) << "ExecutionRegisteredEvent" << eventMap["execId"];
            m_currentExecutions.insert(eventMap["execId"].toString(), things);
        } else if (eventMap["name"] == "ExecutionStateChangedEvent" &&
                   (eventMap["newState"] == "COMPLETED" || eventMap["newState"] == "FAILED")) {
            QList<Thing *> things = m_currentExecutions.take(eventMap["execId"].toString());
            foreach (Thing *thing, things) {
                if (thing->thingClassId() == rollershutterThingClassId) {
                    thing->setStateValue(rollershutterMovingStateTypeId, false);
                } else if (thing->thingClassId() == venetianblindThingClassId) {
                    thing->setStateValue(venetianblindMovingStateTypeId, false);
                } else if (thing->thingClassId() == garagedoorThingClassId) {
                    thing->setStateValue(garagedoorMovingStateTypeId, false);
                } else if (thing->thingClassId() == awningThingClassId) {
                    thing->setStateValue(awningMovingStateTypeId, false);
                }
            }

            QPointer<ThingActionInfo> thingActionInfo = m_pendingActions.take(eventMap["execId"].toString());
            if (!thingActionInfo.isNull()) {
                if (eventMap["newState"] == "COMPLETED") {
                    qCDebug(dcSomfyTahoma()) << "Action finished" << thingActionInfo->thing() << thingActionInfo->action().actionTypeId();
                    thingActionInfo->finish(Thing::ThingErrorNoError);
                } else if (eventMap["newState"] == "FAILED") {
                    qCWarning(dcSomfyTahoma()) << "Action failed" << thingActionInfo->thing() << thingActionInfo->action().actionTypeId();
                    thingActionInfo->finish(Thing::ThingErrorHardwareFailure);
                } else {
                    qCWarning(dcSomfyTahoma()) << "Action in unknown state" << thingActionInfo->thing() << thingActionInfo->action().actionTypeId() << eventMap["newState"].toString();
                    thingActionInfo->finish(Thing::ThingErrorHardwareFailure);
                }
            }
        } else if (eventMap["name"] == "GatewayAliveEvent") {
            thing = myThings().findByParams(ParamList() << Param(gatewayThingGatewayIdParamTypeId, eventMap["gatewayId"]));
            if (thing) {
                qCInfo(dcSomfyTahoma()) << "Gateway connected event received:" << eventMap["gatewayId"];
                thing->setStateValue(gatewayConnectedStateTypeId, true);
                pluginStorage()->beginGroup(thing->id().toString());
                pluginStorage()->setValue("connected", true);
                pluginStorage()->endGroup();
                restoreChildConnectedState(thing);
            } else {
                qCDebug(dcSomfyTahoma()) << "Ignoring gateway connected event for unknown gateway" << eventMap["gatewayId"];
            }
        } else if (eventMap["name"] == "GatewayDownEvent") {
            thing = myThings().findByParams(ParamList() << Param(gatewayThingGatewayIdParamTypeId, eventMap["gatewayId"]));
            if (thing) {
                qCInfo(dcSomfyTahoma()) << "Gateway disconnected event received:" << eventMap["gatewayId"];
                thing->setStateValue(gatewayConnectedStateTypeId, false);
                pluginStorage()->beginGroup(thing->id().toString());
                pluginStorage()->setValue("connected", false);
                pluginStorage()->endGroup();
                markDisconnected(thing);
            } else {
                qCDebug(dcSomfyTahoma()) << "Ignoring gateway disconnected event for unknown gateway" << eventMap["gatewayId"];
            }
        }
    }
}

void IntegrationPluginSomfyTahoma::updateThingStates(const QString &deviceUrl, const QVariantList &stateList)
{
    Thing *thing = myThings().findByParams(ParamList() << Param(rollershutterThingDeviceUrlParamTypeId, deviceUrl));
    if (thing) {
        foreach (const QVariant &stateVariant, stateList) {
            QVariantMap stateMap = stateVariant.toMap();
            if (stateMap["name"] == "core:ClosureState") {
                thing->setStateValue(rollershutterPercentageStateTypeId, stateMap["value"]);
            } else if (stateMap["name"] == "core:StatusState") {
                thing->setStateValue(rollershutterConnectedStateTypeId, stateMap["value"] == "available");
                pluginStorage()->beginGroup(thing->id().toString());
                pluginStorage()->setValue("connected", stateMap["value"] == "available");
                pluginStorage()->endGroup();
            } else if (stateMap["name"] == "core:RSSILevelState") {
                thing->setStateValue(rollershutterSignalStrengthStateTypeId, stateMap["value"]);
            }
        }
        return;
    }
    thing = myThings().findByParams(ParamList() << Param(venetianblindThingDeviceUrlParamTypeId, deviceUrl));
    if (thing) {
        foreach (const QVariant &stateVariant, stateList) {
            QVariantMap stateMap = stateVariant.toMap();
            if (stateMap["name"] == "core:ClosureState") {
                thing->setStateValue(venetianblindPercentageStateTypeId, stateMap["value"]);
            } else if (stateMap["name"] == "core:SlateOrientationState") {
                // Convert percentage (0%/100%, 50%=open) into degree (-90/+90)
                int degree = (stateMap["value"].toInt() * 1.8) - 90;
                thing->setStateValue(venetianblindAngleStateTypeId, degree);
            } else if (stateMap["name"] == "core:StatusState") {
                thing->setStateValue(venetianblindConnectedStateTypeId, stateMap["value"] == "available");
                pluginStorage()->beginGroup(thing->id().toString());
                pluginStorage()->setValue("connected", stateMap["value"] == "available");
                pluginStorage()->endGroup();
            } else if (stateMap["name"] == "core:RSSILevelState") {
                thing->setStateValue(venetianblindSignalStrengthStateTypeId, stateMap["value"]);
            }
        }
        return;
    }
    thing = myThings().findByParams(ParamList() << Param(garagedoorThingDeviceUrlParamTypeId, deviceUrl));
    if (thing) {
        foreach (const QVariant &stateVariant, stateList) {
            QVariantMap stateMap = stateVariant.toMap();
            if (stateMap["name"] == "core:ClosureState") {
                thing->setStateValue(garagedoorPercentageStateTypeId, stateMap["value"]);
                if (stateMap["value"] == 100) {
                    thing->setStateValue(garagedoorStateStateTypeId, "closed");
                } else if (stateMap["value"] == 0) {
                    thing->setStateValue(garagedoorStateStateTypeId, "open");
                } else {
                    thing->setStateValue(garagedoorStateStateTypeId, "intermediate");
                }
            } else if (stateMap["name"] == "core:StatusState") {
                thing->setStateValue(garagedoorConnectedStateTypeId, stateMap["value"] == "available");
                pluginStorage()->beginGroup(thing->id().toString());
                pluginStorage()->setValue("connected", stateMap["value"] == "available");
                pluginStorage()->endGroup();
            } else if (stateMap["name"] == "core:RSSILevelState") {
                thing->setStateValue(garagedoorSignalStrengthStateTypeId, stateMap["value"]);
            }
        }
        return;
    }
    thing = myThings().findByParams(ParamList() << Param(awningThingDeviceUrlParamTypeId, deviceUrl));
    if (thing) {
        foreach (const QVariant &stateVariant, stateList) {
            QVariantMap stateMap = stateVariant.toMap();
            if (stateMap["name"] == "core:DeploymentState") {
                thing->setStateValue(awningPercentageStateTypeId, stateMap["value"]);
            } else if (stateMap["name"] == "core:StatusState") {
                thing->setStateValue(awningConnectedStateTypeId, stateMap["value"] == "available");
                pluginStorage()->beginGroup(thing->id().toString());
                pluginStorage()->setValue("connected", stateMap["value"] == "available");
                pluginStorage()->endGroup();
            } else if (stateMap["name"] == "core:RSSILevelState") {
                thing->setStateValue(awningSignalStrengthStateTypeId, stateMap["value"]);
            }
        }
        return;
    }
    thing = myThings().findByParams(ParamList() << Param(lightThingDeviceUrlParamTypeId, deviceUrl));
    if (thing) {
        foreach (const QVariant &stateVariant, stateList) {
            QVariantMap stateMap = stateVariant.toMap();
            if (stateMap["name"] == "core:OnOffState") {
                thing->setStateValue(lightPowerStateTypeId, stateMap["value"] == "on");
            } else if (stateMap["name"] == "core:LightIntensityState") {
                thing->setStateValue(lightBrightnessStateTypeId, stateMap["value"]);
            } else if (stateMap["name"] == "core:StatusState") {
                thing->setStateValue(lightConnectedStateTypeId, stateMap["value"] == "available");
                pluginStorage()->beginGroup(thing->id().toString());
                pluginStorage()->setValue("connected", stateMap["value"] == "available");
                pluginStorage()->endGroup();
            } else if (stateMap["name"] == "core:RSSILevelState") {
                thing->setStateValue(lightSignalStrengthStateTypeId, stateMap["value"]);
            }
        }
        return;
    }
}

void IntegrationPluginSomfyTahoma::executeAction(ThingActionInfo *info)
{
    qCInfo(dcSomfyTahoma()) << "Action request:" << info->thing() << info->action().actionTypeId() << info->action().params();

    QString deviceUrl;
    QString actionName;
    QJsonArray actionParameters;

    if (info->thing()->thingClassId() == rollershutterThingClassId) {
        deviceUrl = info->thing()->paramValue(rollershutterThingDeviceUrlParamTypeId).toString();
        if (info->action().actionTypeId() == rollershutterPercentageActionTypeId) {
            actionName = "setClosureAndLinearSpeed";
            actionParameters = { info->action().param(rollershutterPercentageActionPercentageParamTypeId).value().toInt(), "lowspeed" };
        } else if (info->action().actionTypeId() == rollershutterOpenActionTypeId) {
            actionName = "setClosureAndLinearSpeed";
            actionParameters = { 0, "lowspeed" };
        } else if (info->action().actionTypeId() == rollershutterCloseActionTypeId) {
            actionName = "setClosureAndLinearSpeed";
            actionParameters = { 100, "lowspeed" };
        } else if (info->action().actionTypeId() == rollershutterStopActionTypeId) {
            actionName = "stop";
        }
    } else if (info->thing()->thingClassId() == venetianblindThingClassId) {
        deviceUrl = info->thing()->paramValue(venetianblindThingDeviceUrlParamTypeId).toString();
        if (info->action().actionTypeId() == venetianblindPercentageActionTypeId) {
            actionName = "setClosure";
            actionParameters = { info->action().param(venetianblindPercentageActionPercentageParamTypeId).value().toInt() };
        } else if (info->action().actionTypeId() == venetianblindAngleActionTypeId) {
            actionName = "setOrientation";
            // Convert degree (-90/+90) into percentage (0%/100%, 50%=open)
            int degree = (info->action().param(venetianblindAngleActionAngleParamTypeId).value().toInt() + 90) / 1.8;
            actionParameters = { degree };
        } else if (info->action().actionTypeId() == venetianblindOpenActionTypeId) {
            actionName = "open";
        } else if (info->action().actionTypeId() == venetianblindCloseActionTypeId) {
            actionName = "close";
        } else if (info->action().actionTypeId() == venetianblindStopActionTypeId) {
            actionName = "stop";
        }
    } else if (info->thing()->thingClassId() == garagedoorThingClassId) {
        deviceUrl = info->thing()->paramValue(garagedoorThingDeviceUrlParamTypeId).toString();
        if (info->action().actionTypeId() == garagedoorPercentageActionTypeId) {
            actionName = "setClosure";
            actionParameters = { info->action().param(garagedoorPercentageActionPercentageParamTypeId).value().toInt() };
        } else if (info->action().actionTypeId() == garagedoorOpenActionTypeId) {
            actionName = "open";
        } else if (info->action().actionTypeId() == garagedoorCloseActionTypeId) {
            actionName = "close";
        } else if (info->action().actionTypeId() == garagedoorStopActionTypeId) {
            actionName = "stop";
        }
    } else if (info->thing()->thingClassId() == awningThingClassId) {
        deviceUrl = info->thing()->paramValue(awningThingDeviceUrlParamTypeId).toString();
        if (info->action().actionTypeId() == awningPercentageActionTypeId) {
            actionName = "setDeployment";
            actionParameters = { info->action().param(awningPercentageActionPercentageParamTypeId).value().toInt() };
        } else if (info->action().actionTypeId() == awningOpenActionTypeId) {
            actionName = "deploy";
        } else if (info->action().actionTypeId() == awningCloseActionTypeId) {
            actionName = "undeploy";
        } else if (info->action().actionTypeId() == awningStopActionTypeId) {
            actionName = "stop";
        }
    } else if (info->thing()->thingClassId() == lightThingClassId) {
        deviceUrl = info->thing()->paramValue(lightThingDeviceUrlParamTypeId).toString();
        if (info->action().actionTypeId() == lightPowerActionTypeId) {
            actionName = info->action().param(lightPowerActionPowerParamTypeId).value().toBool() ? "on" : "off";
        } else if (info->action().actionTypeId() == lightBrightnessActionTypeId) {
            actionName = "setIntensity";
            actionParameters = { info->action().param(lightBrightnessActionBrightnessParamTypeId).value().toInt() };
        }
    }

    if (!actionName.isEmpty()) {
        QJsonDocument jsonRequest{QJsonObject
        {
            {"label", info->thing()->name()},
            {"actions", QJsonArray{QJsonObject{{"deviceURL", deviceUrl},
                                               {"commands", QJsonArray{QJsonObject{{"name", actionName},
                                                                                   {"parameters", actionParameters}}}}}}}
        }};
        SomfyTahomaRequest *request = createSomfyTahomaPostRequest(hardwareManager()->networkManager(), "/exec/apply", "application/json", jsonRequest.toJson(QJsonDocument::Compact), this);
        connect(request, &SomfyTahomaRequest::error, info, [info](){
            info->finish(Thing::ThingErrorHardwareFailure);
        });
        connect(request, &SomfyTahomaRequest::finished, info, [this, info](const QVariant &result){
            qCInfo(dcSomfyTahoma()) << "Action started" << info->thing() << info->action().actionTypeId();
            m_pendingActions.insert(result.toMap()["execId"].toString(), info);
        });
    } else {
        info->finish(Thing::ThingErrorActionTypeNotFound);
    }
}

SomfyTahomaRequest *IntegrationPluginSomfyTahoma::createLoginRequestWithStoredCredentials(Thing *thing)
{
    pluginStorage()->beginGroup(thing->id().toString());
    QString username = pluginStorage()->value("username").toString();
    QString password = pluginStorage()->value("password").toString();
    pluginStorage()->endGroup();
    return createSomfyTahomaLoginRequest(hardwareManager()->networkManager(), username, password, this);
}

void IntegrationPluginSomfyTahoma::markDisconnected(Thing *thing)
{
    if (thing->thingClassId() == tahomaThingClassId) {
        thing->setStateValue(tahomaConnectedStateTypeId, false);
    } else if (thing->thingClassId() == gatewayThingClassId) {
        thing->setStateValue(gatewayConnectedStateTypeId, false);
    } else if (thing->thingClassId() == rollershutterThingClassId) {
        thing->setStateValue(rollershutterConnectedStateTypeId, false);
    } else if (thing->thingClassId() == venetianblindThingClassId) {
        thing->setStateValue(venetianblindConnectedStateTypeId, false);
    } else if (thing->thingClassId() == garagedoorThingClassId) {
        thing->setStateValue(garagedoorConnectedStateTypeId, false);
    } else if (thing->thingClassId() == awningThingClassId) {
        thing->setStateValue(awningConnectedStateTypeId, false);
    } else if (thing->thingClassId() == lightThingClassId) {
        thing->setStateValue(lightConnectedStateTypeId, false);
    }
    foreach (Thing *child, myThings().filterByParentId(thing->id())) {
        markDisconnected(child);
    }
}

void IntegrationPluginSomfyTahoma::restoreChildConnectedState(Thing *thing)
{
    pluginStorage()->beginGroup(thing->id().toString());
    if (pluginStorage()->contains("connected")) {
        if (thing->thingClassId() == gatewayThingClassId) {
            thing->setStateValue(gatewayConnectedStateTypeId, pluginStorage()->value("connected").toBool());
        } else if (thing->thingClassId() == rollershutterThingClassId) {
            thing->setStateValue(rollershutterConnectedStateTypeId, pluginStorage()->value("connected").toBool());
        } else if (thing->thingClassId() == venetianblindThingClassId) {
            thing->setStateValue(venetianblindConnectedStateTypeId, pluginStorage()->value("connected").toBool());
        } else if (thing->thingClassId() == garagedoorThingClassId) {
            thing->setStateValue(garagedoorConnectedStateTypeId, pluginStorage()->value("connected").toBool());
        } else if (thing->thingClassId() == awningThingClassId) {
            thing->setStateValue(awningConnectedStateTypeId, pluginStorage()->value("connected").toBool());
        } else if (thing->thingClassId() == lightThingClassId) {
            thing->setStateValue(lightConnectedStateTypeId, pluginStorage()->value("connected").toBool());
        }
    }
    pluginStorage()->endGroup();
    foreach (Thing *child, myThings().filterByParentId(thing->id())) {
        restoreChildConnectedState(child);
    }
}
