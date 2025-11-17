// SPDX-License-Identifier: GPL-3.0-or-later

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*
* Copyright (C) 2013 - 2024, nymea GmbH
* Copyright (C) 2024 - 2025, chargebyte austria GmbH
*
* This file is part of nymea-plugins.
*
* nymea-plugins is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* (at your option) any later version.
*
* nymea-plugins is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
* General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with nymea-plugins. If not, see <https://www.gnu.org/licenses/>.
*
* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

#ifndef INTEGRATIONPLUGINKODI_H
#define INTEGRATIONPLUGINKODI_H

#include <integrations/integrationplugin.h>
#include <plugintimer.h>

#include "kodi.h"

#include <QHash>
#include <QDebug>
#include <QTcpSocket>

class ZeroConfServiceBrowser;

class IntegrationPluginKodi : public IntegrationPlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "io.nymea.IntegrationPlugin" FILE "integrationpluginkodi.json")
    Q_INTERFACES(IntegrationPlugin)

public:
    explicit IntegrationPluginKodi();
    ~IntegrationPluginKodi();

    void init() override;
    void setupThing(ThingSetupInfo *info) override;
    void postSetupThing(Thing *thing) override;
    void thingRemoved(Thing *thing) override;
    void discoverThings(ThingDiscoveryInfo *info) override;
    void executeAction(ThingActionInfo *info) override;

    void browseThing(BrowseResult *result) override;
    void browserItem(BrowserItemResult *result) override;
    void executeBrowserItem(BrowserActionInfo *info) override;
    void executeBrowserItemAction(BrowserItemActionInfo *info) override;

private:
    struct KodiHostInfo {
        QHostAddress address;
        uint rpcPort = 9090;
        uint httpPort = 8080;
    };

    KodiHostInfo resolve(Thing *thing);

private:
    PluginTimer *m_pluginTimer = nullptr;
    QHash<Thing *, Kodi *> m_kodis;
    ZeroConfServiceBrowser *m_serviceBrowser = nullptr;
    ZeroConfServiceBrowser *m_httpServiceBrowser = nullptr;

    QHash<int, ThingActionInfo *> m_pendingActions;
    QHash<int, BrowserActionInfo *> m_pendingBrowserActions;
    QHash<int, BrowserItemActionInfo *> m_pendingBrowserItemActions;

private slots:
    void onConnectionChanged(bool connected);
    void onStateChanged();
    void onActionExecuted(int actionId, bool success);
    void onBrowserItemExecuted(int actionId, bool success);
    void onBrowserItemActionExecuted(int actionId, bool success);

    void onPlaybackStatusChanged(const QString &playbackStatus);
};

#endif // INTEGRATIONPLUGINKODI_H
