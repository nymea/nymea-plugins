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

#include "integrationpluginzigbeephilipshue.h"
#include "plugininfo.h"
#include "hardware/zigbee/zigbeehardwareresource.h"

IntegrationPluginZigbeePhilipsHue::IntegrationPluginZigbeePhilipsHue()
{
    m_ieeeAddressParamTypeIds[dimmerSwitchThingClassId] = dimmerSwitchThingIeeeAddressParamTypeId;

    m_networkUuidParamTypeIds[dimmerSwitchThingClassId] = dimmerSwitchThingNetworkUuidParamTypeId;

    m_connectedStateTypeIds[dimmerSwitchThingClassId] = dimmerSwitchConnectedStateTypeId;

    m_signalStrengthStateTypeIds[dimmerSwitchThingClassId] = dimmerSwitchSignalStrengthStateTypeId;

    m_versionStateTypeIds[dimmerSwitchThingClassId] = dimmerSwitchVersionStateTypeId;

}

QString IntegrationPluginZigbeePhilipsHue::name() const
{
    return "Philips Hue";
}

bool IntegrationPluginZigbeePhilipsHue::handleNode(ZigbeeNode *node, const QUuid &networkUuid)
{
    // Make sure this is from Philips 0x100b
    if (node->nodeDescriptor().manufacturerCode != Zigbee::Philips)
        return false;

    bool handled = false;

    if (node->endpoints().count() == 2 && node->hasEndpoint(0x01) && node->hasEndpoint(0x02)) {
        ZigbeeNodeEndpoint *endpointOne = node->getEndpoint(0x01);
        ZigbeeNodeEndpoint *endpoinTwo = node->getEndpoint(0x02);

        // Dimmer switch
        if (endpointOne->profile() == Zigbee::ZigbeeProfileLightLink &&
                endpointOne->deviceId() == Zigbee::LightLinkDeviceNonColourSceneController
                && endpoinTwo->profile() == Zigbee::ZigbeeProfileHomeAutomation &&
                endpoinTwo->deviceId() == Zigbee::HomeAutomationDeviceSimpleSensor) {

            qCDebug(dcZigbeePhilipsHue()) << "Handeling Hue dimmer switch" << node << endpointOne << endpoinTwo;
            createThing(dimmerSwitchThingClassId, networkUuid, node);
            initDimmerSwitch(node);
            return true;
        }
    }

    return handled;
}

void IntegrationPluginZigbeePhilipsHue::handleRemoveNode(ZigbeeNode *node, const QUuid &networkUuid)
{
    Q_UNUSED(networkUuid)

    if (m_thingNodes.values().contains(node)) {
        Thing *thing = m_thingNodes.key(node);
        qCDebug(dcZigbeePhilipsHue()) << node << "for" << thing << "has left the network.";
        m_thingNodes.remove(thing);
        emit autoThingDisappeared(thing->id());
    }
}

void IntegrationPluginZigbeePhilipsHue::init()
{
    hardwareManager()->zigbeeResource()->registerHandler(this, ZigbeeHardwareResource::HandlerTypeVendor);
}

void IntegrationPluginZigbeePhilipsHue::setupThing(ThingSetupInfo *info)
{
    Thing *thing = info->thing();
    QUuid networkUuid = thing->paramValue(m_networkUuidParamTypeIds.value(thing->thingClassId())).toUuid();
    ZigbeeAddress zigbeeAddress = ZigbeeAddress(thing->paramValue(m_ieeeAddressParamTypeIds.value(thing->thingClassId())).toString());
    ZigbeeNode *node = hardwareManager()->zigbeeResource()->claimNode(this, networkUuid, zigbeeAddress);
    if (!node) {
        qCWarning(dcZigbeePhilipsHue()) << "Zigbee node for" << info->thing()->name() << "not found.";
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
        qCDebug(dcZigbeePhilipsHue()) << thing << "signal strength changed" << signalStrength << "%";
        thing->setStateValue(m_signalStrengthStateTypeIds.value(thing->thingClassId()), signalStrength);
    });


    // Thing specific setup
    if (thing->thingClassId() == dimmerSwitchThingClassId) {
        ZigbeeNodeEndpoint *endpointZll = node->getEndpoint(0x01);
        ZigbeeNodeEndpoint *endpointHa = node->getEndpoint(0x02);

        // Set the version
        thing->setStateValue(m_versionStateTypeIds.value(thing->thingClassId()), endpointZll->softwareBuildId());


        // Receive on/off commands
        ZigbeeClusterOnOff *onOffCluster = endpointZll->outputCluster<ZigbeeClusterOnOff>(ZigbeeClusterLibrary::ClusterIdOnOff);
        if (!onOffCluster) {
            qCWarning(dcZigbeePhilipsHue()) << "Could not find on/off client cluster on" << thing << endpointZll;
        } else {
            connect(onOffCluster, &ZigbeeClusterOnOff::commandSent, thing, [=](ZigbeeClusterOnOff::Command command){
                if (command == ZigbeeClusterOnOff::CommandOn) {
                    qCDebug(dcZigbeePhilipsHue()) << thing << "pressed ON";
                    emit emitEvent(Event(dimmerSwitchPressedEventTypeId, thing->id(), ParamList() << Param(dimmerSwitchPressedEventButtonNameParamTypeId, "ON")));
                } else {
                    qCWarning(dcZigbeePhilipsHue()) << thing << "unhandled command received" << command;
                }
            });

            connect(onOffCluster, &ZigbeeClusterOnOff::commandOffWithEffectSent, thing, [=](ZigbeeClusterOnOff::Effect effect, quint8 effectVariant){
                qCDebug(dcZigbeePhilipsHue()) << thing << "OFF button pressed" << effect << effectVariant;
                emit emitEvent(Event(dimmerSwitchPressedEventTypeId, thing->id(), ParamList() << Param(dimmerSwitchPressedEventButtonNameParamTypeId, "OFF")));
            });
        }

        // Receive level control commands
        ZigbeeClusterLevelControl *levelCluster = endpointZll->outputCluster<ZigbeeClusterLevelControl>(ZigbeeClusterLibrary::ClusterIdLevelControl);
        if (!levelCluster) {
            qCWarning(dcZigbeePhilipsHue()) << "Could not find level client cluster on" << thing << endpointZll;
        } else {
            connect(levelCluster, &ZigbeeClusterLevelControl::commandStepSent, thing, [=](ZigbeeClusterLevelControl::FadeMode fadeMode, quint8 stepSize, quint16 transitionTime){
                qCDebug(dcZigbeePhilipsHue()) << thing << "level button pressed" << fadeMode << stepSize << transitionTime;
                switch (fadeMode) {
                case ZigbeeClusterLevelControl::FadeModeUp:
                    qCDebug(dcZigbeePhilipsHue()) << thing << "DIM UP pressed";
                    emit emitEvent(Event(dimmerSwitchPressedEventTypeId, thing->id(), ParamList() << Param(dimmerSwitchPressedEventButtonNameParamTypeId, "DIM UP")));
                    break;
                case ZigbeeClusterLevelControl::FadeModeDown:
                    qCDebug(dcZigbeePhilipsHue()) << thing << "DIM DOWN pressed";
                    emit emitEvent(Event(dimmerSwitchPressedEventTypeId, thing->id(), ParamList() << Param(dimmerSwitchPressedEventButtonNameParamTypeId, "DIM DOWN")));
                    break;
                }
            });
        }

        // Get battery level changes
        ZigbeeClusterPowerConfiguration *powerCluster = endpointHa->inputCluster<ZigbeeClusterPowerConfiguration>(ZigbeeClusterLibrary::ClusterIdPowerConfiguration);
        if (!powerCluster) {
            qCWarning(dcZigbeePhilipsHue()) << "Could not find power configuration cluster on" << thing << endpointHa;
        } else {
            // Only set the initial state if the attribute already exists
            if (powerCluster->hasAttribute(ZigbeeClusterPowerConfiguration::AttributeBatteryPercentageRemaining)) {
                thing->setStateValue(dimmerSwitchBatteryLevelStateTypeId, powerCluster->batteryPercentage());
                thing->setStateValue(dimmerSwitchBatteryCriticalStateTypeId, (powerCluster->batteryPercentage() < 10.0));
            }

            connect(powerCluster, &ZigbeeClusterPowerConfiguration::batteryPercentageChanged, thing, [=](double percentage){
                qCDebug(dcZigbeePhilipsHue()) << "Battery percentage changed" << percentage << "%" << thing;
                thing->setStateValue(dimmerSwitchBatteryLevelStateTypeId, percentage);
                thing->setStateValue(dimmerSwitchBatteryCriticalStateTypeId, (percentage < 10.0));
            });
        }

    }

    info->finish(Thing::ThingErrorNoError);
}

void IntegrationPluginZigbeePhilipsHue::executeAction(ThingActionInfo *info)
{
    info->finish(Thing::ThingErrorUnsupportedFeature);
}

void IntegrationPluginZigbeePhilipsHue::thingRemoved(Thing *thing)
{
    ZigbeeNode *node = m_thingNodes.take(thing);
    if (node) {
        QUuid networkUuid = thing->paramValue(m_networkUuidParamTypeIds.value(thing->thingClassId())).toUuid();
        hardwareManager()->zigbeeResource()->removeNodeFromNetwork(networkUuid, node);
    }
}

void IntegrationPluginZigbeePhilipsHue::createThing(const ThingClassId &thingClassId, const QUuid &networkUuid, ZigbeeNode *node)
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

void IntegrationPluginZigbeePhilipsHue::initDimmerSwitch(ZigbeeNode *node)
{
    ZigbeeNodeEndpoint *endpointZll = node->getEndpoint(0x01);
    ZigbeeNodeEndpoint *endpointHa = node->getEndpoint(0x02);

    // Get the current configured bindings for this node
    ZigbeeReply *reply = node->removeAllBindings();
    connect(reply, &ZigbeeReply::finished, node, [=](){
        if (reply->error() != ZigbeeReply::ErrorNoError) {
            qCWarning(dcZigbeePhilipsHue()) << "Failed to remove all bindings for initialization of" << node;
        }

        // Read battery, bind and configure attribute reporting for battery
        if (!endpointHa->hasInputCluster(ZigbeeClusterLibrary::ClusterIdPowerConfiguration)) {
            qCWarning(dcZigbeePhilipsHue()) << "Failed to initialize the power configuration cluster because the cluster could not be found" << node << endpointHa;
            return;
        }

        qCDebug(dcZigbeePhilipsHue()) << "Read power configuration cluster attributes" << node;
        ZigbeeClusterReply *readAttributeReply = endpointHa->getInputCluster(ZigbeeClusterLibrary::ClusterIdPowerConfiguration)->readAttributes({ZigbeeClusterPowerConfiguration::AttributeBatteryPercentageRemaining});
        connect(readAttributeReply, &ZigbeeClusterReply::finished, node, [=](){
            if (readAttributeReply->error() != ZigbeeClusterReply::ErrorNoError) {
                qCWarning(dcZigbeePhilipsHue()) << "Failed to read power cluster attributes" << readAttributeReply->error();
                //emit nodeInitialized(node);
                return;
            }

            qCDebug(dcZigbeePhilipsHue()) << "Read power configuration cluster attributes finished successfully";

            // Bind the cluster to the coordinator
            qCDebug(dcZigbeePhilipsHue()) << "Bind power configuration cluster to coordinator IEEE address";
            ZigbeeDeviceObjectReply * zdoReply = node->deviceObject()->requestBindIeeeAddress(endpointHa->endpointId(), ZigbeeClusterLibrary::ClusterIdPowerConfiguration,
                                                                                              hardwareManager()->zigbeeResource()->coordinatorAddress(node->networkUuid()), 0x01);
            connect(zdoReply, &ZigbeeDeviceObjectReply::finished, node, [=](){
                if (zdoReply->error() != ZigbeeDeviceObjectReply::ErrorNoError) {
                    qCWarning(dcZigbeePhilipsHue()) << "Failed to bind power cluster to coordinator" << zdoReply->error();
                    return;
                }
                qCDebug(dcZigbeePhilipsHue()) << "Bind power configuration cluster to coordinator finished successfully";

                // Configure attribute rporting for battery remaining (0.5 % changes = 1)
                ZigbeeClusterLibrary::AttributeReportingConfiguration reportingConfig;
                reportingConfig.attributeId = ZigbeeClusterPowerConfiguration::AttributeBatteryPercentageRemaining;
                reportingConfig.dataType = Zigbee::Uint8;
                reportingConfig.minReportingInterval = 300;
                reportingConfig.maxReportingInterval = 2700;
                reportingConfig.reportableChange = ZigbeeDataType(static_cast<quint8>(1)).data();

                qCDebug(dcZigbeePhilipsHue()) << "Configure attribute reporting for power configuration cluster to coordinator";
                ZigbeeClusterReply *reportingReply = endpointHa->getInputCluster(ZigbeeClusterLibrary::ClusterIdPowerConfiguration)->configureReporting({reportingConfig});
                connect(reportingReply, &ZigbeeClusterReply::finished, this, [=](){
                    if (reportingReply->error() != ZigbeeClusterReply::ErrorNoError) {
                        qCWarning(dcZigbeePhilipsHue()) << "Failed to configure power cluster attribute reporting" << reportingReply->error();
                        return;
                    }

                    qCDebug(dcZigbeePhilipsHue()) << "Attribute reporting configuration finished for power cluster" << ZigbeeClusterLibrary::parseAttributeReportingStatusRecords(reportingReply->responseFrame().payload);

                    qCDebug(dcZigbeePhilipsHue()) << "Bind on/off cluster to coordinator";
                    ZigbeeDeviceObjectReply * zdoReply = node->deviceObject()->requestBindShortAddress(endpointZll->endpointId(), ZigbeeClusterLibrary::ClusterIdOnOff, 0x0000);
                    connect(zdoReply, &ZigbeeDeviceObjectReply::finished, node, [=](){
                        if (zdoReply->error() != ZigbeeDeviceObjectReply::ErrorNoError) {
                            qCWarning(dcZigbeePhilipsHue()) << "Failed to bind on/off cluster to coordinator" << zdoReply->error();
                            return;
                        }
                        qCDebug(dcZigbeePhilipsHue()) << "Bind on/off cluster to coordinator finished successfully";

                        qCDebug(dcZigbeePhilipsHue()) << "Bind power level cluster to coordinator";
                        ZigbeeDeviceObjectReply * zdoReply = node->deviceObject()->requestBindShortAddress(endpointZll->endpointId(), ZigbeeClusterLibrary::ClusterIdLevelControl, 0x0000);
                        connect(zdoReply, &ZigbeeDeviceObjectReply::finished, node, [=](){
                            if (zdoReply->error() != ZigbeeDeviceObjectReply::ErrorNoError) {
                                qCWarning(dcZigbeePhilipsHue()) << "Failed to bind level cluster to coordinator" << zdoReply->error();
                                return;
                            }
                            qCDebug(dcZigbeePhilipsHue()) << "Bind level cluster to coordinator finished successfully";

                            qCDebug(dcZigbeePhilipsHue()) << "Read binding table from node" << node;
                            ZigbeeReply *reply = node->readBindingTableEntries();
                            connect(reply, &ZigbeeReply::finished, node, [=](){
                                if (reply->error() != ZigbeeReply::ErrorNoError) {
                                    qCWarning(dcZigbeePhilipsHue()) << "Failed to read binding table from" << node;
                                } else {
                                    foreach (const ZigbeeDeviceProfile::BindingTableListRecord &binding, node->bindingTableRecords()) {
                                        qCDebug(dcZigbeePhilipsHue()) << node << binding;
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

