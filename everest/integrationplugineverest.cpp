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

IntegrationPluginTruffle::IntegrationPluginTruffle()
{

}

void IntegrationPluginTruffle::init()
{

}

void IntegrationPluginTruffle::startMonitoringAutoThings()
{
    // TODO: auto setup everest instance running on localhost
}

void IntegrationPluginTruffle::discoverThings(ThingDiscoveryInfo *info)
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
                QString description = result.networkDeviceInfo.address().toString() +
                                      " " + result.networkDeviceInfo.macAddress();
                ThingDescriptor descriptor(everestThingClassId, title, description);

                qCInfo(dcEverest()) << "Discovered -->" << title << description;

                ParamList params;
                params.append(Param(everestThingConnectorParamTypeId, connectorName));

                if (!MacAddress(result.networkDeviceInfo.macAddress()).isNull())
                    params.append(Param(everestThingMacParamTypeId, result.networkDeviceInfo.macAddress()));

                if (!result.networkDeviceInfo.address().isNull())
                    params.append(Param(everestThingAddressParamTypeId,
                                        result.networkDeviceInfo.address().toString()));

                descriptor.setParams(params);

                // Let's check if we aleardy have a thing with those parms
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

void IntegrationPluginTruffle::setupThing(ThingSetupInfo *info)
{
    Thing *thing = info->thing();

    QHostAddress address(thing->paramValue(everestThingAddressParamTypeId).toString());
    MacAddress macAddress(thing->paramValue(everestThingMacParamTypeId).toString());
    QString connector(thing->paramValue(everestThingConnectorParamTypeId).toString());

    if (!macAddress.isNull()) {

        qCInfo(dcEverest()) << "Setting up everest for" << macAddress.toString() << connector;

        EverestClient *everstClient = nullptr;

        foreach (EverestClient *ec, m_everstClients) {
            if (ec->macAddress() == macAddress) {
                // We have already a client for this host
                qCDebug(dcEverest()) << "Using existing" << ec;
                everstClient = ec;
            }
        }

        if (!everstClient) {
            qCDebug(dcEverest()) << "Creating new mac address based everst client";
            everstClient = new EverestClient(this);
            everstClient->setMacAddress(macAddress);
            everstClient->setMonitor(hardwareManager()->networkDeviceDiscovery()->registerMonitor(macAddress));
            m_everstClients.append(everstClient);
            everstClient->start();
        }

        everstClient->addThing(thing);
        m_thingClients.insert(thing, everstClient);
        info->finish(Thing::ThingErrorNoError);
        return;

    } else {

        qCInfo(dcEverest()) << "Setting up IP based everest for" << address.toString() << connector;

        EverestClient *everstClient = nullptr;

        foreach (EverestClient *ec, m_everstClients) {

            if (ec->address().isNull())
                continue;

            if (ec->address() == address) {
                // We have already a client for this host
                qCDebug(dcEverest()) << "Using existing" << ec;
                everstClient = ec;
                break;
            }
        }

        if (!everstClient) {
            qCDebug(dcEverest()) << "Creating new IP based everst client";
            everstClient = new EverestClient(this);
            everstClient->setAddress(address);
            m_everstClients.append(everstClient);
            everstClient->start();
        }

        everstClient->addThing(thing);
        m_thingClients.insert(thing, everstClient);
        info->finish(Thing::ThingErrorNoError);
        return;
    }
}

void IntegrationPluginTruffle::executeAction(ThingActionInfo *info)
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
            double current = info->action().paramValue(everestMaxChargingCurrentActionMaxChargingCurrentParamTypeId).toDouble();
            qCDebug(dcEverest()) << "Setting max charging current to" << current << thing;
            everest->setMaxChargingCurrent(current);
            thing->setStateValue(everestMaxChargingCurrentStateTypeId, current);
            info->finish(Thing::ThingErrorNoError);
        }

        return;
    }

    info->finish(Thing::ThingErrorNoError);
}

void IntegrationPluginTruffle::thingRemoved(Thing *thing)
{
    qCDebug(dcEverest()) << "Remove thing" << thing;
    if (thing->thingClassId() == everestThingClassId) {
        EverestClient *everestClient = m_thingClients.take(thing);
        everestClient->removeThing(thing);
        if (everestClient->things().isEmpty()) {
            qCDebug(dcEverest()) << "Deleting" << everestClient << "since there is no thing left";
            // No more things releated to this client, we can delete it
            m_everstClients.removeAll(everestClient);
            everestClient->deleteLater();
        }
    }
}


