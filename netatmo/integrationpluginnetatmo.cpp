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

#include "integrationpluginnetatmo.h"
#include "netatmoconnection.h"
#include "plugininfo.h"

#include <plugintimer.h>
#include <integrations/thing.h>
#include <network/networkaccessmanager.h>

#include <QDebug>
#include <QUrlQuery>
#include <QJsonDocument>

IntegrationPluginNetatmo::IntegrationPluginNetatmo()
{

}

void IntegrationPluginNetatmo::init()
{

}

void IntegrationPluginNetatmo::startPairing(ThingPairingInfo *info)
{
    if (!loadClientCredentials()) {
        info->finish(Thing::ThingErrorAuthenticationFailure, QT_TR_NOOP("No API key installed."));
        return;
    }

    // Compose url
    NetatmoConnection *connection = new NetatmoConnection(hardwareManager()->networkManager(), m_clientId, m_clientSecret, this);
    QUrl loginUrl = connection->getLoginUrl();

    // Checking the internet connect^ion
    QNetworkReply *reply = hardwareManager()->networkManager()->get(QNetworkRequest(QUrl("https://api.netatmo.net")));
    connect(reply, &QNetworkReply::finished, reply, &QNetworkReply::deleteLater);
    connect(reply, &QNetworkReply::finished, info, [this, reply, info, connection, loginUrl]() {
        //The server replies usually 404 not found on this request
        //int status = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
        if (reply->error() == QNetworkReply::NetworkError::HostNotFoundError) {
            qCWarning(dcNetatmo()) << "Netatmo server is not reachable";
            info->finish(Thing::ThingErrorSetupFailed, QT_TR_NOOP("The Netatmo server is not reachable."));
            return;
        }

        ThingId thingId = info->thingId();
        m_pendingSetups.insert(thingId, connection);
        connect(info, &ThingPairingInfo::aborted, connection, [thingId, this]() {
            qCWarning(dcNetatmo()) << "ThingPairingInfo aborted, cleaning up pending setup connection.";
            m_pendingSetups.take(thingId)->deleteLater();
        });

        qCDebug(dcNetatmo()) << "Netatmo server is reachable. Start the OAuth pairing process";
        info->setOAuthUrl(loginUrl);
        info->finish(Thing::ThingErrorNoError);
    });
}

void IntegrationPluginNetatmo::confirmPairing(ThingPairingInfo *info, const QString &username, const QString &secret)
{
    Q_UNUSED(username)

    qCDebug(dcNetatmo()) << "Confirm pairing" << info->thingName();
    if (info->thingClassId() == netatmoConnectionThingClassId) {
        QUrl url(secret);
        QUrlQuery query(url);
        QByteArray authorizationCode = query.queryItemValue("code").toLocal8Bit();
        if (authorizationCode.isEmpty()) {
            qCWarning(dcNetatmo()) << "Error while pairing to netatmo server. No authorization code received.";
            info->finish(Thing::ThingErrorAuthenticationFailure, QT_TR_NOOP("Failed to log in to the Netatmo server."));
            return;
        }

        NetatmoConnection *connection = m_pendingSetups.value(info->thingId());
        if (!connection) {
            qWarning(dcNetatmo()) << "No NetatmoConnect connection found for device:"  << info->thingName();
            m_pendingSetups.remove(info->thingId());
            info->finish(Thing::ThingErrorHardwareFailure);
            return;
        }

        connect(connection, &NetatmoConnection::receivedRefreshToken, info, [info, this](const QByteArray &refreshToken){
            qCDebug(dcNetatmo()) << "Received token:" << NetatmoConnection::censorDebugOutput(refreshToken);
            pluginStorage()->beginGroup(info->thingId().toString());
            pluginStorage()->setValue("refresh_token", refreshToken);
            pluginStorage()->endGroup();

            info->finish(Thing::ThingErrorNoError);
        });

        qCDebug(dcNetatmo()) << "Authorization code" << NetatmoConnection::censorDebugOutput(authorizationCode);
        if (!connection->getAccessTokenFromAuthorizationCode(authorizationCode)) {
            qCWarning(dcNetatmo()) << "Failed to get token from authorization code.";
            info->finish(Thing::ThingErrorAuthenticationFailure, QT_TR_NOOP("Failed to log in to the Netatmo server."));
            return;
        }

    }
}

void IntegrationPluginNetatmo::setupThing(ThingSetupInfo *info)
{
    Thing *thing = info->thing();

    if (thing->thingClassId() == netatmoConnectionThingClassId) {
        qCDebug(dcNetatmo) << "Setup Netatmo connection" << thing->name() << thing->params();

        if (!loadClientCredentials()) {
            info->finish(Thing::ThingErrorAuthenticationFailure, QT_TR_NOOP("No API key installed."));
            return;
        }

        NetatmoConnection *connection = nullptr;

        // Handle reconfigure
        if (m_connections.contains(thing)) {
            qCDebug(dcNetatmo()) << "Setup after reconfiguration, cleaning up";
            m_connections.take(thing)->deleteLater();
        }

        if (m_pendingSetups.keys().contains(thing->id())) {
            // This thing setup is after a pairing process
            qCDebug(dcNetatmo()) << "Netatmo available after successful OAuth2 setup. Using the existing object";
            connection = m_pendingSetups.take(thing->id());
            m_connections.insert(thing, connection);
            info->finish(Thing::ThingErrorNoError);

            thing->setStateValue("connected", true);
            thing->setStateValue(netatmoConnectionLoggedInStateTypeId, true);

            connect(connection, &NetatmoConnection::authenticatedChanged, thing, [thing](bool authenticated){
                thing->setStateValue(netatmoConnectionLoggedInStateTypeId, authenticated);
            });

        } else {
            // The thing should have been already set up before, lets refresh the token if we have a refresh token,
            // otherwise the user has to perform a reconfigure and login using OAuth2... (also old username password user end up here)

            if (doingLoginMigration(info))
                return;

            // No migration needed...
            setupConnection(info);
            return;
        }
        return;
    } else if (thing->thingClassId() == indoorThingClassId) {
        qCDebug(dcNetatmo) << "Setup Netatmo indoor base station" << thing->params();
        info->finish(Thing::ThingErrorNoError);
        return;
    } else if (thing->thingClassId() == outdoorThingClassId) {
        qCDebug(dcNetatmo) << "Setup netatmo outdoor module" << thing->params();
        info->finish(Thing::ThingErrorNoError);
        return;
    } else if (thing->thingClassId() == windModuleThingClassId) {
        qCDebug(dcNetatmo) << "Setup netatmo wind module" << thing->params();
        info->finish(Thing::ThingErrorNoError);
        return;
    } else if (thing->thingClassId() == rainGaugeThingClassId) {
        qCDebug(dcNetatmo) << "Setup netatmo wind module" << thing->params();
        info->finish(Thing::ThingErrorNoError);
        return;
    }  else if (thing->thingClassId() == indoorModuleThingClassId) {
        qCDebug(dcNetatmo) << "Setup netatmo indoor module" << thing->params();
        info->finish(Thing::ThingErrorNoError);
        return;
    } else {
        Q_ASSERT_X(false, "setupThing", QString("Unhandled thingClassId: %1").arg(info->thing()->thingClassId().toString()).toUtf8());
    }
}

void IntegrationPluginNetatmo::postSetupThing(Thing *thing)
{
    if (thing->thingClassId() == netatmoConnectionThingClassId) {
        refreshConnection(thing);
    } else if (thing->thingClassId() == indoorThingClassId) {
        QString stationId = thing->paramValue(indoorThingMacParamTypeId).toString();
        if (m_temporaryInitData.contains(stationId)) {
            updateModuleStates(thing, m_temporaryInitData.take(stationId));
        }
    } else if (thing->thingClassId() == outdoorThingClassId) {
        QString stationId = thing->paramValue(outdoorThingMacParamTypeId).toString();
        if (m_temporaryInitData.contains(stationId)) {
            updateModuleStates(thing, m_temporaryInitData.take(stationId));
        }
    }

    if (!m_pluginTimer) {
        m_pluginTimer = hardwareManager()->pluginTimerManager()->registerTimer(600);
        connect(m_pluginTimer, &PluginTimer::timeout, this, [this](){
            foreach (Thing *connectionThing, myThings().filterByThingClassId(netatmoConnectionThingClassId)) {
                refreshConnection(connectionThing);
            }
        });
    }
}

void IntegrationPluginNetatmo::thingRemoved(Thing *thing)
{
    if (thing->thingClassId() == netatmoConnectionThingClassId) {
        m_connections.take(thing)->deleteLater();
    }

    if (myThings().isEmpty() && m_pluginTimer) {
        if (m_pluginTimer) {
            hardwareManager()->pluginTimerManager()->unregisterTimer(m_pluginTimer);
            m_pluginTimer = nullptr;
        }
    }
}

void IntegrationPluginNetatmo::setupConnection(ThingSetupInfo *info)
{
    Thing *thing = info->thing();

    qCDebug(dcNetatmo()) << "Setup netatmo account" << thing->name();

    // Load refresh token
    pluginStorage()->beginGroup(thing->id().toString());
    QByteArray refreshToken = pluginStorage()->value("refresh_token").toByteArray();
    pluginStorage()->endGroup();
    if (refreshToken.isEmpty()) {
        info->finish(Thing::ThingErrorAuthenticationFailure, QT_TR_NOOP("Could not authenticate on the server. Please reconfigure the connection."));
        return;
    }

    // Create new connection
    NetatmoConnection *connection = new NetatmoConnection(hardwareManager()->networkManager(), m_clientId, m_clientSecret, thing);
    connect(connection, &NetatmoConnection::authenticatedChanged, info, [this, info, thing, connection](bool authenticated){
        if (authenticated) {
            m_connections.insert(thing, connection);
            qCDebug(dcNetatmo()) << "Authenticated successfully the netatmo connection.";
            info->finish(Thing::ThingErrorNoError);
            thing->setStateValue("connected", true);
        } else {
            qCDebug(dcNetatmo()) << "Authentication process failed.";
            info->finish(Thing::ThingErrorAuthenticationFailure, QT_TR_NOOP("Authentication failed. Please reconfigure the connection."));
        }
    });

    connect(info, &ThingSetupInfo::aborted, connection, [this, thing, connection] {
        if (m_connections.contains(thing))
            m_connections.remove(thing);

        connection->deleteLater();
    });

    connect(connection, &NetatmoConnection::authenticatedChanged, thing, [thing](bool authenticated){
        thing->setStateValue(netatmoConnectionLoggedInStateTypeId, authenticated);
    });

    connection->getAccessTokenFromRefreshToken(refreshToken);
}

void IntegrationPluginNetatmo::refreshConnection(Thing *thing)
{
    qCDebug(dcNetatmo()) << "Refresh connection" << thing;

    NetatmoConnection *connection = m_connections.value(thing);
    if (!connection) {
        qCWarning(dcNetatmo()) << "Failed to refresh data. The connection object does not exist";
        return;
    }

    QNetworkReply *reply = connection->getStationData();
    connect(reply, &QNetworkReply::finished, reply, &QNetworkReply::deleteLater);
    connect(reply, &QNetworkReply::finished, this, [this, reply, thing]() {
        int status = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
        if (reply->error() != QNetworkReply::NoError) {
            qCWarning(dcNetatmo()) << "Failed to refresh station data. Network reply returned with error" << status << reply->errorString();
            thing->setStateValue("connected", false);
            return;
        }

        thing->setStateValue("connected", true);

        QJsonParseError error;
        QJsonDocument jsonDoc = QJsonDocument::fromJson(reply->readAll(), &error);
        if (error.error != QJsonParseError::NoError) {
            qCWarning(dcNetatmo()) << "OAuth2: Failed to get token. Refresh data reply JSON error:" << error.errorString();
            return;
        }

        QVariantMap responseMap = jsonDoc.toVariant().toMap();
        if (responseMap.value("status") != "ok") {
            qCWarning(dcNetatmo()) << "Refresh station data returned status not ok:" << responseMap.value("status").toString();
            return;
        }

        qCDebug(dcNetatmo()) << qUtf8Printable(jsonDoc.toJson(QJsonDocument::Indented));
        QVariantMap bodyMap = responseMap.value("body").toMap();
        thing->setStateValue("username", bodyMap.value("user").toMap().value("mail").toString());
        processRefreshData(thing, bodyMap.value("devices").toList());
    });
}

void IntegrationPluginNetatmo::processRefreshData(Thing *connectionThing, const QVariantList &devices)
{
    foreach (QVariant deviceVariant, devices) {
        QVariantMap deviceMap = deviceVariant.toMap();
        if (deviceMap.value("type").toString() == "NAMain") {
            Thing *existingThing = findIndoorStationThing(deviceMap.value("_id").toString());
            // Check if we have to create the thing (auto)
            if (!existingThing) {
                ThingDescriptor descriptor(indoorThingClassId, deviceMap.value("module_name").toString() + " " + deviceMap.value("station_name").toString(), QString(), connectionThing->id());
                ParamList params;
                params.append(Param(indoorThingMacParamTypeId, deviceMap.value("_id").toString()));
                descriptor.setParams(params);
                m_temporaryInitData.insert(deviceMap.value("_id").toString(), deviceMap);
                emit autoThingsAppeared({descriptor});
            } else {
                // Update the indoor station states
                updateModuleStates(existingThing, deviceMap);
            }
        }

        /* Check modules of this station...
         *
         * NAMain: Indoor station ("Temperature", "CO2", "Humidity", "Noise", "Pressure")
         * NAModule1: Outdoor module ("Temperature, Humidity")
         * NAModule2: Wind module ("Wind")
         * NAModule3: Rain gauge module ("Rain")
         * NAModule4: Indoor module ("Temperature, Humidity, CO2")
         */

        if (deviceMap.contains("modules")) {
            QVariantList modulesList = deviceMap.value("modules").toList();
            foreach (QVariant moduleVariant, modulesList) {
                QVariantMap moduleMap = moduleVariant.toMap();

                if (moduleMap.value("type").toString() == "NAModule1") {
                    Thing *existingThing = findOutdoorModuleThing(moduleMap.value("_id").toString());
                    if (!existingThing) {
                        ThingDescriptor descriptor(outdoorThingClassId, moduleMap.value("module_name").toString() + " " + moduleMap.value("station_name").toString(), QString(), connectionThing->id());
                        ParamList params;
                        params.append(Param(outdoorThingMacParamTypeId, moduleMap.value("_id").toString()));
                        params.append(Param(outdoorThingBaseStationParamTypeId, deviceMap.value("_id").toString()));
                        descriptor.setParams(params);
                        m_temporaryInitData.insert(moduleMap.value("_id").toString(), moduleMap);
                        emit autoThingsAppeared({descriptor});
                    } else {
                        updateModuleStates(existingThing, moduleMap);
                    }
                } else if (moduleMap.value("type").toString() == "NAModule2") {
                    Thing *existingThing = findWindModuleThing(moduleMap.value("_id").toString());
                    if (!existingThing) {
                        ThingDescriptor descriptor(windModuleThingClassId, moduleMap.value("module_name").toString() + " " + moduleMap.value("station_name").toString(), QString(), connectionThing->id());
                        ParamList params;
                        params.append(Param(windModuleThingMacParamTypeId, moduleMap.value("_id").toString()));
                        params.append(Param(windModuleThingBaseStationParamTypeId, deviceMap.value("_id").toString()));
                        descriptor.setParams(params);
                        m_temporaryInitData.insert(moduleMap.value("_id").toString(), moduleMap);
                        emit autoThingsAppeared({descriptor});
                    } else {
                        updateModuleStates(existingThing, moduleMap);
                    }
                } else if (moduleMap.value("type").toString() == "NAModule3") {
                    Thing *existingThing = findRainGaugeModuleThing(moduleMap.value("_id").toString());
                    if (!existingThing) {
                        ThingDescriptor descriptor(rainGaugeThingClassId, moduleMap.value("module_name").toString() + " " + moduleMap.value("station_name").toString(), QString(), connectionThing->id());
                        ParamList params;
                        params.append(Param(rainGaugeThingMacParamTypeId, moduleMap.value("_id").toString()));
                        params.append(Param(rainGaugeThingBaseStationParamTypeId, deviceMap.value("_id").toString()));
                        descriptor.setParams(params);
                        m_temporaryInitData.insert(moduleMap.value("_id").toString(), moduleMap);
                        emit autoThingsAppeared({descriptor});
                    } else {
                        updateModuleStates(existingThing, moduleMap);
                    }
                } else if (moduleMap.value("type").toString() == "NAModule4") {
                    Thing *existingThing = findIndoorModuleThing(moduleMap.value("_id").toString());
                    if (!existingThing) {
                        ThingDescriptor descriptor(indoorModuleThingClassId, moduleMap.value("module_name").toString() + " " + moduleMap.value("station_name").toString(), QString(), connectionThing->id());
                        ParamList params;
                        params.append(Param(indoorModuleThingMacParamTypeId, moduleMap.value("_id").toString()));
                        params.append(Param(indoorModuleThingBaseStationParamTypeId, deviceMap.value("_id").toString()));
                        descriptor.setParams(params);
                        m_temporaryInitData.insert(moduleMap.value("_id").toString(), moduleMap);
                        emit autoThingsAppeared({descriptor});
                    } else {
                        updateModuleStates(existingThing, moduleMap);
                    }
                }
            }
        }
    }
}

Thing *IntegrationPluginNetatmo::findIndoorStationThing(const QString &macAddress)
{
    foreach (Thing *thing, myThings().filterByThingClassId(indoorThingClassId)) {
        if (thing->paramValue(indoorThingMacParamTypeId).toString() == macAddress) {
            return thing;
        }
    }

    return nullptr;
}

Thing *IntegrationPluginNetatmo::findOutdoorModuleThing(const QString &macAddress)
{
    foreach (Thing *thing, myThings().filterByThingClassId(outdoorThingClassId)) {
        if (thing->paramValue(outdoorThingMacParamTypeId).toString() == macAddress) {
            return thing;
        }
    }

    return nullptr;
}

Thing *IntegrationPluginNetatmo::findIndoorModuleThing(const QString &macAddress)
{
    foreach (Thing *thing, myThings().filterByThingClassId(indoorModuleThingClassId)) {
        if (thing->paramValue(indoorModuleThingMacParamTypeId).toString() == macAddress) {
            return thing;
        }
    }

    return nullptr;
}

Thing *IntegrationPluginNetatmo::findWindModuleThing(const QString &macAddress)
{
    foreach (Thing *thing, myThings().filterByThingClassId(windModuleThingClassId)) {
        if (thing->paramValue(windModuleThingMacParamTypeId).toString() == macAddress) {
            return thing;
        }
    }

    return nullptr;
}

Thing *IntegrationPluginNetatmo::findRainGaugeModuleThing(const QString &macAddress)
{
    foreach (Thing *thing, myThings().filterByThingClassId(rainGaugeThingClassId)) {
        if (thing->paramValue(rainGaugeThingMacParamTypeId).toString() == macAddress) {
            return thing;
        }
    }

    return nullptr;
}

void IntegrationPluginNetatmo::updateModuleStates(Thing *thing, const QVariantMap &data)
{
    // check data timestamp
    if (data.contains("last_message"))
        thing->setStateValue("updateTime", data.value("last_message").toInt());

    // update dashboard data
    if (data.contains("dashboard_data")) {
        QVariantMap measurments = data.value("dashboard_data").toMap();
        if (measurments.contains("Temperature"))
            thing->setStateValue("temperature", measurments.value("Temperature").toDouble());

        if (measurments.contains("min_temp"))
            thing->setStateValue("temperatureMin", measurments.value("min_temp").toDouble());

        if (measurments.contains("max_temp"))
            thing->setStateValue("temperatureMax", measurments.value("max_temp").toDouble());

        if (measurments.contains("Humidity"))
            thing->setStateValue("humidity", measurments.value("Humidity").toInt());

        if (measurments.contains("AbsolutePressure"))
            thing->setStateValue("pressure", measurments.value("AbsolutePressure").toDouble());

        if (measurments.contains("CO2"))
            thing->setStateValue("co2", measurments.value("CO2").toInt());

        if (measurments.contains("Noise"))
            thing->setStateValue("noise", measurments.value("Noise").toInt());

        // Wind speed (given in km/s, we need m/s)
        if (measurments.contains("WindStrength"))
            thing->setStateValue("windSpeed", measurments.value("WindStrength").toDouble() / 3.6);

        if (measurments.contains("WindAngle"))
            thing->setStateValue("windDirection", measurments.value("WindAngle").toInt());

        // Rain
        if (measurments.contains("sum_rain_1"))
            thing->setStateValue("rainfallLastHour", measurments.value("sum_rain_1").toInt());

        if (measurments.contains("sum_rain_24"))
            thing->setStateValue("rainfallLastDay", measurments.value("sum_rain_24").toInt());

    }

    // Battery
    if (thing->thingClass().hasStateType("batteryLevel")) {
        if (data.contains("battery_percent")) {
            thing->setStateValue("batteryLevel", data.value("battery_percent").toInt());
        } else {
            // Fallback for older versions, calculate from voltage
            if (data.contains("battery_vp")) {
                int battery = data.value("battery_vp").toInt();
                if (battery >= 6000) {
                    thing->setStateValue("batteryLevel", 100);
                } else if (battery <= 3600) {
                    thing->setStateValue("batteryLevel", 0);
                } else {
                    int delta = battery - 3600;
                    thing->setStateValue("batteryLevel", qRound(100.0 * delta / 2400));
                }
            }
        }

        thing->setStateValue("batteryCritical", thing->stateValue("batteryLevel").toInt() < 10);
    }

    // Signal strength (RF, 90 low, 60 highest)
    if (data.contains("rf_status")) {
        int signalStrength = data.value("rf_status").toInt();
        if (signalStrength <= 60) {
            thing->setStateValue("signalStrength", 100);
        } else if (signalStrength >= 90) {
            thing->setStateValue("signalStrength", 0);
        } else {
            int delta = 30 - (signalStrength - 60);
            thing->setStateValue("signalStrength", qRound(100.0 * delta / 30.0));
        }
    }

    // Wifi status (86=bad, 56=good)"

    if (data.contains("wifi_status")) {
        int signalStrength = data.value("wifi_status").toInt();
        if (signalStrength <= 56) {
            thing->setStateValue("signalStrength", 100);
        } else if (signalStrength >= 86) {
            thing->setStateValue("signalStrength", 0);
        } else {
            int delta = 30 - (signalStrength - 56);
            thing->setStateValue("signalStrength", qRound(100.0 * delta / 30.0));
        }
    }

    // update reachable state
    if (data.contains("reachable"))
        thing->setStateValue("connected", data.value("reachable").toBool());
}

bool IntegrationPluginNetatmo::doingLoginMigration(ThingSetupInfo *info)
{
    // Returns true if we need to perform a login migration
    // 1. First we stored the username and password in thing params and then moved to the plugin storage
    // 2. Username and password login does not work any more since mid 2022 if you change the password, we need to make a propper oauth2 login

    Thing *thing = info->thing();
    QString username; QString password;
    if (pluginStorage()->childGroups().contains(thing->id().toString())) {
        pluginStorage()->beginGroup(thing->id().toString());
        username = pluginStorage()->value("username").toString();
        password = pluginStorage()->value("password").toString();
        pluginStorage()->endGroup();
    } else {
        /* username and password have been stored as thingParams,
         * this is to migrate the params to plug-in storage. */
        ParamTypeId usernameParamTypeId = ParamTypeId("763c2c10-dee5-41c8-9f7e-ded741945e73");
        ParamTypeId passwordParamTypeId = ParamTypeId("c0d892d6-f359-4782-9d7d-8f74a3b53e3e");

        username = thing->paramValue(usernameParamTypeId).toString();
        password = thing->paramValue(passwordParamTypeId).toString();

        // Delete username and password so it wont be visible in the things.conf file.
        thing->setParamValue(ParamTypeId("763c2c10-dee5-41c8-9f7e-ded741945e73"), "");
        thing->setParamValue(ParamTypeId("c0d892d6-f359-4782-9d7d-8f74a3b53e3e"), "");
    }

    if (username.isEmpty() || password.isEmpty()) {
        // No username and no password found from previouse setting...nothing to migrate here
        return false;
    }

    // We found username and password. If we would have done the migration, they would not be there any more

    qCDebug(dcNetatmo()) << "Found deprecated username and password in the settings. Performing migration to plain OAuth2...";

    // Create a connection, get the refresh token, delete the connection and continue with normal setup.
    NetatmoConnection *connection = new NetatmoConnection(hardwareManager()->networkManager(), m_clientId, m_clientSecret, thing);
    connect(info, &ThingSetupInfo::aborted, connection, &NetatmoConnection::deleteLater);
    connect(connection, &NetatmoConnection::authenticatedChanged, info, [this, info, thing, connection](bool authenticated){
        connection->deleteLater();
        if (authenticated) {
            pluginStorage()->beginGroup(thing->id().toString());
            pluginStorage()->setValue("refresh_token", connection->refreshToken());
            // Success, we can delete username and passord once and for all and keep only the refresh token
            pluginStorage()->remove("username");
            pluginStorage()->remove("password");
            pluginStorage()->endGroup();
            qCDebug(dcNetatmo()) << "Migration finished successfully. Continue with normal setup";
            setupConnection(info);
        } else {
            qCDebug(dcNetatmo()) << "Authentication process failed.";
            info->finish(Thing::ThingErrorAuthenticationFailure, QT_TR_NOOP("Authentication failed. Please reconfigure the connection."));
        }
    });

    connection->getAccessTokenFromUsernamePassword(username, password);
    return true;
}

bool IntegrationPluginNetatmo::loadClientCredentials()
{
    QByteArray clientId = configValue(netatmoPluginCustomClientIdParamTypeId).toByteArray();
    QByteArray clientSecret = configValue(netatmoPluginCustomClientSecretParamTypeId).toByteArray();
    if (clientId.isEmpty() || clientSecret.isEmpty()) {
        clientId = apiKeyStorage()->requestKey("netatmo").data("clientId");
        clientSecret = apiKeyStorage()->requestKey("netatmo").data("clientSecret");
    } else {
        qCDebug(dcNetatmo()) << "Using custom client  id and secret from plugin configuration.";
    }

    if (clientId.isEmpty() || clientSecret.isEmpty()) {
        qCWarning(dcNetatmo()) << "No API key installed. Please install a valid api key provider plugin.";
        return false;
    } else {
        qCDebug(dcNetatmo()) << "Using API client secret and key from API key provider";
    }

    m_clientId = clientId;
    m_clientSecret = clientSecret;

    qCDebug(dcNetatmo()) << "API client ID" << NetatmoConnection::censorDebugOutput(m_clientId);
    qCDebug(dcNetatmo()) << "API client secret" << NetatmoConnection::censorDebugOutput(m_clientSecret);

    return true;
}
