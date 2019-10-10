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

#include "devicepluginavahimonitor.h"

#include "devices/device.h"
#include "plugininfo.h"
#include "platform/platformzeroconfcontroller.h"
#include "network/zeroconf/zeroconfservicebrowser.h"

#include <QDebug>
#include <QStringList>
#include <QNetworkInterface>
#include <QDateTime>
#include <QTimer>

DevicePluginAvahiMonitor::DevicePluginAvahiMonitor()
{

}

void DevicePluginAvahiMonitor::setupDevice(DeviceSetupInfo *info)
{
    qCDebug(dcAvahiMonitor()) << "Setup" << info->device()->name() << info->device()->params();

    if (!m_serviceBrowser) {
        m_serviceBrowser = hardwareManager()->zeroConfController()->createServiceBrowser();
        connect(m_serviceBrowser, &ZeroConfServiceBrowser::serviceEntryAdded, this, &DevicePluginAvahiMonitor::onServiceEntryAdded);
        connect(m_serviceBrowser, &ZeroConfServiceBrowser::serviceEntryRemoved, this, &DevicePluginAvahiMonitor::onServiceEntryRemoved);
    }

    foreach (const ZeroConfServiceEntry &entry, m_serviceBrowser->serviceEntries()) {
        if (info->device()->paramValue(avahiDeviceServiceParamTypeId).toString() == entry.name() &&
                info->device()->paramValue(avahiDeviceHostNameParamTypeId).toString() == entry.hostName()) {
            info->device()->setStateValue(avahiIsPresentStateTypeId, true);
            info->device()->setStateValue(avahiLastSeenTimeStateTypeId, QDateTime::currentDateTime());
        }
    }

    info->finish(Device::DeviceErrorNoError);
}

void DevicePluginAvahiMonitor::discoverDevices(DeviceDiscoveryInfo *info)
{
    if (info->deviceClassId() != avahiDeviceClassId) {
        info->finish(Device::DeviceErrorDeviceClassNotFound);
        return;
    }

    if (!m_serviceBrowser) {
        m_serviceBrowser = hardwareManager()->zeroConfController()->createServiceBrowser();
        connect(m_serviceBrowser, &ZeroConfServiceBrowser::serviceEntryAdded, this, &DevicePluginAvahiMonitor::onServiceEntryAdded);
        connect(m_serviceBrowser, &ZeroConfServiceBrowser::serviceEntryRemoved, this, &DevicePluginAvahiMonitor::onServiceEntryRemoved);
    }

    // give it a bit of time to find things
    QTimer::singleShot(2000, info, [this, info](){
        QList<DeviceDescriptor> deviceDescriptors;
        foreach (const ZeroConfServiceEntry &service, m_serviceBrowser->serviceEntries()) {
            DeviceDescriptor deviceDescriptor(avahiDeviceClassId, service.name(), service.serviceType() + " (" + service.hostAddress().toString() + ")");
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

        info->addDeviceDescriptors(deviceDescriptors);
        info->finish(Device::DeviceErrorNoError);
    });
}

void DevicePluginAvahiMonitor::onServiceEntryAdded(const ZeroConfServiceEntry &serviceEntry)
{
    qCDebug(dcAvahiMonitor()) << "Service entry added:" << serviceEntry;
    foreach (Device *device, myDevices()) {
        if (device->paramValue(avahiDeviceServiceParamTypeId).toString() == serviceEntry.name() &&
                device->paramValue(avahiDeviceHostNameParamTypeId).toString() == serviceEntry.hostName()) {
            device->setStateValue(avahiIsPresentStateTypeId, true);
            device->setStateValue(avahiLastSeenTimeStateTypeId, QDateTime::currentDateTime());
        }
    }
}

void DevicePluginAvahiMonitor::onServiceEntryRemoved(const ZeroConfServiceEntry &serviceEntry)
{
    foreach (Device *device, myDevices()) {
        if (device->paramValue(avahiDeviceServiceParamTypeId).toString() == serviceEntry.name() &&
                device->paramValue(avahiDeviceHostNameParamTypeId).toString() == serviceEntry.hostName()) {
            device->setStateValue(avahiIsPresentStateTypeId, false);
        }
    }
}
