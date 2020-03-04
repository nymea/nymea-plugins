/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*
* Copyright 2013 - 2020, nymea GmbH
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

#include "integrations/thing.h"
#include "plugininfo.h"

#include <QDebug>
#include <QStringList>

IntegrationPluginNetworkDetector::IntegrationPluginNetworkDetector()
{
    m_broadcastPing = new BroadcastPing(this);
    connect(m_broadcastPing, &BroadcastPing::finished, this, &IntegrationPluginNetworkDetector::broadcastPingFinished);
}

IntegrationPluginNetworkDetector::~IntegrationPluginNetworkDetector()
{
}

void IntegrationPluginNetworkDetector::init()
{
}

void IntegrationPluginNetworkDetector::setupThing(ThingSetupInfo *info)
{
    Thing *thing = info->thing();
    qCDebug(dcNetworkDetector()) << "Setup" << thing->name() << thing->params();
    DeviceMonitor *monitor = new DeviceMonitor(thing->name(),
                                               thing->paramValue(networkDeviceThingMacAddressParamTypeId).toString(),
                                               thing->paramValue(networkDeviceThingAddressParamTypeId).toString(),
                                               thing->stateValue(networkDeviceIsPresentStateTypeId).toBool(),
                                               this);
    connect(monitor, &DeviceMonitor::reachableChanged, this, &IntegrationPluginNetworkDetector::deviceReachableChanged);
    connect(monitor, &DeviceMonitor::addressChanged, this, &IntegrationPluginNetworkDetector::deviceAddressChanged);
    connect(monitor, &DeviceMonitor::seen, this, &IntegrationPluginNetworkDetector::deviceSeen);
    monitor->setGracePeriod(thing->setting(networkDeviceSettingsGracePeriodParamTypeId).toInt());
    m_monitors.insert(monitor, thing);

    connect(thing, &Thing::settingChanged, this, [this, thing](const ParamTypeId &paramTypeId, const QVariant &value){
        if (paramTypeId != networkDeviceSettingsGracePeriodParamTypeId) {
            return;
        }
        DeviceMonitor *monitor = m_monitors.key(thing);
        if (monitor) {
            monitor->setGracePeriod(value.toInt());
        }
    });

    if (!m_pluginTimer) {
        m_pluginTimer = hardwareManager()->pluginTimerManager()->registerTimer(30);
        connect(m_pluginTimer, &PluginTimer::timeout, m_broadcastPing, &BroadcastPing::run);

        m_broadcastPing->run();
    }

    info->finish(Thing::ThingErrorNoError);
}

void IntegrationPluginNetworkDetector::discoverThings(ThingDiscoveryInfo *info)
{
    Discovery *discovery = new Discovery(this);
    discovery->discoverHosts(25);

    // clean up discovery object when this discovery info is deleted
    connect(info, &ThingDiscoveryInfo::destroyed, discovery, &Discovery::deleteLater);

    connect(discovery, &Discovery::finished, info, [this, info](const QList<Host> &hosts) {
        qCDebug(dcNetworkDetector()) << "Discovery finished. Found" << hosts.count() << "devices";
        foreach (const Host &host, hosts) {
            ThingDescriptor descriptor(networkDeviceThingClassId, host.hostName().isEmpty() ? host.address() : host.hostName(), host.address() + " (" + host.macAddress() + ")");

            foreach (Thing *existingThing, myThings()) {
                if (existingThing->paramValue(networkDeviceThingMacAddressParamTypeId).toString() == host.macAddress()) {
                    descriptor.setThingId(existingThing->id());
                    break;
                }
            }

            ParamList params;
            params << Param(networkDeviceThingMacAddressParamTypeId, host.macAddress());
            params << Param(networkDeviceThingAddressParamTypeId, host.address());
            descriptor.setParams(params);

            info->addThingDescriptor(descriptor);

        }
        info->finish(Thing::ThingErrorNoError);
    });
}

void IntegrationPluginNetworkDetector::thingRemoved(Thing *thing)
{
    DeviceMonitor *monitor = m_monitors.key(thing);
    m_monitors.remove(monitor);
    delete monitor;

    if (m_monitors.isEmpty()) {
        hardwareManager()->pluginTimerManager()->unregisterTimer(m_pluginTimer);
        m_pluginTimer = nullptr;

    }
}

void IntegrationPluginNetworkDetector::broadcastPingFinished()
{
    foreach (DeviceMonitor *monitor, m_monitors.keys()) {
        monitor->update();
    }
}

void IntegrationPluginNetworkDetector::deviceReachableChanged(bool reachable)
{
    DeviceMonitor *monitor = static_cast<DeviceMonitor*>(sender());
    Thing *thing = m_monitors.value(monitor);
    if (thing->stateValue(networkDeviceIsPresentStateTypeId).toBool() != reachable) {
        qCDebug(dcNetworkDetector()) << "Device" << thing->name() << thing->paramValue(networkDeviceThingMacAddressParamTypeId).toString() << "reachable changed" << reachable;
        thing->setStateValue(networkDeviceIsPresentStateTypeId, reachable);
    }
}

void IntegrationPluginNetworkDetector::deviceAddressChanged(const QString &address)
{
    DeviceMonitor *monitor = static_cast<DeviceMonitor*>(sender());
    Thing *thing = m_monitors.value(monitor);
    if (thing->paramValue(networkDeviceThingAddressParamTypeId).toString() != address) {
        qCDebug(dcNetworkDetector()) << "Device" << thing->name() << thing->paramValue(networkDeviceThingMacAddressParamTypeId).toString() << "changed IP address to" << address;
        thing->setParamValue(networkDeviceThingAddressParamTypeId, address);
    }
}

void IntegrationPluginNetworkDetector::deviceSeen()
{
    DeviceMonitor *monitor = static_cast<DeviceMonitor*>(sender());
    Thing *thing = m_monitors.value(monitor);
    QDateTime oldLastSeen = QDateTime::fromTime_t(thing->stateValue(networkDeviceLastSeenTimeStateTypeId).toInt());
    if (oldLastSeen.addSecs(60) < QDateTime::currentDateTime()) {
        thing->setStateValue(networkDeviceLastSeenTimeStateTypeId, QDateTime::currentDateTime().toTime_t());
    }
}
