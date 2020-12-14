﻿/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*
*
* Copyright 2013 - 2020, nymea GmbH
* Contact: contact@nymea.io

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

#include "integrationpluginzigbeegewiss.h"
#include "plugininfo.h"
#include "hardware/zigbee/zigbeehardwareresource.h"

#include <zigbeenodeendpoint.h>

#include <QDebug>

IntegrationPluginZigbeeGewiss::IntegrationPluginZigbeeGewiss()
{
    m_networkUuidParamTypeIds[gewissGwa1501ThingClassId] = gewissGwa1501ThingNetworkUuidParamTypeId;
    m_networkUuidParamTypeIds[gewissGwa1511ThingClassId] = gewissGwa1511ThingNetworkUuidParamTypeId;
    m_networkUuidParamTypeIds[gewissGwa1512ThingClassId] = gewissGwa1512ThingNetworkUuidParamTypeId;
    m_networkUuidParamTypeIds[gewissGwa1513ThingClassId] = gewissGwa1513ThingNetworkUuidParamTypeId;
    m_networkUuidParamTypeIds[gewissGwa1514ThingClassId] = gewissGwa1514ThingNetworkUuidParamTypeId;
    m_networkUuidParamTypeIds[gewissGwa1521ThingClassId] = gewissGwa1521ThingNetworkUuidParamTypeId;

    m_ieeeAddressParamTypeIds[gewissGwa1501ThingClassId] = gewissGwa1501ThingIeeeAddressParamTypeId;
    m_ieeeAddressParamTypeIds[gewissGwa1511ThingClassId] = gewissGwa1511ThingIeeeAddressParamTypeId;
    m_ieeeAddressParamTypeIds[gewissGwa1512ThingClassId] = gewissGwa1512ThingIeeeAddressParamTypeId;
    m_ieeeAddressParamTypeIds[gewissGwa1513ThingClassId] = gewissGwa1513ThingIeeeAddressParamTypeId;
    m_ieeeAddressParamTypeIds[gewissGwa1514ThingClassId] = gewissGwa1514ThingIeeeAddressParamTypeId;
    m_ieeeAddressParamTypeIds[gewissGwa1521ThingClassId] = gewissGwa1521ThingIeeeAddressParamTypeId;

    m_connectedStateTypeIds[gewissGwa1501ThingClassId] = gewissGwa1501ConnectedStateTypeId;
    m_connectedStateTypeIds[gewissGwa1511ThingClassId] = gewissGwa1511ConnectedStateTypeId;
    m_connectedStateTypeIds[gewissGwa1512ThingClassId] = gewissGwa1512ConnectedStateTypeId;
    m_connectedStateTypeIds[gewissGwa1513ThingClassId] = gewissGwa1513ConnectedStateTypeId;
    m_connectedStateTypeIds[gewissGwa1514ThingClassId] = gewissGwa1514ConnectedStateTypeId;
    m_connectedStateTypeIds[gewissGwa1521ThingClassId] = gewissGwa1521ConnectedStateTypeId;

    m_versionStateTypeIds[gewissGwa1501ThingClassId] = gewissGwa1501VersionStateTypeId;
    m_versionStateTypeIds[gewissGwa1511ThingClassId] = gewissGwa1511VersionStateTypeId;
    m_versionStateTypeIds[gewissGwa1512ThingClassId] = gewissGwa1512VersionStateTypeId;
    m_versionStateTypeIds[gewissGwa1513ThingClassId] = gewissGwa1513VersionStateTypeId;
    m_versionStateTypeIds[gewissGwa1514ThingClassId] = gewissGwa1514VersionStateTypeId;
    m_versionStateTypeIds[gewissGwa1521ThingClassId] = gewissGwa1521VersionStateTypeId;

    m_signalStrengthStateTypeIds[gewissGwa1501ThingClassId] = gewissGwa1501SignalStrengthStateTypeId;
    m_signalStrengthStateTypeIds[gewissGwa1511ThingClassId] = gewissGwa1511SignalStrengthStateTypeId;
    m_signalStrengthStateTypeIds[gewissGwa1512ThingClassId] = gewissGwa1512SignalStrengthStateTypeId;
    m_signalStrengthStateTypeIds[gewissGwa1513ThingClassId] = gewissGwa1513SignalStrengthStateTypeId;
    m_signalStrengthStateTypeIds[gewissGwa1514ThingClassId] = gewissGwa1514SignalStrengthStateTypeId;
    m_signalStrengthStateTypeIds[gewissGwa1521ThingClassId] = gewissGwa1521SignalStrengthStateTypeId;

    // Known model identifier
    m_knownGewissDevices.insert("GWA1501_BinaryInput_FC", gewissGwa1501ThingClassId);
    m_knownGewissDevices.insert("GWA1521_Actuator_1_CH_PF", gewissGwa1521ThingClassId);
    m_knownGewissDevices.insert("GWA1511_MotionSensor", gewissGwa1511ThingClassId);
    m_knownGewissDevices.insert("GWA1512_SmokeSensor", gewissGwa1512ThingClassId);
    m_knownGewissDevices.insert("GWA1513_WindowSensor", gewissGwa1513ThingClassId);
    m_knownGewissDevices.insert("GWA1514_FloodingSensor", gewissGwa1512ThingClassId);
}

QString IntegrationPluginZigbeeGewiss::name() const
{
    return "Gewiss";
}

bool IntegrationPluginZigbeeGewiss::handleNode(ZigbeeNode *node, const QUuid &networkUuid)
{
    // Check if this is Gewiss
    foreach (ZigbeeNodeEndpoint *endpoint, node->endpoints()) {
        // Get the model identifier if present from the first endpoint. Also this is out of spec
        if (!endpoint->hasInputCluster(ZigbeeClusterLibrary::ClusterIdBasic)) {
            qCDebug(dcZigBeeGewiss()) << "This endpoint does not have the basic input cluster, skipping" << endpoint;
            continue;
        }

        // Basic cluster exists, so we should have the manufacturer name
        if (!endpoint->manufacturerName().toLower().startsWith("gewiss")) {
            return false;
        }

        ThingClassId thingClassId;
        foreach (const QString &knownGewiss, m_knownGewissDevices.keys()) {
            if (endpoint->modelIdentifier().startsWith(knownGewiss)) {
                thingClassId = m_knownGewissDevices.value(knownGewiss);
                break;
            }
        }
        if (thingClassId.isNull()) {
            qCWarning(dcZigBeeGewiss()) << "Unhandled Gewiss device:" << endpoint->modelIdentifier();
            return false;
        }

        initPowerConfiguration(node);
        createThing(thingClassId, networkUuid, node);
        return true;
    }

    return false;
}

void IntegrationPluginZigbeeGewiss::handleRemoveNode(ZigbeeNode *node, const QUuid &networkUuid)
{
    Q_UNUSED(networkUuid)

    if (m_thingNodes.values().contains(node)) {
        Thing *thing = m_thingNodes.key(node);
        qCDebug(dcZigBeeGewiss()) << node << "for" << thing << "has left the network.";
        m_thingNodes.remove(thing);
        emit autoThingDisappeared(thing->id());
    }
}

void IntegrationPluginZigbeeGewiss::init()
{
    hardwareManager()->zigbeeResource()->registerHandler(this);
}

void IntegrationPluginZigbeeGewiss::setupThing(ThingSetupInfo *info)
{
    qCDebug(dcZigBeeGewiss()) << "Setting up thing" << info->thing()->name();
    Thing *thing = info->thing();
    QUuid networkUuid = thing->paramValue(m_networkUuidParamTypeIds.value(thing->thingClassId())).toUuid();
    ZigbeeAddress zigbeeAddress = ZigbeeAddress(thing->paramValue(m_ieeeAddressParamTypeIds.value(thing->thingClassId())).toString());
    ZigbeeNode *node = hardwareManager()->zigbeeResource()->claimNode(this, networkUuid, zigbeeAddress);
    if (!node) {
        qCWarning(dcZigBeeGewiss()) << "Zigbee node for" << info->thing()->name() << "not found.";
        info->finish(Thing::ThingErrorHardwareNotAvailable);
        return;
    }
    m_thingNodes.insert(thing, node);

    // Update connected state
    thing->setStateValue(m_connectedStateTypeIds.value(thing->thingClassId()), node->reachable());
    connect(node, &ZigbeeNode::reachableChanged, thing, [thing, this](bool reachable){
        thing->setStateValue(m_connectedStateTypeIds.value(thing->thingClassId()), reachable);
    });

    // Update signal strength
    thing->setStateValue(m_signalStrengthStateTypeIds.value(thing->thingClassId()), qRound(node->lqi() * 100.0 / 255.0));
    connect(node, &ZigbeeNode::lqiChanged, thing, [this, thing](quint8 lqi){
        uint signalStrength = qRound(lqi * 100.0 / 255.0);
        qCDebug(dcZigBeeGewiss()) << thing << "signal strength changed" << signalStrength << "%";
        thing->setStateValue(m_signalStrengthStateTypeIds.value(thing->thingClassId()), signalStrength);
    });

    if (thing->thingClassId() == gewissGwa1501ThingClassId) {

        ZigbeeNodeEndpoint *endpoint1 = node->getEndpoint(0x01);
        ZigbeeNodeEndpoint *endpoint2 = node->getEndpoint(0x02);

        if (!endpoint1 || !endpoint2) {
            qCWarning(dcZigBeeGewiss()) << "one ore more endpoints not found" << thing->name();
            return;
        }

        // Get battery level changes
        ZigbeeClusterPowerConfiguration *powerCluster = endpoint1->inputCluster<ZigbeeClusterPowerConfiguration>(ZigbeeClusterLibrary::ClusterIdPowerConfiguration);
        if (!powerCluster) {
            qCWarning(dcZigBeeGewiss()) << "Could not find power configuration cluster on" << thing << endpoint1;
        } else {
            // Only set the initial state if the attribute already exists
            if (powerCluster->hasAttribute(ZigbeeClusterPowerConfiguration::AttributeBatteryPercentageRemaining)) {
                thing->setStateValue(gewissGwa1501BatteryLevelStateTypeId, powerCluster->batteryPercentage());
                thing->setStateValue(gewissGwa1501BatteryCriticalStateTypeId, (powerCluster->batteryPercentage() < 10.0));
            }

            connect(powerCluster, &ZigbeeClusterPowerConfiguration::batteryPercentageChanged, thing, [=](double percentage){
                qCDebug(dcZigBeeGewiss()) << "Battery percentage changed" << percentage << "%" << thing;
                thing->setStateValue(gewissGwa1501BatteryLevelStateTypeId, percentage);
                thing->setStateValue(gewissGwa1501BatteryCriticalStateTypeId, (percentage < 10.0));
            });
        }

        // Receive on/off commands
        ZigbeeClusterOnOff *onOffCluster1 = endpoint1->outputCluster<ZigbeeClusterOnOff>(ZigbeeClusterLibrary::ClusterIdOnOff);
        if (!onOffCluster1) {
            qCWarning(dcZigBeeGewiss()) << "Could not find on/off client cluster on" << thing << endpoint1;
        } else {
            connect(onOffCluster1, &ZigbeeClusterOnOff::commandSent, thing, [=](ZigbeeClusterOnOff::Command command){
                qCDebug(dcZigBeeGewiss()) << thing << "channel 1, on/off changed" << command;
                if (command == ZigbeeClusterOnOff::CommandOn) {
                    emit emitEvent(Event(gewissGwa1501PressedEventTypeId, thing->id(), ParamList() << Param(gewissGwa1501PressedEventButtonNameParamTypeId, "2")));
                } else if (command == ZigbeeClusterOnOff::CommandOff) {
                } else {
                    qCWarning(dcZigBeeGewiss()) << thing << "unhandled command received" << command;
                }
            });

            connect(onOffCluster1, &ZigbeeClusterOnOff::commandOffWithEffectSent, thing, [=](ZigbeeClusterOnOff::Effect effect, quint8 effectVariant){
                qCDebug(dcZigBeeGewiss()) << thing << "OFF button pressed" << effect << effectVariant;
                emit emitEvent(Event(gewissGwa1501PressedEventTypeId, thing->id(), ParamList() << Param(gewissGwa1501PressedEventButtonNameParamTypeId, "OFF")));
            });
        }

        // Receive level control commands
        ZigbeeClusterLevelControl *levelCluster = endpoint1->outputCluster<ZigbeeClusterLevelControl>(ZigbeeClusterLibrary::ClusterIdLevelControl);
        if (!levelCluster) {
            qCWarning(dcZigBeeGewiss()) << "Could not find level client cluster on" << thing << endpoint1;
        } else {
            connect(levelCluster, &ZigbeeClusterLevelControl::commandStepSent, thing, [=](ZigbeeClusterLevelControl::FadeMode fadeMode, quint8 stepSize, quint16 transitionTime){
                qCDebug(dcZigBeeGewiss()) << thing << "level button pressed" << fadeMode << stepSize << transitionTime;
                switch (fadeMode) {
                case ZigbeeClusterLevelControl::FadeModeUp:
                    qCDebug(dcZigBeeGewiss()) << thing << "DIM UP pressed";
                    emit emitEvent(Event(gewissGwa1501PressedEventTypeId, thing->id(), ParamList() << Param(gewissGwa1501PressedEventButtonNameParamTypeId, "1")));
                    break;
                case ZigbeeClusterLevelControl::FadeModeDown:
                    qCDebug(dcZigBeeGewiss()) << thing << "DIM DOWN pressed";
                    emit emitEvent(Event(gewissGwa1501PressedEventTypeId, thing->id(), ParamList() << Param(gewissGwa1501PressedEventButtonNameParamTypeId, "DIM DOWN")));
                    break;
                }
            });
        }

        // Receive on/off commands
        ZigbeeClusterOnOff *onOffCluster2 = endpoint2->outputCluster<ZigbeeClusterOnOff>(ZigbeeClusterLibrary::ClusterIdOnOff);
        if (!onOffCluster2) {
            qCWarning(dcZigBeeGewiss()) << "Could not find on/off client cluster on" << thing << endpoint2;
        } else {
            connect(onOffCluster2, &ZigbeeClusterOnOff::commandSent, thing, [=](ZigbeeClusterOnOff::Command command){
                qCDebug(dcZigBeeGewiss()) << thing << "channel 2, on/off changed" << command;
                if (command == ZigbeeClusterOnOff::CommandOn) {
                    emit emitEvent(Event(gewissGwa1501PressedEventTypeId, thing->id(), ParamList() << Param(gewissGwa1501PressedEventButtonNameParamTypeId, "2")));
                } else if (command == ZigbeeClusterOnOff::CommandOff) {
                } else {
                    qCWarning(dcZigBeeGewiss()) << thing << "unhandled command received" << command;
                }
            });
            connect(onOffCluster2, &ZigbeeClusterOnOff::commandOnWithTimedOffSent, thing, [=] (bool acceptOnlyWhenOn, quint16 onTime, quint16 offTime) {
                Q_UNUSED(acceptOnlyWhenOn)
                qCDebug(dcZigBeeGewiss()) << thing << "On button pressed, including timed off" << offTime << "on time" << onTime;
            });
            connect(onOffCluster2, &ZigbeeClusterOnOff::commandOffWithEffectSent, thing, [=](ZigbeeClusterOnOff::Effect effect, quint8 effectVariant){
                qCDebug(dcZigBeeGewiss()) << thing << "OFF button pressed" << effect << effectVariant;
                emit emitEvent(Event(gewissGwa1501PressedEventTypeId, thing->id(), ParamList() << Param(gewissGwa1501PressedEventButtonNameParamTypeId, "OFF")));
            });

            // Receive level control commands
            ZigbeeClusterLevelControl *levelCluster2 = endpoint2->outputCluster<ZigbeeClusterLevelControl>(ZigbeeClusterLibrary::ClusterIdLevelControl);
            if (!levelCluster2) {
                qCWarning(dcZigBeeGewiss()) << "Could not find level client cluster on" << thing << endpoint2;
            } else {
                connect(levelCluster2, &ZigbeeClusterLevelControl::commandStepSent, thing, [=](ZigbeeClusterLevelControl::FadeMode fadeMode, quint8 stepSize, quint16 transitionTime){
                    qCDebug(dcZigBeeGewiss()) << thing << "level button pressed" << fadeMode << stepSize << transitionTime;
                    switch (fadeMode) {
                    case ZigbeeClusterLevelControl::FadeModeUp:
                        qCDebug(dcZigBeeGewiss()) << thing << "cluster 2 DIM UP pressed";
                        emit emitEvent(Event(gewissGwa1501PressedEventTypeId, thing->id(), ParamList() << Param(gewissGwa1501PressedEventButtonNameParamTypeId, "2")));
                        break;
                    case ZigbeeClusterLevelControl::FadeModeDown:
                        qCDebug(dcZigBeeGewiss()) << thing << "cluster 2 DIM DOWN pressed";
                        emit emitEvent(Event(gewissGwa1501PressedEventTypeId, thing->id(), ParamList() << Param(gewissGwa1501PressedEventButtonNameParamTypeId, "DIM DOWN")));
                        break;
                    }
                });
            }
        }
        return info->finish(Thing::ThingErrorNoError);

        // Single channel relay
    } else if (thing->thingClassId() == gewissGwa1521ThingClassId) {
        ZigbeeNodeEndpoint *endpoint = node->getEndpoint(0x01);
        if (!endpoint) {
            qCWarning(dcZigBeeGewiss()) << "Endpoint not found" << thing->name();
            return;
        }

        ZigbeeClusterOnOff *onOffCluster = endpoint->inputCluster<ZigbeeClusterOnOff>(ZigbeeClusterLibrary::ClusterIdOnOff);
        if (!onOffCluster) {
            qCWarning(dcZigBeeGewiss()) << "Could not find on/off cluster on" << thing << endpoint;
        } else {
            if (onOffCluster->hasAttribute(ZigbeeClusterOnOff::AttributeOnOff)) {
                thing->setStateValue(gewissGwa1521RelayStateTypeId, onOffCluster->power());
            }

            connect(onOffCluster, &ZigbeeClusterOnOff::powerChanged, thing, [thing](bool power){
                qCDebug(dcZigBeeGewiss()) << thing << "power changed" << power;
                thing->setStateValue(gewissGwa1521RelayStateTypeId, power);
            });
        }
        return info->finish(Thing::ThingErrorNoError);
        //Motion sensor
    } else if (thing->thingClassId() == gewissGwa1511ThingClassId) {

        // Home Automation Device Occupacy Sensor
        ZigbeeNodeEndpoint *occupancyEndpoint = node->getEndpoint(0x22);
        if (!occupancyEndpoint) {
            qCWarning(dcZigBeeGewiss()) << "Occupancy endpoint not found" << thing->name();
            return info->finish(Thing::ThingErrorSetupFailed);
        }
        ZigbeeClusterOccupancySensing *occupancyCluster = occupancyEndpoint->inputCluster<ZigbeeClusterOccupancySensing>(ZigbeeClusterLibrary::ClusterIdOccupancySensing);
        if (!occupancyCluster) {
            qCWarning(dcZigBeeGewiss()) << "Occupancy cluster not found" << thing->name();
            return info->finish(Thing::ThingErrorSetupFailed);
        }

        if (occupancyCluster->hasAttribute(ZigbeeClusterOccupancySensing::AttributeOccupancy)) {
            thing->setStateValue(gewissGwa1511IsPresentStateTypeId, occupancyCluster->occupied());
            thing->setStateValue(gewissGwa1511LastSeenTimeStateTypeId, QDateTime::currentMSecsSinceEpoch() / 1000);
        }

        connect(occupancyCluster, &ZigbeeClusterOccupancySensing::occupancyChanged, thing, [this, thing](bool occupancy) {
            qCDebug(dcZigBeeGewiss()) << "occupancy changed" << occupancy;
            // Only change the state if the it changed to true, it will be disabled by the timer
            if (occupancy) {
                thing->setStateValue(gewissGwa1511IsPresentStateTypeId, occupancy);
                m_presenceTimer->start();
            }
            thing->setStateValue(gewissGwa1511LastSeenTimeStateTypeId, QDateTime::currentMSecsSinceEpoch() / 1000);
        });

        if (!m_presenceTimer) {
            m_presenceTimer = hardwareManager()->pluginTimerManager()->registerTimer(1);
        }

        connect(m_presenceTimer, &PluginTimer::timeout, thing, [thing] {
            if (thing->stateValue(gewissGwa1511IsPresentStateTypeId).toBool()) {
                int timeout = thing->setting(gewissGwa1511SettingsTimeoutParamTypeId).toInt();
                QDateTime lastSeenTime = QDateTime::fromMSecsSinceEpoch(thing->stateValue(gewissGwa1511LastSeenTimeStateTypeId).toULongLong() * 1000);
                if (lastSeenTime.addSecs(timeout) < QDateTime::currentDateTime()) {
                    thing->setStateValue(gewissGwa1511IsPresentStateTypeId, false);
                }
            }
        });

        // Home Automation Device
        ZigbeeNodeEndpoint *temperatureEndpoint = node->getEndpoint(0x26);
        if (!temperatureEndpoint) {
            qCWarning(dcZigBeeGewiss()) << "Temperature endpoint not found" << thing->name();
            return info->finish(Thing::ThingErrorSetupFailed);
        }

        ZigbeeClusterTemperatureMeasurement *temperatureCluster = temperatureEndpoint->inputCluster<ZigbeeClusterTemperatureMeasurement>(ZigbeeClusterLibrary::ClusterIdTemperatureMeasurement);
        if (!temperatureCluster) {
            qCWarning(dcZigBeeGewiss()) << "Temperature cluster not found" << thing->name();
            return info->finish(Thing::ThingErrorSetupFailed);
        }

        if (temperatureCluster->hasAttribute(ZigbeeClusterTemperatureMeasurement::AttributeMeasuredValue)) {
            thing->setStateValue(gewissGwa1511TemperatureStateTypeId, temperatureCluster->temperature());
        }
        connect(temperatureCluster, &ZigbeeClusterTemperatureMeasurement::temperatureChanged, thing, [thing](double temperature){
            qCDebug(dcZigBeeGewiss()) << thing << "temperature changed" << temperature << "°C";
            thing->setStateValue(gewissGwa1511TemperatureStateTypeId, temperature);
        });

        //Home Automation Device Light Sensor Endpoint
        ZigbeeNodeEndpoint *lightSensorEndpoint = node->getEndpoint(0x27);
        if (!lightSensorEndpoint) {
            qCWarning(dcZigBeeGewiss()) << "Light sensor endpoint not found" << thing->name();
            return info->finish(Thing::ThingErrorSetupFailed);
        }

        ZigbeeClusterIlluminanceMeasurment *illuminanceCluster = lightSensorEndpoint->inputCluster<ZigbeeClusterIlluminanceMeasurment>(ZigbeeClusterLibrary::ClusterIdIlluminanceMeasurement);
        if (!illuminanceCluster) {
            qCWarning(dcZigBeeGewiss()) << "Illuminance cluster not found" << thing->name();
            return info->finish(Thing::ThingErrorSetupFailed);
        }
        // Only set the state if the cluster actually has the attribute
        if (illuminanceCluster->hasAttribute(ZigbeeClusterIlluminanceMeasurment::AttributeMeasuredValue)) {
            thing->setStateValue(gewissGwa1511LightIntensityStateTypeId, illuminanceCluster->illuminance());
        }

        connect(illuminanceCluster, &ZigbeeClusterIlluminanceMeasurment::illuminanceChanged, thing, [thing](quint16 illuminance){
            qCDebug(dcZigBeeGewiss()) << thing << "light intensity changed" << illuminance << "lux";
            thing->setStateValue(gewissGwa1511LightIntensityStateTypeId, illuminance);
        });


        //Home Automation Power Configuration Endpoint
        ZigbeeNodeEndpoint *powerConfigurationEndpoint = node->getEndpoint(0x23);
        if (!powerConfigurationEndpoint ) {
            qCWarning(dcZigBeeGewiss()) << "Power configuration endpoint not found" << thing->name();
            return info->finish(Thing::ThingErrorSetupFailed);
        }
        // Get battery level changes
        ZigbeeClusterPowerConfiguration *powerCluster = powerConfigurationEndpoint->inputCluster<ZigbeeClusterPowerConfiguration>(ZigbeeClusterLibrary::ClusterIdPowerConfiguration);
        if (!powerCluster) {
            qCWarning(dcZigBeeGewiss()) << "Could not find power configuration cluster on" << thing << powerConfigurationEndpoint ;
            return info->finish(Thing::ThingErrorSetupFailed);
        }
        // Only set the initial state if the attribute already exists
        if (powerCluster->hasAttribute(ZigbeeClusterPowerConfiguration::AttributeBatteryPercentageRemaining)) {
            thing->setStateValue(gewissGwa1511BatteryLevelStateTypeId, powerCluster->batteryPercentage());
            thing->setStateValue(gewissGwa1511BatteryCriticalStateTypeId, (powerCluster->batteryPercentage() < 10.0));
        }

        connect(powerCluster, &ZigbeeClusterPowerConfiguration::batteryPercentageChanged, thing, [=](double percentage){
            qCDebug(dcZigBeeGewiss()) << "Battery percentage changed" << percentage << "%" << thing;
            thing->setStateValue(gewissGwa1511BatteryLevelStateTypeId, percentage);
            thing->setStateValue(gewissGwa1511BatteryCriticalStateTypeId, (percentage < 10.0));
        });

        return info->finish(Thing::ThingErrorNoError);

        // Smoke sensor
    } else if (thing->thingClassId() == gewissGwa1512ThingClassId) {
        ZigbeeNodeEndpoint *endpoint = node->getEndpoint(0x01);
        if (!endpoint) {
            qCWarning(dcZigBeeGewiss()) << "Endpoint not found" << thing->name();
            return;
        }
        return info->finish(Thing::ThingErrorNoError);

        // Window contact sensor
    } else if (thing->thingClassId() == gewissGwa1513ThingClassId) {
        ZigbeeNodeEndpoint *endpoint = node->getEndpoint(0x01);
        if (!endpoint) {
            qCWarning(dcZigBeeGewiss()) << "Endpoint not found" << thing->name();
            return;
        }

        ZigbeeClusterOnOff *onOffCluster = endpoint->inputCluster<ZigbeeClusterOnOff>(ZigbeeClusterLibrary::ClusterIdOnOff);
        if (!onOffCluster) {
            qCWarning(dcZigBeeGewiss()) << "Could not find on/off cluster on" << thing << endpoint;
        } else {
            if (onOffCluster->hasAttribute(ZigbeeClusterOnOff::AttributeOnOff)) {
                thing->setStateValue(gewissGwa1513ClosedStateTypeId, onOffCluster->power());
            }

            connect(onOffCluster, &ZigbeeClusterOnOff::powerChanged, thing, [thing](bool power){
                qCDebug(dcZigBeeGewiss()) << thing << "power changed" << power;
                thing->setStateValue(gewissGwa1513ClosedStateTypeId, power);
            });
        }
        return info->finish(Thing::ThingErrorNoError);
        //Flood sensor
    } else if (thing->thingClassId() == gewissGwa1514ThingClassId) {
        ZigbeeNodeEndpoint *endpoint = node->getEndpoint(0x01);
        if (!endpoint) {
            qCWarning(dcZigBeeGewiss()) << "Endpoint not found" << thing->name();
            return;
        }

        ZigbeeClusterOnOff *onOffCluster = endpoint->inputCluster<ZigbeeClusterOnOff>(ZigbeeClusterLibrary::ClusterIdOnOff);
        if (!onOffCluster) {
            qCWarning(dcZigBeeGewiss()) << "Could not find on/off cluster on" << thing << endpoint;
        } else {
            if (onOffCluster->hasAttribute(ZigbeeClusterOnOff::AttributeOnOff)) {
                thing->setStateValue(gewissGwa1514WaterDetectedStateTypeId, onOffCluster->power());
            }

            connect(onOffCluster, &ZigbeeClusterOnOff::powerChanged, thing, [thing](bool power){
                qCDebug(dcZigBeeGewiss()) << thing << "power changed" << power;
                thing->setStateValue(gewissGwa1514WaterDetectedStateTypeId, power);
            });
        }

        //TODO add temperature sensor
        ZigbeeClusterTemperatureMeasurement *temperatureCluster = endpoint->inputCluster<ZigbeeClusterTemperatureMeasurement>(ZigbeeClusterLibrary::ClusterIdTemperatureMeasurement);
        if (!temperatureCluster) {
            qCWarning(dcZigBeeGewiss()) << "Could not find temperature cluster on" << thing << endpoint;
        }
        return info->finish(Thing::ThingErrorNoError);
    } else {
        qCWarning(dcZigBeeGewiss()) << "Thing class not found" << info->thing()->thingClassId();
        return info->finish(Thing::ThingErrorThingClassNotFound);
    }
}

void IntegrationPluginZigbeeGewiss::executeAction(ThingActionInfo *info)
{
    Thing *thing = info->thing();
    Action action = info->action();

    if (thing->thingClassId() == gewissGwa1521ThingClassId) {
        if (action.actionTypeId() == gewissGwa1521RelayActionTypeId) {
            ZigbeeNode *node = m_thingNodes.value(thing);
            if (!node) {
                qCWarning(dcZigBeeGewiss()) << "Zigbee node for" << thing << "not found.";
                info->finish(Thing::ThingErrorHardwareFailure);
                return;
            }

            if (info->action().actionTypeId() == gewissGwa1521RelayActionTypeId) {
                ZigbeeNodeEndpoint *endpoint = node->getEndpoint(0x01);
                if (!endpoint) {
                    qCWarning(dcZigBeeGewiss()) << "Unable to get the endpoint from node" << node << "for" << thing;
                    info->finish(Thing::ThingErrorSetupFailed);
                    return;
                }

                ZigbeeClusterOnOff *onOffCluster = endpoint->inputCluster<ZigbeeClusterOnOff>(ZigbeeClusterLibrary::ClusterIdOnOff);
                if (!onOffCluster) {
                    qCWarning(dcZigBeeGewiss()) << "Unable to get the OnOff cluster from endpoint" << endpoint << "on" << node << "for" << thing;
                    info->finish(Thing::ThingErrorSetupFailed);
                    return;
                }
                bool power = info->action().param(gewissGwa1521RelayActionRelayParamTypeId).value().toBool();
                ZigbeeClusterReply *reply = (power ? onOffCluster->commandOn() : onOffCluster->commandOff());
                connect(reply, &ZigbeeClusterReply::finished, this, [=](){
                    // Note: reply will be deleted automatically
                    if (reply->error() != ZigbeeClusterReply::ErrorNoError) {
                        info->finish(Thing::ThingErrorHardwareFailure);
                    } else {
                        info->finish(Thing::ThingErrorNoError);
                        thing->setStateValue(gewissGwa1521RelayStateTypeId, power);
                    }
                });
            }
        }
    } else {
        qCDebug(dcZigBeeGewiss()) << "Execute action" << info->thing()->name() << info->action().actionTypeId();
        info->finish(Thing::ThingErrorUnsupportedFeature);
    }
}

void IntegrationPluginZigbeeGewiss::thingRemoved(Thing *thing)
{
    qCDebug(dcZigBeeGewiss()) << "Removing thing" << thing->name();
    ZigbeeNode *node = m_thingNodes.take(thing);
    if (node) {
        QUuid networkUuid = thing->paramValue(m_networkUuidParamTypeIds.value(thing->thingClassId())).toUuid();
        hardwareManager()->zigbeeResource()->removeNodeFromNetwork(networkUuid, node);
    }
}

void IntegrationPluginZigbeeGewiss::createThing(const ThingClassId &thingClassId, const QUuid &networkUuid, ZigbeeNode *node)
{
    ThingDescriptor descriptor(thingClassId);
    QString deviceClassName = supportedThings().findById(thingClassId).displayName();
    descriptor.setTitle(deviceClassName);

    ParamList params;
    params.append(Param(m_networkUuidParamTypeIds[thingClassId], networkUuid.toString()));
    params.append(Param(m_ieeeAddressParamTypeIds[thingClassId], node->extendedAddress().toString()));
    descriptor.setParams(params);
    emit autoThingsAppeared({descriptor});
}

void IntegrationPluginZigbeeGewiss::initPowerConfiguration(ZigbeeNode *node)
{
    qCDebug(dcZigBeeGewiss()) << "Initializing node" << node->modelName();
    ZigbeeNodeEndpoint *powerConfigurationEndpoint;

    Q_FOREACH(ZigbeeNodeEndpoint *endpoint, node->endpoints()) {
        if (endpoint->hasInputCluster(ZigbeeClusterLibrary::ClusterIdPowerConfiguration)) {
            powerConfigurationEndpoint = endpoint;
            break;
        }
    }
    if (!powerConfigurationEndpoint) {
        qCWarning(dcZigBeeGewiss()) << "No power configuration endpoint found";
        return;
    }

    // Get the current configured bindings for this node
    ZigbeeReply *reply = node->removeAllBindings();
    connect(reply, &ZigbeeReply::finished, node, [=](){
        if (reply->error() != ZigbeeReply::ErrorNoError) {
            qCWarning(dcZigBeeGewiss()) << "Failed to remove all bindings for initialization of" << node;
        } else {
            qCDebug(dcZigBeeGewiss()) << "Removed all bindings successfully from" << node;
        }

        // Read battery, bind and configure attribute reporting for battery
        if (!powerConfigurationEndpoint->hasInputCluster(ZigbeeClusterLibrary::ClusterIdPowerConfiguration)) {
            qCWarning(dcZigBeeGewiss()) << "Failed to initialize the power configuration cluster because the cluster could not be found" << node << powerConfigurationEndpoint;
            return;
        }

        qCDebug(dcZigBeeGewiss()) << "Read power configuration cluster attributes" << node;
        ZigbeeClusterReply *readAttributeReply = powerConfigurationEndpoint->getInputCluster(ZigbeeClusterLibrary::ClusterIdPowerConfiguration)->readAttributes({ZigbeeClusterPowerConfiguration::AttributeBatteryPercentageRemaining});
        connect(readAttributeReply, &ZigbeeClusterReply::finished, node, [=](){
            if (readAttributeReply->error() != ZigbeeClusterReply::ErrorNoError) {
                qCWarning(dcZigBeeGewiss()) << "Failed to read power cluster attributes" << readAttributeReply->error();
            } else {
                qCDebug(dcZigBeeGewiss()) << "Read power configuration cluster attributes finished successfully";
            }

            // Bind the cluster to the coordinator
            qCDebug(dcZigBeeGewiss()) << "Bind power configuration cluster to coordinator IEEE address";
            ZigbeeDeviceObjectReply * zdoReply = node->deviceObject()->requestBindIeeeAddress(powerConfigurationEndpoint->endpointId(), ZigbeeClusterLibrary::ClusterIdPowerConfiguration,
                                                                                              hardwareManager()->zigbeeResource()->coordinatorAddress(node->networkUuid()), 0x01);
            connect(zdoReply, &ZigbeeDeviceObjectReply::finished, node, [=](){
                if (zdoReply->error() != ZigbeeDeviceObjectReply::ErrorNoError) {
                    qCWarning(dcZigBeeGewiss()) << "Failed to bind power cluster to coordinator" << zdoReply->error();
                } else {
                    qCDebug(dcZigBeeGewiss()) << "Bind power configuration cluster to coordinator finished successfully";
                }

                // Configure attribute rporting for battery remaining (0.5 % changes = 1)
                ZigbeeClusterLibrary::AttributeReportingConfiguration reportingConfig;
                reportingConfig.attributeId = ZigbeeClusterPowerConfiguration::AttributeBatteryPercentageRemaining;
                reportingConfig.dataType = Zigbee::Uint8;
                reportingConfig.minReportingInterval = 300;
                reportingConfig.maxReportingInterval = 2700;
                reportingConfig.reportableChange = ZigbeeDataType(static_cast<quint8>(1)).data();

                qCDebug(dcZigBeeGewiss()) << "Configure attribute reporting for power configuration cluster to coordinator";
                ZigbeeClusterReply *reportingReply = powerConfigurationEndpoint->getInputCluster(ZigbeeClusterLibrary::ClusterIdPowerConfiguration)->configureReporting({reportingConfig});
                connect(reportingReply, &ZigbeeClusterReply::finished, this, [=] {
                    if (reportingReply->error() != ZigbeeClusterReply::ErrorNoError) {
                        qCWarning(dcZigBeeGewiss()) << "Failed to configure power cluster attribute reporting" << reportingReply->error();
                    } else {
                        qCDebug(dcZigBeeGewiss()) << "Attribute reporting configuration finished for power cluster" << ZigbeeClusterLibrary::parseAttributeReportingStatusRecords(reportingReply->responseFrame().payload);
                    }
                });
            });
        });
    });
}