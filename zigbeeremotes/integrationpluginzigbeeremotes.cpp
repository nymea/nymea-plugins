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

#include "integrationpluginzigbeeremotes.h"
#include "plugininfo.h"
#include "hardware/zigbee/zigbeehardwareresource.h"

#include <QDebug>


IntegrationPluginZigbeeRemotes::IntegrationPluginZigbeeRemotes()
{
}

QString IntegrationPluginZigbeeRemotes::name() const
{
    return "Remotes";
}

void IntegrationPluginZigbeeRemotes::init()
{
    hardwareManager()->zigbeeResource()->registerHandler(this, ZigbeeHardwareResource::HandlerTypeVendor);
}

bool IntegrationPluginZigbeeRemotes::handleNode(ZigbeeNode *node, const QUuid &networkUuid)
{
    qCDebug(dcZigbeeRemotes) << "Evaluating node:" << node << node->nodeDescriptor().manufacturerCode << node->modelName();
    bool handled = false;
    // "Insta" remote (JUNG ZLL 5004)
    if (node->nodeDescriptor().manufacturerCode == 0x117A && node->modelName() == " Remote") {
        ZigbeeNodeEndpoint *endpoint = node->getEndpoint(0x01);
        if (!endpoint) {
            qCWarning(dcZigbeeRemotes()) << "Device claims to be an Insta remote but does not provide endpoint 1";
            return false;
        }

        createThing(instaThingClassId, networkUuid, node, endpoint);

        // Nothing to be done here... The device does not support battery level updates and will send all the commands
        // to the coordinator unconditionally, no need to bind any clusters...

        handled = true;
    }

    return handled;
}

void IntegrationPluginZigbeeRemotes::handleRemoveNode(ZigbeeNode *node, const QUuid &networkUuid)
{
    Q_UNUSED(networkUuid)
    Thing *thing = m_thingNodes.key(node);
    if (thing) {
        qCDebug(dcZigbeeRemotes()) << node << "for" << thing << "has left the network.";
        emit autoThingDisappeared(thing->id());

        // Removing it from our map to prevent a loop that would ask the zigbee network to remove this node (see thingRemoved())
        m_thingNodes.remove(thing);
    }
}

void IntegrationPluginZigbeeRemotes::setupThing(ThingSetupInfo *info)
{
    Thing *thing = info->thing();
    QUuid networkUuid = thing->paramValue("networkUuid").toUuid();
    qCDebug(dcZigbeeRemotes()) << "Nework uuid:" << networkUuid;
    ZigbeeAddress zigbeeAddress = ZigbeeAddress(thing->paramValue("ieeeAddress").toString());
    ZigbeeNode *node = m_thingNodes.value(thing);
    if (!node) {
        node = hardwareManager()->zigbeeResource()->claimNode(this, networkUuid, zigbeeAddress);
    }

    if (!node) {
        qCWarning(dcZigbeeRemotes()) << "Zigbee node for" << info->thing()->name() << "not found.";
        info->finish(Thing::ThingErrorHardwareNotAvailable);
        return;
    }
    m_thingNodes.insert(thing, node);

    // Update connected state
    thing->setStateValue("connected", node->reachable());
    connect(node, &ZigbeeNode::reachableChanged, thing, [thing](bool reachable){
        thing->setStateValue("connected", reachable);
    });

    // Update signal strength
    thing->setStateValue("signalStrength", qRound(node->lqi() * 100.0 / 255.0));
    connect(node, &ZigbeeNode::lqiChanged, thing, [thing](quint8 lqi){
        uint signalStrength = qRound(lqi * 100.0 / 255.0);
        qCDebug(dcZigbeeRemotes()) << thing << "signal strength changed" << signalStrength << "%";
        thing->setStateValue("signalStrength", signalStrength);
    });

    // Type specific setup
    if (thing->thingClassId() == instaThingClassId) {
        ZigbeeNodeEndpoint *endpoint = node->getEndpoint(0x01);

        ZigbeeClusterOnOff *onOffCluster = endpoint->outputCluster<ZigbeeClusterOnOff>(ZigbeeClusterLibrary::ClusterIdOnOff);
        ZigbeeClusterLevelControl *levelControlCluster = endpoint->outputCluster<ZigbeeClusterLevelControl>(ZigbeeClusterLibrary::ClusterIdLevelControl);
        ZigbeeClusterScenes *scenesCluster = endpoint->outputCluster<ZigbeeClusterScenes>(ZigbeeClusterLibrary::ClusterIdScenes);
        if (!onOffCluster || !levelControlCluster || !scenesCluster) {
            qCWarning(dcZigbeeRemotes()) << "Could not find all of the needed clusters for" << thing->name() << "in" << m_thingNodes.value(thing) << "on endpoint" << endpoint->endpointId();
            info->finish(Thing::ThingErrorHardwareNotAvailable);
            return;
        }
        connect(onOffCluster, &ZigbeeClusterOnOff::commandSent, this, [=](ZigbeeClusterOnOff::Command command, const QByteArray &parameters){
            qCDebug(dcZigbeeRemotes()) << "OnOff command received:" << command << parameters;
            switch (command) {
            case ZigbeeClusterOnOff::CommandOn:
                thing->emitEvent(instaPressedEventTypeId, {Param(instaPressedEventButtonNameParamTypeId, "ON")});
                break;
            case ZigbeeClusterOnOff::CommandOffWithEffect:
                thing->emitEvent(instaPressedEventTypeId, {Param(instaPressedEventButtonNameParamTypeId, "OFF")});
                break;
            default:
                qCWarning(dcZigbeeRemotes()) << "Unhandled command from Insta Remote:" << command << parameters.toHex();
            }
        });
        connect(levelControlCluster, &ZigbeeClusterLevelControl::commandStepSent, this, [=](bool withOnOff, ZigbeeClusterLevelControl::StepMode stepMode, quint8 stepSize, quint16 transitionTime){
            qCDebug(dcZigbeeRemotes()) << "Level command received" << withOnOff << stepMode << stepSize << transitionTime;
            thing->emitEvent(instaPressedEventTypeId, {Param(instaPressedEventButtonNameParamTypeId, stepMode == ZigbeeClusterLevelControl::StepModeUp ? "+" : "-")});
        });
        connect(scenesCluster, &ZigbeeClusterScenes::commandSent, this, [=](ZigbeeClusterScenes::Command command, quint16 groupId, quint8 sceneId){
            qCDebug(dcZigbeeRemotes()) << "Scenes command received:" << command << groupId << sceneId;
            thing->emitEvent(instaPressedEventTypeId, {Param(instaPressedEventButtonNameParamTypeId, QString::number(sceneId))});
        });


        // The device also supports setting saturation, color and color temperature. However, it's quite funky to
        // actually get there on the device and that mode seems to be only enabled if there are bindings to
        // actual lamps. Once it's bound to lamps, pressing on and off simultaneously will start cycling through the bound
        // lights and during that mode, the color/saturation/temperature will act on the currently selected lamp only.
        // After some seconds without button press, it will revert back to the default mode where it sends all commands
        // to the coordinator *and* all the bound lights simultaneously.
        // So, in order to get that working we'd need to fake a like and somehow allow binding that via touch-link from a key-combo on the device.

        // Not supporting that here... A user may still additionally bind the device to a lamp and use that feature with the remote....

        info->finish(Thing::ThingErrorNoError);
        return;
    }

    info->finish(Thing::ThingErrorNoError);
}

void IntegrationPluginZigbeeRemotes::thingRemoved(Thing *thing)
{
    ZigbeeNode *node = m_thingNodes.take(thing);
    if (node) {
        QUuid networkUuid = thing->paramValue("networkUuid").toUuid();
        hardwareManager()->zigbeeResource()->removeNodeFromNetwork(networkUuid, node);
    }
}

void IntegrationPluginZigbeeRemotes::createThing(const ThingClassId &thingClassId, const QUuid &networkUuid, ZigbeeNode *node, ZigbeeNodeEndpoint *endpoint)
{
    ThingDescriptor descriptor(thingClassId);
    ThingClass thingClass = supportedThings().findById(thingClassId);
    descriptor.setTitle(QString("%1 (%2 - %3)").arg(thingClass.displayName()).arg(endpoint->manufacturerName()).arg(endpoint->modelIdentifier()));

    ParamList params;
    params.append(Param(thingClass.paramTypes().findByName("networkUuid").id(), networkUuid.toString()));
    params.append(Param(thingClass.paramTypes().findByName("ieeeAddress").id(), node->extendedAddress().toString()));
    descriptor.setParams(params);
    emit autoThingsAppeared({descriptor});
}

void IntegrationPluginZigbeeRemotes::bindPowerConfigurationCluster(ZigbeeNode *node, ZigbeeNodeEndpoint *endpoint)
{
    ZigbeeDeviceObjectReply *bindPowerReply = node->deviceObject()->requestBindIeeeAddress(endpoint->endpointId(), ZigbeeClusterLibrary::ClusterIdPowerConfiguration,
                                                                                           hardwareManager()->zigbeeResource()->coordinatorAddress(node->networkUuid()), 0x01);
    connect(bindPowerReply, &ZigbeeDeviceObjectReply::finished, node, [=](){
        if (bindPowerReply->error() != ZigbeeDeviceObjectReply::ErrorNoError) {
            qCWarning(dcZigbeeRemotes()) << "Failed to bind power configuration cluster" << bindPowerReply->error();
        } else {
            qCDebug(dcZigbeeRemotes()) << "Binding power configuration cluster finished successfully";
        }

        ZigbeeClusterLibrary::AttributeReportingConfiguration batteryPercentageConfig;
        batteryPercentageConfig.attributeId = ZigbeeClusterPowerConfiguration::AttributeBatteryPercentageRemaining;
        batteryPercentageConfig.dataType = Zigbee::Uint8;
        batteryPercentageConfig.minReportingInterval = 300;
        batteryPercentageConfig.maxReportingInterval = 2700;
        batteryPercentageConfig.reportableChange = ZigbeeDataType(static_cast<quint8>(1)).data();

        qCDebug(dcZigbeeRemotes()) << "Configuring attribute reporting for power configuration cluster";
        ZigbeeClusterReply *reportingReply = endpoint->getInputCluster(ZigbeeClusterLibrary::ClusterIdPowerConfiguration)->configureReporting({batteryPercentageConfig});
        connect(reportingReply, &ZigbeeClusterReply::finished, this, [=](){
            if (reportingReply->error() != ZigbeeClusterReply::ErrorNoError) {
                qCWarning(dcZigbeeRemotes()) << "Failed to configure power configuration cluster attribute reporting" << reportingReply->error();
            } else {
                qCDebug(dcZigbeeRemotes()) << "Attribute reporting configuration finished for power configuration cluster" << ZigbeeClusterLibrary::parseAttributeReportingStatusRecords(reportingReply->responseFrame().payload);
            }
        });
    });
}

void IntegrationPluginZigbeeRemotes::bindOnOffCluster(ZigbeeNode *node, ZigbeeNodeEndpoint *endpoint)
{
    ZigbeeDeviceObjectReply *bindOnOffClusterReply = node->deviceObject()->requestBindIeeeAddress(endpoint->endpointId(), ZigbeeClusterLibrary::ClusterIdOnOff,
                                                                                           hardwareManager()->zigbeeResource()->coordinatorAddress(node->networkUuid()), 0x01);
    connect(bindOnOffClusterReply, &ZigbeeDeviceObjectReply::finished, node, [=](){
        if (bindOnOffClusterReply->error() != ZigbeeDeviceObjectReply::ErrorNoError) {
            qCWarning(dcZigbeeRemotes()) << "Failed to bind on/off cluster" << bindOnOffClusterReply->error();
        } else {
            qCDebug(dcZigbeeRemotes()) << "Bound on/off cluster successfully";
        }
    });
}

void IntegrationPluginZigbeeRemotes::bindLevelControlCluster(ZigbeeNode *node, ZigbeeNodeEndpoint *endpoint)
{
    ZigbeeDeviceObjectReply *bindLevelControlClusterReply = node->deviceObject()->requestBindIeeeAddress(endpoint->endpointId(), ZigbeeClusterLibrary::ClusterIdLevelControl,
                                                                                           hardwareManager()->zigbeeResource()->coordinatorAddress(node->networkUuid()), 0x01);
    connect(bindLevelControlClusterReply, &ZigbeeDeviceObjectReply::finished, node, [=](){
        if (bindLevelControlClusterReply->error() != ZigbeeDeviceObjectReply::ErrorNoError) {
            qCWarning(dcZigbeeRemotes()) << "Failed to bind level control cluster" << bindLevelControlClusterReply->error();
        } else {
            qCDebug(dcZigbeeRemotes()) << "Bound level control cluster successfully";
        }
    });
}

void IntegrationPluginZigbeeRemotes::bindScenesCluster(ZigbeeNode *node, ZigbeeNodeEndpoint *endpoint)
{
    ZigbeeDeviceObjectReply *bindScenesClusterReply = node->deviceObject()->requestBindIeeeAddress(endpoint->endpointId(), ZigbeeClusterLibrary::ClusterIdScenes,
                                                                                           hardwareManager()->zigbeeResource()->coordinatorAddress(node->networkUuid()), 0x01);
    connect(bindScenesClusterReply, &ZigbeeDeviceObjectReply::finished, node, [=](){
        if (bindScenesClusterReply->error() != ZigbeeDeviceObjectReply::ErrorNoError) {
            qCWarning(dcZigbeeRemotes()) << "Failed to bind on/off cluster" << bindScenesClusterReply->error();
        } else {
            qCDebug(dcZigbeeRemotes()) << "Bound on/off cluster successfully";
        }
    });
}

void IntegrationPluginZigbeeRemotes::connectToPowerConfigurationCluster(Thing *thing, ZigbeeNodeEndpoint *endpoint)
{
    // Get battery level changes
    ZigbeeClusterPowerConfiguration *powerCluster = endpoint->inputCluster<ZigbeeClusterPowerConfiguration>(ZigbeeClusterLibrary::ClusterIdPowerConfiguration);
    if (powerCluster) {
        // If the power cluster attributes are already available, read values now
        if (powerCluster->hasAttribute(ZigbeeClusterPowerConfiguration::AttributeBatteryPercentageRemaining)) {
            thing->setStateValue("batteryLevel", powerCluster->batteryPercentage());
            thing->setStateValue("batteryCritical", (powerCluster->batteryPercentage() < 10.0));
        }
        // Refresh power cluster attributes in any case
        ZigbeeClusterReply *reply = powerCluster->readAttributes({ZigbeeClusterPowerConfiguration::AttributeBatteryPercentageRemaining});
        connect(reply, &ZigbeeClusterReply::finished, thing, [=](){
            if (reply->error() != ZigbeeClusterReply::ErrorNoError) {
                qCWarning(dcZigbeeRemotes()) << "Reading power configuration cluster attributes finished with error" << reply->error();
                return;
            }
            thing->setStateValue("batteryLevel", powerCluster->batteryPercentage());
            thing->setStateValue("batteryCritical", (powerCluster->batteryPercentage() < 10.0));
        });

        // Connect to battery level changes
        connect(powerCluster, &ZigbeeClusterPowerConfiguration::batteryPercentageChanged, thing, [=](double percentage){
            thing->setStateValue("batteryLevel", percentage);
            thing->setStateValue("batteryCritical", (percentage < 10.0));
        });
    }
}
