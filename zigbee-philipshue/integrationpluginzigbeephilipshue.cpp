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

#include <math.h>

IntegrationPluginZigbeePhilipsHue::IntegrationPluginZigbeePhilipsHue()
{
    m_ieeeAddressParamTypeIds[dimmerSwitchThingClassId] = dimmerSwitchThingIeeeAddressParamTypeId;
    m_ieeeAddressParamTypeIds[outdoorSensorThingClassId] = outdoorSensorThingIeeeAddressParamTypeId;

    m_networkUuidParamTypeIds[dimmerSwitchThingClassId] = dimmerSwitchThingNetworkUuidParamTypeId;
    m_networkUuidParamTypeIds[outdoorSensorThingClassId] = outdoorSensorThingNetworkUuidParamTypeId;

    m_connectedStateTypeIds[dimmerSwitchThingClassId] = dimmerSwitchConnectedStateTypeId;
    m_connectedStateTypeIds[outdoorSensorThingClassId] = outdoorSensorConnectedStateTypeId;

    m_signalStrengthStateTypeIds[dimmerSwitchThingClassId] = dimmerSwitchSignalStrengthStateTypeId;
    m_signalStrengthStateTypeIds[outdoorSensorThingClassId] = outdoorSensorSignalStrengthStateTypeId;

    m_versionStateTypeIds[dimmerSwitchThingClassId] = dimmerSwitchVersionStateTypeId;
    m_versionStateTypeIds[outdoorSensorThingClassId] = outdoorSensorVersionStateTypeId;
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
                endpointOne->deviceId() == Zigbee::LightLinkDeviceNonColourSceneController &&
                endpoinTwo->profile() == Zigbee::ZigbeeProfileHomeAutomation &&
                endpoinTwo->deviceId() == Zigbee::HomeAutomationDeviceSimpleSensor) {

            qCDebug(dcZigbeePhilipsHue()) << "Handeling Hue dimmer switch" << node << endpointOne << endpoinTwo;
            createThing(dimmerSwitchThingClassId, networkUuid, node);
            initDimmerSwitch(node);
            return true;
        }

        // Outdoor sensor
        if (endpointOne->profile() == Zigbee::ZigbeeProfileLightLink &&
                endpointOne->deviceId() == Zigbee::LightLinkDeviceOnOffSensor &&
                endpoinTwo->profile() == Zigbee::ZigbeeProfileHomeAutomation &&
                endpoinTwo->deviceId() == Zigbee::HomeAutomationDeviceOccupacySensor) {

            qCDebug(dcZigbeePhilipsHue()) << "Handeling Hue outdoor sensor" << node << endpointOne << endpoinTwo;
            createThing(outdoorSensorThingClassId, networkUuid, node);
            initOutdoorSensor(node);
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

    if (thing->thingClassId() == outdoorSensorThingClassId) {
        ZigbeeNodeEndpoint *endpointHa = node->getEndpoint(0x02);

        // Set the version
        thing->setStateValue(m_versionStateTypeIds.value(thing->thingClassId()), endpointHa->softwareBuildId());

        // Get battery level changes
        ZigbeeClusterPowerConfiguration *powerCluster = endpointHa->inputCluster<ZigbeeClusterPowerConfiguration>(ZigbeeClusterLibrary::ClusterIdPowerConfiguration);
        if (!powerCluster) {
            qCWarning(dcZigbeePhilipsHue()) << "Could not find power configuration cluster on" << thing << endpointHa;
        } else {
            // Only set the initial state if the attribute already exists
            if (powerCluster->hasAttribute(ZigbeeClusterPowerConfiguration::AttributeBatteryPercentageRemaining)) {
                thing->setStateValue(outdoorSensorBatteryLevelStateTypeId, powerCluster->batteryPercentage());
                thing->setStateValue(outdoorSensorBatteryCriticalStateTypeId, (powerCluster->batteryPercentage() < 10.0));
            }

            connect(powerCluster, &ZigbeeClusterPowerConfiguration::batteryPercentageChanged, thing, [=](double percentage){
                qCDebug(dcZigbeePhilipsHue()) << "Battery percentage changed" << percentage << "%" << thing;
                thing->setStateValue(outdoorSensorBatteryLevelStateTypeId, percentage);
                thing->setStateValue(outdoorSensorBatteryCriticalStateTypeId, (percentage < 10.0));
            });
        }

        ZigbeeClusterOccupancySensing *occupancyCluster = endpointHa->inputCluster<ZigbeeClusterOccupancySensing>(ZigbeeClusterLibrary::ClusterIdOccupancySensing);
        if (!occupancyCluster) {
            qCWarning(dcZigbeePhilipsHue()) << "Occupancy cluster not found on" << thing;
        } else {
            if (!m_presenceTimer) {
                m_presenceTimer = hardwareManager()->pluginTimerManager()->registerTimer(1);
            }

            connect(m_presenceTimer, &PluginTimer::timeout, thing, [thing](){
                if (thing->stateValue(outdoorSensorIsPresentStateTypeId).toBool()) {
                    int timeout = thing->setting(outdoorSensorSettingsTimeoutParamTypeId).toInt();
                    QDateTime lastSeenTime = QDateTime::fromMSecsSinceEpoch(thing->stateValue(outdoorSensorLastSeenTimeStateTypeId).toULongLong() * 1000);
                    if (lastSeenTime.addSecs(timeout) < QDateTime::currentDateTime()) {
                        qCDebug(dcZigbeePhilipsHue()) << thing << "Presence timeout";
                        thing->setStateValue(outdoorSensorIsPresentStateTypeId, false);
                    }
                }
            });

            connect(occupancyCluster, &ZigbeeClusterOccupancySensing::occupancyChanged, thing, [=](bool occupancy){
                qCDebug(dcZigbeePhilipsHue()) << thing << "occupancy cluster changed" << occupancy;
                // Only change the state if the it changed to true, it will be disabled by the timer
                if (occupancy) {
                    thing->setStateValue(outdoorSensorIsPresentStateTypeId, true);
                    thing->setStateValue(outdoorSensorLastSeenTimeStateTypeId, QDateTime::currentMSecsSinceEpoch() / 1000);
                    m_presenceTimer->start();
                }
            });
        }

        ZigbeeClusterTemperatureMeasurement *temperatureCluster = endpointHa->inputCluster<ZigbeeClusterTemperatureMeasurement>(ZigbeeClusterLibrary::ClusterIdTemperatureMeasurement);
        if (!temperatureCluster) {
            qCWarning(dcZigbeePhilipsHue()) << "Could not find the temperature measurement server cluster on" << thing << endpointHa;
        } else {
            // Only set the state if the cluster actually has the attribute
            if (temperatureCluster->hasAttribute(ZigbeeClusterTemperatureMeasurement::AttributeMeasuredValue)) {
                thing->setStateValue(outdoorSensorTemperatureStateTypeId, temperatureCluster->temperature());
            }

            connect(temperatureCluster, &ZigbeeClusterTemperatureMeasurement::temperatureChanged, thing, [thing](double temperature){
                qCDebug(dcZigbeePhilipsHue()) << thing << "temperature changed" << temperature << "°C";
                thing->setStateValue(outdoorSensorTemperatureStateTypeId, temperature);
            });
        }

        ZigbeeClusterIlluminanceMeasurment *illuminanceCluster = endpointHa->inputCluster<ZigbeeClusterIlluminanceMeasurment>(ZigbeeClusterLibrary::ClusterIdIlluminanceMeasurement);
        if (!illuminanceCluster) {
            qCWarning(dcZigbeePhilipsHue()) << "Could not find the illuminance measurement server cluster on" << thing << endpointHa;
        } else {
            // Only set the state if the cluster actually has the attribute
            if (illuminanceCluster->hasAttribute(ZigbeeClusterIlluminanceMeasurment::AttributeMeasuredValue)) {
                int convertedValue = pow(10, illuminanceCluster->illuminance() / 10000) - 1;
                qCDebug(dcZigbeePhilipsHue()) << thing << "illuminance" << illuminanceCluster->illuminance() << "-->" << convertedValue << "lux";
                thing->setStateValue(outdoorSensorLightIntensityStateTypeId, convertedValue);
            }

            connect(illuminanceCluster, &ZigbeeClusterIlluminanceMeasurment::illuminanceChanged, thing, [thing](quint16 illuminance){
                int convertedValue = pow(10, illuminance / 10000) - 1;
                qCDebug(dcZigbeePhilipsHue()) << thing << "illuminance changed" << illuminance << "-->" << convertedValue << "lux";
                thing->setStateValue(outdoorSensorLightIntensityStateTypeId, convertedValue);
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
        } else {
            qCDebug(dcZigbeePhilipsHue()) << "Removed all bindings successfully from" << node;
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
            } else {
                qCDebug(dcZigbeePhilipsHue()) << "Read power configuration cluster attributes finished successfully";
            }


            // Bind the cluster to the coordinator
            qCDebug(dcZigbeePhilipsHue()) << "Bind power configuration cluster to coordinator IEEE address";
            ZigbeeDeviceObjectReply * zdoReply = node->deviceObject()->requestBindIeeeAddress(endpointHa->endpointId(), ZigbeeClusterLibrary::ClusterIdPowerConfiguration,
                                                                                              hardwareManager()->zigbeeResource()->coordinatorAddress(node->networkUuid()), 0x01);
            connect(zdoReply, &ZigbeeDeviceObjectReply::finished, node, [=](){
                if (zdoReply->error() != ZigbeeDeviceObjectReply::ErrorNoError) {
                    qCWarning(dcZigbeePhilipsHue()) << "Failed to bind power cluster to coordinator" << zdoReply->error();
                } else {
                    qCDebug(dcZigbeePhilipsHue()) << "Bind power configuration cluster to coordinator finished successfully";
                }

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
                    } else {
                        qCDebug(dcZigbeePhilipsHue()) << "Attribute reporting configuration finished for power cluster" << ZigbeeClusterLibrary::parseAttributeReportingStatusRecords(reportingReply->responseFrame().payload);
                    }

                    qCDebug(dcZigbeePhilipsHue()) << "Bind on/off cluster to coordinator";
                    ZigbeeDeviceObjectReply * zdoReply = node->deviceObject()->requestBindShortAddress(endpointZll->endpointId(), ZigbeeClusterLibrary::ClusterIdOnOff, 0x0000);
                    connect(zdoReply, &ZigbeeDeviceObjectReply::finished, node, [=](){
                        if (zdoReply->error() != ZigbeeDeviceObjectReply::ErrorNoError) {
                            qCWarning(dcZigbeePhilipsHue()) << "Failed to bind on/off cluster to coordinator" << zdoReply->error();
                        } else {
                            qCDebug(dcZigbeePhilipsHue()) << "Bind on/off cluster to coordinator finished successfully";
                        }

                        qCDebug(dcZigbeePhilipsHue()) << "Bind power level cluster to coordinator";
                        ZigbeeDeviceObjectReply * zdoReply = node->deviceObject()->requestBindShortAddress(endpointZll->endpointId(), ZigbeeClusterLibrary::ClusterIdLevelControl, 0x0000);
                        connect(zdoReply, &ZigbeeDeviceObjectReply::finished, node, [=](){
                            if (zdoReply->error() != ZigbeeDeviceObjectReply::ErrorNoError) {
                                qCWarning(dcZigbeePhilipsHue()) << "Failed to bind level cluster to coordinator" << zdoReply->error();
                            } else {
                                qCDebug(dcZigbeePhilipsHue()) << "Bind level cluster to coordinator finished successfully";
                            }

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

void IntegrationPluginZigbeePhilipsHue::initOutdoorSensor(ZigbeeNode *node)
{
    ZigbeeNodeEndpoint *endpointHa = node->getEndpoint(0x02);

    // Get the current configured bindings for this node
    ZigbeeReply *reply = node->removeAllBindings();
    connect(reply, &ZigbeeReply::finished, node, [=](){
        if (reply->error() != ZigbeeReply::ErrorNoError) {
            qCWarning(dcZigbeePhilipsHue()) << "Failed to remove all bindings for initialization of" << node;
        } else {
            qCDebug(dcZigbeePhilipsHue()) << "Removed all bindings successfully from" << node;
        }

        // Read battery, bind and configure attribute reporting for battery
        if (!endpointHa->hasInputCluster(ZigbeeClusterLibrary::ClusterIdPowerConfiguration)) {
            qCWarning(dcZigbeePhilipsHue()) << "Failed to initialize the power configuration cluster because the cluster could not be found" << node << endpointHa;
            return;
        }

        // Init power configuration cluster
        qCDebug(dcZigbeePhilipsHue()) << "Read power configuration cluster attributes" << node;
        ZigbeeClusterReply *readAttributeReply = endpointHa->getInputCluster(ZigbeeClusterLibrary::ClusterIdPowerConfiguration)->readAttributes({ZigbeeClusterPowerConfiguration::AttributeBatteryPercentageRemaining});
        connect(readAttributeReply, &ZigbeeClusterReply::finished, node, [=](){
            if (readAttributeReply->error() != ZigbeeClusterReply::ErrorNoError) {
                qCWarning(dcZigbeePhilipsHue()) << "Failed to read power cluster attributes" << readAttributeReply->error();
            } else {
                qCDebug(dcZigbeePhilipsHue()) << "Read power configuration cluster attributes finished successfully";
            }

            // Bind the cluster to the coordinator
            qCDebug(dcZigbeePhilipsHue()) << "Bind power configuration cluster to coordinator IEEE address";
            ZigbeeDeviceObjectReply * zdoReply = node->deviceObject()->requestBindIeeeAddress(endpointHa->endpointId(), ZigbeeClusterLibrary::ClusterIdPowerConfiguration,
                                                                                              hardwareManager()->zigbeeResource()->coordinatorAddress(node->networkUuid()), 0x01);
            connect(zdoReply, &ZigbeeDeviceObjectReply::finished, node, [=](){
                if (zdoReply->error() != ZigbeeDeviceObjectReply::ErrorNoError) {
                    qCWarning(dcZigbeePhilipsHue()) << "Failed to bind power cluster to coordinator" << zdoReply->error();
                } else {
                    qCDebug(dcZigbeePhilipsHue()) << "Bind power configuration cluster to coordinator finished successfully";
                }

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
                    } else {
                        qCDebug(dcZigbeePhilipsHue()) << "Attribute reporting configuration finished for power cluster" << ZigbeeClusterLibrary::parseAttributeReportingStatusRecords(reportingReply->responseFrame().payload);
                    }


                    // Init temperature measurment cluster
                    qCDebug(dcZigbeePhilipsHue()) << "Bind temperature cluster to coordinator IEEE address";
                    ZigbeeDeviceObjectReply * zdoReply = node->deviceObject()->requestBindIeeeAddress(endpointHa->endpointId(), ZigbeeClusterLibrary::ClusterIdTemperatureMeasurement,
                                                                                                      hardwareManager()->zigbeeResource()->coordinatorAddress(node->networkUuid()), 0x01);
                    connect(zdoReply, &ZigbeeDeviceObjectReply::finished, node, [=](){
                        if (zdoReply->error() != ZigbeeDeviceObjectReply::ErrorNoError) {
                            qCWarning(dcZigbeePhilipsHue()) << "Failed to bind temperature cluster to coordinator" << zdoReply->error();
                        } else {
                            qCDebug(dcZigbeePhilipsHue()) << "Bind temperature cluster to coordinator finished successfully";
                        }

                        // Read current temperature
                        if (!endpointHa->hasInputCluster(ZigbeeClusterLibrary::ClusterIdTemperatureMeasurement)) {
                            qCWarning(dcZigbeePhilipsHue()) << "Failed to initialize the temperature cluster because the cluster could not be found" << node << endpointHa;
                            return;
                        }

                        qCDebug(dcZigbeePhilipsHue()) << "Read temperature cluster attributes" << node;
                        ZigbeeClusterReply *readAttributeReply = endpointHa->getInputCluster(ZigbeeClusterLibrary::ClusterIdTemperatureMeasurement)->readAttributes({ZigbeeClusterTemperatureMeasurement::AttributeMeasuredValue});
                        connect(readAttributeReply, &ZigbeeClusterReply::finished, node, [=](){
                            if (readAttributeReply->error() != ZigbeeClusterReply::ErrorNoError) {
                                qCWarning(dcZigbeePhilipsHue()) << "Failed to read temperature cluster attributes" << readAttributeReply->error();
                            } else {
                                qCDebug(dcZigbeePhilipsHue()) << "Read temperature cluster attributes finished successfully";
                            }

                            // Configure attribute reporting for temperature
                            ZigbeeClusterLibrary::AttributeReportingConfiguration reportingConfig;
                            reportingConfig.attributeId = ZigbeeClusterTemperatureMeasurement::AttributeMeasuredValue;
                            reportingConfig.dataType = Zigbee::Int16;
                            reportingConfig.minReportingInterval = 300;
                            reportingConfig.maxReportingInterval = 600;
                            reportingConfig.reportableChange = ZigbeeDataType(static_cast<qint16>(10)).data();

                            qCDebug(dcZigbeePhilipsHue()) << "Configure attribute reporting for temperature cluster to coordinator";
                            ZigbeeClusterReply *reportingReply = endpointHa->getInputCluster(ZigbeeClusterLibrary::ClusterIdTemperatureMeasurement)->configureReporting({reportingConfig});
                            connect(reportingReply, &ZigbeeClusterReply::finished, this, [=](){
                                if (reportingReply->error() != ZigbeeClusterReply::ErrorNoError) {
                                    qCWarning(dcZigbeePhilipsHue()) << "Failed to configure temperature cluster attribute reporting" << reportingReply->error();
                                } else {
                                    qCDebug(dcZigbeePhilipsHue()) << "Attribute reporting configuration finished for temperature cluster" << ZigbeeClusterLibrary::parseAttributeReportingStatusRecords(reportingReply->responseFrame().payload);
                                }

                                // Init illuminance measurment cluster
                                qCDebug(dcZigbeePhilipsHue()) << "Bind illuminance measurement cluster to coordinator IEEE address";
                                ZigbeeDeviceObjectReply * zdoReply = node->deviceObject()->requestBindIeeeAddress(endpointHa->endpointId(), ZigbeeClusterLibrary::ClusterIdIlluminanceMeasurement,
                                                                                                                  hardwareManager()->zigbeeResource()->coordinatorAddress(node->networkUuid()), 0x01);
                                connect(zdoReply, &ZigbeeDeviceObjectReply::finished, node, [=](){
                                    if (zdoReply->error() != ZigbeeDeviceObjectReply::ErrorNoError) {
                                        qCWarning(dcZigbeePhilipsHue()) << "Failed to bind illuminance measurement cluster to coordinator" << zdoReply->error();
                                    } else {
                                        qCDebug(dcZigbeePhilipsHue()) << "Bind illuminance measurement cluster to coordinator finished successfully";
                                    }

                                    // Read current illuminance
                                    if (!endpointHa->hasInputCluster(ZigbeeClusterLibrary::ClusterIdIlluminanceMeasurement)) {
                                        qCWarning(dcZigbeePhilipsHue()) << "Failed to initialize the illuminance measurement cluster because the cluster could not be found" << node << endpointHa;
                                        return;
                                    }

                                    qCDebug(dcZigbeePhilipsHue()) << "Read illuminance measurement cluster attributes" << node;
                                    ZigbeeClusterReply *readAttributeReply = endpointHa->getInputCluster(ZigbeeClusterLibrary::ClusterIdIlluminanceMeasurement)->readAttributes({ZigbeeClusterIlluminanceMeasurment::AttributeMeasuredValue});
                                    connect(readAttributeReply, &ZigbeeClusterReply::finished, node, [=](){
                                        if (readAttributeReply->error() != ZigbeeClusterReply::ErrorNoError) {
                                            qCWarning(dcZigbeePhilipsHue()) << "Failed to read illuminance measurement cluster attributes" << readAttributeReply->error();
                                        } else {
                                            qCDebug(dcZigbeePhilipsHue()) << "Read illuminance measurement cluster attributes finished successfully";
                                        }

                                        // Configure attribute reporting

                                        // Configure attribute reporting for temperature
                                        ZigbeeClusterLibrary::AttributeReportingConfiguration reportingConfig;
                                        reportingConfig.attributeId = ZigbeeClusterIlluminanceMeasurment::AttributeMeasuredValue;
                                        reportingConfig.dataType = Zigbee::Uint16;
                                        reportingConfig.minReportingInterval = 300;
                                        reportingConfig.maxReportingInterval = 600;
                                        reportingConfig.reportableChange = ZigbeeDataType(static_cast<quint16>(1)).data();

                                        qCDebug(dcZigbeePhilipsHue()) << "Configure attribute reporting for illuminance measurement cluster to coordinator";
                                        ZigbeeClusterReply *reportingReply = endpointHa->getInputCluster(ZigbeeClusterLibrary::ClusterIdIlluminanceMeasurement)->configureReporting({reportingConfig});
                                        connect(reportingReply, &ZigbeeClusterReply::finished, this, [=](){
                                            if (reportingReply->error() != ZigbeeClusterReply::ErrorNoError) {
                                                qCWarning(dcZigbeePhilipsHue()) << "Failed to configure illuminance measurement cluster attribute reporting" << reportingReply->error();
                                            } else {
                                                qCDebug(dcZigbeePhilipsHue()) << "Attribute reporting configuration finished for illuminance measurement cluster" << ZigbeeClusterLibrary::parseAttributeReportingStatusRecords(reportingReply->responseFrame().payload);
                                            }


                                            // Init occupancy sensing cluster
                                            qCDebug(dcZigbeePhilipsHue()) << "Bind occupancy sensing cluster to coordinator IEEE address";
                                            ZigbeeDeviceObjectReply * zdoReply = node->deviceObject()->requestBindIeeeAddress(endpointHa->endpointId(), ZigbeeClusterLibrary::ClusterIdOccupancySensing,
                                                                                                                              hardwareManager()->zigbeeResource()->coordinatorAddress(node->networkUuid()), 0x01);
                                            connect(zdoReply, &ZigbeeDeviceObjectReply::finished, node, [=](){
                                                if (zdoReply->error() != ZigbeeDeviceObjectReply::ErrorNoError) {
                                                    qCWarning(dcZigbeePhilipsHue()) << "Failed to bind occupancy sensing cluster to coordinator" << zdoReply->error();
                                                } else {
                                                    qCDebug(dcZigbeePhilipsHue()) << "Bind occupancy sensing cluster to coordinator finished successfully";
                                                }


                                                qCDebug(dcZigbeePhilipsHue()) << "Read occupancy sensing cluster attributes" << node;
                                                ZigbeeClusterReply *readAttributeReply = endpointHa->getInputCluster(ZigbeeClusterLibrary::ClusterIdOccupancySensing)->readAttributes({ZigbeeClusterOccupancySensing::AttributeOccupancy, ZigbeeClusterOccupancySensing::AttributeOccupancySensorType});
                                                connect(readAttributeReply, &ZigbeeClusterReply::finished, node, [=](){
                                                    if (readAttributeReply->error() != ZigbeeClusterReply::ErrorNoError) {
                                                        qCWarning(dcZigbeePhilipsHue()) << "Failed to read occupancy sensing cluster attributes" << readAttributeReply->error();
                                                    } else {
                                                        qCDebug(dcZigbeePhilipsHue()) << "Read occupancy sensing cluster attributes finished successfully";
                                                    }

                                                    // Configure attribute reporting for temperature
                                                    ZigbeeClusterLibrary::AttributeReportingConfiguration reportingConfig;
                                                    reportingConfig.attributeId = ZigbeeClusterOccupancySensing::AttributeOccupancy;
                                                    reportingConfig.dataType = Zigbee::BitMap8;
                                                    reportingConfig.minReportingInterval = 0;
                                                    reportingConfig.maxReportingInterval = 300;

                                                    qCDebug(dcZigbeePhilipsHue()) << "Configure attribute reporting for occupancy cluster to coordinator";
                                                    ZigbeeClusterReply *reportingReply = endpointHa->getInputCluster(ZigbeeClusterLibrary::ClusterIdOccupancySensing)->configureReporting({reportingConfig});
                                                    connect(reportingReply, &ZigbeeClusterReply::finished, this, [=](){
                                                        if (reportingReply->error() != ZigbeeClusterReply::ErrorNoError) {
                                                            qCWarning(dcZigbeePhilipsHue()) << "Failed to configure occupancy cluster attribute reporting" << reportingReply->error();
                                                        } else {
                                                            qCDebug(dcZigbeePhilipsHue()) << "Attribute reporting configuration finished for occupancy cluster" << ZigbeeClusterLibrary::parseAttributeReportingStatusRecords(reportingReply->responseFrame().payload);
                                                        }
                                                    });
                                                });
                                            });
                                        });
                                    });
                                });
                            });
                        });
                    });
                });
            });
        });
    });
}

