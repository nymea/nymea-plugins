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

#include "integrationpluginzigbeetradfri.h"
#include "plugininfo.h"
#include "hardware/zigbee/zigbeehardwareresource.h"


IntegrationPluginZigbeeTradfri::IntegrationPluginZigbeeTradfri()
{
    m_ieeeAddressParamTypeIds[onOffSwitchThingClassId] = onOffSwitchThingIeeeAddressParamTypeId;

    m_networkUuidParamTypeIds[onOffSwitchThingClassId] = onOffSwitchThingNetworkUuidParamTypeId;

    m_endpointIdParamTypeIds[onOffSwitchThingClassId] = onOffSwitchThingEndpointIdParamTypeId;

    m_connectedStateTypeIds[onOffSwitchThingClassId] = onOffSwitchConnectedStateTypeId;

    m_signalStrengthStateTypeIds[onOffSwitchThingClassId] = onOffSwitchSignalStrengthStateTypeId;

    m_versionStateTypeIds[onOffSwitchThingClassId] = onOffSwitchVersionStateTypeId;
}

QString IntegrationPluginZigbeeTradfri::name() const
{
    return "Ikea TRADFRI";
}

bool IntegrationPluginZigbeeTradfri::handleNode(ZigbeeNode *node, const QUuid &networkUuid)
{
    // Make sure this is from ikea 0x117C
    if (node->nodeDescriptor().manufacturerCode != Zigbee::Ikea)
        return false;

    bool handled = false;
    foreach (ZigbeeNodeEndpoint *endpoint, node->endpoints()) {
        if (endpoint->profile() == Zigbee::ZigbeeProfileHomeAutomation &&
                endpoint->deviceId() == Zigbee::HomeAutomationDeviceNonColourController) {
            qCDebug(dcZigbeeTradfri()) << "Handeling TRADFRI on/off switch" << node << endpoint;
            ThingDescriptor descriptor(onOffSwitchThingClassId);
            QString deviceClassName = supportedThings().findById(onOffSwitchThingClassId).displayName();
            descriptor.setTitle(deviceClassName);

            ParamList params;
            params.append(Param(onOffSwitchThingNetworkUuidParamTypeId, networkUuid.toString()));
            params.append(Param(onOffSwitchThingIeeeAddressParamTypeId, node->extendedAddress().toString()));
            descriptor.setParams(params);
            emit autoThingsAppeared({descriptor});
            handled = true;
        }
    }

    return handled;
}

void IntegrationPluginZigbeeTradfri::handleRemoveNode(ZigbeeNode *node, const QUuid &networkUuid)
{
    Q_UNUSED(networkUuid)
    Thing *thing = m_thingNodes.key(node);
    if (thing) {
        qCDebug(dcZigbeeTradfri()) << node << "for" << thing << "has left the network.";
        emit autoThingDisappeared(thing->id());

        // Removing it from our map to prevent a loop that would ask the zigbee network to remove this node (see thingRemoved())
        m_thingNodes.remove(thing);
    }
}

void IntegrationPluginZigbeeTradfri::init()
{
    hardwareManager()->zigbeeResource()->registerHandler(this, ZigbeeHardwareResource::HandlerTypeVendor);
}

void IntegrationPluginZigbeeTradfri::setupThing(ThingSetupInfo *info)
{
    Thing *thing = info->thing();
    QUuid networkUuid = thing->paramValue(m_networkUuidParamTypeIds.value(thing->thingClassId())).toUuid();
    ZigbeeAddress zigbeeAddress = ZigbeeAddress(thing->paramValue(m_ieeeAddressParamTypeIds.value(thing->thingClassId())).toString());
    ZigbeeNode *node = hardwareManager()->zigbeeResource()->claimNode(this, networkUuid, zigbeeAddress);
    if (!node) {
        qCWarning(dcZigbeeTradfri()) << "Zigbee node for" << info->thing()->name() << "not found.";
        info->finish(Thing::ThingErrorHardwareNotAvailable);
        return;
    }
    m_thingNodes.insert(thing, node);

    ZigbeeNodeEndpoint *endpoint = findEndpoint(thing);
    if (!endpoint) {
        qCWarning(dcZigbeeTradfri()) << "Could not find endpoint for" << thing;
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
        qCDebug(dcZigbeeTradfri()) << thing << "signal strength changed" << signalStrength << "%";
        thing->setStateValue(m_signalStrengthStateTypeIds.value(thing->thingClassId()), signalStrength);
    });

    // Set the version
    thing->setStateValue(m_versionStateTypeIds.value(thing->thingClassId()), endpoint->softwareBuildId());

    if (thing->thingClassId() == onOffSwitchThingClassId) {

        // Receive on/off commands
        ZigbeeClusterOnOff *onOffCluster = endpoint->outputCluster<ZigbeeClusterOnOff>(ZigbeeClusterLibrary::ClusterIdOnOff);
        if (!onOffCluster) {
            qCWarning(dcZigbeeTradfri()) << "Could not find on/off client cluster on" << thing << endpoint;
        } else {
            connect(onOffCluster, &ZigbeeClusterOnOff::commandSent, thing, [=](ZigbeeClusterOnOff::Command command){
                qCDebug(dcZigbeeTradfri()) << thing << "button pressed" << command;
                if (command == ZigbeeClusterOnOff::CommandOn) {
                    qCDebug(dcZigbeeTradfri()) << thing << "pressed ON";
                    emit emitEvent(Event(onOffSwitchPressedEventTypeId, thing->id(), ParamList() << Param(onOffSwitchLongPressedEventButtonNameParamTypeId, "ON")));
                } else if (command == ZigbeeClusterOnOff::CommandOff) {
                    qCDebug(dcZigbeeTradfri()) << thing << "pressed OFF";
                    emit emitEvent(Event(onOffSwitchPressedEventTypeId, thing->id(), ParamList() << Param(onOffSwitchLongPressedEventButtonNameParamTypeId, "OFF")));
                }
            });
        }

        // Receive level cntrol commands
        ZigbeeClusterLevelControl *levelCluster = endpoint->outputCluster<ZigbeeClusterLevelControl>(ZigbeeClusterLibrary::ClusterIdLevelControl);
        if (!levelCluster) {
            qCWarning(dcZigbeeTradfri()) << "Could not find level client cluster on" << thing << endpoint;
        } else {
            connect(levelCluster, &ZigbeeClusterLevelControl::commandSent, thing, [=](ZigbeeClusterLevelControl::Command command, const QByteArray &payload){
                qCDebug(dcZigbeeTradfri()) << thing << "button pressed" << command << payload.toHex();
                switch (command) {
                case ZigbeeClusterLevelControl::CommandMoveWithOnOff:
                    qCDebug(dcZigbeeTradfri()) << thing << "long pressed ON";
                    emit emitEvent(Event(onOffSwitchLongPressedEventTypeId, thing->id(), ParamList() << Param(onOffSwitchLongPressedEventButtonNameParamTypeId, "ON")));
                    break;
                case ZigbeeClusterLevelControl::CommandMove:
                    qCDebug(dcZigbeeTradfri()) << thing << "long pressed OFF";
                    emit emitEvent(Event(onOffSwitchLongPressedEventTypeId, thing->id(), ParamList() << Param(onOffSwitchLongPressedEventButtonNameParamTypeId, "OFF")));
                    break;
                default:
                    break;
                }
            });
        }

        ZigbeeClusterPowerConfiguration *powerCluster = endpoint->inputCluster<ZigbeeClusterPowerConfiguration>(ZigbeeClusterLibrary::ClusterIdPowerConfiguration);
        if (!powerCluster) {
            qCWarning(dcZigbeeTradfri()) << "Could not find power configuration cluster on" << thing << endpoint;
        } else {
            // Only set the initial state if the attribute already exists
            if (powerCluster->hasAttribute(ZigbeeClusterPowerConfiguration::AttributeBatteryPercentageRemaining)) {
                thing->setStateValue(onOffSwitchBatteryLevelStateTypeId, powerCluster->batteryPercentage());
                thing->setStateValue(onOffSwitchBatteryCriticalStateTypeId, (powerCluster->batteryPercentage() < 10.0));
            }

            connect(powerCluster, &ZigbeeClusterPowerConfiguration::batteryPercentageChanged, thing, [=](double percentage){
                qCDebug(dcZigbeeTradfri()) << "Battery percentage changed" << percentage << "%" << thing;
                thing->setStateValue(onOffSwitchBatteryLevelStateTypeId, percentage);
                thing->setStateValue(onOffSwitchBatteryCriticalStateTypeId, (percentage < 10.0));
            });
        }
    }

    info->finish(Thing::ThingErrorNoError);
}

void IntegrationPluginZigbeeTradfri::executeAction(ThingActionInfo *info)
{
    info->finish(Thing::ThingErrorUnsupportedFeature);
}

void IntegrationPluginZigbeeTradfri::thingRemoved(Thing *thing)
{
    ZigbeeNode *node = m_thingNodes.take(thing);
    if (node) {
        QUuid networkUuid = thing->paramValue(m_networkUuidParamTypeIds.value(thing->thingClassId())).toUuid();
        hardwareManager()->zigbeeResource()->removeNodeFromNetwork(networkUuid, node);
    }
}

ZigbeeNodeEndpoint *IntegrationPluginZigbeeTradfri::findEndpoint(Thing *thing)
{
    ZigbeeNode *node = m_thingNodes.value(thing);
    if (!node) {
        qCWarning(dcZigbeeTradfri()) << "Could not find the node for" << thing;
        return nullptr;
    }

    quint8 endpointId = thing->paramValue(m_endpointIdParamTypeIds.value(thing->thingClassId())).toUInt();
    return node->getEndpoint(endpointId);
}

