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

#ifndef INTEGRATIONPLUGINBLUOS_H
#define INTEGRATIONPLUGINBLUOS_H

#include "bluos.h"

#include <integrations/integrationplugin.h>
#include <platform/platformzeroconfcontroller.h>
#include <network/zeroconf/zeroconfservicebrowser.h>
#include <plugintimer.h>

#include <QUdpSocket>
#include <QNetworkAccessManager>

class PluginTimer;

class IntegrationPluginBluOS: public IntegrationPlugin
{
    Q_OBJECT

    Q_PLUGIN_METADATA(IID "io.nymea.IntegrationPlugin" FILE "integrationpluginbluos.json")
    Q_INTERFACES(IntegrationPlugin)

public:
    explicit IntegrationPluginBluOS();

    void init() override;
    void discoverThings(ThingDiscoveryInfo *info) override;
    void setupThing(ThingSetupInfo *info) override;
    void postSetupThing(Thing *thing) override;
    void thingRemoved(Thing *thing) override;
    void executeAction(ThingActionInfo *info) override;

    void browseThing(BrowseResult *result) override;
    void browserItem(BrowserItemResult *result) override;
    void executeBrowserItem(BrowserActionInfo *info) override;

private:
    PluginTimer *m_pluginTimer = nullptr;
    ZeroConfServiceBrowser *m_serviceBrowser = nullptr;

    QHash<ThingId, BluOS *> m_bluos;
    QHash<BluOS *, ThingSetupInfo *> m_asyncSetup;
    QHash<QUuid, ThingActionInfo *> m_asyncActions;
    QHash<QUuid, BrowseResult *> m_asyncBrowseResults;
    QHash<QUuid, BrowserActionInfo *> m_asyncExecuteBrowseItems;
    QHash<QUuid, BrowserItemResult *> m_asyncBrowseItemResults;

private slots:
    void onConnectionChanged(bool connected);
    void onStatusResponseReceived(const BluOS::StatusResponse &status);
    void onActionExecuted(QUuid actionId, bool success);
    void onVolumeReceived(int volume, bool mute);
    void onShuffleStateReceived(bool state);
    void onRepeatModeReceived(BluOS::RepeatMode mode);

    void onPresetsReceived(QUuid requestId, const QList<BluOS::Preset> &presets);
    void onSourcesReceived(QUuid requestId, const QList<BluOS::Source> &sources);
    void onBrowseResultReceived(QUuid requestId, const QList<BluOS::Source> &sources);

};
#endif // INTEGRATIONPLUGINBLUOS_H
