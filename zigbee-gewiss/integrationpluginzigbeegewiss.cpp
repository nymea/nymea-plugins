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

    m_ieeeAddressParamTypeIds[gewissGwa1501ThingClassId] = gewissGwa1501ThingIeeeAddressParamTypeId;

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

        createThing(thingClassId, networkUuid, node);
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
    QUuid networkUuid = thing->paramValue(m_networkUuidParamTypeIds.value(thing->thingClassId())).toUuid();
    ZigbeeAddress zigbeeAddress = ZigbeeAddress(thing->paramValue(m_ieeeAddressParamTypeIds.value(thing->thingClassId())).toString());
    ZigbeeNode *node = hardwareManager()->zigbeeResource()->claimNode(this, networkUuid, zigbeeAddress);
    if (!node) {
        qCWarning(dcZigBeeGewiss()) << "Zigbee node for" << info->thing()->name() << "not found.";
        info->finish(Thing::ThingErrorHardwareNotAvailable);
        return;
    }
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

    if (thing->thingClassId() == gewissGwa1501ThingClassId) {
        initGwa1501(node);

        ZigbeeNodeEndpoint *endpoint1 = node->getEndpoint(0x01);
        ZigbeeNodeEndpoint *endpoint2 = node->getEndpoint(0x02);

        if (!endpoint1 || !endpoint2) {
            qCWarning(dcZigBeeGewiss()) << "one ore more endpoints not found" << thing->name();
            return;
        }

        // Get battery level changes
        ZigbeeClusterPowerConfiguration *powerCluster = endpoint1->inputCluster<ZigbeeClusterPowerConfiguration>(ZigbeeClusterLibrary::ClusterIdPowerConfiguration);
        if (!powerCluster) {
            qCWarning(dcZigBeeGewiss()) << "Could not find power configuration cluster on" << thing << endpoint1;
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

        // Receive on/off commands
        ZigbeeClusterOnOff *onOffCluster1 = endpoint1->outputCluster<ZigbeeClusterOnOff>(ZigbeeClusterLibrary::ClusterIdOnOff);
        if (!onOffCluster1) {
            qCWarning(dcZigBeeGewiss()) << "Could not find on/off client cluster on" << thing << endpoint1;
        } else {
            connect(onOffCluster1, &ZigbeeClusterOnOff::commandSent, thing, [=](ZigbeeClusterOnOff::Command command){
                if (command == ZigbeeClusterOnOff::CommandOn) {
                    qCDebug(dcZigBeeGewiss()) << thing << "pressed ON";
                    emit emitEvent(Event(gewissGwa1501PressedEventTypeId, thing->id(), ParamList() << Param(gewissGwa1501PressedEventButtonNameParamTypeId, "ON")));
                } else {
                    qCWarning(dcZigBeeGewiss()) << thing << "unhandled command received" << command;
                }
            });

            connect(onOffCluster1, &ZigbeeClusterOnOff::commandOffWithEffectSent, thing, [=](ZigbeeClusterOnOff::Effect effect, quint8 effectVariant){
                qCDebug(dcZigBeeGewiss()) << thing << "OFF button pressed" << effect << effectVariant;
                emit emitEvent(Event(gewissGwa1501PressedEventTypeId, thing->id(), ParamList() << Param(gewissGwa1501PressedEventButtonNameParamTypeId, "OFF")));
            });
        }

        // Receive on/off commands
        ZigbeeClusterOnOff *onOffCluster2 = endpoint2->outputCluster<ZigbeeClusterOnOff>(ZigbeeClusterLibrary::ClusterIdOnOff);
        if (!onOffCluster2) {
            qCWarning(dcZigBeeGewiss()) << "Could not find on/off client cluster on" << thing << endpoint2;
        } else {
            connect(onOffCluster2, &ZigbeeClusterOnOff::commandSent, thing, [=](ZigbeeClusterOnOff::Command command){
                if (command == ZigbeeClusterOnOff::CommandOn) {
                    qCDebug(dcZigBeeGewiss()) << thing << "pressed ON";
                    emit emitEvent(Event(gewissGwa1501PressedEventTypeId, thing->id(), ParamList() << Param(gewissGwa1501PressedEventButtonNameParamTypeId, "ON")));
                } else {
                    qCWarning(dcZigBeeGewiss()) << thing << "unhandled command received" << command;
                }
            });

            connect(onOffCluster2, &ZigbeeClusterOnOff::commandOffWithEffectSent, thing, [=](ZigbeeClusterOnOff::Effect effect, quint8 effectVariant){
                qCDebug(dcZigBeeGewiss()) << thing << "OFF button pressed" << effect << effectVariant;
                emit emitEvent(Event(gewissGwa1501PressedEventTypeId, thing->id(), ParamList() << Param(gewissGwa1501PressedEventButtonNameParamTypeId, "OFF")));
            });
        }
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

void IntegrationPluginZigbeeGewiss::createThing(const ThingClassId &thingClassId, const QUuid &networkUuid, ZigbeeNode *node)
{
    ThingDescriptor descriptor(thingClassId);
    QString deviceClassName = supportedThings().findById(thingClassId).displayName();
    descriptor.setTitle(deviceClassName);

    ParamList params;
    params.append(Param(m_networkUuidParamTypeIds[thingClassId], networkUuid.toString()));
    params.append(Param(m_ieeeAddressParamTypeIds[thingClassId], node->extendedAddress().toString()));
    descriptor.setParams(params);
    emit autoThingsAppeared({descriptor});
}

void IntegrationPluginZigbeeGewiss::initGwa1501(ZigbeeNode *node)
{
    ZigbeeNodeEndpoint *endpoint1 = node->getEndpoint(0x01);
    //ZigbeeNodeEndpoint *endpoint2 = node->getEndpoint(0x02); TODO

    // Get the current configured bindings for this node
    ZigbeeReply *reply = node->removeAllBindings();
    connect(reply, &ZigbeeReply::finished, node, [=](){
        if (reply->error() != ZigbeeReply::ErrorNoError) {
            qCWarning(dcZigBeeGewiss()) << "Failed to remove all bindings for initialization of" << node;
        } else {
            qCDebug(dcZigBeeGewiss()) << "Removed all bindings successfully from" << node;
        }

        // Read battery, bind and configure attribute reporting for battery
        if (!endpoint1->hasInputCluster(ZigbeeClusterLibrary::ClusterIdPowerConfiguration)) {
            qCWarning(dcZigBeeGewiss()) << "Failed to initialize the power configuration cluster because the cluster could not be found" << node << endpoint1;
            return;
        }

        qCDebug(dcZigBeeGewiss()) << "Read power configuration cluster attributes" << node;
        ZigbeeClusterReply *readAttributeReply = endpoint1->getInputCluster(ZigbeeClusterLibrary::ClusterIdPowerConfiguration)->readAttributes({ZigbeeClusterPowerConfiguration::AttributeBatteryPercentageRemaining});
        connect(readAttributeReply, &ZigbeeClusterReply::finished, node, [=](){
            if (readAttributeReply->error() != ZigbeeClusterReply::ErrorNoError) {
                qCWarning(dcZigBeeGewiss()) << "Failed to read power cluster attributes" << readAttributeReply->error();
            } else {
                qCDebug(dcZigBeeGewiss()) << "Read power configuration cluster attributes finished successfully";
            }

            // Bind the cluster to the coordinator
            qCDebug(dcZigBeeGewiss()) << "Bind power configuration cluster to coordinator IEEE address";
            ZigbeeDeviceObjectReply * zdoReply = node->deviceObject()->requestBindIeeeAddress(endpoint1->endpointId(), ZigbeeClusterLibrary::ClusterIdPowerConfiguration,
                                                                                              hardwareManager()->zigbeeResource()->coordinatorAddress(node->networkUuid()), 0x01);
            connect(zdoReply, &ZigbeeDeviceObjectReply::finished, node, [=](){
                if (zdoReply->error() != ZigbeeDeviceObjectReply::ErrorNoError) {
                    qCWarning(dcZigBeeGewiss()) << "Failed to bind power cluster to coordinator" << zdoReply->error();
                } else {
                    qCDebug(dcZigBeeGewiss()) << "Bind power configuration cluster to coordinator finished successfully";
                }

                // Configure attribute rporting for battery remaining (0.5 % changes = 1)
                ZigbeeClusterLibrary::AttributeReportingConfiguration reportingConfig;
                reportingConfig.attributeId = ZigbeeClusterPowerConfiguration::AttributeBatteryPercentageRemaining;
                reportingConfig.dataType = Zigbee::Uint8;
                reportingConfig.minReportingInterval = 300;
                reportingConfig.maxReportingInterval = 2700;
                reportingConfig.reportableChange = ZigbeeDataType(static_cast<quint8>(1)).data();

                qCDebug(dcZigBeeGewiss()) << "Configure attribute reporting for power configuration cluster to coordinator";
                ZigbeeClusterReply *reportingReply = endpoint1->getInputCluster(ZigbeeClusterLibrary::ClusterIdPowerConfiguration)->configureReporting({reportingConfig});
                connect(reportingReply, &ZigbeeClusterReply::finished, this, [=] {
                    if (reportingReply->error() != ZigbeeClusterReply::ErrorNoError) {
                        qCWarning(dcZigBeeGewiss()) << "Failed to configure power cluster attribute reporting" << reportingReply->error();
                    } else {
                        qCDebug(dcZigBeeGewiss()) << "Attribute reporting configuration finished for power cluster" << ZigbeeClusterLibrary::parseAttributeReportingStatusRecords(reportingReply->responseFrame().payload);
                    }

                    // On/off cluster
                    qCDebug(dcZigBeeGewiss()) << "Bind on/off cluster to coordinator";
                    ZigbeeDeviceObjectReply * zdoReply = node->deviceObject()->requestBindGroupAddress(endpoint1->endpointId(), ZigbeeClusterLibrary::ClusterIdOnOff, 0x0000);
                    connect(zdoReply, &ZigbeeDeviceObjectReply::finished, node, [=](){
                        if (zdoReply->error() != ZigbeeDeviceObjectReply::ErrorNoError) {
                            qCWarning(dcZigBeeGewiss()) << "Failed to bind on/off cluster to coordinator" << zdoReply->error();
                        } else {
                            qCDebug(dcZigBeeGewiss()) << "Bind  cluster to coordinator finished successfully";
                        }


                        // Init Level cluster
                        qCDebug(dcZigBeeGewiss()) << "Bind power level cluster to coordinator";
                        ZigbeeDeviceObjectReply * zdoReply = node->deviceObject()->requestBindGroupAddress(endpoint1->endpointId(), ZigbeeClusterLibrary::ClusterIdLevelControl, 0x0000);
                        connect(zdoReply, &ZigbeeDeviceObjectReply::finished, node, [=](){
                            if (zdoReply->error() != ZigbeeDeviceObjectReply::ErrorNoError) {
                                qCWarning(dcZigBeeGewiss()) << "Failed to bind level cluster to coordinator" << zdoReply->error();
                            } else {
                                qCDebug(dcZigBeeGewiss()) << "Bind level cluster to coordinator finished successfully";
                            }

                            // Read final bindings
                            qCDebug(dcZigBeeGewiss()) << "Read binding table from node" << node;
                            ZigbeeReply *reply = node->readBindingTableEntries();
                            connect(reply, &ZigbeeReply::finished, node, [=](){
                                if (reply->error() != ZigbeeReply::ErrorNoError) {
                                    qCWarning(dcZigBeeGewiss()) << "Failed to read binding table from" << node;
                                } else {
                                    foreach (const ZigbeeDeviceProfile::BindingTableListRecord &binding, node->bindingTableRecords()) {
                                        qCDebug(dcZigBeeGewiss()) << node << binding;
                                    }
                                }
                            });
                        });
                    });
                });
            });
        });
    });
}
