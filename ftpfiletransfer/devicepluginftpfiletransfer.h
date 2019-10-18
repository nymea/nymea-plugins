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

#ifndef DEVICEPLUGINFTPFILETRANSFER_H
#define DEVICEPLUGINFTPFILETRANSFER_H

#include "devices/deviceplugin.h"
#include "plugintimer.h"
#include "filesystem.h"
#include "ftpupload.h"

#include <QHash>
#include <QDebug>

class DevicePluginFtpFileTransfer : public DevicePlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "io.nymea.DevicePlugin" FILE "devicepluginftpfiletransfer.json")
    Q_INTERFACES(DevicePlugin)

public:
    explicit DevicePluginFtpFileTransfer();

    void setupDevice(DeviceSetupInfo *info) override;
    void deviceRemoved(Device *device) override;

    void startPairing(DevicePairingInfo *info) override;
    void confirmPairing(DevicePairingInfo *info, const QString &username, const QString &secret) override;

    void browseDevice(BrowseResult *result) override;
    void browserItem(BrowserItemResult *result) override;
    void executeBrowserItem(BrowserActionInfo *info) override;
    void executeBrowserItemAction(BrowserItemActionInfo *info) override;

private:
    FileSystem *m_fileSystem;
    QHash<Device *, FtpUpload *> m_ftpUploads;

    QHash<int, ActionId> m_pendingActions;
    QHash<int, ActionId> m_pendingBrowserItemActions;

private slots:
    void onConnectionChanged();
    void onUploadProgress(int percentage);
    void onUploadFinished(bool success);
};

#endif // DEVICEPLUGINFTPFILETRANSFER_H
