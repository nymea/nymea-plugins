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

#include "integrationpluginzigbeegeneric.h"
#include "plugininfo.h"
#include "hardware/zigbee/zigbeehardwareresource.h"

#include "zcl/hvac/zigbeeclusterthermostat.h"

#include <QDebug>

static QHash<ThingClassId, StateTypeId> batteryLevelStateTypeIds = {
    {thermostatThingClassId, thermostatBatteryLevelStateTypeId},
    {doorLockThingClassId, doorLockBatteryLevelStateTypeId},
    {doorSensorThingClassId, doorSensorBatteryLevelStateTypeId}
};

static QHash<ThingClassId, StateTypeId> batteryCriticalStateTypeIds = {
    {thermostatThingClassId, thermostatBatteryCriticalStateTypeId},
    {doorLockThingClassId, doorLockBatteryCriticalStateTypeId},
    {doorSensorThingClassId, doorSensorBatteryCriticalStateTypeId}
};

static QHash<ThingClassId, ParamTypeId> ieeeAddressParamTypeIds = {
    {thermostatThingClassId, thermostatThingIeeeAddressParamTypeId},
    {powerSocketThingClassId, powerSocketThingIeeeAddressParamTypeId},
    {doorLockThingClassId, doorLockThingIeeeAddressParamTypeId},
    {doorSensorThingClassId, doorSensorThingIeeeAddressParamTypeId}
};

static QHash<ThingClassId, ParamTypeId> networkUuidParamTypeIds = {
    {thermostatThingClassId, thermostatThingNetworkUuidParamTypeId},
    {powerSocketThingClassId, powerSocketThingNetworkUuidParamTypeId},
    {doorLockThingClassId, doorLockThingNetworkUuidParamTypeId},
    {doorSensorThingClassId, doorSensorThingNetworkUuidParamTypeId}
};

static QHash<ThingClassId, ParamTypeId> endpointIdParamTypeIds = {
    {thermostatThingClassId, thermostatThingEndpointIdParamTypeId},
    {powerSocketThingClassId, powerSocketThingEndpointIdParamTypeId},
    {doorLockThingClassId, doorLockThingEndpointIdParamTypeId},
    {doorSensorThingClassId, doorSensorThingEndpointIdParamTypeId}
};

static QHash<ThingClassId, ParamTypeId> modelIdParamTypeIds = {
    {thermostatThingClassId, thermostatThingManufacturerParamTypeId},
    {powerSocketThingClassId, powerSocketThingManufacturerParamTypeId},
    {doorLockThingClassId, doorLockThingManufacturerParamTypeId},
    {doorSensorThingClassId, doorSensorThingManufacturerParamTypeId}
};

static QHash<ThingClassId, ParamTypeId> manufacturerIdParamTypeIds = {
    {thermostatThingClassId, thermostatThingModelParamTypeId},
    {powerSocketThingClassId, powerSocketThingModelParamTypeId},
    {doorLockThingClassId, doorLockThingModelParamTypeId},
    {doorSensorThingClassId, doorSensorThingModelParamTypeId}
};

static QHash<ThingClassId, StateTypeId> connectedStateTypeIds = {
    {thermostatThingClassId, thermostatConnectedStateTypeId},
    {powerSocketThingClassId, powerSocketConnectedStateTypeId},
    {doorLockThingClassId, doorLockConnectedStateTypeId},
    {doorSensorThingClassId, doorSensorConnectedStateTypeId}
};

static QHash<ThingClassId, StateTypeId> signalStrengthStateTypeIds = {
    {thermostatThingClassId, thermostatSignalStrengthStateTypeId},
    {powerSocketThingClassId, powerSocketSignalStrengthStateTypeId},
    {doorLockThingClassId, doorLockSignalStrengthStateTypeId},
    {doorSensorThingClassId, doorSensorSignalStrengthStateTypeId}
};

static QHash<ThingClassId, StateTypeId> versionStateTypeIds = {
    {thermostatThingClassId, thermostatVersionStateTypeId},
    {powerSocketThingClassId, powerSocketVersionStateTypeId},
    {doorLockThingClassId, doorLockVersionStateTypeId}
};


IntegrationPluginZigbeeGeneric::IntegrationPluginZigbeeGeneric()
{
}

QString IntegrationPluginZigbeeGeneric::name() const
{
    return "Generic";
}

bool IntegrationPluginZigbeeGeneric::handleNode(ZigbeeNode *node, const QUuid &networkUuid)
{
    bool handled = false;
    foreach (ZigbeeNodeEndpoint *endpoint, node->endpoints()) {
        qCDebug(dcZigbeeGeneric()) << "Checking node endpoint:" << endpoint->endpointId() << endpoint->deviceId();

        // Check thermostat
        if (endpoint->profile() == Zigbee::ZigbeeProfile::ZigbeeProfileHomeAutomation &&
                endpoint->deviceId() == Zigbee::HomeAutomationDeviceThermostat) {
            qCDebug(dcZigbeeGeneric()) << "Handling thermostat endpoint for" << node << endpoint;
            createThing(thermostatThingClassId, networkUuid, node, endpoint);
            initThermostat(node, endpoint);
            handled = true;
        }

        // Check on/off thing
        if ((endpoint->profile() == Zigbee::ZigbeeProfile::ZigbeeProfileLightLink && endpoint->deviceId() == Zigbee::LightLinkDevice::LightLinkDeviceOnOffPlugin) ||
                (endpoint->profile() == Zigbee::ZigbeeProfile::ZigbeeProfileHomeAutomation && endpoint->deviceId() == Zigbee::HomeAutomationDeviceOnOffPlugin) ||
                (endpoint->profile() == Zigbee::ZigbeeProfile::ZigbeeProfileHomeAutomation && endpoint->deviceId() == Zigbee::HomeAutomationDeviceMainPowerOutlet) ||
                (endpoint->profile() == Zigbee::ZigbeeProfile::ZigbeeProfileHomeAutomation && endpoint->deviceId() == Zigbee::HomeAutomationDeviceSmartPlug)) {

            // Simple on/off device
            if (endpoint->hasInputCluster(ZigbeeClusterLibrary::ClusterIdOnOff)) {
                // FIXME: create powersocket with metering for SmartPlug device ID
                qCDebug(dcZigbeeGeneric()) << "Handling power socket endpoint for" << node << endpoint;
                createThing(powerSocketThingClassId, networkUuid, node, endpoint);
                initSimplePowerSocket(node, endpoint);
                handled = true;
            }
        }

        // Check door lock
        if (endpoint->profile() == Zigbee::ZigbeeProfile::ZigbeeProfileHomeAutomation && endpoint->deviceId() == Zigbee::HomeAutomationDeviceDoorLock) {
            if (!endpoint->hasInputCluster(ZigbeeClusterLibrary::ClusterIdPowerConfiguration) ||
                    !endpoint->hasInputCluster(ZigbeeClusterLibrary::ClusterIdDoorLock)) {
                qCWarning(dcZigbeeGeneric()) << "Endpoint claims to be a door lock, but the appropriate input clusters could not be found" << node << endpoint;
            } else {
                qCDebug(dcZigbeeGeneric()) << "Handling door lock endpoint for" << node << endpoint;
                createThing(doorLockThingClassId, networkUuid, node, endpoint);
                // Initialize bindings and cluster attributes
                initDoorLock(node, endpoint);
                handled = true;
            }
        }

        // Security sensors
        if (endpoint->profile() == Zigbee::ZigbeeProfile::ZigbeeProfileHomeAutomation && endpoint->deviceId() == Zigbee::HomeAutomationDeviceIsaZone) {
            qCInfo(dcZigbeeGeneric()) << "ISA Zone device found!";
            // We need to read the Type cluster to determine what this actually is...
            ZigbeeClusterIasZone *iasZoneCluster = endpoint->inputCluster<ZigbeeClusterIasZone>(ZigbeeClusterLibrary::ClusterIdIasZone);
            ZigbeeClusterReply *reply = iasZoneCluster->readAttributes({ZigbeeClusterIasZone::AttributeZoneType});
            connect(reply, &ZigbeeClusterReply::finished, this, [=](){
                if (reply->error() != ZigbeeClusterReply::ErrorNoError) {
                    qCWarning(dcZigbeeGeneric()) << "Reading IAS Zone type attribute finished with error" << reply->error();
                    return;
                }

                QList<ZigbeeClusterLibrary::ReadAttributeStatusRecord> attributeStatusRecords = ZigbeeClusterLibrary::parseAttributeStatusRecords(reply->responseFrame().payload);
                if (attributeStatusRecords.count() != 1 || attributeStatusRecords.first().attributeId != ZigbeeClusterIasZone::AttributeZoneType) {
                    qCWarning(dcZigbeeGeneric()) << "Unexpected reply in reading IAS Zone device type:" << attributeStatusRecords;
                    return;
                }
                ZigbeeClusterLibrary::ReadAttributeStatusRecord iasZoneTypeRecord = attributeStatusRecords.first();
                qCDebug(dcZigbeeGeneric()) << "IAS Zone device type:" << iasZoneTypeRecord.dataType.toUInt16();
                switch (iasZoneTypeRecord.dataType.toUInt16()) {
                case ZigbeeClusterIasZone::ZoneTypeContactSwitch:
                    qCInfo(dcZigbeeGeneric()) << "Creating contact switch thing";
                    createThing(doorSensorThingClassId, networkUuid, node, endpoint);
                    initDoorSensor(node, endpoint);
                    break;
                default:
                    qCWarning(dcZigbeeGeneric()) << "Unhandled IAS Zone device type:" << "0x" + QString::number(iasZoneTypeRecord.dataType.toUInt16(), 16);

                }

            });

            handled = true;
        }
    }

    return handled;
}

void IntegrationPluginZigbeeGeneric::handleRemoveNode(ZigbeeNode *node, const QUuid &networkUuid)
{
    Q_UNUSED(networkUuid)
    Thing *thing = m_thingNodes.key(node);
    if (thing) {
        qCDebug(dcZigbeeGeneric()) << node << "for" << thing << "has left the network.";
        emit autoThingDisappeared(thing->id());

        // Removing it from our map to prevent a loop that would ask the zigbee network to remove this node (see thingRemoved())
        m_thingNodes.remove(thing);
    }
}

void IntegrationPluginZigbeeGeneric::init()
{
    hardwareManager()->zigbeeResource()->registerHandler(this, ZigbeeHardwareResource::HandlerTypeCatchAll);
}

void IntegrationPluginZigbeeGeneric::setupThing(ThingSetupInfo *info)
{
    Thing *thing = info->thing();
    QUuid networkUuid = thing->paramValue(networkUuidParamTypeIds.value(thing->thingClassId())).toUuid();
    qCDebug(dcZigbeeGeneric()) << "Nework uuid:" << networkUuid;
    ZigbeeAddress zigbeeAddress = ZigbeeAddress(thing->paramValue(ieeeAddressParamTypeIds.value(thing->thingClassId())).toString());
    ZigbeeNode *node = m_thingNodes.value(thing);
    if (!node) {
        node = hardwareManager()->zigbeeResource()->claimNode(this, networkUuid, zigbeeAddress);
    }

    if (!node) {
        qCWarning(dcZigbeeGeneric()) << "Zigbee node for" << info->thing()->name() << "not found.´";
        info->finish(Thing::ThingErrorHardwareNotAvailable);
        return;
    }
    m_thingNodes.insert(thing, node);

    ZigbeeNodeEndpoint *endpoint = findEndpoint(thing);
    if (!endpoint) {
        qCWarning(dcZigbeeGeneric()) << "Could not find endpoint for" << thing;
        info->finish(Thing::ThingErrorSetupFailed);
        return;
    }

    // Update connected state
    thing->setStateValue(connectedStateTypeIds.value(thing->thingClassId()), node->reachable());
    connect(node, &ZigbeeNode::reachableChanged, thing, [thing, this](bool reachable){
        thing->setStateValue(connectedStateTypeIds.value(thing->thingClassId()), reachable);
    });

    // Update signal strength
    thing->setStateValue(signalStrengthStateTypeIds.value(thing->thingClassId()), qRound(node->lqi() * 100.0 / 255.0));
    connect(node, &ZigbeeNode::lqiChanged, thing, [this, thing](quint8 lqi){
        uint signalStrength = qRound(lqi * 100.0 / 255.0);
        qCDebug(dcZigbeeGeneric()) << thing << "signal strength changed" << signalStrength << "%";
        thing->setStateValue(signalStrengthStateTypeIds.value(thing->thingClassId()), signalStrength);
    });

    // Set the version
    thing->setStateValue(versionStateTypeIds.value(thing->thingClassId()), endpoint->softwareBuildId());

    if (batteryLevelStateTypeIds.contains(thing->thingClassId())) {
        connectToPowerConfigurationCluster(thing, endpoint);
    }

    // Type specific setup
    if (thing->thingClassId() == thermostatThingClassId) {
        ZigbeeClusterThermostat *thermostatCluster = endpoint->inputCluster<ZigbeeClusterThermostat>(ZigbeeClusterLibrary::ClusterIdThermostat);
        if (!thermostatCluster) {
            qCWarning(dcZigbeeGeneric()) << "Failed to read thermostat cluster";
            info->finish(Thing::ThingErrorHardwareFailure);
            return;
        }

        // Read initial attribute values
        ZigbeeClusterReply *reply = thermostatCluster->readAttributes({ZigbeeClusterThermostat::AttributeLocalTemperature,
                                                                       ZigbeeClusterThermostat::AttributeOccupiedHeatingSetpoint,
                                                                       ZigbeeClusterThermostat::AttributePIHeatingDemand,
                                                                       ZigbeeClusterThermostat::AttributePICoolingDemand});
        connect(reply, &ZigbeeClusterReply::finished, thing, [=](){
            if (reply->error() != ZigbeeClusterReply::ErrorNoError) {
                qCWarning(dcZigbeeGeneric()) << "Reading thermostat attributes finished with error" << reply->error();
                return;
            }

            QList<ZigbeeClusterLibrary::ReadAttributeStatusRecord> attributeStatusRecords = ZigbeeClusterLibrary::parseAttributeStatusRecords(reply->responseFrame().payload);
            foreach (const ZigbeeClusterLibrary::ReadAttributeStatusRecord &record, attributeStatusRecords) {
                if (record.attributeId == ZigbeeClusterThermostat::AttributeLocalTemperature) {
                    thing->setStateValue(thermostatTemperatureStateTypeId, record.dataType.toInt16() * 0.01);
                }
                if (record.attributeId == ZigbeeClusterThermostat::AttributeOccupiedHeatingSetpoint) {
                    thing->setStateValue(thermostatTargetTemperatureStateTypeId, record.dataType.toInt16() * 0.01);
                }
                if (record.attributeId == ZigbeeClusterThermostat::AttributePIHeatingDemand) {
                    thing->setStateValue(thermostatHeatingOnStateTypeId, record.dataType.toUInt8() > 0);
                }
                if (record.attributeId == ZigbeeClusterThermostat::AttributePICoolingDemand) {
                    thing->setStateValue(thermostatCoolingOnStateTypeId, record.dataType.toUInt8() > 0);
                }
            }
        });

        // Connect to attribute changes
        connect(thermostatCluster, &ZigbeeClusterThermostat::attributeChanged, thing, [thing](const ZigbeeClusterAttribute &attribute){
            qCDebug(dcZigbeeGeneric()) << "Thermostat attribute changed" << thing->name() << attribute.id() << attribute.dataType();
            if (attribute.id() == ZigbeeClusterThermostat::AttributeOccupiedHeatingSetpoint) {
                thing->setStateValue(thermostatTargetTemperatureStateTypeId, attribute.dataType().toUInt16() * 0.01);
            }
            if (attribute.id() == ZigbeeClusterThermostat::AttributeLocalTemperature) {
                thing->setStateValue(thermostatTemperatureStateTypeId, attribute.dataType().toUInt16() * 0.01);
            }
            if (attribute.id() == ZigbeeClusterThermostat::AttributePIHeatingDemand) {
                thing->setStateValue(thermostatHeatingOnStateTypeId, attribute.dataType().toUInt8() > 0);
            }
            if (attribute.id() == ZigbeeClusterThermostat::AttributePICoolingDemand) {
                thing->setStateValue(thermostatCoolingOnStateTypeId, attribute.dataType().toUInt8() > 0);
            }
        });
    }

    if (thing->thingClassId() == powerSocketThingClassId) {
        ZigbeeClusterOnOff *onOffCluster = endpoint->inputCluster<ZigbeeClusterOnOff>(ZigbeeClusterLibrary::ClusterIdOnOff);
        if (onOffCluster) {
            if (onOffCluster->hasAttribute(ZigbeeClusterOnOff::AttributeOnOff)) {
                thing->setStateValue(powerSocketPowerStateTypeId, onOffCluster->power());
            }
            connect(onOffCluster, &ZigbeeClusterOnOff::powerChanged, thing, [thing](bool power){
                qCDebug(dcZigbeeGeneric()) << thing << "power changed" << power;
                thing->setStateValue(powerSocketPowerStateTypeId, power);
            });

            connect(node, &ZigbeeNode::reachableChanged, thing, [=](bool reachable){
                if (reachable) {
                    ZigbeeClusterReply *reply = onOffCluster->readAttributes({ZigbeeClusterOnOff::AttributeOnOff});
                    connect(reply, &ZigbeeClusterReply::finished, thing, [=](){
                        if (reply->error() != ZigbeeClusterReply::ErrorNoError) {
                            qCWarning(dcZigbeeGeneric()) << "Reading attribute from" << thing << "finished with error" << reply->error();
                        }
                        // Note: the state will be updated using the power changed signal from the cluster
                    });
                }
            });
        } else {
            qCWarning(dcZigbeeGeneric()) << "Could not find the OnOff input cluster on" << thing << endpoint;
        }
    }

    if (thing->thingClassId() == doorLockThingClassId) {

        // Get door state changes
        ZigbeeClusterDoorLock *doorLockCluster = endpoint->inputCluster<ZigbeeClusterDoorLock>(ZigbeeClusterLibrary::ClusterIdDoorLock);
        if (!doorLockCluster) {
            qCWarning(dcZigbeeGeneric()) << "Could not find door lock cluster on" << thing << endpoint;
        } else {
            // Only set the initial state if the attribute already exists
            if (doorLockCluster->hasAttribute(ZigbeeClusterDoorLock::AttributeDoorState)) {
                qCDebug(dcZigbeeGeneric()) << thing << doorLockCluster->doorState();
                // TODO: check if we can use smart lock and set appropriate state
            }

            connect(doorLockCluster, &ZigbeeClusterDoorLock::lockStateChanged, thing, [=](ZigbeeClusterDoorLock::LockState lockState){
                qCDebug(dcZigbeeGeneric()) << thing << "lock state changed" << lockState;
                // TODO: check if we can use smart lock and set appropriate state
            });
        }
    }

    if (thing->thingClassId() == doorSensorThingClassId) {
        ZigbeeClusterIasZone *iasZoneCluster = endpoint->inputCluster<ZigbeeClusterIasZone>(ZigbeeClusterLibrary::ClusterIdIasZone);
        if (!iasZoneCluster) {
            qCWarning(dcZigbeeGeneric()) << "Could not find IAS zone cluster on" << thing << endpoint;
        } else {
            if (iasZoneCluster->hasAttribute(ZigbeeClusterIasZone::AttributeZoneStatus)) {
                qCDebug(dcZigbeeGeneric()) << thing << iasZoneCluster->zoneStatus();
                ZigbeeClusterIasZone::ZoneStatusFlags zoneStatus = iasZoneCluster->zoneStatus();
                thing->setStateValue(doorSensorClosedStateTypeId, !zoneStatus.testFlag(ZigbeeClusterIasZone::ZoneStatusAlarm1) && !zoneStatus.testFlag(ZigbeeClusterIasZone::ZoneStatusAlarm2));
            }
            connect(iasZoneCluster, &ZigbeeClusterIasZone::zoneStatusChanged, thing, [=](ZigbeeClusterIasZone::ZoneStatusFlags zoneStatus, quint8 extendedStatus, quint8 zoneId, quint16 delays) {
                qCDebug(dcZigbeeGeneric()) << "Zone status changed to:" << zoneStatus << extendedStatus << zoneId << delays;
                thing->setStateValue(doorSensorClosedStateTypeId, !zoneStatus.testFlag(ZigbeeClusterIasZone::ZoneStatusAlarm1) && !zoneStatus.testFlag(ZigbeeClusterIasZone::ZoneStatusAlarm2));
            });
        }
    }

    info->finish(Thing::ThingErrorNoError);
}

void IntegrationPluginZigbeeGeneric::executeAction(ThingActionInfo *info)
{
    if (!hardwareManager()->zigbeeResource()->available()) {
        info->finish(Thing::ThingErrorHardwareNotAvailable);
        return;
    }

    // Get the node
    Thing *thing = info->thing();
    ZigbeeNode *node = m_thingNodes.value(thing);
    if (!node->reachable()) {
        info->finish(Thing::ThingErrorHardwareNotAvailable);
        return;
    }

    // Get the endpoint
    ZigbeeNodeEndpoint *endpoint = findEndpoint(thing);
    if (!endpoint) {
        info->finish(Thing::ThingErrorHardwareNotAvailable);
        return;
    }

    if (thing->thingClassId() == thermostatThingClassId) {
        if (info->action().actionTypeId() == thermostatTargetTemperatureActionTypeId) {
            ZigbeeClusterThermostat *thermostatCluster = endpoint->inputCluster<ZigbeeClusterThermostat>(ZigbeeClusterLibrary::ClusterIdThermostat);
            if (!thermostatCluster) {
                qCWarning(dcZigbeeGeneric()) << "Thermostat cluster not found on thing" << thing->name();
                info->finish(Thing::ThingErrorHardwareFailure);
                return;
            }
            qint16 targetTemp = qRound(info->action().paramValue(thermostatTargetTemperatureStateTypeId).toDouble() * 10) * 10;
            // TODO: The following should probably move into libnymea-zibee prividing a
            // thermostatCluster->writeOccupiedHeatingSetpoint(targetTemp);
            ZigbeeDataType dataType(targetTemp);
            QList<ZigbeeClusterLibrary::WriteAttributeRecord> attributes;
            ZigbeeClusterLibrary::WriteAttributeRecord occupiedHeatingSetpointAttribute;
            occupiedHeatingSetpointAttribute.attributeId = ZigbeeClusterThermostat::AttributeOccupiedHeatingSetpoint;
            occupiedHeatingSetpointAttribute.dataType = dataType.dataType();
            occupiedHeatingSetpointAttribute.data = dataType.data();
            attributes.append(occupiedHeatingSetpointAttribute);
            qCDebug(dcZigbeeGeneric()) << "Writing target temp" << targetTemp << occupiedHeatingSetpointAttribute.data;
            ZigbeeClusterReply *reply = thermostatCluster->writeAttributes(attributes);
            connect(reply, &ZigbeeClusterReply::finished, info, [info, reply](){
                qCDebug(dcZigbeeGeneric()) << "Writing attributes finished:" << reply->error();
                if (reply->error() != ZigbeeClusterReply::ErrorNoError) {
                    info->finish(Thing::ThingErrorHardwareFailure);
                    return;
                }
                info->thing()->setStateValue(thermostatTargetTemperatureStateTypeId, info->action().paramValue(thermostatTargetTemperatureActionTargetTemperatureParamTypeId));
                info->finish(Thing::ThingErrorNoError);
            });
            return;
        }
    }

    if (thing->thingClassId() == powerSocketThingClassId) {
        if (info->action().actionTypeId() == powerSocketAlertActionTypeId) {
            ZigbeeClusterIdentify *identifyCluster = endpoint->inputCluster<ZigbeeClusterIdentify>(ZigbeeClusterLibrary::ClusterIdIdentify);
            if (!identifyCluster) {
                qCWarning(dcZigbeeGeneric()) << "Could not find identify cluster for" << thing << "in" << m_thingNodes.value(thing);
                info->finish(Thing::ThingErrorHardwareFailure);
                return;
            }

            // Send the command trough the network
            ZigbeeClusterReply *reply = identifyCluster->identify(2);
            connect(reply, &ZigbeeClusterReply::finished, this, [reply, info](){
                // Note: reply will be deleted automatically
                if (reply->error() != ZigbeeClusterReply::ErrorNoError) {
                    info->finish(Thing::ThingErrorHardwareFailure);
                } else {
                    info->finish(Thing::ThingErrorNoError);
                }
            });
            return;
        }

        if (info->action().actionTypeId() == powerSocketPowerActionTypeId) {
            ZigbeeClusterOnOff *onOffCluster = endpoint->inputCluster<ZigbeeClusterOnOff>(ZigbeeClusterLibrary::ClusterIdOnOff);
            if (!onOffCluster) {
                qCWarning(dcZigbeeGeneric()) << "Could not find on/off cluster for" << thing << "in" << endpoint;
                info->finish(Thing::ThingErrorHardwareFailure);
                return;
            }

            // Send the command trough the network
            bool power = info->action().param(powerSocketPowerActionPowerParamTypeId).value().toBool();
            qCDebug(dcZigbeeGeneric()) << "Set power for" << thing << "to" << power;
            ZigbeeClusterReply *reply = (power ? onOffCluster->commandOn() : onOffCluster->commandOff());
            connect(reply, &ZigbeeClusterReply::finished, info, [=](){
                // Note: reply will be deleted automatically
                if (reply->error() != ZigbeeClusterReply::ErrorNoError) {
                    qCWarning(dcZigbeeGeneric()) << "Failed to set power on" << thing << reply->error();
                    info->finish(Thing::ThingErrorHardwareFailure);
                } else {
                    info->finish(Thing::ThingErrorNoError);
                    qCDebug(dcZigbeeGeneric()) << "Set power finished successfully for" << thing;
                    thing->setStateValue(powerSocketPowerStateTypeId, power);
                }
            });
            return;
        }
    }

    if (thing->thingClassId() == doorLockThingClassId) {
        if (info->action().actionTypeId() == doorLockOpenActionTypeId) {
            ZigbeeClusterDoorLock *doorLockCluster = endpoint->inputCluster<ZigbeeClusterDoorLock>(ZigbeeClusterLibrary::ClusterIdDoorLock);
            if (!doorLockCluster) {
                qCWarning(dcZigbeeGeneric()) << "Could not find door lock cluster for" << thing << "in" << m_thingNodes.value(thing);
                info->finish(Thing::ThingErrorHardwareFailure);
                return;
            }

            // Send the command trough the network
            ZigbeeClusterReply *reply = doorLockCluster->unlockDoor();
            connect(reply, &ZigbeeClusterReply::finished, this, [reply, info](){
                // Note: reply will be deleted automatically
                if (reply->error() != ZigbeeClusterReply::ErrorNoError) {
                    info->finish(Thing::ThingErrorHardwareFailure);
                } else {
                    info->finish(Thing::ThingErrorNoError);
                }
            });
            return;
        }

        if (info->action().actionTypeId() == doorLockCloseActionTypeId) {
            ZigbeeClusterDoorLock *doorLockCluster = endpoint->inputCluster<ZigbeeClusterDoorLock>(ZigbeeClusterLibrary::ClusterIdDoorLock);
            if (!doorLockCluster) {
                qCWarning(dcZigbeeGeneric()) << "Could not find door lock cluster for" << thing << "in" << m_thingNodes.value(thing);
                info->finish(Thing::ThingErrorHardwareFailure);
                return;
            }

            // Send the command trough the network
            ZigbeeClusterReply *reply = doorLockCluster->lockDoor();
            connect(reply, &ZigbeeClusterReply::finished, this, [reply, info](){
                // Note: reply will be deleted automatically
                if (reply->error() != ZigbeeClusterReply::ErrorNoError) {
                    info->finish(Thing::ThingErrorHardwareFailure);
                } else {
                    info->finish(Thing::ThingErrorNoError);
                }
            });
            return;
        }
    }

    info->finish(Thing::ThingErrorUnsupportedFeature);
}

void IntegrationPluginZigbeeGeneric::thingRemoved(Thing *thing)
{
    ZigbeeNode *node = m_thingNodes.take(thing);
    if (node) {
        QUuid networkUuid = thing->paramValue(networkUuidParamTypeIds.value(thing->thingClassId())).toUuid();
        hardwareManager()->zigbeeResource()->removeNodeFromNetwork(networkUuid, node);
    }
}

ZigbeeNodeEndpoint *IntegrationPluginZigbeeGeneric::findEndpoint(Thing *thing)
{
    ZigbeeNode *node = m_thingNodes.value(thing);
    if (!node) {
        qCWarning(dcZigbeeGeneric()) << "Could not find the node for" << thing;
        return nullptr;
    }

    quint8 endpointId = thing->paramValue(endpointIdParamTypeIds.value(thing->thingClassId())).toUInt();
    return node->getEndpoint(endpointId);
}

void IntegrationPluginZigbeeGeneric::createThing(const ThingClassId &thingClassId, const QUuid &networkUuid, ZigbeeNode *node, ZigbeeNodeEndpoint *endpoint)
{
    ThingDescriptor descriptor(thingClassId);
    QString deviceClassName = supportedThings().findById(thingClassId).displayName();
    descriptor.setTitle(QString("%1 (%2 - %3)").arg(deviceClassName).arg(endpoint->manufacturerName()).arg(endpoint->modelIdentifier()));

    ParamList params;
    params.append(Param(networkUuidParamTypeIds[thingClassId], networkUuid.toString()));
    params.append(Param(ieeeAddressParamTypeIds[thingClassId], node->extendedAddress().toString()));
    params.append(Param(endpointIdParamTypeIds[thingClassId], endpoint->endpointId()));
    params.append(Param(modelIdParamTypeIds[thingClassId], endpoint->modelIdentifier()));
    params.append(Param(manufacturerIdParamTypeIds[thingClassId], endpoint->manufacturerName()));
    descriptor.setParams(params);
    emit autoThingsAppeared({descriptor});
}

void IntegrationPluginZigbeeGeneric::initSimplePowerSocket(ZigbeeNode *node, ZigbeeNodeEndpoint *endpoint)
{
    // Get the on/off server cluster from the endpoint
    ZigbeeClusterOnOff *onOffCluster = endpoint->inputCluster<ZigbeeClusterOnOff>(ZigbeeClusterLibrary::ClusterIdOnOff);
    if (!onOffCluster)
        return;

    qCDebug(dcZigbeeGeneric()) << "Reading on/off power value for" << node << endpoint;
    ZigbeeClusterReply *reply = onOffCluster->readAttributes({ZigbeeClusterOnOff::AttributeOnOff});
    connect(reply, &ZigbeeClusterReply::finished, node, [=](){
        if (reply->error() != ZigbeeClusterReply::ErrorNoError) {
            qCWarning(dcZigbeeGeneric()) << "Failed to read on/off cluster attribute from" << node << endpoint << reply->error();
            return;
        }
    });
}

void IntegrationPluginZigbeeGeneric::initDoorLock(ZigbeeNode *node, ZigbeeNodeEndpoint *endpoint)
{
    bindPowerConfigurationCluster(node, endpoint);

    qCDebug(dcZigbeeGeneric()) << "Binding door lock cluster ";
    ZigbeeDeviceObjectReply * zdoReply = node->deviceObject()->requestBindIeeeAddress(endpoint->endpointId(), ZigbeeClusterLibrary::ClusterIdDoorLock, hardwareManager()->zigbeeResource()->coordinatorAddress(node->networkUuid()), 0x01);
    connect(zdoReply, &ZigbeeDeviceObjectReply::finished, node, [=](){
        if (zdoReply->error() != ZigbeeDeviceObjectReply::ErrorNoError) {
            qCWarning(dcZigbeeGeneric()) << "Failed to door lock cluster to coordinator" << zdoReply->error();
        } else {
            qCDebug(dcZigbeeGeneric()) << "Bind door lock cluster to coordinator finished successfully";
        }

        // Configure attribute reporting for lock state
        ZigbeeClusterLibrary::AttributeReportingConfiguration reportingConfig;
        reportingConfig.attributeId = ZigbeeClusterDoorLock::AttributeLockState;
        reportingConfig.dataType = Zigbee::Enum8;
        reportingConfig.minReportingInterval = 60;
        reportingConfig.maxReportingInterval = 120;
        reportingConfig.reportableChange = ZigbeeDataType(static_cast<quint8>(1)).data();

        qCDebug(dcZigbeeGeneric()) << "Configure attribute reporting for door lock cluster to coordinator";
        ZigbeeClusterReply *reportingReply = endpoint->getInputCluster(ZigbeeClusterLibrary::ClusterIdDoorLock)->configureReporting({reportingConfig});
        connect(reportingReply, &ZigbeeClusterReply::finished, this, [=](){
            if (reportingReply->error() != ZigbeeClusterReply::ErrorNoError) {
                qCWarning(dcZigbeeGeneric()) << "Failed to door lock cluster attribute reporting" << reportingReply->error();
            } else {
                qCDebug(dcZigbeeGeneric()) << "Attribute reporting configuration finished for door lock cluster" << ZigbeeClusterLibrary::parseAttributeReportingStatusRecords(reportingReply->responseFrame().payload);
            }
        });
    });
}

void IntegrationPluginZigbeeGeneric::initThermostat(ZigbeeNode *node, ZigbeeNodeEndpoint *endpoint)
{
    bindPowerConfigurationCluster(node, endpoint);

    qCDebug(dcZigbeeGeneric()) << "Binding thermostat custer";
    ZigbeeDeviceObjectReply *bindThermostatReply = node->deviceObject()->requestBindIeeeAddress(endpoint->endpointId(), ZigbeeClusterLibrary::ClusterIdThermostat,
                                                                                     hardwareManager()->zigbeeResource()->coordinatorAddress(node->networkUuid()), 0x01);
    connect(bindThermostatReply, &ZigbeeDeviceObjectReply::finished, node, [=](){
        if (bindThermostatReply->error() != ZigbeeDeviceObjectReply::ErrorNoError) {
            qCWarning(dcZigbeeGeneric()) << "Failed to bind thermostat cluster" << bindThermostatReply->error();
        } else {
            qCDebug(dcZigbeeGeneric()) << "Binding thermostat cluster finished successfully";
        }

        ZigbeeClusterLibrary::AttributeReportingConfiguration reportingOccupiedHeatingSetpointConfig;
        reportingOccupiedHeatingSetpointConfig.attributeId = ZigbeeClusterThermostat::AttributeOccupiedHeatingSetpoint;
        reportingOccupiedHeatingSetpointConfig.dataType = Zigbee::Int16;
        reportingOccupiedHeatingSetpointConfig.minReportingInterval = 300;
        reportingOccupiedHeatingSetpointConfig.maxReportingInterval = 2700;
        reportingOccupiedHeatingSetpointConfig.reportableChange = ZigbeeDataType(static_cast<quint8>(1)).data();

        qCDebug(dcZigbeeGeneric()) << "Configuring attribute reporting for thermostat cluster";
        ZigbeeClusterReply *reportingReply = endpoint->getInputCluster(ZigbeeClusterLibrary::ClusterIdThermostat)->configureReporting({reportingOccupiedHeatingSetpointConfig});
        connect(reportingReply, &ZigbeeClusterReply::finished, this, [=](){
            if (reportingReply->error() != ZigbeeClusterReply::ErrorNoError) {
                qCWarning(dcZigbeeGeneric()) << "Failed to configure thermostat cluster attribute reporting" << reportingReply->error();
            } else {
                qCDebug(dcZigbeeGeneric()) << "Attribute reporting configuration finished for thermostat cluster" << ZigbeeClusterLibrary::parseAttributeReportingStatusRecords(reportingReply->responseFrame().payload);
            }
        });
    });
}

void IntegrationPluginZigbeeGeneric::initDoorSensor(ZigbeeNode *node, ZigbeeNodeEndpoint *endpoint)
{
    bindPowerConfigurationCluster(node, endpoint);

    qCDebug(dcZigbeeGeneric()) << "Binding IAS custer";
    ZigbeeDeviceObjectReply *bindIasClusterReply = node->deviceObject()->requestBindIeeeAddress(endpoint->endpointId(), ZigbeeClusterLibrary::ClusterIdIasZone,
                                                                                     hardwareManager()->zigbeeResource()->coordinatorAddress(node->networkUuid()), 0x01);
    connect(bindIasClusterReply, &ZigbeeDeviceObjectReply::finished, node, [=](){
        if (bindIasClusterReply->error() != ZigbeeDeviceObjectReply::ErrorNoError) {
            qCWarning(dcZigbeeGeneric()) << "Failed to bind IAS zone cluster" << bindIasClusterReply->error();
        } else {
            qCDebug(dcZigbeeGeneric()) << "Binding IAS zone cluster finished successfully";
        }

        ZigbeeClusterLibrary::AttributeReportingConfiguration reportingStatusConfig;
        reportingStatusConfig.attributeId = ZigbeeClusterIasZone::AttributeZoneStatus;
        reportingStatusConfig.dataType = Zigbee::Int16;
        reportingStatusConfig.minReportingInterval = 300;
        reportingStatusConfig.maxReportingInterval = 2700;
        reportingStatusConfig.reportableChange = ZigbeeDataType(static_cast<quint8>(1)).data();

        qCDebug(dcZigbeeGeneric()) << "Configuring attribute reporting for thermostat cluster";
        ZigbeeClusterReply *reportingReply = endpoint->getInputCluster(ZigbeeClusterLibrary::ClusterIdIasZone)->configureReporting({reportingStatusConfig});
        connect(reportingReply, &ZigbeeClusterReply::finished, this, [=](){
            if (reportingReply->error() != ZigbeeClusterReply::ErrorNoError) {
                qCWarning(dcZigbeeGeneric()) << "Failed to configure IAS Zone cluster status attribute reporting" << reportingReply->error();
            } else {
                qCDebug(dcZigbeeGeneric()) << "Attribute reporting configuration finished for IAS Zone cluster" << ZigbeeClusterLibrary::parseAttributeReportingStatusRecords(reportingReply->responseFrame().payload);
            }
        });
    });
}

void IntegrationPluginZigbeeGeneric::bindPowerConfigurationCluster(ZigbeeNode *node, ZigbeeNodeEndpoint *endpoint)
{
    ZigbeeDeviceObjectReply *bindPowerReply = node->deviceObject()->requestBindIeeeAddress(endpoint->endpointId(), ZigbeeClusterLibrary::ClusterIdPowerConfiguration,
                                                                                           hardwareManager()->zigbeeResource()->coordinatorAddress(node->networkUuid()), 0x01);
    connect(bindPowerReply, &ZigbeeDeviceObjectReply::finished, node, [=](){
        if (bindPowerReply->error() != ZigbeeDeviceObjectReply::ErrorNoError) {
            qCWarning(dcZigbeeGeneric()) << "Failed to bind power configuration cluster" << bindPowerReply->error();
        } else {
            qCDebug(dcZigbeeGeneric()) << "Binding power configuration cluster finished successfully";
        }

        ZigbeeClusterLibrary::AttributeReportingConfiguration batteryPercentageConfig;
        batteryPercentageConfig.attributeId = ZigbeeClusterPowerConfiguration::AttributeBatteryPercentageRemaining;
        batteryPercentageConfig.dataType = Zigbee::Uint8;
        batteryPercentageConfig.minReportingInterval = 60; // for production use 300;
        batteryPercentageConfig.maxReportingInterval = 120; // for production use 2700;
        batteryPercentageConfig.reportableChange = ZigbeeDataType(static_cast<quint8>(1)).data();

        qCDebug(dcZigbeeGeneric()) << "Configuring attribute reporting for power configuration cluster";
        ZigbeeClusterReply *reportingReply = endpoint->getInputCluster(ZigbeeClusterLibrary::ClusterIdPowerConfiguration)->configureReporting({batteryPercentageConfig});
        connect(reportingReply, &ZigbeeClusterReply::finished, this, [=](){
            if (reportingReply->error() != ZigbeeClusterReply::ErrorNoError) {
                qCWarning(dcZigbeeGeneric()) << "Failed to configure power configuration cluster attribute reporting" << reportingReply->error();
            } else {
                qCDebug(dcZigbeeGeneric()) << "Attribute reporting configuration finished for power configuration cluster" << ZigbeeClusterLibrary::parseAttributeReportingStatusRecords(reportingReply->responseFrame().payload);
            }
        });
    });
}

void IntegrationPluginZigbeeGeneric::connectToPowerConfigurationCluster(Thing *thing, ZigbeeNodeEndpoint *endpoint)
{
    // Get battery level changes
    ZigbeeClusterPowerConfiguration *powerCluster = endpoint->inputCluster<ZigbeeClusterPowerConfiguration>(ZigbeeClusterLibrary::ClusterIdPowerConfiguration);
    if (powerCluster) {
        // If the power cluster attributes are already available, read values now
        if (powerCluster->hasAttribute(ZigbeeClusterPowerConfiguration::AttributeBatteryPercentageRemaining)) {
            thing->setStateValue(batteryLevelStateTypeIds.value(thing->thingClassId()), powerCluster->batteryPercentage());
            thing->setStateValue(batteryCriticalStateTypeIds.value(thing->thingClassId()), (powerCluster->batteryPercentage() < 10.0));
        }
        // Refresh power cluster attributes in any case
        ZigbeeClusterReply *reply = powerCluster->readAttributes({ZigbeeClusterPowerConfiguration::AttributeBatteryPercentageRemaining});
        connect(reply, &ZigbeeClusterReply::finished, thing, [=](){
            if (reply->error() != ZigbeeClusterReply::ErrorNoError) {
                qCWarning(dcZigbeeGeneric()) << "Reading power configuration cluster attributes finished with error" << reply->error();
                return;
            }
            thing->setStateValue(batteryLevelStateTypeIds.value(thing->thingClassId()), powerCluster->batteryPercentage());
            thing->setStateValue(batteryCriticalStateTypeIds.value(thing->thingClassId()), (powerCluster->batteryPercentage() < 10.0));
        });

        // Connect to battery level changes
        connect(powerCluster, &ZigbeeClusterPowerConfiguration::batteryPercentageChanged, thing, [=](double percentage){
            thing->setStateValue(batteryLevelStateTypeIds.value(thing->thingClassId()), percentage);
            thing->setStateValue(batteryCriticalStateTypeIds.value(thing->thingClassId()), (percentage < 10.0));
        });
    }
}
