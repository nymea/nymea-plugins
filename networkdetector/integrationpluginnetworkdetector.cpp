/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*
* Copyright 2013 - 2022, nymea GmbH
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

#include "integrationpluginnetworkdetector.h"

#include "plugininfo.h"

#include <QDebug>
#include <QStringList>

IntegrationPluginNetworkDetector::IntegrationPluginNetworkDetector()
{

}


IntegrationPluginNetworkDetector::~IntegrationPluginNetworkDetector()
{

}

void IntegrationPluginNetworkDetector::init()
{

}

void IntegrationPluginNetworkDetector::discoverThings(ThingDiscoveryInfo *info)
{
    if (!hardwareManager()->networkDeviceDiscovery()->available()) {
        qCWarning(dcNetworkDetector()) << "Failed to discover network devices. The network device discovery is not available.";
        info->finish(Thing::ThingErrorHardwareNotAvailable, QT_TR_NOOP("Unable to discovery devices in your network."));
        return;
    }

    qCDebug(dcNetworkDetector()) << "Starting network discovery...";
    NetworkDeviceDiscoveryReply *discoveryReply = hardwareManager()->networkDeviceDiscovery()->discover();
    connect(discoveryReply, &NetworkDeviceDiscoveryReply::hostAddressDiscovered, this, [=](const QHostAddress &address){
        qCDebug(dcNetworkDetector()) << "Address discovered" << address.toString();
    });

    connect(discoveryReply, &NetworkDeviceDiscoveryReply::networkDeviceInfoAdded, this, [=](const NetworkDeviceInfo &networkDeviceInfo){
        qCDebug(dcNetworkDetector()) << "-->" << networkDeviceInfo;
    });

    connect(discoveryReply, &NetworkDeviceDiscoveryReply::finished, this, [=](){
        ThingDescriptors descriptors;
        qCDebug(dcNetworkDetector()) << "Discovery finished. Found" << discoveryReply->networkDeviceInfos().count() << "devices";
        foreach (const NetworkDeviceInfo &networkDeviceInfo, discoveryReply->networkDeviceInfos()) {

            qCDebug(dcNetworkDetector()) << "-->" << networkDeviceInfo;
            QString title;
            if (networkDeviceInfo.hostName().isEmpty()) {
                title = networkDeviceInfo.macAddress();
            } else {
                title = networkDeviceInfo.hostName() + " (" + networkDeviceInfo.macAddress() + ")";
            }
            QString description;
            if (networkDeviceInfo.macAddressManufacturer().isEmpty()) {
                description =  networkDeviceInfo.address().toString();
            } else {
                description = networkDeviceInfo.address().toString() + " - " + networkDeviceInfo.macAddressManufacturer();
            }

            ThingDescriptor descriptor(networkDeviceThingClassId, title, description);
            ParamList params;
            params.append(Param(networkDeviceThingMacAddressParamTypeId, networkDeviceInfo.macAddress()));
            descriptor.setParams(params);

            // Check if we already have set up this device
            Things existingThings = myThings().filterByParam(networkDeviceThingMacAddressParamTypeId, networkDeviceInfo.macAddress());
            if (existingThings.count() == 1) {
                qCDebug(dcNetworkDetector()) << "This network device already exists in the system" << networkDeviceInfo;
                descriptor.setThingId(existingThings.first()->id());
            }
            descriptors.append(descriptor);
        }
        info->addThingDescriptors(descriptors);
        info->finish(Thing::ThingErrorNoError);
    });
}

void IntegrationPluginNetworkDetector::setupThing(ThingSetupInfo *info)
{
    Thing *thing = info->thing();
    qCDebug(dcNetworkDetector()) << "Setup" << thing->name() << thing->params();

    if (thing->thingClassId() == networkDeviceThingClassId) {

        MacAddress macAddress(thing->paramValue(networkDeviceThingMacAddressParamTypeId).toString());
        if (macAddress.isNull()) {
            qCWarning(dcNetworkDetector()) << "Invalid mac address:" << thing->paramValue(networkDeviceThingMacAddressParamTypeId).toString();
            info->finish(Thing::ThingErrorInvalidParameter, QT_TR_NOOP("The configured MAC address is not valid."));
            return;
        }

        NetworkDeviceMonitor *monitor = hardwareManager()->networkDeviceDiscovery()->registerMonitor(macAddress);
        connect(monitor, &NetworkDeviceMonitor::reachableChanged, this, [=](bool reachable){
            qCDebug(dcNetworkDetector()) << thing << "reachable changed to" << reachable;
            thing->setStateValue(networkDeviceIsPresentStateTypeId, reachable);
        });

        connect(monitor, &NetworkDeviceMonitor::lastSeenChanged, this, [=](const QDateTime &lastSeen){
            QDateTime minuteBased = QDateTime::fromMSecsSinceEpoch((monitor->lastSeen().toMSecsSinceEpoch() / 60000)  * 60000);
            qCDebug(dcNetworkDetector()) << thing << "last seen changed to" << lastSeen.toString() << minuteBased.toString();
            thing->setStateValue(networkDeviceLastSeenTimeStateTypeId, minuteBased.toMSecsSinceEpoch() / 1000);
        });

        connect(monitor, &NetworkDeviceMonitor::networkDeviceInfoChanged, this, [=](const NetworkDeviceInfo &networkInfo){
            qCDebug(dcNetworkDetector()) << thing << "changed" << networkInfo;
            thing->setStateValue(networkDeviceAddressStateTypeId, networkInfo.address().toString());
            thing->setStateValue(networkDeviceHostNameStateTypeId, networkInfo.hostName());
            thing->setStateValue(networkDeviceMacManufacturerNameStateTypeId, networkInfo.macAddressManufacturer());
            thing->setStateValue(networkDeviceNetworkInterfaceStateTypeId, monitor->networkDeviceInfo().networkInterface().name());
        });

        m_monitors.insert(thing, monitor);
        info->finish(Thing::ThingErrorNoError);

        thing->setStateValue(networkDeviceAddressStateTypeId, monitor->networkDeviceInfo().address().toString());
        thing->setStateValue(networkDeviceHostNameStateTypeId, monitor->networkDeviceInfo().hostName());
        thing->setStateValue(networkDeviceMacManufacturerNameStateTypeId, monitor->networkDeviceInfo().macAddressManufacturer());
        thing->setStateValue(networkDeviceNetworkInterfaceStateTypeId, monitor->networkDeviceInfo().networkInterface().name());
        thing->setStateValue(networkDeviceIsPresentStateTypeId, monitor->reachable());
        if (!monitor->lastSeen().isNull()) {
            QDateTime minuteBased = QDateTime::fromMSecsSinceEpoch((monitor->lastSeen().toMSecsSinceEpoch() / 60000)  * 60000);
            thing->setStateValue(networkDeviceLastSeenTimeStateTypeId, minuteBased.toMSecsSinceEpoch() / 1000);        }
        return;
    }

    info->finish(Thing::ThingErrorThingClassNotFound);
}

void IntegrationPluginNetworkDetector::thingRemoved(Thing *thing)
{
    if (m_monitors.contains(thing)) {
        hardwareManager()->networkDeviceDiscovery()->unregisterMonitor(m_monitors.take(thing));
    }
}

void IntegrationPluginNetworkDetector::executeAction(ThingActionInfo *info)
{
    if (info->thing()->thingClassId() == networkDeviceThingClassId) {

        NetworkDeviceMonitor *monitor = m_monitors.value(info->thing());
        if (!monitor) {
            qCWarning(dcNetworkDetector()) << "Could not execute action. There is no monitor registered for" << info->thing();
            info->finish(Thing::ThingErrorHardwareNotAvailable);
            return;
        }

        if (info->action().actionTypeId() == networkDevicePingActionTypeId) {
            PingReply *pingReply = hardwareManager()->networkDeviceDiscovery()->ping(monitor->networkDeviceInfo().address());
            connect(pingReply, &PingReply::finished, info, [=](){
                if (pingReply->error() == PingReply::ErrorNoError) {
                    qCDebug(dcNetworkDetector()) << "Ping finished for" << monitor->networkDeviceInfo().address().toString() << pingReply->duration() << "ms";
                    info->finish(Thing::ThingErrorNoError);
                } else {
                    qCWarning(dcNetworkDetector()) << "Ping" << monitor->networkDeviceInfo().address().toString() << "finished with error" << pingReply->error();
                    info->finish(Thing::ThingErrorHardwareNotAvailable);
                }
            });
        } else if (info->action().actionTypeId() == networkDeviceArpRequestActionTypeId) {
            bool result = hardwareManager()->networkDeviceDiscovery()->sendArpRequest(monitor->networkDeviceInfo().address());
            if (result) {
                qCDebug(dcNetworkDetector()) << "ARP request sent successfully to" << monitor->networkDeviceInfo().address().toString();
                info->finish(Thing::ThingErrorNoError);
            } else {
                qCWarning(dcNetworkDetector()) << "Failed to send ARP request to" << monitor->networkDeviceInfo().address().toString();
                info->finish(Thing::ThingErrorHardwareFailure);
            }
        } else if (info->action().actionTypeId() == networkDeviceLookupHostActionTypeId) {
            int lookupId = QHostInfo::lookupHost(monitor->networkDeviceInfo().address().toString(), this, SLOT(onHostLookupFinished(QHostInfo)));
            m_pendingHostLookup.insert(lookupId, info);
            connect(info, &ThingActionInfo::aborted, this, [=](){
                m_pendingHostLookup.remove(lookupId);
            });
        }
    }
}

void IntegrationPluginNetworkDetector::onHostLookupFinished(const QHostInfo &info)
{
    ThingActionInfo *actionInfo = m_pendingHostLookup.take(info.lookupId());
    if (!actionInfo) {
        qCWarning(dcNetworkDetector()) << "Host loopup finished for" << info.addresses() << info.hostName() << "but the action does not exist any more.";
        return;
    }

    qCDebug(dcNetworkDetector()) << "Host lookup finished" << info.addresses() << info.hostName() << info.error();
    if (info.error() != QHostInfo::NoError) {
        qCWarning(dcNetworkDetector()) << "Error occured during host lookup:" << info.errorString();
        actionInfo->finish(Thing::ThingErrorHardwareFailure);
    } else {
        actionInfo->finish(Thing::ThingErrorNoError);
    }
}
