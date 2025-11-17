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

#include "integrationpluginfronius.h"
#include "froniusdiscovery.h"
#include "plugininfo.h"

#include <plugintimer.h>

#include <QUrl>
#include <QDebug>
#include <QPointer>
#include <QUrlQuery>
#include <QJsonDocument>

// Notes: Test IPs: 93.82.221.82 | 88.117.152.99

IntegrationPluginFronius::IntegrationPluginFronius(QObject *parent): IntegrationPlugin(parent)
{

}

void IntegrationPluginFronius::discoverThings(ThingDiscoveryInfo *info)
{
    if (!hardwareManager()->networkDeviceDiscovery()->available()) {
        qCWarning(dcFronius()) << "Failed to discover network devices. The network device discovery is not available.";
        info->finish(Thing::ThingErrorHardwareNotAvailable, QT_TR_NOOP("Unable to discover devices in your network."));
        return;
    }

    qCInfo(dcFronius()) << "Starting network discovery...";
    FroniusDiscovery *discovery = new FroniusDiscovery(hardwareManager()->networkManager(), hardwareManager()->networkDeviceDiscovery(), info);
    connect(discovery, &FroniusDiscovery::discoveryFinished, info, [=](){
        ThingDescriptors descriptors;
        qCInfo(dcFronius()) << "Discovery finished. Found" << discovery->discoveryResults().count() << "devices";
        foreach (const NetworkDeviceInfo &networkDeviceInfo, discovery->discoveryResults()) {
            qCInfo(dcFronius()) << "Discovered Fronius on" << networkDeviceInfo;

            QString title;
            if (networkDeviceInfo.hostName().isEmpty()) {
                title += "Fronius Solar";
            } else {
                title += "Fronius Solar (" + networkDeviceInfo.hostName() + ")";
            }

            QString description;
            if (networkDeviceInfo.macAddressInfos().count() == 1) {
                MacAddressInfo macInfo = networkDeviceInfo.macAddressInfos().constFirst();
                if (macInfo.vendorName().isEmpty()) {
                    description = macInfo.macAddress().toString();
                } else {
                    description = macInfo.macAddress().toString() + " (" + macInfo.vendorName() + ")";
                }
            }

            ThingDescriptor descriptor(connectionThingClassId, title, description);
            ParamList params;
            params.append(Param(connectionThingMacAddressParamTypeId, networkDeviceInfo.thingParamValueMacAddress()));
            params.append(Param(connectionThingHostNameParamTypeId, networkDeviceInfo.thingParamValueHostName()));
            params.append(Param(connectionThingAddressParamTypeId, networkDeviceInfo.thingParamValueAddress()));
            descriptor.setParams(params);

            // Check if we already have set up this device
            Thing *existingThing = myThings().findByParams(params);
            if (existingThing) {
                qCDebug(dcFronius()) << "This thing already exists in the system." << existingThing;
                descriptor.setThingId(existingThing->id());
            }

            info->addThingDescriptor(descriptor);
        }
        info->finish(Thing::ThingErrorNoError);
    });

    discovery->startDiscovery();
}

void IntegrationPluginFronius::setupThing(ThingSetupInfo *info)
{
    Thing *thing = info->thing();
    qCDebug(dcFronius()) << "Setting up" << thing;

    if (thing->thingClassId() == connectionThingClassId) {

        // Handle reconfigure
        if (m_froniusConnections.values().contains(thing)) {
            FroniusSolarConnection *connection = m_froniusConnections.key(thing);
            m_froniusConnections.remove(connection);
            connection->deleteLater();
        }

        if (m_monitors.contains(thing))
            hardwareManager()->networkDeviceDiscovery()->unregisterMonitor(m_monitors.take(thing));

        NetworkDeviceMonitor *monitor = hardwareManager()->networkDeviceDiscovery()->registerMonitor(thing);
        if (!monitor) {
            qCWarning(dcFronius()) << "Unable to register monitor with the given params" << thing->params();
            info->finish(Thing::ThingErrorInvalidParameter, QT_TR_NOOP("Unable to set up the connection with this configuration, please reconfigure the connection."));
            return;
        }

        qCInfo(dcFronius()) << "Set up Fronius connection " << monitor;
        m_monitors.insert(thing, monitor);

        FroniusSolarConnection *connection = new FroniusSolarConnection(hardwareManager()->networkManager(), monitor->networkDeviceInfo().address(), thing);
        connect(monitor, &NetworkDeviceMonitor::networkDeviceInfoChanged, this, [=](const NetworkDeviceInfo &networkDeviceInfo){
            qCDebug(dcFronius()) << "Network device info changed for" << thing << networkDeviceInfo;
            if (networkDeviceInfo.isValid()) {
                connection->setAddress(networkDeviceInfo.address());
                refreshConnection(connection);
            } else {
                connection->setAddress(QHostAddress());
            }
        });

        connect(connection, &FroniusSolarConnection::availableChanged, this, [=](bool available){
            qCDebug(dcFronius()) << thing << "Available changed" << available;
            thing->setStateValue("connected", available);

            if (!available) {
                // Update all child things, they will be set to available once the connection starts working again
                foreach (Thing *childThing, myThings().filterByParentId(thing->id())) {
                    // Reset live data states in order to show missing information by 0 line and not by keeping the last known value
                    if (childThing->thingClassId() == inverterThingClassId) {
                        markInverterAsDisconnected(childThing);
                    } else if (childThing->thingClassId() == meterThingClassId) {
                        markMeterAsDisconnected(childThing);
                    } else if (childThing->thingClassId() == storageThingClassId) {
                        markStorageAsDisconnected(childThing);
                    }
                }
            }
        });

        if (info->isInitialSetup()) {
            // Verify the version
            FroniusNetworkReply *reply = connection->getVersion();
            connect(reply, &FroniusNetworkReply::finished, info, [=] {
                QByteArray data = reply->networkReply()->readAll();
                if (reply->networkReply()->error() != QNetworkReply::NoError) {
                    qCWarning(dcFronius()) << "Network request error:" << reply->networkReply()->error() << reply->networkReply()->errorString() << reply->networkReply()->url();
                    if (reply->networkReply()->error() == QNetworkReply::ContentNotFoundError) {
                        info->finish(Thing::ThingErrorHardwareNotAvailable, QT_TR_NOOP("The device does not reply to our requests. Please verify that the Fronius Solar API is enabled on the device."));
                    } else {
                        info->finish(Thing::ThingErrorHardwareNotAvailable, QT_TR_NOOP("The device is not reachable."));
                    }
                    return;
                }

                // Convert the rawdata to a JSON document
                QJsonParseError error;
                QJsonDocument jsonDoc = QJsonDocument::fromJson(data, &error);
                if (error.error != QJsonParseError::NoError) {
                    qCWarning(dcFronius()) << "Failed to parse JSON data" << data << ":" << error.errorString() << data;
                    info->finish(Thing::ThingErrorHardwareFailure, QT_TR_NOOP("The data received from the device could not be processed because the format is unknown."));
                    return;
                }

                QVariantMap versionResponseMap = jsonDoc.toVariant().toMap();
                qCDebug(dcFronius()) << "Compatibility version" << versionResponseMap.value("CompatibilityRange").toString();

                // Knwon version with broken JSON API
                if (versionResponseMap.value("CompatibilityRange").toString() == "1.6-2") {
                    qCWarning(dcFronius()) << "The Fronius data logger has a version which is known to have a broken JSON API firmware.";
                    info->finish(Thing::ThingErrorHardwareFailure, QT_TR_NOOP("The firmware version 1.6-2 of this Fronius data logger contains errors preventing proper operation. Please update your Fronius device and try again."));
                    return;
                }

                m_froniusConnections.insert(connection, thing);
                info->finish(Thing::ThingErrorNoError);

                // Update the already known states
                thing->setStateValue("connected", true);
                thing->setStateValue(connectionVersionStateTypeId, versionResponseMap.value("CompatibilityRange").toString());
            });
        } else {
            // Let the available state handle the connected state, this already worked once...
            m_froniusConnections.insert(connection, thing);
            info->finish(Thing::ThingErrorNoError);
        }
    } else if ((thing->thingClassId() == inverterThingClassId ||
                thing->thingClassId() == meterThingClassId ||
                thing->thingClassId() == storageThingClassId)) {

        // Verify the parent connection
        Thing *parentThing = myThings().findById(thing->parentId());
        if (!parentThing) {
            qCWarning(dcFronius()) << "Could not find the parent for" << thing;
            info->finish(Thing::ThingErrorHardwareNotAvailable);
            return;
        }

        FroniusSolarConnection *connection = m_froniusConnections.key(parentThing);
        if (!connection) {
            qCWarning(dcFronius()) << "Could not find the parent connection for" << thing;
            info->finish(Thing::ThingErrorHardwareNotAvailable);
            return;
        }

        info->finish(Thing::ThingErrorNoError);

    } else {
        Q_ASSERT_X(false, "setupThing", QString("Unhandled thingClassId: %1").arg(thing->thingClassId().toString()).toUtf8());
    }
}

void IntegrationPluginFronius::postSetupThing(Thing *thing)
{
    qCDebug(dcFronius()) << "Post setup" << thing->name();

    if (thing->thingClassId() == connectionThingClassId) {

        // Create a refresh timer for monitoring the active devices
        if (!m_connectionRefreshTimer) {
            m_connectionRefreshTimer = hardwareManager()->pluginTimerManager()->registerTimer(2);
            connect(m_connectionRefreshTimer, &PluginTimer::timeout, this, [this]() {
                foreach (FroniusSolarConnection *connection, m_froniusConnections.keys()) {
                    refreshConnection(connection);
                }
            });

            m_connectionRefreshTimer->start();
        }

        // Refresh now
        FroniusSolarConnection *connection = m_froniusConnections.key(thing);
        if (connection) {
            thing->setStateValue("connected", connection->available());
            refreshConnection(connection);
        }
    }
}

void IntegrationPluginFronius::thingRemoved(Thing *thing)
{
    if (thing->thingClassId() == connectionThingClassId) {
        if (m_froniusConnections.values().contains(thing)) {
            FroniusSolarConnection *connection = m_froniusConnections.key(thing);
            m_froniusConnections.remove(connection);
            connection->deleteLater();
        }

        if (m_monitors.contains(thing)) {
            hardwareManager()->networkDeviceDiscovery()->unregisterMonitor(m_monitors.take(thing));
        }
    }

    if (myThings().filterByThingClassId(connectionThingClassId).isEmpty()) {
        hardwareManager()->pluginTimerManager()->unregisterTimer(m_connectionRefreshTimer);
        m_connectionRefreshTimer = nullptr;
    }
}

void IntegrationPluginFronius::executeAction(ThingActionInfo *info)
{
    Q_UNUSED(info)
}

void IntegrationPluginFronius::refreshConnection(FroniusSolarConnection *connection)
{
    if (connection->busy()) {
        qCDebug(dcFronius()) << "The connection is busy. Skipping refresh cycle for host" << connection->address().toString();
        return;
    }

    if (connection->address().isNull()) {
        qCDebug(dcFronius()) << "The connection has no IP configured yet. Skipping refresh cycle until known";
        return;
    }

    // Note: this call will be used to monitor the available state of the connection internally
    FroniusNetworkReply *reply = connection->getActiveDevices();
    connect(reply, &FroniusNetworkReply::finished, this, [=]() {
        if (reply->networkReply()->error() != QNetworkReply::NoError) {
            // Note: the connection warns about any errors if available changed
            return;
        }

        Thing *connectionThing = m_froniusConnections.value(connection);
        if (!connectionThing)
            return;

        QByteArray data = reply->networkReply()->readAll();

        QJsonParseError error;
        QJsonDocument jsonDoc = QJsonDocument::fromJson(data, &error);
        if (error.error != QJsonParseError::NoError) {
            qCWarning(dcFronius()) << "Failed to parse JSON data" << data << ":" << error.errorString();
            return;
        }

        // Parse the data for thing information
        QList<ThingDescriptor> thingDescriptors;

        QVariantMap bodyMap = jsonDoc.toVariant().toMap().value("Body").toMap();
        //qCDebug(dcFronius()) << "System:" << qUtf8Printable(QJsonDocument::fromVariant(bodyMap).toJson());

        // Check if there are new inverters
        QVariantMap inverterMap = bodyMap.value("Data").toMap().value("Inverter").toMap();
        foreach (const QString &inverterId, inverterMap.keys()) {
            QVariantMap inverterInfo = inverterMap.value(inverterId).toMap();
            const QString serialNumber = inverterInfo.value("Serial").toString();

            // Note: we use the id to identify for backwards compatibility
            if (myThings().filterByParentId(connectionThing->id()).filterByParam(inverterThingIdParamTypeId, inverterId).isEmpty()) {
                QString thingDescription = connectionThing->name();
                ThingDescriptor descriptor(inverterThingClassId, "Fronius Solar Inverter", thingDescription, connectionThing->id());
                ParamList params;
                params.append(Param(inverterThingIdParamTypeId, inverterId));
                params.append(Param(inverterThingSerialNumberParamTypeId, serialNumber));
                descriptor.setParams(params);
                thingDescriptors.append(descriptor);
            }
        }

        // Check if there are new meters
        QVariantMap meterMap = bodyMap.value("Data").toMap().value("Meter").toMap();
        foreach (const QString &meterId, meterMap.keys()) {
            // Note: we use the id to identify for backwards compatibility
            if (myThings().filterByParentId(connectionThing->id()).filterByParam(meterThingIdParamTypeId, meterId).isEmpty()) {
                // Get the meter realtime data for details
                FroniusNetworkReply *realtimeDataReply = connection->getMeterRealtimeData(meterId.toInt());
                connect(realtimeDataReply, &FroniusNetworkReply::finished, this, [=]() {
                    if (realtimeDataReply->networkReply()->error() != QNetworkReply::NoError)
                        return;

                    QByteArray data = realtimeDataReply->networkReply()->readAll();

                    QJsonParseError error;
                    QJsonDocument jsonDoc = QJsonDocument::fromJson(data, &error);
                    if (error.error != QJsonParseError::NoError) {
                        qCWarning(dcFronius()) << "Meter: Failed to parse JSON data" << data << ":" << error.errorString();
                        return;
                    }

                    // Parse the data and update the states of our device
                    QVariantMap dataMap = jsonDoc.toVariant().toMap().value("Body").toMap().value("Data").toMap();
                    QString thingName;
                    QString serialNumber;
                    QString model;

                    if (dataMap.contains("Details")) {
                        QVariantMap details = dataMap.value("Details").toMap();
                        model = dataMap.value("Details").toMap().value("Model", "Smart Meter").toString();
                        thingName = details.value("Manufacturer", "Fronius").toString() + " " + model;
                        serialNumber = details.value("Serial").toString();
                    } else {
                        thingName = connectionThing->name() + " Meter " + meterId;
                    }

                    // Note: some inverters have a S0 meter connected, which measures the load, not the grid power and also provides only the current power and no additionl information
                    // Since we assume a meter on the grid root of the household, we don't update the meter here, but in the updatePowerFlow method.
                    if (model.toLower().contains("s0")) {
                        qCDebug(dcFronius()) << "Detected weak meter on inverter (S0). Using the plant overview grid power as meter information for this one.";
                        m_weakMeterConnections[connection] = true;
                    } else {
                        m_weakMeterConnections[connection] = false;
                    }

                    ThingDescriptor descriptor(meterThingClassId, thingName, QString(), connectionThing->id());
                    ParamList params;
                    params.append(Param(meterThingIdParamTypeId, meterId));
                    params.append(Param(meterThingSerialNumberParamTypeId, serialNumber));
                    descriptor.setParams(params);
                    emit autoThingsAppeared(ThingDescriptors() << descriptor);
                });
            }
        }

        // Check if there are new energy storages
        QVariantMap storageMap = bodyMap.value("Data").toMap().value("Storage").toMap();
        foreach (const QString &storageId, storageMap.keys()) {
            // Note: we use the id to identify for backwards compatibility
            if (myThings().filterByParentId(connectionThing->id()).filterByParam(storageThingIdParamTypeId, storageId).isEmpty()) {

                // Get the meter realtime data for details
                FroniusNetworkReply *realtimeDataReply = connection->getStorageRealtimeData(storageId.toInt());
                connect(realtimeDataReply, &FroniusNetworkReply::finished, this, [=]() {
                    if (realtimeDataReply->networkReply()->error() != QNetworkReply::NoError)
                        return;

                    QByteArray data = realtimeDataReply->networkReply()->readAll();

                    QJsonParseError error;
                    QJsonDocument jsonDoc = QJsonDocument::fromJson(data, &error);
                    if (error.error != QJsonParseError::NoError) {
                        qCWarning(dcFronius()) << "Storage: Failed to parse JSON data" << data << ":" << error.errorString();
                        return;
                    }

                    // Parse the data and update the states of our device
                    QVariantMap dataMap = jsonDoc.toVariant().toMap().value("Body").toMap().value("Data").toMap().value("Controller").toMap();

                    QString thingName;
                    QString serialNumber;
                    if (dataMap.contains("Details")) {
                        QVariantMap details = dataMap.value("Details").toMap();
                        thingName = details.value("Manufacturer", "Fronius").toString() + " " + details.value("Model", "Energy Storage").toString();
                        serialNumber = details.value("Serial").toString();
                    } else {
                        thingName = connectionThing->name() + " Storage " + storageId;
                    }

                    ThingDescriptor descriptor(storageThingClassId, thingName, QString(), connectionThing->id());
                    ParamList params;
                    params.append(Param(storageThingIdParamTypeId, storageId));
                    params.append(Param(storageThingSerialNumberParamTypeId, serialNumber));
                    descriptor.setParams(params);
                    emit autoThingsAppeared(ThingDescriptors() << descriptor);
                });
            }
        }

        // Inform about unhandled devices
        QVariantMap ohmpilotMap = bodyMap.value("Data").toMap().value("Ohmpilot").toMap();
        foreach (QString ohmpilotId, ohmpilotMap.keys()) {
            qCDebug(dcFronius()) << "Unhandled device Ohmpilot" << ohmpilotId;
        }

        QVariantMap sensorCardMap = bodyMap.value("Data").toMap().value("SensorCard").toMap();
        foreach (QString sensorCardId, sensorCardMap.keys()) {
            qCDebug(dcFronius()) << "Unhandled device SensorCard" << sensorCardId;
        }

        QVariantMap stringControlMap = bodyMap.value("Data").toMap().value("StringControl").toMap();
        foreach (QString stringControlId, stringControlMap.keys()) {
            qCDebug(dcFronius()) << "Unhandled device StringControl" << stringControlId;
        }

        if (!thingDescriptors.empty()) {
            emit autoThingsAppeared(thingDescriptors);
            thingDescriptors.clear();
        }

        // All devices
        updatePowerFlow(connection);
        updateInverters(connection);
        updateMeters(connection);
        updateStorages(connection);
    });
}

void IntegrationPluginFronius::updatePowerFlow(FroniusSolarConnection *connection)
{
    Thing *parentThing = m_froniusConnections.value(connection);

    // Get power flow realtime data and update storage and pv power values according to the total values
    // The inverter details inform about the PV production after feeding the storage, but we should use the total
    // to make sure the sum is correct. Battery seems to be feeded DC to DC before the AC power convertion
    FroniusNetworkReply *powerFlowReply = connection->getPowerFlowRealtimeData();
    connect(powerFlowReply, &FroniusNetworkReply::finished, this, [=]() {
        if (powerFlowReply->networkReply()->error() != QNetworkReply::NoError)
            return;

        QByteArray data = powerFlowReply->networkReply()->readAll();

        QJsonParseError error;
        QJsonDocument jsonDoc = QJsonDocument::fromJson(data, &error);
        if (error.error != QJsonParseError::NoError) {
            qCWarning(dcFronius()) << "PowerFlow: Failed to parse JSON data" << data << ":" << error.errorString();
            return;
        }

        // Parse the data and update the states of our device
        QVariantMap dataMap = jsonDoc.toVariant().toMap().value("Body").toMap().value("Data").toMap();
        qCDebug(dcFronius()) << "Power flow data" << qUtf8Printable(QJsonDocument::fromVariant(dataMap).toJson(QJsonDocument::Indented));
        Things availableInverters = myThings().filterByParentId(parentThing->id()).filterByThingClassId(inverterThingClassId);
        if (availableInverters.count() > 0) {
            if (availableInverters.count() == 1) {
                // Note: this is the actual power if there is a storage (the inverter object returns the energy before DC convertion fpor the storage
                Thing *inverterThing = availableInverters.first();
                double pvPower = dataMap.value("Site").toMap().value("P_PV").toDouble();
                inverterThing->setStateValue(inverterCurrentPowerStateTypeId, - pvPower);
            } else {
                // Let's set the individual PV values
                foreach (Thing *inverterThing, availableInverters) {
                    QVariantMap inverterMap = dataMap.value("Inverters").toMap().value(QString::number(inverterThing->paramValue(inverterThingIdParamTypeId).toInt())).toMap();
                    if (!inverterMap.isEmpty()) {
                        double inverterPower = inverterMap.value("P").toDouble();
                        inverterThing->setStateValue(inverterCurrentPowerStateTypeId, -inverterPower);
                    }
                }
            }
        }

        // Find the storage for this connection and update the current power
        Things availableStorages = myThings().filterByParentId(parentThing->id()).filterByThingClassId(storageThingClassId);
        // TODO: check what to set if there are more than one battery connected
        if (availableStorages.count() == 1) {
            Thing *storageThing = availableStorages.first();
            // Note: negative (charge), positiv (discharge)
            double akkuPower = - dataMap.value("Site").toMap().value("P_Akku").toDouble();
            storageThing->setStateValue(storageCurrentPowerStateTypeId, akkuPower);
            if (akkuPower < 0) {
                storageThing->setStateValue(storageChargingStateStateTypeId, "discharging");
            } else if (akkuPower > 0) {
                storageThing->setStateValue(storageChargingStateStateTypeId, "charging");
            } else {
                storageThing->setStateValue(storageChargingStateStateTypeId, "idle");
            }
        }

        Things availableMeters = myThings().filterByParentId(parentThing->id()).filterByThingClassId(meterThingClassId);
        if (availableMeters.count() == 1 && m_weakMeterConnections.value(connection)) {
            Thing *meterThing = availableMeters.first();
            double gridPower = dataMap.value("Site").toMap().value("P_Grid").toDouble();
            qCDebug(dcFronius()) << "Using power flow grid power for the weak S0 meter" << gridPower << "House consumption" << dataMap.value("Site").toMap().value("P_Load").toDouble();
            meterThing->setStateValue(meterCurrentPowerStateTypeId, gridPower);
        }
    });
}


void IntegrationPluginFronius::updateInverters(FroniusSolarConnection *connection)
{
    Thing *parentThing = m_froniusConnections.value(connection);
    foreach (Thing *inverterThing, myThings().filterByParentId(parentThing->id()).filterByThingClassId(inverterThingClassId)) {
        int inverterId = inverterThing->paramValue(inverterThingIdParamTypeId).toInt();

        // Get the inverter realtime data
        FroniusNetworkReply *realtimeDataReply = connection->getInverterRealtimeData(inverterId);
        connect(realtimeDataReply, &FroniusNetworkReply::finished, this, [=]() {
            if (realtimeDataReply->networkReply()->error() != QNetworkReply::NoError) {
                m_thingRequestErrorCounter[inverterThing] = m_thingRequestErrorCounter.value(inverterThing, 0) + 1;
                if (m_thingRequestErrorCounter.value(inverterThing) >= m_thingRequestErrorCountLimit) {
                    if (inverterThing->stateValue("connected").toBool()) {
                        qCWarning(dcFronius()) << "The inverter" << inverterThing << "received" << m_thingRequestErrorCountLimit << "errors. Mark thing as offline";
                    }
                    // Thing does not seem to be reachable
                    markInverterAsDisconnected(inverterThing);
                }
                return;
            }

            // Reset the error counter on a successfull refresh
            m_thingRequestErrorCounter[inverterThing] = 0;

            QByteArray data = realtimeDataReply->networkReply()->readAll();

            QJsonParseError error;
            QJsonDocument jsonDoc = QJsonDocument::fromJson(data, &error);
            if (error.error != QJsonParseError::NoError) {
                qCWarning(dcFronius()) << "Inverter: Failed to parse JSON data" << data << ":" << error.errorString();
                markInverterAsDisconnected(inverterThing);
                return;
            }

            // Parse the data and update the states of our device
            QVariantMap dataMap = jsonDoc.toVariant().toMap().value("Body").toMap().value("Data").toMap();
            //qCDebug(dcFronius()) << "Inverter data" << qUtf8Printable(QJsonDocument::fromVariant(dataMap).toJson(QJsonDocument::Indented));

            // Note: this is the PV power after feeding the battery, we have to use the total PV production from the power flow
            //if (dataMap.contains("PAC")) {
            //    QVariantMap map = dataMap.value("PAC").toMap();
            //    if (map.value("Unit") == "W") {
            //        inverterThing->setStateValue(inverterCurrentPowerStateTypeId, - map.value("Value").toDouble());
            //    }
            //}

            // Set the inverter device state
            if (dataMap.contains("DAY_ENERGY")) {
                QVariantMap map = dataMap.value("DAY_ENERGY").toMap();
                if (map.value("Unit") == "Wh") {
                    inverterThing->setStateValue(inverterEnergyDayStateTypeId, map.value("Value").toDouble() / 1000);
                }
            }

            if (dataMap.contains("YEAR_ENERGY")) {
                QVariantMap map = dataMap.value("YEAR_ENERGY").toMap();
                if (map.value("Unit") == "Wh") {
                    inverterThing->setStateValue(inverterEnergyYearStateTypeId, map.value("Value").toDouble() / 1000);
                }
            }

            if (dataMap.contains("TOTAL_ENERGY")) {
                QVariantMap map = dataMap.value("TOTAL_ENERGY").toMap();
                if (map.value("Unit") == "Wh") {
                    inverterThing->setStateValue(inverterTotalEnergyProducedStateTypeId, map.value("Value").toDouble() / 1000);
                }
            }

            inverterThing->setStateValue("connected", true);
        });
    }
}

void IntegrationPluginFronius::updateMeters(FroniusSolarConnection *connection)
{
    Thing *parentThing = m_froniusConnections.value(connection);
    foreach (Thing *meterThing, myThings().filterByParentId(parentThing->id()).filterByThingClassId(meterThingClassId)) {
        int meterId = meterThing->paramValue(meterThingIdParamTypeId).toInt();

        // Get the inverter realtime data
        FroniusNetworkReply *realtimeDataReply = connection->getMeterRealtimeData(meterId);
        connect(realtimeDataReply, &FroniusNetworkReply::finished, this, [=]() {
            if (realtimeDataReply->networkReply()->error() != QNetworkReply::NoError) {
                m_thingRequestErrorCounter[meterThing] = m_thingRequestErrorCounter.value(meterThing, 0) + 1;
                if (m_thingRequestErrorCounter.value(meterThing) >= m_thingRequestErrorCountLimit) {
                    if (meterThing->stateValue("connected").toBool()) {
                        qCWarning(dcFronius()) << "The meter" << meterThing << "received" << m_thingRequestErrorCountLimit << "errors. Mark thing as offline";
                    }
                    // Thing does not seem to be reachable
                    markMeterAsDisconnected(meterThing);
                }
                return;
            }

            // Reset the error counter on a successfull refresh
            m_thingRequestErrorCounter[meterThing] = 0;

            QByteArray data = realtimeDataReply->networkReply()->readAll();

            QJsonParseError error;
            QJsonDocument jsonDoc = QJsonDocument::fromJson(data, &error);
            if (error.error != QJsonParseError::NoError) {
                qCWarning(dcFronius()) << "Meter: Failed to parse JSON data" << data << ":" << error.errorString();
                markMeterAsDisconnected(meterThing);
                return;
            }


            // Parse the data and update the states of our device
            QVariantMap dataMap = jsonDoc.toVariant().toMap().value("Body").toMap().value("Data").toMap();
            qCDebug(dcFronius()) << "Meter data" << qUtf8Printable(QJsonDocument::fromVariant(dataMap).toJson(QJsonDocument::Indented));

            meterThing->setStateValue("connected", true);

            // Note: some inverters have a S0 meter connected, which measures the load, not the grid power and also provides only the current power and no additionl information
            // Since we assume a meter on the grid root of the household, we don't update the meter here, but in the updatePowerFlow method.
            QString model;
            if (dataMap.contains("Details")) {
                model = dataMap.value("Details").toMap().value("Model", "Smart Meter").toString();
            }

            // Note: maybe we find a better way to detect a weak meter, for now, we only knwo it does not provide any additional informaiton and has a S0 in the name
            if (model.toLower().contains("s0")) {
                qCDebug(dcFronius()) << "Ignoring this meter since there are not enought information available (S0). Using the plant overview grid power as meter information.";
                m_weakMeterConnections[connection] = true;
                return;
            } else {
                m_weakMeterConnections[connection] = false;
            }

            // Power
            if (dataMap.contains("PowerReal_P_Sum")) {
                meterThing->setStateValue(meterCurrentPowerStateTypeId, dataMap.value("PowerReal_P_Sum").toDouble());
            }

            if (dataMap.contains("PowerReal_P_Phase_1")) {
                meterThing->setStateValue(meterCurrentPowerPhaseAStateTypeId, dataMap.value("PowerReal_P_Phase_1").toDouble());
            }

            if (dataMap.contains("PowerReal_P_Phase_2")) {
                meterThing->setStateValue(meterCurrentPowerPhaseBStateTypeId, dataMap.value("PowerReal_P_Phase_2").toDouble());
            }

            if (dataMap.contains("PowerReal_P_Phase_3")) {
                meterThing->setStateValue(meterCurrentPowerPhaseCStateTypeId, dataMap.value("PowerReal_P_Phase_3").toDouble());
            }

            // Current
            if (dataMap.contains("Current_AC_Phase_1")) {
                meterThing->setStateValue(meterCurrentPhaseAStateTypeId, dataMap.value("Current_AC_Phase_1").toDouble());
            }

            if (dataMap.contains("Current_AC_Phase_2")) {
                meterThing->setStateValue(meterCurrentPhaseBStateTypeId, dataMap.value("Current_AC_Phase_2").toDouble());
            }

            if (dataMap.contains("Current_AC_Phase_3")) {
                meterThing->setStateValue(meterCurrentPhaseCStateTypeId, dataMap.value("Current_AC_Phase_3").toDouble());
            }

            // Voltage
            if (dataMap.contains("Voltage_AC_Phase_1")) {
                meterThing->setStateValue(meterVoltagePhaseAStateTypeId, dataMap.value("Voltage_AC_Phase_1").toDouble());
            }

            if (dataMap.contains("Voltage_AC_Phase_2")) {
                meterThing->setStateValue(meterVoltagePhaseBStateTypeId, dataMap.value("Voltage_AC_Phase_2").toDouble());
            }

            if (dataMap.contains("Voltage_AC_Phase_3")) {
                meterThing->setStateValue(meterVoltagePhaseCStateTypeId, dataMap.value("Voltage_AC_Phase_3").toDouble());
            }

            // Total energy
            if (dataMap.contains("EnergyReal_WAC_Sum_Produced")) {
                meterThing->setStateValue(meterTotalEnergyProducedStateTypeId, dataMap.value("EnergyReal_WAC_Sum_Produced").toInt()/1000.00);
            }

            if (dataMap.contains("EnergyReal_WAC_Sum_Consumed")) {
                meterThing->setStateValue(meterTotalEnergyConsumedStateTypeId, dataMap.value("EnergyReal_WAC_Sum_Consumed").toInt()/1000.00);
            }

            // Frequency
            if (dataMap.contains("Frequency_Phase_Average")) {
                meterThing->setStateValue(meterFrequencyStateTypeId, dataMap.value("Frequency_Phase_Average").toDouble());
            }
        });
    }
}

void IntegrationPluginFronius::updateStorages(FroniusSolarConnection *connection)
{
    Thing *parentThing = m_froniusConnections.value(connection);
    foreach (Thing *storageThing, myThings().filterByParentId(parentThing->id()).filterByThingClassId(storageThingClassId)) {
        int storageId = storageThing->paramValue(storageThingIdParamTypeId).toInt();

        // Get the storage realtime data
        FroniusNetworkReply *realtimeDataReply = connection->getStorageRealtimeData(storageId);
        connect(realtimeDataReply, &FroniusNetworkReply::finished, this, [=]() {
            if (realtimeDataReply->networkReply()->error() != QNetworkReply::NoError) {
                m_thingRequestErrorCounter[storageThing] = m_thingRequestErrorCounter.value(storageThing, 0) + 1;
                if (m_thingRequestErrorCounter.value(storageThing) >= m_thingRequestErrorCountLimit) {
                    if (storageThing->stateValue("connected").toBool()) {
                        qCWarning(dcFronius()) << "The storage" << storageThing << "received" << m_thingRequestErrorCountLimit << "errors. Mark thing as offline";
                    }
                    // Thing does not seem to be reachable
                    markStorageAsDisconnected(storageThing);
                }
                return;
            }

            // Reset the error counter on a successfull refresh
            m_thingRequestErrorCounter[storageThing] = 0;

            if (realtimeDataReply->networkReply()->error() != QNetworkReply::NoError) {
                // Thing does not seem to be reachable
                markStorageAsDisconnected(storageThing);
                return;
            }

            QByteArray data = realtimeDataReply->networkReply()->readAll();

            QJsonParseError error;
            QJsonDocument jsonDoc = QJsonDocument::fromJson(data, &error);
            if (error.error != QJsonParseError::NoError) {
                qCWarning(dcFronius()) << "Storage: Failed to parse JSON data" << data << ":" << error.errorString();
                markStorageAsDisconnected(storageThing);
                return;
            }

            // Parse the data and update the states of our device
            QVariantMap dataMap = jsonDoc.toVariant().toMap().value("Body").toMap().value("Data").toMap();
            //qCDebug(dcFronius()) << "Storage data" << qUtf8Printable(QJsonDocument::fromVariant(dataMap).toJson(QJsonDocument::Indented));

            QVariantMap storageInfoMap = dataMap.value("Controller").toMap();

            // copy retrieved information to thing states
            if (storageInfoMap.contains("StateOfCharge_Relative")) {
                storageThing->setStateValue(storageBatteryLevelStateTypeId, storageInfoMap.value("StateOfCharge_Relative").toInt());
                if (storageThing->stateValue(storageChargingStateStateTypeId).toString() == "charging" && (storageInfoMap.value("StateOfCharge_Relative").toInt() < 5)) {
                    storageThing->setStateValue(storageBatteryCriticalStateTypeId, true);
                } else {
                    storageThing->setStateValue(storageBatteryCriticalStateTypeId, false);
                }
            }

            if (storageInfoMap.contains("Temperature_Cell"))
                storageThing->setStateValue(storageCellTemperatureStateTypeId, storageInfoMap.value("Temperature_Cell").toDouble());

            if (storageInfoMap.contains("Capacity_Maximum"))
                storageThing->setStateValue(storageCapacityStateTypeId, storageInfoMap.value("Capacity_Maximum").toDouble());


            storageThing->setStateValue("connected", true);
        });
    }
}

void IntegrationPluginFronius::markInverterAsDisconnected(Thing *thing)
{
    thing->setStateValue("connected", false);
    thing->setStateValue("currentPower", 0);
    // Note: do not reset the energy counters since they are always counting up until reset on the device
}

void IntegrationPluginFronius::markMeterAsDisconnected(Thing *thing)
{
    thing->setStateValue("connected", false);
    thing->setStateValue("currentPower", 0);
    thing->setStateValue("voltagePhaseA", 0);
    thing->setStateValue("voltagePhaseB", 0);
    thing->setStateValue("voltagePhaseC", 0);
    thing->setStateValue("currentPhaseA", 0);
    thing->setStateValue("currentPhaseB", 0);
    thing->setStateValue("currentPhaseC", 0);
    thing->setStateValue("currentPowerPhaseA", 0);
    thing->setStateValue("currentPowerPhaseB", 0);
    thing->setStateValue("currentPowerPhaseC", 0);
    thing->setStateValue("frequency", 0);
    // Note: do not reset the energy counters since they are always counting up until reset on the device
}

void IntegrationPluginFronius::markStorageAsDisconnected(Thing *thing)
{
    thing->setStateValue("connected", false);
    thing->setStateValue("currentPower", 0);
    thing->setStateValue("chargingState", "idle");
    // Note: do not reset the energy counters since they are always counting up until reset on the device
}

