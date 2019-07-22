/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *                                                                         *
 *  Copyright (C) 2016 Simon Stürz <simon.stuerz@guh.io>                   *
 *  Copyright (C) 2016 Michael Zanetti <michael_zanetti@gmx.net>           *
 *                                                                         *
 *  This file is part of nymea.                                            *
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

#include "devicepluginnetworkdetector.h"

#include "devices/device.h"
#include "plugininfo.h"

#include <QDebug>
#include <QStringList>

DevicePluginNetworkDetector::DevicePluginNetworkDetector()
{
    m_discovery = new Discovery(this);
    connect(m_discovery, &Discovery::finished, this, &DevicePluginNetworkDetector::discoveryFinished);

    m_broadcastPing = new BroadcastPing(this);
    connect(m_broadcastPing, &BroadcastPing::finished, this, &DevicePluginNetworkDetector::broadcastPingFinished);
}

DevicePluginNetworkDetector::~DevicePluginNetworkDetector()
{
    if (m_discovery->isRunning()) {
        m_discovery->abort();
    }
}

void DevicePluginNetworkDetector::init()
{
}

Device::DeviceSetupStatus DevicePluginNetworkDetector::setupDevice(Device *device)
{
    qCDebug(dcNetworkDetector()) << "Setup" << device->name() << device->params();
    DeviceMonitor *monitor = new DeviceMonitor(device->name(),
                                               device->paramValue(networkDeviceDeviceMacAddressParamTypeId).toString(),
                                               device->paramValue(networkDeviceDeviceAddressParamTypeId).toString(),
                                               device->stateValue(networkDeviceIsPresentStateTypeId).toBool(),
                                               this);
    connect(monitor, &DeviceMonitor::reachableChanged, this, &DevicePluginNetworkDetector::deviceReachableChanged);
    connect(monitor, &DeviceMonitor::addressChanged, this, &DevicePluginNetworkDetector::deviceAddressChanged);
    connect(monitor, &DeviceMonitor::seen, this, &DevicePluginNetworkDetector::deviceSeen);
    monitor->setGracePeriod(device->setting(networkDeviceSettingsGracePeriodParamTypeId).toInt());
    m_monitors.insert(monitor, device);

    connect(device, &Device::settingChanged, this, [this, device](const ParamTypeId &paramTypeId, const QVariant &value){
        if (paramTypeId != networkDeviceSettingsGracePeriodParamTypeId) {
            return;
        }
        DeviceMonitor *monitor = m_monitors.key(device);
        if (monitor) {
            monitor->setGracePeriod(value.toInt());
        }
    });

    if (!m_pluginTimer) {
        m_pluginTimer = hardwareManager()->pluginTimerManager()->registerTimer(30);
        connect(m_pluginTimer, &PluginTimer::timeout, m_broadcastPing, &BroadcastPing::run);

        m_broadcastPing->run();
    }

    return Device::DeviceSetupStatusSuccess;
}

Device::DeviceError DevicePluginNetworkDetector::discoverDevices(const DeviceClassId &deviceClassId, const ParamList &params)
{
    Q_UNUSED(params)


    if (deviceClassId != networkDeviceDeviceClassId)
        return Device::DeviceErrorDeviceClassNotFound;

    if (m_discovery->isRunning()) {
        qCWarning(dcNetworkDetector()) << "Network discovery already running";
        return Device::DeviceErrorDeviceInUse;
    }

    m_discovery->discoverHosts(25);

    return Device::DeviceErrorAsync;
}

void DevicePluginNetworkDetector::deviceRemoved(Device *device)
{
    DeviceMonitor *monitor = m_monitors.key(device);
    m_monitors.remove(monitor);
    delete monitor;

    if (m_monitors.isEmpty()) {
        hardwareManager()->pluginTimerManager()->unregisterTimer(m_pluginTimer);
        m_pluginTimer = nullptr;

    }
}

void DevicePluginNetworkDetector::broadcastPingFinished()
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

        DeviceDescriptor descriptor(networkDeviceDeviceClassId, host.hostName().isEmpty() ? host.address() : host.hostName(), host.address() + " (" + host.macAddress() + ")");

        foreach (Device *existingDevice, myDevices()) {
            if (existingDevice->paramValue(networkDeviceDeviceMacAddressParamTypeId).toString() == host.macAddress()) {
                descriptor.setDeviceId(existingDevice->id());
                break;
            }
        }

        ParamList paramList;
        Param macAddress(networkDeviceDeviceMacAddressParamTypeId, host.macAddress());
        Param address(networkDeviceDeviceAddressParamTypeId, host.address());
        paramList.append(macAddress);
        paramList.append(address);
        descriptor.setParams(paramList);

        foreach (Device *existingDevice, myDevices()) {
            if (existingDevice->paramValue(networkDeviceDeviceMacAddressParamTypeId).toString() == host.macAddress()) {
                descriptor.setDeviceId(existingDevice->id());
                break;
            }
        }

        discoveredDevices.append(descriptor);
    }

    emit devicesDiscovered(networkDeviceDeviceClassId, discoveredDevices);
}

void DevicePluginNetworkDetector::deviceReachableChanged(bool reachable)
{
    DeviceMonitor *monitor = static_cast<DeviceMonitor*>(sender());
    Device *device = m_monitors.value(monitor);
    if (device->stateValue(networkDeviceIsPresentStateTypeId).toBool() != reachable) {
        qCDebug(dcNetworkDetector()) << "Device" << device->name() << device->paramValue(networkDeviceDeviceMacAddressParamTypeId).toString() << "reachable changed" << reachable;
        device->setStateValue(networkDeviceIsPresentStateTypeId, reachable);
    }
}

void DevicePluginNetworkDetector::deviceAddressChanged(const QString &address)
{
    DeviceMonitor *monitor = static_cast<DeviceMonitor*>(sender());
    Device *device = m_monitors.value(monitor);
    if (device->paramValue(networkDeviceDeviceAddressParamTypeId).toString() != address) {
        qCDebug(dcNetworkDetector()) << "Device" << device->name() << device->paramValue(networkDeviceDeviceMacAddressParamTypeId).toString() << "changed IP address to" << address;
        device->setParamValue(networkDeviceDeviceAddressParamTypeId.toString(), address);
    }
}

void DevicePluginNetworkDetector::deviceSeen()
{
    DeviceMonitor *monitor = static_cast<DeviceMonitor*>(sender());
    Device *device = m_monitors.value(monitor);
    QDateTime oldLastSeen = QDateTime::fromTime_t(device->stateValue(networkDeviceLastSeenTimeStateTypeId).toInt());
    if (oldLastSeen.addSecs(60) < QDateTime::currentDateTime()) {
        device->setStateValue(networkDeviceLastSeenTimeStateTypeId, QDateTime::currentDateTime().toTime_t());
    }
}
