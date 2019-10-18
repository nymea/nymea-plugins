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

#include "devicepluginftpfiletransfer.h"
#include "devices/device.h"
#include "plugininfo.h"

#include <QHostInfo>

DevicePluginFtpFileTransfer::DevicePluginFtpFileTransfer()
{

}

void DevicePluginFtpFileTransfer::setupDevice(DeviceSetupInfo *info)
{
    Device *device = info->device();

    if (device->deviceClassId() == ftpFileTransferDeviceClassId) {
        if (!m_fileSystem) {
            m_fileSystem = new FileSystem(this);
        }

        pluginStorage()->beginGroup(device->id().toString());
        QString username = pluginStorage()->value("username").toString();
        QString password = pluginStorage()->value("password").toString();
        pluginStorage()->endGroup();

        QHostAddress address = QHostAddress(device->paramValue(ftpFileTransferDeviceIpParamTypeId).toString());
        int port = device->paramValue(ftpFileTransferDevicePortParamTypeId).toInt();

        FtpUpload *ftpUpload = new FtpUpload(address, port, username, password, this);

        m_ftpUploads.insert(device, ftpUpload);
        info->finish(Device::DeviceErrorNoError);
        return;
    }
    qCWarning(dcFtpFileTransfer()) << "Device class not found";
    return;
}

void DevicePluginFtpFileTransfer::deviceRemoved(Device *device)
{
    if (device->deviceClassId() == ftpFileTransferDeviceClassId) {
        FtpUpload *ftpUpload = m_ftpUploads.take(device);
        if (ftpUpload)
            ftpUpload->deleteLater();
    }
    if (myDevices().isEmpty()) {
        m_fileSystem->deleteLater();
    }
}


void DevicePluginFtpFileTransfer::startPairing(DevicePairingInfo *info)
{
    if (info->deviceClassId() == ftpFileTransferDeviceClassId) {
        info->finish(Device::DeviceErrorNoError);
        return;
    }
}

void DevicePluginFtpFileTransfer::confirmPairing(DevicePairingInfo *info, const QString &username, const QString &secret)
{
    if (info->deviceClassId() == ftpFileTransferDeviceClassId) {
        pluginStorage()->beginGroup(info->deviceId().toString());
        pluginStorage()->setValue("username", username);
        pluginStorage()->setValue("password", secret);
        pluginStorage()->endGroup();

        info->finish(Device::DeviceErrorNoError);
        return;
    }
}

void DevicePluginFtpFileTransfer::browseDevice(BrowseResult *result)
{
    qCDebug(dcFtpFileTransfer()) << "Browse device called" << result->itemId();
    m_fileSystem->browseDevice(result);
    return;
}

void DevicePluginFtpFileTransfer::browserItem(BrowserItemResult *result)
{
    qCDebug(dcFtpFileTransfer()) << "Browse Item called" << result->itemId();
    m_fileSystem->browserItem(result);
    return;
}

void DevicePluginFtpFileTransfer::executeBrowserItem(BrowserActionInfo *info)
{
    qCDebug(dcFtpFileTransfer()) << "Execute browser Item called" << info->browserAction().itemId();

    info->finish(Device::DeviceErrorNoError);
    return;
}

void DevicePluginFtpFileTransfer::executeBrowserItemAction(BrowserItemActionInfo *info)
{
    qCDebug(dcFtpFileTransfer()) << "Execute browser Item action called" << info->browserItemAction().itemId();

    if (info->browserItemAction().actionTypeId() == ftpFileTransferUploadBrowserItemActionTypeId) {
        FtpUpload *ftpUpload = m_ftpUploads.value(info->device());
        if (!ftpUpload)
            return;
        ftpUpload->uploadFile(info->browserItemAction().itemId(), "");
        info->finish(Device::DeviceErrorNoError);
    }
    return;
}

void DevicePluginFtpFileTransfer::onConnectionChanged()
{

}

void DevicePluginFtpFileTransfer::onUploadProgress(int percentage)
{
    FtpUpload *ftpUpload = static_cast<FtpUpload *>(sender());
    Device *device = m_ftpUploads.key(ftpUpload);
    if (!device) {
        return;
    }
    device->setStateValue(ftpFileTransferUploadProgressStateTypeId, percentage);
    device->setStateValue(ftpFileTransferUploadInProgressStateTypeId, true);
}

void DevicePluginFtpFileTransfer::onUploadFinished(bool success)
{
    Q_UNUSED(success);
    FtpUpload *ftpUpload = static_cast<FtpUpload *>(sender());
    Device *device = m_ftpUploads.key(ftpUpload);
    if (!device) {
        return;
    }
    device->setStateValue(ftpFileTransferUploadInProgressStateTypeId, false);
}
