/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *                                                                         *
 *  Copyright (C) 2016 Simon Stürz <simon.stuerz@guh.io>                   *
 *  Copyright (C) 2016 Michael Zanetti <michael_zanetti@gmx.net>           *
 *                                                                         *
 *  This file is part of guh.                                              *
 *                                                                         *
 *  This library is free software; you can redistribute it and/or          *
 *  modify it under the terms of the GNU Lesser General Public             *
 *  License as published by the Free Software Foundation; either           *
 *  version 2.1 of the License, or (at your option) any later version.     *
 *                                                                         *
 *  This library is distributed in the hope that it will be useful,        *
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of         *
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU      *
 *  Lesser General Public License for more details.                        *
 *                                                                         *
 *  You should have received a copy of the GNU Lesser General Public       *
 *  License along with this library; If not, see                           *
 *  <http://www.gnu.org/licenses/>.                                        *
 *                                                                         *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

/*!
    \page networkdetector.html
    \title Network Detector
    \brief Plugin to monitor devices in the local network.

    \ingroup plugins
    \ingroup guh-plugins


    This plugin allows to find and monitor network devices in your local network by using the hostname of the devices.

    \underline{NOTE}: the application \c nmap has to be installed and guh has to run as root.

    \chapter Plugin properties
    Following JSON file contains the definition and the description of all available \l{DeviceClass}{DeviceClasses}
    and \l{Vendor}{Vendors} of this \l{DevicePlugin}.

    For more details how to read this JSON file please check out the documentation for \l{The plugin JSON File}.

    \quotefile plugins/deviceplugins/networkdetector/devicepluginnetworkdetector.json
*/


#include "devicepluginnetworkdetector.h"

#include "plugin/device.h"
#include "devicemanager.h"
#include "plugininfo.h"

#include <QDebug>
#include <QStringList>

DevicePluginNetworkDetector::DevicePluginNetworkDetector()
{
    m_discovery = new Discovery(this);
    connect(m_discovery, &Discovery::finished, this, &DevicePluginNetworkDetector::discoveryFinished);
}

DevicePluginNetworkDetector::~DevicePluginNetworkDetector()
{
    hardwareManager()->pluginTimerManager()->unregisterTimer(m_pluginTimer);

    if (m_discovery->isRunning()) {
        m_discovery->abort();
    }
}

void DevicePluginNetworkDetector::init()
{
    m_pluginTimer = hardwareManager()->pluginTimerManager()->registerTimer(10);
    connect(m_pluginTimer, &PluginTimer::timeout, this, &DevicePluginNetworkDetector::onPluginTimer);
}

DeviceManager::DeviceSetupStatus DevicePluginNetworkDetector::setupDevice(Device *device)
{
    qCDebug(dcNetworkDetector()) << "Setup" << device->name() << device->params();
    DeviceMonitor *monitor = new DeviceMonitor(device->paramValue(macAddressParamTypeId).toString(), device->paramValue(addressParamTypeId).toString(), this);
    connect(monitor, &DeviceMonitor::reachableChanged, this, &DevicePluginNetworkDetector::deviceReachableChanged);
    connect(monitor, &DeviceMonitor::addressChanged, this, &DevicePluginNetworkDetector::deviceAddressChanged);
    m_monitors.insert(monitor, device);

    return DeviceManager::DeviceSetupStatusSuccess;
}

DeviceManager::DeviceError DevicePluginNetworkDetector::discoverDevices(const DeviceClassId &deviceClassId, const ParamList &params)
{
    Q_UNUSED(params)


    if (deviceClassId != networkDeviceDeviceClassId)
        return DeviceManager::DeviceErrorDeviceClassNotFound;

    if (m_discovery->isRunning()) {
        qCWarning(dcNetworkDetector()) << "Network discovery already running";
        return DeviceManager::DeviceErrorDeviceInUse;
    }

    m_discovery->discoverHosts(14);

    return DeviceManager::DeviceErrorAsync;
}

void DevicePluginNetworkDetector::deviceRemoved(Device *device)
{
    DeviceMonitor *monitor = m_monitors.key(device);
    m_monitors.remove(monitor);
    delete monitor;
}

void DevicePluginNetworkDetector::onPluginTimer()
{
    foreach (DeviceMonitor *monitor, m_monitors.keys()) {
        monitor->update();
    }
}

void DevicePluginNetworkDetector::discoveryFinished(const QList<Host> &hosts)
{
    qCDebug(dcNetworkDetector()) << "Discovery finished. Found" << hosts.count() << "devices";
    QList<DeviceDescriptor> discoveredDevices;
    foreach (const Host &host, hosts) {
        DeviceDescriptor descriptor(networkDeviceDeviceClassId, (host.hostName().isEmpty() ? host.address() : host.hostName() + "(" + host.address() + ")"), host.macAddress());

        ParamList paramList;
        Param macAddress(macAddressParamTypeId, host.macAddress());
        Param address(addressParamTypeId, host.address());
        paramList.append(macAddress);
        paramList.append(address);
        descriptor.setParams(paramList);

        discoveredDevices.append(descriptor);
    }

    emit devicesDiscovered(networkDeviceDeviceClassId, discoveredDevices);
}

void DevicePluginNetworkDetector::deviceReachableChanged(bool reachable)
{
    DeviceMonitor *monitor = static_cast<DeviceMonitor*>(sender());
    Device *device = m_monitors.value(monitor);
    if (device->stateValue(inRangeStateTypeId).toBool() != reachable) {
        qCDebug(dcNetworkDetector()) << "Device" << device->paramValue(macAddressParamTypeId).toString() << "reachable changed" << reachable;
        device->setStateValue(inRangeStateTypeId, reachable);
    }
}

void DevicePluginNetworkDetector::deviceAddressChanged(const QString &address)
{
    DeviceMonitor *monitor = static_cast<DeviceMonitor*>(sender());
    Device *device = m_monitors.value(monitor);
    if (device->paramValue(addressParamTypeId).toString() != address) {
        device->setParamValue(addressParamTypeId.toString(), address);
    }
}
