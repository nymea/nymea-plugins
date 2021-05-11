/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*
*
* Copyright 2013 - 2021, nymea GmbH
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

#include "integrationpluginzigbeedevelco.h"
#include "plugininfo.h"
#include "hardware/zigbee/zigbeehardwareresource.h"

#include <QDebug>

#include <zigbeeutils.h>

IntegrationPluginZigbeeDevelco::IntegrationPluginZigbeeDevelco()
{
    m_ieeeAddressParamTypeIds[ioModuleThingClassId] = ioModuleThingIeeeAddressParamTypeId;

    m_networkUuidParamTypeIds[ioModuleThingClassId] = ioModuleThingNetworkUuidParamTypeId;

    m_connectedStateTypeIds[ioModuleThingClassId] = ioModuleConnectedStateTypeId;

    m_signalStrengthStateTypeIds[ioModuleThingClassId] = ioModuleSignalStrengthStateTypeId;
}

QString IntegrationPluginZigbeeDevelco::name() const
{
    return "Develco";
}

bool IntegrationPluginZigbeeDevelco::handleNode(ZigbeeNode *node, const QUuid &networkUuid)
{
    // Filter for develco manufacturer code
    if (node->nodeDescriptor().manufacturerCode != Zigbee::Develco)
        return false;

    bool handled = false;
    if (node->modelName() == "IOMZB-110" || node->modelName() == "DIOZB-110") {
        if (node->hasEndpoint(IO_MODULE_EP_INPUT1) && node->hasEndpoint(IO_MODULE_EP_INPUT2) &&
                node->hasEndpoint(IO_MODULE_EP_INPUT3) && node->hasEndpoint(IO_MODULE_EP_INPUT4) &&
                node->hasEndpoint(IO_MODULE_EP_OUTPUT1 && node->hasEndpoint(IO_MODULE_EP_OUTPUT2))) {
            qCDebug(dcZigbeeDevelco()) << "Found IO module" << node << networkUuid.toString();
            initIoModule(node);
            createThing(ioModuleThingClassId, networkUuid, node);
            handled = true;
        }
    }

    return handled;
}

void IntegrationPluginZigbeeDevelco::handleRemoveNode(ZigbeeNode *node, const QUuid &networkUuid)
{
    Q_UNUSED(networkUuid)
    Thing *thing = m_thingNodes.key(node);
    if (thing) {
        qCDebug(dcZigbeeDevelco()) << node << "for" << thing << "has left the network.";
        emit autoThingDisappeared(thing->id());

        // Removing it from our map to prevent a loop that would ask the zigbee network to remove this node (see thingRemoved())
        m_thingNodes.remove(thing);
    }
}

void IntegrationPluginZigbeeDevelco::init()
{
    hardwareManager()->zigbeeResource()->registerHandler(this, ZigbeeHardwareResource::HandlerTypeVendor);
}

void IntegrationPluginZigbeeDevelco::setupThing(ThingSetupInfo *info)
{
    qCDebug(dcZigbeeDevelco()) << "Setup" << info->thing();

    // Get the node for this thing
    Thing *thing = info->thing();
    QUuid networkUuid = thing->paramValue(m_networkUuidParamTypeIds.value(thing->thingClassId())).toUuid();
    ZigbeeAddress zigbeeAddress = ZigbeeAddress(thing->paramValue(m_ieeeAddressParamTypeIds.value(thing->thingClassId())).toString());
    ZigbeeNode *node = m_thingNodes.value(thing);
    if (!node) {
        node = hardwareManager()->zigbeeResource()->claimNode(this, networkUuid, zigbeeAddress);
        if (!node) {
            qCWarning(dcZigbeeDevelco()) << "Coud not find zigbee node for" << thing;
            info->finish(Thing::ThingErrorHardwareNotAvailable);
            return;
        }
    }

    m_thingNodes.insert(thing, node);

    // Update signal strength
    thing->setStateValue(m_signalStrengthStateTypeIds.value(thing->thingClassId()), qRound(node->lqi() * 100.0 / 255.0));
    connect(node, &ZigbeeNode::lqiChanged, thing, [this, thing](quint8 lqi){
        uint signalStrength = qRound(lqi * 100.0 / 255.0);
        qCDebug(dcZigbeeDevelco()) << thing << "signal strength changed" << signalStrength << "%";
        thing->setStateValue(m_signalStrengthStateTypeIds.value(thing->thingClassId()), signalStrength);
    });

    if (thing->thingClassId() == ioModuleThingClassId) {
        // Set the version from the manufacturer specific attribute in base cluster
        ZigbeeNodeEndpoint *primaryEndpoint = node->getEndpoint(IO_MODULE_EP_INPUT1);
        if (!primaryEndpoint) {
            qCWarning(dcZigbeeDevelco()) << "Failed to set up IO module" << thing << ". Could not find endpoint for version parsing.";
            info->finish(Thing::ThingErrorSetupFailed);
            return;
        }
        if (primaryEndpoint->hasInputCluster(ZigbeeClusterLibrary::ClusterIdBasic)) {
            ZigbeeCluster *basicCluster = primaryEndpoint->getInputCluster(ZigbeeClusterLibrary::ClusterIdBasic);
            if (basicCluster->hasAttribute(DEVELCO_ATTRIBUTE_SW_VERSION)) {
                thing->setStateValue(ioModuleVersionStateTypeId, parseDevelcoVersionString(primaryEndpoint));
            } else {
                readDevelcoFirmwareVersion(node, primaryEndpoint);
            }

            connect(basicCluster, &ZigbeeCluster::attributeChanged, this, [=](const ZigbeeClusterAttribute &attribute){
                if (attribute.id() == DEVELCO_ATTRIBUTE_SW_VERSION) {
                    thing->setStateValue(ioModuleVersionStateTypeId, parseDevelcoVersionString(primaryEndpoint));
                }
            });
        }

        // Handle reachable state from node
        thing->setStateValue(m_connectedStateTypeIds.value(thing->thingClassId()), node->reachable());
        connect(node, &ZigbeeNode::reachableChanged, thing, [=](bool reachable){
            thing->setStateValue(m_connectedStateTypeIds.value(thing->thingClassId()), reachable);
            if (reachable) {
                readDevelcoFirmwareVersion(node, node->getEndpoint(IO_MODULE_EP_INPUT1));
                readIoModuleOutputPowerStates(thing);
                readIoModuleInputPowerStates(thing);
            }
        });


        // Output 1
        ZigbeeNodeEndpoint *output1Endpoint = node->getEndpoint(IO_MODULE_EP_OUTPUT1);
        if (!output1Endpoint) {
            qCWarning(dcZigbeeDevelco()) << "Could not find endpoint for output 1 on" << thing << node;
        } else {
            ZigbeeClusterOnOff *onOffCluster = output1Endpoint->inputCluster<ZigbeeClusterOnOff>(ZigbeeClusterLibrary::ClusterIdOnOff);
            if (!onOffCluster) {
                qCWarning(dcZigbeeDevelco()) << "Could not find On/Off cluster on" << thing << node << output1Endpoint;
            } else {
                if (onOffCluster->hasAttribute(ZigbeeClusterOnOff::AttributeOnOff)) {
                    thing->setStateValue(ioModuleOutput1StateTypeId, onOffCluster->power());
                }
                connect(onOffCluster, &ZigbeeClusterOnOff::powerChanged, thing, [thing](bool power){
                    qCDebug(dcZigbeeDevelco()) << thing << "output 1 power changed to" << power;
                    thing->setStateValue(ioModuleOutput1StateTypeId, power);
                });
            }
        }

        // Output 2
        ZigbeeNodeEndpoint *output2Endpoint = node->getEndpoint(IO_MODULE_EP_OUTPUT2);
        if (!output2Endpoint) {
            qCWarning(dcZigbeeDevelco()) << "Could not find endpoint for output 2 on" << thing << node;
        } else {
            ZigbeeClusterOnOff *onOffCluster = output2Endpoint->inputCluster<ZigbeeClusterOnOff>(ZigbeeClusterLibrary::ClusterIdOnOff);
            if (!onOffCluster) {
                qCWarning(dcZigbeeDevelco()) << "Could not find On/Off cluster on" << thing << node << output2Endpoint;
            } else {
                if (onOffCluster->hasAttribute(ZigbeeClusterOnOff::AttributeOnOff)) {
                    thing->setStateValue(ioModuleOutput2StateTypeId, onOffCluster->power());
                }
                connect(onOffCluster, &ZigbeeClusterOnOff::powerChanged, thing, [thing](bool power){
                    qCDebug(dcZigbeeDevelco()) << thing << "output 2 power changed to" << power;
                    thing->setStateValue(ioModuleOutput2StateTypeId, power);
                });
            }
        }

        // Input 1
        ZigbeeNodeEndpoint *input1Endpoint = node->getEndpoint(IO_MODULE_EP_INPUT1);
        if (!input1Endpoint) {
            qCWarning(dcZigbeeDevelco()) << "Could not find endpoint for input 1 on" << thing << node;
        } else {
            ZigbeeClusterBinaryInput *binaryInputCluster = input1Endpoint->inputCluster<ZigbeeClusterBinaryInput>(ZigbeeClusterLibrary::ClusterIdBinaryInput);
            if (!binaryInputCluster) {
                qCWarning(dcZigbeeDevelco()) << "Could not find BinaryInput cluster on" << thing << node << input1Endpoint;
            } else {
                if (binaryInputCluster->hasAttribute(ZigbeeClusterLibrary::ClusterIdBinaryInput)) {
                    thing->setStateValue(ioModuleInput1StateTypeId, binaryInputCluster->presentValue());
                }
                connect(binaryInputCluster, &ZigbeeClusterBinaryInput::presentValueChanged, thing, [thing](bool presentValue){
                    qCDebug(dcZigbeeDevelco()) << thing << "input 1 changed to" << presentValue;
                    thing->setStateValue(ioModuleInput1StateTypeId, presentValue);
                });
            }
        }

        // Input 2
        ZigbeeNodeEndpoint *input2Endpoint = node->getEndpoint(IO_MODULE_EP_INPUT2);
        if (!input2Endpoint) {
            qCWarning(dcZigbeeDevelco()) << "Could not find endpoint for input 2 on" << thing << node;
        } else {
            ZigbeeClusterBinaryInput *binaryInputCluster = input2Endpoint->inputCluster<ZigbeeClusterBinaryInput>(ZigbeeClusterLibrary::ClusterIdBinaryInput);
            if (!binaryInputCluster) {
                qCWarning(dcZigbeeDevelco()) << "Could not find BinaryInput cluster on" << thing << node << input2Endpoint;
            } else {
                if (binaryInputCluster->hasAttribute(ZigbeeClusterLibrary::ClusterIdBinaryInput)) {
                    thing->setStateValue(ioModuleInput2StateTypeId, binaryInputCluster->presentValue());
                }
                connect(binaryInputCluster, &ZigbeeClusterBinaryInput::presentValueChanged, thing, [thing](bool presentValue){
                    qCDebug(dcZigbeeDevelco()) << thing << "input 2 changed to" << presentValue;
                    thing->setStateValue(ioModuleInput2StateTypeId, presentValue);
                });
            }
        }

        // Input 3
        ZigbeeNodeEndpoint *input3Endpoint = node->getEndpoint(IO_MODULE_EP_INPUT3);
        if (!input3Endpoint) {
            qCWarning(dcZigbeeDevelco()) << "Could not find endpoint for input 3 on" << thing << node;
        } else {
            ZigbeeClusterBinaryInput *binaryInputCluster = input3Endpoint->inputCluster<ZigbeeClusterBinaryInput>(ZigbeeClusterLibrary::ClusterIdBinaryInput);
            if (!binaryInputCluster) {
                qCWarning(dcZigbeeDevelco()) << "Could not find BinaryInput cluster on" << thing << node << input3Endpoint;
            } else {
                if (binaryInputCluster->hasAttribute(ZigbeeClusterLibrary::ClusterIdBinaryInput)) {
                    thing->setStateValue(ioModuleInput3StateTypeId, binaryInputCluster->presentValue());
                }
                connect(binaryInputCluster, &ZigbeeClusterBinaryInput::presentValueChanged, thing, [thing](bool presentValue){
                    qCDebug(dcZigbeeDevelco()) << thing << "input 3 changed to" << presentValue;
                    thing->setStateValue(ioModuleInput3StateTypeId, presentValue);
                });
            }
        }

        // Input 4
        ZigbeeNodeEndpoint *input4Endpoint = node->getEndpoint(IO_MODULE_EP_INPUT4);
        if (!input4Endpoint) {
            qCWarning(dcZigbeeDevelco()) << "Could not find endpoint for input 4 on" << thing << node;
        } else {
            ZigbeeClusterBinaryInput *binaryInputCluster = input4Endpoint->inputCluster<ZigbeeClusterBinaryInput>(ZigbeeClusterLibrary::ClusterIdBinaryInput);
            if (!binaryInputCluster) {
                qCWarning(dcZigbeeDevelco()) << "Could not find BinaryInput cluster on" << thing << node << input4Endpoint;
            } else {
                if (binaryInputCluster->hasAttribute(ZigbeeClusterLibrary::ClusterIdBinaryInput)) {
                    thing->setStateValue(ioModuleInput4StateTypeId, binaryInputCluster->presentValue());
                }
                connect(binaryInputCluster, &ZigbeeClusterBinaryInput::presentValueChanged, thing, [thing](bool presentValue){
                    qCDebug(dcZigbeeDevelco()) << thing << "input 4 changed to" << presentValue;
                    thing->setStateValue(ioModuleInput4StateTypeId, presentValue);
                });
            }
        }
    }

    info->finish(Thing::ThingErrorNoError);
}

void IntegrationPluginZigbeeDevelco::postSetupThing(Thing *thing)
{
    if (thing->thingClassId() == ioModuleThingClassId) {
        if (m_thingNodes.value(thing)->reachable()) {
            readIoModuleOutputPowerStates(thing);
            readIoModuleInputPowerStates(thing);
        }
    }
}

void IntegrationPluginZigbeeDevelco::executeAction(ThingActionInfo *info)
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

    if (thing->thingClassId() == ioModuleThingClassId) {
        // Identify
        if (info->action().actionTypeId() == ioModuleAlertActionTypeId) {
            ZigbeeNodeEndpoint *primaryEndpoint = node->getEndpoint(IO_MODULE_EP_INPUT1);
            if (!primaryEndpoint) {
                qCWarning(dcZigbeeDevelco()) << "Could not find endpoint for execute action on" << thing << node;
                info->finish(Thing::ThingErrorHardwareFailure);
                return;
            }

            ZigbeeClusterIdentify *identifyCluster = primaryEndpoint->inputCluster<ZigbeeClusterIdentify>(ZigbeeClusterLibrary::ClusterIdIdentify);
            if (!identifyCluster) {
                qCWarning(dcZigbeeDevelco()) << "Could not find identify cluster for" << thing << "in" << node;
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

        // Output 1
        if (info->action().actionTypeId() == ioModuleOutput1ActionTypeId) {
            bool power = info->action().paramValue(ioModuleOutput1ActionOutput1ParamTypeId).toBool();
            qCDebug(dcZigbeeDevelco()) << "Set output 1 power of" << thing << "to" << power;

            ZigbeeNodeEndpoint *output1Endpoint = node->getEndpoint(IO_MODULE_EP_OUTPUT1);
            if (!output1Endpoint) {
                qCWarning(dcZigbeeDevelco()) << "Could not find endpoint for output 1 on" << thing << node;
                info->finish(Thing::ThingErrorHardwareFailure);
                return;
            }

            ZigbeeClusterOnOff *onOffCluster = output1Endpoint->inputCluster<ZigbeeClusterOnOff>(ZigbeeClusterLibrary::ClusterIdOnOff);
            if (!onOffCluster) {
                qCWarning(dcZigbeeDevelco()) << "Could not find On/Off cluster on" << thing << node << output1Endpoint;
                info->finish(Thing::ThingErrorHardwareFailure);
                return;
            }

            ZigbeeClusterReply *reply = (power ? onOffCluster->commandOn() : onOffCluster->commandOff());
            connect(reply, &ZigbeeClusterReply::finished, info, [=](){
                // Note: reply will be deleted automatically
                if (reply->error() != ZigbeeClusterReply::ErrorNoError) {
                    qCWarning(dcZigbeeDevelco()) << "Failed to set power for output 1 on" << thing << reply->error();
                    info->finish(Thing::ThingErrorHardwareFailure);
                } else {
                    info->finish(Thing::ThingErrorNoError);
                    qCDebug(dcZigbeeDevelco()) << "Set power on output 1 finished successfully for" << thing;
                    thing->setStateValue(ioModuleOutput1StateTypeId, power);
                }
            });
            return;
        }

        // Output 2
        if (info->action().actionTypeId() == ioModuleOutput2ActionTypeId) {
            bool power = info->action().paramValue(ioModuleOutput2ActionOutput2ParamTypeId).toBool();
            qCDebug(dcZigbeeDevelco()) << "Set output 2 power of" << thing << "to" << power;

            ZigbeeNodeEndpoint *output2Endpoint = node->getEndpoint(IO_MODULE_EP_OUTPUT2);
            if (!output2Endpoint) {
                qCWarning(dcZigbeeDevelco()) << "Could not find endpoint for output 2 on" << thing << node;
                info->finish(Thing::ThingErrorHardwareFailure);
                return;
            }

            ZigbeeClusterOnOff *onOffCluster = output2Endpoint->inputCluster<ZigbeeClusterOnOff>(ZigbeeClusterLibrary::ClusterIdOnOff);
            if (!onOffCluster) {
                qCWarning(dcZigbeeDevelco()) << "Could not find On/Off cluster on" << thing << node << output2Endpoint;
                info->finish(Thing::ThingErrorHardwareFailure);
                return;
            }

            ZigbeeClusterReply *reply = (power ? onOffCluster->commandOn() : onOffCluster->commandOff());
            connect(reply, &ZigbeeClusterReply::finished, info, [=](){
                // Note: reply will be deleted automatically
                if (reply->error() != ZigbeeClusterReply::ErrorNoError) {
                    qCWarning(dcZigbeeDevelco()) << "Failed to set power for output 2 on" << thing << reply->error();
                    info->finish(Thing::ThingErrorHardwareFailure);
                } else {
                    info->finish(Thing::ThingErrorNoError);
                    qCDebug(dcZigbeeDevelco()) << "Set power on output 2 finished successfully for" << thing;
                    thing->setStateValue(ioModuleOutput2StateTypeId, power);
                }
            });
            return;
        }

        // Impulse action
        if (info->action().actionTypeId() == ioModuleImpulseOutput1ActionTypeId || info->action().actionTypeId() == ioModuleImpulseOutput2ActionTypeId) {
            // Uint for time is 1/10 s
            uint impulseDuration = thing->settings().paramValue(ioModuleSettingsImpulseDurationParamTypeId).toUInt();
            quint16 impulseDurationScaled = static_cast<quint16>(qRound(impulseDuration / 100.0));

            ZigbeeNodeEndpoint *endpoint = nullptr;
            if (info->action().actionTypeId() == ioModuleImpulseOutput1ActionTypeId) {
                endpoint = node->getEndpoint(IO_MODULE_EP_OUTPUT1);
                qCDebug(dcZigbeeDevelco()) << "Execute output 1 impulse with" << impulseDurationScaled * 100 << "ms";
            } else if (info->action().actionTypeId() == ioModuleImpulseOutput2ActionTypeId) {
                endpoint = node->getEndpoint(IO_MODULE_EP_OUTPUT2);
                qCDebug(dcZigbeeDevelco()) << "Execute output 2 impulse with" << impulseDurationScaled * 100 << "ms";
            }

            if (!endpoint) {
                qCWarning(dcZigbeeDevelco()) << "Could not find endpoint for impulse action on" << thing << node;
                info->finish(Thing::ThingErrorHardwareFailure);
                return;
            }

            ZigbeeClusterOnOff *onOffCluster = endpoint->inputCluster<ZigbeeClusterOnOff>(ZigbeeClusterLibrary::ClusterIdOnOff);
            if (!onOffCluster) {
                qCWarning(dcZigbeeDevelco()) << "Could not find On/Off cluster on" << thing << node << endpoint;
                info->finish(Thing::ThingErrorHardwareFailure);
                return;
            }

            ZigbeeClusterReply *reply = onOffCluster->commandOnWithTimedOff(false, impulseDurationScaled, 0);
            connect(reply, &ZigbeeClusterReply::finished, info, [=](){
                // Note: reply will be deleted automatically
                if (reply->error() != ZigbeeClusterReply::ErrorNoError) {
                    qCWarning(dcZigbeeDevelco()) << "Failed to set on with timed off on" << thing << endpoint << reply->error();
                    info->finish(Thing::ThingErrorHardwareFailure);
                } else {
                    info->finish(Thing::ThingErrorNoError);
                    qCDebug(dcZigbeeDevelco()) << "Set on with timed off on finished successfully for" << thing << endpoint;
                }
            });
            return;
        }
    }

    info->finish(Thing::ThingErrorUnsupportedFeature);
}

void IntegrationPluginZigbeeDevelco::thingRemoved(Thing *thing)
{
    ZigbeeNode *node = m_thingNodes.take(thing);
    if (node) {
        QUuid networkUuid = thing->paramValue(m_networkUuidParamTypeIds.value(thing->thingClassId())).toUuid();
        hardwareManager()->zigbeeResource()->removeNodeFromNetwork(networkUuid, node);
    }
}

void IntegrationPluginZigbeeDevelco::createThing(const ThingClassId &thingClassId, const QUuid &networkUuid, ZigbeeNode *node)
{
    ThingDescriptor descriptor(thingClassId);
    QString deviceClassName = supportedThings().findById(thingClassId).displayName();
    descriptor.setTitle(QString("%1 (%2 - %3)").arg(deviceClassName, node->manufacturerName(), node->modelName()));

    ParamList params;
    params.append(Param(m_networkUuidParamTypeIds[thingClassId], networkUuid.toString()));
    params.append(Param(m_ieeeAddressParamTypeIds[thingClassId], node->extendedAddress().toString()));
    descriptor.setParams(params);
    emit autoThingsAppeared({descriptor});
}

QString IntegrationPluginZigbeeDevelco::parseDevelcoVersionString(ZigbeeNodeEndpoint *endpoint)
{
    QString versionString;
    ZigbeeCluster *basicCluster = endpoint->getInputCluster(ZigbeeClusterLibrary::ClusterIdBasic);
    if (!basicCluster) {
        qCWarning(dcZigbeeDevelco()) << "Could not find basic cluster on" << endpoint << "for version parsing";
        return versionString;
    }

    if (!basicCluster->hasAttribute(DEVELCO_ATTRIBUTE_SW_VERSION)) {
        qCWarning(dcZigbeeDevelco()) << "Could not find manufacturer specific develco software version attribute in basic cluster on" << endpoint;
        return versionString;
    }

    ZigbeeClusterAttribute versionAttribute = basicCluster->attribute(DEVELCO_ATTRIBUTE_SW_VERSION);
    // 1 Byte octet string length, 3 byte version infromation
    if (versionAttribute.dataType().data().length() < 4 || versionAttribute.dataType().data().at(0) != 3) {
        qCWarning(dcZigbeeDevelco()) << "Failed to parse version string from manufacturer specific develco software version attribute" << versionAttribute;
        return versionString;
    }

    int majorVersion = static_cast<int>(versionAttribute.dataType().data().at(1));
    int minorVersion = static_cast<int>(versionAttribute.dataType().data().at(2));
    int patchVersion = static_cast<int>(versionAttribute.dataType().data().at(3));
    versionString = QString("%1.%2.%3").arg(majorVersion).arg(minorVersion).arg(patchVersion);
    //qCDebug(dcZigbeeDevelco()) << versionAttribute << ZigbeeUtils::convertByteArrayToHexString(versionAttribute.dataType().data()) << versionString;

    return versionString;
}

void IntegrationPluginZigbeeDevelco::initIoModule(ZigbeeNode *node)
{
    qCDebug(dcZigbeeDevelco()) << "Start initializing IO Module" << node;
    readDevelcoFirmwareVersion(node, node->getEndpoint(IO_MODULE_EP_INPUT1));

    // Binding and reporting outputs
    configureOnOffPowerReporting(node, node->getEndpoint(IO_MODULE_EP_OUTPUT1));
    configureOnOffPowerReporting(node, node->getEndpoint(IO_MODULE_EP_OUTPUT2));

    // Binding and reporting inputs
    configureBinaryInputReporting(node, node->getEndpoint(IO_MODULE_EP_INPUT1));
    configureBinaryInputReporting(node, node->getEndpoint(IO_MODULE_EP_INPUT2));
    configureBinaryInputReporting(node, node->getEndpoint(IO_MODULE_EP_INPUT3));
    configureBinaryInputReporting(node, node->getEndpoint(IO_MODULE_EP_INPUT4));
}

void IntegrationPluginZigbeeDevelco::configureOnOffPowerReporting(ZigbeeNode *node, ZigbeeNodeEndpoint *endpoint)
{
    qCDebug(dcZigbeeDevelco()) << "Bind on/off cluster to coordinator IEEE address" << node << endpoint;
    ZigbeeDeviceObjectReply * zdoReply = node->deviceObject()->requestBindIeeeAddress(endpoint->endpointId(), ZigbeeClusterLibrary::ClusterIdOnOff, hardwareManager()->zigbeeResource()->coordinatorAddress(node->networkUuid()), 0x01);
    connect(zdoReply, &ZigbeeDeviceObjectReply::finished, node, [=](){
        if (zdoReply->error() != ZigbeeDeviceObjectReply::ErrorNoError) {
            qCWarning(dcZigbeeDevelco()) << "Failed to bind on/off cluster to coordinator" << zdoReply->error();
        } else {
            qCDebug(dcZigbeeDevelco()) << "Bind on/off cluster to coordinator finished successfully";
        }

        // Configure attribute reporting for lock state
        ZigbeeClusterLibrary::AttributeReportingConfiguration reportingConfig;
        reportingConfig.attributeId = ZigbeeClusterOnOff::AttributeOnOff;
        reportingConfig.minReportingInterval = 0;
        reportingConfig.maxReportingInterval = 600;
        reportingConfig.dataType = Zigbee::Bool;

        qCDebug(dcZigbeeDevelco()) << "Configure attribute reporting for on/off cluster" << node << endpoint;
        ZigbeeClusterReply *reportingReply = endpoint->getInputCluster(ZigbeeClusterLibrary::ClusterIdOnOff)->configureReporting({reportingConfig});
        connect(reportingReply, &ZigbeeClusterReply::finished, this, [=](){
            if (reportingReply->error() != ZigbeeClusterReply::ErrorNoError) {
                qCWarning(dcZigbeeDevelco()) << "Failed configure attribute reporting on on/off cluster" << reportingReply->error();
            } else {
                qCDebug(dcZigbeeDevelco()) << "Attribute reporting configuration finished for on/off cluster" << ZigbeeClusterLibrary::parseAttributeReportingStatusRecords(reportingReply->responseFrame().payload);
            }
        });
    });
}

void IntegrationPluginZigbeeDevelco::configureBinaryInputReporting(ZigbeeNode *node, ZigbeeNodeEndpoint *endpoint)
{
    qCDebug(dcZigbeeDevelco()) << "Bind binary input cluster to coordinator IEEE address" << node << endpoint;
    ZigbeeDeviceObjectReply * zdoReply = node->deviceObject()->requestBindIeeeAddress(endpoint->endpointId(), ZigbeeClusterLibrary::ClusterIdBinaryInput, hardwareManager()->zigbeeResource()->coordinatorAddress(node->networkUuid()), 0x01);
    connect(zdoReply, &ZigbeeDeviceObjectReply::finished, node, [=](){
        if (zdoReply->error() != ZigbeeDeviceObjectReply::ErrorNoError) {
            qCWarning(dcZigbeeDevelco()) << "Failed to bind binary input cluster to coordinator" << zdoReply->error();
        } else {
            qCDebug(dcZigbeeDevelco()) << "Bind binary input cluster to coordinator finished successfully";
        }

        // Configure attribute reporting for lock state
        ZigbeeClusterLibrary::AttributeReportingConfiguration reportingConfig;
        reportingConfig.attributeId = ZigbeeClusterBinaryInput::AttributePresentValue;
        reportingConfig.minReportingInterval = 0;
        reportingConfig.maxReportingInterval = 600;
        reportingConfig.dataType = Zigbee::Bool;

        qCDebug(dcZigbeeDevelco()) << "Configure attribute reporting for binary input cluster" << node << endpoint;
        ZigbeeClusterReply *reportingReply = endpoint->getInputCluster(ZigbeeClusterLibrary::ClusterIdBinaryInput)->configureReporting({reportingConfig});
        connect(reportingReply, &ZigbeeClusterReply::finished, this, [=](){
            if (reportingReply->error() != ZigbeeClusterReply::ErrorNoError) {
                qCWarning(dcZigbeeDevelco()) << "Failed configure attribute reporting on binary input cluster" << reportingReply->error();
            } else {
                qCDebug(dcZigbeeDevelco()) << "Attribute reporting configuration finished for on binary input cluster" << ZigbeeClusterLibrary::parseAttributeReportingStatusRecords(reportingReply->responseFrame().payload);
            }
        });
    });
}

void IntegrationPluginZigbeeDevelco::readDevelcoFirmwareVersion(ZigbeeNode *node, ZigbeeNodeEndpoint *endpoint)
{
    // Read manufacturer specific basic cluster attribute 0x8000
    ZigbeeClusterBasic *basicCluster = endpoint->inputCluster<ZigbeeClusterBasic>(ZigbeeClusterLibrary::ClusterIdBasic);
    if (!basicCluster) {
        qCWarning(dcZigbeeDevelco()) << "Could not find basic cluster for manufacturer specific attribute reading on" << node << endpoint;
        return;
    }

    // We have to read the color capabilities
    ZigbeeClusterReply *reply = basicCluster->readAttributes({DEVELCO_ATTRIBUTE_SW_VERSION}, Zigbee::Develco);
    connect(reply, &ZigbeeClusterReply::finished, node, [=](){
        if (reply->error() != ZigbeeClusterReply::ErrorNoError) {
            qCWarning(dcZigbeeDevelco()) << "Failed to read manufacturer specific version attribute on" << node << endpoint << basicCluster;
            return;
        }

        qCDebug(dcZigbeeDevelco()) << "Reading develco manufacturer specific version attributes finished successfully";
    });
}

void IntegrationPluginZigbeeDevelco::readOnOffPowerAttribute(ZigbeeNode *node, ZigbeeNodeEndpoint *endpoint)
{
    qCDebug(dcZigbeeDevelco()) << "Reading power states of" << node << "on" << endpoint;

    ZigbeeClusterOnOff *onOffCluster = endpoint->inputCluster<ZigbeeClusterOnOff>(ZigbeeClusterLibrary::ClusterIdOnOff);
    if (!onOffCluster) {
        qCWarning(dcZigbeeDevelco()) << "Could not find On/Off cluster on" << node << endpoint;
    } else {
        ZigbeeClusterReply *reply = onOffCluster->readAttributes({ZigbeeClusterOnOff::AttributeOnOff});
        connect(reply, &ZigbeeClusterReply::finished, node, [=](){
            if (reply->error() != ZigbeeClusterReply::ErrorNoError) {
                qCWarning(dcZigbeeDevelco()) << "Failed to read On/Off power attribute from" << node << endpoint << onOffCluster;
            }
            // Will be updated trough the attribute changed signal
        });
    }
}

void IntegrationPluginZigbeeDevelco::readBinaryInputPresentValueAttribute(ZigbeeNode *node, ZigbeeNodeEndpoint *endpoint)
{
    ZigbeeClusterBinaryInput *binaryInputCluster = endpoint->inputCluster<ZigbeeClusterBinaryInput>(ZigbeeClusterLibrary::ClusterIdBinaryInput);
    if (!binaryInputCluster) {
        qCWarning(dcZigbeeDevelco()) << "Could not find BinaryInput cluster on" << node << endpoint;
    } else {
        ZigbeeClusterReply *reply = binaryInputCluster->readAttributes({ZigbeeClusterBinaryInput::AttributePresentValue});
        connect(reply, &ZigbeeClusterReply::finished, node, [=](){
            if (reply->error() != ZigbeeClusterReply::ErrorNoError) {
                qCWarning(dcZigbeeDevelco()) << "Failed to read binary input value attribute from" << node << endpoint << binaryInputCluster;
            }
            // Will be updated trough the attribute changed signal
        });
    }
}

void IntegrationPluginZigbeeDevelco::readIoModuleOutputPowerStates(Thing *thing)
{
    ZigbeeNode *node = m_thingNodes.value(thing);
    if (!node) {
        qCWarning(dcZigbeeDevelco()) << "Could not find zigbee node for" << thing;
        return;
    }

    qCDebug(dcZigbeeDevelco()) << "Start reading power states of" << thing << node;

    // Read output 1 power state
    ZigbeeNodeEndpoint *output1Endpoint = node->getEndpoint(IO_MODULE_EP_OUTPUT1);
    if (!output1Endpoint) {
        qCWarning(dcZigbeeDevelco()) << "Could not find endpoint for output 1 on" << thing << node;
    } else {
        readOnOffPowerAttribute(node, output1Endpoint);
    }

    // Read output 2 power state
    ZigbeeNodeEndpoint *output2Endpoint = node->getEndpoint(IO_MODULE_EP_OUTPUT2);
    if (!output2Endpoint) {
        qCWarning(dcZigbeeDevelco()) << "Could not find endpoint for output 2 on" << thing << node;
    } else {
        readOnOffPowerAttribute(node, output2Endpoint);
    }
}

void IntegrationPluginZigbeeDevelco::readIoModuleInputPowerStates(Thing *thing)
{
    ZigbeeNode *node = m_thingNodes.value(thing);
    if (!node) {
        qCWarning(dcZigbeeDevelco()) << "Could not find zigbee node for" << thing;
        return;
    }

    // Read input 1 state
    ZigbeeNodeEndpoint *input1Endpoint = node->getEndpoint(IO_MODULE_EP_INPUT1);
    if (!input1Endpoint) {
        qCWarning(dcZigbeeDevelco()) << "Could not find endpoint for input 1 on" << thing << node;
    } else {
        readBinaryInputPresentValueAttribute(node, input1Endpoint);
    }

    // Read input 2 state
    ZigbeeNodeEndpoint *input2Endpoint = node->getEndpoint(IO_MODULE_EP_INPUT2);
    if (!input2Endpoint) {
        qCWarning(dcZigbeeDevelco()) << "Could not find endpoint for input 2 on" << thing << node;
    } else {
        readBinaryInputPresentValueAttribute(node, input2Endpoint);
    }

    // Read input 3 state
    ZigbeeNodeEndpoint *input3Endpoint = node->getEndpoint(IO_MODULE_EP_INPUT3);
    if (!input3Endpoint) {
        qCWarning(dcZigbeeDevelco()) << "Could not find endpoint for input 3 on" << thing << node;
    } else {
        readBinaryInputPresentValueAttribute(node, input3Endpoint);
    }

    // Read input 4 state
    ZigbeeNodeEndpoint *input4Endpoint = node->getEndpoint(IO_MODULE_EP_INPUT4);
    if (!input4Endpoint) {
        qCWarning(dcZigbeeDevelco()) << "Could not find endpoint for input 4 on" << thing << node;
    } else {
        readBinaryInputPresentValueAttribute(node, input4Endpoint);
    }
}

