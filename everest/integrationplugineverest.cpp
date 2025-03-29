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

#include "integrationplugineverest.h"
#include "plugininfo.h"
#include "everestdiscovery.h"

#include <network/networkdevicediscovery.h>

IntegrationPluginEverest::IntegrationPluginEverest()
{

}

void IntegrationPluginEverest::init()
{

}

void IntegrationPluginEverest::startMonitoringAutoThings()
{
    // Check on localhost if there is any EVerest instance running and if we have to set up a thing for this EV charger
    // Since this integration plugin is most luikly running on an EV charger running EVerest, the local instance should
    // be set up automatically. Additional instances in the network can still be added by running a normal network discovery

    EverestDiscovery *discovery = new EverestDiscovery(nullptr, this);
    connect(discovery, &EverestDiscovery::finished, discovery, &EverestDiscovery::deleteLater);
    connect(discovery, &EverestDiscovery::finished, this, [this, discovery](){

        ThingDescriptors descriptors;

        foreach (const EverestDiscovery::Result &result, discovery->results()) {

            // Create one EV charger foreach available connector on that host
            foreach(const QString &connectorName, result.connectors) {

                QString title = QString("EVerest");
                QString description = connectorName;
                ThingDescriptor descriptor(everestThingClassId, title, description);

                qCInfo(dcEverest()) << "Discovered -->" << title << description;

                ParamList params;
                params.append(Param(everestThingConnectorParamTypeId, connectorName));
                params.append(Param(everestThingAddressParamTypeId, result.networkDeviceInfo.address().toString()));
                descriptor.setParams(params);

                // Let's check if we aleardy have a thing with those params
                bool thingExists = true;
                Thing *existingThing = nullptr;
                foreach (Thing *thing, myThings()) {
                    foreach(const Param &param, params) {
                        if (param.value() != thing->paramValue(param.paramTypeId())) {
                            thingExists = false;
                            break;
                        }
                    }

                    // The params are equal, we already have set up this thing
                    if (thingExists) {
                        existingThing = thing;
                    }
                }

                // Add only connectors we don't have set up yet
                if (existingThing) {
                    qCDebug(dcEverest()) << "Discovered EVerest connector on localhost but we already set up this connector" << existingThing->name() << existingThing->params();
                } else {
                    qCDebug(dcEverest()) << "Adding new EVerest connector on localhost" << title << params;
                    descriptors.append(descriptor);
                }
            }
        }

        if (!descriptors.isEmpty()) {
            qCDebug(dcEverest()) << "Adding" << descriptors.count() << "new EVerest instances.";
            emit autoThingsAppeared(descriptors);
        }
    });

    discovery->startLocalhost();
}

void IntegrationPluginEverest::discoverThings(ThingDiscoveryInfo *info)
{
    qCDebug(dcEverest()) << "Start discovering Everest systems in the local network";
    if (!hardwareManager()->networkDeviceDiscovery()->available()) {
        qCWarning(dcEverest()) << "The network discovery is not available on this platform.";
        info->finish(Thing::ThingErrorUnsupportedFeature, QT_TR_NOOP("The network device discovery is not available."));
        return;
    }

    EverestDiscovery *discovery = new EverestDiscovery(hardwareManager()->networkDeviceDiscovery(), this);
    connect(discovery, &EverestDiscovery::finished, discovery, &EverestDiscovery::deleteLater);
    connect(discovery, &EverestDiscovery::finished, info, [this, info, discovery](){

        foreach (const EverestDiscovery::Result &result, discovery->results()) {

            // Create one EV charger foreach available connector on that host
            foreach(const QString &connectorName, result.connectors) {

                QString title = QString("Everest (%1)").arg(connectorName);
                QString description;
                MacAddressInfo macInfo;

                switch (result.networkDeviceInfo.monitorMode()) {
                case NetworkDeviceInfo::MonitorModeMac:
                    macInfo = result.networkDeviceInfo.macAddressInfos().constFirst();
                    description = result.networkDeviceInfo.address().toString();
                    if (!macInfo.vendorName().isEmpty())
                        description += " - " + result.networkDeviceInfo.macAddressInfos().constFirst().vendorName();

                    break;
                case NetworkDeviceInfo::MonitorModeHostName:
                    description = result.networkDeviceInfo.address().toString();
                    break;
                case NetworkDeviceInfo::MonitorModeIp:
                    description = "Interface: " +  result.networkDeviceInfo.networkInterface().name();
                    break;
                }

                ThingDescriptor descriptor(everestThingClassId, title, description);
                qCInfo(dcEverest()) << "Discovered -->" << title << description;

                // Note: the network device info already provides the correct set of parameters in order to be used by the monitor
                // depending on the possibilities within this network. It is not recommended to fill in all information available.
                // Only the information available depending on the monitor mode are relevant for the monitor.
                ParamList params;
                params.append(Param(everestThingConnectorParamTypeId, connectorName));
                params.append(Param(everestThingMacAddressParamTypeId, result.networkDeviceInfo.thingParamValueMacAddress()));
                params.append(Param(everestThingHostNameParamTypeId, result.networkDeviceInfo.thingParamValueHostName()));
                params.append(Param(everestThingAddressParamTypeId, result.networkDeviceInfo.thingParamValueAddress()));
                descriptor.setParams(params);

                // Let's check if we aleardy have a thing with those params
                bool thingExists = true;
                Thing *existingThing = nullptr;
                foreach (Thing *thing, myThings()) {
                    if (thing->thingClassId() != info->thingClassId())
                        continue;

                    foreach(const Param &param, params) {
                        if (param.value() != thing->paramValue(param.paramTypeId())) {
                            thingExists = false;
                            break;
                        }
                    }

                    // The params are equal, we already know this thing
                    if (thingExists)
                        existingThing = thing;
                }

                // Set the thing ID id we already have this device (for reconfiguration)
                if (existingThing)
                    descriptor.setThingId(existingThing->id());

                info->addThingDescriptor(descriptor);
            }
        }

        // All discovery results processed, we are done
        info->finish(Thing::ThingErrorNoError);
    });

    discovery->start();
}

void IntegrationPluginEverest::setupThing(ThingSetupInfo *info)
{
    Thing *thing = info->thing();

    QHostAddress address(thing->paramValue(everestThingAddressParamTypeId).toString());
    MacAddress macAddress(thing->paramValue(everestThingMacAddressParamTypeId).toString());
    QString hostName(thing->paramValue(everestThingHostNameParamTypeId).toString());
    QString connector(thing->paramValue(everestThingConnectorParamTypeId).toString());

    EverestClient *everstClient = nullptr;

    foreach (EverestClient *ec, m_everstClients) {
        if (ec->monitor()->macAddress() == macAddress &&
            ec->monitor()->hostName() == hostName &&
            ec->monitor()->address() == address) {
            // We have already a client for this host
            qCDebug(dcEverest()) << "Using existing" << ec;
            everstClient = ec;
        }
    }

    if (!everstClient) {
        everstClient = new EverestClient(this);
        everstClient->setMonitor(hardwareManager()->networkDeviceDiscovery()->registerMonitor(thing));
        m_everstClients.append(everstClient);
        qCDebug(dcEverest()) << "Created new" << everstClient;
        everstClient->start();
    }

    everstClient->addThing(thing);
    m_thingClients.insert(thing, everstClient);
    info->finish(Thing::ThingErrorNoError);
    return;
}

void IntegrationPluginEverest::executeAction(ThingActionInfo *info)
{
    qCDebug(dcEverest()) << "Executing action for thing" << info->thing()
    << info->action().actionTypeId().toString() << info->action().params();

    if (info->thing()->thingClassId() == everestThingClassId) {

        Thing *thing = info->thing();
        EverestClient *everstClient = m_thingClients.value(thing);
        if (!everstClient) {
            qCWarning(dcEverest()) << "Failed to execute action. Unable to find everst client for" << thing;
            info->finish(Thing::ThingErrorHardwareFailure);
            return;
        }

        Everest *everest = everstClient->getEverest(thing);
        if (!everest) {
            qCWarning(dcEverest()) << "Failed to execute action. Unable to find everst for"
                                   << thing << "on" << everstClient;
            info->finish(Thing::ThingErrorHardwareFailure);
            return;
        }

        if (!thing->stateValue(everestConnectedStateTypeId).toBool()) {
            info->finish(Thing::ThingErrorHardwareNotAvailable);
            return;
        }

        // All checks where good, let's execute the action
        if (info->action().actionTypeId() == everestPowerActionTypeId) {
            bool power = info->action().paramValue(everestPowerActionPowerParamTypeId).toBool();
            qCDebug(dcEverest()) << (power ? "Resume charging on" : "Pause charging on") << thing;
            everest->enableCharging(power);
            thing->setStateValue(everestPowerStateTypeId, power);
            info->finish(Thing::ThingErrorNoError);
        } else if (info->action().actionTypeId() == everestMaxChargingCurrentActionTypeId) {
            // Note: once we support phase switching, we cannot use the
            uint phaseCount = thing->stateValue(everestDesiredPhaseCountStateTypeId).toUInt();
            double current = info->action().paramValue(everestMaxChargingCurrentActionMaxChargingCurrentParamTypeId).toDouble();
            qCDebug(dcEverest()).nospace() << "Setting max charging current to " << current << "A (Phases: " << phaseCount << ") " << thing;
            everest->setMaxChargingCurrentAndPhaseCount(phaseCount, current);
            thing->setStateValue(everestMaxChargingCurrentStateTypeId, current);
            info->finish(Thing::ThingErrorNoError);
        } else if (info->action().actionTypeId() == everestDesiredPhaseCountActionTypeId) {
            uint phaseCount = info->action().paramValue(everestDesiredPhaseCountActionDesiredPhaseCountParamTypeId).toUInt();
            double current = thing->stateValue(everestMaxChargingCurrentStateTypeId).toDouble();
            qCDebug(dcEverest()).nospace() << "Setting desired phase count to " << phaseCount << " (" << current << "A) " << thing;
            everest->setMaxChargingCurrentAndPhaseCount(phaseCount, current);
            thing->setStateValue(everestDesiredPhaseCountStateTypeId, phaseCount);
            info->finish(Thing::ThingErrorNoError);
        }

        return;
    }

    info->finish(Thing::ThingErrorNoError);
}

void IntegrationPluginEverest::thingRemoved(Thing *thing)
{
    qCDebug(dcEverest()) << "Remove thing" << thing;
    if (thing->thingClassId() == everestThingClassId) {
        EverestClient *everestClient = m_thingClients.take(thing);
        everestClient->removeThing(thing);
        if (everestClient->things().isEmpty()) {
            qCDebug(dcEverest()) << "Deleting" << everestClient << "since there is no thing left";
            // No more things related to this client, we can delete it
            m_everstClients.removeAll(everestClient);

            // Unregister monitor
            if (everestClient->monitor())
                hardwareManager()->networkDeviceDiscovery()->unregisterMonitor(everestClient->monitor());


            everestClient->deleteLater();
        }


    }
}


