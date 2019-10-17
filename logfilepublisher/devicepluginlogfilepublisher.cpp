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

#include <QHostInfo>

DevicePluginLogfilePublisher::DevicePluginLogfilePublisher()
{

}

void DevicePluginLogfilePublisher::setupDevice(DeviceSetupInfo *info)
{
    Device *device = info->device();

    if (device->deviceClassId() == logfilePublisherDeviceClassId) {
        if (!m_fileSystem) {
            m_fileSystem = new FileSystem(this);
        }

        pluginStorage()->beginGroup(device->id().toString());
        QString username = pluginStorage()->value("username").toString();
        QString password = pluginStorage()->value("password").toString();
        pluginStorage()->endGroup();

        QHostAddress address = QHostAddress(device->paramValue(logfilePublisherDeviceIpParamTypeId).toString());
        int port = device->paramValue(logfilePublisherDevicePortParamTypeId).toInt();

        FtpUpload *ftpUpload = new FtpUpload(address, port, username, password, this);

        m_ftpUploads.insert(device, ftpUpload);
        info->finish(Device::DeviceErrorNoError);
        return;
    }
    qWarning(dcLogfilePublisher()) << "Device class not found";
    return;
}

void DevicePluginLogfilePublisher::deviceRemoved(Device *device)
{
    if (device->deviceClassId() == logfilePublisherDeviceClassId) {
        FtpUpload *ftpUpload = m_ftpUploads.take(device);
        if (ftpUpload)
            ftpUpload->deleteLater();
    }
    if (myDevices().isEmpty()) {
        m_fileSystem->deleteLater();
    }
}


void DevicePluginLogfilePublisher::startPairing(DevicePairingInfo *info)
{
    if (info->deviceClassId() == logfilePublisherDeviceClassId) {
        info->finish(Device::DeviceErrorNoError);
        return;
    }
}

void DevicePluginLogfilePublisher::confirmPairing(DevicePairingInfo *info, const QString &username, const QString &secret)
{
    if (info->deviceClassId() == logfilePublisherDeviceClassId) {
        pluginStorage()->beginGroup(info->deviceId().toString());
        pluginStorage()->setValue("username", username);
        pluginStorage()->setValue("password", secret);
        pluginStorage()->endGroup();

        info->finish(Device::DeviceErrorNoError);
        return;
    }
}

void DevicePluginLogfilePublisher::browseDevice(BrowseResult *result)
{
    qDebug(dcLogfilePublisher()) << "Browse device called" << result->itemId();
    m_fileSystem->browseDevice(result);
    return;
}

void DevicePluginLogfilePublisher::browserItem(BrowserItemResult *result)
{
    qDebug(dcLogfilePublisher()) << "Browse Item called" << result->itemId();
    m_fileSystem->browserItem(result);
    return;
}

void DevicePluginLogfilePublisher::executeBrowserItem(BrowserActionInfo *info)
{
    qDebug(dcLogfilePublisher()) << "Execute browser Item called" << info->browserAction().itemId();

    info->finish(Device::DeviceErrorNoError);
    return;
}

void DevicePluginLogfilePublisher::executeBrowserItemAction(BrowserItemActionInfo *info)
{
    qDebug(dcLogfilePublisher()) << "Execute browser Item action called" << info->browserItemAction().itemId();

    if (info->browserItemAction().actionTypeId() == logfilePublisherUploadBrowserItemActionTypeId) {
        FtpUpload *ftpUpload = m_ftpUploads.value(info->device());
        if (!ftpUpload)
            return;
        ftpUpload->uploadFile(info->browserItemAction().itemId(), "");
        info->finish(Device::DeviceErrorNoError);
    }
    return;
}

void DevicePluginLogfilePublisher::onConnectionChanged()
{

}

void DevicePluginLogfilePublisher::onUploadProgress(int percentage)
{
    FtpUpload *ftpUpload = static_cast<FtpUpload *>(sender());
    Device *device = m_ftpUploads.key(ftpUpload);
    if (!device) {
        return;
    }
    device->setStateValue(logfilePublisherUploadProgressStateTypeId, percentage);
    device->setStateValue(logfilePublisherUploadInProgressStateTypeId, true);
}

void DevicePluginLogfilePublisher::onUploadFinished(bool success)
{
    Q_UNUSED(success);
    FtpUpload *ftpUpload = static_cast<FtpUpload *>(sender());
    Device *device = m_ftpUploads.key(ftpUpload);
    if (!device) {
        return;
    }
    device->setStateValue(logfilePublisherUploadInProgressStateTypeId, false);
}
