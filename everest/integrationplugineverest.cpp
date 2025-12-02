// SPDX-License-Identifier: GPL-3.0-or-later

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*
* Copyright (C) 2013 - 2024, nymea GmbH
* Copyright (C) 2024 - 2025, chargebyte austria GmbH
*
* This file is part of nymea-plugins.
*
* nymea-plugins is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* (at your option) any later version.
*
* nymea-plugins is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
* General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with nymea-plugins. If not, see <https://www.gnu.org/licenses/>.
*
* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

#include "integrationplugineverest.h"
#include "plugininfo.h"

#include "mqtt/everestmqttdiscovery.h"
#include "jsonrpc/everestevse.h"
#include "jsonrpc/everestjsonrpcdiscovery.h"

#include <QFileInfo>

#include <nymeasettings.h>
#include <network/networkdevicediscovery.h>

IntegrationPluginEverest::IntegrationPluginEverest()
{

}

void IntegrationPluginEverest::init()
{
    QFileInfo enableMqttFileInfo(NymeaSettings::settingsPath() + "/everest-mqtt");
    m_useMqtt = enableMqttFileInfo.exists();
    if (m_useMqtt) {
        qCDebug(dcEverest()) << "MQTT API module autodetection enabled, found enable file" << enableMqttFileInfo.absoluteFilePath();
    } else {
        qCDebug(dcEverest()) << "MQTT API module autodetection disabled, the enable file" << enableMqttFileInfo.absoluteFilePath() << "does not exist.";
    }
}

void IntegrationPluginEverest::startMonitoringAutoThings()
{
    // Check localhost for a websocket RpcApi module

    // Let's check if we aleardy have a thing with those params
    if (myThings().filterByThingClassId(everestConnectionThingClassId).filterByParam(everestConnectionThingAddressParamTypeId, "127.0.0.1").isEmpty()) {

        // No localhost connection, trying to connect

        QUrl url;
        url.setScheme("ws");
        url.setHost("127.0.0.1");
        url.setPort(8080);

        qCDebug(dcEverest()) << "AutoSetup: There is no localhost everest connection set up. Checking everest RpcApi modules on" << url.toString();

        EverestJsonRpcClient *client = new EverestJsonRpcClient(this);
        connect(client, &EverestJsonRpcClient::availableChanged, this, [this, client](bool available){
            if (available) {
                qCDebug(dcEverest()) << "AutoSetup: Found JsonRpc interface on" << client->serverUrl().toString();

                QString title = QString("EVerest connection");
                ThingDescriptor descriptor(everestConnectionThingClassId, title);

                ParamList params;
                params.append(Param(everestConnectionThingAddressParamTypeId, client->serverUrl().host()));
                params.append(Param(everestConnectionThingMacAddressParamTypeId, QString()));
                params.append(Param(everestConnectionThingHostNameParamTypeId, QString()));
                params.append(Param(everestConnectionThingPortParamTypeId, client->serverUrl().port()));
                descriptor.setParams(params);

                qCDebug(dcEverest()) << "AutoSetup: Adding new localhost EVerest connection instances.";
                emit autoThingsAppeared(ThingDescriptors() << descriptor);

                client->disconnectFromServer();
                client->deleteLater();

                // Disable any auto setup retry logic...
                m_autodetectCounter = m_autodetectCounterLimit;
            }
        });

        connect(client, &EverestJsonRpcClient::connectionErrorOccurred, this, [this, client, url](){
            m_autodetectCounter++;

            if (m_autodetectCounter <= m_autodetectCounterLimit) {
                qCDebug(dcEverest()) << "AutoSetup: The connection to" << client->serverUrl().toString() << "failed. Retry" << m_autodetectCounter << "/" << m_autodetectCounterLimit << "in 15 seconds...";
                QTimer::singleShot(15000, client, [client, url](){
                    client->connectToServer(url);
                });
            } else {
                qCDebug(dcEverest()) << "AutoSetup: The connection to" << client->serverUrl().toString() << "failed after" << m_autodetectCounterLimit << "retries. Stopping AutoSetup.";
                client->disconnectFromServer();
                client->deleteLater();
            }
        });

        client->connectToServer(url);
    }

    if (m_useMqtt) {

        // Check on localhost if there is any EVerest instance running and if we have to set up a thing for this EV charger
        // Since this integration plugin is most liekly running on an EV charger running EVerest, the local instance should
        // be set up automatically. Additional instances in the network can still be added by running a normal network discovery

        EverestMqttDiscovery *mqttDiscovery = new EverestMqttDiscovery(nullptr, this);
        connect(mqttDiscovery, &EverestMqttDiscovery::finished, mqttDiscovery, &EverestMqttDiscovery::deleteLater);
        connect(mqttDiscovery, &EverestMqttDiscovery::finished, this, [this, mqttDiscovery](){

            ThingDescriptors descriptors;

            foreach (const EverestMqttDiscovery::Result &result, mqttDiscovery->results()) {

                // Create one EV charger foreach available connector on that host
                foreach(const QString &connectorName, result.connectors) {

                    QString title = QString("EVerest");
                    QString description = connectorName;
                    ThingDescriptor descriptor(everestMqttThingClassId, title, description);

                    qCInfo(dcEverest()) << "Discovered -->" << title << description;

                    ParamList params;
                    params.append(Param(everestMqttThingConnectorParamTypeId, connectorName));
                    params.append(Param(everestMqttThingAddressParamTypeId, result.networkDeviceInfo.address().toString()));
                    descriptor.setParams(params);

                    // Let's check if we aleardy have a thing with those params
                    bool thingExists = true;
                    Thing *existingThing = nullptr;
                    foreach (Thing *thing, myThings()) {
                        foreach(const Param &param, params) {
                            if (param.value() != thing->paramValue(param.paramTypeId())) {
                                thingExists = false;
                                break;
                            }
                        }

                        // The params are equal, we already have set up this thing
                        if (thingExists) {
                            existingThing = thing;
                        }
                    }

                    // Add only connectors we don't have set up yet
                    if (existingThing) {
                        qCDebug(dcEverest()) << "Discovered EVerest connector on localhost but we already set up this connector" << existingThing->name() << existingThing->params();
                    } else {
                        qCDebug(dcEverest()) << "Adding new EVerest connector on localhost" << title << params;
                        descriptors.append(descriptor);
                    }
                }
            }

            if (!descriptors.isEmpty()) {
                qCDebug(dcEverest()) << "Adding" << descriptors.count() << "new EVerest instances.";
                emit autoThingsAppeared(descriptors);
            }
        });

        mqttDiscovery->startLocalhost();
    }

}

void IntegrationPluginEverest::discoverThings(ThingDiscoveryInfo *info)
{
    qCDebug(dcEverest()) << "Start discovering Everest systems in the local network";
    if (!hardwareManager()->networkDeviceDiscovery()->available()) {
        qCWarning(dcEverest()) << "The network discovery is not available on this platform.";
        info->finish(Thing::ThingErrorUnsupportedFeature, QT_TR_NOOP("The network device discovery is not available."));
        return;
    }

    if (m_useMqtt) {
        if (info->thingClassId() == everestMqttThingClassId) {
            EverestMqttDiscovery *mqttDiscovery = new EverestMqttDiscovery(hardwareManager()->networkDeviceDiscovery(), this);
            connect(mqttDiscovery, &EverestMqttDiscovery::finished, mqttDiscovery, &EverestMqttDiscovery::deleteLater);
            connect(mqttDiscovery, &EverestMqttDiscovery::finished, info, [this, info, mqttDiscovery](){

                foreach (const EverestMqttDiscovery::Result &result, mqttDiscovery->results()) {

                    // Create one EV charger foreach available connector on that host
                    foreach(const QString &connectorName, result.connectors) {

                        QString title = QString("Everest (%1)").arg(connectorName);
                        QString description;
                        MacAddressInfo macInfo;

                        switch (result.networkDeviceInfo.monitorMode()) {
                        case NetworkDeviceInfo::MonitorModeMac:
                            macInfo = result.networkDeviceInfo.macAddressInfos().constFirst();
                            description = result.networkDeviceInfo.address().toString();
                            if (!macInfo.vendorName().isEmpty())
                                description += " - " + result.networkDeviceInfo.macAddressInfos().constFirst().vendorName();

                            break;
                        case NetworkDeviceInfo::MonitorModeHostName:
                            description = result.networkDeviceInfo.address().toString();
                            break;
                        case NetworkDeviceInfo::MonitorModeIp:
                            description = "Interface: " +  result.networkDeviceInfo.networkInterface().name();
                            break;
                        }

                        ThingDescriptor descriptor(everestMqttThingClassId, title, description);
                        qCInfo(dcEverest()) << "Discovered -->" << title << description;

                        // Note: the network device info already provides the correct set of parameters in order to be used by the monitor
                        // depending on the possibilities within this network. It is not recommended to fill in all information available.
                        // Only the information available depending on the monitor mode are relevant for the monitor.
                        ParamList params;
                        params.append(Param(everestMqttThingConnectorParamTypeId, connectorName));
                        params.append(Param(everestMqttThingMacAddressParamTypeId, result.networkDeviceInfo.thingParamValueMacAddress()));
                        params.append(Param(everestMqttThingHostNameParamTypeId, result.networkDeviceInfo.thingParamValueHostName()));
                        params.append(Param(everestMqttThingAddressParamTypeId, result.networkDeviceInfo.thingParamValueAddress()));
                        descriptor.setParams(params);

                        // Let's check if we aleardy have a thing with those params
                        bool thingExists = true;
                        Thing *existingThing = nullptr;
                        foreach (Thing *thing, myThings()) {
                            if (thing->thingClassId() != info->thingClassId())
                                continue;

                            foreach(const Param &param, params) {
                                if (param.value() != thing->paramValue(param.paramTypeId())) {
                                    thingExists = false;
                                    break;
                                }
                            }

                            // The params are equal, we already know this thing
                            if (thingExists)
                                existingThing = thing;
                        }

                        // Set the thing ID id we already have this device (for reconfiguration)
                        if (existingThing)
                            descriptor.setThingId(existingThing->id());

                        info->addThingDescriptor(descriptor);
                    }
                }

                // All discovery results processed, we are done
                info->finish(Thing::ThingErrorNoError);
            });

            mqttDiscovery->start();
            return;
        }
    }

    if (info->thingClassId() == everestConnectionThingClassId) {
        quint16 port = info->params().paramValue(everestConnectionDiscoveryPortParamTypeId).toUInt();
        EverestJsonRpcDiscovery *jsonRpcDiscovery = new EverestJsonRpcDiscovery(hardwareManager()->networkDeviceDiscovery(), port, this);
        connect(jsonRpcDiscovery, &EverestJsonRpcDiscovery::finished, jsonRpcDiscovery, &EverestJsonRpcDiscovery::deleteLater);
        connect(jsonRpcDiscovery, &EverestJsonRpcDiscovery::finished, info, [this, info, jsonRpcDiscovery, port](){

            foreach (const EverestJsonRpcDiscovery::Result &result, jsonRpcDiscovery->results()) {
                // Create one EV charger foreach available connector on that host
                QString title = QString("Everest connection");
                QString description;
                MacAddressInfo macInfo;

                switch (result.networkDeviceInfo.monitorMode()) {
                case NetworkDeviceInfo::MonitorModeMac:
                    macInfo = result.networkDeviceInfo.macAddressInfos().constFirst();
                    description = result.networkDeviceInfo.address().toString();
                    if (!macInfo.vendorName().isEmpty())
                        description += " - " + result.networkDeviceInfo.macAddressInfos().constFirst().vendorName();

                    break;
                case NetworkDeviceInfo::MonitorModeHostName:
                    description = result.networkDeviceInfo.address().toString();
                    break;
                case NetworkDeviceInfo::MonitorModeIp:
                    description = "Interface: " +  result.networkDeviceInfo.networkInterface().name();
                    break;
                }

                ThingDescriptor descriptor(everestConnectionThingClassId, title, description);
                qCInfo(dcEverest()) << "Discovered -->" << title << description;

                // Note: the network device info already provides the correct set of parameters in order to be used by the monitor
                // depending on the possibilities within this network. It is not recommended to fill in all information available.
                // Only the information available depending on the monitor mode are relevant for the monitor.
                ParamList params;
                params.append(Param(everestConnectionThingMacAddressParamTypeId, result.networkDeviceInfo.thingParamValueMacAddress()));
                params.append(Param(everestConnectionThingHostNameParamTypeId, result.networkDeviceInfo.thingParamValueHostName()));
                params.append(Param(everestConnectionThingAddressParamTypeId, result.networkDeviceInfo.thingParamValueAddress()));
                params.append(Param(everestConnectionThingPortParamTypeId, port));
                descriptor.setParams(params);

                // Let's check if we aleardy have a thing with those params
                bool thingExists = true;
                Thing *existingThing = nullptr;
                foreach (Thing *thing, myThings()) {
                    if (thing->thingClassId() != info->thingClassId())
                        continue;

                    foreach(const Param &param, params) {
                        if (param.value() != thing->paramValue(param.paramTypeId())) {
                            thingExists = false;
                            break;
                        }
                    }

                    // The params are equal, we already know this thing
                    if (thingExists)
                        existingThing = thing;
                }

                // Set the thing ID id we already have this device (for reconfiguration)
                if (existingThing)
                    descriptor.setThingId(existingThing->id());

                info->addThingDescriptor(descriptor);
            }

            // All discovery results processed, we are done
            info->finish(Thing::ThingErrorNoError);
        });

        jsonRpcDiscovery->start();
        return;
    }

    info->finish(Thing::ThingErrorUnsupportedFeature);
}

void IntegrationPluginEverest::setupThing(ThingSetupInfo *info)
{
    Thing *thing = info->thing();

    qCDebug(dcEverest()) << "Setting up" << thing << thing->params();

    if (thing->thingClassId() == everestMqttThingClassId) {

        QHostAddress address(thing->paramValue(everestMqttThingAddressParamTypeId).toString());
        MacAddress macAddress(thing->paramValue(everestMqttThingMacAddressParamTypeId).toString());
        QString hostName(thing->paramValue(everestMqttThingHostNameParamTypeId).toString());
        QString connector(thing->paramValue(everestMqttThingConnectorParamTypeId).toString());

        EverestMqttClient *everstClient = nullptr;

        foreach (EverestMqttClient *ec, m_everstMqttClients) {
            if (ec->monitor()->macAddress() == macAddress &&
                ec->monitor()->hostName() == hostName &&
                ec->monitor()->address() == address) {
                // We have already a client for this host
                qCDebug(dcEverest()) << "Using existing" << ec;
                everstClient = ec;
            }
        }

        if (!everstClient) {
            everstClient = new EverestMqttClient(this);
            everstClient->setMonitor(hardwareManager()->networkDeviceDiscovery()->registerMonitor(thing));
            m_everstMqttClients.append(everstClient);
            qCDebug(dcEverest()) << "Created new" << everstClient;
            everstClient->start();
        }

        everstClient->addThing(thing);
        m_thingClients.insert(thing, everstClient);
        info->finish(Thing::ThingErrorNoError);
        return;
    } else if (thing->thingClassId() == everestConnectionThingClassId) {

        quint16 port = thing->paramValue(everestConnectionThingPortParamTypeId).toUInt();

        EverestConnection *connection = nullptr;

        // Handle reconfigure
        if (m_everstConnections.contains(thing)) {
            connection = m_everstConnections.take(thing);
            hardwareManager()->networkDeviceDiscovery()->unregisterMonitor(connection->monitor());
            connection->deleteLater();
            connection = nullptr;
        }

        connection = new EverestConnection(port, this);
        connection->setMonitor(hardwareManager()->networkDeviceDiscovery()->registerMonitor(thing));
        m_everstConnections.insert(thing, connection);

        connect(connection, &EverestConnection::availableChanged, thing, [this, thing, connection](bool available){
            thing->setStateValue(everestConnectionConnectedStateTypeId, available);
            // Update connected state of child things
            foreach (Thing *child, myThings().filterByParentId(thing->id())) {
                child->setStateValue("connected", available);
            }

            if (available) {
                thing->setStateValue(everestConnectionApiVersionStateTypeId, connection->client()->apiVersion());

                // Sync things with availabe EvseInfos

                ThingDescriptors descriptors;
                qCDebug(dcEverest()) << "The client is now available, synching things...";
                foreach (const EverestJsonRpcClient::EVSEInfo &evseInfo, connection->client()->evseInfos()) {

                    // FIXME: somehow we need to now if this evse is AC or DC in order to spawn the right child thingclass.

                    // Check if we already have a child device for this index
                    bool alreadyAdded = false;
                    foreach (Thing *childThing, myThings().filterByParentId(thing->id())) {
                        if (childThing->paramValue("index").toInt() == evseInfo.index) {
                            qCDebug(dcEverest()) << "-> Found already added charger with index" << evseInfo.index;
                            alreadyAdded = true;
                            break;
                        }
                    }

                    if (!alreadyAdded) {
                        qCDebug(dcEverest()) << "-> Adding new discovered AC charger on" << connection->client()->serverUrl();
                        ThingDescriptor descriptor(everestChargerAcThingClassId, evseInfo.id, evseInfo.description, thing->id());
                        descriptor.setParams(ParamList() << Param(everestChargerAcThingIndexParamTypeId, evseInfo.index));
                        descriptors.append(descriptor);
                    }
                }

                // TODO: evaluate if any thing dissapeared

                if (!descriptors.isEmpty()) {
                    emit autoThingsAppeared(descriptors);
                }

            }
        });

        info->finish(Thing::ThingErrorNoError);

        connection->start();
        return;
    } else if (thing->thingClassId() == everestChargerAcThingClassId) {

        Thing *parentThing = myThings().findById(thing->parentId());
        EverestConnection *connection = m_everstConnections.value(parentThing);
        if (!connection) {
            info->finish(Thing::ThingErrorHardwareNotAvailable);
            return;
        }

        info->finish(Thing::ThingErrorNoError);

        connection->addThing(thing);
        return;
    }
}

void IntegrationPluginEverest::executeAction(ThingActionInfo *info)
{
    qCDebug(dcEverest()) << "Executing action for thing" << info->thing()
    << info->action().actionTypeId().toString() << info->action().params();

    if (info->thing()->thingClassId() == everestMqttThingClassId) {

        Thing *thing = info->thing();
        EverestMqttClient *everstClient = m_thingClients.value(thing);
        if (!everstClient) {
            qCWarning(dcEverest()) << "Failed to execute action. Unable to find everst client for" << thing;
            info->finish(Thing::ThingErrorHardwareFailure);
            return;
        }

        EverestMqtt *everest = everstClient->getEverest(thing);
        if (!everest) {
            qCWarning(dcEverest()) << "Failed to execute action. Unable to find everst for"
                                   << thing << "on" << everstClient;
            info->finish(Thing::ThingErrorHardwareFailure);
            return;
        }

        if (!thing->stateValue(everestMqttConnectedStateTypeId).toBool()) {
            info->finish(Thing::ThingErrorHardwareNotAvailable);
            return;
        }

        // All checks where good, let's execute the action
        if (info->action().actionTypeId() == everestMqttPowerActionTypeId) {
            bool power = info->action().paramValue(everestMqttPowerActionPowerParamTypeId).toBool();
            qCDebug(dcEverest()) << (power ? "Resume charging on" : "Pause charging on") << thing;
            everest->enableCharging(power);
            thing->setStateValue(everestMqttPowerStateTypeId, power);
            info->finish(Thing::ThingErrorNoError);
        } else if (info->action().actionTypeId() == everestMqttMaxChargingCurrentActionTypeId) {
            // Note: once we support phase switching, we cannot use the
            uint phaseCount = thing->stateValue(everestMqttDesiredPhaseCountStateTypeId).toUInt();
            double current = info->action().paramValue(everestMqttMaxChargingCurrentActionMaxChargingCurrentParamTypeId).toDouble();
            qCDebug(dcEverest()).nospace() << "Setting max charging current to " << current << "A (Phases: " << phaseCount << ") " << thing;
            everest->setMaxChargingCurrentAndPhaseCount(phaseCount, current);
            thing->setStateValue(everestMqttMaxChargingCurrentStateTypeId, current);
            info->finish(Thing::ThingErrorNoError);
        } else if (info->action().actionTypeId() == everestMqttDesiredPhaseCountActionTypeId) {
            uint phaseCount = info->action().paramValue(everestMqttDesiredPhaseCountActionDesiredPhaseCountParamTypeId).toUInt();
            double current = thing->stateValue(everestMqttMaxChargingCurrentStateTypeId).toDouble();
            qCDebug(dcEverest()).nospace() << "Setting desired phase count to " << phaseCount << " (" << current << "A) " << thing;
            everest->setMaxChargingCurrentAndPhaseCount(phaseCount, current);
            thing->setStateValue(everestMqttDesiredPhaseCountStateTypeId, phaseCount);
            info->finish(Thing::ThingErrorNoError);
        }

        return;
    } else if (info->thing()->thingClassId() == everestChargerAcThingClassId) {
        Thing *thing = info->thing();
        Thing *parentThing = myThings().findById(thing->parentId());
        EverestConnection *connection = m_everstConnections.value(parentThing);
        if (!connection) {
            info->finish(Thing::ThingErrorHardwareNotAvailable);
            return;
        }

        if (!thing->stateValue(everestChargerAcConnectedStateTypeId).toBool()) {
            info->finish(Thing::ThingErrorHardwareNotAvailable);
            return;
        }

        EverestEvse *evse = connection->getEvse(thing);
        if (!evse) {
            info->finish(Thing::ThingErrorHardwareNotAvailable);
            return;
        }

        if (info->action().actionTypeId() == everestChargerAcPowerActionTypeId) {
            bool power = info->action().paramValue(everestChargerAcPowerActionPowerParamTypeId).toBool();
            qCDebug(dcEverest()) << "Execute power action" << power;
            EverestJsonRpcReply *reply = evse->setChargingAllowed(power) ;
            connect(reply, &EverestJsonRpcReply::finished, reply, &EverestJsonRpcReply::deleteLater);
            connect(reply, &EverestJsonRpcReply::finished, this, [info, reply, power](){
                if (reply->error()) {
                    qCWarning(dcEverest()) << "Execute action reply finished with error" << reply->error();
                    info->finish(Thing::ThingErrorHardwareFailure);
                    return;
                }

                QVariantMap result = reply->response().value("result").toMap();
                EverestJsonRpcClient::ResponseError error = EverestJsonRpcClient::parseResponseError(result.value("error").toString());
                if (error) {
                    qCWarning(dcEverest()) << "Execute action reply finished with an error" << reply->method() << error;
                    info->finish(Thing::ThingErrorHardwareFailure);
                    return;
                }

                info->thing()->setStateValue(everestChargerAcCurrentPowerStateTypeId, power);
                info->finish(Thing::ThingErrorNoError);
            });
        } else if (info->action().actionTypeId() == everestChargerAcMaxChargingCurrentActionTypeId) {
            double current = info->action().paramValue(everestChargerAcMaxChargingCurrentActionMaxChargingCurrentParamTypeId).toDouble();
            qCDebug(dcEverest()) << "Execute action set max charging current" << current << "[A]";
            EverestJsonRpcReply *reply = evse->setACChargingCurrent(current) ;
            connect(reply, &EverestJsonRpcReply::finished, reply, &EverestJsonRpcReply::deleteLater);
            connect(reply, &EverestJsonRpcReply::finished, this, [info, reply, current](){
                if (reply->error()) {
                    qCWarning(dcEverest()) << "Execute action reply finished with error" << reply->error();
                    info->finish(Thing::ThingErrorHardwareFailure);
                    return;
                }

                QVariantMap result = reply->response().value("result").toMap();
                EverestJsonRpcClient::ResponseError error = EverestJsonRpcClient::parseResponseError(result.value("error").toString());
                if (error) {
                    qCWarning(dcEverest()) << "Execute action reply finished with an error" << reply->method() << error;
                    info->finish(Thing::ThingErrorHardwareFailure);
                    return;
                }

                info->thing()->setStateValue(everestChargerAcMaxChargingCurrentStateTypeId, current);
                info->finish(Thing::ThingErrorNoError);
            });
        } else if (info->action().actionTypeId() == everestChargerAcDesiredPhaseCountActionTypeId) {
            int phaseCount = info->action().paramValue(everestChargerAcDesiredPhaseCountActionDesiredPhaseCountParamTypeId).toInt();
            qCDebug(dcEverest()) << "Execute action set phase count" << phaseCount;
            EverestJsonRpcReply *reply = evse->setACChargingPhaseCount(phaseCount);
            connect(reply, &EverestJsonRpcReply::finished, reply, &EverestJsonRpcReply::deleteLater);
            connect(reply, &EverestJsonRpcReply::finished, this, [info, reply, phaseCount](){
                if (reply->error()) {
                    qCWarning(dcEverest()) << "Execute action reply finished with error" << reply->error();
                    info->finish(Thing::ThingErrorHardwareFailure);
                    return;
                }

                QVariantMap result = reply->response().value("result").toMap();
                EverestJsonRpcClient::ResponseError error = EverestJsonRpcClient::parseResponseError(result.value("error").toString());
                if (error) {
                    qCWarning(dcEverest()) << "Execute action reply finished with an error" << reply->method() << error;
                    info->finish(Thing::ThingErrorHardwareFailure);
                    return;
                }

                info->thing()->setStateValue(everestChargerAcDesiredPhaseCountStateTypeId, phaseCount);
                info->finish(Thing::ThingErrorNoError);
            });
        }

        return;
    }


    info->finish(Thing::ThingErrorNoError);
}

void IntegrationPluginEverest::thingRemoved(Thing *thing)
{
    qCDebug(dcEverest()) << "Remove thing" << thing;

    if (thing->thingClassId() == everestMqttThingClassId) {
        EverestMqttClient *everestClient = m_thingClients.take(thing);
        everestClient->removeThing(thing);
        if (everestClient->things().isEmpty()) {
            qCDebug(dcEverest()) << "Deleting" << everestClient << "since there is no thing left";
            // No more things related to this client, we can delete it
            m_everstMqttClients.removeAll(everestClient);

            // Unregister monitor
            if (everestClient->monitor())
                hardwareManager()->networkDeviceDiscovery()->unregisterMonitor(everestClient->monitor());

            everestClient->deleteLater();
        }
    } else if (thing->thingClassId() == everestConnectionThingClassId) {
        m_everstConnections.take(thing)->deleteLater();
    } else if (thing->thingClassId() == everestChargerAcThingClassId) {
        Thing *parentThing = myThings().findById(thing->parentId());
        EverestConnection *connection = m_everstConnections.value(parentThing);
        if (!connection)
            return;

        connection->removeThing(thing);
    }
}


