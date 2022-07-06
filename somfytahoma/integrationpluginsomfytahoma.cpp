/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*
* Copyright 2013 - 2022, nymea GmbH
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
#include "network/zeroconf/zeroconfservicebrowser.h"
#include "platform/platformzeroconfcontroller.h"

#include "plugininfo.h"
#include "somfytahomarequests.h"

void IntegrationPluginSomfyTahoma::init()
{
    m_zeroConfBrowser = hardwareManager()->zeroConfController()->createServiceBrowser("_kizboxdev._tcp");
}

void IntegrationPluginSomfyTahoma::discoverThings(ThingDiscoveryInfo *info)
{
    foreach (const ZeroConfServiceEntry &entry, m_zeroConfBrowser->serviceEntries()) {
        qCDebug(dcSomfyTahoma()) << "Found local gateway:" << entry;

        ThingDescriptor descriptor(info->thingClassId(), "Somfy TaHoma Gateway", entry.hostAddress().toString());
        ParamList params;
        params << Param(gatewayThingGatewayPinParamTypeId, entry.txt("gateway_pin"));
        descriptor.setParams(params);

        Things existingThings = myThings().filterByParam(gatewayThingGatewayPinParamTypeId, entry.txt("gateway_pin"));
        if (existingThings.count() == 1) {
            qCDebug(dcSomfyTahoma()) << "This gateway already exists in the system!";
            descriptor.setThingId(existingThings.first()->id());
        }

        info->addThingDescriptor(descriptor);
    }

    info->finish(Thing::ThingErrorNoError);
}

void IntegrationPluginSomfyTahoma::startPairing(ThingPairingInfo *info)
{
    info->finish(Thing::ThingErrorNoError, QT_TR_NOOP("Please enter the cloud login credentials for Somfy TaHoma in order to set up local access to the Gateway."));
}

void IntegrationPluginSomfyTahoma::confirmPairing(ThingPairingInfo *info, const QString &username, const QString &password)
{
    // Request local token from cloud account.
    SomfyTahomaRequest *request = createCloudSomfyTahomaLoginRequest(hardwareManager()->networkManager(), username, password, this);
    connect(request, &SomfyTahomaRequest::error, info, [info](){
        info->finish(Thing::ThingErrorAuthenticationFailure, QT_TR_NOOP("Failed to login to Somfy TaHoma."));
    });
    connect(request, &SomfyTahomaRequest::finished, info, [this, info, username, password](const QVariant &/*result*/){
        SomfyTahomaRequest *request = createCloudSomfyTahomaGetRequest(hardwareManager()->networkManager(), "/config/" + info->params().paramValue(gatewayThingGatewayPinParamTypeId).toString() + "/local/tokens/generate", this);
        connect(request, &SomfyTahomaRequest::error, info, [info](){
            info->finish(Thing::ThingErrorAuthenticationFailure, QT_TR_NOOP("Failed to generate token."));
        });
        connect(request, &SomfyTahomaRequest::finished, info, [this, info, username, password](const QVariant &result){
            QString token = result.toMap()["token"].toString();
            QJsonDocument jsonRequest{QJsonObject{
                {"label", "nymea_" + info->thingId().toString()},
                {"token", token},
                {"scope", "devmode"},
            }};
            SomfyTahomaRequest *request = createCloudSomfyTahomaPostRequest(hardwareManager()->networkManager(), "/config/" + info->params().paramValue(gatewayThingGatewayPinParamTypeId).toString() + "/local/tokens", "application/json", jsonRequest.toJson(QJsonDocument::Compact), this);
            connect(request, &SomfyTahomaRequest::error, info, [info](){
                info->finish(Thing::ThingErrorAuthenticationFailure, QT_TR_NOOP("Failed to activate token."));
            });
            connect(request, &SomfyTahomaRequest::finished, info, [this, info, username, password, token](const QVariant &/*result*/){
                pluginStorage()->beginGroup(info->thingId().toString());
                pluginStorage()->setValue("username", username);
                pluginStorage()->setValue("password", password);
                pluginStorage()->setValue("token", token);
                pluginStorage()->endGroup();

                info->finish(Thing::ThingErrorNoError);
            });
        });
    });
}

void IntegrationPluginSomfyTahoma::setupThing(ThingSetupInfo *info)
{
    // Compatibility to older cloud based versions of the plugin.
    if (info->thing()->thingClassId() == tahomaThingClassId ||
            (info->thing()->thingClassId() == gatewayThingClassId && getToken(info->thing()).isEmpty())) {
        info->finish(Thing::ThingErrorSetupFailed, QT_TR_NOOP("The Somfy Plugin switched to local connection. Please enable 'Developer Mode' on somfy.com, remove the account from Nymea and re-setup the Somfy TaHoma Gateway."));
        return;
    }

    if (info->thing()->thingClassId() == gatewayThingClassId) {
        SomfyTahomaRequest *request = createLocalSomfyTahomaGetRequest(hardwareManager()->networkManager(), getHost(info->thing()), getToken(info->thing()), "/setup", this);
        connect(request, &SomfyTahomaRequest::error, info, [info](){
            info->finish(Thing::ThingErrorHardwareFailure, QT_TR_NOOP("Failed to connect to gateway."));
        });
        connect(request, &SomfyTahomaRequest::finished, info, [info, this](const QVariant &result){
            QList<ThingDescriptor> unknownDevices;
            QUuid gatewayId = info->thing()->id();

            foreach (const QVariant &deviceVariant, result.toMap()["devices"].toList()) {
                QVariantMap deviceMap = deviceVariant.toMap();
                QString type = deviceMap.value("controllableName").toString();
                QString deviceUrl = deviceMap.value("deviceURL").toString();
                QString label = deviceMap.value("label").toString();

                if (type.startsWith(QStringLiteral("io:RollerShutter"))) {
                    Thing *thing = myThings().findByParams(ParamList() << Param(rollershutterThingDeviceUrlParamTypeId, deviceUrl));
                    if (thing) {
                        qCDebug(dcSomfyTahoma()) << "Found existing roller shutter:" << label << deviceUrl;
                    } else {
                        qCInfo(dcSomfyTahoma()) << "Found new roller shutter:" << label << deviceUrl;
                        ThingDescriptor descriptor(rollershutterThingClassId, label, QString(), gatewayId);
                        descriptor.setParams(ParamList() << Param(rollershutterThingDeviceUrlParamTypeId, deviceUrl));
                        unknownDevices.append(descriptor);
                    }
                } else if (type == QStringLiteral("io:ExteriorVenetianBlindIOComponent")) {
                    Thing *thing = myThings().findByParams(ParamList() << Param(venetianblindThingDeviceUrlParamTypeId, deviceUrl));
                    if (thing) {
                        qCDebug(dcSomfyTahoma()) << "Found existing venetian blind:" << label << deviceUrl;
                    } else {
                        qCInfo(dcSomfyTahoma()) << "Found new venetian blind:" << label << deviceUrl;
                        ThingDescriptor descriptor(venetianblindThingClassId, label, QString(), gatewayId);
                        descriptor.setParams(ParamList() << Param(venetianblindThingDeviceUrlParamTypeId, deviceUrl));
                        unknownDevices.append(descriptor);
                    }
                } else if (type == QStringLiteral("io:GarageOpenerIOComponent")) {
                    Thing *thing = myThings().findByParams(ParamList() << Param(garagedoorThingDeviceUrlParamTypeId, deviceUrl));
                    if (thing) {
                        qCDebug(dcSomfyTahoma()) << "Found existing garage door:" << label << deviceUrl;
                    } else {
                        qCInfo(dcSomfyTahoma()) << "Found new garage door:" << label << deviceUrl;
                        ThingDescriptor descriptor(garagedoorThingClassId, label, QString(), gatewayId);
                        descriptor.setParams(ParamList() << Param(garagedoorThingDeviceUrlParamTypeId, deviceUrl));
                        unknownDevices.append(descriptor);
                    }
                } else if (type == QStringLiteral("io:HorizontalAwningIOComponent")) {
                    Thing *thing = myThings().findByParams(ParamList() << Param(awningThingDeviceUrlParamTypeId, deviceUrl));
                    if (thing) {
                        qCDebug(dcSomfyTahoma()) << "Found existing awning:" << label << deviceUrl;
                    } else {
                        qCInfo(dcSomfyTahoma()) << "Found new awning:" << label << deviceUrl;
                        ThingDescriptor descriptor(awningThingClassId, label, QString(), gatewayId);
                        descriptor.setParams(ParamList() << Param(awningThingDeviceUrlParamTypeId, deviceUrl));
                        unknownDevices.append(descriptor);
                    }
                } else if (type == QStringLiteral("io:DimmableLightIOComponent")) {
                    Thing *thing = myThings().findByParams(ParamList() << Param(lightThingDeviceUrlParamTypeId, deviceUrl));
                    if (thing) {
                        qCDebug(dcSomfyTahoma()) << "Found existing light:" << label << deviceUrl;
                    } else {
                        qCInfo(dcSomfyTahoma()) << "Found new light:" << label << deviceUrl;
                        ThingDescriptor descriptor(lightThingClassId, label, QString(), gatewayId);
                        descriptor.setParams(ParamList() << Param(lightThingDeviceUrlParamTypeId, deviceUrl));
                        unknownDevices.append(descriptor);
                    }
                } else if (type == QStringLiteral("io:SomfySmokeIOSystemSensor")) {
                    Thing *thing = myThings().findByParams(ParamList() << Param(smokedetectorThingDeviceUrlParamTypeId, deviceUrl));
                    if (thing) {
                        qCDebug(dcSomfyTahoma()) << "Found existing smoke detector:" << label << deviceUrl;
                    } else {
                        qCInfo(dcSomfyTahoma()) << "Found new smoke detector:" << label << deviceUrl;
                        ThingDescriptor descriptor(smokedetectorThingClassId, label, QString(), gatewayId);
                        descriptor.setParams(ParamList() << Param(smokedetectorThingDeviceUrlParamTypeId, deviceUrl));
                        unknownDevices.append(descriptor);
                    }
                } else if (type == QStringLiteral("io:StackComponent") ||
                           type.startsWith("internal:")) {
                    continue;
                } else {
                    qCInfo(dcSomfyTahoma()) << "Found unsupperted Somfy device:" << label << type << deviceUrl;
                }
            }
            info->finish(Thing::ThingErrorNoError);
            if (!unknownDevices.isEmpty()) {
                emit autoThingsAppeared(unknownDevices);
            }
        });
    }

    else if (info->thing()->thingClassId() == rollershutterThingClassId ||
             info->thing()->thingClassId() == venetianblindThingClassId ||
             info->thing()->thingClassId() == garagedoorThingClassId ||
             info->thing()->thingClassId() == awningThingClassId ||
             info->thing()->thingClassId() == lightThingClassId ||
             info->thing()->thingClassId() == smokedetectorThingClassId) {
        info->finish(Thing::ThingErrorNoError);
    }
}

void IntegrationPluginSomfyTahoma::postSetupThing(Thing *thing)
{
    if (thing->thingClassId() != gatewayThingClassId) {
        return;
    }

    // Call /setup and update the state of all devices
    SomfyTahomaRequest *setupRequest = createLocalSomfyTahomaGetRequest(hardwareManager()->networkManager(), getHost(thing), getToken(thing), "/setup", this);
    connect(setupRequest, &SomfyTahomaRequest::error, this, [this, thing](){
        markDisconnected(thing);
    });
    connect(setupRequest, &SomfyTahomaRequest::finished, this, [this, thing](const QVariant &result){
        thing->setStateValue(gatewayConnectedStateTypeId, true);
        foreach (const QVariant &deviceVariant, result.toMap()["devices"].toList()) {
            updateThingStates(deviceVariant.toMap()["deviceURL"].toString(), deviceVariant.toMap()["states"].toList());
        }
    });

    // Register event handler
    SomfyTahomaRequest *eventRegistrationRequest = createLocalSomfyTahomaPostRequest(hardwareManager()->networkManager(), getHost(thing), getToken(thing), "/events/register", "application/json", QByteArray(), this);
    connect(eventRegistrationRequest, &SomfyTahomaRequest::error, this, [this, thing](){
        markDisconnected(thing);
    });
    connect(eventRegistrationRequest, &SomfyTahomaRequest::finished, this, [this, thing](const QVariant &result){
        QString eventListenerId = result.toMap()["id"].toString();
        m_eventPollTimer[thing] = hardwareManager()->pluginTimerManager()->registerTimer(2 /*sec*/);
        connect(m_eventPollTimer[thing], &PluginTimer::timeout, thing, [this, thing, eventListenerId](){
            SomfyTahomaRequest *eventFetchRequest = createLocalSomfyTahomaEventFetchRequest(hardwareManager()->networkManager(), getHost(thing), getToken(thing), eventListenerId, this);
            connect(eventFetchRequest, &SomfyTahomaRequest::error, thing, [this, thing](QNetworkReply::NetworkError error){
                qCWarning(dcSomfyTahoma()) << "Failed to fetch events:" << error;
                markDisconnected(thing);
            });
            connect(eventFetchRequest, &SomfyTahomaRequest::finished, thing, [this, thing](const QVariant &result){
                thing->setStateValue(gatewayConnectedStateTypeId, true);
                restoreChildConnectedState(thing);
                handleEvents(result.toList());
            });
        });
    });
}

void IntegrationPluginSomfyTahoma::thingRemoved(Thing *thing)
{
    m_eventPollTimer.remove(thing);

    if (thing->thingClassId() != gatewayThingClassId) {
        return;
    }

    // Unregister local token from cloud account.
    pluginStorage()->beginGroup(thing->id().toString());
    QString username = pluginStorage()->value("username").toString();
    QString password = pluginStorage()->value("password").toString();
    QString gatewayPin = thing->paramValue(gatewayThingGatewayPinParamTypeId).toString();
    QString tokenName = "nymea_" + thing->id().toString();
    pluginStorage()->endGroup();

    SomfyTahomaRequest *request = createCloudSomfyTahomaLoginRequest(hardwareManager()->networkManager(), username, password, this);
    connect(request, &SomfyTahomaRequest::error, this, [](){
        qCWarning(dcSomfyTahoma()) << "Failed to login to Somfy TaHoma.";
    });
    connect(request, &SomfyTahomaRequest::finished, this, [this, gatewayPin, tokenName](const QVariant &/*result*/){
        SomfyTahomaRequest *request = createCloudSomfyTahomaGetRequest(hardwareManager()->networkManager(), "/config/" + gatewayPin + "/local/tokens/devmode", this);
        connect(request, &SomfyTahomaRequest::error, this, [](){
            qCWarning(dcSomfyTahoma()) << "Failed to get list of tokens.";
        });
        connect(request, &SomfyTahomaRequest::finished, this, [this, gatewayPin, tokenName](const QVariant &result){
            foreach (const QVariant &tokenVariant, result.toList()) {
                QVariantMap tokenMap = tokenVariant.toMap();
                QString label = tokenMap["label"].toString();
                QString uuid = tokenMap["uuid"].toString();
                if (label == tokenName) {
                    SomfyTahomaRequest *request = createCloudSomfyTahomaDeleteRequest(hardwareManager()->networkManager(), "/config/" + gatewayPin + "/local/tokens/" + uuid, this);
                    connect(request, &SomfyTahomaRequest::error, this, [](){
                        qCWarning(dcSomfyTahoma()) << "Failed to remove token.";
                    });
                }
            }
        });
    });
}

void IntegrationPluginSomfyTahoma::handleEvents(const QVariantList &events)
{
    Thing *thing;
    foreach (const QVariant &eventVariant, events) {
        QVariantMap eventMap = eventVariant.toMap();

        QString device = eventMap["deviceURL"].toString();
        thing = myThings().findByParams(ParamList() << Param(rollershutterThingDeviceUrlParamTypeId, eventMap["deviceURL"]));
        if (thing) {
            device = thing->name();
        }
        thing = myThings().findByParams(ParamList() << Param(venetianblindThingDeviceUrlParamTypeId, eventMap["deviceURL"]));
        if (thing) {
            device = thing->name();
        }
        thing = myThings().findByParams(ParamList() << Param(garagedoorThingDeviceUrlParamTypeId, eventMap["deviceURL"]));
        if (thing) {
            device = thing->name();
        }
        thing = myThings().findByParams(ParamList() << Param(awningThingDeviceUrlParamTypeId, eventMap["deviceURL"]));
        if (thing) {
            device = thing->name();
        }
        thing = myThings().findByParams(ParamList() << Param(lightThingDeviceUrlParamTypeId, eventMap["deviceURL"]));
        if (thing) {
            device = thing->name();
        }
        thing = myThings().findByParams(ParamList() << Param(smokedetectorThingDeviceUrlParamTypeId, eventMap["deviceURL"]));
        if (thing) {
            device = thing->name();
        }
        qCDebug(dcSomfyTahoma()) << "Got event" << eventMap["name"].toString() << "for device" << device;
        qCDebug(dcSomfyTahoma()) << qUtf8Printable(QJsonDocument::fromVariant(eventVariant).toJson());

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
    thing = myThings().findByParams(ParamList() << Param(smokedetectorThingDeviceUrlParamTypeId, deviceUrl));
    if (thing) {
        foreach (const QVariant &stateVariant, stateList) {
            QVariantMap stateMap = stateVariant.toMap();
            if (stateMap["name"] == "core:SmokeState") {
                thing->setStateValue(smokedetectorFireDetectedStateTypeId, stateMap["value"] == "detected");
            } else if (stateMap["name"] == "core:MaintenanceRadioPartBatteryState") {
                QString radioBattery = stateMap["value"].toString();
                pluginStorage()->beginGroup(thing->id().toString());
                pluginStorage()->setValue("radioBatteryState", stateMap["value"]);
                QString sensorBattery = pluginStorage()->value("sensorBatteryState", "normal").toString();
                pluginStorage()->endGroup();
                if (radioBattery == "normal" && sensorBattery == "normal") {
                    thing->setStateValue(smokedetectorBatteryCriticalStateTypeId, false);
                } else {
                    qCWarning(dcSomfyTahoma()) << "Smoke Detector" << thing->name() << " radio battery is low!";
                    thing->setStateValue(smokedetectorBatteryCriticalStateTypeId, true);
                }
            } else if (stateMap["name"] == "core:MaintenanceSensorPartBatteryState") {
                QString sensorBattery = stateMap["value"].toString();
                pluginStorage()->beginGroup(thing->id().toString());
                pluginStorage()->setValue("radioBatteryState", stateMap["value"]);
                QString radioBattery = pluginStorage()->value("radioBatteryState", "normal").toString();
                pluginStorage()->endGroup();
                if (radioBattery == "normal" && sensorBattery == "normal") {
                    thing->setStateValue(smokedetectorBatteryCriticalStateTypeId, false);
                } else {
                    qCWarning(dcSomfyTahoma()) << "Smoke Detector" << thing->name() << " sensor battery is low!";
                    thing->setStateValue(smokedetectorBatteryCriticalStateTypeId, true);
                }
            } else if (stateMap["name"] == "core:StatusState") {
                thing->setStateValue(smokedetectorConnectedStateTypeId, stateMap["value"] == "available");
                pluginStorage()->beginGroup(thing->id().toString());
                pluginStorage()->setValue("connected", stateMap["value"] == "available");
                pluginStorage()->endGroup();
            } else if (stateMap["name"] == "core:RSSILevelState") {
                thing->setStateValue(smokedetectorSignalStrengthStateTypeId, stateMap["value"]);
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
            actionName = "setClosure";
            actionParameters = { info->action().param(rollershutterPercentageActionPercentageParamTypeId).value().toInt() };
        } else if (info->action().actionTypeId() == rollershutterOpenActionTypeId) {
            actionName = "open";
        } else if (info->action().actionTypeId() == rollershutterCloseActionTypeId) {
            actionName = "close";
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
        SomfyTahomaRequest *request = createLocalSomfyTahomaPostRequest(hardwareManager()->networkManager(), getHost(info->thing()), getToken(info->thing()), "/exec/apply", "application/json", jsonRequest.toJson(QJsonDocument::Compact), this);
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

void IntegrationPluginSomfyTahoma::markDisconnected(Thing *thing)
{
    if (thing->thingClassId() == gatewayThingClassId) {
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
    } else if (thing->thingClassId() == smokedetectorThingClassId) {
        thing->setStateValue(smokedetectorConnectedStateTypeId, false);
    }
    foreach (Thing *child, myThings().filterByParentId(thing->id())) {
        markDisconnected(child);
    }
}

void IntegrationPluginSomfyTahoma::restoreChildConnectedState(Thing *thing)
{
    pluginStorage()->beginGroup(thing->id().toString());
    if (pluginStorage()->contains("connected")) {
        if (thing->thingClassId() == rollershutterThingClassId) {
            thing->setStateValue(rollershutterConnectedStateTypeId, pluginStorage()->value("connected").toBool());
        } else if (thing->thingClassId() == venetianblindThingClassId) {
            thing->setStateValue(venetianblindConnectedStateTypeId, pluginStorage()->value("connected").toBool());
        } else if (thing->thingClassId() == garagedoorThingClassId) {
            thing->setStateValue(garagedoorConnectedStateTypeId, pluginStorage()->value("connected").toBool());
        } else if (thing->thingClassId() == awningThingClassId) {
            thing->setStateValue(awningConnectedStateTypeId, pluginStorage()->value("connected").toBool());
        } else if (thing->thingClassId() == lightThingClassId) {
            thing->setStateValue(lightConnectedStateTypeId, pluginStorage()->value("connected").toBool());
        } else if (thing->thingClassId() == smokedetectorThingClassId) {
            thing->setStateValue(smokedetectorConnectedStateTypeId, pluginStorage()->value("connected").toBool());
        }
    }
    pluginStorage()->endGroup();
    foreach (Thing *child, myThings().filterByParentId(thing->id())) {
        restoreChildConnectedState(child);
    }
}

QString IntegrationPluginSomfyTahoma::getHost(Thing *thing) const
{
    Thing *gateway = thing;
    if (!thing->parentId().isNull()) {
        gateway = myThings().findById(thing->parentId());
    }

    QString gatewayPin = gateway->paramValue(gatewayThingGatewayPinParamTypeId).toString();
    ZeroConfServiceEntry zeroConfEntry;
    foreach (const ZeroConfServiceEntry &entry, m_zeroConfBrowser->serviceEntries()) {
        if (gatewayPin == entry.txt("gateway_pin")) {
            zeroConfEntry = entry;
        }
    }
    QString host;
    pluginStorage()->beginGroup(gateway->id().toString());
    if (zeroConfEntry.isValid()) {
        host = zeroConfEntry.hostAddress().toString() + ":" + QString::number(zeroConfEntry.port());
        pluginStorage()->setValue("cachedAddress", host);
    } else if (pluginStorage()->contains("cachedAddress")){
        host = pluginStorage()->value("cachedAddress").toString();
    } else {
        qCWarning(dcSomfyTahoma()) << "Unable to determine IP address for:" << gatewayPin;
    }
    pluginStorage()->endGroup();

    return host;
}

QString IntegrationPluginSomfyTahoma::getToken(Thing *thing) const
{
    Thing *gateway = thing;
    if (!thing->parentId().isNull()) {
        gateway = myThings().findById(thing->parentId());
    }

    QString token;
    pluginStorage()->beginGroup(gateway->id().toString());
    token = pluginStorage()->value("token").toString();
    pluginStorage()->endGroup();
    return token;
}
