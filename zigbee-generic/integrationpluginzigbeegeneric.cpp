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

    m_networkUuidParamTypeIds[thermostatThingClassId] = thermostatThingNetworkUuidParamTypeId;

    m_connectedStateTypeIds[thermostatThingClassId] = thermostatConnectedStateTypeId;

    m_signalStrengthStateTypeIds[thermostatThingClassId] = thermostatSignalStrengthStateTypeId;
}

QString IntegrationPluginZigbeeGeneric::name() const
{
    return "Generic";
}

bool IntegrationPluginZigbeeGeneric::handleNode(ZigbeeNode *node, const QUuid &networkUuid)
{
    qCDebug(dcZigbeeGeneric()) << "handleNode called for:" << node;

    QHash<quint16, ThingClassId> devicesThingClassIdsMap;
    devicesThingClassIdsMap.insert(Zigbee::HomeAutomationDeviceThermostat, thermostatThingClassId);

    bool handled = false;

    foreach (ZigbeeNodeEndpoint *endpoint, node->endpoints()) {
        qCDebug(dcZigbeeGeneric()) << "Checking ndoe endpoint:" << endpoint->endpointId() << endpoint->deviceId();

        if (devicesThingClassIdsMap.contains(endpoint->deviceId())) {
            ThingClassId thingClassId = devicesThingClassIdsMap.value(endpoint->deviceId());
            ThingDescriptor descriptor(thingClassId, endpoint->modelIdentifier(), endpoint->manufacturerName());
            ParamList params;
            params << Param(m_ieeeAddressParamTypeIds.value(thingClassId), node->extendedAddress().toString());
            params << Param(m_networkUuidParamTypeIds.value(thingClassId), networkUuid.toString());
            descriptor.setParams(params);
            emit autoThingsAppeared({descriptor});

            handled = true;
        }
    }

    return handled;
}

void IntegrationPluginZigbeeGeneric::handleRemoveNode(ZigbeeNode *node, const QUuid &networkUuid)
{
    Q_UNUSED(networkUuid)
    Thing *thing = m_zigbeeNodes.key(node);
    if (thing) {
        qCDebug(dcZigbeeGeneric()) << node << "for" << thing << "has left the network.";
        emit autoThingDisappeared(thing->id());

        // Removing it from our map to prevent a loop that would ask the zigbee network to remove this node (see thingRemoved())
        m_zigbeeNodes.remove(thing);
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
    ZigbeeNode *node = m_zigbeeNodes.value(thing);
    if (!node) {
        node = hardwareManager()->zigbeeResource()->claimNode(this, networkUuid, zigbeeAddress);
    }
    if (!node) {
        qCWarning(dcZigbeeGeneric()) << "Zigbee node for" << info->thing()->name() << "not found.´";
        info->finish(Thing::ThingErrorHardwareNotAvailable);
        return;
    }

    m_zigbeeNodes.insert(thing, node);

    ZigbeeNodeEndpoint *endpoint = node->getEndpoint(0x01);
    if (!endpoint) {
        qCWarning(dcZigbeeGeneric()) << "Zigbee endpoint 1 not found on" << thing->name();
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

    info->finish(Thing::ThingErrorNoError);
}

void IntegrationPluginZigbeeGeneric::executeAction(ThingActionInfo *info)
{
    info->finish(Thing::ThingErrorUnsupportedFeature);
}

void IntegrationPluginZigbeeGeneric::thingRemoved(Thing *thing)
{
    ZigbeeNode *node = m_zigbeeNodes.take(thing);
    if (node) {
        QUuid networkUuid = thing->paramValue(m_networkUuidParamTypeIds.value(thing->thingClassId())).toUuid();
        hardwareManager()->zigbeeResource()->removeNodeFromNetwork(networkUuid, node);
    }
}
