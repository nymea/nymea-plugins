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

#include "integrationpluginzigbeegenericlights.h"
#include "plugininfo.h"
#include "hardware/zigbee/zigbeehardwareresource.h"

#include <QDebug>

IntegrationPluginZigbeeGenericLights::IntegrationPluginZigbeeGenericLights()
{
    m_ieeeAddressParamTypeIds[onOffLightThingClassId] = onOffLightThingIeeeAddressParamTypeId;

    m_networkUuidParamTypeIds[onOffLightThingClassId] = onOffLightThingNetworkUuidParamTypeId;

    m_endpointIdParamTypeIds[onOffLightThingClassId] = onOffLightThingEndpointIdParamTypeId;

    m_connectedStateTypeIds[onOffLightThingClassId] = onOffLightConnectedStateTypeId;

    m_signalStrengthStateTypeIds[onOffLightThingClassId] = onOffLightSignalStrengthStateTypeId;

    m_versionStateTypeIds[onOffLightThingClassId] = onOffLightVersionStateTypeId;
}

QString IntegrationPluginZigbeeGenericLights::name() const
{
    return "Generic lights";
}

bool IntegrationPluginZigbeeGenericLights::handleNode(ZigbeeNode *node, const QUuid &networkUuid)
{
    foreach (ZigbeeNodeEndpoint *endpoint, node->endpoints()) {
        if ((endpoint->profile() == Zigbee::ZigbeeProfile::ZigbeeProfileLightLink &&
             endpoint->deviceId() == Zigbee::LightLinkDevice::LightLinkDeviceOnOffLight) ||
                (endpoint->profile() == Zigbee::ZigbeeProfile::ZigbeeProfileHomeAutomation &&
                 endpoint->deviceId() == Zigbee::HomeAutomationDeviceOnOffLight)) {

            ThingDescriptor descriptor(onOffLightThingClassId);
            QString deviceClassName = supportedThings().findById(onOffLightThingClassId).displayName();
            descriptor.setTitle(QString("%1 (%2 - %3)").arg(deviceClassName).arg(endpoint->manufacturerName()).arg(endpoint->modelIdentifier()));
            qCDebug(dcZigbeeGenericLights()) << "Handeling generic on/off light" << descriptor.title();

            ParamList params;
            params.append(Param(onOffLightThingNetworkUuidParamTypeId, networkUuid.toString()));
            params.append(Param(onOffLightThingIeeeAddressParamTypeId, node->extendedAddress().toString()));
            params.append(Param(onOffLightThingEndpointIdParamTypeId, endpoint->endpointId()));
            params.append(Param(onOffLightThingModelParamTypeId, endpoint->modelIdentifier()));
            params.append(Param(onOffLightThingManufacturerParamTypeId, endpoint->manufacturerName()));
            descriptor.setParams(params);
            emit autoThingsAppeared({descriptor});

            return true;
        }
    }

    return false;
}

void IntegrationPluginZigbeeGenericLights::handleRemoveNode(ZigbeeNode *node, const QUuid &networkUuid)
{
    Q_UNUSED(networkUuid)

    if (m_thingNodes.values().contains(node)) {
        Thing *thing = m_thingNodes.key(node);
        qCDebug(dcZigbeeGenericLights()) << node << "for" << thing << "has left the network.";
        m_thingNodes.remove(thing);
        emit autoThingDisappeared(thing->id());
    }
}

void IntegrationPluginZigbeeGenericLights::init()
{
    hardwareManager()->zigbeeResource()->registerHandler(this, ZigbeeHardwareResource::HandlerTypeCatchAll);
}

void IntegrationPluginZigbeeGenericLights::setupThing(ThingSetupInfo *info)
{
    Thing *thing = info->thing();

    // Get the node for this thing
    QUuid networkUuid = thing->paramValue(m_networkUuidParamTypeIds.value(thing->thingClassId())).toUuid();
    ZigbeeAddress zigbeeAddress = ZigbeeAddress(thing->paramValue(m_ieeeAddressParamTypeIds.value(thing->thingClassId())).toString());
    ZigbeeNode *node = hardwareManager()->zigbeeResource()->getNode(networkUuid, zigbeeAddress);
    if (!node) {
        qCWarning(dcZigbeeGenericLights()) << "Zigbee node for" << info->thing()->name() << "not found.´";
        info->finish(Thing::ThingErrorHardwareNotAvailable);
        return;
    }

    // Store the node thing association for removing
    m_thingNodes.insert(thing, node);

    // Get the endpoint for this thing
    ZigbeeNodeEndpoint *endpoint = findEndpoint(thing);
    if (!endpoint) {
        qCWarning(dcZigbeeGenericLights()) << "Unable to get the endpoint from node" << node << "for" << thing;
        info->finish(Thing::ThingErrorSetupFailed);
        return;
    }

    // General thing setup

    // Update connected state
    thing->setStateValue(m_connectedStateTypeIds.value(thing->thingClassId()), node->reachable());
    connect(node, &ZigbeeNode::reachableChanged, thing, [thing, this](bool reachable){
        thing->setStateValue(m_connectedStateTypeIds.value(thing->thingClassId()), reachable);
    });

    // Update signal strength
    thing->setStateValue(m_signalStrengthStateTypeIds.value(thing->thingClassId()), qRound(node->lqi() * 100.0 / 255.0));
    connect(node, &ZigbeeNode::lqiChanged, thing, [this, thing](quint8 lqi){
        uint signalStrength = qRound(lqi * 100.0 / 255.0);
        qCDebug(dcZigbeeGenericLights()) << thing << "signal strength changed" << signalStrength << "%";
        thing->setStateValue(m_signalStrengthStateTypeIds.value(thing->thingClassId()), signalStrength);
    });

    // Set the version
    thing->setStateValue(m_versionStateTypeIds.value(thing->thingClassId()), endpoint->softwareBuildId());

    // Thing specific setup
    if (thing->thingClassId() == onOffLightThingClassId) {

        ZigbeeClusterOnOff *onOffCluster = endpoint->inputCluster<ZigbeeClusterOnOff>(ZigbeeClusterLibrary::ClusterIdOnOff);
        if (!onOffCluster) {
            qCWarning(dcZigbeeGenericLights()) << "Could not find on/off cluster for" << thing << "in" << node;
            info->finish(Thing::ThingErrorHardwareFailure);
            return;
        }

        connect(onOffCluster, &ZigbeeClusterOnOff::powerChanged, thing, [thing](bool power){
            qCDebug(dcZigbeeGenericLights()) << thing << "power state changed" << power;
            thing->setStateValue(onOffLightPowerStateTypeId, power);
        });

    }

    info->finish(Thing::ThingErrorNoError);
}

void IntegrationPluginZigbeeGenericLights::executeAction(ThingActionInfo *info)
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

    if (thing->thingClassId() == onOffLightThingClassId) {

        if (info->action().actionTypeId() == onOffLightAlertActionTypeId) {
            // Get the identify server cluster from the endpoint
            ZigbeeClusterIdentify *identifyCluster = endpoint->inputCluster<ZigbeeClusterIdentify>(ZigbeeClusterLibrary::ClusterIdIdentify);
            if (!identifyCluster) {
                qCWarning(dcZigbeeGenericLights()) << "Could not find identify cluster for" << thing << "in" << node;
                info->finish(Thing::ThingErrorHardwareFailure);
                return;
            }

            // Send the command trough the network
            ZigbeeClusterReply *reply =  identifyCluster->identify(2);
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

        if (info->action().actionTypeId() == onOffLightPowerActionTypeId) {
            // Get the identify server cluster from the endpoint
            ZigbeeClusterOnOff *onOffCluster = endpoint->inputCluster<ZigbeeClusterOnOff>(ZigbeeClusterLibrary::ClusterIdOnOff);
            if (!onOffCluster) {
                qCWarning(dcZigbeeGenericLights()) << "Could not find on/off cluster for" << thing << "in" << node;
                info->finish(Thing::ThingErrorHardwareFailure);
                return;
            }

            // Send the command trough the network
            bool power = info->action().param(onOffLightPowerActionPowerParamTypeId).value().toBool();
            ZigbeeClusterReply *reply = (power ? onOffCluster->commandOn() : onOffCluster->commandOff());
            connect(reply, &ZigbeeClusterReply::finished, thing, [reply, info, power, thing](){
                // Note: reply will be deleted automatically
                if (reply->error() != ZigbeeClusterReply::ErrorNoError) {
                    info->finish(Thing::ThingErrorHardwareFailure);
                } else {
                    info->finish(Thing::ThingErrorNoError);
                    thing->setStateValue(onOffLightPowerStateTypeId, power);
                }
            });
            return;
        }
    }

    info->finish(Thing::ThingErrorUnsupportedFeature);
}

void IntegrationPluginZigbeeGenericLights::thingRemoved(Thing *thing)
{
    ZigbeeNode *node = m_thingNodes.take(thing);
    if (node) {
        QUuid networkUuid = thing->paramValue(m_networkUuidParamTypeIds.value(thing->thingClassId())).toUuid();
        hardwareManager()->zigbeeResource()->removeNodeFromNetwork(networkUuid, node);
    }
}

ZigbeeNodeEndpoint *IntegrationPluginZigbeeGenericLights::findEndpoint(Thing *thing)
{
    ZigbeeNode *node = m_thingNodes.value(thing);
    if (!node) {
        qCWarning(dcZigbeeGenericLights()) << "Could not find the node for" << thing;
        return nullptr;
    }

    quint8 endpointId = thing->paramValue(m_endpointIdParamTypeIds.value(thing->thingClassId())).toUInt();
    return node->getEndpoint(endpointId);
}
