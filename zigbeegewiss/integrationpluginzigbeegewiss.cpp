/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
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
    m_networkUuidParamTypeIds[gwa1501BinaryInputThingClassId] = gwa1501BinaryInputThingNetworkUuidParamTypeId;
    m_networkUuidParamTypeIds[gwa1511MotionSensorThingClassId] = gwa1511MotionSensorThingNetworkUuidParamTypeId;
    m_networkUuidParamTypeIds[gwa1512SmokeSensorThingClassId] = gwa1512SmokeSensorThingNetworkUuidParamTypeId;
    m_networkUuidParamTypeIds[gwa1513WindowSensorThingClassId] = gwa1513WindowSensorThingNetworkUuidParamTypeId;
    m_networkUuidParamTypeIds[gwa1514FloodSensorThingClassId] = gwa1514FloodSensorThingNetworkUuidParamTypeId;
    m_networkUuidParamTypeIds[gwa1521ActuatorThingClassId] = gwa1521ActuatorThingNetworkUuidParamTypeId;

    m_ieeeAddressParamTypeIds[gwa1501BinaryInputThingClassId] = gwa1501BinaryInputThingIeeeAddressParamTypeId;
    m_ieeeAddressParamTypeIds[gwa1511MotionSensorThingClassId] = gwa1511MotionSensorThingIeeeAddressParamTypeId;
    m_ieeeAddressParamTypeIds[gwa1512SmokeSensorThingClassId] = gwa1512SmokeSensorThingIeeeAddressParamTypeId;
    m_ieeeAddressParamTypeIds[gwa1513WindowSensorThingClassId] = gwa1513WindowSensorThingIeeeAddressParamTypeId;
    m_ieeeAddressParamTypeIds[gwa1514FloodSensorThingClassId] = gwa1514FloodSensorThingIeeeAddressParamTypeId;
    m_ieeeAddressParamTypeIds[gwa1521ActuatorThingClassId] = gwa1521ActuatorThingIeeeAddressParamTypeId;

    m_connectedStateTypeIds[gwa1501BinaryInputThingClassId] = gwa1501BinaryInputConnectedStateTypeId;
    m_connectedStateTypeIds[gwa1511MotionSensorThingClassId] = gwa1511MotionSensorConnectedStateTypeId;
    m_connectedStateTypeIds[gwa1512SmokeSensorThingClassId] = gwa1512SmokeSensorConnectedStateTypeId;
    m_connectedStateTypeIds[gwa1513WindowSensorThingClassId] = gwa1513WindowSensorConnectedStateTypeId;
    m_connectedStateTypeIds[gwa1514FloodSensorThingClassId] = gwa1514FloodSensorConnectedStateTypeId;
    m_connectedStateTypeIds[gwa1521ActuatorThingClassId] = gwa1521ActuatorConnectedStateTypeId;

    m_signalStrengthStateTypeIds[gwa1501BinaryInputThingClassId] = gwa1501BinaryInputSignalStrengthStateTypeId;
    m_signalStrengthStateTypeIds[gwa1511MotionSensorThingClassId] = gwa1511MotionSensorSignalStrengthStateTypeId;
    m_signalStrengthStateTypeIds[gwa1512SmokeSensorThingClassId] = gwa1512SmokeSensorSignalStrengthStateTypeId;
    m_signalStrengthStateTypeIds[gwa1513WindowSensorThingClassId] = gwa1513WindowSensorSignalStrengthStateTypeId;
    m_signalStrengthStateTypeIds[gwa1514FloodSensorThingClassId] = gwa1514FloodSensorSignalStrengthStateTypeId;
    m_signalStrengthStateTypeIds[gwa1521ActuatorThingClassId] = gwa1521ActuatorSignalStrengthStateTypeId;

    m_temperatureStateTypeIds[gwa1511MotionSensorThingClassId] = gwa1511MotionSensorTemperatureStateTypeId;
    m_temperatureStateTypeIds[gwa1512SmokeSensorThingClassId] = gwa1512SmokeSensorTemperatureStateTypeId;
    m_temperatureStateTypeIds[gwa1513WindowSensorThingClassId] = gwa1513WindowSensorTemperatureStateTypeId;
    m_temperatureStateTypeIds[gwa1514FloodSensorThingClassId] = gwa1514FloodSensorTemperatureStateTypeId;

    m_batteryLevelStateTypeIds[gwa1501BinaryInputThingClassId] = gwa1501BinaryInputBatteryLevelStateTypeId;
    m_batteryLevelStateTypeIds[gwa1511MotionSensorThingClassId] = gwa1511MotionSensorBatteryLevelStateTypeId;
    m_batteryLevelStateTypeIds[gwa1512SmokeSensorThingClassId] = gwa1512SmokeSensorBatteryLevelStateTypeId;
    m_batteryLevelStateTypeIds[gwa1513WindowSensorThingClassId] = gwa1513WindowSensorBatteryLevelStateTypeId;
    m_batteryLevelStateTypeIds[gwa1514FloodSensorThingClassId] = gwa1514FloodSensorBatteryLevelStateTypeId;

    m_batteryCriticalStateTypeIds[gwa1501BinaryInputThingClassId] = gwa1501BinaryInputBatteryCriticalStateTypeId;
    m_batteryCriticalStateTypeIds[gwa1511MotionSensorThingClassId] = gwa1511MotionSensorBatteryCriticalStateTypeId;
    m_batteryCriticalStateTypeIds[gwa1512SmokeSensorThingClassId] = gwa1512SmokeSensorBatteryCriticalStateTypeId;
    m_batteryCriticalStateTypeIds[gwa1513WindowSensorThingClassId] = gwa1513WindowSensorBatteryCriticalStateTypeId;
    m_batteryCriticalStateTypeIds[gwa1514FloodSensorThingClassId] = gwa1514FloodSensorBatteryCriticalStateTypeId;

    // Known model identifier
    m_knownGewissDevices.insert("GWA1501_BinaryInput_FC", gwa1501BinaryInputThingClassId);
    m_knownGewissDevices.insert("GWA1511_MotionSensor", gwa1511MotionSensorThingClassId);
    m_knownGewissDevices.insert("GWA1512_SmokeSensor", gwa1512SmokeSensorThingClassId);
    m_knownGewissDevices.insert("GWA1513_WindowSensor", gwa1513WindowSensorThingClassId);
    m_knownGewissDevices.insert("GWA1514_FloodingSensor", gwa1514FloodSensorThingClassId);
    m_knownGewissDevices.insert("GWA1521_Actuator_1_CH_PF", gwa1521ActuatorThingClassId);
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

        // Window sensor
        if (thingClassId == gwa1513WindowSensorThingClassId) {
            if (!initWindowSensor(node)) {
                return false;
            }
        } else if (thingClassId == gwa1501BinaryInputThingClassId) {
            if (!initBinaryInput(node)) {
                return false;
            }
        }

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

    if (thing->thingClassId() == gwa1501BinaryInputThingClassId) {

        ZigbeeNodeEndpoint *endpoint1 = node->getEndpoint(0x01);
        ZigbeeNodeEndpoint *endpoint2 = node->getEndpoint(0x02);

        if (!endpoint1 || !endpoint2) {
            qCWarning(dcZigBeeGewiss()) << "one ore more endpoints not found" << thing->name();
            return;
        }

        ZigbeeClusterBinaryInput *binaryCluster1 = endpoint1->inputCluster<ZigbeeClusterBinaryInput>(ZigbeeClusterLibrary::ClusterIdBinaryInput);
        if (!binaryCluster1) {
            qCWarning(dcZigBeeGewiss()) << "Could not find binary client cluster on" << thing << endpoint1;
        } else {
            connect(binaryCluster1, &ZigbeeClusterBinaryInput::attributeChanged, thing, [=](const ZigbeeClusterAttribute &attribute){
                qCDebug(dcZigBeeGewiss()) << thing << "Attribute changed" << attribute;
            });
        }
        ZigbeeClusterBinaryInput *binaryCluster2 = endpoint2->inputCluster<ZigbeeClusterBinaryInput>(ZigbeeClusterLibrary::ClusterIdBinaryInput);
        if (!binaryCluster2) {
            qCWarning(dcZigBeeGewiss()) << "Could not find binary client cluster on" << thing << endpoint1;
        } else {
            connect(binaryCluster2, &ZigbeeClusterBinaryInput::attributeChanged, thing, [=](const ZigbeeClusterAttribute &attribute){
                qCDebug(dcZigBeeGewiss()) << thing << "Attribute changed" << attribute;
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
                    emit emitEvent(Event(gwa1501BinaryInputPressedEventTypeId, thing->id(), ParamList() << Param(gwa1501BinaryInputPressedEventButtonNameParamTypeId, "2")));
                } else if (command == ZigbeeClusterOnOff::CommandOff) {
                } else {
                    qCWarning(dcZigBeeGewiss()) << thing << "unhandled command received" << command;
                }
            });
            connect(onOffCluster1, &ZigbeeClusterOnOff::commandOnWithTimedOffSent, thing, [=] (bool acceptOnlyWhenOn, quint16 onTime, quint16 offTime) {
                Q_UNUSED(acceptOnlyWhenOn)
                qCDebug(dcZigBeeGewiss()) << thing << "On button pressed, including timed off" << offTime << "on time" << onTime;
            });
            connect(onOffCluster1, &ZigbeeClusterOnOff::commandOffWithEffectSent, thing, [=](ZigbeeClusterOnOff::Effect effect, quint8 effectVariant){
                qCDebug(dcZigBeeGewiss()) << thing << "OFF button pressed" << effect << effectVariant;
                emit emitEvent(Event(gwa1501BinaryInputPressedEventTypeId, thing->id(), ParamList() << Param(gwa1501BinaryInputPressedEventButtonNameParamTypeId, "OFF")));
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
                    emit emitEvent(Event(gwa1501BinaryInputPressedEventTypeId, thing->id(), ParamList() << Param(gwa1501BinaryInputPressedEventButtonNameParamTypeId, "1")));
                    break;
                case ZigbeeClusterLevelControl::FadeModeDown:
                    qCDebug(dcZigBeeGewiss()) << thing << "DIM DOWN pressed";
                    emit emitEvent(Event(gwa1501BinaryInputPressedEventTypeId, thing->id(), ParamList() << Param(gwa1501BinaryInputPressedEventButtonNameParamTypeId, "DIM DOWN")));
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
                    emit emitEvent(Event(gwa1501BinaryInputPressedEventTypeId, thing->id(), ParamList() << Param(gwa1501BinaryInputPressedEventButtonNameParamTypeId, "2")));
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
                emit emitEvent(Event(gwa1501BinaryInputPressedEventTypeId, thing->id(), ParamList() << Param(gwa1501BinaryInputPressedEventButtonNameParamTypeId, "OFF")));
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
                        emit emitEvent(Event(gwa1501BinaryInputPressedEventTypeId, thing->id(), ParamList() << Param(gwa1501BinaryInputPressedEventButtonNameParamTypeId, "2")));
                        break;
                    case ZigbeeClusterLevelControl::FadeModeDown:
                        qCDebug(dcZigBeeGewiss()) << thing << "cluster 2 DIM DOWN pressed";
                        emit emitEvent(Event(gwa1501BinaryInputPressedEventTypeId, thing->id(), ParamList() << Param(gwa1501BinaryInputPressedEventButtonNameParamTypeId, "DIM DOWN")));
                        break;
                    }
                });
            }
        }
        return info->finish(Thing::ThingErrorNoError);

        // Single channel relay
    } else if (thing->thingClassId() == gwa1521ActuatorThingClassId) {
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
                thing->setStateValue(gwa1521ActuatorRelayStateTypeId, onOffCluster->power());
            }

            connect(onOffCluster, &ZigbeeClusterOnOff::powerChanged, thing, [thing](bool power){
                qCDebug(dcZigBeeGewiss()) << thing << "power changed" << power;
                thing->setStateValue(gwa1521ActuatorRelayStateTypeId, power);
            });
        }
        return info->finish(Thing::ThingErrorNoError);
        //Motion sensor
    } else if (thing->thingClassId() == gwa1511MotionSensorThingClassId) {

        // Home Automation Device Occupacy Sensor
        ZigbeeNodeEndpoint *occupancyEndpoint = node->getEndpoint(0x22);
        if (!occupancyEndpoint) {
            qCWarning(dcZigBeeGewiss()) << "Occupancy endpoint not found" << thing->name();
            return info->finish(Thing::ThingErrorSetupFailed);
        }

        connect(occupancyEndpoint, &ZigbeeNodeEndpoint::clusterAttributeChanged, this, [thing](ZigbeeCluster *cluster, const ZigbeeClusterAttribute &attribute){
            qCDebug(dcZigBeeGewiss()) << "Cluster attribute changed" << thing->name() << cluster;
            if (cluster->clusterId() == ZigbeeClusterLibrary::ClusterIdIasZone) {
                if (attribute.id() == ZigbeeClusterIasZone::AttributeZoneState) {
                    bool valueOk = false;
                    ZigbeeClusterIasZone::ZoneStatusFlags zoneStatus = static_cast<ZigbeeClusterIasZone::ZoneStatusFlags>(attribute.dataType().toUInt16(&valueOk));
                    if (!valueOk) {
                        qCWarning(dcZigBeeGewiss()) << thing << "failed to convert attribute data to uint16 flag. Not updating the states from" << attribute;
                    } else {
                        qCDebug(dcZigBeeGewiss()) << thing << "zone status changed" << zoneStatus;
                    }
                }
            }
        });

        ZigbeeClusterOccupancySensing *occupancyCluster = occupancyEndpoint->inputCluster<ZigbeeClusterOccupancySensing>(ZigbeeClusterLibrary::ClusterIdOccupancySensing);
        if (!occupancyCluster) {
            qCWarning(dcZigBeeGewiss()) << "Occupancy cluster not found" << thing->name();
            return info->finish(Thing::ThingErrorSetupFailed);
        }

        if (occupancyCluster->hasAttribute(ZigbeeClusterOccupancySensing::AttributeOccupancy)) {
            thing->setStateValue(gwa1511MotionSensorIsPresentStateTypeId, occupancyCluster->occupied());
            thing->setStateValue(gwa1511MotionSensorLastSeenTimeStateTypeId, QDateTime::currentMSecsSinceEpoch() / 1000);
        }

        connect(occupancyCluster, &ZigbeeClusterOccupancySensing::occupancyChanged, thing, [this, thing](bool occupancy) {
            qCDebug(dcZigBeeGewiss()) << "occupancy changed" << occupancy;
            // Only change the state if the it changed to true, it will be disabled by the timer
            if (occupancy) {
                thing->setStateValue(gwa1511MotionSensorIsPresentStateTypeId, occupancy);
                m_presenceTimer->start();
            }
            thing->setStateValue(gwa1511MotionSensorLastSeenTimeStateTypeId, QDateTime::currentMSecsSinceEpoch() / 1000);
        });

        if (!m_presenceTimer) {
            m_presenceTimer = hardwareManager()->pluginTimerManager()->registerTimer(1);
        }

        connect(m_presenceTimer, &PluginTimer::timeout, thing, [thing] {
            if (thing->stateValue(gwa1511MotionSensorIsPresentStateTypeId).toBool()) {
                int timeout = thing->setting(gwa1511MotionSensorSettingsTimeoutParamTypeId).toInt();
                QDateTime lastSeenTime = QDateTime::fromMSecsSinceEpoch(thing->stateValue(gwa1511MotionSensorLastSeenTimeStateTypeId).toULongLong() * 1000);
                if (lastSeenTime.addSecs(timeout) < QDateTime::currentDateTime()) {
                    thing->setStateValue(gwa1511MotionSensorIsPresentStateTypeId, false);
                }
            }
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
            thing->setStateValue(gwa1511MotionSensorLightIntensityStateTypeId, illuminanceCluster->illuminance());
        }

        connect(illuminanceCluster, &ZigbeeClusterIlluminanceMeasurment::illuminanceChanged, thing, [thing](quint16 illuminance){
            qCDebug(dcZigBeeGewiss()) << thing << "light intensity changed" << illuminance << "lux";
            thing->setStateValue(gwa1511MotionSensorLightIntensityStateTypeId, illuminance);
        });

        return info->finish(Thing::ThingErrorNoError);

        // Smoke sensor
    } else if (thing->thingClassId() == gwa1512SmokeSensorThingClassId) {
        ZigbeeNodeEndpoint *endpoint = node->getEndpoint(0x01);
        if (!endpoint) {
            qCWarning(dcZigBeeGewiss()) << "Endpoint not found" << thing->name();
            return;
        }
        return info->finish(Thing::ThingErrorNoError);

    } else if (thing->thingClassId() == gwa1513WindowSensorThingClassId) {
        // Window sensor
        ZigbeeNodeEndpoint *isaEndpoint = nullptr;
        foreach (ZigbeeNodeEndpoint *endpoint, node->endpoints()) {
            if (endpoint->profile() == Zigbee::ZigbeeProfileHomeAutomation && endpoint->deviceId() == Zigbee::HomeAutomationDeviceIsaZone) {
                isaEndpoint = endpoint;
                break;
            }
        }

        if (!isaEndpoint) {
            qCWarning(dcZigBeeGewiss()) << "ISA zone endpoint could not be found for" << thing;
        } else {
            // Set the current zone status
            ZigbeeClusterIasZone *iasZoneCluster = isaEndpoint->inputCluster<ZigbeeClusterIasZone>(ZigbeeClusterLibrary::ClusterIdIasZone);
            if (!iasZoneCluster) {
                qCWarning(dcZigBeeGewiss()) << "ISA zone cluster could not be found on" << thing << node << isaEndpoint;
            } else {
                if (iasZoneCluster->hasAttribute(ZigbeeClusterIasZone::AttributeZoneStatus)) {
                    thing->setStateValue(gwa1513WindowSensorClosedStateTypeId, !iasZoneCluster->zoneStatus().testFlag(ZigbeeClusterIasZone::ZoneStatusAlarm1));
                }

                connect(iasZoneCluster, &ZigbeeClusterIasZone::zoneStatusChanged, this, [=](ZigbeeClusterIasZone::ZoneStatusFlags zoneStatus, quint8 extendedStatus, quint8 zoneId, quint16 delay){
                    qCDebug(dcZigBeeGewiss()) << thing << "zone status changed" << zoneStatus << extendedStatus << zoneId << delay;
                    thing->setStateValue(gwa1513WindowSensorClosedStateTypeId, !iasZoneCluster->zoneStatus().testFlag(ZigbeeClusterIasZone::ZoneStatusAlarm1));
                    // TODO: casing opened alarm event
                    // TODO: battery critical could be set with the battery flag
                });
            }

            ZigbeeClusterPowerConfiguration *powerCluster = isaEndpoint->inputCluster<ZigbeeClusterPowerConfiguration>(ZigbeeClusterLibrary::ClusterIdPowerConfiguration);
            if (!powerCluster) {
                qCWarning(dcZigBeeGewiss()) << "Power configuration cluster could not be found on" << thing << node << isaEndpoint;
            } else {
                // Only set the initial state if the attribute already exists
                if (powerCluster->hasAttribute(ZigbeeClusterPowerConfiguration::AttributeBatteryPercentageRemaining)) {
                    thing->setStateValue(gwa1513WindowSensorBatteryLevelStateTypeId, powerCluster->batteryPercentage());
                    thing->setStateValue(gwa1513WindowSensorBatteryCriticalStateTypeId, (powerCluster->batteryPercentage() < 10.0));
                }

                connect(powerCluster, &ZigbeeClusterPowerConfiguration::batteryPercentageChanged, thing, [=](double percentage){
                    qCDebug(dcZigBeeGewiss()) << "Battery percentage changed" << percentage << "%" << thing;
                    thing->setStateValue(gwa1513WindowSensorBatteryLevelStateTypeId, percentage);
                    thing->setStateValue(gwa1513WindowSensorBatteryCriticalStateTypeId, (percentage < 10.0));
                });
            }
        }


        ZigbeeNodeEndpoint *temperatureSensorEndpoint = nullptr;
        foreach (ZigbeeNodeEndpoint *endpoint, node->endpoints()) {
            if (endpoint->profile() == Zigbee::ZigbeeProfileHomeAutomation && endpoint->deviceId() == Zigbee::HomeAutomationDeviceTemperatureSensor) {
                temperatureSensorEndpoint = endpoint;
                break;
            }
        }

        if (!temperatureSensorEndpoint) {
            qCWarning(dcZigBeeGewiss()) << "Temperature sensor endpoint could not be found for" << thing;
        } else {
            ZigbeeClusterTemperatureMeasurement *temperatureCluster = temperatureSensorEndpoint->inputCluster<ZigbeeClusterTemperatureMeasurement>(ZigbeeClusterLibrary::ClusterIdTemperatureMeasurement);
            if (!temperatureCluster) {
                qCWarning(dcZigBeeGewiss()) << "Could not find the temperature measurement server cluster on" << thing << temperatureSensorEndpoint;
            } else {
                // Only set the state if the cluster actually has the attribute
                if (temperatureCluster->hasAttribute(ZigbeeClusterTemperatureMeasurement::AttributeMeasuredValue)) {
                    thing->setStateValue(gwa1513WindowSensorTemperatureStateTypeId, temperatureCluster->temperature());
                }

                connect(temperatureCluster, &ZigbeeClusterTemperatureMeasurement::temperatureChanged, thing, [thing](double temperature){
                    qCDebug(dcZigBeeGewiss()) << thing << "temperature changed" << temperature << "°C";
                    thing->setStateValue(gwa1513WindowSensorTemperatureStateTypeId, temperature);
                });
            }
        }

        return info->finish(Thing::ThingErrorNoError);
        //Flood sensor
    } else if (thing->thingClassId() == gwa1514FloodSensorThingClassId) {
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
                thing->setStateValue(gwa1514FloodSensorWaterDetectedStateTypeId, onOffCluster->power());
            }

            connect(onOffCluster, &ZigbeeClusterOnOff::powerChanged, thing, [thing](bool power){
                qCDebug(dcZigBeeGewiss()) << thing << "power changed" << power;
                thing->setStateValue(gwa1514FloodSensorWaterDetectedStateTypeId, power);
            });
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

    if (thing->thingClassId() == gwa1521ActuatorThingClassId) {
        if (action.actionTypeId() == gwa1521ActuatorRelayActionTypeId) {
            ZigbeeNode *node = m_thingNodes.value(thing);
            if (!node) {
                qCWarning(dcZigBeeGewiss()) << "Zigbee node for" << thing << "not found.";
                info->finish(Thing::ThingErrorHardwareFailure);
                return;
            }

            if (info->action().actionTypeId() == gwa1521ActuatorRelayActionTypeId) {
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
                bool power = info->action().param(gwa1521ActuatorRelayActionRelayParamTypeId).value().toBool();
                ZigbeeClusterReply *reply = (power ? onOffCluster->commandOn() : onOffCluster->commandOff());
                connect(reply, &ZigbeeClusterReply::finished, this, [=](){
                    // Note: reply will be deleted automatically
                    if (reply->error() != ZigbeeClusterReply::ErrorNoError) {
                        info->finish(Thing::ThingErrorHardwareFailure);
                    } else {
                        info->finish(Thing::ThingErrorNoError);
                        thing->setStateValue(gwa1521ActuatorRelayStateTypeId, power);
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

bool IntegrationPluginZigbeeGewiss::initTemperatureCluster(ZigbeeNode *node, Thing *thing)
{
    qCDebug(dcZigBeeGewiss()) << "Initializing temperature cluster" << node->modelName();
    ZigbeeNodeEndpoint *temperatureEndpoint;

    Q_FOREACH(ZigbeeNodeEndpoint *endpoint, node->endpoints()) {
        if (endpoint->hasInputCluster(ZigbeeClusterLibrary::ClusterIdTemperatureMeasurement)) {
            temperatureEndpoint = endpoint;
            break;
        }
    }
    if (!temperatureEndpoint) {
        qCWarning(dcZigBeeGewiss()) << "No temperature endpoint found";
        return false;
    }

    if (!temperatureEndpoint) {
        qCWarning(dcZigBeeGewiss()) << "Temperature endpoint not found";
        return false;
    }

    ZigbeeClusterTemperatureMeasurement *temperatureCluster = temperatureEndpoint->inputCluster<ZigbeeClusterTemperatureMeasurement>(ZigbeeClusterLibrary::ClusterIdTemperatureMeasurement);
    if (!temperatureCluster) {
        qCWarning(dcZigBeeGewiss()) << "Temperature cluster not found";
        return false;
    }

    if (temperatureCluster->hasAttribute(ZigbeeClusterTemperatureMeasurement::AttributeMeasuredValue)) {
        thing->setStateValue(m_temperatureStateTypeIds.value(thing->thingClassId()), temperatureCluster->temperature());
    }
    connect(temperatureCluster, &ZigbeeClusterTemperatureMeasurement::temperatureChanged, thing, [this, thing](double temperature){
        qCDebug(dcZigBeeGewiss()) << thing << "temperature changed" << temperature << "°C";
        thing->setStateValue(m_temperatureStateTypeIds.value(thing->thingClassId()), temperature);
    });
    return true;
}

bool IntegrationPluginZigbeeGewiss::initPowerConfigurationCluster(ZigbeeNode *node, Thing *thing)
{
    qCDebug(dcZigBeeGewiss()) << "Initializing power configuration cluster" << node->modelName();
    ZigbeeNodeEndpoint *powerConfigurationEndpoint;

    Q_FOREACH(ZigbeeNodeEndpoint *endpoint, node->endpoints()) {
        if (endpoint->hasInputCluster(ZigbeeClusterLibrary::ClusterIdPowerConfiguration)) {
            powerConfigurationEndpoint = endpoint;
            break;
        }
    }
    if (!powerConfigurationEndpoint) {
        qCWarning(dcZigBeeGewiss()) << "No power configuration endpoint found";
        return false;
    }
    // Get battery level changes
    ZigbeeClusterPowerConfiguration *powerCluster = powerConfigurationEndpoint->inputCluster<ZigbeeClusterPowerConfiguration>(ZigbeeClusterLibrary::ClusterIdPowerConfiguration);
    if (!powerCluster) {
        qCWarning(dcZigBeeGewiss()) << "Could not find power configuration cluster on" << thing << powerConfigurationEndpoint;
    } else {
        // Only set the initial state if the attribute already exists
        if (powerCluster->hasAttribute(ZigbeeClusterPowerConfiguration::AttributeBatteryPercentageRemaining)) {
            thing->setStateValue(m_batteryLevelStateTypeIds.value(thing->thingClassId()), powerCluster->batteryPercentage());
            thing->setStateValue(m_batteryCriticalStateTypeIds.value(thing->thingClassId()), (powerCluster->batteryPercentage() < 10.0));
        }

        connect(powerCluster, &ZigbeeClusterPowerConfiguration::batteryPercentageChanged, thing, [=](double percentage){
            qCDebug(dcZigBeeGewiss()) << "Battery percentage changed" << percentage << "%" << thing;
            thing->setStateValue(m_batteryLevelStateTypeIds.value(thing->thingClassId()), percentage);
            thing->setStateValue(m_batteryCriticalStateTypeIds.value(thing->thingClassId()), (percentage < 10.0));
        });
    }
    return true;
}

bool IntegrationPluginZigbeeGewiss::initBinaryInput(ZigbeeNode *node)
{
    ZigbeeNodeEndpoint *endpoint1 = node->getEndpoint(0x01);
    ZigbeeNodeEndpoint *endpoint2 = node->getEndpoint(0x02);

    if (!endpoint1 || !endpoint2) {
        qCWarning(dcZigBeeGewiss()) << "Could not find endpoint on node" << node;
        return false;
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
        if (!endpoint1->hasInputCluster(ZigbeeClusterLibrary::ClusterIdPowerConfiguration)) {
            qCWarning(dcZigBeeGewiss()) << "Failed to initialize the power configuration cluster because the cluster could not be found" << node << endpoint1;
            return;
        }

        qCDebug(dcZigBeeGewiss()) << "Read power configuration cluster attributes" << node;
        ZigbeeClusterReply *readAttributeReply = endpoint1->getInputCluster(ZigbeeClusterLibrary::ClusterIdPowerConfiguration)->readAttributes({ZigbeeClusterPowerConfiguration::AttributeBatteryPercentageRemaining});
        connect(readAttributeReply, &ZigbeeClusterReply::finished, node, [=](){
            if (readAttributeReply->error() != ZigbeeClusterReply::ErrorNoError) {
                qCWarning(dcZigBeeGewiss()) << "Failed to read power cluster attributes" << readAttributeReply->error();
            } else {
                qCDebug(dcZigBeeGewiss()) << "Read power configuration cluster attributes finished successfully";
            }


            // Bind the cluster to the coordinator
            qCDebug(dcZigBeeGewiss()) << "Bind power configuration cluster to coordinator IEEE address";
            ZigbeeDeviceObjectReply * zdoReply = node->deviceObject()->requestBindIeeeAddress(endpoint1->endpointId(), ZigbeeClusterLibrary::ClusterIdPowerConfiguration,
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
                ZigbeeClusterReply *reportingReply = endpoint1->getInputCluster(ZigbeeClusterLibrary::ClusterIdPowerConfiguration)->configureReporting({reportingConfig});
                connect(reportingReply, &ZigbeeClusterReply::finished, this, [=](){
                    if (reportingReply->error() != ZigbeeClusterReply::ErrorNoError) {
                        qCWarning(dcZigBeeGewiss()) << "Failed to configure power cluster attribute reporting" << reportingReply->error();
                    } else {
                        qCDebug(dcZigBeeGewiss()) << "Attribute reporting configuration finished for power cluster" << ZigbeeClusterLibrary::parseAttributeReportingStatusRecords(reportingReply->responseFrame().payload);
                    }


                    qCDebug(dcZigBeeGewiss()) << "Bind  binary 1 cluster to coordinator";
                    ZigbeeDeviceObjectReply * zdoReply = node->deviceObject()->requestBindGroupAddress(endpoint1->endpointId(), ZigbeeClusterLibrary::ClusterIdBinaryInput, 0x0000);
                    connect(zdoReply, &ZigbeeDeviceObjectReply::finished, node, [=](){
                        if (zdoReply->error() != ZigbeeDeviceObjectReply::ErrorNoError) {
                            qCWarning(dcZigBeeGewiss()) << "Failed to bind bianry cluster to coordinator" << zdoReply->error();
                        } else {
                            qCDebug(dcZigBeeGewiss()) << "Bind binary cluster to coordinator finished successfully";
                        }

                        qCDebug(dcZigBeeGewiss()) << "Bind binary 2 cluster to coordinator";
                        ZigbeeDeviceObjectReply * zdoReply = node->deviceObject()->requestBindGroupAddress(endpoint2->endpointId(), ZigbeeClusterLibrary::ClusterIdBinaryInput, 0x0000);
                        connect(zdoReply, &ZigbeeDeviceObjectReply::finished, node, [=](){
                            if (zdoReply->error() != ZigbeeDeviceObjectReply::ErrorNoError) {
                                qCWarning(dcZigBeeGewiss()) << "Failed to bind binary cluster 2 to coordinator" << zdoReply->error();
                            } else {
                                qCDebug(dcZigBeeGewiss()) << "Bind binary cluster 2 to coordinator finished successfully";
                            }

                            qCDebug(dcZigBeeGewiss()) << "Bind power level cluster to coordinator";
                            ZigbeeDeviceObjectReply * zdoReply = node->deviceObject()->requestBindGroupAddress(endpoint1->endpointId(), ZigbeeClusterLibrary::ClusterIdLevelControl, 0x0000);
                            connect(zdoReply, &ZigbeeDeviceObjectReply::finished, node, [=](){
                                if (zdoReply->error() != ZigbeeDeviceObjectReply::ErrorNoError) {
                                    qCWarning(dcZigBeeGewiss()) << "Failed to bind level cluster to coordinator" << zdoReply->error();
                                } else {
                                    qCDebug(dcZigBeeGewiss()) << "Bind level cluster to coordinator finished successfully";
                                }

                                qCDebug(dcZigBeeGewiss()) << "Read final binding table from node" << node;
                                ZigbeeReply *reply = node->readBindingTableEntries();
                                connect(reply, &ZigbeeReply::finished, node, [=](){
                                    if (reply->error() != ZigbeeReply::ErrorNoError) {
                                        qCWarning(dcZigBeeGewiss()) << "Failed to read binding table from" << node;
                                    } else {
                                        foreach (const ZigbeeDeviceProfile::BindingTableListRecord &binding, node->bindingTableRecords()) {
                                            qCDebug(dcZigBeeGewiss()) << node << binding;
                                        }
                                    }
                                });
                            });
                        });
                    });
                });
            });
        });
    });
    return true;
}

bool IntegrationPluginZigbeeGewiss::initWindowSensor(ZigbeeNode *node)
{
    ZigbeeNodeEndpoint *isaEndpoint = nullptr;
    foreach (ZigbeeNodeEndpoint *endpoint, node->endpoints()) {
        if (endpoint->profile() == Zigbee::ZigbeeProfileHomeAutomation && endpoint->deviceId() == Zigbee::HomeAutomationDeviceIsaZone) {
            isaEndpoint = endpoint;
            break;
        }
    }

    if (!isaEndpoint) {
        qCWarning(dcZigBeeGewiss()) << "Could not find ISA zone endpoint on node" << node;
        return false;
    }

    if (!isaEndpoint->hasInputCluster(ZigbeeClusterLibrary::ClusterIdPowerConfiguration)) {
        qCWarning(dcZigBeeGewiss()) << "Failed to initialize the power configuration cluster because the cluster could not be found" << node << isaEndpoint;
        return false;
    }

    if (!isaEndpoint->hasInputCluster(ZigbeeClusterLibrary::ClusterIdIasZone)) {
        qCWarning(dcZigBeeGewiss()) << "Failed to initialize the IAS zone cluster because the cluster could not be found" << node << isaEndpoint;
        return false;
    }

    ZigbeeNodeEndpoint *temperatureSensorEndpoint = nullptr;
    foreach (ZigbeeNodeEndpoint *endpoint, node->endpoints()) {
        if (endpoint->profile() == Zigbee::ZigbeeProfileHomeAutomation && endpoint->deviceId() == Zigbee::HomeAutomationDeviceTemperatureSensor) {
            temperatureSensorEndpoint = endpoint;
            break;
        }
    }

    if (!temperatureSensorEndpoint) {
        qCWarning(dcZigBeeGewiss()) << "Could not find temperature sensor endpoint on node" << node;
        return false;
    }

    if (!temperatureSensorEndpoint->hasInputCluster(ZigbeeClusterLibrary::ClusterIdTemperatureMeasurement)) {
        qCWarning(dcZigBeeGewiss()) << "Failed to initialize the temperature cluster because the cluster could not be found" << node << temperatureSensorEndpoint;
        return false;
    }


    qCDebug(dcZigBeeGewiss()) << "Initialize window sensor on endpoint" << isaEndpoint;
    // Get the current configured bindings for this node
    ZigbeeReply *reply = node->removeAllBindings();
    connect(reply, &ZigbeeReply::finished, node, [=](){
        if (reply->error() != ZigbeeReply::ErrorNoError) {
            qCWarning(dcZigBeeGewiss()) << "Failed to remove all bindings for initialization of" << node;
        } else {
            qCDebug(dcZigBeeGewiss()) << "Removed all bindings successfully from" << node;
        }

        // Read battery, bind and configure attribute reporting for battery
        qCDebug(dcZigBeeGewiss()) << "Read power configuration cluster attributes" << node;
        ZigbeeClusterReply *readAttributeReply = isaEndpoint->getInputCluster(ZigbeeClusterLibrary::ClusterIdPowerConfiguration)->readAttributes({ZigbeeClusterPowerConfiguration::AttributeBatteryPercentageRemaining});
        connect(readAttributeReply, &ZigbeeClusterReply::finished, node, [=](){
            if (readAttributeReply->error() != ZigbeeClusterReply::ErrorNoError) {
                qCWarning(dcZigBeeGewiss()) << "Failed to read power cluster attributes" << readAttributeReply->error();
            } else {
                qCDebug(dcZigBeeGewiss()) << "Read power configuration cluster attributes finished successfully";
            }

            // Bind the cluster to the coordinator
            qCDebug(dcZigBeeGewiss()) << "Bind power configuration cluster to coordinator IEEE address";
            ZigbeeDeviceObjectReply * zdoReply = node->deviceObject()->requestBindIeeeAddress(isaEndpoint->endpointId(), ZigbeeClusterLibrary::ClusterIdPowerConfiguration,
                                                                                              hardwareManager()->zigbeeResource()->coordinatorAddress(node->networkUuid()), 0x01);
            connect(zdoReply, &ZigbeeDeviceObjectReply::finished, node, [=](){
                if (zdoReply->error() != ZigbeeDeviceObjectReply::ErrorNoError) {
                    qCWarning(dcZigBeeGewiss()) << "Failed to bind power cluster to coordinator" << zdoReply->error();
                } else {
                    qCDebug(dcZigBeeGewiss()) << "Bind power configuration cluster to coordinator finished successfully";
                }

                // Configure attribute reporting for battery remaining (0.5 % changes = 1)
                ZigbeeClusterLibrary::AttributeReportingConfiguration reportingConfig;
                reportingConfig.attributeId = ZigbeeClusterPowerConfiguration::AttributeBatteryPercentageRemaining;
                reportingConfig.dataType = Zigbee::Uint8;
                reportingConfig.minReportingInterval = 300;
                reportingConfig.maxReportingInterval = 2700;
                reportingConfig.reportableChange = ZigbeeDataType(static_cast<quint8>(1)).data();

                qCDebug(dcZigBeeGewiss()) << "Configure attribute reporting for power configuration cluster to coordinator";
                ZigbeeClusterReply *reportingReply = isaEndpoint->getInputCluster(ZigbeeClusterLibrary::ClusterIdPowerConfiguration)->configureReporting({reportingConfig});
                connect(reportingReply, &ZigbeeClusterReply::finished, this, [=] {
                    if (reportingReply->error() != ZigbeeClusterReply::ErrorNoError) {
                        qCWarning(dcZigBeeGewiss()) << "Failed to configure power cluster attribute reporting" << reportingReply->error();
                    } else {
                        qCDebug(dcZigBeeGewiss()) << "Attribute reporting configuration finished for power cluster" << ZigbeeClusterLibrary::parseAttributeReportingStatusRecords(reportingReply->responseFrame().payload);
                    }



                    // Init the ISA zone cluster
                    ZigbeeClusterReply *readAttributeReply = isaEndpoint->getInputCluster(ZigbeeClusterLibrary::ClusterIdIasZone)->readAttributes({ZigbeeClusterIasZone::AttributeZoneType, ZigbeeClusterIasZone::AttributeZoneState, ZigbeeClusterIasZone::AttributeZoneStatus});
                    connect(readAttributeReply, &ZigbeeClusterReply::finished, node, [=](){
                        if (readAttributeReply->error() != ZigbeeClusterReply::ErrorNoError) {
                            qCWarning(dcZigBeeGewiss()) << "Failed to read ISA zone attributes" << readAttributeReply->error();
                        } else {
                            qCDebug(dcZigBeeGewiss()) << "Read ISA zone cluster attributes finished successfully.";
                        }

                        // Bind the cluster to the coordinator
                        qCDebug(dcZigBeeGewiss()) << "Bind ISA zone cluster to coordinator IEEE address";
                        ZigbeeDeviceObjectReply * zdoReply = node->deviceObject()->requestBindIeeeAddress(isaEndpoint->endpointId(), ZigbeeClusterLibrary::ClusterIdIasZone,
                                                                                                          hardwareManager()->zigbeeResource()->coordinatorAddress(node->networkUuid()), 0x01);
                        connect(zdoReply, &ZigbeeDeviceObjectReply::finished, node, [=](){
                            if (zdoReply->error() != ZigbeeDeviceObjectReply::ErrorNoError) {
                                qCWarning(dcZigBeeGewiss()) << "Failed to bind ISA zone cluster to coordinator" << zdoReply->error();
                            } else {
                                qCDebug(dcZigBeeGewiss()) << "Bind ISA zone cluster to coordinator finished successfully";
                            }


                            qCDebug(dcZigBeeGewiss()) << "Initialize temperature sensor on endpoint" << temperatureSensorEndpoint;
                            qCDebug(dcZigBeeGewiss()) << "Bind temperature cluster to coordinator IEEE address";
                            ZigbeeDeviceObjectReply * zdoReply = node->deviceObject()->requestBindIeeeAddress(temperatureSensorEndpoint->endpointId(), ZigbeeClusterLibrary::ClusterIdTemperatureMeasurement,
                                                                                                              hardwareManager()->zigbeeResource()->coordinatorAddress(node->networkUuid()), 0x01);
                            connect(zdoReply, &ZigbeeDeviceObjectReply::finished, node, [=](){
                                if (zdoReply->error() != ZigbeeDeviceObjectReply::ErrorNoError) {
                                    qCWarning(dcZigBeeGewiss()) << "Failed to bind temperature cluster to coordinator" << zdoReply->error();
                                } else {
                                    qCDebug(dcZigBeeGewiss()) << "Bind temperature cluster to coordinator finished successfully";
                                }

                                // Read current temperature
                                qCDebug(dcZigBeeGewiss()) << "Read temperature cluster attributes" << node;
                                ZigbeeClusterReply *readAttributeReply = temperatureSensorEndpoint->getInputCluster(ZigbeeClusterLibrary::ClusterIdTemperatureMeasurement)->readAttributes({ZigbeeClusterTemperatureMeasurement::AttributeMeasuredValue});
                                connect(readAttributeReply, &ZigbeeClusterReply::finished, node, [=](){
                                    if (readAttributeReply->error() != ZigbeeClusterReply::ErrorNoError) {
                                        qCWarning(dcZigBeeGewiss()) << "Failed to read temperature cluster attributes" << readAttributeReply->error();
                                    } else {
                                        qCDebug(dcZigBeeGewiss()) << "Read temperature cluster attributes finished successfully";
                                    }

                                    // Configure attribute reporting for temperature
                                    ZigbeeClusterLibrary::AttributeReportingConfiguration reportingConfig;
                                    reportingConfig.attributeId = ZigbeeClusterTemperatureMeasurement::AttributeMeasuredValue;
                                    reportingConfig.dataType = Zigbee::Int16;
                                    reportingConfig.minReportingInterval = 300;
                                    reportingConfig.maxReportingInterval = 600;
                                    reportingConfig.reportableChange = ZigbeeDataType(static_cast<qint16>(10)).data();

                                    qCDebug(dcZigBeeGewiss()) << "Configure attribute reporting for temperature cluster to coordinator";
                                    ZigbeeClusterReply *reportingReply = temperatureSensorEndpoint->getInputCluster(ZigbeeClusterLibrary::ClusterIdTemperatureMeasurement)->configureReporting({reportingConfig});
                                    connect(reportingReply, &ZigbeeClusterReply::finished, this, [=](){
                                        if (reportingReply->error() != ZigbeeClusterReply::ErrorNoError) {
                                            qCWarning(dcZigBeeGewiss()) << "Failed to configure temperature cluster attribute reporting" << reportingReply->error();
                                        } else {
                                            qCDebug(dcZigBeeGewiss()) << "Attribute reporting configuration finished for temperature cluster" << ZigbeeClusterLibrary::parseAttributeReportingStatusRecords(reportingReply->responseFrame().payload);
                                        }
                                    });
                                });
                            });
                        });
                    });
                });
            });
        });
    });

    return true;
}
