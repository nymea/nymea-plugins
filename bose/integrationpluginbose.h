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

#ifndef INTEGRATIONPLUGINBOSE_H
#define INTEGRATIONPLUGINBOSE_H

#include <integrations/integrationplugin.h>
#include <network/zeroconf/zeroconfservicebrowser.h>
#include <plugintimer.h>

#include "soundtouch.h"
#include "soundtouchtypes.h"

#include <QHash>
#include <QDebug>
#include <QUuid>

class IntegrationPluginBose : public IntegrationPlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "io.nymea.IntegrationPlugin" FILE "integrationpluginbose.json")
    Q_INTERFACES(IntegrationPlugin)

public:
    explicit IntegrationPluginBose();
    ~IntegrationPluginBose() override;

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
    QString m_consumerKey;
    ZeroConfServiceBrowser *m_serviceBrowser = nullptr;
    PluginTimer *m_pluginTimer = nullptr;

    QHash<Thing *, SoundTouch *> m_soundTouch;
    QHash<QUuid, ThingActionInfo *> m_pendingActions;
    QHash<QUuid, BrowseResult *> m_asyncBrowseResults;
    QHash<QUuid, BrowserActionInfo *> m_asyncExecuteBrowseItems;
    QHash<QUuid, BrowserItemResult *> m_asyncBrowseItemResults;
    QHash<Thing *, SourcesObject> m_sourcesObjects;

private slots:
    void onPluginTimer();
    void onConnectionChanged(bool connected);
    void onDeviceNameChanged();
    void onRequestExecuted(QUuid requestId, bool success);

    void onInfoObjectReceived(QUuid requestId, InfoObject infoObject);
    void onNowPlayingObjectReceived(QUuid requestId, NowPlayingObject nowPlaying);
    void onVolumeObjectReceived(QUuid requestId, VolumeObject volume);
    void onSourcesObjectReceived(QUuid requestId, SourcesObject sources);
    void onBassObjectReceived(QUuid requestId, BassObject bass);
    void onBassCapabilitiesObjectReceived(QUuid requestId, BassCapabilitiesObject bassCapabilities);
    void onGroupObjectReceived(QUuid requestId, GroupObject group);
    void onZoneObjectReceived(QUuid requestId, ZoneObject zone);
    void onPresetsReceived(QUuid requestId, QList<PresetObject> presets);

    void updateConsumerKey();
};

#endif // INTEGRATIONPLUGINBOSE_H
