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
    m_ieeeAddressParamTypeIds[remoteThingClassId] = remoteThingIeeeAddressParamTypeId;
    m_ieeeAddressParamTypeIds[motionSensorThingClassId] = motionSensorThingIeeeAddressParamTypeId;
    m_ieeeAddressParamTypeIds[signalRepeaterThingClassId] = signalRepeaterThingIeeeAddressParamTypeId;

    m_networkUuidParamTypeIds[onOffSwitchThingClassId] = onOffSwitchThingNetworkUuidParamTypeId;
    m_networkUuidParamTypeIds[remoteThingClassId] = remoteThingNetworkUuidParamTypeId;
    m_networkUuidParamTypeIds[motionSensorThingClassId] = motionSensorThingNetworkUuidParamTypeId;
    m_networkUuidParamTypeIds[signalRepeaterThingClassId] = signalRepeaterThingNetworkUuidParamTypeId;

    m_endpointIdParamTypeIds[onOffSwitchThingClassId] = onOffSwitchThingEndpointIdParamTypeId;
    m_endpointIdParamTypeIds[remoteThingClassId] = remoteThingEndpointIdParamTypeId;
    m_endpointIdParamTypeIds[motionSensorThingClassId] = motionSensorThingEndpointIdParamTypeId;
    m_endpointIdParamTypeIds[signalRepeaterThingClassId] = signalRepeaterThingEndpointIdParamTypeId;

    m_connectedStateTypeIds[onOffSwitchThingClassId] = onOffSwitchConnectedStateTypeId;
    m_connectedStateTypeIds[remoteThingClassId] = remoteConnectedStateTypeId;
    m_connectedStateTypeIds[motionSensorThingClassId] = motionSensorConnectedStateTypeId;
    m_connectedStateTypeIds[signalRepeaterThingClassId] = signalRepeaterConnectedStateTypeId;

    m_signalStrengthStateTypeIds[onOffSwitchThingClassId] = onOffSwitchSignalStrengthStateTypeId;
    m_signalStrengthStateTypeIds[remoteThingClassId] = remoteSignalStrengthStateTypeId;
    m_signalStrengthStateTypeIds[motionSensorThingClassId] = motionSensorSignalStrengthStateTypeId;
    m_signalStrengthStateTypeIds[signalRepeaterThingClassId] = signalRepeaterSignalStrengthStateTypeId;

    m_versionStateTypeIds[onOffSwitchThingClassId] = onOffSwitchVersionStateTypeId;
    m_versionStateTypeIds[remoteThingClassId] = remoteVersionStateTypeId;
    m_versionStateTypeIds[motionSensorThingClassId] = motionSensorVersionStateTypeId;
    m_versionStateTypeIds[signalRepeaterThingClassId] = signalRepeaterVersionStateTypeId;
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
        if (endpoint->profile() == Zigbee::ZigbeeProfileHomeAutomation && endpoint->deviceId() == Zigbee::HomeAutomationDeviceNonColourController) {
            qCDebug(dcZigbeeTradfri()) << "Handeling TRADFRI on/off switch" << node << endpoint;
            createThing(onOffSwitchThingClassId, networkUuid, node, endpoint);
            initOnOffSwitch(node, endpoint);
            handled = true;
        }

        if (endpoint->profile() == Zigbee::ZigbeeProfileHomeAutomation && endpoint->deviceId() == Zigbee::HomeAutomationDeviceOnOffSensor) {
            qCDebug(dcZigbeeTradfri()) << "Handeling TRADFRI motion sensor" << node << endpoint;
            createThing(motionSensorThingClassId, networkUuid, node, endpoint);
            initMotionSensor(node, endpoint);
            handled = true;
        }

        if (endpoint->profile() == Zigbee::ZigbeeProfileLightLink && endpoint->deviceId() == Zigbee::LightLinkDeviceNonColourSceneController) {
            qCDebug(dcZigbeeTradfri()) << "Handeling TRADFRI remote control" << node << endpoint;
            createThing(remoteThingClassId, networkUuid, node, endpoint);
            initRemote(node, endpoint);
            handled = true;
        }

        if (endpoint->profile() == Zigbee::ZigbeeProfileHomeAutomation && endpoint->deviceId() == Zigbee::HomeAutomationDeviceRangeExtender) {
            qCDebug(dcZigbeeTradfri()) << "Handeling TRADFRI signal repeater" << node << endpoint;
            createThing(signalRepeaterThingClassId, networkUuid, node, endpoint);
            handled = true;
        }
    }

    return handled;
}

void IntegrationPluginZigbeeTradfri::handleRemoveNode(ZigbeeNode *node, const QUuid &networkUuid)
{
    Q_UNUSED(networkUuid)

    if (m_thingNodes.values().contains(node)) {
        Thing *thing = m_thingNodes.key(node);
        qCDebug(dcZigbeeTradfri()) << node << "for" << thing << "has left the network.";
        m_thingNodes.remove(thing);
        emit autoThingDisappeared(thing->id());
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

    // Thing specific setup
    if (thing->thingClassId() == onOffSwitchThingClassId) {
        // Get battery level changes
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

        // Receive level control commands
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
    }

    if (thing->thingClassId() == motionSensorThingClassId) {
        // Get battery level changes
        ZigbeeClusterPowerConfiguration *powerCluster = endpoint->inputCluster<ZigbeeClusterPowerConfiguration>(ZigbeeClusterLibrary::ClusterIdPowerConfiguration);
        if (!powerCluster) {
            qCWarning(dcZigbeeTradfri()) << "Could not find power configuration cluster on" << thing << endpoint;
        } else {
            // Only set the initial state if the attribute already exists
            if (powerCluster->hasAttribute(ZigbeeClusterPowerConfiguration::AttributeBatteryPercentageRemaining)) {
                thing->setStateValue(motionSensorBatteryLevelStateTypeId, powerCluster->batteryPercentage());
                thing->setStateValue(motionSensorBatteryCriticalStateTypeId, (powerCluster->batteryPercentage() < 10.0));
            }

            connect(powerCluster, &ZigbeeClusterPowerConfiguration::batteryPercentageChanged, thing, [=](double percentage){
                qCDebug(dcZigbeeTradfri()) << "Battery percentage changed" << percentage << "%" << thing;
                thing->setStateValue(motionSensorBatteryLevelStateTypeId, percentage);
                thing->setStateValue(motionSensorBatteryCriticalStateTypeId, (percentage < 10.0));
            });
        }

        // Create plugintimer if required
        if (!m_presenceTimer) {
            m_presenceTimer = hardwareManager()->pluginTimerManager()->registerTimer(1);
        }
        connect(m_presenceTimer, &PluginTimer::timeout, thing, [thing](){
            if (thing->stateValue(motionSensorIsPresentStateTypeId).toBool()) {
                int timeout = thing->setting(motionSensorSettingsTimeoutParamTypeId).toInt();
                QDateTime lastSeenTime = QDateTime::fromMSecsSinceEpoch(thing->stateValue(motionSensorLastSeenTimeStateTypeId).toULongLong() * 1000);
                if (lastSeenTime.addSecs(timeout) < QDateTime::currentDateTime()) {
                    thing->setStateValue(motionSensorIsPresentStateTypeId, false);
                }
            }
        });

        // Receive on/off commands
        ZigbeeClusterOnOff *onOffCluster = endpoint->outputCluster<ZigbeeClusterOnOff>(ZigbeeClusterLibrary::ClusterIdOnOff);
        if (!onOffCluster) {
            qCWarning(dcZigbeeTradfri()) << "Could not find on/off client cluster on" << thing << endpoint;
        } else {
            connect(onOffCluster, &ZigbeeClusterOnOff::commandOnWithTimedOffSent, thing, [=](bool acceptOnlyWhenOn, quint16 onTime, quint16 offTime){
                qCDebug(dcZigbeeTradfri()) << thing << "command received: Accept only when on:" << acceptOnlyWhenOn << "On time:" << onTime / 10 << "s" << "Off time:" << offTime / 10 << "s";
                thing->setStateValue(motionSensorLastSeenTimeStateTypeId, QDateTime::currentDateTime().toMSecsSinceEpoch() / 1000);
                thing->setStateValue(motionSensorIsPresentStateTypeId, true);
                m_presenceTimer->start();
            });
        }
    }

    if (thing->thingClassId() == remoteThingClassId) {
        // Get battery level changes
        ZigbeeClusterPowerConfiguration *powerCluster = endpoint->inputCluster<ZigbeeClusterPowerConfiguration>(ZigbeeClusterLibrary::ClusterIdPowerConfiguration);
        if (!powerCluster) {
            qCWarning(dcZigbeeTradfri()) << "Could not find power configuration cluster on" << thing << endpoint;
        } else {
            // Only set the initial state if the attribute already exists
            if (powerCluster->hasAttribute(ZigbeeClusterPowerConfiguration::AttributeBatteryPercentageRemaining)) {
                thing->setStateValue(remoteBatteryLevelStateTypeId, powerCluster->batteryPercentage());
                thing->setStateValue(remoteBatteryCriticalStateTypeId, (powerCluster->batteryPercentage() < 10.0));
            }

            connect(powerCluster, &ZigbeeClusterPowerConfiguration::batteryPercentageChanged, thing, [=](double percentage){
                qCDebug(dcZigbeeTradfri()) << "Battery percentage changed" << percentage << "%" << thing;
                thing->setStateValue(remoteBatteryLevelStateTypeId, percentage);
                thing->setStateValue(remoteBatteryCriticalStateTypeId, (percentage < 10.0));
            });
        }

        // Receive on/off commands
        ZigbeeClusterOnOff *onOffCluster = endpoint->outputCluster<ZigbeeClusterOnOff>(ZigbeeClusterLibrary::ClusterIdOnOff);
        if (!onOffCluster) {
            qCWarning(dcZigbeeTradfri()) << "Could not find on/off client cluster on" << thing << endpoint;
        } else {
            connect(onOffCluster, &ZigbeeClusterOnOff::commandSent, thing, [=](ZigbeeClusterOnOff::Command command){
                qCDebug(dcZigbeeTradfri()) << thing << "button pressed" << command;
                if (command == ZigbeeClusterOnOff::CommandToggle) {
                    qCDebug(dcZigbeeTradfri()) << thing << "pressed power";
                    emit emitEvent(Event(remotePressedEventTypeId, thing->id(), ParamList() << Param(remotePressedEventButtonNameParamTypeId, "Power")));
                }
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

void IntegrationPluginZigbeeTradfri::createThing(const ThingClassId &thingClassId, const QUuid &networkUuid, ZigbeeNode *node, ZigbeeNodeEndpoint *endpoint)
{
    ThingDescriptor descriptor(thingClassId);
    QString deviceClassName = supportedThings().findById(thingClassId).displayName();
    descriptor.setTitle(deviceClassName);

    ParamList params;
    params.append(Param(m_networkUuidParamTypeIds[thingClassId], networkUuid.toString()));
    params.append(Param(m_ieeeAddressParamTypeIds[thingClassId], node->extendedAddress().toString()));
    params.append(Param(m_endpointIdParamTypeIds[thingClassId], endpoint->endpointId()));
    descriptor.setParams(params);
    emit autoThingsAppeared({descriptor});
}

void IntegrationPluginZigbeeTradfri::initOnOffSwitch(ZigbeeNode *node, ZigbeeNodeEndpoint *endpoint)
{
    // Get the current configured bindings for this node
    ZigbeeReply *reply = node->removeAllBindings();
    connect(reply, &ZigbeeReply::finished, node, [=](){
        if (reply->error() != ZigbeeReply::ErrorNoError) {
            qCWarning(dcZigbeeTradfri()) << "Failed to remove all bindings for initialization of" << node;
        } else {
            qCDebug(dcZigbeeTradfri()) << "Removed all bindings successfully from" << node;
        }

        // Read battery, bind and configure attribute reporting for battery
        if (!endpoint->hasInputCluster(ZigbeeClusterLibrary::ClusterIdPowerConfiguration)) {
            qCWarning(dcZigbeeTradfri()) << "Failed to initialize the power configuration cluster because the cluster could not be found" << node << endpoint;
            return;
        }

        qCDebug(dcZigbeeTradfri()) << "Read power configuration cluster attributes" << node;
        ZigbeeClusterReply *readAttributeReply = endpoint->getInputCluster(ZigbeeClusterLibrary::ClusterIdPowerConfiguration)->readAttributes({ZigbeeClusterPowerConfiguration::AttributeBatteryPercentageRemaining});
        connect(readAttributeReply, &ZigbeeClusterReply::finished, node, [=](){
            if (readAttributeReply->error() != ZigbeeClusterReply::ErrorNoError) {
                qCWarning(dcZigbeeTradfri()) << "Failed to read power cluster attributes" << readAttributeReply->error();
            } else {
                qCDebug(dcZigbeeTradfri()) << "Read power configuration cluster attributes finished successfully";
            }


            // Bind the cluster to the coordinator
            qCDebug(dcZigbeeTradfri()) << "Bind power configuration cluster to coordinator IEEE address";
            ZigbeeDeviceObjectReply * zdoReply = node->deviceObject()->requestBindIeeeAddress(endpoint->endpointId(), ZigbeeClusterLibrary::ClusterIdPowerConfiguration,
                                                                                              hardwareManager()->zigbeeResource()->coordinatorAddress(node->networkUuid()), 0x01);
            connect(zdoReply, &ZigbeeDeviceObjectReply::finished, node, [=](){
                if (zdoReply->error() != ZigbeeDeviceObjectReply::ErrorNoError) {
                    qCWarning(dcZigbeeTradfri()) << "Failed to bind power cluster to coordinator" << zdoReply->error();
                } else {
                    qCDebug(dcZigbeeTradfri()) << "Bind power configuration cluster to coordinator finished successfully";
                }

                // Configure attribute rporting for battery remaining (0.5 % changes = 1)
                ZigbeeClusterLibrary::AttributeReportingConfiguration reportingConfig;
                reportingConfig.attributeId = ZigbeeClusterPowerConfiguration::AttributeBatteryPercentageRemaining;
                reportingConfig.dataType = Zigbee::Uint8;
                reportingConfig.minReportingInterval = 300;
                reportingConfig.maxReportingInterval = 2700;
                reportingConfig.reportableChange = ZigbeeDataType(static_cast<quint8>(1)).data();

                qCDebug(dcZigbeeTradfri()) << "Configure attribute reporting for power configuration cluster to coordinator";
                ZigbeeClusterReply *reportingReply = endpoint->getInputCluster(ZigbeeClusterLibrary::ClusterIdPowerConfiguration)->configureReporting({reportingConfig});
                connect(reportingReply, &ZigbeeClusterReply::finished, this, [=](){
                    if (reportingReply->error() != ZigbeeClusterReply::ErrorNoError) {
                        qCWarning(dcZigbeeTradfri()) << "Failed to configure power cluster attribute reporting" << reportingReply->error();
                    } else {
                        qCDebug(dcZigbeeTradfri()) << "Attribute reporting configuration finished for power cluster" << ZigbeeClusterLibrary::parseAttributeReportingStatusRecords(reportingReply->responseFrame().payload);
                    }


                    qCDebug(dcZigbeeTradfri()) << "Bind power configuration cluster to coordinator";
                    ZigbeeDeviceObjectReply * zdoReply = node->deviceObject()->requestBindShortAddress(endpoint->endpointId(), ZigbeeClusterLibrary::ClusterIdOnOff, 0x0000);
                    connect(zdoReply, &ZigbeeDeviceObjectReply::finished, node, [=](){
                        if (zdoReply->error() != ZigbeeDeviceObjectReply::ErrorNoError) {
                            qCWarning(dcZigbeeTradfri()) << "Failed to bind on/off cluster to coordinator" << zdoReply->error();
                        } else {
                            qCDebug(dcZigbeeTradfri()) << "Bind on/off cluster to coordinator finished successfully";
                        }

                        qCDebug(dcZigbeeTradfri()) << "Bind power level cluster to coordinator";
                        ZigbeeDeviceObjectReply * zdoReply = node->deviceObject()->requestBindShortAddress(endpoint->endpointId(), ZigbeeClusterLibrary::ClusterIdLevelControl, 0x0000);
                        connect(zdoReply, &ZigbeeDeviceObjectReply::finished, node, [=](){
                            if (zdoReply->error() != ZigbeeDeviceObjectReply::ErrorNoError) {
                                qCWarning(dcZigbeeTradfri()) << "Failed to bind level cluster to coordinator" << zdoReply->error();
                            } else {
                                qCDebug(dcZigbeeTradfri()) << "Bind level cluster to coordinator finished successfully";
                            }

                            qCDebug(dcZigbeeTradfri()) << "Read final binding table from node" << node;
                            ZigbeeReply *reply = node->readBindingTableEntries();
                            connect(reply, &ZigbeeReply::finished, node, [=](){
                               if (reply->error() != ZigbeeReply::ErrorNoError) {
                                   qCWarning(dcZigbeeTradfri()) << "Failed to read binding table from" << node;
                               } else {
                                   foreach (const ZigbeeDeviceProfile::BindingTableListRecord &binding, node->bindingTableRecords()) {
                                       qCDebug(dcZigbeeTradfri()) << node << binding;
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

void IntegrationPluginZigbeeTradfri::initRemote(ZigbeeNode *node, ZigbeeNodeEndpoint *endpoint)
{
    // Get the current configured bindings for this node
    ZigbeeReply *reply = node->removeAllBindings();
    connect(reply, &ZigbeeReply::finished, node, [=](){
        if (reply->error() != ZigbeeReply::ErrorNoError) {
            qCWarning(dcZigbeeTradfri()) << "Failed to remove all bindings for initialization of" << node;
        } else {
            qCDebug(dcZigbeeTradfri()) << "Removed all bindings successfully from" << node;
        }

        // Read battery, bind and configure attribute reporting for battery

        if (!endpoint->hasInputCluster(ZigbeeClusterLibrary::ClusterIdPowerConfiguration)) {
            qCWarning(dcZigbeeTradfri()) << "Failed to initialize the power configuration cluster because the cluster could not be found" << node << endpoint;
            return;
        }

        qCDebug(dcZigbeeTradfri()) << "Read power configuration cluster attributes" << node;
        ZigbeeClusterReply *readAttributeReply = endpoint->getInputCluster(ZigbeeClusterLibrary::ClusterIdPowerConfiguration)->readAttributes({ZigbeeClusterPowerConfiguration::AttributeBatteryPercentageRemaining});
        connect(readAttributeReply, &ZigbeeClusterReply::finished, node, [=](){
            if (readAttributeReply->error() != ZigbeeClusterReply::ErrorNoError) {
                qCWarning(dcZigbeeTradfri()) << "Failed to read power cluster attributes" << readAttributeReply->error();
            } else {
                qCDebug(dcZigbeeTradfri()) << "Read power configuration cluster attributes finished successfully";
            }


            // Bind the cluster to the coordinator
            qCDebug(dcZigbeeTradfri()) << "Bind power configuration cluster to coordinator IEEE address";
            ZigbeeDeviceObjectReply * zdoReply = node->deviceObject()->requestBindIeeeAddress(endpoint->endpointId(), ZigbeeClusterLibrary::ClusterIdPowerConfiguration,
                                                                                              hardwareManager()->zigbeeResource()->coordinatorAddress(node->networkUuid()), 0x01);
            connect(zdoReply, &ZigbeeDeviceObjectReply::finished, node, [=](){
                if (zdoReply->error() != ZigbeeDeviceObjectReply::ErrorNoError) {
                    qCWarning(dcZigbeeTradfri()) << "Failed to bind power cluster to coordinator" << zdoReply->error();
                } else {
                    qCDebug(dcZigbeeTradfri()) << "Bind power configuration cluster to coordinator finished successfully";
                }

                // Configure attribute rporting for battery remaining (0.5 % changes = 1)
                ZigbeeClusterLibrary::AttributeReportingConfiguration reportingConfig;
                reportingConfig.attributeId = ZigbeeClusterPowerConfiguration::AttributeBatteryPercentageRemaining;
                reportingConfig.dataType = Zigbee::Uint8;
                reportingConfig.minReportingInterval = 300;
                reportingConfig.maxReportingInterval = 2700;
                reportingConfig.reportableChange = ZigbeeDataType(static_cast<quint8>(1)).data();

                qCDebug(dcZigbeeTradfri()) << "Configure attribute reporting for power configuration cluster to coordinator";
                ZigbeeClusterReply *reportingReply = endpoint->getInputCluster(ZigbeeClusterLibrary::ClusterIdPowerConfiguration)->configureReporting({reportingConfig});
                connect(reportingReply, &ZigbeeClusterReply::finished, this, [=](){
                    if (reportingReply->error() != ZigbeeClusterReply::ErrorNoError) {
                        qCWarning(dcZigbeeTradfri()) << "Failed to configure power cluster attribute reporting" << reportingReply->error();
                    } else {
                        qCDebug(dcZigbeeTradfri()) << "Attribute reporting configuration finished for power cluster" << ZigbeeClusterLibrary::parseAttributeReportingStatusRecords(reportingReply->responseFrame().payload);
                    }


                    qCDebug(dcZigbeeTradfri()) << "Bind power configuration cluster to coordinator";
                    ZigbeeDeviceObjectReply * zdoReply = node->deviceObject()->requestBindShortAddress(endpoint->endpointId(), ZigbeeClusterLibrary::ClusterIdOnOff, 0x0000);
                    connect(zdoReply, &ZigbeeDeviceObjectReply::finished, node, [=](){
                        if (zdoReply->error() != ZigbeeDeviceObjectReply::ErrorNoError) {
                            qCWarning(dcZigbeeTradfri()) << "Failed to bind on/off cluster to coordinator" << zdoReply->error();
                        } else {
                            qCDebug(dcZigbeeTradfri()) << "Bind on/off cluster to coordinator finished successfully";
                        }

                        qCDebug(dcZigbeeTradfri()) << "Bind power level cluster to coordinator";
                        ZigbeeDeviceObjectReply * zdoReply = node->deviceObject()->requestBindShortAddress(endpoint->endpointId(), ZigbeeClusterLibrary::ClusterIdLevelControl, 0x0000);
                        connect(zdoReply, &ZigbeeDeviceObjectReply::finished, node, [=](){
                            if (zdoReply->error() != ZigbeeDeviceObjectReply::ErrorNoError) {
                                qCWarning(dcZigbeeTradfri()) << "Failed to bind level cluster to coordinator" << zdoReply->error();
                            } else {
                                qCDebug(dcZigbeeTradfri()) << "Bind level cluster to coordinator finished successfully";
                            }

                            qCDebug(dcZigbeeTradfri()) << "Read binding table from node" << node;
                            ZigbeeReply *reply = node->readBindingTableEntries();
                            connect(reply, &ZigbeeReply::finished, node, [=](){
                               if (reply->error() != ZigbeeReply::ErrorNoError) {
                                   qCWarning(dcZigbeeTradfri()) << "Failed to read binding table from" << node;
                               } else {
                                   foreach (const ZigbeeDeviceProfile::BindingTableListRecord &binding, node->bindingTableRecords()) {
                                       qCDebug(dcZigbeeTradfri()) << node << binding;
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

void IntegrationPluginZigbeeTradfri::initMotionSensor(ZigbeeNode *node, ZigbeeNodeEndpoint *endpoint)
{
    // Get the current configured bindings for this node
    ZigbeeReply *reply = node->removeAllBindings();
    connect(reply, &ZigbeeReply::finished, node, [=](){
        if (reply->error() != ZigbeeReply::ErrorNoError) {
            qCWarning(dcZigbeeTradfri()) << "Failed to remove all bindings for initialization of" << node;
        } else {
            qCDebug(dcZigbeeTradfri()) << "Removed all bindings successfully from" << node;
        }

        // Read battery, bind and configure attribute reporting for battery

        if (!endpoint->hasInputCluster(ZigbeeClusterLibrary::ClusterIdPowerConfiguration)) {
            qCWarning(dcZigbeeTradfri()) << "Failed to initialize the power configuration cluster because the cluster could not be found" << node << endpoint;
            return;
        }

        qCDebug(dcZigbeeTradfri()) << "Read power configuration cluster attributes" << node;
        ZigbeeClusterReply *readAttributeReply = endpoint->getInputCluster(ZigbeeClusterLibrary::ClusterIdPowerConfiguration)->readAttributes({ZigbeeClusterPowerConfiguration::AttributeBatteryPercentageRemaining});
        connect(readAttributeReply, &ZigbeeClusterReply::finished, node, [=](){
            if (readAttributeReply->error() != ZigbeeClusterReply::ErrorNoError) {
                qCWarning(dcZigbeeTradfri()) << "Failed to read power cluster attributes" << readAttributeReply->error();
            } else {
                qCDebug(dcZigbeeTradfri()) << "Read power configuration cluster attributes finished successfully";
            }

            // Bind the cluster to the coordinator
            qCDebug(dcZigbeeTradfri()) << "Bind power configuration cluster to coordinator IEEE address";
            ZigbeeDeviceObjectReply * zdoReply = node->deviceObject()->requestBindIeeeAddress(endpoint->endpointId(), ZigbeeClusterLibrary::ClusterIdPowerConfiguration,
                                                                                              hardwareManager()->zigbeeResource()->coordinatorAddress(node->networkUuid()), 0x01);
            connect(zdoReply, &ZigbeeDeviceObjectReply::finished, node, [=](){
                if (zdoReply->error() != ZigbeeDeviceObjectReply::ErrorNoError) {
                    qCWarning(dcZigbeeTradfri()) << "Failed to bind power cluster to coordinator" << zdoReply->error();
                } else {
                    qCDebug(dcZigbeeTradfri()) << "Bind power configuration cluster to coordinator finished successfully";
                }

                // Configure attribute rporting for battery remaining (0.5 % changes = 1)
                ZigbeeClusterLibrary::AttributeReportingConfiguration reportingConfig;
                reportingConfig.attributeId = ZigbeeClusterPowerConfiguration::AttributeBatteryPercentageRemaining;
                reportingConfig.dataType = Zigbee::Uint8;
                reportingConfig.minReportingInterval = 300;
                reportingConfig.maxReportingInterval = 2700;
                reportingConfig.reportableChange = ZigbeeDataType(static_cast<quint8>(1)).data();

                qCDebug(dcZigbeeTradfri()) << "Configure attribute reporting for power configuration cluster to coordinator";
                ZigbeeClusterReply *reportingReply = endpoint->getInputCluster(ZigbeeClusterLibrary::ClusterIdPowerConfiguration)->configureReporting({reportingConfig});
                connect(reportingReply, &ZigbeeClusterReply::finished, this, [=](){
                    if (reportingReply->error() != ZigbeeClusterReply::ErrorNoError) {
                        qCWarning(dcZigbeeTradfri()) << "Failed to configure power cluster attribute reporting" << reportingReply->error();
                    } else {
                        qCDebug(dcZigbeeTradfri()) << "Attribute reporting configuration finished for power cluster" << ZigbeeClusterLibrary::parseAttributeReportingStatusRecords(reportingReply->responseFrame().payload);
                    }

                    qCDebug(dcZigbeeTradfri()) << "Bind on/off cluster to coordinator";
                    ZigbeeDeviceObjectReply * zdoReply = node->deviceObject()->requestBindShortAddress(endpoint->endpointId(), ZigbeeClusterLibrary::ClusterIdOnOff, 0x0000);
                    connect(zdoReply, &ZigbeeDeviceObjectReply::finished, node, [=](){
                        if (zdoReply->error() != ZigbeeDeviceObjectReply::ErrorNoError) {
                            qCWarning(dcZigbeeTradfri()) << "Failed to bind on/off cluster to coordinator" << zdoReply->error();
                            return;
                        }
                        qCDebug(dcZigbeeTradfri()) << "Bind on/off cluster to coordinator finished successfully";
                    });
                });
            });
        });
    });
}

