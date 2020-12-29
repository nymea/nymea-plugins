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

IntegrationPluginZigbeeGeneric::IntegrationPluginZigbeeGeneric()
{
    m_ieeeAddressParamTypeIds[thermostatThingClassId] = thermostatThingIeeeAddressParamTypeId;
    m_ieeeAddressParamTypeIds[powerSocketThingClassId] = powerSocketThingIeeeAddressParamTypeId;
    m_ieeeAddressParamTypeIds[doorLockThingClassId] = doorLockThingIeeeAddressParamTypeId;

    m_networkUuidParamTypeIds[thermostatThingClassId] = thermostatThingNetworkUuidParamTypeId;
    m_networkUuidParamTypeIds[powerSocketThingClassId] = powerSocketThingNetworkUuidParamTypeId;
    m_networkUuidParamTypeIds[doorLockThingClassId] = doorLockThingNetworkUuidParamTypeId;

    m_endpointIdParamTypeIds[thermostatThingClassId] = thermostatThingEndpointIdParamTypeId;
    m_endpointIdParamTypeIds[powerSocketThingClassId] = powerSocketThingEndpointIdParamTypeId;
    m_endpointIdParamTypeIds[doorLockThingClassId] = doorLockThingEndpointIdParamTypeId;

    m_manufacturerIdParamTypeIds[thermostatThingClassId] = thermostatThingManufacturerParamTypeId;
    m_manufacturerIdParamTypeIds[powerSocketThingClassId] = powerSocketThingManufacturerParamTypeId;
    m_manufacturerIdParamTypeIds[doorLockThingClassId] = doorLockThingManufacturerParamTypeId;

    m_modelIdParamTypeIds[thermostatThingClassId] = thermostatThingModelParamTypeId;
    m_modelIdParamTypeIds[powerSocketThingClassId] = powerSocketThingModelParamTypeId;
    m_modelIdParamTypeIds[doorLockThingClassId] = doorLockThingModelParamTypeId;

    m_connectedStateTypeIds[thermostatThingClassId] = thermostatConnectedStateTypeId;
    m_connectedStateTypeIds[powerSocketThingClassId] = powerSocketConnectedStateTypeId;
    m_connectedStateTypeIds[doorLockThingClassId] = doorLockConnectedStateTypeId;

    m_signalStrengthStateTypeIds[thermostatThingClassId] = thermostatSignalStrengthStateTypeId;
    m_signalStrengthStateTypeIds[powerSocketThingClassId] = powerSocketSignalStrengthStateTypeId;
    m_signalStrengthStateTypeIds[doorLockThingClassId] = doorLockSignalStrengthStateTypeId;

    m_versionStateTypeIds[thermostatThingClassId] = thermostatVersionStateTypeId;
    m_versionStateTypeIds[powerSocketThingClassId] = powerSocketVersionStateTypeId;
    m_versionStateTypeIds[doorLockThingClassId] = doorLockVersionStateTypeId;
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
            qCDebug(dcZigbeeGeneric()) << "Handeling thermostat endpoint for" << node << endpoint;
            createThing(thermostatThingClassId, networkUuid, node, endpoint);
            handled = true;
        }

        // Check on/off plug
        if ((endpoint->profile() == Zigbee::ZigbeeProfile::ZigbeeProfileLightLink &&
             endpoint->deviceId() == Zigbee::LightLinkDevice::LightLinkDeviceOnOffPlugin) ||
                (endpoint->profile() == Zigbee::ZigbeeProfile::ZigbeeProfileHomeAutomation &&
                 endpoint->deviceId() == Zigbee::HomeAutomationDeviceOnOffPlugin) ||
                (endpoint->profile() == Zigbee::ZigbeeProfile::ZigbeeProfileHomeAutomation &&
                 endpoint->deviceId() == Zigbee::HomeAutomationDeviceMainPowerOutlet)) {

            qCDebug(dcZigbeeGeneric()) << "Handeling power socket endpoint for" << node << endpoint;
            createThing(powerSocketThingClassId, networkUuid, node, endpoint);
            handled = true;
        }

        // Check door lock
        if (endpoint->profile() == Zigbee::ZigbeeProfile::ZigbeeProfileHomeAutomation && endpoint->deviceId() == Zigbee::HomeAutomationDeviceDoorLock) {
            if (!endpoint->hasInputCluster(ZigbeeClusterLibrary::ClusterIdPowerConfiguration) ||
                    !endpoint->hasInputCluster(ZigbeeClusterLibrary::ClusterIdDoorLock)) {
                qCWarning(dcZigbeeGeneric()) << "Endpoint claims to be a door lock, but the appropriate input clusters could not be found" << node << endpoint;
            } else {
                qCDebug(dcZigbeeGeneric()) << "Handeling door lock endpoint for" << node << endpoint;
                createThing(doorLockThingClassId, networkUuid, node, endpoint);
                // Initialize bindings and cluster attributes
                initializeDoorLock(node, endpoint);
                handled = true;
            }
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
    QUuid networkUuid = thing->paramValue(m_networkUuidParamTypeIds.value(thing->thingClassId())).toUuid();
    qCDebug(dcZigbeeGeneric()) << "Nework uuid:" << networkUuid;
    ZigbeeAddress zigbeeAddress = ZigbeeAddress(thing->paramValue(m_ieeeAddressParamTypeIds.value(thing->thingClassId())).toString());
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
    thing->setStateValue(m_connectedStateTypeIds.value(thing->thingClassId()), node->reachable());
    connect(node, &ZigbeeNode::reachableChanged, thing, [thing, this](bool reachable){
        thing->setStateValue(m_connectedStateTypeIds.value(thing->thingClassId()), reachable);
    });

    // Update signal strength
    thing->setStateValue(m_signalStrengthStateTypeIds.value(thing->thingClassId()), qRound(node->lqi() * 100.0 / 255.0));
    connect(node, &ZigbeeNode::lqiChanged, thing, [this, thing](quint8 lqi){
        uint signalStrength = qRound(lqi * 100.0 / 255.0);
        qCDebug(dcZigbeeGeneric()) << thing << "signal strength changed" << signalStrength << "%";
        thing->setStateValue(m_signalStrengthStateTypeIds.value(thing->thingClassId()), signalStrength);
    });

    // Set the version
    thing->setStateValue(m_versionStateTypeIds.value(thing->thingClassId()), endpoint->softwareBuildId());

    // Type specific setup
    if (thing->thingClassId() == thermostatThingClassId) {
        ZigbeeClusterThermostat *thermostatCluster = endpoint->inputCluster<ZigbeeClusterThermostat>(ZigbeeClusterLibrary::ClusterIdThermostat);
        if (!thermostatCluster) {
            qCWarning(dcZigbeeGeneric()) << "Failed to read thermostat cluster";
            return;
        }
        //        thermostatCluster->attribute(ZigbeeClusterLibrary::ClusterIdThermostat);

        // We need to read them from the lamp
        ZigbeeClusterReply *reply = thermostatCluster->readAttributes({ZigbeeClusterThermostat::AttributeLocalTemperature, ZigbeeClusterThermostat::AttributeOccupiedHeatingSetpoint});
        connect(reply, &ZigbeeClusterReply::finished, thing, [=](){
            if (reply->error() != ZigbeeClusterReply::ErrorNoError) {
                qCWarning(dcZigbeeGeneric()) << "Reading loacal temperature attribute finished with error" << reply->error();
                return;
            }

            QList<ZigbeeClusterLibrary::ReadAttributeStatusRecord> attributeStatusRecords = ZigbeeClusterLibrary::parseAttributeStatusRecords(reply->responseFrame().payload);

            foreach (const ZigbeeClusterLibrary::ReadAttributeStatusRecord &record, attributeStatusRecords) {
                if (record.attributeId == ZigbeeClusterThermostat::AttributeLocalTemperature) {
                    bool valueOk = false;
                    quint16 localTemperature = record.dataType.toUInt16(&valueOk);
                    if (!valueOk) {
                        qCWarning(dcZigbeeGeneric()) << "Failed to read local temperature" << attributeStatusRecords;
                        return;
                    }
                    thing->setStateValue(thermostatTemperatureStateTypeId, localTemperature * 0.01);
                }

                if (record.attributeId == ZigbeeClusterThermostat::AttributeOccupiedHeatingSetpoint) {
                    bool valueOk = false;
                    quint16 targetTemperature = record.dataType.toUInt16(&valueOk);
                    if (!valueOk) {
                        qCWarning(dcZigbeeGeneric()) << "Failed to read local temperature" << attributeStatusRecords;
                        return;
                    }
                    thing->setStateValue(thermostatTargetTemperatureStateTypeId, targetTemperature * 0.01);
                }
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

        // Get battery level changes
        ZigbeeClusterPowerConfiguration *powerCluster = endpoint->inputCluster<ZigbeeClusterPowerConfiguration>(ZigbeeClusterLibrary::ClusterIdPowerConfiguration);
        if (!powerCluster) {
            qCWarning(dcZigbeeGeneric()) << "Could not find power configuration cluster on" << thing << endpoint;
        } else {
            // Only set the initial state if the attribute already exists
            if (powerCluster->hasAttribute(ZigbeeClusterPowerConfiguration::AttributeBatteryPercentageRemaining)) {
                thing->setStateValue(doorLockBatteryLevelStateTypeId, powerCluster->batteryPercentage());
                thing->setStateValue(doorLockBatteryCriticalStateTypeId, (powerCluster->batteryPercentage() < 10.0));
            }

            connect(powerCluster, &ZigbeeClusterPowerConfiguration::batteryPercentageChanged, thing, [=](double percentage){
                qCDebug(dcZigbeeGeneric()) << "Battery percentage changed" << percentage << "%" << thing;
                thing->setStateValue(doorLockBatteryLevelStateTypeId, percentage);
                thing->setStateValue(doorLockBatteryCriticalStateTypeId, (percentage < 10.0));
            });
        }

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
        QUuid networkUuid = thing->paramValue(m_networkUuidParamTypeIds.value(thing->thingClassId())).toUuid();
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

    quint8 endpointId = thing->paramValue(m_endpointIdParamTypeIds.value(thing->thingClassId())).toUInt();
    return node->getEndpoint(endpointId);
}

void IntegrationPluginZigbeeGeneric::createThing(const ThingClassId &thingClassId, const QUuid &networkUuid, ZigbeeNode *node, ZigbeeNodeEndpoint *endpoint)
{
    ThingDescriptor descriptor(thingClassId);
    QString deviceClassName = supportedThings().findById(thingClassId).displayName();
    descriptor.setTitle(QString("%1 (%2 - %3)").arg(deviceClassName).arg(endpoint->manufacturerName()).arg(endpoint->modelIdentifier()));

    ParamList params;
    params.append(Param(m_networkUuidParamTypeIds[thingClassId], networkUuid.toString()));
    params.append(Param(m_ieeeAddressParamTypeIds[thingClassId], node->extendedAddress().toString()));
    params.append(Param(m_endpointIdParamTypeIds[thingClassId], endpoint->endpointId()));
    params.append(Param(m_modelIdParamTypeIds[thingClassId], endpoint->modelIdentifier()));
    params.append(Param(m_manufacturerIdParamTypeIds[thingClassId], endpoint->manufacturerName()));
    descriptor.setParams(params);
    emit autoThingsAppeared({descriptor});
}

void IntegrationPluginZigbeeGeneric::initializeDoorLock(ZigbeeNode *node, ZigbeeNodeEndpoint *endpoint)
{
    qCDebug(dcZigbeeGeneric()) << "Read power configuration cluster attributes" << node;
    ZigbeeClusterReply *readAttributeReply = endpoint->getInputCluster(ZigbeeClusterLibrary::ClusterIdPowerConfiguration)->readAttributes({ZigbeeClusterPowerConfiguration::AttributeBatteryPercentageRemaining});
    connect(readAttributeReply, &ZigbeeClusterReply::finished, node, [=](){
        if (readAttributeReply->error() != ZigbeeClusterReply::ErrorNoError) {
            qCWarning(dcZigbeeGeneric()) << "Failed to read power configuration cluster attributes" << readAttributeReply->error();
        } else {
            qCDebug(dcZigbeeGeneric()) << "Read power configuration cluster attributes finished successfully";
        }

        // Bind the cluster to the coordinator
        qCDebug(dcZigbeeGeneric()) << "Bind power configuration cluster to coordinator IEEE address";
        ZigbeeDeviceObjectReply * zdoReply = node->deviceObject()->requestBindIeeeAddress(endpoint->endpointId(), ZigbeeClusterLibrary::ClusterIdPowerConfiguration, hardwareManager()->zigbeeResource()->coordinatorAddress(node->networkUuid()), 0x01);
        connect(zdoReply, &ZigbeeDeviceObjectReply::finished, node, [=](){
            if (zdoReply->error() != ZigbeeDeviceObjectReply::ErrorNoError) {
                qCWarning(dcZigbeeGeneric()) << "Failed to bind power cluster to coordinator" << zdoReply->error();
            } else {
                qCDebug(dcZigbeeGeneric()) << "Bind power configuration cluster to coordinator finished successfully";
            }

            // Configure attribute rporting for battery remaining (0.5 % changes = 1)
            ZigbeeClusterLibrary::AttributeReportingConfiguration reportingConfig;
            reportingConfig.attributeId = ZigbeeClusterPowerConfiguration::AttributeBatteryPercentageRemaining;
            reportingConfig.dataType = Zigbee::Uint8;
            reportingConfig.minReportingInterval = 60; // for production use 300;
            reportingConfig.maxReportingInterval = 120; // for production use 2700;
            reportingConfig.reportableChange = ZigbeeDataType(static_cast<quint8>(1)).data();

            qCDebug(dcZigbeeGeneric()) << "Configure attribute reporting for power configuration cluster to coordinator";
            ZigbeeClusterReply *reportingReply = endpoint->getInputCluster(ZigbeeClusterLibrary::ClusterIdPowerConfiguration)->configureReporting({reportingConfig});
            connect(reportingReply, &ZigbeeClusterReply::finished, this, [=](){
                if (reportingReply->error() != ZigbeeClusterReply::ErrorNoError) {
                    qCWarning(dcZigbeeGeneric()) << "Failed to configure power cluster attribute reporting" << reportingReply->error();
                } else {
                    qCDebug(dcZigbeeGeneric()) << "Attribute reporting configuration finished for power cluster" << ZigbeeClusterLibrary::parseAttributeReportingStatusRecords(reportingReply->responseFrame().payload);
                }

                // Configure door lock attribute reporting and read initial values
                qCDebug(dcZigbeeGeneric()) << "Read door lock cluster attributes" << node;
                ZigbeeClusterReply *readAttributeReply = endpoint->getInputCluster(ZigbeeClusterLibrary::ClusterIdDoorLock)->readAttributes({ZigbeeClusterDoorLock::AttributeDoorState, ZigbeeClusterDoorLock::AttributeLockType});
                connect(readAttributeReply, &ZigbeeClusterReply::finished, node, [=](){
                    if (readAttributeReply->error() != ZigbeeClusterReply::ErrorNoError) {
                        qCWarning(dcZigbeeGeneric()) << "Failed to read door lock attributes" << readAttributeReply->error();
                    } else {
                        qCDebug(dcZigbeeGeneric()) << "Read door lock cluster attributes finished successfully";
                    }

                    // Bind the cluster to the coordinator
                    qCDebug(dcZigbeeGeneric()) << "Bind door lock cluster to coordinator IEEE address";
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
                });
            });
        });
    });
}
