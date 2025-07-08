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

}

IntegrationPluginSenec::~IntegrationPluginSenec()
{

}

void IntegrationPluginSenec::startPairing(ThingPairingInfo *info)
{
    info->finish(Thing::ThingErrorNoError, QT_TR_NOOP("Please enter username and password for your SENEC.home account."));
}

void IntegrationPluginSenec::confirmPairing(ThingPairingInfo *info, const QString &username, const QString &secret)
{
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
    if (thing) {
        SenecAccount *account = m_accounts.value(myThings().findById(thing->parentId()));
        QString id = thing->paramValue(senecStorageThingIdParamTypeId).toString();
        QNetworkReply *reply = account->getDashboard(id);
        connect(reply, &QNetworkReply::finished, reply, &QNetworkReply::deleteLater);
        connect(reply, &QNetworkReply::finished, account, [reply, thing] {

            int status = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
            // Check HTTP status code
            if (status != 200 || reply->error() != QNetworkReply::NoError) {
                qCWarning(dcSenec()) << "Dashboard request finished with error. Status:" << status << "Error:" << reply->errorString();
                thing->setStateValue(senecStorageConnectedStateTypeId, false);
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

            float currentPower = 0;

            if (batteryCharge != 0) {
                currentPower = batteryCharge;
                thing->setStateValue(senecStorageChargingStateStateTypeId, "charging");
            } else if (batteryDischarge != 0) {
                currentPower = -batteryDischarge;
                thing->setStateValue(senecStorageChargingStateStateTypeId, "discharging");
            } else {
                thing->setStateValue(senecStorageChargingStateStateTypeId, "idle");
            }

            thing->setStateValue(senecStorageCurrentPowerStateTypeId, currentPower);
            thing->setStateValue(senecStorageBatteryLevelStateTypeId, batteryLevel);
            thing->setStateValue(senecStorageBatteryCriticalStateTypeId, batteryLevel < 10);
        });
    } else {
        foreach (Thing *storageThing, myThings().filterByThingClassId(senecStorageThingClassId)) {
            refresh(storageThing);
        }
    }
}
