/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *                                                                         *
 *  Copyright (C) 2015 Simon Stürz <simon.stuerz@guh.io>                   *
 *  Copyright (C) 2014 Michael Zanetti <michael_zanetti@gmx.net>           *
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

/*!
    \page avahimonitor.html
    \title Avahi Monitor
    \brief Plugin to monitor zeroconf devices in the local network.

    \ingroup plugins
    \ingroup nymea-plugins-maker

    \chapter Plugin properties
    Following JSON file contains the definition and the description of all available \l{DeviceClass}{DeviceClasses}
    and \l{Vendor}{Vendors} of this \l{DevicePlugin}.

    For more details how to read this JSON file please check out the documentation for \l{The plugin JSON File}.

    \quotefile plugins/deviceplugins/avahimonitor/devicepluginavahimonitor.json
*/


#include "devicepluginavahimonitor.h"

#include "plugin/device.h"
#include "devicemanager.h"
#include "plugininfo.h"
#include "platform/platformzeroconfcontroller.h"
#include "network/zeroconf/zeroconfservicebrowser.h"

#include <QDebug>
#include <QStringList>
#include <QNetworkInterface>
#include <QDateTime>

DevicePluginAvahiMonitor::DevicePluginAvahiMonitor()
{

}

DeviceManager::DeviceSetupStatus DevicePluginAvahiMonitor::setupDevice(Device *device)
{
    qCDebug(dcAvahiMonitor()) << "Setup" << device->name() << device->params();

    return DeviceManager::DeviceSetupStatusSuccess;
}

DeviceManager::DeviceError DevicePluginAvahiMonitor::discoverDevices(const DeviceClassId &deviceClassId, const ParamList &params)
{
    Q_UNUSED(params)

    if (deviceClassId != avahiDeviceClassId)
        return DeviceManager::DeviceErrorDeviceClassNotFound;

    if (!m_serviceBrowser) {
        m_serviceBrowser = hardwareManager()->zeroConfController()->createServiceBrowser();
        connect(m_serviceBrowser, &ZeroConfServiceBrowser::serviceEntryAdded, this, &DevicePluginAvahiMonitor::onServiceEntryAdded);
        connect(m_serviceBrowser, &ZeroConfServiceBrowser::serviceEntryRemoved, this, &DevicePluginAvahiMonitor::onServiceEntryRemoved);
    }

    QList<DeviceDescriptor> deviceDescriptors;
    foreach (const ZeroConfServiceEntry &service, m_serviceBrowser->serviceEntries()) {
        DeviceDescriptor deviceDescriptor(avahiDeviceClassId, service.name(), service.hostAddress().toString());
        ParamList params;
        params.append(Param(avahiDeviceServiceParamTypeId, service.name()));
        params.append(Param(avahiDeviceHostNameParamTypeId, service.hostName()));
        deviceDescriptor.setParams(params);
        foreach (Device *existingDevice, myDevices()) {
            if (existingDevice->paramValue(avahiDeviceServiceParamTypeId).toString() == service.name() && existingDevice->paramValue(avahiDeviceHostNameParamTypeId).toString() == service.hostName()) {
                deviceDescriptor.setDeviceId(existingDevice->id());
                break;
            }
        }
        deviceDescriptors.append(deviceDescriptor);
    }

    emit devicesDiscovered(avahiDeviceClassId, deviceDescriptors);

    return DeviceManager::DeviceErrorAsync;
}

void DevicePluginAvahiMonitor::onServiceEntryAdded(const ZeroConfServiceEntry &serviceEntry)
{
    qCDebug(dcAvahiMonitor()) << "Service entry added:" << serviceEntry;
    foreach (Device *device, myDevices()) {
        if (device->paramValue(avahiDeviceServiceParamTypeId).toString() == serviceEntry.name()) {
            device->setStateValue(avahiIsPresentStateTypeId, true);
            device->setStateValue(avahiLastSeenTimeStateTypeId, QDateTime::currentDateTime());
        }
    }
}

void DevicePluginAvahiMonitor::onServiceEntryRemoved(const ZeroConfServiceEntry &serviceEntry)
{
    foreach (Device *device, myDevices()) {
        if (device->paramValue(avahiDeviceServiceParamTypeId).toString() == serviceEntry.name()) {
            device->setStateValue(avahiIsPresentStateTypeId, false);
        }
    }
}
