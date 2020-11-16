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
    if (!hardwareManager()->zigbeeResource()->available()) {
        qCWarning(dcZigbeeGeneric()) << "Zigbee is not available. Not setting up" << info->thing()->name();
        info->finish(Thing::ThingErrorHardwareNotAvailable);
        return;
    }
    Thing *thing = info->thing();
    QUuid networkUuid = thing->paramValue(m_networkUuidParamTypeIds.value(thing->thingClassId())).toUuid();
    qCDebug(dcZigbeeGeneric()) << "Nework uuid:" << networkUuid;
    ZigbeeAddress zigbeeAddress = ZigbeeAddress(thing->paramValue(m_ieeeAddressParamTypeIds.value(thing->thingClassId())).toString());
    ZigbeeNode *node = hardwareManager()->zigbeeResource()->claimNode(this, networkUuid, zigbeeAddress);
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
        qCDebug(dcZigbeeGeneric()) << thing << "signal strength changed" << signalStrength << "%";
        thing->setStateValue(m_signalStrengthStateTypeIds.value(thing->thingClassId()), signalStrength);
    });

    // Type specific setup
    if (thing->thingClassId() == thermostatThingClassId) {
        // TODO: Thermostat cluster is missing
//        ZigbeeClusterThermostat *thermostatCluster = endpoint->outputCluster<ZigbeeClusterThermostat>(ZigbeeClusterLibrary::ClusterIdThermostat);
//        thermostatCluster->attribute(ZigbeeClusterLibrary::ClusterId);
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
