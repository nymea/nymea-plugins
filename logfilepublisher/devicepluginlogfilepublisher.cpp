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

DevicePluginLogfilePublisher::~DevicePluginLogfilePublisher()
{

}

Device::DeviceSetupStatus DevicePluginLogfilePublisher::setupDevice(Device *device)
{
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
        m_ftpUploads->insert(device, ftpUpload);

        return Device::DeviceSetupStatusSuccess;
    }
    return Device::DeviceSetupStatusFailure;
}

void DevicePluginLogfilePublisher::deviceRemoved(Device *device)
{
    if (device->deviceClassId() == logfilePublisherDeviceClassId) {
        FtpUpload *ftpUpload = m_ftpUploads->take(device);
        if (ftpUpload)
            ftpUpload->deleteLater();
    }

    if (myDevices().isEmpty()) {
        m_fileSystem->deleteLater();
    }
}


DevicePairingInfo DevicePluginLogfilePublisher::pairDevice(DevicePairingInfo &devicePairingInfo)
{
    qCDebug(dcLogfilePublisher()) << "PairDevice:" << devicePairingInfo.deviceClassId();

    devicePairingInfo.setStatus(Device::DeviceErrorNoError);
    devicePairingInfo.setMessage(tr("Please enter username and password for your ring doorbell account."));
    return devicePairingInfo;
}

DevicePairingInfo DevicePluginLogfilePublisher::confirmPairing(DevicePairingInfo &devicePairingInfo, const QString &username, const QString &secret)
{
    pluginStorage()->beginGroup(devicePairingInfo.deviceId().toString());
    pluginStorage()->setValue("username", username);
    pluginStorage()->setValue("password", secret);
    pluginStorage()->endGroup();

    devicePairingInfo.setStatus(Device::DeviceErrorNoError);
    return devicePairingInfo;
}


Device::DeviceError DevicePluginLogfilePublisher::executeAction(Device *device, const Action &action)
{
    if (device->deviceClassId() == logfilePublisherDeviceClassId) {
        FtpUpload *ftpUpload = m_ftpUploads->value(device);
        if (!ftpUpload)
            return  Device::DeviceErrorItemNotExecutable;

        if (action.actionTypeId() == logfilePublisherUploadActionTypeId) {

            QString fileName = device->stateValue(logfilePublisherFilenameStateTypeId).toString();
            QString targetFileName = QString::number(QDateTime::currentSecsSinceEpoch()) + "_" + QHostInfo::localHostName() + "_" + fileName;

            ftpUpload->uploadFile(fileName ,targetFileName);
            return Device::DeviceErrorNoError;
        }
        if (action.actionTypeId() == logfilePublisherLogFileBrowserItemActionTypeId) {
            qDebug(dcLogfilePublisher()) << "Logfile browser item action triggered";
            return Device::DeviceErrorNoError;
        }
        return Device::DeviceErrorActionTypeNotFound;
    }
    return Device::DeviceErrorDeviceClassNotFound;
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
    device->setStateValue(logfilePublisherFilenameStateTypeId, browserAction.itemId());
    return Device::DeviceErrorNoError;
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
    Device *device = m_ftpUploads->key(ftpUpload);
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
    Device *device = m_ftpUploads->key(ftpUpload);
    if (!device) {
        return;
    }
    device->setStateValue(logfilePublisherUploadInProgressStateTypeId, false);
}
