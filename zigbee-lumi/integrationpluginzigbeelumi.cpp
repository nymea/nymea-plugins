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

#include "integrationpluginzigbeelumi.h"
#include "plugininfo.h"
#include "hardware/zigbee/zigbeehardwareresource.h"

#include <zigbeenodeendpoint.h>

#include <QDebug>

IntegrationPluginZigbeeLumi::IntegrationPluginZigbeeLumi()
{
    m_networkUuidParamTypeIds[lumiHTSensorThingClassId] = lumiHTSensorThingNetworkUuidParamTypeId;
    m_networkUuidParamTypeIds[lumiButtonSensorThingClassId] = lumiButtonSensorThingNetworkUuidParamTypeId;
    m_networkUuidParamTypeIds[lumiMagnetSensorThingClassId] = lumiMagnetSensorThingNetworkUuidParamTypeId;
    m_networkUuidParamTypeIds[lumiMotionSensorThingClassId] = lumiMotionSensorThingNetworkUuidParamTypeId;
    m_networkUuidParamTypeIds[lumiWaterSensorThingClassId] = lumiWaterSensorThingNetworkUuidParamTypeId;
    m_networkUuidParamTypeIds[lumiWeatherSensorThingClassId] = lumiWeatherSensorThingNetworkUuidParamTypeId;
    m_networkUuidParamTypeIds[lumiVibrationSensorThingClassId] = lumiVibrationSensorThingNetworkUuidParamTypeId;
    m_networkUuidParamTypeIds[lumiPowerSocketThingClassId] = lumiPowerSocketThingNetworkUuidParamTypeId;
    m_networkUuidParamTypeIds[lumiRelayThingClassId] = lumiRelayThingNetworkUuidParamTypeId;
    m_networkUuidParamTypeIds[lumiRemoteThingClassId] = lumiRemoteThingNetworkUuidParamTypeId;

    m_zigbeeAddressParamTypeIds[lumiHTSensorThingClassId] = lumiHTSensorThingIeeeAddressParamTypeId;
    m_zigbeeAddressParamTypeIds[lumiButtonSensorThingClassId] = lumiButtonSensorThingIeeeAddressParamTypeId;
    m_zigbeeAddressParamTypeIds[lumiMagnetSensorThingClassId] = lumiMagnetSensorThingIeeeAddressParamTypeId;
    m_zigbeeAddressParamTypeIds[lumiMotionSensorThingClassId] = lumiMotionSensorThingIeeeAddressParamTypeId;
    m_zigbeeAddressParamTypeIds[lumiWaterSensorThingClassId] = lumiWaterSensorThingIeeeAddressParamTypeId;
    m_zigbeeAddressParamTypeIds[lumiWeatherSensorThingClassId] = lumiWeatherSensorThingIeeeAddressParamTypeId;
    m_zigbeeAddressParamTypeIds[lumiVibrationSensorThingClassId] = lumiVibrationSensorThingIeeeAddressParamTypeId;
    m_zigbeeAddressParamTypeIds[lumiPowerSocketThingClassId] = lumiPowerSocketThingIeeeAddressParamTypeId;
    m_zigbeeAddressParamTypeIds[lumiRelayThingClassId] = lumiRelayThingIeeeAddressParamTypeId;
    m_zigbeeAddressParamTypeIds[lumiRemoteThingClassId] = lumiRemoteThingIeeeAddressParamTypeId;

    m_connectedStateTypeIds[lumiHTSensorThingClassId] = lumiHTSensorConnectedStateTypeId;
    m_connectedStateTypeIds[lumiButtonSensorThingClassId] = lumiButtonSensorConnectedStateTypeId;
    m_connectedStateTypeIds[lumiMagnetSensorThingClassId] = lumiMagnetSensorConnectedStateTypeId;
    m_connectedStateTypeIds[lumiMotionSensorThingClassId] = lumiMotionSensorConnectedStateTypeId;
    m_connectedStateTypeIds[lumiWaterSensorThingClassId] = lumiWaterSensorConnectedStateTypeId;
    m_connectedStateTypeIds[lumiWeatherSensorThingClassId] = lumiWeatherSensorConnectedStateTypeId;
    m_connectedStateTypeIds[lumiVibrationSensorThingClassId] = lumiVibrationSensorConnectedStateTypeId;
    m_connectedStateTypeIds[lumiPowerSocketThingClassId] = lumiPowerSocketConnectedStateTypeId;
    m_connectedStateTypeIds[lumiRelayThingClassId] = lumiRelayConnectedStateTypeId;
    m_connectedStateTypeIds[lumiRemoteThingClassId] = lumiRemoteConnectedStateTypeId;

    m_versionStateTypeIds[lumiHTSensorThingClassId] = lumiHTSensorVersionStateTypeId;
    m_versionStateTypeIds[lumiButtonSensorThingClassId] = lumiButtonSensorVersionStateTypeId;
    m_versionStateTypeIds[lumiMagnetSensorThingClassId] = lumiMagnetSensorVersionStateTypeId;
    m_versionStateTypeIds[lumiMotionSensorThingClassId] = lumiMotionSensorVersionStateTypeId;
    m_versionStateTypeIds[lumiWaterSensorThingClassId] = lumiWaterSensorVersionStateTypeId;
    m_versionStateTypeIds[lumiWeatherSensorThingClassId] = lumiWeatherSensorVersionStateTypeId;
    m_versionStateTypeIds[lumiVibrationSensorThingClassId] = lumiVibrationSensorVersionStateTypeId;
    m_versionStateTypeIds[lumiPowerSocketThingClassId] = lumiPowerSocketVersionStateTypeId;
    m_versionStateTypeIds[lumiRelayThingClassId] = lumiRelayVersionStateTypeId;
    m_versionStateTypeIds[lumiRemoteThingClassId] = lumiRemoteVersionStateTypeId;

    m_signalStrengthStateTypeIds[lumiHTSensorThingClassId] = lumiHTSensorSignalStrengthStateTypeId;
    m_signalStrengthStateTypeIds[lumiButtonSensorThingClassId] = lumiButtonSensorSignalStrengthStateTypeId;
    m_signalStrengthStateTypeIds[lumiMagnetSensorThingClassId] = lumiMagnetSensorSignalStrengthStateTypeId;
    m_signalStrengthStateTypeIds[lumiMotionSensorThingClassId] = lumiMotionSensorSignalStrengthStateTypeId;
    m_signalStrengthStateTypeIds[lumiWaterSensorThingClassId] = lumiWaterSensorSignalStrengthStateTypeId;
    m_signalStrengthStateTypeIds[lumiWeatherSensorThingClassId] = lumiWeatherSensorSignalStrengthStateTypeId;
    m_signalStrengthStateTypeIds[lumiVibrationSensorThingClassId] = lumiVibrationSensorSignalStrengthStateTypeId;
    m_signalStrengthStateTypeIds[lumiPowerSocketThingClassId] = lumiPowerSocketSignalStrengthStateTypeId;
    m_signalStrengthStateTypeIds[lumiRelayThingClassId] = lumiRelaySignalStrengthStateTypeId;
    m_signalStrengthStateTypeIds[lumiRemoteThingClassId] = lumiRemoteSignalStrengthStateTypeId;

    // Known model identifier
    m_knownLumiDevices.insert("lumi.sensor_ht", lumiHTSensorThingClassId);
    m_knownLumiDevices.insert("lumi.sensor_magnet", lumiMagnetSensorThingClassId);
    m_knownLumiDevices.insert("lumi.sensor_switch", lumiButtonSensorThingClassId);
    m_knownLumiDevices.insert("lumi.sensor_motion", lumiMotionSensorThingClassId);
    m_knownLumiDevices.insert("lumi.sensor_wleak", lumiWaterSensorThingClassId);
    m_knownLumiDevices.insert("lumi.weather", lumiWeatherSensorThingClassId);
    m_knownLumiDevices.insert("lumi.vibration", lumiVibrationSensorThingClassId);
    m_knownLumiDevices.insert("lumi.plug", lumiPowerSocketThingClassId);
    m_knownLumiDevices.insert("lumi.relay", lumiRelayThingClassId);
    m_knownLumiDevices.insert("lumi.remote", lumiRemoteThingClassId);
}

QString IntegrationPluginZigbeeLumi::name() const
{
    return "Lumi";
}

bool IntegrationPluginZigbeeLumi::handleNode(ZigbeeNode *node, const QUuid &networkUuid)
{
    // Check if this is Lumi
    // Note: Lumi / Xiaomi / Aquara devices are not in the specs, some older models do not
    // send the node descriptor or use a inconsistent manufacturer code. We use the manufacturer
    // name for matching since that has shown to be most constant
    foreach (ZigbeeNodeEndpoint *endpoint, node->endpoints()) {
        // Get the model identifier if present from the first endpoint. Also this is out of spec
        if (!endpoint->hasInputCluster(ZigbeeClusterLibrary::ClusterIdBasic)) {
            qCDebug(dcZigbeeLumi()) << "This lumi endpoint does not have the basic input cluster, so we skipp it" << endpoint;
            continue;
        }

        // Basic cluster exists, so we should have the manufacturer name
        if (!endpoint->manufacturerName().toLower().startsWith("lumi")) {
            return false;
        }

        ThingClassId thingClassId;
        foreach (const QString &knownLumi, m_knownLumiDevices.keys()) {
            if (endpoint->modelIdentifier().startsWith(knownLumi)) {
                thingClassId = m_knownLumiDevices.value(knownLumi);
                break;
            }
        }
        if (thingClassId.isNull()) {
            qCWarning(dcZigbeeLumi()) << "Unhandled Lumi device:" << endpoint->modelIdentifier();
            return false;
        }

        ThingDescriptor descriptor(thingClassId, supportedThings().findById(thingClassId).displayName());
        ParamList params;
        params << Param(m_networkUuidParamTypeIds.value(thingClassId), networkUuid.toString());
        params << Param(m_zigbeeAddressParamTypeIds.value(thingClassId), node->extendedAddress().toString());
        descriptor.setParams(params);
        emit autoThingsAppeared({descriptor});
        return true;
    }

    return false;
}

void IntegrationPluginZigbeeLumi::handleRemoveNode(ZigbeeNode *node, const QUuid &networkUuid)
{
    Q_UNUSED(networkUuid)

    if (m_thingNodes.values().contains(node)) {
        Thing *thing = m_thingNodes.key(node);
        qCDebug(dcZigbeeLumi()) << node << "for" << thing << "has left the network.";
        m_thingNodes.remove(thing);
        emit autoThingDisappeared(thing->id());
    }
}

void IntegrationPluginZigbeeLumi::init()
{
    hardwareManager()->zigbeeResource()->registerHandler(this);
}

void IntegrationPluginZigbeeLumi::setupThing(ThingSetupInfo *info)
{
    Thing *thing = info->thing();

    // Get the node for this thing
    QUuid networkUuid = thing->paramValue(m_networkUuidParamTypeIds.value(thing->thingClassId())).toUuid();
    ZigbeeAddress zigbeeAddress = ZigbeeAddress(thing->paramValue(m_zigbeeAddressParamTypeIds.value(thing->thingClassId())).toString());
    ZigbeeNode *node = hardwareManager()->zigbeeResource()->claimNode(this, networkUuid, zigbeeAddress);
    if (!node) {
        qCWarning(dcZigbeeLumi()) << "Zigbee node for" << info->thing()->name() << "not found.´";
        info->finish(Thing::ThingErrorHardwareNotAvailable);
        return;
    }

    // Get the endpoint of interest (0x01) for this device
    ZigbeeNodeEndpoint *endpoint = node->getEndpoint(0x01);
    if (!endpoint) {
        qCWarning(dcZigbeeLumi()) << "Zigbee endpoint 1 not found on" << thing->name();
        info->finish(Thing::ThingErrorSetupFailed);
        return;
    }

    // Store the node thing association for removing
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
        qCDebug(dcZigbeeLumi()) << thing << "signal strength changed" << signalStrength << "%";
        thing->setStateValue(m_signalStrengthStateTypeIds.value(thing->thingClassId()), signalStrength);
    });

    // Set the version
    thing->setStateValue(m_versionStateTypeIds.value(thing->thingClassId()), endpoint->softwareBuildId());

    // Thing specific setup
    if (thing->thingClassId() == lumiMagnetSensorThingClassId) {
        ZigbeeClusterOnOff *onOffCluster = endpoint->inputCluster<ZigbeeClusterOnOff>(ZigbeeClusterLibrary::ClusterIdOnOff);
        if (onOffCluster) {
            // Only set the state if the cluster actually has the attribute
            if (onOffCluster->hasAttribute(ZigbeeClusterOnOff::AttributeOnOff)) {
                thing->setStateValue(lumiMagnetSensorClosedStateTypeId, !onOffCluster->power());
            }

            connect(onOffCluster, &ZigbeeClusterOnOff::powerChanged, thing, [thing](bool power){
                qCDebug(dcZigbeeLumi()) << thing << "state changed" << (power ? "closed" : "open");
                thing->setStateValue(lumiMagnetSensorClosedStateTypeId, !power);
            });
        } else {
            qCWarning(dcZigbeeLumi()) << "Could not find the OnOff input cluster on" << thing << endpoint;
        }
    }


    if (thing->thingClassId() == lumiMotionSensorThingClassId) {
        ZigbeeClusterOccupancySensing *occupancyCluster = endpoint->inputCluster<ZigbeeClusterOccupancySensing>(ZigbeeClusterLibrary::ClusterIdOccupancySensing);
        if (occupancyCluster) {
            if (occupancyCluster->hasAttribute(ZigbeeClusterOccupancySensing::AttributeOccupancy)) {
                thing->setStateValue(lumiMotionSensorIsPresentStateTypeId, occupancyCluster->occupied());
                thing->setStateValue(lumiMotionSensorLastSeenTimeStateTypeId, QDateTime::currentMSecsSinceEpoch() / 1000);
            }

            connect(occupancyCluster, &ZigbeeClusterOccupancySensing::occupancyChanged, thing, [this, thing](bool occupancy){
                qCDebug(dcZigbeeLumi()) << "occupancy changed" << occupancy;
                // Only change the state if the it changed to true, it will be disabled by the timer
                if (occupancy) {
                    thing->setStateValue(lumiMotionSensorIsPresentStateTypeId, occupancy);
                    m_presenceTimer->start();
                }

                thing->setStateValue(lumiMotionSensorLastSeenTimeStateTypeId, QDateTime::currentMSecsSinceEpoch() / 1000);
            });

            if (!m_presenceTimer) {
                m_presenceTimer = hardwareManager()->pluginTimerManager()->registerTimer(1);
            }

            connect(m_presenceTimer, &PluginTimer::timeout, thing, [thing](){
                if (thing->stateValue(lumiMotionSensorIsPresentStateTypeId).toBool()) {
                    int timeout = thing->setting(lumiMotionSensorSettingsTimeoutParamTypeId).toInt();
                    QDateTime lastSeenTime = QDateTime::fromMSecsSinceEpoch(thing->stateValue(lumiMotionSensorLastSeenTimeStateTypeId).toULongLong() * 1000);
                    if (lastSeenTime.addSecs(timeout) < QDateTime::currentDateTime()) {
                        thing->setStateValue(lumiMotionSensorIsPresentStateTypeId, false);
                    }
                }
            });
        } else {
            qCWarning(dcZigbeeLumi()) << "Occupancy cluster not found on" << thing->name();
        }

        ZigbeeClusterIlluminanceMeasurment *illuminanceCluster = endpoint->inputCluster<ZigbeeClusterIlluminanceMeasurment>(ZigbeeClusterLibrary::ClusterIdIlluminanceMeasurement);
        if (illuminanceCluster) {
            // Only set the state if the cluster actually has the attribute
            if (illuminanceCluster->hasAttribute(ZigbeeClusterIlluminanceMeasurment::AttributeMeasuredValue)) {
                thing->setStateValue(lumiMotionSensorLightIntensityStateTypeId, illuminanceCluster->illuminance());
            }

            connect(illuminanceCluster, &ZigbeeClusterIlluminanceMeasurment::illuminanceChanged, thing, [thing](quint16 illuminance){
                qCDebug(dcZigbeeLumi()) << thing << "light intensity changed" << illuminance << "lux";
                thing->setStateValue(lumiMotionSensorLightIntensityStateTypeId, illuminance);
            });
        } else {
            qCWarning(dcZigbeeLumi()) << "Illuminance cluster not found on" << thing->name();
        }
    }


    if (thing->thingClassId() == lumiHTSensorThingClassId) {
        ZigbeeClusterTemperatureMeasurement *temperatureCluster = endpoint->inputCluster<ZigbeeClusterTemperatureMeasurement>(ZigbeeClusterLibrary::ClusterIdTemperatureMeasurement);
        if (temperatureCluster) {
            // Only set the state if the cluster actually has the attribute
            if (temperatureCluster->hasAttribute(ZigbeeClusterTemperatureMeasurement::AttributeMeasuredValue)) {
                thing->setStateValue(lumiHTSensorTemperatureStateTypeId, temperatureCluster->temperature());
            }

            connect(temperatureCluster, &ZigbeeClusterTemperatureMeasurement::temperatureChanged, thing, [thing](double temperature){
                qCDebug(dcZigbeeLumi()) << thing << "temperature changed" << temperature << "°C";
                thing->setStateValue(lumiHTSensorTemperatureStateTypeId, temperature);
            });
        } else {
            qCWarning(dcZigbeeLumi()) << "Could not find the temperature measurement server cluster on" << thing << endpoint;
        }

        ZigbeeClusterRelativeHumidityMeasurement *humidityCluster = endpoint->inputCluster<ZigbeeClusterRelativeHumidityMeasurement>(ZigbeeClusterLibrary::ClusterIdRelativeHumidityMeasurement);
        if (humidityCluster) {
            // Only set the state if the cluster actually has the attribute
            if (humidityCluster->hasAttribute(ZigbeeClusterRelativeHumidityMeasurement::AttributeMeasuredValue)) {
                thing->setStateValue(lumiHTSensorHumidityStateTypeId, humidityCluster->humidity());
            }

            connect(humidityCluster, &ZigbeeClusterRelativeHumidityMeasurement::humidityChanged, thing, [thing](double humidity){
                qCDebug(dcZigbeeLumi()) << thing << "humidity changed" << humidity << "%";
                thing->setStateValue(lumiHTSensorHumidityStateTypeId, humidity);
            });
        } else {
            qCWarning(dcZigbeeLumi()) << "Could not find the relative humidity measurement server cluster on" << thing << endpoint;
        }
    }
    

    if (thing->thingClassId() == lumiWeatherSensorThingClassId) {
        ZigbeeClusterTemperatureMeasurement *temperatureCluster = endpoint->inputCluster<ZigbeeClusterTemperatureMeasurement>(ZigbeeClusterLibrary::ClusterIdTemperatureMeasurement);
        if (temperatureCluster) {
            // Only set the state if the cluster actually has the attribute
            if (temperatureCluster->hasAttribute(ZigbeeClusterTemperatureMeasurement::AttributeMeasuredValue)) {
                thing->setStateValue(lumiWeatherSensorTemperatureStateTypeId, temperatureCluster->temperature());
            }

            connect(temperatureCluster, &ZigbeeClusterTemperatureMeasurement::temperatureChanged, thing, [thing](double temperature){
                qCDebug(dcZigbeeLumi()) << thing << "temperature changed" << temperature << "°C";
                thing->setStateValue(lumiWeatherSensorTemperatureStateTypeId, temperature);
            });
        } else {
            qCWarning(dcZigbeeLumi()) << "Could not find the temperature measurement server cluster on" << thing << endpoint;
        }

        ZigbeeClusterRelativeHumidityMeasurement *humidityCluster = endpoint->inputCluster<ZigbeeClusterRelativeHumidityMeasurement>(ZigbeeClusterLibrary::ClusterIdRelativeHumidityMeasurement);
        if (humidityCluster) {
            // Only set the state if the cluster actually has the attribute
            if (humidityCluster->hasAttribute(ZigbeeClusterRelativeHumidityMeasurement::AttributeMeasuredValue)) {
                thing->setStateValue(lumiWeatherSensorHumidityStateTypeId, humidityCluster->humidity());
            }

            connect(humidityCluster, &ZigbeeClusterRelativeHumidityMeasurement::humidityChanged, thing, [thing](double humidity){
                qCDebug(dcZigbeeLumi()) << thing << "humidity changed" << humidity << "%";
                thing->setStateValue(lumiWeatherSensorHumidityStateTypeId, humidity);
            });
        } else {
            qCWarning(dcZigbeeLumi()) << "Could not find the relative humidity measurement server cluster on" << thing << endpoint;
        }

        ZigbeeClusterPressureMeasurement *pressureCluster = endpoint->inputCluster<ZigbeeClusterPressureMeasurement>(ZigbeeClusterLibrary::ClusterIdPressureMeasurement);
        if (pressureCluster) {
            // Only set the state if the cluster actually has the attribute
            if (pressureCluster->hasAttribute(ZigbeeClusterPressureMeasurement::AttributeMeasuredValue)) {
                thing->setStateValue(lumiWeatherSensorPressureStateTypeId, pressureCluster->pressure() * 101);
            }

            connect(pressureCluster, &ZigbeeClusterPressureMeasurement::pressureChanged, thing, [thing](double pressure){
                thing->setStateValue(lumiWeatherSensorPressureStateTypeId, pressure * 101);
            });
        } else {
            qCWarning(dcZigbeeLumi()) << "Could not find the pressure measurement server cluster on" << thing << endpoint;
        }
    }


    if (thing->thingClassId() == lumiWaterSensorThingClassId) {        
        connect(endpoint, &ZigbeeNodeEndpoint::clusterAttributeChanged, this, [thing](ZigbeeCluster *cluster, const ZigbeeClusterAttribute &attribute){
            if (cluster->clusterId() == ZigbeeClusterLibrary::ClusterIdIasZone) {
                if (attribute.id() == ZigbeeClusterIasZone::AttributeZoneState) {
                    bool valueOk = false;
                    ZigbeeClusterIasZone::ZoneStatusFlags zoneStatus = static_cast<ZigbeeClusterIasZone::ZoneStatusFlags>(attribute.dataType().toUInt16(&valueOk));
                    if (!valueOk) {
                        qCWarning(dcZigbeeLumi()) << thing << "failed to convert attribute data to uint16 flag. Not updating the states from" << attribute;
                    } else {
                        qCDebug(dcZigbeeLumi()) << thing << "zone status changed" << zoneStatus;

                        // Water detected gets indicated in the Alarm1 flag
                        if (zoneStatus.testFlag(ZigbeeClusterIasZone::ZoneStatusAlarm1)) {
                            thing->setStateValue(lumiWaterSensorWaterDetectedStateTypeId, true);
                        } else {
                            thing->setStateValue(lumiWaterSensorWaterDetectedStateTypeId, false);
                        }

                        // Battery alarm
                        if (zoneStatus.testFlag(ZigbeeClusterIasZone::ZoneStatusBattery)) {
                            thing->setStateValue(lumiWaterSensorBatteryCriticalStateTypeId, true);
                        } else {
                            thing->setStateValue(lumiWaterSensorBatteryCriticalStateTypeId, false);
                        }
                    }
                }
            }
        });
    }


    if (thing->thingClassId() == lumiVibrationSensorThingClassId) {
        connect(endpoint, &ZigbeeNodeEndpoint::clusterAttributeChanged, this, [this, thing](ZigbeeCluster *cluster, const ZigbeeClusterAttribute &attribute){
            if (cluster->clusterId() == ZigbeeClusterLibrary::ClusterIdDoorLock) {
                // Note: shoehow the vibration sensor is using the door lock cluster, with undocumented attribitues.
                // This device is completly out of spec, so we just recognize the vibration trough tests and it looks like
                // attribute id 85 is the indicator for vibration. The payload contains an unsigned int, but not sure what it indicates yet

                if (attribute.id() == 85) {
                    bool valueOk = false;
                    quint16 value = attribute.dataType().toUInt16(&valueOk);
                    if (!valueOk) {
                        qCWarning(dcZigbeeLumi()) << thing << "failed to convert attribute data to uint16." << attribute;
                    } else {
                        qCDebug(dcZigbeeLumi()) << thing << "vibration attribute changed" << value;
                        emitEvent(Event(lumiVibrationSensorVibrationDetectedEventTypeId, thing->id()));
                    }
                }
            }
        });
    }

    if (thing->thingClassId() == lumiButtonSensorThingClassId) {
        ZigbeeClusterOnOff *onOffCluster = endpoint->inputCluster<ZigbeeClusterOnOff>(ZigbeeClusterLibrary::ClusterIdOnOff);
        if (onOffCluster) {
            connect(onOffCluster, &ZigbeeClusterOnOff::powerChanged, thing, [this, thing](bool power){
                qCDebug(dcZigbeeLumi()) << thing << "state changed" << (power ? "closed" : "open");
                if (!power) {
                    emitEvent(Event(lumiButtonSensorPressedEventTypeId, thing->id()));
                }
            });
        } else {
            qCWarning(dcZigbeeLumi()) << "Could not find the OnOff input cluster on" << thing << endpoint;
        }
    }


    if (thing->thingClassId() == lumiPowerSocketThingClassId) {
        ZigbeeClusterOnOff *onOffCluster = endpoint->inputCluster<ZigbeeClusterOnOff>(ZigbeeClusterLibrary::ClusterIdOnOff);
        if (onOffCluster) {
            if (onOffCluster->hasAttribute(ZigbeeClusterOnOff::AttributeOnOff)) {
                thing->setStateValue(lumiPowerSocketPowerStateTypeId, onOffCluster->power());
            }

            connect(onOffCluster, &ZigbeeClusterOnOff::powerChanged, thing, [thing](bool power){
                qCDebug(dcZigbeeLumi()) << thing << "power changed" << power;
                thing->setStateValue(lumiPowerSocketPowerStateTypeId, power);
            });
        } else {
            qCWarning(dcZigbeeLumi()) << "Could not find the OnOff input cluster on" << thing << endpoint;
        }
    }

    if (thing->thingClassId() == lumiRemoteThingClassId) {
        // Since we are here again out of spec, we just can react on cluster and endpoint signals
        connect(node, &ZigbeeNode::endpointClusterAttributeChanged, thing, [this, thing](ZigbeeNodeEndpoint *endpoint, ZigbeeCluster *cluster, const ZigbeeClusterAttribute &attribute){
            switch (endpoint->endpointId()) {
            case 0x01:
                if (cluster->clusterId() == ZigbeeClusterLibrary::ClusterIdMultistateInput && attribute.id() == ZigbeeClusterMultistateInput::AttributePresentValue) {
                    quint16 value = attribute.dataType().toUInt16();
                    if (value == 1) {
                        emit emitEvent(Event(lumiRemotePressedEventTypeId, thing->id(), ParamList() << Param(lumiRemotePressedEventButtonNameParamTypeId, "1")));
                    } else {
                        emit emitEvent(Event(lumiRemoteLongPressedEventTypeId, thing->id(), ParamList() << Param(lumiRemoteLongPressedEventButtonNameParamTypeId, "1")));
                    }
                }
                break;
            case 0x02:
                if (cluster->clusterId() == ZigbeeClusterLibrary::ClusterIdMultistateInput && attribute.id() == ZigbeeClusterMultistateInput::AttributePresentValue) {
                    quint16 value = attribute.dataType().toUInt16();
                    if (value == 1) {
                        emit emitEvent(Event(lumiRemotePressedEventTypeId, thing->id(), ParamList() << Param(lumiRemotePressedEventButtonNameParamTypeId, "2")));
                    } else {
                        emit emitEvent(Event(lumiRemoteLongPressedEventTypeId, thing->id(), ParamList() << Param(lumiRemoteLongPressedEventButtonNameParamTypeId, "2")));
                    }
                }
                break;
            case 0x03:
                if (cluster->clusterId() == ZigbeeClusterLibrary::ClusterIdMultistateInput && attribute.id() == ZigbeeClusterMultistateInput::AttributePresentValue) {
                    quint16 value = attribute.dataType().toUInt16();
                    if (value == 1) {
                        emit emitEvent(Event(lumiRemotePressedEventTypeId, thing->id(), ParamList() << Param(lumiRemotePressedEventButtonNameParamTypeId, "1+2")));
                    } else {
                        emit emitEvent(Event(lumiRemoteLongPressedEventTypeId, thing->id(), ParamList() << Param(lumiRemoteLongPressedEventButtonNameParamTypeId, "1+2")));
                    }
                }
                break;
            default:
                qCWarning(dcZigbeeLumi()) << "Received attribute changed signal from unhandled endpoint" << thing << endpoint << cluster << attribute;
                break;
            }
        });
    }

    if (thing->thingClassId() == lumiRelayThingClassId) {
        // Get the 2 endpoints
        ZigbeeNodeEndpoint *endpoint1 = node->getEndpoint(0x01);
        if (!endpoint1) {
            ZigbeeClusterOnOff *onOffCluster = endpoint1->inputCluster<ZigbeeClusterOnOff>(ZigbeeClusterLibrary::ClusterIdOnOff);
            if (onOffCluster) {
                if (onOffCluster->hasAttribute(ZigbeeClusterOnOff::AttributeOnOff)) {
                    thing->setStateValue(lumiRelayRelay1StateTypeId, onOffCluster->power());
                }

                connect(onOffCluster, &ZigbeeClusterOnOff::powerChanged, thing, [thing](bool power){
                    qCDebug(dcZigbeeLumi()) << thing << "power changed" << power;
                    thing->setStateValue(lumiRelayRelay1StateTypeId, power);
                });
            } else {
                qCWarning(dcZigbeeLumi()) << "Could not find the OnOff input cluster on" << thing << endpoint1;
            }
        } else {
            qCWarning(dcZigbeeLumi()) << "Could not find endpoint 1 on" << thing << node;
        }

        ZigbeeNodeEndpoint *endpoint2 = node->getEndpoint(0x02);
        if (!endpoint2) {
            ZigbeeClusterOnOff *onOffCluster = endpoint2->inputCluster<ZigbeeClusterOnOff>(ZigbeeClusterLibrary::ClusterIdOnOff);
            if (onOffCluster) {
                if (onOffCluster->hasAttribute(ZigbeeClusterOnOff::AttributeOnOff)) {
                    thing->setStateValue(lumiRelayRelay2StateTypeId, onOffCluster->power());
                }

                connect(onOffCluster, &ZigbeeClusterOnOff::powerChanged, thing, [thing](bool power){
                    qCDebug(dcZigbeeLumi()) << thing << "power changed" << power;
                    thing->setStateValue(lumiRelayRelay2StateTypeId, power);
                });
            } else {
                qCWarning(dcZigbeeLumi()) << "Could not find the OnOff input cluster on" << thing << endpoint1;
            }
        } else {
            qCWarning(dcZigbeeLumi()) << "Could not find endpoint 2 on" << thing << node;
        }
    }

    info->finish(Thing::ThingErrorNoError);
}

void IntegrationPluginZigbeeLumi::executeAction(ThingActionInfo *info)
{
    Thing *thing = info->thing();

    if (thing->thingClassId() == lumiPowerSocketThingClassId) {
        ZigbeeNode *node = m_thingNodes.value(thing);
        if (!node) {
            qCWarning(dcZigbeeLumi()) << "Zigbee node for" << thing << "not found.";
            info->finish(Thing::ThingErrorHardwareFailure);
            return;
        }

        ZigbeeNodeEndpoint *endpoint = node->getEndpoint(0x01);
        if (!endpoint) {
            qCWarning(dcZigbeeLumi()) << "Unable to get the endpoint from node" << node << "for" << thing;
            info->finish(Thing::ThingErrorSetupFailed);
            return;
        }

        ZigbeeClusterOnOff *onOffCluster = endpoint->inputCluster<ZigbeeClusterOnOff>(ZigbeeClusterLibrary::ClusterIdOnOff);
        if (!onOffCluster) {
            qCWarning(dcZigbeeLumi()) << "Could not find on/off cluster for" << thing << "in" << node;
            info->finish(Thing::ThingErrorHardwareFailure);
            return;
        }

        bool power = info->action().param(lumiPowerSocketPowerActionPowerParamTypeId).value().toBool();
        qCDebug(dcZigbeeLumi()) << "Set power for" << info->thing() << "to" << power;
        ZigbeeClusterReply *reply = (power ? onOffCluster->commandOn() : onOffCluster->commandOff());
        connect(reply, &ZigbeeClusterReply::finished, info, [=](){
            // Note: reply will be deleted automatically
            if (reply->error() != ZigbeeClusterReply::ErrorNoError) {
                qCWarning(dcZigbeeLumi()) << "Failed to set power on" << thing << reply->error();
                info->finish(Thing::ThingErrorHardwareFailure);
            } else {
                info->finish(Thing::ThingErrorNoError);
                qCDebug(dcZigbeeLumi()) << "Set power finished successfully for" << thing;
                thing->setStateValue(lumiPowerSocketPowerStateTypeId, power);
            }
        });
        return;
    }

    if (thing->thingClassId() == lumiRelayThingClassId) {
        ZigbeeNode *node = m_thingNodes.value(thing);
        if (!node) {
            qCWarning(dcZigbeeLumi()) << "Zigbee node for" << thing << "not found.";
            info->finish(Thing::ThingErrorHardwareFailure);
            return;
        }

        if (info->action().actionTypeId() == lumiRelayRelay1ActionTypeId) {
            ZigbeeNodeEndpoint *endpoint = node->getEndpoint(0x01);
            if (!endpoint) {
                qCWarning(dcZigbeeLumi()) << "Unable to get the endpoint from node" << node << "for" << thing;
                info->finish(Thing::ThingErrorSetupFailed);
                return;
            }

            ZigbeeClusterOnOff *onOffCluster = endpoint->inputCluster<ZigbeeClusterOnOff>(ZigbeeClusterLibrary::ClusterIdOnOff);
            if (!onOffCluster) {
                qCWarning(dcZigbeeLumi()) << "Unable to get the OnOff cluster from endpoint" << endpoint << "on" << node << "for" << thing;
                info->finish(Thing::ThingErrorSetupFailed);
                return;
            }
            bool power = info->action().param(lumiRelayRelay1ActionRelay1ParamTypeId).value().toBool();
            ZigbeeClusterReply *reply = (power ? onOffCluster->commandOn() : onOffCluster->commandOff());
            connect(reply, &ZigbeeClusterReply::finished, this, [=](){
                // Note: reply will be deleted automatically
                if (reply->error() != ZigbeeClusterReply::ErrorNoError) {
                    info->finish(Thing::ThingErrorHardwareFailure);
                } else {
                    info->finish(Thing::ThingErrorNoError);
                    thing->setStateValue(lumiRelayRelay1StateTypeId, power);
                }
            });
        }

        if (info->action().actionTypeId() == lumiRelayRelay2ActionTypeId) {
            ZigbeeNodeEndpoint *endpoint = node->getEndpoint(0x02);
            if (!endpoint) {
                qCWarning(dcZigbeeLumi()) << "Unable to get the endpoint from node" << node << "for" << thing;
                info->finish(Thing::ThingErrorSetupFailed);
                return;
            }

            ZigbeeClusterOnOff *onOffCluster = endpoint->inputCluster<ZigbeeClusterOnOff>(ZigbeeClusterLibrary::ClusterIdOnOff);
            if (!onOffCluster) {
                qCWarning(dcZigbeeLumi()) << "Unable to get the OnOff cluster from endpoint" << endpoint << "on" << node << "for" << thing;
                info->finish(Thing::ThingErrorSetupFailed);
                return;
            }
            bool power = info->action().param(lumiRelayRelay2ActionRelay2ParamTypeId).value().toBool();
            ZigbeeClusterReply *reply = (power ? onOffCluster->commandOn() : onOffCluster->commandOff());
            connect(reply, &ZigbeeClusterReply::finished, this, [=](){
                // Note: reply will be deleted automatically
                if (reply->error() != ZigbeeClusterReply::ErrorNoError) {
                    info->finish(Thing::ThingErrorHardwareFailure);
                } else {
                    info->finish(Thing::ThingErrorNoError);
                    thing->setStateValue(lumiRelayRelay2StateTypeId, power);
                }
            });
        }
    }

    info->finish(Thing::ThingErrorUnsupportedFeature);
}

void IntegrationPluginZigbeeLumi::thingRemoved(Thing *thing)
{
    ZigbeeNode *node = m_thingNodes.take(thing);
    if (node) {
        QUuid networkUuid = thing->paramValue(m_networkUuidParamTypeIds.value(thing->thingClassId())).toUuid();
        hardwareManager()->zigbeeResource()->removeNodeFromNetwork(networkUuid, node);
    }
}
