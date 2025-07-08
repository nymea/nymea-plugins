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

            Thing *meter = meterThing(thing);
            if (meter) {
                meter->setStateValue(senecMeterCurrentPowerStateTypeId, storage->gridPower());
            }
        });

        connect(thing, &Thing::settingChanged, this, [this, thing](const ParamTypeId &paramTypeId, const QVariant &value){
            if (paramTypeId == senecStorageLanSettingsAddMeterParamTypeId) {

                if (value.toBool()) {
                    // Check if we have to add the meter
                    if (!meterThing(thing)) {
                        qCDebug(dcSenec()) << "Add meter for" << thing->name();
                        emit autoThingsAppeared(ThingDescriptors() << ThingDescriptor(senecMeterThingClassId, "SENEC Meter", QString(), thing->id()));
                    }
                } else {
                    // Check if we have to remove the meter
                    Thing *meter = meterThing(thing);
                    if (meter) {
                        qCDebug(dcSenec()) << "Remove meter thing for" << thing->name();
                        emit autoThingDisappeared(meter->id());
                    }
                }
            }
        });

        info->finish(Thing::ThingErrorNoError);

    } else if (thing->thingClassId() == senecMeterThingClassId) {
        Thing *parentThing = myThings().findById(thing->parentId());
        if (parentThing) {
            thing->setStateValue(senecMeterConnectedStateTypeId, parentThing->stateValue(senecStorageLanConnectedStateTypeId).toBool());
        }

        info->finish(Thing::ThingErrorNoError);
    }
}

void IntegrationPluginSenec::postSetupThing(Thing *thing)
{
    if (thing->thingClassId() == senecStorageLanThingClassId) {
        m_storages.value(thing)->initialize();
    }

    // Create the refresh timer if not already set up
    if (!m_refreshTimer) {
        qCDebug(dcSenec()) << "Starting refresh timer ...";
        m_refreshTimer = hardwareManager()->pluginTimerManager()->registerTimer(5);
        connect(m_refreshTimer, &PluginTimer::timeout, this, &IntegrationPluginSenec::refresh);
        m_refreshTimer->start();
    }
}

void IntegrationPluginSenec::thingRemoved(Thing *thing)
{
    if (thing->thingClassId() == senecStorageLanThingClassId) {
        hardwareManager()->networkDeviceDiscovery()->unregisterMonitor(m_monitors.take(thing));
        m_storages.take(thing)->deleteLater();
    }

    if (myThings().isEmpty()) {
        qCDebug(dcSenec()) << "Stopping refresh timer";
        hardwareManager()->pluginTimerManager()->unregisterTimer(m_refreshTimer);
        m_refreshTimer = nullptr;
    }
}

Thing *IntegrationPluginSenec::meterThing(Thing *storageThing) const
{
    Things meterThings = myThings().filterByThingClassId(senecMeterThingClassId).filterByParentId(storageThing->id());
    if (meterThings.isEmpty())
        return nullptr;

    return meterThings.first();
}

void IntegrationPluginSenec::setConnected(Thing *storageThing, bool connected)
{
    storageThing->setStateValue(senecStorageLanConnectedStateTypeId, connected);

    Thing *meter = meterThing(storageThing);
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
