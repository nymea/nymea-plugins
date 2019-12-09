/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *                                                                         *
 *  Copyright (C) 2015 Simon Stürz <simon.stuerz@guh.io>                   *
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

#ifndef DEVICEPLUGINKODI_H
#define DEVICEPLUGINKODI_H

#include "devices/deviceplugin.h"
#include "plugintimer.h"
#include "kodi.h"

#include <QHash>
#include <QDebug>
#include <QTcpSocket>

class ZeroConfServiceBrowser;

class DevicePluginKodi : public DevicePlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "io.nymea.DevicePlugin" FILE "devicepluginkodi.json")
    Q_INTERFACES(DevicePlugin)

public:
    explicit DevicePluginKodi();
    ~DevicePluginKodi();

    void init() override;
    void setupDevice(DeviceSetupInfo *info) override;
    void deviceRemoved(Device *device) override;
    void discoverDevices(DeviceDiscoveryInfo *info) override;
    void executeAction(DeviceActionInfo *info) override;

    void browseDevice(BrowseResult *result) override;
    void browserItem(BrowserItemResult *result) override;
    void executeBrowserItem(BrowserActionInfo *info) override;
    void executeBrowserItemAction(BrowserItemActionInfo *info) override;

private:
    PluginTimer *m_pluginTimer;
    QHash<Kodi*, Device*> m_kodis;
    QHash<Kodi*, DeviceSetupInfo*> m_asyncSetups;
    ZeroConfServiceBrowser *m_serviceBrowser = nullptr;
    ZeroConfServiceBrowser *m_httpServiceBrowser = nullptr;

    QHash<int, DeviceActionInfo*> m_pendingActions;
    QHash<int, BrowserActionInfo*> m_pendingBrowserActions;
    QHash<int, BrowserItemActionInfo*> m_pendingBrowserItemActions;

private slots:
    void onPluginTimer();
    void onConnectionChanged(bool connected);
    void onStateChanged();
    void onActionExecuted(int actionId, bool success);
    void onBrowserItemExecuted(int actionId, bool success);
    void onBrowserItemActionExecuted(int actionId, bool success);

    void onPlaybackStatusChanged(const QString &playbackStatus);
};

#endif // DEVICEPLUGINKODI_H
