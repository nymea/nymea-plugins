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

#include "integrationpluginnetatmo.h"
#include "integrations/thing.h"
#include "plugininfo.h"
#include "network/networkaccessmanager.h"

#include <QDebug>
#include <QUrlQuery>
#include <QJsonDocument>

IntegrationPluginNetatmo::IntegrationPluginNetatmo()
{

}

void IntegrationPluginNetatmo::startPairing(ThingPairingInfo *info)
{
    info->finish(Thing::ThingErrorNoError, QT_TR_NOOP("Please enter the login credentials for your Netatmo account."));
}

void IntegrationPluginNetatmo::confirmPairing(ThingPairingInfo *info, const QString &username, const QString &password)
{
    OAuth2 *authentication = new OAuth2("561c015d49c75f0d1cce6e13", "GuvKkdtu7JQlPD47qTTepRR9hQ0CUPAj4Tae3Ohcq", this);
    authentication->setUrl(QUrl("https://api.netatmo.net/oauth2/token"));
    authentication->setUsername(username);
    authentication->setPassword(password);
    authentication->setScope("read_station read_thermostat write_thermostat");

    connect(authentication, &OAuth2::authenticationChanged, info, [this, info, username, password, authentication](){
        if (authentication->authenticated()) {
            pluginStorage()->beginGroup(info->thingId().toString());
            pluginStorage()->setValue("username", username);
            pluginStorage()->setValue("password", password);
            pluginStorage()->endGroup();
            info->finish(Thing::ThingErrorNoError);
        } else {
            info->finish(Thing::ThingErrorAuthenticationFailure, "Wrong username of password");
        }
    });
    authentication->startAuthentication();
    connect(info, &QObject::destroyed, authentication, &OAuth2::deleteLater);
}

void IntegrationPluginNetatmo::setupThing(ThingSetupInfo *info)
{
    Thing *thing = info->thing();

    if (thing->thingClassId() == netatmoConnectionThingClassId) {
        qCDebug(dcNetatmo) << "Setup netatmo connection" << thing->name() << thing->params();

        QString username;
        QString password;

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

            pluginStorage()->beginGroup(info->thing()->id().toString());
            pluginStorage()->setValue("username", username);
            pluginStorage()->setValue("password", password);
            pluginStorage()->endGroup();

            // Delete username and password so it wont be visible in the things.conf file.
            thing->setParamValue(ParamTypeId("763c2c10-dee5-41c8-9f7e-ded741945e73"), "");
            thing->setParamValue(ParamTypeId("c0d892d6-f359-4782-9d7d-8f74a3b53e3e"), "");
        }

        OAuth2 *authentication = new OAuth2("561c015d49c75f0d1cce6e13", "GuvKkdtu7JQlPD47qTTepRR9hQ0CUPAj4Tae3Ohcq", this);
        authentication->setUrl(QUrl("https://api.netatmo.net/oauth2/token"));
        authentication->setUsername(username);
        authentication->setPassword(password);
        authentication->setScope("read_station read_thermostat write_thermostat");
        m_authentications.insert(authentication, thing);

        // Update thing connected state based on OAuth connected state
        connect(authentication, &OAuth2::authenticationChanged, thing, [this, thing, authentication](){
            thing->setStateValue(netatmoConnectionConnectedStateTypeId, authentication->authenticated());
            if (authentication->authenticated()) {
                refreshData(thing, authentication->token());
            }
        });
        authentication->startAuthentication();

        // Report thing setup finished when authentication reports success
        connect(authentication, &OAuth2::authenticationChanged, info, [info, authentication](){
            if (!authentication->authenticated()) {
                authentication->deleteLater();
                info->finish(Thing::ThingErrorAuthenticationFailure, QT_TR_NOOP("Error logging in to Netatmo server."));
                return;
            }

            info->finish(Thing::ThingErrorNoError);
        });

        return;

    } else if (thing->thingClassId() == indoorThingClassId) {
        qCDebug(dcNetatmo) << "Setup netatmo indoor base station" << thing->params();
        NetatmoBaseStation *indoor = new NetatmoBaseStation(thing->paramValue(indoorThingNameParamTypeId).toString(),
                                                            thing->paramValue(indoorThingMacParamTypeId).toString(),
                                                            this);

        m_indoorDevices.insert(indoor, thing);
        connect(indoor, SIGNAL(statesChanged()), this, SLOT(onIndoorStatesChanged()));

        return info->finish(Thing::ThingErrorNoError);
    } else if (thing->thingClassId() == outdoorThingClassId) {
        qCDebug(dcNetatmo) << "Setup netatmo outdoor module" << thing->params();


        // Migrate thing parameters after changing param type UUIDs in 0.14.
        QMap<QString, ParamTypeId> migrationMap;
        migrationMap.insert("a97d256c-e159-4aa0-bc71-6bd7cd0688b3", outdoorThingNameParamTypeId);
        migrationMap.insert("157d470a-e579-4d0e-b879-6b5bfa8e34ae", outdoorThingMacParamTypeId);

        ParamList migratedParams;
        foreach (const Param &oldParam, thing->params()) {
            QString oldId = oldParam.paramTypeId().toString();
            oldId.remove(QRegExp("[{}]"));
            if (migrationMap.contains(oldId)) {
                ParamTypeId newId = migrationMap.value(oldId);
                QVariant oldValue = oldParam.value();
                qCDebug(dcNetatmo()) << "Migrating netatmo outdoor station param:" << oldId << "->" << newId << ":" << oldValue;
                Param newParam(newId, oldValue);
                migratedParams << newParam;
            } else {
                migratedParams << oldParam;
            }
        }
        thing->setParams(migratedParams);
        // Migration done

        NetatmoOutdoorModule *outdoor = new NetatmoOutdoorModule(thing->paramValue(outdoorThingNameParamTypeId).toString(),
                                                                 thing->paramValue(outdoorThingMacParamTypeId).toString(),
                                                                 thing->paramValue(outdoorThingBaseStationParamTypeId).toString(),
                                                                 this);

        m_outdoorDevices.insert(outdoor, thing);
        connect(outdoor, SIGNAL(statesChanged()), this, SLOT(onOutdoorStatesChanged()));

        return info->finish(Thing::ThingErrorNoError);
    }
    qCWarning(dcNetatmo()) << "Unhandled thing class in setupDevice";
}

void IntegrationPluginNetatmo::thingRemoved(Thing *thing)
{
    if (thing->thingClassId() == netatmoConnectionThingClassId) {
        OAuth2 * authentication = m_authentications.key(thing);
        m_authentications.remove(authentication);
        authentication->deleteLater();
    } else if (thing->thingClassId() == indoorThingClassId) {
        NetatmoBaseStation *indoor = m_indoorDevices.key(thing);
        m_indoorDevices.remove(indoor);
        indoor->deleteLater();
    } else if (thing->thingClassId() == outdoorThingClassId) {
        NetatmoOutdoorModule *outdoor = m_outdoorDevices.key(thing);
        m_outdoorDevices.remove(outdoor);
        outdoor->deleteLater();
    }

    if (myThings().isEmpty() && m_pluginTimer) {
        hardwareManager()->pluginTimerManager()->unregisterTimer(m_pluginTimer);
        m_pluginTimer = nullptr;
    }
}

void IntegrationPluginNetatmo::postSetupThing(Thing *thing)
{
    if (thing->thingClassId() == indoorThingClassId) {
        QString stationId = thing->paramValue(indoorThingMacParamTypeId).toString();
        if (m_indoorStationInitData.contains(stationId) && m_indoorDevices.values().contains(thing)) {
            m_indoorDevices.key(thing)->updateStates(m_indoorStationInitData.take(stationId));
        }
    } else if (thing->thingClassId() == outdoorThingClassId) {
        QString stationId = thing->paramValue(outdoorThingMacParamTypeId).toString();
        if (m_outdoorStationInitData.contains(stationId) && m_outdoorDevices.values().contains(thing)) {
            m_outdoorDevices.key(thing)->updateStates(m_outdoorStationInitData.take(stationId));
        }
    }

    if (!m_pluginTimer) {
        m_pluginTimer = hardwareManager()->pluginTimerManager()->registerTimer(600);
        connect(m_pluginTimer, &PluginTimer::timeout, this, &IntegrationPluginNetatmo::onPluginTimer);
    }
}

void IntegrationPluginNetatmo::refreshData(Thing *thing, const QString &token)
{
    QUrlQuery query;
    query.addQueryItem("access_token", token);

    QUrl url("https://api.netatmo.com/api/getstationsdata");
    url.setQuery(query);

    QNetworkReply *reply = hardwareManager()->networkManager()->get(QNetworkRequest(url));
    connect(reply, &QNetworkReply::finished, this, &IntegrationPluginNetatmo::onNetworkReplyFinished);

    m_refreshRequest.insert(reply, thing);
}

void IntegrationPluginNetatmo::processRefreshData(const QVariantMap &data, Thing *connectionDevice)
{
    // Process the data
    if (data.contains("body")) {
        // check devices
        if (data.value("body").toMap().contains("devices")) {
            QVariantList deviceList = data.value("body").toMap().value("devices").toList();
            // check devices
            foreach (QVariant deviceVariant, deviceList) {
                QVariantMap deviceMap = deviceVariant.toMap();
                // we support currently only NAMain devices
                if (deviceMap.value("type").toString() == "NAMain") {
                    Thing *indoorDevice = findIndoorDevice(deviceMap.value("_id").toString());
                    // check if we have to create the thing (auto)
                    if (!indoorDevice) {
                        ThingDescriptor descriptor(indoorThingClassId, deviceMap.value("module_name").toString(), deviceMap.value("station_name").toString(), connectionDevice->id());
                        ParamList params;
                        params.append(Param(indoorThingNameParamTypeId, deviceMap.value("station_name").toString()));
                        params.append(Param(indoorThingMacParamTypeId, deviceMap.value("_id").toString()));
                        descriptor.setParams(params);
                        m_indoorStationInitData.insert(deviceMap.value("_id").toString(), deviceMap);
                        emit autoThingsAppeared({descriptor});
                    } else {
                        if (m_indoorDevices.values().contains(indoorDevice)) {
                            m_indoorDevices.key(indoorDevice)->updateStates(deviceMap);
                        }
                    }
                }

                // check modules
                if (deviceMap.contains("modules")) {
                    QVariantList modulesList = deviceMap.value("modules").toList();
                    foreach (QVariant moduleVariant, modulesList) {
                        QVariantMap moduleMap = moduleVariant.toMap();
                        // we support currently only NAModule1
                        if (moduleMap.value("type").toString() == "NAModule1") {
                            Thing *outdoorDevice = findOutdoorDevice(moduleMap.value("_id").toString());

                            // check if we have to create the thing (auto)
                            if (!outdoorDevice) {
                                ThingDescriptor descriptor(outdoorThingClassId, "Outdoor Module", moduleMap.value("module_name").toString(), connectionDevice->id());
                                ParamList params;
                                params.append(Param(outdoorThingNameParamTypeId, moduleMap.value("module_name").toString()));
                                params.append(Param(outdoorThingMacParamTypeId, moduleMap.value("_id").toString()));
                                params.append(Param(outdoorThingBaseStationParamTypeId, deviceMap.value("_id").toString()));
                                descriptor.setParams(params);
                                m_outdoorStationInitData.insert(moduleMap.value("_id").toString(), moduleMap);
                                emit autoThingsAppeared({descriptor});
                            } else {
                                if (m_outdoorDevices.values().contains(outdoorDevice)) {
                                    m_outdoorDevices.key(outdoorDevice)->updateStates(moduleMap);
                                }
                            }
                        }
                    }
                }
            }
        }
    }
}

Thing *IntegrationPluginNetatmo::findIndoorDevice(const QString &macAddress)
{
    foreach (Thing *thing, myThings()) {
        if (thing->thingClassId() == indoorThingClassId) {
            if (thing->paramValue(indoorThingMacParamTypeId).toString() == macAddress) {
                return thing;
            }
        }
    }
    return nullptr;
}

Thing *IntegrationPluginNetatmo::findOutdoorDevice(const QString &macAddress)
{
    foreach (Thing *thing, myThings()) {
        if (thing->thingClassId() == outdoorThingClassId) {
            if (thing->paramValue(outdoorThingMacParamTypeId).toString() == macAddress) {
                return thing;
            }
        }
    }
    return nullptr;
}

void IntegrationPluginNetatmo::onPluginTimer()
{
    foreach (OAuth2 *authentication, m_authentications.keys()) {
        if (authentication->authenticated()) {
            refreshData(m_authentications.value(authentication), authentication->token());
        } else {
            authentication->startAuthentication();
        }
    }
}

void IntegrationPluginNetatmo::onNetworkReplyFinished()
{
    QNetworkReply *reply = static_cast<QNetworkReply *>(sender());
    reply->deleteLater();

    int status = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
    // update values request
    if (m_refreshRequest.keys().contains(reply)) {
        Thing *thing = m_refreshRequest.take(reply);

        // check HTTP status code
        if (status != 200) {
            qCWarning(dcNetatmo) << "Device list reply HTTP error:" << status << reply->errorString();
            thing->setStateValue(netatmoConnectionConnectedStateTypeId, false);
            return;
        }
        thing->setStateValue(netatmoConnectionConnectedStateTypeId, true);
        // check JSON file
        QJsonParseError error;
        QJsonDocument jsonDoc = QJsonDocument::fromJson(reply->readAll(), &error);
        if (error.error != QJsonParseError::NoError) {
            qCWarning(dcNetatmo) << "Device list reply JSON error:" << error.errorString();
            return;
        }

        qCDebug(dcNetatmo) << qUtf8Printable(jsonDoc.toJson());
        processRefreshData(jsonDoc.toVariant().toMap(), thing);
    }
}

void IntegrationPluginNetatmo::onIndoorStatesChanged()
{
    NetatmoBaseStation *indoor = static_cast<NetatmoBaseStation *>(sender());
    Thing *thing = m_indoorDevices.value(indoor);

    thing->setStateValue(indoorUpdateTimeStateTypeId, indoor->lastUpdate());
    thing->setStateValue(indoorTemperatureStateTypeId, indoor->temperature());
    thing->setStateValue(indoorTemperatureMinStateTypeId, indoor->minTemperature());
    thing->setStateValue(indoorTemperatureMaxStateTypeId, indoor->maxTemperature());
    thing->setStateValue(indoorPressureStateTypeId, indoor->pressure());
    thing->setStateValue(indoorHumidityStateTypeId, indoor->humidity());
    thing->setStateValue(indoorCo2StateTypeId, indoor->co2());
    thing->setStateValue(indoorNoiseStateTypeId, indoor->noise());
    thing->setStateValue(indoorWifiStrengthStateTypeId, indoor->wifiStrength());
}

void IntegrationPluginNetatmo::onOutdoorStatesChanged()
{
    NetatmoOutdoorModule *outdoor = static_cast<NetatmoOutdoorModule *>(sender());
    Thing *thing = m_outdoorDevices.value(outdoor);

    thing->setStateValue(outdoorUpdateTimeStateTypeId, outdoor->lastUpdate());
    thing->setStateValue(outdoorTemperatureStateTypeId, outdoor->temperature());
    thing->setStateValue(outdoorTemperatureMinStateTypeId, outdoor->minTemperature());
    thing->setStateValue(outdoorTemperatureMaxStateTypeId, outdoor->maxTemperature());
    thing->setStateValue(outdoorHumidityStateTypeId, outdoor->humidity());
    thing->setStateValue(outdoorSignalStrengthStateTypeId, outdoor->signalStrength());
    thing->setStateValue(outdoorBatteryLevelStateTypeId, outdoor->battery());
    thing->setStateValue(outdoorBatteryCriticalStateTypeId, outdoor->battery() < 10);
}


