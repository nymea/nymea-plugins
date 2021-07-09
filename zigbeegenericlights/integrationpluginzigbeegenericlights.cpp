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

#include <zigbeeutils.h>

IntegrationPluginZigbeeGenericLights::IntegrationPluginZigbeeGenericLights()
{
    // Common thing params map
    m_ieeeAddressParamTypeIds[onOffLightThingClassId] = onOffLightThingIeeeAddressParamTypeId;
    m_ieeeAddressParamTypeIds[dimmableLightThingClassId] = dimmableLightThingIeeeAddressParamTypeId;
    m_ieeeAddressParamTypeIds[colorTemperatureLightThingClassId] = colorTemperatureLightThingIeeeAddressParamTypeId;
    m_ieeeAddressParamTypeIds[colorLightThingClassId] = colorLightThingIeeeAddressParamTypeId;

    m_networkUuidParamTypeIds[onOffLightThingClassId] = onOffLightThingNetworkUuidParamTypeId;
    m_networkUuidParamTypeIds[dimmableLightThingClassId] = dimmableLightThingNetworkUuidParamTypeId;
    m_networkUuidParamTypeIds[colorTemperatureLightThingClassId] = colorTemperatureLightThingNetworkUuidParamTypeId;
    m_networkUuidParamTypeIds[colorLightThingClassId] = colorLightThingNetworkUuidParamTypeId;

    m_endpointIdParamTypeIds[onOffLightThingClassId] = onOffLightThingEndpointIdParamTypeId;
    m_endpointIdParamTypeIds[dimmableLightThingClassId] = dimmableLightThingEndpointIdParamTypeId;
    m_endpointIdParamTypeIds[colorTemperatureLightThingClassId] = colorTemperatureLightThingEndpointIdParamTypeId;
    m_endpointIdParamTypeIds[colorLightThingClassId] = colorLightThingEndpointIdParamTypeId;

    m_manufacturerIdParamTypeIds[onOffLightThingClassId] = onOffLightThingManufacturerParamTypeId;
    m_manufacturerIdParamTypeIds[dimmableLightThingClassId] = dimmableLightThingManufacturerParamTypeId;
    m_manufacturerIdParamTypeIds[colorTemperatureLightThingClassId] = colorTemperatureLightThingManufacturerParamTypeId;
    m_manufacturerIdParamTypeIds[colorLightThingClassId] = colorLightThingManufacturerParamTypeId;

    m_modelIdParamTypeIds[onOffLightThingClassId] = onOffLightThingModelParamTypeId;
    m_modelIdParamTypeIds[dimmableLightThingClassId] = dimmableLightThingModelParamTypeId;
    m_modelIdParamTypeIds[colorTemperatureLightThingClassId] = colorTemperatureLightThingModelParamTypeId;
    m_modelIdParamTypeIds[colorLightThingClassId] = colorLightThingModelParamTypeId;


    // Common states map
    m_connectedStateTypeIds[onOffLightThingClassId] = onOffLightConnectedStateTypeId;
    m_connectedStateTypeIds[dimmableLightThingClassId] = dimmableLightConnectedStateTypeId;
    m_connectedStateTypeIds[colorTemperatureLightThingClassId] = colorTemperatureLightConnectedStateTypeId;
    m_connectedStateTypeIds[colorLightThingClassId] = colorLightConnectedStateTypeId;

    m_signalStrengthStateTypeIds[onOffLightThingClassId] = onOffLightSignalStrengthStateTypeId;
    m_signalStrengthStateTypeIds[dimmableLightThingClassId] = dimmableLightSignalStrengthStateTypeId;
    m_signalStrengthStateTypeIds[colorTemperatureLightThingClassId] = colorTemperatureLightSignalStrengthStateTypeId;
    m_signalStrengthStateTypeIds[colorLightThingClassId] = colorLightSignalStrengthStateTypeId;

    m_versionStateTypeIds[onOffLightThingClassId] = onOffLightVersionStateTypeId;
    m_versionStateTypeIds[dimmableLightThingClassId] = dimmableLightVersionStateTypeId;
    m_versionStateTypeIds[colorTemperatureLightThingClassId] = colorTemperatureLightVersionStateTypeId;
    m_versionStateTypeIds[colorLightThingClassId] = colorLightVersionStateTypeId;
}

QString IntegrationPluginZigbeeGenericLights::name() const
{
    return "Generic lights";
}

bool IntegrationPluginZigbeeGenericLights::handleNode(ZigbeeNode *node, const QUuid &networkUuid)
{
    bool handled = false;
    foreach (ZigbeeNodeEndpoint *endpoint, node->endpoints()) {
        if ((endpoint->profile() == Zigbee::ZigbeeProfile::ZigbeeProfileLightLink &&
             endpoint->deviceId() == Zigbee::LightLinkDevice::LightLinkDeviceOnOffLight) ||
                (endpoint->profile() == Zigbee::ZigbeeProfile::ZigbeeProfileHomeAutomation &&
                 endpoint->deviceId() == Zigbee::HomeAutomationDeviceOnOffLight)) {

            qCDebug(dcZigbeeGenericLights()) << "Handling on/off light for" << node << endpoint;
            createLightThing(onOffLightThingClassId, networkUuid, node, endpoint);
            handled = true;
        }

        if ((endpoint->profile() == Zigbee::ZigbeeProfile::ZigbeeProfileLightLink &&
             endpoint->deviceId() == Zigbee::LightLinkDevice::LightLinkDeviceDimmableLight) ||
                (endpoint->profile() == Zigbee::ZigbeeProfile::ZigbeeProfileHomeAutomation &&
                 endpoint->deviceId() == Zigbee::HomeAutomationDeviceDimmableLight)) {

            qCDebug(dcZigbeeGenericLights()) << "Handling dimmable light for" << node << endpoint;
            createLightThing(dimmableLightThingClassId, networkUuid, node, endpoint);
            handled = true;
        }

        if ((endpoint->profile() == Zigbee::ZigbeeProfileLightLink &&
             endpoint->deviceId() == Zigbee::LightLinkDeviceColourTemperatureLight) ||
                (endpoint->profile() == Zigbee::ZigbeeProfileHomeAutomation &&
                 endpoint->deviceId() == Zigbee::HomeAutomationDeviceColourTemperatureLight)) {

            qCDebug(dcZigbeeGenericLights()) << "Handling color temperature light for" << node << endpoint;
            createLightThing(colorTemperatureLightThingClassId, networkUuid, node, endpoint);
            handled = true;
        }

        if ((endpoint->profile() == Zigbee::ZigbeeProfileLightLink && endpoint->deviceId() == Zigbee::LightLinkDeviceColourLight) ||
                (endpoint->profile() == Zigbee::ZigbeeProfileLightLink && endpoint->deviceId() == Zigbee::LightLinkDeviceExtendedColourLight) ||
                (endpoint->profile() == Zigbee::ZigbeeProfileHomeAutomation && endpoint->deviceId() == Zigbee::HomeAutomationDeviceExtendedColourLight)) {

            qCDebug(dcZigbeeGenericLights()) << "Handling color light for" << node << endpoint;
            createLightThing(colorLightThingClassId, networkUuid, node, endpoint);
            handled = true;
        }
    }

    return handled;
}

void IntegrationPluginZigbeeGenericLights::handleRemoveNode(ZigbeeNode *node, const QUuid &networkUuid)
{
    Q_UNUSED(networkUuid)

    if (m_thingNodes.values().contains(node)) {
        Thing *thing = m_thingNodes.key(node);
        qCDebug(dcZigbeeGenericLights()) << node << "for" << thing << "has left the network.";
        m_thingNodes.remove(thing);
        emit autoThingDisappeared(thing->id());

        if (m_colorTemperatureRanges.contains(thing)) {
            m_colorTemperatureRanges.remove(thing);
        }

        if (m_colorCapabilities.contains(thing)) {
            m_colorCapabilities.remove(thing);
        }
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
    ZigbeeNode *node = hardwareManager()->zigbeeResource()->claimNode(this, networkUuid, zigbeeAddress);
    if (!node) {
        qCWarning(dcZigbeeGenericLights()) << "Zigbee node for" << info->thing()->name() << "not found.";
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

    // On/Off light
    if (thing->thingClassId() == onOffLightThingClassId) {

        // Get the on/off cluster
        ZigbeeClusterOnOff *onOffCluster = endpoint->inputCluster<ZigbeeClusterOnOff>(ZigbeeClusterLibrary::ClusterIdOnOff);
        if (!onOffCluster) {
            qCWarning(dcZigbeeGenericLights()) << "Could not find on/off cluster for" << thing << "in" << node;
            info->finish(Thing::ThingErrorHardwareFailure);
            return;
        }

        if (onOffCluster->hasAttribute(ZigbeeClusterOnOff::AttributeOnOff)) {
            thing->setStateValue(onOffLightPowerStateTypeId, onOffCluster->power());
        }

        // Update the power state if the node power value changes
        connect(onOffCluster, &ZigbeeClusterOnOff::powerChanged, thing, [thing](bool power){
            qCDebug(dcZigbeeGenericLights()) << thing << "power state changed" << power;
            thing->setStateValue(onOffLightPowerStateTypeId, power);
        });

        // Read the power state once the node gets reachable
        connect(node, &ZigbeeNode::reachableChanged, thing, [=](bool reachable){
            if (reachable) {
                configureLightPowerReporting(node, endpoint);
                readLightPowerState(thing);
            }
        });

        if (node->reachable()) {
            configureLightPowerReporting(node, endpoint);
        }
    }

    // Dimmable light
    if (thing->thingClassId() == dimmableLightThingClassId) {

        // Get the on/off cluster
        ZigbeeClusterOnOff *onOffCluster = endpoint->inputCluster<ZigbeeClusterOnOff>(ZigbeeClusterLibrary::ClusterIdOnOff);
        if (!onOffCluster) {
            qCWarning(dcZigbeeGenericLights()) << "Could not find on/off cluster for" << thing << "in" << node;
            info->finish(Thing::ThingErrorHardwareFailure);
            return;
        }

        // Only set the state if the cluster actually has the attribute
        if (onOffCluster->hasAttribute(ZigbeeClusterOnOff::AttributeOnOff)) {
            thing->setStateValue(dimmableLightPowerStateTypeId, onOffCluster->power());
        }

        // Update the power state if the node power value changes
        connect(onOffCluster, &ZigbeeClusterOnOff::powerChanged, thing, [thing](bool power){
            qCDebug(dcZigbeeGenericLights()) << thing << "power state changed" << power;
            thing->setStateValue(dimmableLightPowerStateTypeId, power);
        });


        // Get the level cluster
        ZigbeeClusterLevelControl *levelCluster = endpoint->inputCluster<ZigbeeClusterLevelControl>(ZigbeeClusterLibrary::ClusterIdLevelControl);
        if (!levelCluster) {
            qCWarning(dcZigbeeGenericLights()) << "Could not find level cluster for" << thing << "in" << node;
            info->finish(Thing::ThingErrorHardwareFailure);
            return;
        }

        // Only set the state if the cluster actually has the attribute
        if (levelCluster->hasAttribute(ZigbeeClusterLevelControl::AttributeCurrentLevel)) {
            int percentage = qRound(levelCluster->currentLevel() * 100.0 / 255.0);
            thing->setStateValue(dimmableLightBrightnessStateTypeId, percentage);
        }

        connect(levelCluster, &ZigbeeClusterLevelControl::currentLevelChanged, thing, [this, thing](quint8 level){
            // Let's check if we have a pending action, if so,
            // do not update the state and wait until we reached the desired brightness level
            if (m_pendingBrightnessActions.contains(thing)) {
                quint8 targetBrightness = m_pendingBrightnessActions.value(thing)->action().paramValue(dimmableLightBrightnessActionBrightnessParamTypeId).toInt();
                quint8 targetLevel = static_cast<quint8>(qRound(255.0 * targetBrightness / 100.0));
                // Note check +-1 since there could be some rounding issue
                if (level == targetLevel || level == targetLevel - 1 || level == targetLevel + 1) {
                    qCDebug(dcZigbeeGenericLights()) << "Brightness moving finihed" << level << "-->" << targetLevel;
                    qCDebug(dcZigbeeGenericLights()) << thing << "level state changed" << targetLevel << targetBrightness << "%";
                    thing->setStateValue(dimmableLightBrightnessStateTypeId, targetBrightness);
                    ThingActionInfo *info = m_pendingBrightnessActions.take(thing);
                    info->finish(Thing::ThingErrorNoError);
                } else {
                    qCDebug(dcZigbeeGenericLights()) << "Brightness moving" << level << "-->" << targetLevel;
                }
            } else {
                int percentage = qRound(level * 100.0 / 255.0);
                qCDebug(dcZigbeeGenericLights()) << thing << "level state changed" << level << percentage << "%";
                thing->setStateValue(dimmableLightBrightnessStateTypeId, percentage);
            }
        });

        // Read the states once the node gets reachable
        connect(node, &ZigbeeNode::reachableChanged, thing, [=](bool reachable){
            if (reachable) {
                configureLightPowerReporting(node, endpoint);
                configureLightBrightnessReporting(node, endpoint);
                readLightPowerState(thing);
                readLightLevelState(thing);
            }
        });

        if (node->reachable()) {
            configureLightPowerReporting(node, endpoint);
            configureLightBrightnessReporting(node, endpoint);
        }
    }

    // Color temperature light
    if (thing->thingClassId() == colorTemperatureLightThingClassId) {

        // Get the on/off cluster
        ZigbeeClusterOnOff *onOffCluster = endpoint->inputCluster<ZigbeeClusterOnOff>(ZigbeeClusterLibrary::ClusterIdOnOff);
        if (!onOffCluster) {
            qCWarning(dcZigbeeGenericLights()) << "Could not find on/off cluster for" << thing << "in" << node;
            info->finish(Thing::ThingErrorHardwareFailure);
            return;
        }

        // Only set the state if the cluster actually has the attribute
        if (onOffCluster->hasAttribute(ZigbeeClusterOnOff::AttributeOnOff)) {
            thing->setStateValue(colorTemperatureLightPowerStateTypeId, onOffCluster->power());
        }

        // Update the power state if the node power value changes
        connect(onOffCluster, &ZigbeeClusterOnOff::powerChanged, thing, [thing](bool power){
            qCDebug(dcZigbeeGenericLights()) << thing << "power state changed" << power;
            thing->setStateValue(colorTemperatureLightPowerStateTypeId, power);
        });


        // Get the level cluster
        ZigbeeClusterLevelControl *levelCluster = endpoint->inputCluster<ZigbeeClusterLevelControl>(ZigbeeClusterLibrary::ClusterIdLevelControl);
        if (!levelCluster) {
            qCWarning(dcZigbeeGenericLights()) << "Could not find level cluster for" << thing << "in" << node;
            info->finish(Thing::ThingErrorHardwareFailure);
            return;
        }

        // Only set the state if the cluster actually has the attribute
        if (levelCluster->hasAttribute(ZigbeeClusterLevelControl::AttributeCurrentLevel)) {
            int percentage = qRound(levelCluster->currentLevel() * 100.0 / 255.0);
            thing->setStateValue(colorTemperatureLightBrightnessStateTypeId, percentage);
        }

        connect(levelCluster, &ZigbeeClusterLevelControl::currentLevelChanged, thing, [this, thing](quint8 level){
            // Let's check if we have a pending action, if so,
            // do not update the state and wait until we reached the desired brightness level
            if (m_pendingBrightnessActions.contains(thing)) {
                quint8 targetBrightness = m_pendingBrightnessActions.value(thing)->action().paramValue(colorTemperatureLightBrightnessActionBrightnessParamTypeId).toInt();
                quint8 targetLevel = static_cast<quint8>(qRound(255.0 * targetBrightness / 100.0));
                // Note check +-1 since there could be some rounding issue
                if (level == targetLevel || level == targetLevel - 1 || level == targetLevel + 1) {
                    qCDebug(dcZigbeeGenericLights()) << "Brightness moving finihed" << level << "-->" << targetLevel;
                    qCDebug(dcZigbeeGenericLights()) << thing << "level state changed" << targetLevel << targetBrightness << "%";
                    thing->setStateValue(colorTemperatureLightBrightnessStateTypeId, targetBrightness);
                    ThingActionInfo *info = m_pendingBrightnessActions.take(thing);
                    info->finish(Thing::ThingErrorNoError);
                } else {
                    qCDebug(dcZigbeeGenericLights()) << "Brightness moving" << level << "-->" << targetLevel;
                }
            } else {
                int percentage = qRound(level * 100.0 / 255.0);
                qCDebug(dcZigbeeGenericLights()) << thing << "level state changed" << level << percentage << "%";
                thing->setStateValue(colorTemperatureLightBrightnessStateTypeId, percentage);
            }
        });


        // Get color cluster
        ZigbeeClusterColorControl *colorCluster = endpoint->inputCluster<ZigbeeClusterColorControl>(ZigbeeClusterLibrary::ClusterIdColorControl);
        if (!colorCluster) {
            qCWarning(dcZigbeeGenericLights()) << "Could not find color cluster for" << thing << "in" << node;
            info->finish(Thing::ThingErrorHardwareFailure);
            return;
        }

        // Only set the state if the cluster actually has the attribute
        if (colorCluster->hasAttribute(ZigbeeClusterColorControl::AttributeColorTemperatureMireds)) {
            int mappedValue = mapColorTemperatureToScaledValue(thing, colorCluster->colorTemperatureMireds());
            thing->setStateValue(colorTemperatureLightColorTemperatureStateTypeId, mappedValue);
        }

        connect(colorCluster, &ZigbeeClusterColorControl::colorTemperatureMiredsChanged, thing, [this, thing](quint16 colorTemperatureMired){
            qCDebug(dcZigbeeGenericLights()) << "Actual color temperature is" << colorTemperatureMired << "mireds";
            int mappedValue = mapColorTemperatureToScaledValue(thing, colorTemperatureMired);
            qCDebug(dcZigbeeGenericLights()) << "Mapped color temperature is" << mappedValue;
            thing->setStateValue(colorTemperatureLightColorTemperatureStateTypeId, mappedValue);
        });

        // Read the states once the node gets reachable
        connect(node, &ZigbeeNode::reachableChanged, thing, [=](bool reachable){
            if (reachable) {
                configureLightPowerReporting(node, endpoint);
                configureLightBrightnessReporting(node, endpoint);

                readColorTemperatureRange(thing);
                readLightPowerState(thing);
                readLightLevelState(thing);
                readLightColorTemperatureState(thing);
            }
        });

        if (node->reachable()) {
            configureLightPowerReporting(node, endpoint);
            configureLightBrightnessReporting(node, endpoint);
        }
    }

    // Color light
    if (thing->thingClassId() == colorLightThingClassId) {

        // Get the on/off cluster
        ZigbeeClusterOnOff *onOffCluster = endpoint->inputCluster<ZigbeeClusterOnOff>(ZigbeeClusterLibrary::ClusterIdOnOff);
        if (!onOffCluster) {
            qCWarning(dcZigbeeGenericLights()) << "Could not find on/off cluster for" << thing << "in" << node;
            info->finish(Thing::ThingErrorHardwareFailure);
            return;
        }

        // Only set the state if the cluster actually has the attribute
        if (onOffCluster->hasAttribute(ZigbeeClusterOnOff::AttributeOnOff)) {
            thing->setStateValue(colorLightPowerStateTypeId, onOffCluster->power());
        }

        // Update the power state if the node power value changes
        connect(onOffCluster, &ZigbeeClusterOnOff::powerChanged, thing, [thing](bool power){
            qCDebug(dcZigbeeGenericLights()) << thing << "power state changed" << power;
            thing->setStateValue(colorLightPowerStateTypeId, power);
        });


        // Get the level cluster
        ZigbeeClusterLevelControl *levelCluster = endpoint->inputCluster<ZigbeeClusterLevelControl>(ZigbeeClusterLibrary::ClusterIdLevelControl);
        if (!levelCluster) {
            qCWarning(dcZigbeeGenericLights()) << "Could not find level cluster for" << thing << "in" << node;
            info->finish(Thing::ThingErrorHardwareFailure);
            return;
        }

        // Only set the state if the cluster actually has the attribute
        if (levelCluster->hasAttribute(ZigbeeClusterLevelControl::AttributeCurrentLevel)) {
            int percentage = qRound(levelCluster->currentLevel() * 100.0 / 255.0);
            thing->setStateValue(colorLightBrightnessStateTypeId, percentage);
        }

        connect(levelCluster, &ZigbeeClusterLevelControl::currentLevelChanged, thing, [this, thing](quint8 level){
            // Let's check if we have a pending action, if so,
            // do not update the state and wait until we reached the desired brightness level
            if (m_pendingBrightnessActions.contains(thing)) {
                quint8 targetBrightness = m_pendingBrightnessActions.value(thing)->action().paramValue(colorLightBrightnessActionBrightnessParamTypeId).toInt();
                quint8 targetLevel = static_cast<quint8>(qRound(255.0 * targetBrightness / 100.0));
                // Note check +-1 since there could be some rounding issue
                if (level == targetLevel || level == targetLevel - 1 || level == targetLevel + 1) {
                    qCDebug(dcZigbeeGenericLights()) << "Brightness moving finihed" << level << "-->" << targetLevel;
                    qCDebug(dcZigbeeGenericLights()) << thing << "level state changed" << targetLevel << targetBrightness << "%";
                    thing->setStateValue(colorLightBrightnessStateTypeId, targetBrightness);
                    ThingActionInfo *info = m_pendingBrightnessActions.take(thing);
                    info->finish(Thing::ThingErrorNoError);
                } else {
                    qCDebug(dcZigbeeGenericLights()) << "Brightness moving" << level << "-->" << targetLevel;
                }
            } else {
                int percentage = qRound(level * 100.0 / 255.0);
                qCDebug(dcZigbeeGenericLights()) << thing << "level state changed" << level << percentage << "%";
                thing->setStateValue(colorLightBrightnessStateTypeId, percentage);
            }
        });

        // Get color cluster
        ZigbeeClusterColorControl *colorCluster = endpoint->inputCluster<ZigbeeClusterColorControl>(ZigbeeClusterLibrary::ClusterIdColorControl);
        if (!colorCluster) {
            qCWarning(dcZigbeeGenericLights()) << "Could not find color cluster for" << thing << "in" << node;
            info->finish(Thing::ThingErrorHardwareFailure);
            return;
        }

        // Only set the state if the cluster actually has the attribute
        if (colorCluster->hasAttribute(ZigbeeClusterColorControl::AttributeColorTemperatureMireds)) {
            int mappedValue = mapColorTemperatureToScaledValue(thing, colorCluster->colorTemperatureMireds());
            thing->setStateValue(colorLightColorTemperatureStateTypeId, mappedValue);
        }

        connect(colorCluster, &ZigbeeClusterColorControl::colorTemperatureMiredsChanged, thing, [this, thing](quint16 colorTemperatureMired){
            qCDebug(dcZigbeeGenericLights()) << "Actual color temperature is" << colorTemperatureMired << "mireds";
            int mappedValue = mapColorTemperatureToScaledValue(thing, colorTemperatureMired);
            qCDebug(dcZigbeeGenericLights()) << "Mapped color temperature is" << mappedValue;
            thing->setStateValue(colorLightColorTemperatureStateTypeId, mappedValue);
        });

        // Read the states once the node gets reachable
        connect(node, &ZigbeeNode::reachableChanged, thing, [=](bool reachable){
            if (reachable) {
                configureLightPowerReporting(node, endpoint);
                configureLightBrightnessReporting(node, endpoint);

                // Note: this will also read the color temperature range if available
                readColorCapabilities(thing);
                readLightPowerState(thing);
                readLightLevelState(thing);
                readLightColorXYState(thing);
            }
        });

        if (node->reachable()) {
            configureLightPowerReporting(node, endpoint);
            configureLightBrightnessReporting(node, endpoint);
        }
    }

    info->finish(Thing::ThingErrorNoError);
}

void IntegrationPluginZigbeeGenericLights::postSetupThing(Thing *thing)
{
    if (thing->thingClassId() == onOffLightThingClassId) {
        ZigbeeNode *node = m_thingNodes.value(thing);
        if (node && node->reachable()) {
            readLightPowerState(thing);
        }
    } else if (thing->thingClassId() == dimmableLightThingClassId) {
        ZigbeeNode *node = m_thingNodes.value(thing);
        if (node && node->reachable()) {
            readLightPowerState(thing);
            readLightLevelState(thing);
        }
    } else if (thing->thingClassId() == colorTemperatureLightThingClassId) {
        ZigbeeNode *node = m_thingNodes.value(thing);
        if (node && node->reachable()) {
            readColorTemperatureRange(thing);
            readLightPowerState(thing);
            readLightLevelState(thing);
            readLightColorTemperatureState(thing);
        }
    } else if (thing->thingClassId() == colorLightThingClassId) {
        ZigbeeNode *node = m_thingNodes.value(thing);
        if (node && node->reachable()) {
            readColorCapabilities(thing);
            readLightPowerState(thing);
            readLightLevelState(thing);
            readLightColorXYState(thing);
        }
    }
}

void IntegrationPluginZigbeeGenericLights::executeAction(ThingActionInfo *info)
{
    if (!hardwareManager()->zigbeeResource()->available()) {
        qCDebug(dcZigbeeGenericLights()) << "Failed to execute action" << info->thing() << info->action().actionTypeId().toString() << "because the hardware is not available.";
        info->finish(Thing::ThingErrorHardwareNotAvailable);
        return;
    }

    // Get the node
    Thing *thing = info->thing();
    ZigbeeNode *node = m_thingNodes.value(thing);
    if (!node->reachable()) {
        qCDebug(dcZigbeeGenericLights()) << "Failed to execute action" << info->thing() << info->action().actionTypeId().toString() << "because node seems not to be reachable.";
        info->finish(Thing::ThingErrorHardwareNotAvailable);
        return;
    }

    // Get the endpoint
    ZigbeeNodeEndpoint *endpoint = findEndpoint(thing);
    if (!endpoint) {
        qCDebug(dcZigbeeGenericLights()) << "Failed to execute action" << info->thing() << info->action().actionTypeId().toString() << "because node endpoint could not be found.";
        info->finish(Thing::ThingErrorHardwareNotAvailable);
        return;
    }

    // On/Off light
    if (thing->thingClassId() == onOffLightThingClassId) {
        if (info->action().actionTypeId() == onOffLightAlertActionTypeId) {
            executeAlertAction(info, endpoint);
            return;
        }

        if (info->action().actionTypeId() == onOffLightPowerActionTypeId) {
            // Get the on/off server cluster from the endpoint
            bool power = info->action().param(onOffLightPowerActionPowerParamTypeId).value().toBool();
            executePowerAction(info, endpoint, onOffLightPowerStateTypeId, power);
            return;
        }
    }

    // Dimmable light
    if (thing->thingClassId() == dimmableLightThingClassId) {
        if (info->action().actionTypeId() == dimmableLightAlertActionTypeId) {
            executeAlertAction(info, endpoint);
            return;
        }

        if (info->action().actionTypeId() == dimmableLightPowerActionTypeId) {
            bool power = info->action().param(dimmableLightPowerActionPowerParamTypeId).value().toBool();
            executePowerAction(info, endpoint, dimmableLightPowerStateTypeId, power);
            return;
        }

        if (info->action().actionTypeId() == dimmableLightBrightnessActionTypeId) {
            int brightness = info->action().param(dimmableLightBrightnessActionBrightnessParamTypeId).value().toInt();
            quint8 level = static_cast<quint8>(qRound(255.0 * brightness / 100.0));
            executeBrightnessAction(info, endpoint, dimmableLightPowerStateTypeId, dimmableLightBrightnessStateTypeId, brightness, level);
            return;
        }
    }

    // Color temperature light
    if (thing->thingClassId() == colorTemperatureLightThingClassId) {
        if (info->action().actionTypeId() == colorTemperatureLightAlertActionTypeId) {
            executeAlertAction(info, endpoint);
            return;
        }

        if (info->action().actionTypeId() == colorTemperatureLightPowerActionTypeId) {
            bool power = info->action().param(colorTemperatureLightPowerActionPowerParamTypeId).value().toBool();
            executePowerAction(info, endpoint, colorTemperatureLightPowerStateTypeId, power);
            return;
        }

        if (info->action().actionTypeId() == colorTemperatureLightBrightnessActionTypeId) {
            int brightness = info->action().param(colorTemperatureLightBrightnessActionBrightnessParamTypeId).value().toInt();
            quint8 level = static_cast<quint8>(qRound(255.0 * brightness / 100.0));
            executeBrightnessAction(info, endpoint, colorTemperatureLightPowerStateTypeId, colorTemperatureLightBrightnessStateTypeId, brightness, level);
            return;
        }

        if (info->action().actionTypeId() == colorTemperatureLightColorTemperatureActionTypeId) {
            int colorTemperatureScaled = info->action().param(colorTemperatureLightColorTemperatureActionColorTemperatureParamTypeId).value().toInt();
            executeColorTemperatureAction(info, endpoint, colorTemperatureLightColorTemperatureStateTypeId, colorTemperatureScaled);
            return;
        }
    }

    // Color light
    if (thing->thingClassId() == colorLightThingClassId) {
        if (info->action().actionTypeId() == colorLightAlertActionTypeId) {
            executeAlertAction(info, endpoint);
            return;
        }

        if (info->action().actionTypeId() == colorLightPowerActionTypeId) {
            bool power = info->action().param(colorLightPowerActionPowerParamTypeId).value().toBool();
            executePowerAction(info, endpoint, colorLightPowerStateTypeId, power);
            return;
        }

        if (info->action().actionTypeId() == colorLightBrightnessActionTypeId) {
            int brightness = info->action().param(colorLightBrightnessActionBrightnessParamTypeId).value().toInt();
            quint8 level = static_cast<quint8>(qRound(255.0 * brightness / 100.0));
            executeBrightnessAction(info, endpoint, colorLightPowerStateTypeId, colorLightBrightnessStateTypeId, brightness, level);
            return;
        }

        if (info->action().actionTypeId() == colorLightColorTemperatureActionTypeId) {
            int colorTemperatureScaled = info->action().param(colorLightColorTemperatureActionColorTemperatureParamTypeId).value().toInt();
            if (m_colorCapabilities[thing].testFlag(ZigbeeClusterColorControl::ColorCapabilityColorTemperature)) {
                // Native color temperature available
                executeColorTemperatureAction(info, endpoint, colorLightColorTemperatureStateTypeId, colorTemperatureScaled);
                return;
            }

            // Note: there is no color temperature capability, we have to emulate the color using default min/max values
            // Convert the scaled value into the min/max color temperature interval
            quint16 colorTemperature = mapScaledValueToColorTemperature(thing, colorTemperatureScaled);
            qCDebug(dcZigbeeGenericLights()) << "Mapping action value" << colorTemperatureScaled << "to the color temperature in the range of [" << m_colorTemperatureRanges[thing].minValue << "," << m_colorTemperatureRanges[thing].maxValue << "] -->" << colorTemperature << "mired";
            // Note: the color temperature command/attribute is not supported. Using xy colors to interpolate the temperature colors
            QColor temperatureColor = ZigbeeUtils::interpolateColorFromColorTemperature(colorTemperature, m_colorTemperatureRanges[thing].minValue, m_colorTemperatureRanges[thing].maxValue);
            QPoint temperatureColorXyInt = ZigbeeUtils::convertColorToXYInt(temperatureColor);
            qCDebug(dcZigbeeGenericLights()) << "Mapping interpolated value" << temperatureColor << "mired to the xy color" << temperatureColorXyInt;

            // Set color
            ZigbeeClusterColorControl *colorCluster = endpoint->inputCluster<ZigbeeClusterColorControl>(ZigbeeClusterLibrary::ClusterIdColorControl);
            if (!colorCluster) {
                qCWarning(dcZigbeeGenericLights()) << "Could not find color control cluster for" << thing << "in" << m_thingNodes.value(thing);
                info->finish(Thing::ThingErrorHardwareFailure);
                return;
            }

            ZigbeeClusterReply *reply = colorCluster->commandMoveToColor(temperatureColorXyInt.x(), temperatureColorXyInt.y(), 2);
            connect(reply, &ZigbeeClusterReply::finished, info, [=](){
                if (reply->error() != ZigbeeClusterReply::ErrorNoError) {
                    info->finish(Thing::ThingErrorHardwareFailure);
                } else {
                    qCDebug(dcZigbeeGenericLights()) << "Set color temperature" << colorTemperature << "mired finished successfully" << "(scalled" << colorTemperatureScaled << ")";
                    thing->setStateValue(colorLightColorTemperatureStateTypeId, colorTemperatureScaled);
                    info->finish(Thing::ThingErrorNoError);
                }
            });

            return;
        }

        if (info->action().actionTypeId() == colorLightColorActionTypeId) {
            QColor color = info->action().param(colorLightColorActionColorParamTypeId).value().value<QColor>();
            QPoint xyColorInt = ZigbeeUtils::convertColorToXYInt(color);
            qCDebug(dcZigbeeGenericLights()) << "Set color" << color.toRgb() << xyColorInt;
            executeColorAction(info, endpoint, colorLightColorStateTypeId, color);
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

    if (m_colorTemperatureRanges.contains(thing)) {
        m_colorTemperatureRanges.remove(thing);
    }

    if (m_colorCapabilities.contains(thing)) {
        m_colorCapabilities.remove(thing);
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

void IntegrationPluginZigbeeGenericLights::createLightThing(const ThingClassId &thingClassId, const QUuid &networkUuid, ZigbeeNode *node, ZigbeeNodeEndpoint *endpoint)
{
    ThingDescriptor descriptor(thingClassId);
    QString deviceClassName = supportedThings().findById(thingClassId).displayName();
    descriptor.setTitle(QString("%1 (%2 - %3)").arg(deviceClassName).arg(endpoint->manufacturerName()).arg(endpoint->modelIdentifier()));

    ParamList params;
    params.append(Param(m_networkUuidParamTypeIds[thingClassId], networkUuid.toString()));
    params.append(Param(m_ieeeAddressParamTypeIds[thingClassId], node->extendedAddress().toString()));
    params.append(Param(m_endpointIdParamTypeIds[thingClassId], endpoint->endpointId()));
    params.append(Param(m_modelIdParamTypeIds[thingClassId], endpoint->modelIdentifier()));
    params.append(Param(m_manufacturerIdParamTypeIds[thingClassId], endpoint->manufacturerName()));
    descriptor.setParams(params);
    emit autoThingsAppeared({descriptor});
}

void IntegrationPluginZigbeeGenericLights::executeAlertAction(ThingActionInfo *info, ZigbeeNodeEndpoint *endpoint)
{
    Thing *thing = info->thing();
    ZigbeeClusterIdentify *identifyCluster = endpoint->inputCluster<ZigbeeClusterIdentify>(ZigbeeClusterLibrary::ClusterIdIdentify);
    if (!identifyCluster) {
        qCWarning(dcZigbeeGenericLights()) << "Could not find identify cluster for" << thing << "in" << m_thingNodes.value(thing);
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
}

void IntegrationPluginZigbeeGenericLights::executePowerAction(ThingActionInfo *info, ZigbeeNodeEndpoint *endpoint, const StateTypeId &powerStateTypeId, bool power)
{
    Thing *thing = info->thing();
    ZigbeeClusterOnOff *onOffCluster = endpoint->inputCluster<ZigbeeClusterOnOff>(ZigbeeClusterLibrary::ClusterIdOnOff);
    if (!onOffCluster) {
        qCWarning(dcZigbeeGenericLights()) << "Could not find on/off cluster for" << thing << "in" << m_thingNodes.value(thing);
        info->finish(Thing::ThingErrorHardwareFailure);
        return;
    }

    // Send the command trough the network
    qCDebug(dcZigbeeGenericLights()) << "Set power for" << info->thing() << "to" << power;
    ZigbeeClusterReply *reply = (power ? onOffCluster->commandOn() : onOffCluster->commandOff());
    connect(reply, &ZigbeeClusterReply::finished, info, [=](){
        // Note: reply will be deleted automatically
        if (reply->error() != ZigbeeClusterReply::ErrorNoError) {
            qCWarning(dcZigbeeGenericLights()) << "Failed to set power on" << thing << reply->error();
            info->finish(Thing::ThingErrorHardwareFailure);
        } else {
            info->finish(Thing::ThingErrorNoError);
            qCDebug(dcZigbeeGenericLights()) << "Set power finished successfully for" << thing;
            thing->setStateValue(powerStateTypeId, power);
        }
    });
}

void IntegrationPluginZigbeeGenericLights::executeBrightnessAction(ThingActionInfo *info, ZigbeeNodeEndpoint *endpoint, const StateTypeId &powerStateTypeId, const StateTypeId &brightnessStateTypeId, int brightness, quint8 level)
{
    Thing *thing = info->thing();
    ZigbeeClusterLevelControl *levelCluster = endpoint->inputCluster<ZigbeeClusterLevelControl>(ZigbeeClusterLibrary::ClusterIdLevelControl);
    if (!levelCluster) {
        qCWarning(dcZigbeeGenericLights()) << "Could not find level control cluster for" << thing << "in" << m_thingNodes.value(thing);
        info->finish(Thing::ThingErrorHardwareFailure);
        return;
    }

    ZigbeeClusterReply *reply = levelCluster->commandMoveToLevelWithOnOff(level, 5);
    connect(reply, &ZigbeeClusterReply::finished, info, [=](){
        // Note: reply will be deleted automatically
        if (reply->error() != ZigbeeClusterReply::ErrorNoError) {
            info->finish(Thing::ThingErrorHardwareFailure);
        } else {
            if (m_pendingBrightnessActions.contains(thing)) {
                qCDebug(dcZigbeeGenericLights()) << "Finish previouse action";
                m_pendingBrightnessActions.take(thing)->finish(Thing::ThingErrorNoError);
            }

            connect(info, &ThingActionInfo::aborted, thing, [=](){
                 qCWarning(dcZigbeeGenericLights()) << "Action aborted";
                 if (m_pendingBrightnessActions.values().contains(info)) {
                     m_pendingBrightnessActions.take(thing);
                 }
            });

            // If we are already on the desired level, finish the action
            if (levelCluster->currentLevel() == level) {
                thing->setStateValue(powerStateTypeId, (brightness > 0 ? true : false));
                thing->setStateValue(brightnessStateTypeId, brightness);
                info->finish(Thing::ThingErrorNoError);
                return;
            }

            // We are not yet on the target level, let's wait until the brightness has been reached
            m_pendingBrightnessActions.insert(thing, info);
            qCDebug(dcZigbeeGenericLights()) << "Wait until the brightness has reached the target value";
        }
    });
}

void IntegrationPluginZigbeeGenericLights::executeColorTemperatureAction(ThingActionInfo *info, ZigbeeNodeEndpoint *endpoint, const StateTypeId &colorTemperatureStateTypeId, int colorTemperatureScaled)
{
    Thing *thing = info->thing();
    ZigbeeClusterColorControl *colorCluster = endpoint->inputCluster<ZigbeeClusterColorControl>(ZigbeeClusterLibrary::ClusterIdColorControl);
    if (!colorCluster) {
        qCWarning(dcZigbeeGenericLights()) << "Could not find color control cluster for" << thing << "in" << m_thingNodes.value(thing);
        info->finish(Thing::ThingErrorHardwareFailure);
        return;
    }

    // Scale the value to the actual color temperature of this lamp
    quint16 colorTemperature = mapScaledValueToColorTemperature(thing, colorTemperatureScaled);
    // Note: time unit is 1/10 s
    ZigbeeClusterReply *reply = colorCluster->commandMoveToColorTemperature(colorTemperature, 5);
    connect(reply, &ZigbeeClusterReply::finished, info, [=](){
        // Note: reply will be deleted automatically
        if (reply->error() != ZigbeeClusterReply::ErrorNoError) {
            info->finish(Thing::ThingErrorHardwareFailure);
        } else {
            info->finish(Thing::ThingErrorNoError);
            thing->setStateValue(colorTemperatureStateTypeId, colorTemperatureScaled);
        }
    });
}

void IntegrationPluginZigbeeGenericLights::executeColorAction(ThingActionInfo *info, ZigbeeNodeEndpoint *endpoint, const StateTypeId &colorStateTypeId, const QColor &color)
{
    Thing *thing = info->thing();
    ZigbeeClusterColorControl *colorCluster = endpoint->inputCluster<ZigbeeClusterColorControl>(ZigbeeClusterLibrary::ClusterIdColorControl);
    if (!colorCluster) {
        qCWarning(dcZigbeeGenericLights()) << "Could not find color control cluster for" << thing << "in" << m_thingNodes.value(thing);
        info->finish(Thing::ThingErrorHardwareFailure);
        return;
    }

    // Note: time unit is 1/10 s
    QPoint colorXyInt = ZigbeeUtils::convertColorToXYInt(color);
    ZigbeeClusterReply *reply = colorCluster->commandMoveToColor(colorXyInt.x(), colorXyInt.y(), 5);
    connect(reply, &ZigbeeClusterReply::finished, info, [=](){
        // Note: reply will be deleted automatically
        if (reply->error() != ZigbeeClusterReply::ErrorNoError) {
            info->finish(Thing::ThingErrorHardwareFailure);
        } else {
            info->finish(Thing::ThingErrorNoError);
            thing->setStateValue(colorStateTypeId, color);
        }
    });
}

void IntegrationPluginZigbeeGenericLights::readLightPowerState(Thing *thing)
{
    // Get the node
    ZigbeeNode *node = m_thingNodes.value(thing);
    if (!node->reachable())
        return;

    // Get the endpoint
    ZigbeeNodeEndpoint *endpoint = findEndpoint(thing);
    if (!endpoint)
        return;

    // Get the on/off server cluster from the endpoint
    ZigbeeClusterOnOff *onOffCluster = endpoint->inputCluster<ZigbeeClusterOnOff>(ZigbeeClusterLibrary::ClusterIdOnOff);
    if (!onOffCluster)
        return;

    qCDebug(dcZigbeeGenericLights()) << "Reading on/off power value for" << thing << "on" << node;
    ZigbeeClusterReply *reply = onOffCluster->readAttributes({ZigbeeClusterOnOff::AttributeOnOff});
    connect(reply, &ZigbeeClusterReply::finished, thing, [thing, reply](){
        if (reply->error() != ZigbeeClusterReply::ErrorNoError) {
            qCWarning(dcZigbeeGenericLights()) << "Failed to read on/off cluster attribute from" << thing << reply->error();
            return;
        }
    });
}

void IntegrationPluginZigbeeGenericLights::readLightLevelState(Thing *thing)
{
    // Get the node
    ZigbeeNode *node = m_thingNodes.value(thing);
    if (!node->reachable())
        return;

    // Get the endpoint
    ZigbeeNodeEndpoint *endpoint = findEndpoint(thing);
    if (!endpoint)
        return;

    // Get the level server cluster from the endpoint
    ZigbeeClusterLevelControl *levelCluster = endpoint->inputCluster<ZigbeeClusterLevelControl>(ZigbeeClusterLibrary::ClusterIdLevelControl);
    if (!levelCluster)
        return;

    qCDebug(dcZigbeeGenericLights()) << "Reading level value for" << thing << "on" << node;
    ZigbeeClusterReply *reply = levelCluster->readAttributes({ZigbeeClusterLevelControl::AttributeCurrentLevel});
    connect(reply, &ZigbeeClusterReply::finished, thing, [thing, reply](){
        if (reply->error() != ZigbeeClusterReply::ErrorNoError) {
            qCWarning(dcZigbeeGenericLights()) << "Failed to read level cluster attribute from" << thing << reply->error();
            return;
        }
    });
}

void IntegrationPluginZigbeeGenericLights::readLightColorTemperatureState(Thing *thing)
{
    ZigbeeNodeEndpoint *endpoint = findEndpoint(thing);
    if (!endpoint) {
        qCWarning(dcZigbeeGenericLights()) << "Failed to read color temperature for" << thing << "because the node could not be found";
        return;
    }

    // Get the color server cluster from the endpoint
    ZigbeeClusterColorControl *colorCluster = endpoint->inputCluster<ZigbeeClusterColorControl>(ZigbeeClusterLibrary::ClusterIdColorControl);
    if (!colorCluster) {
        qCWarning(dcZigbeeGenericLights()) << "Failed to read color temperature for" << thing << "because the color cluster could not be found on" << endpoint;
        return;
    }

    ZigbeeClusterReply *reply = colorCluster->readAttributes({ZigbeeClusterColorControl::AttributeColorTemperatureMireds});
    connect(reply, &ZigbeeClusterReply::finished, thing, [=](){
        if (reply->error() != ZigbeeClusterReply::ErrorNoError) {
            qCWarning(dcZigbeeGenericLights()) << "Failed to read ColorControl cluster attribute color temperature" << reply->error();
            return;
        }
    });
}

void IntegrationPluginZigbeeGenericLights::readLightColorXYState(Thing *thing)
{
    ZigbeeNodeEndpoint *endpoint = findEndpoint(thing);
    if (!endpoint) {
        qCWarning(dcZigbeeGenericLights()) << "Failed to read color xy for" << thing << "because the node could not be found";
        return;
    }

    // Get the color server cluster from the endpoint
    ZigbeeClusterColorControl *colorCluster = endpoint->inputCluster<ZigbeeClusterColorControl>(ZigbeeClusterLibrary::ClusterIdColorControl);
    if (!colorCluster) {
        qCWarning(dcZigbeeGenericLights()) << "Failed to read color xy for" << thing << "because the color cluster could not be found on" << endpoint;
        return;
    }

    ZigbeeClusterReply *reply = colorCluster->readAttributes({ZigbeeClusterColorControl::AttributeCurrentX, ZigbeeClusterColorControl::AttributeCurrentY});
    connect(reply, &ZigbeeClusterReply::finished, thing, [=](){
        if (reply->error() != ZigbeeClusterReply::ErrorNoError) {
            qCWarning(dcZigbeeGenericLights()) << "Failed to read ColorControl cluster attribute" << reply->error();
            return;
        }

        // Note: the attribute gets updated internally and the state gets updated with the currentLevelChanged signal
        qCDebug(dcZigbeeGenericLights()) << "Reading ColorControl cluster attribute color x and y finished successfully";

        QList<ZigbeeClusterLibrary::ReadAttributeStatusRecord> attributeStatusRecords = ZigbeeClusterLibrary::parseAttributeStatusRecords(reply->responseFrame().payload);
        if (attributeStatusRecords.count() != 2) {
            qCWarning(dcZigbeeGenericLights()) << "Did not receive color x and y attribute values from" << thing;
            return;
        }

        // Parse the attribute status records and calculate the color
        quint16 currentX = 0; quint16 currentY = 0;
        foreach (const ZigbeeClusterLibrary::ReadAttributeStatusRecord &attributeStatusRecord, attributeStatusRecords) {
            qCDebug(dcZigbeeGenericLights()) << "Received read attribute status record" << thing << attributeStatusRecord;
            if (attributeStatusRecord.attributeId == ZigbeeClusterColorControl::AttributeCurrentX) {
                bool valueOk = false;
                currentX = attributeStatusRecord.dataType.toUInt16(&valueOk);
                if (!valueOk) {
                    qCWarning(dcZigbeeGenericLights()) << "Failed to convert color x attribute values from" << thing << attributeStatusRecord;
                    return;
                }
                continue;
            }

            if (attributeStatusRecord.attributeId == ZigbeeClusterColorControl::AttributeCurrentY) {
                bool valueOk = false;
                currentY = attributeStatusRecord.dataType.toUInt16(&valueOk);
                if (!valueOk) {
                    qCWarning(dcZigbeeGenericLights()) << "Failed to convert color y attribute values from" << thing << attributeStatusRecord;
                    return;
                }
                continue;
            }
        }

        // Set the current color
        QColor color = ZigbeeUtils::convertXYToColor(currentX, currentY);
        qCDebug(dcZigbeeGenericLights()) << "Current color" << color.toRgb() << QPoint(currentX, currentY);
        thing->setStateValue(colorLightColorStateTypeId, color);
    });
}

void IntegrationPluginZigbeeGenericLights::configureLightPowerReporting(ZigbeeNode *node, ZigbeeNodeEndpoint *endpoint)
{
    // Bind the cluster to the coordinator
    qCDebug(dcZigbeeGenericLights()) << "Bind on/off cluster to coordinator IEEE address";
    ZigbeeDeviceObjectReply * zdoReply = node->deviceObject()->requestBindIeeeAddress(endpoint->endpointId(), ZigbeeClusterLibrary::ClusterIdOnOff, hardwareManager()->zigbeeResource()->coordinatorAddress(node->networkUuid()), 0x01);
    connect(zdoReply, &ZigbeeDeviceObjectReply::finished, node, [=](){
        if (zdoReply->error() != ZigbeeDeviceObjectReply::ErrorNoError) {
            qCWarning(dcZigbeeGenericLights()) << "Failed to bind on/off cluster to coordinator" << zdoReply->error();
        } else {
            qCDebug(dcZigbeeGenericLights()) << "Bind on/off cluster to coordinator finished successfully";
        }

        // Configure attribute reporting for lock state
        ZigbeeClusterLibrary::AttributeReportingConfiguration reportingConfig;
        reportingConfig.attributeId = ZigbeeClusterOnOff::AttributeOnOff;
        reportingConfig.dataType = Zigbee::Bool;
        reportingConfig.reportableChange = ZigbeeDataType(static_cast<quint8>(1)).data();

        qCDebug(dcZigbeeGenericLights()) << "Configure attribute reporting for on/off cluster";
        ZigbeeClusterReply *reportingReply = endpoint->getInputCluster(ZigbeeClusterLibrary::ClusterIdOnOff)->configureReporting({reportingConfig});
        connect(reportingReply, &ZigbeeClusterReply::finished, this, [=](){
            if (reportingReply->error() != ZigbeeClusterReply::ErrorNoError) {
                qCWarning(dcZigbeeGenericLights()) << "Failed configure attribute reporting on on/off cluster" << reportingReply->error();
            } else {
                qCDebug(dcZigbeeGenericLights()) << "Attribute reporting configuration finished for on/off cluster" << ZigbeeClusterLibrary::parseAttributeReportingStatusRecords(reportingReply->responseFrame().payload);
            }
        });
    });
}

void IntegrationPluginZigbeeGenericLights::configureLightBrightnessReporting(ZigbeeNode *node, ZigbeeNodeEndpoint *endpoint)
{
    qCDebug(dcZigbeeGenericLights()) << "Bind level cluster to coordinator IEEE address";
    ZigbeeDeviceObjectReply * zdoReply = node->deviceObject()->requestBindIeeeAddress(endpoint->endpointId(), ZigbeeClusterLibrary::ClusterIdLevelControl, hardwareManager()->zigbeeResource()->coordinatorAddress(node->networkUuid()), 0x01);
    connect(zdoReply, &ZigbeeDeviceObjectReply::finished, node, [=](){
        if (zdoReply->error() != ZigbeeDeviceObjectReply::ErrorNoError) {
            qCWarning(dcZigbeeGenericLights()) << "Failed to bind level cluster to coordinator" << zdoReply->error();
        } else {
            qCDebug(dcZigbeeGenericLights()) << "Bind level cluster to coordinator finished successfully";
        }

        // Configure attribute reporting for lock state
        ZigbeeClusterLibrary::AttributeReportingConfiguration reportingConfig;
        reportingConfig.attributeId = ZigbeeClusterLevelControl::AttributeCurrentLevel;
        reportingConfig.dataType = Zigbee::Uint8;
        reportingConfig.reportableChange = ZigbeeDataType(static_cast<quint8>(1)).data();

        qCDebug(dcZigbeeGenericLights()) << "Configure attribute reporting for level cluster";
        ZigbeeClusterReply *reportingReply = endpoint->getInputCluster(ZigbeeClusterLibrary::ClusterIdLevelControl)->configureReporting({reportingConfig});
        connect(reportingReply, &ZigbeeClusterReply::finished, this, [=](){
            if (reportingReply->error() != ZigbeeClusterReply::ErrorNoError) {
                qCWarning(dcZigbeeGenericLights()) << "Failed configure attribute reporting on level cluster" << reportingReply->error();
            } else {
                qCDebug(dcZigbeeGenericLights()) << "Attribute reporting configuration finished for level cluster" << ZigbeeClusterLibrary::parseAttributeReportingStatusRecords(reportingReply->responseFrame().payload);
            }
        });
    });
}

void IntegrationPluginZigbeeGenericLights::readColorCapabilities(Thing *thing)
{
    ZigbeeNodeEndpoint *endpoint = findEndpoint(thing);
    if (!endpoint) {
        qCWarning(dcZigbeeGenericLights()) << "Failed to read color capabilities for" << thing << "because the node could not be found";
        return;
    }

    // Get the color server cluster from the endpoint
    ZigbeeClusterColorControl *colorCluster = endpoint->inputCluster<ZigbeeClusterColorControl>(ZigbeeClusterLibrary::ClusterIdColorControl);
    if (!colorCluster) {
        qCWarning(dcZigbeeGenericLights()) << "Failed to read color color capabilities for" << thing << "because the color cluster could not be found on" << endpoint;
        return;
    }

    // Check if we know already the color capabilities for this lamp
    if (colorCluster->hasAttribute(ZigbeeClusterColorControl::AttributeColorCapabilities)) {
        m_colorCapabilities[thing] = colorCluster->colorCapabilities();
        qCDebug(dcZigbeeGenericLights()) << "Found cached color capabilities for" << thing << colorCluster->colorCapabilities();
        processColorCapabilities(thing);
        return;
    }

    // We have to read the color capabilities
    ZigbeeClusterReply *reply = colorCluster->readAttributes({ZigbeeClusterColorControl::AttributeColorCapabilities});
    connect(reply, &ZigbeeClusterReply::finished, thing, [=](){
        if (reply->error() != ZigbeeClusterReply::ErrorNoError) {
            qCWarning(dcZigbeeGenericLights()) << "Failed to read color capabilitie for" << thing << "because the node could not be found";
            return;
        }

        m_colorCapabilities[thing] = colorCluster->colorCapabilities();
        processColorCapabilities(thing);
    });
}

void IntegrationPluginZigbeeGenericLights::processColorCapabilities(Thing *thing)
{
    // Fetch all information required depending on the capabilities
    qCDebug(dcZigbeeGenericLights()) << "Loading information depending on the lamp capabilities" << thing << m_colorCapabilities[thing];
    if (m_colorCapabilities[thing].testFlag(ZigbeeClusterColorControl::ColorCapabilityColorTemperature)) {
        qCDebug(dcZigbeeGenericLights()) << "The lamp is capable of native controlling the color temperature";

        // Read min/max value, otherwise emulate the color temperature using the color map
        readColorTemperatureRange(thing);
    } else {
        qCDebug(dcZigbeeGenericLights()) << "The lamp has no native color temperature capability, emulating them using color map.";

        // TODO: continue with color fetching (xy, hsv, gamut values)

        qCDebug(dcZigbeeGenericLights()) << "Lamp capabilities information complete";
    }
}

void IntegrationPluginZigbeeGenericLights::readColorTemperatureRange(Thing *thing)
{
    ZigbeeNodeEndpoint *endpoint = findEndpoint(thing);
    if (!endpoint) {
        qCWarning(dcZigbeeGenericLights()) << "Failed to read color temperature range for" << thing << "because the node could not be found";
        return;
    }

    // Get the color server cluster from the endpoint
    ZigbeeClusterColorControl *colorCluster = endpoint->inputCluster<ZigbeeClusterColorControl>(ZigbeeClusterLibrary::ClusterIdColorControl);
    if (!colorCluster) {
        qCWarning(dcZigbeeGenericLights()) << "Failed to read color temperature range for" << thing << "because the color cluster could not be found on" << endpoint;
        return;
    }

    // Check if we can use the cached values from the database
    if (readCachedColorTemperatureRange(thing, colorCluster)) {
        qCDebug(dcZigbeeGenericLights()) << "Using cached color temperature mireds interval for mapping" << thing << "[" << m_colorTemperatureRanges[thing].minValue << "," << m_colorTemperatureRanges[thing].maxValue << "] mired";
        return;
    }

    // Init with default values
    m_colorTemperatureRanges[thing] = ColorTemperatureRange();

    // We need to read them from the lamp
    ZigbeeClusterReply *reply = colorCluster->readAttributes({ZigbeeClusterColorControl::AttributeColorTempPhysicalMinMireds, ZigbeeClusterColorControl::AttributeColorTempPhysicalMaxMireds});
    connect(reply, &ZigbeeClusterReply::finished, thing, [=](){
        if (reply->error() != ZigbeeClusterReply::ErrorNoError) {
            qCWarning(dcZigbeeGenericLights()) << "Reading color temperature range attributes finished with error" << reply->error();
            qCWarning(dcZigbeeGenericLights()) << "Failed to read color temperature min/max interval values. Using default values for" << thing << "[" << m_colorTemperatureRanges[thing].minValue << "," << m_colorTemperatureRanges[thing].maxValue << "] mired";
            return;
        }

        QList<ZigbeeClusterLibrary::ReadAttributeStatusRecord> attributeStatusRecords = ZigbeeClusterLibrary::parseAttributeStatusRecords(reply->responseFrame().payload);
        if (attributeStatusRecords.count() != 2) {
            qCWarning(dcZigbeeGenericLights()) << "Did not receive temperature min/max interval values from" << thing;
            qCWarning(dcZigbeeGenericLights()) << "Using default values for" << thing << "[" << m_colorTemperatureRanges[thing].minValue << "," << m_colorTemperatureRanges[thing].maxValue << "] mired" ;
            return;
        }

        // Parse the attribute status records
        foreach (const ZigbeeClusterLibrary::ReadAttributeStatusRecord &attributeStatusRecord, attributeStatusRecords) {
            if (attributeStatusRecord.attributeId == ZigbeeClusterColorControl::AttributeColorTempPhysicalMinMireds) {
                bool valueOk = false;
                quint16 minMiredsValue = attributeStatusRecord.dataType.toUInt16(&valueOk);
                if (!valueOk) {
                    qCWarning(dcZigbeeGenericLights()) << "Failed to read color temperature min mireds attribute value and convert it" << attributeStatusRecord;
                    break;
                }

                m_colorTemperatureRanges[thing].minValue = minMiredsValue;
            }

            if (attributeStatusRecord.attributeId == ZigbeeClusterColorControl::AttributeColorTempPhysicalMaxMireds) {
                bool valueOk = false;
                quint16 maxMiredsValue = attributeStatusRecord.dataType.toUInt16(&valueOk);
                if (!valueOk) {
                    qCWarning(dcZigbeeGenericLights()) << "Failed to read color temperature max mireds attribute value and convert it" << attributeStatusRecord;
                    break;
                }

                m_colorTemperatureRanges[thing].maxValue = maxMiredsValue;
            }
        }

        qCDebug(dcZigbeeGenericLights()) << "Using lamp specific color temperature mireds interval for mapping" << thing << "[" <<  m_colorTemperatureRanges[thing].minValue << "," << m_colorTemperatureRanges[thing].maxValue << "] mired";
    });

}

bool IntegrationPluginZigbeeGenericLights::readCachedColorTemperatureRange(Thing *thing, ZigbeeClusterColorControl *colorCluster)
{
    if (colorCluster->hasAttribute(ZigbeeClusterColorControl::AttributeColorTempPhysicalMinMireds) && colorCluster->hasAttribute(ZigbeeClusterColorControl::AttributeColorTempPhysicalMaxMireds)) {
        ZigbeeClusterAttribute minMiredsAttribute = colorCluster->attribute(ZigbeeClusterColorControl::AttributeColorTempPhysicalMinMireds);
        bool valueOk = false;
        quint16 minMiredsValue = minMiredsAttribute.dataType().toUInt16(&valueOk);
        if (!valueOk) {
            qCWarning(dcZigbeeGenericLights()) << "Failed to read color temperature min mireds attribute value and convert it" << minMiredsAttribute;
            return false;
        }

        ZigbeeClusterAttribute maxMiredsAttribute = colorCluster->attribute(ZigbeeClusterColorControl::AttributeColorTempPhysicalMaxMireds);
        quint16 maxMiredsValue = maxMiredsAttribute.dataType().toUInt16(&valueOk);
        if (!valueOk) {
            qCWarning(dcZigbeeGenericLights()) << "Failed to read color temperature max mireds attribute value and convert it" << maxMiredsAttribute;
            return false;
        }

        ColorTemperatureRange range;
        range.minValue = minMiredsValue;
        range.maxValue = maxMiredsValue;
        m_colorTemperatureRanges[thing] = range;
        return true;
    }

    return false;
}

quint16 IntegrationPluginZigbeeGenericLights::mapScaledValueToColorTemperature(Thing *thing, int scaledColorTemperature)
{
    // Make sure we have values to scale
    if (!m_colorTemperatureRanges.contains(thing)) {
        m_colorTemperatureRanges[thing] = ColorTemperatureRange();
    }

    double percentage = static_cast<double>((scaledColorTemperature - m_minScaleValue)) / (m_maxScaleValue - m_minScaleValue);
    //qCDebug(dcZigbeeGenericLights()) << "Mapping color temperature value" << scaledColorTemperature << "between" << m_minScaleValue << m_maxScaleValue << "is" << percentage * 100 << "%";
    double mappedValue = (m_colorTemperatureRanges[thing].maxValue - m_colorTemperatureRanges[thing].minValue) * percentage + m_colorTemperatureRanges[thing].minValue;
    //qCDebug(dcZigbeeGenericLights()) << "Mapping color temperature value" << scaledColorTemperature << "is" << mappedValue << "mireds";
    return static_cast<quint16>(qRound(mappedValue));
}

int IntegrationPluginZigbeeGenericLights::mapColorTemperatureToScaledValue(Thing *thing, quint16 colorTemperature)
{
    // Make sure we have values to scale
    if (!m_colorTemperatureRanges.contains(thing)) {
        m_colorTemperatureRanges[thing] = ColorTemperatureRange();
    }

    double percentage = static_cast<double>((colorTemperature - m_colorTemperatureRanges[thing].minValue)) / (m_colorTemperatureRanges[thing].maxValue - m_colorTemperatureRanges[thing].minValue);
    //qCDebug(dcZigbee()) << "Mapping color temperature value" << colorTemperature << "mirred" << m_minColorTemperature << m_maxColorTemperature << "is" << percentage * 100 << "%";
    double mappedValue = (m_maxScaleValue - m_minScaleValue) * percentage + m_minScaleValue;
    //qCDebug(dcZigbee()) << "Mapping color temperature value" << colorTemperature << "results into the scaled value of" << mappedValue;
    return static_cast<int>(qRound(mappedValue));
}
