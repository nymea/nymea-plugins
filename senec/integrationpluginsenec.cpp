/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*
* Copyright 2013 - 2025, nymea GmbH
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

#include "integrationpluginsenec.h"
#include "plugininfo.h"

#include <QJsonDocument>
#include <QJsonParseError>

IntegrationPluginSenec::IntegrationPluginSenec()
{
    // Testing the convert methods
    // QString rawValue = "fl_42A2E5E4"; // 81.449
    // float value = SenecStorageLan::parseFloat(rawValue);
    // qCWarning(dcSenec()) << rawValue << value;

    // QString rawValue = "st_foobar";
    // QString value = SenecStorageLan::parseString(rawValue);
    // qCWarning(dcSenec()) << rawValue << value;

}

IntegrationPluginSenec::~IntegrationPluginSenec()
{

}

void IntegrationPluginSenec::discoverThings(ThingDiscoveryInfo *info)
{
    if (info->thingClassId() == senecConnectionThingClassId) {

        if (!hardwareManager()->networkDeviceDiscovery()->available()) {
            qCWarning(dcSenec()) << "Failed to discover network devices. The network device discovery is not available.";
            info->finish(Thing::ThingErrorHardwareNotAvailable, QT_TR_NOOP("Unable to discover devices in your network."));
            return;
        }

        // qCInfo(dcESPSomfyRTS()) << "Starting network discovery...";
        // EspSomfyRtsDiscovery *discovery = new EspSomfyRtsDiscovery(hardwareManager()->networkManager(), hardwareManager()->networkDeviceDiscovery(), info);
        // connect(discovery, &EspSomfyRtsDiscovery::discoveryFinished, info, [=](){
        //     ThingDescriptors descriptors;
        //     qCInfo(dcESPSomfyRTS()) << "Discovery finished. Found" << discovery->results().count() << "devices";
        //     foreach (const EspSomfyRtsDiscovery::Result &result, discovery->results()) {
        //         qCInfo(dcESPSomfyRTS()) << "Discovered device on" << result.networkDeviceInfo;

        //         QString title = "ESP Somfy RTS (" + result.name + ")";
        //         QString description = result.networkDeviceInfo.address().toString();

        //         ThingDescriptor descriptor(espSomfyRtsThingClassId, title, description);

        //         ParamList params;
        //         params << Param(espSomfyRtsThingMacAddressParamTypeId, result.networkDeviceInfo.thingParamValueMacAddress());
        //         params << Param(espSomfyRtsThingHostNameParamTypeId, result.networkDeviceInfo.thingParamValueHostName());
        //         params << Param(espSomfyRtsThingAddressParamTypeId, result.networkDeviceInfo.thingParamValueAddress());
        //         descriptor.setParams(params);

        //         // Check if we already have set up this device
        //         Thing *existingThing = myThings().findByParams(params);
        //         if (existingThing) {
        //             qCDebug(dcESPSomfyRTS()) << "This thing already exists in the system:" << result.networkDeviceInfo;
        //             descriptor.setThingId(existingThing->id());
        //         }

        //         info->addThingDescriptor(descriptor);
        //     }

        //     info->finish(Thing::ThingErrorNoError);
        // });

        // discovery->startDiscovery();
    }
}

void IntegrationPluginSenec::startPairing(ThingPairingInfo *info)
{
    info->finish(Thing::ThingErrorNoError, QT_TR_NOOP("Please enter username and password for your SENEC.home account."));
}

void IntegrationPluginSenec::confirmPairing(ThingPairingInfo *info, const QString &username, const QString &secret)
{
    if (info->thingClassId() == senecAccountThingClassId) {

        qCDebug(dcSenec()) << "Start logging in" << username << secret.left(2) + QString(secret.length() - 2, '*');

        QVariantMap requestMap;
        requestMap.insert("username", username);
        requestMap.insert("password", secret);

        QNetworkRequest request(SenecAccount::loginUrl());
        request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

        QNetworkReply *reply = hardwareManager()->networkManager()->post(request, QJsonDocument::fromVariant(requestMap).toJson(QJsonDocument::Indented));
        connect(reply, &QNetworkReply::finished, reply, &QNetworkReply::deleteLater);
        connect(reply, &QNetworkReply::finished, this, [reply, info, username, this] {

            int status = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
            // Check HTTP status code
            if (status != 200 || reply->error() != QNetworkReply::NoError) {
                qCWarning(dcSenec()) << "Login request finished with error. Status:" << status << "Error:" << reply->errorString();
                info->finish(Thing::ThingErrorAuthenticationFailure, QT_TR_NOOP("Username or password is invalid."));
                return;
            }

            // Note: as of now (API 4.4.3) the login seems to return a static token, which does not require any refresh.
            // Not as bad as saving user and password on the device, but almost ...

            // https://documenter.getpostman.com/view/932140/2s9YXib2td

            QByteArray responseData = reply->readAll();

            QJsonParseError jsonError;
            QVariantMap responseMap = QJsonDocument::fromJson(responseData, &jsonError).toVariant().toMap();
            if (jsonError.error != QJsonParseError::NoError) {
                qCWarning(dcSenec()) << "Login request finished successfully, but the response contains invalid JSON object:" << responseData;
                info->finish(Thing::ThingErrorAuthenticationFailure);
                return;
            }

            if (!responseMap.contains("token") || !responseMap.contains("refreshToken")) {
                qCWarning(dcSenec()) << "Login request finished successfully, but the response JSON does not contain the expected properties" << qUtf8Printable(responseData);
                info->finish(Thing::ThingErrorHardwareFailure);
                return;
            }

            QString token = responseMap.value("token").toString();
            QString refreshToken = responseMap.value("refreshToken").toString();

            pluginStorage()->beginGroup(info->thingId().toString());
            pluginStorage()->setValue("username", username);
            pluginStorage()->setValue("token", token);
            pluginStorage()->setValue("refreshToken", refreshToken);
            pluginStorage()->endGroup();

            info->finish(Thing::ThingErrorNoError);
        });
    }
}

void IntegrationPluginSenec::setupThing(ThingSetupInfo *info)
{
    Thing *thing = info->thing();
    qCDebug(dcSenec()) << "Setting up" << thing->name() << thing->params();

    if (thing->thingClassId() == senecAccountThingClassId) {
        // Load login information
        pluginStorage()->beginGroup(thing->id().toString());
        QString token = pluginStorage()->value("token").toString();
        QString refreshToken = pluginStorage()->value("refreshToken").toString();
        QString username = pluginStorage()->value("username").toString();
        pluginStorage()->endGroup();

        SenecAccount *account = new SenecAccount(hardwareManager()->networkManager(), username, token, refreshToken, this);
        m_accounts.insert(thing, account);
        info->finish(Thing::ThingErrorNoError);

        thing->setStateValue(senecAccountUserDisplayNameStateTypeId, username);

    } else if (thing->thingClassId() == senecStorageThingClassId) {

        connect(thing, &Thing::settingChanged, this, [this, thing](const ParamTypeId &paramTypeId, const QVariant &value){
            if (paramTypeId == senecStorageSettingsCapacityParamTypeId) {
                thing->setStateValue(senecStorageCapacityStateTypeId, value.toDouble());
            } else if (paramTypeId == senecStorageSettingsAddMeterParamTypeId) {

                if (value.toBool()) {
                    // Check if we have to add the meter
                    if (myThings().filterByThingClassId(senecMeterThingClassId).filterByParentId(thing->id()).isEmpty()) {
                        qCDebug(dcSenec()) << "Add meter for" << thing->name();
                        emit autoThingsAppeared(ThingDescriptors() << ThingDescriptor(senecMeterThingClassId, "SENEC Meter", QString(), thing->id()));
                    }
                } else {
                    // Check if we have to remove the meter
                    Things existingMeters = myThings().filterByThingClassId(senecMeterThingClassId).filterByParentId(thing->id());
                    if (!existingMeters.isEmpty()) {
                        qCDebug(dcSenec()) << "Remove meter thing for" << thing->name();
                        emit autoThingDisappeared(existingMeters.takeFirst()->id());
                    }
                }
            }
        });

        info->finish(Thing::ThingErrorNoError);

    } else if (thing->thingClassId() == senecMeterThingClassId) {
        info->finish(Thing::ThingErrorNoError);
    }
}

void IntegrationPluginSenec::postSetupThing(Thing *thing)
{
    if (thing->thingClassId() == senecAccountThingClassId) {

        // Search for now things, first poll the
        SenecAccount *account = m_accounts.value(thing);

        // Check installation, create things if not already created
        QNetworkReply *reply = account->getSystems();
        connect(reply, &QNetworkReply::finished, reply, &QNetworkReply::deleteLater);
        connect(reply, &QNetworkReply::finished, account, [reply, thing, this] {

            int status = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
            // Check HTTP status code
            if (status != 200 || reply->error() != QNetworkReply::NoError) {
                qCWarning(dcSenec()) << "Systems request finished with error. Status:" << status << "Error:" << reply->errorString();
                if (status == 401) {
                    qCWarning(dcSenec()) << "Authentication error, reconfigure and re-login should fix this problem.";
                    thing->setStateValue(senecAccountLoggedInStateTypeId, false);
                }

                thing->setStateValue(senecAccountConnectedStateTypeId, false);
                return;
            }

            QByteArray responseData = reply->readAll();

            QJsonParseError jsonError;
            QJsonDocument jsonDoc = QJsonDocument::fromJson(responseData, &jsonError);
            QVariant responseVariant = jsonDoc.toVariant();
            if (jsonError.error != QJsonParseError::NoError) {
                qCWarning(dcSenec()) << "Systems request finished successfully, but the response contains invalid JSON object:" << responseData;
                thing->setStateValue(senecAccountConnectedStateTypeId, false);
                return;
            }

            qCDebug(dcSenec()) << "Systems request finished successfully" << qUtf8Printable(jsonDoc.toJson());
            thing->setStateValue(senecAccountLoggedInStateTypeId, true);
            thing->setStateValue(senecAccountConnectedStateTypeId, true);

            ThingDescriptors descriptors;
            foreach (const QVariant &installation, responseVariant.toList()) {
                QVariantMap installationMap = installation.toMap();

                // Note: for now we only support V4 systems
                if (!installationMap.value("systemType").toString().toLower().contains("v4"))
                    continue;

                // This is a V4 storage, let's check if we already created one
                QString id = installationMap.value("id").toString();
                Things existingThings = myThings().filterByThingClassId(senecStorageThingClassId).filterByParam(senecStorageThingIdParamTypeId, id);
                if (existingThings.isEmpty()) {
                    qCDebug(dcSenec()) << "Creating new storage for" << id;
                    ThingDescriptor descriptor(senecStorageThingClassId, "SENEC.Home P4", id);
                    descriptor.setParentId(thing->id());
                    ParamList params;
                    params.append(Param(senecStorageThingIdParamTypeId, id));
                    descriptor.setParams(params);
                    descriptors.append(descriptor);
                } else {
                    qCDebug(dcSenec()) << "Thing for storage" << id << "already created.";
                }
            }

            if (!descriptors.isEmpty()) {
                qCDebug(dcSenec()) << "Adding" << descriptors.count() << "new SENEC.Home" << (descriptors.count() > 1 ? "storages" : "storage");
                emit autoThingsAppeared(descriptors);
            }
        });
    } else if (thing->thingClassId() == senecStorageThingClassId) {

        thing->setStateValue(senecStorageCapacityStateTypeId, thing->setting(senecStorageSettingsCapacityParamTypeId).toDouble());

        SenecAccount *account = m_accounts.value(myThings().findById(thing->parentId()));
        QString id = thing->paramValue(senecStorageThingIdParamTypeId).toString();

        QNetworkReply *reply = account->getTechnicalData(id);
        connect(reply, &QNetworkReply::finished, reply, &QNetworkReply::deleteLater);
        connect(reply, &QNetworkReply::finished, account, [reply, thing, this] {

            int status = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
            // Check HTTP status code
            if (status != 200 || reply->error() != QNetworkReply::NoError) {
                qCWarning(dcSenec()) << "Technical data request finished with error. Status:" << status << "Error:" << reply->errorString();
                thing->setStateValue(senecStorageConnectedStateTypeId, false);
                return;
            }

            QByteArray responseData = reply->readAll();

            QJsonParseError jsonError;
            QJsonDocument jsonDoc = QJsonDocument::fromJson(responseData, &jsonError);
            //QVariantMap responseMap = jsonDoc.toVariant().toMap();
            if (jsonError.error != QJsonParseError::NoError) {
                qCWarning(dcSenec()) << "Technical data request finished successfully, but the response contains invalid JSON object:" << responseData;
                return;
            }

            qCDebug(dcSenec()) << "Technical data request finished successfully" << qUtf8Printable(jsonDoc.toJson());
            thing->setStateValue(senecStorageConnectedStateTypeId, true);

            refresh(thing);
        });
    }

    // Create the refresh timer if not already set up
    if (!m_refreshTimer) {
        qCDebug(dcSenec()) << "Starting refresh timer ...";
        m_refreshTimer = hardwareManager()->pluginTimerManager()->registerTimer(5);
        connect(m_refreshTimer, &PluginTimer::timeout, this, [this](){
            refresh();
        });
        m_refreshTimer->start();
    }
}

void IntegrationPluginSenec::thingRemoved(Thing *thing)
{
    if (thing->thingClassId() == senecAccountThingClassId) {
        if (m_accounts.contains(thing))
            m_accounts.take(thing)->deleteLater();

        // Wipe any stored login information
        pluginStorage()->beginGroup(thing->id().toString());
        pluginStorage()->remove("");
        pluginStorage()->endGroup();
    }

    if (myThings().isEmpty()) {
        qCDebug(dcSenec()) << "Stopping refresh timer";
        hardwareManager()->pluginTimerManager()->unregisterTimer(m_refreshTimer);
        m_refreshTimer = nullptr;
    }
}

void IntegrationPluginSenec::executeAction(ThingActionInfo *info)
{
    info->finish(Thing::ThingErrorNoError);
}

void IntegrationPluginSenec::refresh(Thing *thing)
{
    // If no thing given, refresh all storages recursive
    if (!thing) {
        foreach (Thing *storageThing, myThings().filterByThingClassId(senecStorageThingClassId)) {
            refresh(storageThing);
        }

        return;
    }

    Thing *parentThing = myThings().findById(thing->parentId());
    SenecAccount *account = m_accounts.value(parentThing);
    QString id = thing->paramValue(senecStorageThingIdParamTypeId).toString();

    QNetworkReply *reply = account->getDashboard(id);
    connect(reply, &QNetworkReply::finished, reply, &QNetworkReply::deleteLater);
    connect(reply, &QNetworkReply::finished, account, [reply, thing, this] {

        // Check if we have a meter
        Thing *meterThing = nullptr;
        Things meterThings = myThings().filterByThingClassId(senecMeterThingClassId).filterByParentId(thing->id());
        if (!meterThings.isEmpty()) {
            meterThing = meterThings.first();
        }

        int status = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
        // Check HTTP status code
        if (status != 200 || reply->error() != QNetworkReply::NoError) {
            qCWarning(dcSenec()) << "Dashboard request finished with error. Status:" << status << "Error:" << reply->errorString();
            thing->setStateValue(senecStorageConnectedStateTypeId, false);
            if (meterThing) {
                meterThing->setStateValue(senecMeterConnectedStateTypeId, false);
            }
            return;
        }

        QByteArray responseData = reply->readAll();

        QJsonParseError jsonError;
        QJsonDocument jsonDoc = QJsonDocument::fromJson(responseData, &jsonError);
        QVariantMap responseMap = jsonDoc.toVariant().toMap();
        if (jsonError.error != QJsonParseError::NoError) {
            qCWarning(dcSenec()) << "Dashboard request finished successfully, but the response contains invalid JSON object:" << responseData;
            return;
        }

        qCDebug(dcSenec()) << "Dashboard request finished successfully" << qUtf8Printable(jsonDoc.toJson());
        thing->setStateValue(senecStorageConnectedStateTypeId, true);

        QVariantMap currentDataMap = responseMap.value("currently").toMap();
        float batteryCharge = qRound(currentDataMap.value("batteryChargeInW").toFloat() * 100) / 100.0;
        float batteryDischarge = qRound(currentDataMap.value("batteryDischargeInW").toFloat() * 100) / 100.0;
        int batteryLevel = currentDataMap.value("batteryLevelInPercent").toInt();

        // qCDebug(dcSenec()) << "charge:" << batteryCharge << "W" << "discharge:" << batteryDischarge << "W" << "level" << batteryLevel << "%";

        // Note: there are some situations where the battery is charging and discharging at the same time.
        // In that case we use the bigger power. Maybe we cloudl als sum them up, that should be tested...

        float currentPower = 0;
        if (batteryCharge != 0 && batteryDischarge != 0) {
            if (batteryCharge > batteryDischarge) {
                currentPower = batteryCharge;
            } else {
                currentPower = -batteryDischarge;
            }
        } else if (batteryCharge != 0) {
            currentPower = batteryCharge;
        } else if (batteryDischarge != 0) {
            currentPower = -batteryDischarge;
        }

        if (currentPower > 0) {
            thing->setStateValue(senecStorageChargingStateStateTypeId, "charging");
        } else if (currentPower < 0) {
            thing->setStateValue(senecStorageChargingStateStateTypeId, "discharging");
        } else {
            thing->setStateValue(senecStorageChargingStateStateTypeId, "idle");
        }

        thing->setStateValue(senecStorageCurrentPowerStateTypeId, currentPower);
        thing->setStateValue(senecStorageBatteryLevelStateTypeId, batteryLevel);
        thing->setStateValue(senecStorageBatteryCriticalStateTypeId, batteryLevel < 10);

        // Check if we have a meter
        if (meterThing) {
            float gridConsume = qRound(currentDataMap.value("gridDrawInW").toFloat() * 100) / 100.0;
            float gridReturn = qRound(currentDataMap.value("gridFeedInInW").toFloat() * 100) / 100.0;

            qCDebug(dcSenec()) << "Grid power: consume" << gridConsume << "W" << "return:" << gridReturn << "W";

            double currentPower = 0;

            if (gridConsume != 0) {
                currentPower = gridConsume;
            } else if (gridReturn != 0) {
                currentPower = -gridReturn;
            }

            meterThing->setStateValue(senecMeterCurrentPowerStateTypeId, currentPower);
            meterThing->setStateValue(senecMeterConnectedStateTypeId, true);
        }
    });
}
