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
#include "senecdiscovery.h"
#include "plugininfo.h"

#include <network/networkdevicediscovery.h>

IntegrationPluginSenec::IntegrationPluginSenec()
{
    // Testing the convert methods

    // QString rawValue = "fl_42A2E5E4"; // 81.449
    // float value = SenecStorageLan::parseFloat(rawValue);
    // qCWarning(dcSenec()) << rawValue << value;

    // QString rawValue = "st_foobar"; // foobar
    // QString value = SenecStorageLan::parseString(rawValue);
    // qCWarning(dcSenec()) << rawValue << value;

    // QString rawValue = "u8_64"; // 100
    // quint8 value = SenecStorageLan::parseUInt8(rawValue);
    // qCWarning(dcSenec()) << rawValue << value;
}

IntegrationPluginSenec::~IntegrationPluginSenec()
{

}

void IntegrationPluginSenec::discoverThings(ThingDiscoveryInfo *info)
{
    if (info->thingClassId() == senecStorageLanThingClassId) {

        if (!hardwareManager()->networkDeviceDiscovery()->available()) {
            qCWarning(dcSenec()) << "Failed to discover network devices. The network device discovery is not available.";
            info->finish(Thing::ThingErrorHardwareNotAvailable, QT_TR_NOOP("Unable to discover devices in your network."));
            return;
        }

        qCInfo(dcSenec()) << "Starting network discovery...";
        SenecDiscovery *discovery = new SenecDiscovery(hardwareManager()->networkManager(), hardwareManager()->networkDeviceDiscovery(), info);
        connect(discovery, &SenecDiscovery::discoveryFinished, info, [=](){
            ThingDescriptors descriptors;
            qCInfo(dcSenec()) << "Discovery finished. Found" << discovery->results().count() << "devices";
            foreach (const SenecDiscovery::Result &result, discovery->results()) {
                qCInfo(dcSenec()) << "Discovered device on" << result.networkDeviceInfo;

                QString title = "SENEC connection (" + result.deviceId + ")";
                QString description = result.networkDeviceInfo.address().toString();

                ThingDescriptor descriptor(senecStorageLanThingClassId, title, description);

                ParamList params;
                params << Param(senecStorageLanThingMacAddressParamTypeId, result.networkDeviceInfo.thingParamValueMacAddress());
                params << Param(senecStorageLanThingHostNameParamTypeId, result.networkDeviceInfo.thingParamValueHostName());
                params << Param(senecStorageLanThingAddressParamTypeId, result.networkDeviceInfo.thingParamValueAddress());
                descriptor.setParams(params);

                // Check if we already have set up this device
                Thing *existingThing = myThings().findByParams(params);
                if (existingThing) {
                    qCDebug(dcSenec()) << "This thing already exists in the system:" << result.networkDeviceInfo;
                    descriptor.setThingId(existingThing->id());
                }

                info->addThingDescriptor(descriptor);
            }

            info->finish(Thing::ThingErrorNoError);
        });

        discovery->startDiscovery();
    }
}

void IntegrationPluginSenec::startPairing(ThingPairingInfo *info)
{
    info->finish(Thing::ThingErrorNoError, QT_TR_NOOP("Please sign in on developer.senec.com with your SENEC account, subscribe to the SENEC.Connect product and enter the subscription key from your profile."));
}

void IntegrationPluginSenec::confirmPairing(ThingPairingInfo *info, const QString &username, const QString &secret)
{
    Q_UNUSED(username)

    QString subscriptionKey = secret.trimmed();
    if (subscriptionKey.isEmpty()) {
        info->finish(Thing::ThingErrorAuthenticationFailure, QT_TR_NOOP("Please enter a subscription key."));
        return;
    }

    qCDebug(dcSenec()) << "Verifying SENEC.Connect subscription key" << subscriptionKey.left(4) + QString(subscriptionKey.length() - 4, '*');

    SenecConnect *senecConnect = new SenecConnect(hardwareManager()->networkManager(), subscriptionKey, info);
    QNetworkReply *reply = senecConnect->getDeviceData();
    connect(reply, &QNetworkReply::finished, reply, &QNetworkReply::deleteLater);
    connect(reply, &QNetworkReply::finished, info, [this, reply, info, subscriptionKey] {

        int status = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
        if (status == 401 || status == 403) {
            qCWarning(dcSenec()) << "SENEC.Connect rejected the subscription key. Status:" << status;
            info->finish(Thing::ThingErrorAuthenticationFailure, QT_TR_NOOP("The subscription key is not valid."));
            return;
        }

        if (status != 200 || reply->error() != QNetworkReply::NoError) {
            qCWarning(dcSenec()) << "SENEC.Connect request finished with error. Status:" << status << "Error:" << reply->errorString();
            info->finish(Thing::ThingErrorHardwareNotAvailable, QT_TR_NOOP("Unable to connect to SENEC.Connect. Please try again later."));
            return;
        }

        pluginStorage()->beginGroup(info->thingId().toString());
        pluginStorage()->setValue("subscriptionKey", subscriptionKey);
        pluginStorage()->endGroup();

        info->finish(Thing::ThingErrorNoError);
    });
}

void IntegrationPluginSenec::setupThing(ThingSetupInfo *info)
{
    Thing *thing = info->thing();
    qCDebug(dcSenec()) << "Setting up" << thing->name() << thing->params();

    if (thing->thingClassId() == senecStorageLanThingClassId) {

        // Handle reconfigure
        if (m_monitors.contains(thing)) {
            qCDebug(dcSenec()) << "Unregister existing monitor and recreate a new one...";
            hardwareManager()->networkDeviceDiscovery()->unregisterMonitor(m_monitors.take(thing));
        }

        if (m_storages.contains(thing))
            m_storages.take(thing)->deleteLater();

        NetworkDeviceMonitor *monitor = hardwareManager()->networkDeviceDiscovery()->registerMonitor(thing);
        m_monitors.insert(thing, monitor);

        SenecStorageLan *storage = new SenecStorageLan(hardwareManager()->networkManager(), this);
        m_storages.insert(thing, storage);

        storage->setAddress(monitor->networkDeviceInfo().address());
        connect(monitor, &NetworkDeviceMonitor::reachableChanged, storage, [monitor, storage](bool reachable){
            if (reachable) {
                storage->setAddress(monitor->networkDeviceInfo().address());
            }
        });

        connect(storage, &SenecStorageLan::availableChanged, thing, [this, thing](bool available){
            setConnected(thing, available);
        });

        connect(storage, &SenecStorageLan::initializeFinished, thing, [storage, thing](bool success){
            if (success) {
                thing->setStateValue(senecStorageLanCapacityStateTypeId, storage->capacity());
            }
        });

        connect(storage, &SenecStorageLan::updatedFinished, thing, [storage, thing, this](bool success){
            setConnected(thing, storage->available());

            if (!success)
                return;

            int batteryLevel = qRound(storage->batteryLevel());
            thing->setStateValue(senecStorageLanBatteryLevelStateTypeId, batteryLevel);
            thing->setStateValue(senecStorageLanBatteryCriticalStateTypeId, batteryLevel < 10);
            thing->setStateValue(senecStorageLanCurrentPowerStateTypeId, storage->batteryPower());
            if (storage->batteryPower() > 0) {
                thing->setStateValue(senecStorageLanChargingStateStateTypeId, "charging");
            } else if (storage->batteryPower() < 0) {
                thing->setStateValue(senecStorageLanChargingStateStateTypeId, "discharging");
            } else {
                thing->setStateValue(senecStorageLanChargingStateStateTypeId, "idle");
            }

            Thing *meter = childThing(thing, senecMeterThingClassId);
            if (meter) {
                meter->setStateValue(senecMeterCurrentPowerStateTypeId, storage->gridPower());
            }
        });

        connect(thing, &Thing::settingChanged, this, [this, thing](const ParamTypeId &paramTypeId, const QVariant &value){
            if (paramTypeId == senecStorageLanSettingsAddMeterParamTypeId) {

                if (value.toBool()) {
                    // Check if we have to add the meter
                    if (!childThing(thing, senecMeterThingClassId)) {
                        qCDebug(dcSenec()) << "Add meter for" << thing->name();
                        emit autoThingsAppeared(ThingDescriptors() << ThingDescriptor(senecMeterThingClassId, "SENEC Meter", QString(), thing->id()));
                    }
                } else {
                    // Check if we have to remove the meter
                    Thing *meter = childThing(thing, senecMeterThingClassId);
                    if (meter) {
                        qCDebug(dcSenec()) << "Remove meter thing for" << thing->name();
                        emit autoThingDisappeared(meter->id());
                    }
                }
            }
        });

        info->finish(Thing::ThingErrorNoError);

    } else if (thing->thingClassId() == senecConnectThingClassId) {

        pluginStorage()->beginGroup(thing->id().toString());
        QString subscriptionKey = pluginStorage()->value("subscriptionKey").toString();
        pluginStorage()->endGroup();

        if (subscriptionKey.isEmpty()) {
            info->finish(Thing::ThingErrorAuthenticationFailure, QT_TR_NOOP("The SENEC.Connect subscription key is missing. Please reconfigure the connection."));
            return;
        }

        // Handle reconfigure
        if (m_connects.contains(thing))
            m_connects.take(thing)->deleteLater();

        m_connects.insert(thing, new SenecConnect(hardwareManager()->networkManager(), subscriptionKey, this));
        info->finish(Thing::ThingErrorNoError);

    } else if (thing->thingClassId() == senecStorageCloudThingClassId) {
        info->finish(Thing::ThingErrorNoError);

    } else if (thing->thingClassId() == senecMeterThingClassId) {
        Thing *parentThing = myThings().findById(thing->parentId());
        if (parentThing && parentThing->thingClassId() == senecStorageLanThingClassId) {
            thing->setStateValue(senecMeterConnectedStateTypeId, parentThing->stateValue(senecStorageLanConnectedStateTypeId).toBool());
        } else if (parentThing && parentThing->thingClassId() == senecStorageCloudThingClassId) {
            thing->setStateValue(senecMeterConnectedStateTypeId, parentThing->stateValue(senecStorageCloudConnectedStateTypeId).toBool());
        }

        info->finish(Thing::ThingErrorNoError);

    } else if (thing->thingClassId() == senecInverterThingClassId) {
        Thing *parentThing = myThings().findById(thing->parentId());
        if (parentThing) {
            thing->setStateValue(senecInverterConnectedStateTypeId, parentThing->stateValue(senecStorageCloudConnectedStateTypeId).toBool());
        }

        info->finish(Thing::ThingErrorNoError);
    }
}

void IntegrationPluginSenec::postSetupThing(Thing *thing)
{
    if (thing->thingClassId() == senecStorageLanThingClassId) {
        m_storages.value(thing)->initialize();

        if (!m_refreshTimer) {
            qCDebug(dcSenec()) << "Starting LAN refresh timer ...";
            m_refreshTimer = hardwareManager()->pluginTimerManager()->registerTimer(5);
            connect(m_refreshTimer, &PluginTimer::timeout, this, &IntegrationPluginSenec::refresh);
            m_refreshTimer->start();
        }

    } else if (thing->thingClassId() == senecConnectThingClassId) {
        refreshCloud(thing);

        // SENEC.Connect allows 45000 requests per month, which is roughly one request per minute
        if (!m_cloudRefreshTimer) {
            qCDebug(dcSenec()) << "Starting SENEC.Connect refresh timer ...";
            m_cloudRefreshTimer = hardwareManager()->pluginTimerManager()->registerTimer(60);
            connect(m_cloudRefreshTimer, &PluginTimer::timeout, this, &IntegrationPluginSenec::refreshClouds);
            m_cloudRefreshTimer->start();
        }

    } else if (thing->thingClassId() == senecStorageCloudThingClassId) {
        ThingDescriptors descriptors;
        if (!childThing(thing, senecMeterThingClassId))
            descriptors.append(ThingDescriptor(senecMeterThingClassId, "SENEC Meter", thing->name(), thing->id()));

        if (!childThing(thing, senecInverterThingClassId))
            descriptors.append(ThingDescriptor(senecInverterThingClassId, "SENEC PV inverter", thing->name(), thing->id()));

        if (!descriptors.isEmpty()) {
            qCDebug(dcSenec()) << "Adding" << descriptors.count() << "child things for" << thing->name();
            emit autoThingsAppeared(descriptors);
        }
    }
}

void IntegrationPluginSenec::thingRemoved(Thing *thing)
{
    if (thing->thingClassId() == senecStorageLanThingClassId) {
        hardwareManager()->networkDeviceDiscovery()->unregisterMonitor(m_monitors.take(thing));
        m_storages.take(thing)->deleteLater();
    }

    if (thing->thingClassId() == senecConnectThingClassId) {
        if (m_connects.contains(thing))
            m_connects.take(thing)->deleteLater();

        pluginStorage()->beginGroup(thing->id().toString());
        pluginStorage()->remove("");
        pluginStorage()->endGroup();
    }

    m_lastEnergyUpdates.remove(thing);

    if (m_refreshTimer && m_storages.isEmpty()) {
        qCDebug(dcSenec()) << "Stopping LAN refresh timer";
        hardwareManager()->pluginTimerManager()->unregisterTimer(m_refreshTimer);
        m_refreshTimer = nullptr;
    }

    if (m_cloudRefreshTimer && m_connects.isEmpty()) {
        qCDebug(dcSenec()) << "Stopping SENEC.Connect refresh timer";
        hardwareManager()->pluginTimerManager()->unregisterTimer(m_cloudRefreshTimer);
        m_cloudRefreshTimer = nullptr;
    }
}

Thing *IntegrationPluginSenec::childThing(Thing *parentThing, const ThingClassId &thingClassId) const
{
    Things childThings = myThings().filterByThingClassId(thingClassId).filterByParentId(parentThing->id());
    if (childThings.isEmpty())
        return nullptr;

    return childThings.first();
}

void IntegrationPluginSenec::setConnected(Thing *storageThing, bool connected)
{
    storageThing->setStateValue(senecStorageLanConnectedStateTypeId, connected);

    Thing *meter = childThing(storageThing, senecMeterThingClassId);
    if (meter) {
        meter->setStateValue(senecMeterConnectedStateTypeId, connected);
    }
}

void IntegrationPluginSenec::refresh()
{
    foreach (Thing *storageThing, myThings().filterByThingClassId(senecStorageLanThingClassId)) {
        SenecStorageLan *storage = m_storages.value(storageThing);
        // Retry fetching the static information until the storage could be initialized once
        if (storage->initialized()) {
            storage->update();
        } else {
            storage->initialize();
        }
    }
}

void IntegrationPluginSenec::refreshClouds()
{
    foreach (Thing *connectThing, myThings().filterByThingClassId(senecConnectThingClassId)) {
        refreshCloud(connectThing);
    }
}

void IntegrationPluginSenec::refreshCloud(Thing *connectThing)
{
    SenecConnect *senecConnect = m_connects.value(connectThing);
    if (!senecConnect)
        return;

    QNetworkReply *reply = senecConnect->getDeviceData();
    connect(reply, &QNetworkReply::finished, reply, &QNetworkReply::deleteLater);
    connect(reply, &QNetworkReply::finished, connectThing, [this, reply, connectThing] {

        int status = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
        if (status == 401 || status == 403) {
            qCWarning(dcSenec()) << "SENEC.Connect request has been rejected. The subscription key seems to be invalid. Status:" << status;
            connectThing->setStateValue(senecConnectLoggedInStateTypeId, false);
            setCloudConnected(connectThing, false);
            return;
        }

        if (status != 200 || reply->error() != QNetworkReply::NoError) {
            qCWarning(dcSenec()) << "SENEC.Connect request finished with error. Status:" << status << "Error:" << reply->errorString();
            setCloudConnected(connectThing, false);
            return;
        }

        QByteArray responseData = reply->readAll();
        qCDebug(dcSenec()) << "SENEC.Connect request finished successfully" << qUtf8Printable(responseData);

        bool ok = false;
        QList<SenecConnect::SystemData> systems = SenecConnect::parseDeviceData(responseData, &ok);
        if (!ok) {
            qCWarning(dcSenec()) << "SENEC.Connect request finished successfully, but the response contains invalid data:" << responseData;
            setCloudConnected(connectThing, false);
            return;
        }

        connectThing->setStateValue(senecConnectLoggedInStateTypeId, true);
        connectThing->setStateValue(senecConnectConnectedStateTypeId, true);
        processCloudData(connectThing, systems);
    });
}

void IntegrationPluginSenec::processCloudData(Thing *connectThing, const QList<SenecConnect::SystemData> &systems)
{
    ThingDescriptors descriptors;
    Things storageThings = myThings().filterByThingClassId(senecStorageCloudThingClassId).filterByParentId(connectThing->id());

    foreach (const SenecConnect::SystemData &system, systems) {
        Things existingThings = storageThings.filterByParam(senecStorageCloudThingSystemIdParamTypeId, system.systemId);
        if (existingThings.isEmpty()) {
            qCDebug(dcSenec()) << "Creating new storage for" << system.systemId << system.model;
            ThingDescriptor descriptor(senecStorageCloudThingClassId, system.model.isEmpty() ? "SENEC.Home storage" : system.model, system.systemId, connectThing->id());
            descriptor.setParams(ParamList() << Param(senecStorageCloudThingSystemIdParamTypeId, system.systemId)
                                             << Param(senecStorageCloudThingSerialNumberParamTypeId, system.serialNumber));
            descriptors.append(descriptor);
            continue;
        }

        Thing *storageThing = existingThings.first();
        Thing *meter = childThing(storageThing, senecMeterThingClassId);
        Thing *inverter = childThing(storageThing, senecInverterThingClassId);

        storageThing->setStateValue(senecStorageCloudConnectedStateTypeId, system.active);
        if (meter)
            meter->setStateValue(senecMeterConnectedStateTypeId, system.active);

        if (inverter)
            inverter->setStateValue(senecInverterConnectedStateTypeId, system.active);

        if (!system.active) {
            qCDebug(dcSenec()) << "System" << system.systemId << "does not provide live data. Marking as disconnected.";
            continue;
        }

        int batteryLevel = qRound(system.batteryLevel);
        storageThing->setStateValue(senecStorageCloudCapacityStateTypeId, system.capacity);
        storageThing->setStateValue(senecStorageCloudBatteryLevelStateTypeId, batteryLevel);
        storageThing->setStateValue(senecStorageCloudBatteryCriticalStateTypeId, batteryLevel < 10);
        storageThing->setStateValue(senecStorageCloudCurrentPowerStateTypeId, system.batteryPower);
        if (system.batteryPower > 0) {
            storageThing->setStateValue(senecStorageCloudChargingStateStateTypeId, "charging");
        } else if (system.batteryPower < 0) {
            storageThing->setStateValue(senecStorageCloudChargingStateStateTypeId, "discharging");
        } else {
            storageThing->setStateValue(senecStorageCloudChargingStateStateTypeId, "idle");
        }

        if (meter) {
            updateEnergy(meter, system.gridPower, senecMeterTotalEnergyConsumedStateTypeId, senecMeterTotalEnergyProducedStateTypeId);
            meter->setStateValue(senecMeterCurrentPowerStateTypeId, system.gridPower);
        }

        if (inverter) {
            updateEnergy(inverter, -system.production, StateTypeId(), senecInverterTotalEnergyProducedStateTypeId);
            inverter->setStateValue(senecInverterCurrentPowerStateTypeId, -system.production);
        }
    }

    if (!descriptors.isEmpty()) {
        qCDebug(dcSenec()) << "Adding" << descriptors.count() << "new SENEC.Home" << (descriptors.count() > 1 ? "storages" : "storage");
        emit autoThingsAppeared(descriptors);
    }
}

void IntegrationPluginSenec::setCloudConnected(Thing *connectThing, bool connected)
{
    connectThing->setStateValue(senecConnectConnectedStateTypeId, connected);

    foreach (Thing *storageThing, myThings().filterByThingClassId(senecStorageCloudThingClassId).filterByParentId(connectThing->id())) {
        storageThing->setStateValue(senecStorageCloudConnectedStateTypeId, connected);

        Thing *meter = childThing(storageThing, senecMeterThingClassId);
        if (meter)
            meter->setStateValue(senecMeterConnectedStateTypeId, connected);

        Thing *inverter = childThing(storageThing, senecInverterThingClassId);
        if (inverter)
            inverter->setStateValue(senecInverterConnectedStateTypeId, connected);
    }
}

void IntegrationPluginSenec::updateEnergy(Thing *thing, double currentPower, const StateTypeId &consumedStateTypeId, const StateTypeId &producedStateTypeId)
{
    // The cloud API only provides power values, so the energy totals are integrated between two samples
    QDateTime now = QDateTime::currentDateTimeUtc();
    QDateTime lastUpdate = m_lastEnergyUpdates.value(thing);
    m_lastEnergyUpdates.insert(thing, now);

    // Skip the first sample and gaps (restart, connection loss) instead of extrapolating
    if (!lastUpdate.isValid() || lastUpdate.secsTo(now) > 300)
        return;

    // Trapezoidal integration using the previous power value still stored in the state
    double hoursPassed = lastUpdate.msecsTo(now) / 1000.0 / 3600.0;
    double averagePower = (thing->stateValue("currentPower").toDouble() + currentPower) / 2.0;
    double energy = qAbs(averagePower) / 1000.0 * hoursPassed;

    if (averagePower > 0 && !consumedStateTypeId.isNull()) {
        thing->setStateValue(consumedStateTypeId, thing->stateValue(consumedStateTypeId).toDouble() + energy);
    } else if (averagePower < 0 && !producedStateTypeId.isNull()) {
        thing->setStateValue(producedStateTypeId, thing->stateValue(producedStateTypeId).toDouble() + energy);
    }
}
