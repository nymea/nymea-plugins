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

    m_zigbeeAddressParamTypeIds[lumiHTSensorThingClassId] = lumiHTSensorThingIeeeAddressParamTypeId;
    m_zigbeeAddressParamTypeIds[lumiButtonSensorThingClassId] = lumiButtonSensorThingIeeeAddressParamTypeId;
    m_zigbeeAddressParamTypeIds[lumiMagnetSensorThingClassId] = lumiMagnetSensorThingIeeeAddressParamTypeId;
    m_zigbeeAddressParamTypeIds[lumiMotionSensorThingClassId] = lumiMotionSensorThingIeeeAddressParamTypeId;
    m_zigbeeAddressParamTypeIds[lumiWaterSensorThingClassId] = lumiWaterSensorThingIeeeAddressParamTypeId;

    m_connectedStateTypeIds[lumiHTSensorThingClassId] = lumiHTSensorConnectedStateTypeId;
    m_connectedStateTypeIds[lumiButtonSensorThingClassId] = lumiButtonSensorConnectedStateTypeId;
    m_connectedStateTypeIds[lumiMagnetSensorThingClassId] = lumiMagnetSensorConnectedStateTypeId;
    m_connectedStateTypeIds[lumiMotionSensorThingClassId] = lumiMotionSensorConnectedStateTypeId;
    m_connectedStateTypeIds[lumiWaterSensorThingClassId] = lumiWaterSensorConnectedStateTypeId;

    m_signalStrengthStateTypeIds[lumiHTSensorThingClassId] = lumiHTSensorSignalStrengthStateTypeId;
    m_signalStrengthStateTypeIds[lumiButtonSensorThingClassId] = lumiButtonSensorSignalStrengthStateTypeId;
    m_signalStrengthStateTypeIds[lumiMagnetSensorThingClassId] = lumiMagnetSensorSignalStrengthStateTypeId;
    m_signalStrengthStateTypeIds[lumiMotionSensorThingClassId] = lumiMotionSensorSignalStrengthStateTypeId;
    m_signalStrengthStateTypeIds[lumiWaterSensorThingClassId] = lumiWaterSensorSignalStrengthStateTypeId;

}

QString IntegrationPluginZigbeeLumi::name() const
{
    return "Lumi";
}

bool IntegrationPluginZigbeeLumi::handleNode(ZigbeeNode *node, const QUuid &networkUuid)
{
    // Check if this is Lumi
    // Note: Lumi / Xiaomi / Aquara devices are not in the specs, so no enum here
    if (node->nodeDescriptor().manufacturerCode != 0x1037) {
        return false;
    }

    foreach (ZigbeeNodeEndpoint *endpoint, node->endpoints()) {
        // Get the model identifier if present from the first endpoint. Also this is out of spec
        if (!endpoint->hasInputCluster(ZigbeeClusterLibrary::ClusterIdBasic)) {
            qCWarning(dcZigbeeLumi()) << "This lumi device does not have the basic input cluster yet.";
            continue;
        }

        QHash<QString, ThingClassId> knownLumiDevices;
        knownLumiDevices.insert("lumi.sensor_ht", lumiHTSensorThingClassId);
        knownLumiDevices.insert("lumi.sensor_magnet", lumiMagnetSensorThingClassId);
        knownLumiDevices.insert("lumi.sensor_switch", lumiButtonSensorThingClassId);
        knownLumiDevices.insert("lumi.sensor_motion", lumiMotionSensorThingClassId);
        knownLumiDevices.insert("lumi.sensor_water", lumiWaterSensorThingClassId);

        ThingClassId thingClassId;
        foreach (const QString &knownLumi, knownLumiDevices.keys()) {
            if (endpoint->modelIdentifier().startsWith(knownLumi)) {
                thingClassId = knownLumiDevices.value(knownLumi);
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

void IntegrationPluginZigbeeLumi::init()
{
    hardwareManager()->zigbeeResource()->registerHandler(this);
}

void IntegrationPluginZigbeeLumi::setupThing(ThingSetupInfo *info)
{
    if (!hardwareManager()->zigbeeResource()->available()) {
        qCWarning(dcZigbeeLumi()) << "Zigbee is not available. Not setting up" << info->thing()->name();
        info->finish(Thing::ThingErrorHardwareNotAvailable);
        return;
    }
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

    ZigbeeNodeEndpoint *endpoint = node->getEndpoint(0x01);
    if (!endpoint) {
        qCWarning(dcZigbeeLumi()) << "Zigbee endpoint 1 not found on" << thing->name();
        info->finish(Thing::ThingErrorSetupFailed);
        return;
    }
    
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


    if (thing->thingClassId() == lumiMagnetSensorThingClassId) {
        ZigbeeClusterOnOff *onOffCluster = endpoint->inputCluster<ZigbeeClusterOnOff>(ZigbeeClusterLibrary::ClusterIdOnOff);
        if (onOffCluster) {
            thing->setStateValue(lumiMagnetSensorClosedStateTypeId, !onOffCluster->powered());
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
            thing->setStateValue(lumiMotionSensorIsPresentStateTypeId, occupancyCluster->occupancy());
            connect(occupancyCluster, &ZigbeeClusterOccupancySensing::occupancyChanged, thing, [thing](bool occupancy){
                qCDebug(dcZigbeeLumi()) << "occupancy changed" << occupancy;
                thing->setStateValue(lumiMotionSensorIsPresentStateTypeId, occupancy);
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
            thing->setStateValue(lumiHTSensorTemperatureStateTypeId, illuminanceCluster->illuminance());
            connect(illuminanceCluster, &ZigbeeClusterIlluminanceMeasurment::illuminanceChanged, thing, [thing](quint16 illuminance){
                thing->setStateValue(lumiMotionSensorLightIntensityStateTypeId, illuminance);
            });
        } else {
            qCWarning(dcZigbeeLumi()) << "Illuminance cluster not found on" << thing->name();
        }
    }

    if (thing->thingClassId() == lumiHTSensorThingClassId) {
        ZigbeeClusterTemperatureMeasurement *temperatureCluster = endpoint->inputCluster<ZigbeeClusterTemperatureMeasurement>(ZigbeeClusterLibrary::ClusterIdTemperatureMeasurement);
        if (temperatureCluster) {
            thing->setStateValue(lumiHTSensorTemperatureStateTypeId, temperatureCluster->temperature());
            connect(temperatureCluster, &ZigbeeClusterTemperatureMeasurement::temperatureChanged, thing, [thing](double temperature){
                thing->setStateValue(lumiHTSensorTemperatureStateTypeId, temperature);
            });
        } else {
            qCWarning(dcZigbeeLumi()) << "Could not find the temperature measurement server cluster on" << thing << endpoint;
        }

        ZigbeeClusterRelativeHumidityMeasurement *humidityCluster = endpoint->inputCluster<ZigbeeClusterRelativeHumidityMeasurement>(ZigbeeClusterLibrary::ClusterIdRelativeHumidityMeasurement);
        if (humidityCluster) {
            thing->setStateValue(lumiHTSensorHumidityStateTypeId, humidityCluster->humidity());
            connect(humidityCluster, &ZigbeeClusterRelativeHumidityMeasurement::humidityChanged, thing, [thing](double humidity){
                thing->setStateValue(lumiHTSensorHumidityStateTypeId, humidity);
            });
        } else {
            qCWarning(dcZigbeeLumi()) << "Could not find the relative humidity measurement server cluster on" << thing << endpoint;
        }
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
    Q_UNUSED(thing)
}
