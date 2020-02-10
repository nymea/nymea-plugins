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
