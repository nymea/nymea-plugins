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

#include "integrationplugintexasinstruments.h"
#include "plugininfo.h"
#include "sensortag.h"

#include <hardware/bluetoothlowenergy/bluetoothlowenergymanager.h>
#include <plugintimer.h>

#include <QBluetoothDeviceInfo>

IntegrationPluginTexasInstruments::IntegrationPluginTexasInstruments(QObject *parent) : IntegrationPlugin(parent)
{

}

IntegrationPluginTexasInstruments::~IntegrationPluginTexasInstruments()
{

}

void IntegrationPluginTexasInstruments::discoverThings(ThingDiscoveryInfo *info)
{
    Q_ASSERT_X(info->thingClassId() == sensorTagThingClassId, "DevicePluginTexasInstruments", "Unhandled ThingClassId!");

    if (!hardwareManager()->bluetoothLowEnergyManager()->available() || !hardwareManager()->bluetoothLowEnergyManager()->enabled()) {
        return info->finish(Thing::ThingErrorHardwareNotAvailable, QT_TR_NOOP("Bluetooth is not available on this system."));
    }

    BluetoothDiscoveryReply *reply = hardwareManager()->bluetoothLowEnergyManager()->discoverDevices();
    connect(reply, &BluetoothDiscoveryReply::finished, reply, &BluetoothDiscoveryReply::deleteLater);

    connect(reply, &BluetoothDiscoveryReply::finished, info, [this, info, reply](){

        if (reply->error() != BluetoothDiscoveryReply::BluetoothDiscoveryReplyErrorNoError) {
            qCWarning(dcTexasInstruments()) << "Bluetooth discovery error:" << reply->error();
            info->finish(Thing::ThingErrorHardwareFailure, QT_TR_NOOP("An error happened during Bluetooth discovery."));
            return;
        }

        foreach (const auto &deviceInfo, reply->discoveredDevices()) {

            if (deviceInfo.first.name().contains("SensorTag")) {

                ThingDescriptor descriptor(sensorTagThingClassId, "Sensor Tag", deviceInfo.first.address().toString());

                Things existingThing = myThings().filterByParam(sensorTagThingMacParamTypeId, deviceInfo.first.address().toString());
                if (!existingThing.isEmpty()) {
                    descriptor.setThingId(existingThing.first()->id());
                }

                ParamList params;
                params.append(Param(sensorTagThingMacParamTypeId, deviceInfo.first.address().toString()));
                foreach (Thing *existingThing, myThings()) {
                    if (existingThing->paramValue(sensorTagThingMacParamTypeId).toString() == deviceInfo.first.address().toString()) {
                        descriptor.setThingId(existingThing->id());
                        break;
                    }
                }
                descriptor.setParams(params);
                info->addThingDescriptor(descriptor);
            }
        }

        info->finish(Thing::ThingErrorNoError);
    });
}

void IntegrationPluginTexasInstruments::setupThing(ThingSetupInfo *info)
{
    Thing *thing = info->thing();
    qCDebug(dcTexasInstruments()) << "Setting up Multi Sensor" << thing->name() << thing->params();

    QBluetoothAddress address = QBluetoothAddress(thing->paramValue(sensorTagThingMacParamTypeId).toString());
    QBluetoothDeviceInfo deviceInfo = QBluetoothDeviceInfo(address, thing->name(), 0);

    BluetoothLowEnergyDevice *bluetoothDevice = hardwareManager()->bluetoothLowEnergyManager()->registerDevice(deviceInfo, QLowEnergyController::PublicAddress);

    SensorTag *sensorTag = new SensorTag(thing, bluetoothDevice, this);
    m_sensorTags.insert(thing, sensorTag);

    if (!m_reconnectTimer) {
        m_reconnectTimer = hardwareManager()->pluginTimerManager()->registerTimer(10);
        connect(m_reconnectTimer, &PluginTimer::timeout, this, [this](){
            foreach (SensorTag *sensorTag, m_sensorTags) {
                if (!sensorTag->bluetoothDevice()->connected()) {
                    sensorTag->bluetoothDevice()->connectDevice();
                }
            }
        });
    }

    info->finish(Thing::ThingErrorNoError);
}

void IntegrationPluginTexasInstruments::postSetupThing(Thing *thing)
{
    // Try to connect right after setup
    SensorTag *sensorTag = m_sensorTags.value(thing);

    // Configure sensor with state configurations
    sensorTag->setTemperatureSensorEnabled(thing->stateValue(sensorTagTemperatureSensorEnabledStateTypeId).toBool());
    sensorTag->setHumiditySensorEnabled(thing->stateValue(sensorTagHumiditySensorEnabledStateTypeId).toBool());
    sensorTag->setPressureSensorEnabled(thing->stateValue(sensorTagPressureSensorEnabledStateTypeId).toBool());
    sensorTag->setOpticalSensorEnabled(thing->stateValue(sensorTagOpticalSensorEnabledStateTypeId).toBool());
    sensorTag->setAccelerometerEnabled(thing->stateValue(sensorTagAccelerometerEnabledStateTypeId).toBool());
    sensorTag->setGyroscopeEnabled(thing->stateValue(sensorTagGyroscopeEnabledStateTypeId).toBool());
    sensorTag->setMagnetometerEnabled(thing->stateValue(sensorTagMagnetometerEnabledStateTypeId).toBool());
    sensorTag->setMeasurementPeriod(thing->stateValue(sensorTagMeasurementPeriodStateTypeId).toInt());
    sensorTag->setMeasurementPeriodMovement(thing->stateValue(sensorTagMeasurementPeriodMovementStateTypeId).toInt());

    // Connect to the sensor
    sensorTag->bluetoothDevice()->connectDevice();
}

void IntegrationPluginTexasInstruments::thingRemoved(Thing *thing)
{
    if (!m_sensorTags.contains(thing)) {
        return;
    }

    SensorTag *sensorTag = m_sensorTags.take(thing);
    hardwareManager()->bluetoothLowEnergyManager()->unregisterDevice(sensorTag->bluetoothDevice());
    sensorTag->deleteLater();

    if (myThings().isEmpty()) {
        hardwareManager()->pluginTimerManager()->unregisterTimer(m_reconnectTimer);
        m_reconnectTimer = nullptr;
    }
}

void IntegrationPluginTexasInstruments::executeAction(ThingActionInfo *info)
{
    Thing *thing = info->thing();
    Action action = info->action();

    SensorTag *sensorTag = m_sensorTags.value(thing);
    if (action.actionTypeId() == sensorTagBuzzerActionTypeId) {
        sensorTag->setBuzzerPower(action.param(sensorTagBuzzerActionBuzzerParamTypeId).value().toBool());
        return info->finish(Thing::ThingErrorNoError);
    } else if (action.actionTypeId() == sensorTagGreenLedActionTypeId) {
        sensorTag->setGreenLedPower(action.param(sensorTagGreenLedActionGreenLedParamTypeId).value().toBool());
        return info->finish(Thing::ThingErrorNoError);
    } else if (action.actionTypeId() == sensorTagRedLedActionTypeId) {
        sensorTag->setRedLedPower(action.param(sensorTagRedLedActionRedLedParamTypeId).value().toBool());
        return info->finish(Thing::ThingErrorNoError);
    } else if (action.actionTypeId() == sensorTagBuzzerImpulseActionTypeId) {
        sensorTag->buzzerImpulse();
        return info->finish(Thing::ThingErrorNoError);
    } else if (action.actionTypeId() == sensorTagTemperatureSensorEnabledActionTypeId) {
        bool enabled = action.param(sensorTagTemperatureSensorEnabledActionTemperatureSensorEnabledParamTypeId).value().toBool();
        thing->setStateValue(sensorTagTemperatureSensorEnabledStateTypeId, enabled);
        sensorTag->setTemperatureSensorEnabled(enabled);
        return info->finish(Thing::ThingErrorNoError);
    } else if (action.actionTypeId() == sensorTagHumiditySensorEnabledActionTypeId) {
        bool enabled = action.param(sensorTagHumiditySensorEnabledActionHumiditySensorEnabledParamTypeId).value().toBool();
        thing->setStateValue(sensorTagHumiditySensorEnabledStateTypeId, enabled);
        sensorTag->setHumiditySensorEnabled(enabled);
        return info->finish(Thing::ThingErrorNoError);
    } else if (action.actionTypeId() == sensorTagPressureSensorEnabledActionTypeId) {
        bool enabled = action.param(sensorTagPressureSensorEnabledActionPressureSensorEnabledParamTypeId).value().toBool();
        thing->setStateValue(sensorTagPressureSensorEnabledStateTypeId, enabled);
        sensorTag->setPressureSensorEnabled(enabled);
        return info->finish(Thing::ThingErrorNoError);
    } else if (action.actionTypeId() == sensorTagOpticalSensorEnabledActionTypeId) {
        bool enabled = action.param(sensorTagOpticalSensorEnabledActionOpticalSensorEnabledParamTypeId).value().toBool();
        thing->setStateValue(sensorTagOpticalSensorEnabledStateTypeId, enabled);
        sensorTag->setOpticalSensorEnabled(enabled);
        return info->finish(Thing::ThingErrorNoError);
    } else if (action.actionTypeId() == sensorTagAccelerometerEnabledActionTypeId) {
        bool enabled = action.param(sensorTagAccelerometerEnabledActionAccelerometerEnabledParamTypeId).value().toBool();
        thing->setStateValue(sensorTagAccelerometerEnabledStateTypeId, enabled);
        sensorTag->setAccelerometerEnabled(enabled);
        return info->finish(Thing::ThingErrorNoError);
    } else if (action.actionTypeId() == sensorTagGyroscopeEnabledActionTypeId) {
        bool enabled = action.param(sensorTagGyroscopeEnabledActionGyroscopeEnabledParamTypeId).value().toBool();
        thing->setStateValue(sensorTagGyroscopeEnabledStateTypeId, enabled);
        sensorTag->setGyroscopeEnabled(enabled);
        return info->finish(Thing::ThingErrorNoError);
    } else if (action.actionTypeId() == sensorTagMagnetometerEnabledActionTypeId) {
        bool enabled = action.param(sensorTagMagnetometerEnabledActionMagnetometerEnabledParamTypeId).value().toBool();
        thing->setStateValue(sensorTagMagnetometerEnabledStateTypeId, enabled);
        sensorTag->setMagnetometerEnabled(enabled);
        return info->finish(Thing::ThingErrorNoError);
    } else if (action.actionTypeId() == sensorTagMeasurementPeriodActionTypeId) {
        int period = action.param(sensorTagMeasurementPeriodActionMeasurementPeriodParamTypeId).value().toInt();
        thing->setStateValue(sensorTagMeasurementPeriodStateTypeId, period);
        sensorTag->setMeasurementPeriod(period);
        return info->finish(Thing::ThingErrorNoError);
    } else if (action.actionTypeId() == sensorTagMeasurementPeriodMovementActionTypeId) {
        int period = action.param(sensorTagMeasurementPeriodMovementActionMeasurementPeriodMovementParamTypeId).value().toInt();
        thing->setStateValue(sensorTagMeasurementPeriodMovementStateTypeId, period);
        sensorTag->setMeasurementPeriodMovement(period);
        return info->finish(Thing::ThingErrorNoError);
    } else if (action.actionTypeId() == sensorTagMovementSensitivityActionTypeId) {
        int sensitivity = action.param(sensorTagMovementSensitivityActionMovementSensitivityParamTypeId).value().toInt();
        thing->setStateValue(sensorTagMovementSensitivityStateTypeId, sensitivity);
        sensorTag->setMovementSensitivity(sensitivity);
        return info->finish(Thing::ThingErrorNoError);
    }

    Q_ASSERT_X(false, "TexasInstruments", "Unhandled action type: " + action.actionTypeId().toString().toUtf8());
}
