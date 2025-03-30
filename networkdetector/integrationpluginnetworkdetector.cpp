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

void IntegrationPluginNetworkDetector::discoverThings(ThingDiscoveryInfo *info)
{
    if (!hardwareManager()->networkDeviceDiscovery()->available()) {
        qCWarning(dcNetworkDetector()) << "Failed to discover network devices. The network device discovery is not available.";
        info->finish(Thing::ThingErrorHardwareNotAvailable, QT_TR_NOOP("Unable to discovery devices in your network."));
        return;
    }

    qCDebug(dcNetworkDetector()) << "Starting network discovery...";
    NetworkDeviceDiscoveryReply *discoveryReply = hardwareManager()->networkDeviceDiscovery()->discover();
    connect(discoveryReply, &NetworkDeviceDiscoveryReply::finished, discoveryReply, &NetworkDeviceDiscoveryReply::deleteLater);

    connect(discoveryReply, &NetworkDeviceDiscoveryReply::hostAddressDiscovered, this, [=](const QHostAddress &address){
        qCDebug(dcNetworkDetector()) << "Host address discovered" << address.toString();
    });

    connect(discoveryReply, &NetworkDeviceDiscoveryReply::finished, info, [=](){
        qCDebug(dcNetworkDetector()) << "Discovery finished. Found" << discoveryReply->networkDeviceInfos().count() << "devices";
        foreach (const NetworkDeviceInfo &networkDeviceInfo, discoveryReply->networkDeviceInfos()) {

            qCDebug(dcNetworkDetector()) << "-->" << networkDeviceInfo;

            QString title;
            QString description;
            MacAddressInfo macInfo;

            switch (networkDeviceInfo.monitorMode()) {
            case NetworkDeviceInfo::MonitorModeMac:
                macInfo = networkDeviceInfo.macAddressInfos().constFirst();
                description = networkDeviceInfo.address().toString();
                if (!macInfo.vendorName().isEmpty())
                    description += " - " + networkDeviceInfo.macAddressInfos().constFirst().vendorName();

                if (networkDeviceInfo.hostName().isEmpty()) {
                    title = macInfo.macAddress().toString();
                } else {
                    title = networkDeviceInfo.hostName() + " (" + macInfo.macAddress().toString() + ")";
                }

                break;
            case NetworkDeviceInfo::MonitorModeHostName:
                title = networkDeviceInfo.hostName();
                description = networkDeviceInfo.address().toString();
                break;
            case NetworkDeviceInfo::MonitorModeIp:
                title = "Network device " + networkDeviceInfo.address().toString();
                description = "Interface: " + networkDeviceInfo.networkInterface().name();
                break;
            }

            ThingDescriptor descriptor(networkDeviceThingClassId, title, description);

            // Note: the network device info already provides the correct set of parameters in order to be used by the monitor
            // depending on the possibilities within this network. It is not recommended to fill in all information available.
            // Only the information available depending on the monitor mode are relevant for the monitor.
            ParamList params;
            params.append(Param(networkDeviceThingMacAddressParamTypeId, networkDeviceInfo.thingParamValueMacAddress()));
            params.append(Param(networkDeviceThingHostNameParamTypeId, networkDeviceInfo.thingParamValueHostName()));
            params.append(Param(networkDeviceThingAddressParamTypeId, networkDeviceInfo.thingParamValueAddress()));
            descriptor.setParams(params);

            info->addThingDescriptor(descriptor);
        }
        info->finish(Thing::ThingErrorNoError);
    });
}

void IntegrationPluginNetworkDetector::setupThing(ThingSetupInfo *info)
{
    Thing *thing = info->thing();
    qCDebug(dcNetworkDetector()) << "Setup" << thing->name() << thing->params();

    if (thing->thingClassId() == networkDeviceThingClassId) {

        // Handle reconfigure
        if (m_monitors.contains(thing)) {
            hardwareManager()->networkDeviceDiscovery()->unregisterMonitor(m_monitors.take(thing));
        }

        NetworkDeviceMonitor *monitor = hardwareManager()->networkDeviceDiscovery()->registerMonitor(thing);
        m_monitors.insert(thing, monitor);
        info->finish(Thing::ThingErrorNoError);

        connect(monitor, &NetworkDeviceMonitor::reachableChanged, thing, [=](bool reachable){
            qCDebug(dcNetworkDetector()) << thing->name() << "monitor reachable changed to" << reachable;
            if (reachable) {
                thing->setStateValue(networkDeviceIsPresentStateTypeId, reachable);
                // Remove any possible running grace priod timer
                if (m_gracePeriodTimers.contains(thing)) {
                    hardwareManager()->pluginTimerManager()->unregisterTimer(m_gracePeriodTimers.take(thing));
                }
            } else {
                int gracePeriodSeconds = thing->setting(networkDeviceSettingsGracePeriodParamTypeId).toInt() * 60;
                if (gracePeriodSeconds == 0) {
                    thing->setStateValue(networkDeviceIsPresentStateTypeId, reachable);
                } else {
                    // Let's wait the grace period before setting not present
                    PluginTimer *timer = hardwareManager()->pluginTimerManager()->registerTimer(gracePeriodSeconds);
                    connect(timer, &PluginTimer::timeout, thing, [=](){
                        if (thing->stateValue(networkDeviceIsPresentStateTypeId).toBool() && monitor->lastSeen().addSecs(gracePeriodSeconds) < QDateTime::currentDateTime()) {
                            qCDebug(dcNetworkDetector()) << thing->name() << "still not reachable after a grace period of" << gracePeriodSeconds << "seconds. Mark device as not reachable.";
                            thing->setStateValue(networkDeviceIsPresentStateTypeId, reachable);
                            hardwareManager()->pluginTimerManager()->unregisterTimer(m_gracePeriodTimers.take(thing));
                        }
                    });

                    m_gracePeriodTimers.insert(thing, timer);
                    qCDebug(dcNetworkDetector()) << "Starting grace period timer" << m_gracePeriodTimers << "seconds";
                    timer->start();
                }
            }
        });

        connect(monitor, &NetworkDeviceMonitor::lastSeenChanged, thing, [=](const QDateTime &lastSeen){
            QDateTime minuteBased = QDateTime::fromMSecsSinceEpoch((monitor->lastSeen().toMSecsSinceEpoch() / 60000)  * 60000);
            qCDebug(dcNetworkDetector()) << thing << "last seen changed to" << lastSeen.toString() << minuteBased.toString();
            thing->setStateValue(networkDeviceLastSeenTimeStateTypeId, minuteBased.toMSecsSinceEpoch() / 1000);
        });

        connect(monitor, &NetworkDeviceMonitor::networkDeviceInfoChanged, thing, [=](const NetworkDeviceInfo &networkInfo){
            Q_UNUSED(networkInfo)
            updateStates(thing, monitor);
        });

        // Set initial states from cache
        qCDebug(dcNetworkDetector()) << "Set initial states for:" << thing << monitor->networkDeviceInfo();
        updateStates(thing, monitor);

    } else {
        info->finish(Thing::ThingErrorThingClassNotFound);
    }
}

void IntegrationPluginNetworkDetector::thingRemoved(Thing *thing)
{
    if (m_monitors.contains(thing)) {
        hardwareManager()->networkDeviceDiscovery()->unregisterMonitor(m_monitors.take(thing));
    }

    if (m_gracePeriodTimers.contains(thing)) {
        hardwareManager()->pluginTimerManager()->unregisterTimer(m_gracePeriodTimers.take(thing));
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

void IntegrationPluginNetworkDetector::updateStates(Thing *thing, NetworkDeviceMonitor *monitor)
{
    thing->setStateValue(networkDeviceLastSeenTimeStateTypeId, monitor->lastSeen().toMSecsSinceEpoch() / 1000);
    thing->setStateValue(networkDeviceIsPresentStateTypeId, monitor->reachable());
    thing->setStateValue(networkDeviceAddressStateTypeId, monitor->networkDeviceInfo().address().toString());
    thing->setStateValue(networkDeviceHostNameStateTypeId, monitor->networkDeviceInfo().hostName());
    if (monitor->networkDeviceInfo().monitorMode() == NetworkDeviceInfo::MonitorModeMac && monitor->networkDeviceInfo().macAddressInfos().count() == 1)
        thing->setStateValue(networkDeviceMacManufacturerNameStateTypeId, monitor->networkDeviceInfo().macAddressInfos().constFirst().vendorName());

    thing->setStateValue(networkDeviceNetworkInterfaceStateTypeId, monitor->networkDeviceInfo().networkInterface().name());
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
