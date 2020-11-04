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

#include <QDebug>

IntegrationPluginZigbeeGeneric::IntegrationPluginZigbeeGeneric()
{

}

QString IntegrationPluginZigbeeGeneric::name() const
{
    return "Generic";
}

bool IntegrationPluginZigbeeGeneric::handleNode(ZigbeeNode *node, const QUuid &networkUuid)
{
    foreach (ZigbeeNodeEndpoint *endpoint, node->endpoints()) {
        qCDebug(dcZigbeeGeneric) << "Endpoint profile:" << endpoint->profile() << endpoint->deviceId() << networkUuid;
//        if ((endpoint->profile() == Zigbee::ZigbeeProfile::ZigbeeProfileLightLink && endpoint->deviceId() == Zigbee::LightLinkDevice::LightLinkDeviceOnOff
//                (endpoint->profile() == Zigbee::ZigbeeProfile::ZigbeeProfileHomeAutomation && endpoint->deviceId() == Zigbee::HomeAutomationDeviceOnOf>

//            // Create generic power socket
//            qCDebug(dcZigbee()) << "This device is an power socket";
//            if (myThings().filterByThingClassId(genericPowerSocketThingClassId)
//                    .filterByParam(genericPowerSocketThingIeeeAddressParamTypeId, node->extendedAddress().toString())
//                    .isEmpty()) {
//                qCDebug(dcZigbee()) << "Adding new generic power socket";
//                ThingDescriptor descriptor(genericPowerSocketThingClassId);
//                QString deviceClassName = supportedThings().findById(genericPowerSocketThingClassId).displayName();
//                descriptor.setTitle(QString("%1 (%2 - %3)").arg(deviceClassName).arg(endpoint->manufacturerName()).arg(endpoint->modelIdentifier()));
//                ParamList params;
//                params.append(Param(genericPowerSocketThingIeeeAddressParamTypeId, node->extendedAddress().toString()));
//                params.append(Param(genericPowerSocketThingManufacturerParamTypeId, endpoint->manufacturerName()));
//                params.append(Param(genericPowerSocketThingModelParamTypeId, endpoint->modelIdentifier()));
//                descriptor.setParams(params);
//                descriptor.setParentId(networkManagerDevice->id());
//                emit autoThingsAppeared({descriptor});
//            } else {
//                qCDebug(dcZigbee()) << "The device for this node has already been created.";
//            }
//            return true;
//        }
    }


    return false;
}

void IntegrationPluginZigbeeGeneric::init()
{
    hardwareManager()->zigbeeResource()->registerHandler(this, ZigbeeHardwareResource::HandlerTypeCatchAll);
}

void IntegrationPluginZigbeeGeneric::setupThing(ThingSetupInfo *info)
{
    info->finish(Thing::ThingErrorNoError);
}

void IntegrationPluginZigbeeGeneric::executeAction(ThingActionInfo *info)
{
    info->finish(Thing::ThingErrorUnsupportedFeature);
}

void IntegrationPluginZigbeeGeneric::thingRemoved(Thing *thing)
{
    Q_UNUSED(thing)
}
