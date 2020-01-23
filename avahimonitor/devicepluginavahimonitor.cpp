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
