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

#ifndef DEVICEPLUGINLOGFILEPUBLISHER_H
#define DEVICEPLUGINLOGFILEPUBLISHER_H

#include "devices/deviceplugin.h"
#include "plugintimer.h"
#include "filesystem.h"
#include "ftpupload.h"

#include <QHash>
#include <QDebug>

class DevicePluginLogfilePublisher : public DevicePlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "io.nymea.DevicePlugin" FILE "devicepluginlogfilepublisher.json")
    Q_INTERFACES(DevicePlugin)

public:
    explicit DevicePluginLogfilePublisher();
    ~DevicePluginLogfilePublisher() override;

    void init() override;
    Device::DeviceSetupStatus setupDevice(Device *device) override;
    void deviceRemoved(Device *device) override;
    Device::DeviceError discoverDevices(const DeviceClassId &deviceClassId, const ParamList &params) override;
    Device::DeviceError executeAction(Device *device, const Action &action) override;

    Device::BrowseResult browseDevice(Device *device, Device::BrowseResult result, const QString &itemId, const QLocale &locale) override;
    Device::BrowserItemResult browserItem(Device *device, Device::BrowserItemResult result, const QString &itemId, const QLocale &locale) override;
    Device::DeviceError executeBrowserItem(Device *device, const BrowserAction &browserAction) override;
    Device::DeviceError executeBrowserItemAction(Device *device, const BrowserItemAction &browserItemAction) override;

private:
    PluginTimer *m_pluginTimer;
    FileSystem *m_fileSystem;
    QHash<Device *, FtpUpload *> *m_ftpUploads;

    QHash<int, ActionId> m_pendingActions;
    QHash<int, ActionId> m_pendingBrowserItemActions;

private slots:
    void onPluginTimer();
    void onConnectionChanged();
};

#endif // DEVICEPLUGINLOGFILEPUBLISHER_H
