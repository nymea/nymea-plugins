/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *                                                                         *
 *  Copyright (C) 2019 Bernhard Trinnes <bernhard.trinnes@nymea.io>        *
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

#include "devicepluginlogfilepublisher.h"
#include "devices/device.h"
#include "plugininfo.h"


DevicePluginLogfilePublisher::DevicePluginLogfilePublisher()
{

}

DevicePluginLogfilePublisher::~DevicePluginLogfilePublisher()
{

}

void DevicePluginLogfilePublisher::init()
{

}

Device::DeviceSetupStatus DevicePluginLogfilePublisher::setupDevice(Device *device)
{
    if (device->deviceClassId() == logfilePublisherDeviceClassId) {
        if (!m_fileSystem) {
            m_fileSystem = new FileSystem(this);
        }

        QString address = device->paramValue(logfilePublisherDeviceIpParamTypeId).toString();
        FtpUpload *ftpUpload = new FtpUpload(hardwareManager()->networkManager(),address , "", "", this);
        m_ftpUploads->insert(device, ftpUpload);

        return Device::DeviceSetupStatusSuccess;
    }
    return Device::DeviceSetupStatusFailure;
}

void DevicePluginLogfilePublisher::deviceRemoved(Device *device)
{
    Q_UNUSED(device)
}

Device::DeviceError DevicePluginLogfilePublisher::discoverDevices(const DeviceClassId &deviceClassId, const ParamList &params)
{
    Q_UNUSED(deviceClassId)
    Q_UNUSED(params)
    return Device::DeviceErrorNoError;
}

Device::DeviceError DevicePluginLogfilePublisher::executeAction(Device *device, const Action &action)
{
    if (device->deviceClassId() == logfilePublisherDeviceClassId) {

        if (action.actionTypeId() == logFilePublisherUploadActionTypeId) {

        }
        if (action.actionTypeId() == logfilePublisherLogFileBrowserItemActionTypeId) {

        }
    }
    return Device::DeviceErrorNoError;
}


Device::BrowseResult DevicePluginLogfilePublisher::browseDevice(Device *device, Device::BrowseResult result, const QString &itemId, const QLocale &locale)
{
     Q_UNUSED(device)
    Q_UNUSED(locale)

    return m_fileSystem->browse(itemId, result);
}

Device::BrowserItemResult DevicePluginLogfilePublisher::browserItem(Device *device, Device::BrowserItemResult result, const QString &itemId, const QLocale &locale)
{
    Q_UNUSED(locale)
    Q_UNUSED(device)
    return m_fileSystem->browserItem(itemId, result);
}

Device::DeviceError DevicePluginLogfilePublisher::executeBrowserItem(Device *device, const BrowserAction &browserAction)
{
    Q_UNUSED(device);
    return m_fileSystem->launchBrowserItem(browserAction.itemId());
}

Device::DeviceError DevicePluginLogfilePublisher::executeBrowserItemAction(Device *device, const BrowserItemAction &browserItemAction)
{
    Q_UNUSED(device)

    int id = m_fileSystem->executeBrowserItemAction(browserItemAction.itemId(), browserItemAction.actionTypeId());
    if (id == -1) {
        return Device::DeviceErrorHardwareFailure;
    }
    m_pendingBrowserItemActions.insert(id, browserItemAction.id());
    return Device::DeviceErrorAsync;
}


void DevicePluginLogfilePublisher::onConnectionChanged()
{

}

void DevicePluginLogfilePublisher::onUploadProgress(int percentage)
{
    FtpUpload *ftpUpload = static_cast<FtpUpload *>(sender());
    Device *device;
    if (!device) {
        return;
    }
    //device->setStateValue(logfil)
}
