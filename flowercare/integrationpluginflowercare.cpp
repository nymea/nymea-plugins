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

/*
 * ServieUUIDs:
 * {00001204-0000-1000-8000-00805f9b34fb}
 * {00001206-0000-1000-8000-00805f9b34fb}
 * {00001800-0000-1000-8000-00805f9b34fb}
 * {00001801-0000-1000-8000-00805f9b34fb}
 * {0000fe95-0000-1000-8000-00805f9b34fb}
 * {0000fef5-0000-1000-8000-00805f9b34fb}
 */

#include <hardware/bluetoothlowenergy/bluetoothlowenergymanager.h>

#include "plugininfo.h"
#include "integrationpluginflowercare.h"
#include "flowercare.h"

IntegrationPluginFlowercare::IntegrationPluginFlowercare()
{

}

IntegrationPluginFlowercare::~IntegrationPluginFlowercare()
{
    if (m_reconnectTimer) {
        hardwareManager()->pluginTimerManager()->unregisterTimer(m_reconnectTimer);
    }
}

void IntegrationPluginFlowercare::init()
{
}

void IntegrationPluginFlowercare::discoverThings(ThingDiscoveryInfo *info)
{
    if (!hardwareManager()->bluetoothLowEnergyManager()->available())
        return info->finish(Thing::ThingErrorHardwareNotAvailable, QT_TR_NOOP("Cannot discover Bluetooth devices. Bluetooth is not available on this system."));

    if (!hardwareManager()->bluetoothLowEnergyManager()->enabled())
        return info->finish(Thing::ThingErrorHardwareNotAvailable, QT_TR_NOOP("Cannot discover Bluetooth devices. Bluetooth is disabled."));

    BluetoothDiscoveryReply *reply = hardwareManager()->bluetoothLowEnergyManager()->discoverDevices();

    connect(reply, &BluetoothDiscoveryReply::finished, info, [this, info, reply](){
        reply->deleteLater();

        if (reply->error() != BluetoothDiscoveryReply::BluetoothDiscoveryReplyErrorNoError) {
            qCWarning(dcFlowerCare()) << "Bluetooth discovery error:" << reply->error();
            info->finish(Thing::ThingErrorHardwareFailure, QT_TR_NOOP("An error happened during Bluetooth discovery."));
            return;
        }

        qCDebug(dcFlowerCare()) << "Discovery finished";

        foreach (const auto &deviceInfo, reply->discoveredDevices()) {
            qCDebug(dcFlowerCare()) << "Discovered device" << deviceInfo.first.name();
            if (deviceInfo.first.name().contains("Flower care")) {
                ThingDescriptor descriptor(flowerCareThingClassId, deviceInfo.first.name(), deviceInfo.first.address().toString());
                ParamList params;
                params << Param(flowerCareThingMacParamTypeId, deviceInfo.first.address().toString());
                descriptor.setParams(params);
                foreach (Thing *existingThing, myThings()) {
                    if (existingThing->paramValue(flowerCareThingMacParamTypeId).toString() == deviceInfo.first.address().toString()) {
                        descriptor.setThingId(existingThing->id());
                        break;
                    }
                }
                info->addThingDescriptor(descriptor);
            }
        }

        info->finish(Thing::ThingErrorNoError);
    });

}

void IntegrationPluginFlowercare::setupThing(ThingSetupInfo *info)
{
    Thing *thing = info->thing();

    qCDebug(dcFlowerCare) << "Setting up Flower care" << thing->name() << thing->params();

    QBluetoothAddress address = QBluetoothAddress(thing->paramValue(flowerCareThingMacParamTypeId).toString());
    QBluetoothDeviceInfo deviceInfo = QBluetoothDeviceInfo(address, thing->name(), 0);

    BluetoothLowEnergyDevice *bluetoothDevice = hardwareManager()->bluetoothLowEnergyManager()->registerDevice(deviceInfo, QLowEnergyController::PublicAddress);
    FlowerCare *flowerCare = new FlowerCare(bluetoothDevice, this);
    connect(flowerCare, &FlowerCare::finished, this, &IntegrationPluginFlowercare::onSensorDataReceived);
    m_list.insert(thing, flowerCare);

    m_refreshMinutes[flowerCare] = 0;

    if (!m_reconnectTimer) {
        m_reconnectTimer = hardwareManager()->pluginTimerManager()->registerTimer();
        connect(m_reconnectTimer, &PluginTimer::timeout, this, &IntegrationPluginFlowercare::onPluginTimer);
    }

    // Update refresh schedule when the refresh rate setting is changed
    connect(thing, &Thing::settingChanged, flowerCare, [this, thing] {
        FlowerCare *flowerCare = m_list.value(thing);
        int refreshInterval = thing->setting(flowerCareSettingsRefreshRateParamTypeId).toInt();
        if (m_refreshMinutes[flowerCare] > refreshInterval) {
            m_refreshMinutes[flowerCare] = refreshInterval;
        }
    });

    info->finish(Thing::ThingErrorNoError);
}

void IntegrationPluginFlowercare::postSetupThing(Thing *thing)
{
    FlowerCare *flowerCare = m_list.value(thing);
    flowerCare->refreshData();
}

void IntegrationPluginFlowercare::thingRemoved(Thing *thing)
{
    FlowerCare *flowerCare = m_list.take(thing);
    if (!flowerCare) {
        return;
    }

    hardwareManager()->bluetoothLowEnergyManager()->unregisterDevice(flowerCare->btDevice());
    flowerCare->deleteLater();

    if (m_list.isEmpty() && m_reconnectTimer) {
        hardwareManager()->pluginTimerManager()->unregisterTimer(m_reconnectTimer);
        m_reconnectTimer = nullptr;
    }
}

void IntegrationPluginFlowercare::onPluginTimer()
{

    foreach (FlowerCare *flowerCare, m_list) {
        if (--m_refreshMinutes[flowerCare] <= 0) {
            qCDebug(dcFlowerCare()) << "Refreshing" << flowerCare->btDevice()->address();
            flowerCare->refreshData();
        } else {
            qCDebug(dcFlowerCare()) << "Not refreshing" << flowerCare->btDevice()->address() << " Next refresh in" << m_refreshMinutes[flowerCare] << "minutes";
        }

        // If we had 2 or more failed connection attempts, mark it as disconnected
        if (m_refreshMinutes[flowerCare] < -2) {
            qCDebug(dcFlowerCare()) << "Failed to refresh for"<< (m_refreshMinutes[flowerCare] * -1) << "minutes. Marking as unreachable";
            m_list.key(flowerCare)->setStateValue(flowerCareConnectedStateTypeId, false);
        }
    }
}

void IntegrationPluginFlowercare::onSensorDataReceived(quint8 batteryLevel, double degreeCelsius, double lux, double moisture, double fertility)
{
    FlowerCare *flowerCare = static_cast<FlowerCare*>(sender());
    Thing *thing = m_list.key(flowerCare);
    thing->setStateValue(flowerCareConnectedStateTypeId, true);
    thing->setStateValue(flowerCareBatteryLevelStateTypeId, batteryLevel);
    thing->setStateValue(flowerCareBatteryCriticalStateTypeId, batteryLevel <= 10);
    thing->setStateValue(flowerCareTemperatureStateTypeId, degreeCelsius);
    thing->setStateValue(flowerCareLightIntensityStateTypeId, lux);
    thing->setStateValue(flowerCareMoistureStateTypeId, moisture);
    thing->setStateValue(flowerCareConductivityStateTypeId, fertility);

    m_refreshMinutes[flowerCare] = thing->setting(flowerCareSettingsRefreshRateParamTypeId).toInt();
}
