/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*
* Copyright 2013 - 2024, nymea GmbH
* Contact: contact@nymea.io
*
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

#include "integrationpluginespsomfyrts.h"

#include <QUrlQuery>
#include <QHostAddress>
#include <QDataStream>
#include <QJsonDocument>
#include <QJsonParseError>

#include "plugininfo.h"
#include "espsomfyrtsdiscovery.h"

IntegrationPluginEspSomfyRts::IntegrationPluginEspSomfyRts()
{

}

void IntegrationPluginEspSomfyRts::init()
{

}

void IntegrationPluginEspSomfyRts::discoverThings(ThingDiscoveryInfo *info)
{
    if (!hardwareManager()->networkDeviceDiscovery()->available()) {
        qCWarning(dcESPSomfyRTS()) << "Failed to discover network devices. The network device discovery is not available.";
        info->finish(Thing::ThingErrorHardwareNotAvailable, QT_TR_NOOP("Unable to discover devices in your network."));
        return;
    }

    qCInfo(dcESPSomfyRTS()) << "Starting network discovery...";
    EspSomfyRtsDiscovery *discovery = new EspSomfyRtsDiscovery(hardwareManager()->networkManager(), hardwareManager()->networkDeviceDiscovery(), info);
    connect(discovery, &EspSomfyRtsDiscovery::discoveryFinished, info, [=](){
        ThingDescriptors descriptors;
        qCInfo(dcESPSomfyRTS()) << "Discovery finished. Found" << discovery->results().count() << "devices";
        foreach (const EspSomfyRtsDiscovery::Result &result, discovery->results()) {
            qCInfo(dcESPSomfyRTS()) << "Discovered device on" << result.networkDeviceInfo;
            if (result.networkDeviceInfo.macAddress().isNull())
                continue;

            QString title = "ESP Somfy RTS (" + result.name + ")";
            QString description = result.networkDeviceInfo.address().toString() + " (" + result.networkDeviceInfo.macAddress() + ")";

            ThingDescriptor descriptor(espSomfyRtsThingClassId, title, description);

            // Check if we already have set up this device
            Things existingThings = myThings().filterByParam(espSomfyRtsThingMacAddressParamTypeId, result.networkDeviceInfo.macAddress());
            if (existingThings.count() == 1) {
                qCDebug(dcESPSomfyRTS()) << "This thing already exists in the system." << existingThings.first() << result.networkDeviceInfo;
                descriptor.setThingId(existingThings.first()->id());
            }

            ParamList params;
            params << Param(espSomfyRtsThingMacAddressParamTypeId, result.networkDeviceInfo.macAddress());
            descriptor.setParams(params);
            info->addThingDescriptor(descriptor);
        }

        info->finish(Thing::ThingErrorNoError);
    });

    discovery->startDiscovery();
}

void IntegrationPluginEspSomfyRts::setupThing(ThingSetupInfo *info)
{
    Q_UNUSED(info)

}

void IntegrationPluginEspSomfyRts::postSetupThing(Thing *thing)
{
    Q_UNUSED(thing)
}

void IntegrationPluginEspSomfyRts::thingRemoved(Thing *thing)
{
    Q_UNUSED(thing)
}

void IntegrationPluginEspSomfyRts::executeAction(ThingActionInfo *info)
{
    Thing *thing = info->thing();
    Action action = info->action();

    Q_UNUSED(thing)
    Q_UNUSED(action)
 }
