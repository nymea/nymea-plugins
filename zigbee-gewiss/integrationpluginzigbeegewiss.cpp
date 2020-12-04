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
    m_networkUuidParamTypeIds[gewissGwa1501ThingClassId] = gewissGwa1501ThingNetworkUuidParamTypeId;

    m_zigbeeAddressParamTypeIds[gewissGwa1501ThingClassId] = gewissGwa1501ThingIeeeAddressParamTypeId;

    m_connectedStateTypeIds[gewissGwa1501ThingClassId] = gewissGwa1501ConnectedStateTypeId;

    m_versionStateTypeIds[gewissGwa1501ThingClassId] = gewissGwa1501VersionStateTypeId;

    m_signalStrengthStateTypeIds[gewissGwa1501ThingClassId] = gewissGwa1501SignalStrengthStateTypeId;

    // Known model identifier
    m_knownGewissDevices.insert("GWA1501_BinaryInput_FC", gewissGwa1501ThingClassId);
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
    Thing *thing = info->thing();

    // Get the node for this thing
    QUuid networkUuid = thing->paramValue(m_networkUuidParamTypeIds.value(thing->thingClassId())).toUuid();
    ZigbeeAddress zigbeeAddress = ZigbeeAddress(thing->paramValue(m_zigbeeAddressParamTypeIds.value(thing->thingClassId())).toString());
    ZigbeeNode *node = hardwareManager()->zigbeeResource()->claimNode(this, networkUuid, zigbeeAddress);
    if (!node) {
        qCWarning(dcZigBeeGewiss()) << "Zigbee node for" << info->thing()->name() << "not found.´";
        info->finish(Thing::ThingErrorHardwareNotAvailable);
        return;
    }

    // Get the endpoint of interest (0x01) for this device
    ZigbeeNodeEndpoint *endpoint = node->getEndpoint(0x01);
    if (!endpoint) {
        qCWarning(dcZigBeeGewiss()) << "Zigbee endpoint 1 not found on" << thing;
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
        qCDebug(dcZigBeeGewiss()) << thing << "signal strength changed" << signalStrength << "%";
        thing->setStateValue(m_signalStrengthStateTypeIds.value(thing->thingClassId()), signalStrength);
    });

    // Set the version
    thing->setStateValue(m_versionStateTypeIds.value(thing->thingClassId()), endpoint->softwareBuildId());


    if (thing->thingClassId() == gewissGwa1501ThingClassId) {

        ZigbeeNodeEndpoint *endpointHa = node->getEndpoint(0x01);
        if (!endpoint) {
            qCWarning(dcZigBeeGewiss()) << "endpoint 0x01 not found" << thing->name();
            return;
        }
        ZigbeeClusterPowerConfiguration *powerCluster = node->getEndpoint(0x01)->inputCluster<ZigbeeClusterPowerConfiguration>(ZigbeeClusterLibrary::ClusterIdPowerConfiguration);
        if (!powerCluster) {
            qCWarning(dcZigBeeGewiss()) << "Could not find power configuration cluster on" << thing << endpointHa;
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

        connect(node, &ZigbeeNode::endpointClusterAttributeChanged, thing, [this, thing](ZigbeeNodeEndpoint *endpoint, ZigbeeCluster *cluster, const ZigbeeClusterAttribute &attribute){
            switch (endpoint->endpointId()) {
            case 0x01:
                if (cluster->clusterId() == ZigbeeClusterLibrary::ClusterIdBinaryInput && attribute.id() == ZigbeeClusterBinaryInput::AttributePresentValue) {
                    quint16 value = attribute.dataType().toUInt16();
                    if (value == 1) {
                        emit emitEvent(Event(gewissGwa1501PressedEventTypeId, thing->id(), ParamList() << Param(gewissGwa1501PressedEventButtonNameParamTypeId, "1")));
                    } else {
                        emit emitEvent(Event(gewissGwa1501LongPressedEventTypeId, thing->id(), ParamList() << Param(gewissGwa1501PressedEventButtonNameParamTypeId, "1")));
                    }
                }
                break;
            case 0x02:
                if (cluster->clusterId() == ZigbeeClusterLibrary::ClusterIdBinaryInput && attribute.id() == ZigbeeClusterBinaryInput::AttributePresentValue) {
                    quint16 value = attribute.dataType().toUInt16();
                    if (value == 1) {
                        emit emitEvent(Event(gewissGwa1501PressedEventTypeId, thing->id(), ParamList() << Param(gewissGwa1501PressedEventButtonNameParamTypeId, "2")));
                    } else {
                        emit emitEvent(Event(gewissGwa1501LongPressedEventTypeId, thing->id(), ParamList() << Param(gewissGwa1501LongPressedEventButtonNameParamTypeId, "2")));
                    }
                }
                break;
            default:
                qCWarning(dcZigBeeGewiss()) << "Received attribute changed signal from unhandled endpoint" << thing << endpoint << cluster << attribute;
                break;
            }
        });
    }

    info->finish(Thing::ThingErrorNoError);
}

void IntegrationPluginZigbeeGewiss::executeAction(ThingActionInfo *info)
{
    info->finish(Thing::ThingErrorUnsupportedFeature);
}

void IntegrationPluginZigbeeGewiss::thingRemoved(Thing *thing)
{
    ZigbeeNode *node = m_thingNodes.take(thing);
    if (node) {
        QUuid networkUuid = thing->paramValue(m_networkUuidParamTypeIds.value(thing->thingClassId())).toUuid();
        hardwareManager()->zigbeeResource()->removeNodeFromNetwork(networkUuid, node);
    }
}
