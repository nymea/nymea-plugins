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

    m_zigbeeAddressParamTypeIds[lumiHTSensorThingClassId] = lumiHTSensorThingIeeeAddressParamTypeId;
    m_zigbeeAddressParamTypeIds[lumiButtonSensorThingClassId] = lumiButtonSensorThingIeeeAddressParamTypeId;
    m_zigbeeAddressParamTypeIds[lumiMagnetSensorThingClassId] = lumiMagnetSensorThingIeeeAddressParamTypeId;
    m_zigbeeAddressParamTypeIds[lumiMotionSensorThingClassId] = lumiMotionSensorThingIeeeAddressParamTypeId;
    m_zigbeeAddressParamTypeIds[lumiWaterSensorThingClassId] = lumiWaterSensorThingIeeeAddressParamTypeId;
    m_zigbeeAddressParamTypeIds[lumiWeatherSensorThingClassId] = lumiWeatherSensorThingIeeeAddressParamTypeId;
    m_zigbeeAddressParamTypeIds[lumiVibrationSensorThingClassId] = lumiVibrationSensorThingIeeeAddressParamTypeId;

    m_connectedStateTypeIds[lumiHTSensorThingClassId] = lumiHTSensorConnectedStateTypeId;
    m_connectedStateTypeIds[lumiButtonSensorThingClassId] = lumiButtonSensorConnectedStateTypeId;
    m_connectedStateTypeIds[lumiMagnetSensorThingClassId] = lumiMagnetSensorConnectedStateTypeId;
    m_connectedStateTypeIds[lumiMotionSensorThingClassId] = lumiMotionSensorConnectedStateTypeId;
    m_connectedStateTypeIds[lumiWaterSensorThingClassId] = lumiWaterSensorConnectedStateTypeId;
    m_connectedStateTypeIds[lumiWeatherSensorThingClassId] = lumiWeatherSensorConnectedStateTypeId;
    m_connectedStateTypeIds[lumiVibrationSensorThingClassId] = lumiVibrationSensorConnectedStateTypeId;

    m_versionStateTypeIds[lumiHTSensorThingClassId] = lumiHTSensorVersionStateTypeId;
    m_versionStateTypeIds[lumiButtonSensorThingClassId] = lumiButtonSensorVersionStateTypeId;
    m_versionStateTypeIds[lumiMagnetSensorThingClassId] = lumiMagnetSensorVersionStateTypeId;
    m_versionStateTypeIds[lumiMotionSensorThingClassId] = lumiMotionSensorVersionStateTypeId;
    m_versionStateTypeIds[lumiWaterSensorThingClassId] = lumiWaterSensorVersionStateTypeId;
    m_versionStateTypeIds[lumiWeatherSensorThingClassId] = lumiWeatherSensorVersionStateTypeId;
    m_versionStateTypeIds[lumiVibrationSensorThingClassId] = lumiVibrationSensorVersionStateTypeId;

    m_signalStrengthStateTypeIds[lumiHTSensorThingClassId] = lumiHTSensorSignalStrengthStateTypeId;
    m_signalStrengthStateTypeIds[lumiButtonSensorThingClassId] = lumiButtonSensorSignalStrengthStateTypeId;
    m_signalStrengthStateTypeIds[lumiMagnetSensorThingClassId] = lumiMagnetSensorSignalStrengthStateTypeId;
    m_signalStrengthStateTypeIds[lumiMotionSensorThingClassId] = lumiMotionSensorSignalStrengthStateTypeId;
    m_signalStrengthStateTypeIds[lumiWaterSensorThingClassId] = lumiWaterSensorSignalStrengthStateTypeId;
    m_signalStrengthStateTypeIds[lumiWeatherSensorThingClassId] = lumiWeatherSensorSignalStrengthStateTypeId;
    m_signalStrengthStateTypeIds[lumiVibrationSensorThingClassId] = lumiVibrationSensorSignalStrengthStateTypeId;

    // Known model identifier
    m_knownLumiDevices.insert("lumi.sensor_ht", lumiHTSensorThingClassId);
    m_knownLumiDevices.insert("lumi.sensor_magnet", lumiMagnetSensorThingClassId);
    m_knownLumiDevices.insert("lumi.sensor_switch", lumiButtonSensorThingClassId);
    m_knownLumiDevices.insert("lumi.sensor_motion", lumiMotionSensorThingClassId);
    m_knownLumiDevices.insert("lumi.sensor_wleak", lumiWaterSensorThingClassId);
    m_knownLumiDevices.insert("lumi.weather", lumiWeatherSensorThingClassId);
    m_knownLumiDevices.insert("lumi.vibration", lumiVibrationSensorThingClassId);
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

    if (m_nodeThings.values().contains(node)) {
        Thing *thing = m_nodeThings.key(node);
        qCDebug(dcZigbeeLumi()) << "Node for" << thing << node << "has left the network. Removing thing";
        m_nodeThings.remove(thing);
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
    QUuid networkUuid = thing->paramValue(m_networkUuidParamTypeIds.value(thing->thingClassId())).toUuid();
    qCDebug(dcZigbeeLumi()) << "Nework uuid:" << networkUuid;
    ZigbeeAddress zigbeeAddress = ZigbeeAddress(thing->paramValue(m_zigbeeAddressParamTypeIds.value(thing->thingClassId())).toString());

    ZigbeeNode *node = hardwareManager()->zigbeeResource()->getNode(networkUuid, zigbeeAddress);
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
    
    m_nodeThings.insert(thing, node);

    // General signals and states
    // Update connected state
    thing->setStateValue(m_connectedStateTypeIds.value(thing->thingClassId()), hardwareManager()->zigbeeResource()->networkState(networkUuid) == ZigbeeNetwork::StateRunning);
    connect(hardwareManager()->zigbeeResource(), &ZigbeeHardwareResource::networkStateChanged, thing, [thing, this](const QUuid &networkUuid, ZigbeeNetwork::State state){
        if (thing->paramValue(m_networkUuidParamTypeIds.value(thing->thingClassId())).toUuid() == networkUuid) {
            thing->setStateValue(m_connectedStateTypeIds.value(thing->thingClassId()), state == ZigbeeNetwork::StateRunning);
        }
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


    info->finish(Thing::ThingErrorNoError);


    //    if (thing->thingClassId() == lumiButtonSensorThingClassId) {
    //        qCDebug(dcZigbee()) << "Lumi button sensor" << thing;
    //        ZigbeeAddress ieeeAddress(thing->paramValue(lumiButtonSensorThingIeeeAddressParamTypeId).toString());
    //        ZigbeeNetwork *network = findParentNetwork(thing);
    //        LumiButtonSensor *sensor = new LumiButtonSensor(network, ieeeAddress, thing, this);
    //        connect(sensor, &LumiButtonSensor::buttonPressed, this, [this, thing](){
    //            qCDebug(dcZigbee()) << thing << "clicked event";
    //            emit emitEvent(Event(lumiButtonSensorPressedEventTypeId, thing->id()));
    //        });
    //        connect(sensor, &LumiButtonSensor::buttonLongPressed, this, [this, thing](){
    //            qCDebug(dcZigbee()) << thing << "long pressed event";
    //            emit emitEvent(Event(lumiButtonSensorLongPressedEventTypeId, thing->id()));
    //        });

    //        m_zigbeeDevices.insert(thing, sensor);
    //        info->finish(Thing::ThingErrorNoError);
    //        return;
    //    }


    //    if (thing->thingClassId() == lumiWaterSensorThingClassId) {
    //        qCDebug(dcZigbee()) << "Lumi water sensor" << thing;
    //        ZigbeeAddress ieeeAddress(thing->paramValue(lumiWaterSensorThingIeeeAddressParamTypeId).toString());
    //        ZigbeeNetwork *network = findParentNetwork(thing);
    //        LumiWaterSensor *sensor = new LumiWaterSensor(network, ieeeAddress, thing, this);
    //        m_zigbeeDevices.insert(thing, sensor);
    //        info->finish(Thing::ThingErrorNoError);
    //        return;
    //    }

    //    info->finish(Thing::ThingErrorThingClassNotFound);
}

void IntegrationPluginZigbeeLumi::executeAction(ThingActionInfo *info)
{
    info->finish(Thing::ThingErrorUnsupportedFeature);
}

void IntegrationPluginZigbeeLumi::thingRemoved(Thing *thing)
{
    ZigbeeNode *node = m_nodeThings.take(thing);
    if (node) {
        QUuid networkUuid = thing->paramValue(m_networkUuidParamTypeIds.value(thing->thingClassId())).toUuid();
        hardwareManager()->zigbeeResource()->removeNodeFromNetwork(networkUuid, node);
    }
}
