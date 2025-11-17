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

#ifndef INTEGRATIONPLUGINSONOS_H
#define INTEGRATIONPLUGINSONOS_H

#include <integrations/integrationplugin.h>

#include <QHash>
#include <QDebug>

#include "sonos.h"

class PluginTimer;
class BrowseResult;
class BrowseResult;
class BrowserItemResult;

class IntegrationPluginSonos : public IntegrationPlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "io.nymea.IntegrationPlugin" FILE "integrationpluginsonos.json")
    Q_INTERFACES(IntegrationPlugin)

public:
    explicit IntegrationPluginSonos();
    ~IntegrationPluginSonos() override;

    void setupThing(ThingSetupInfo *info) override;
    void startPairing(ThingPairingInfo *info) override;
    void confirmPairing(ThingPairingInfo *info, const QString &username, const QString &secret) override;

    void postSetupThing(Thing *thing) override;
    void startMonitoringAutoThings() override;
    void thingRemoved(Thing *thing) override;
    void executeAction(ThingActionInfo *info) override;

    void browseThing(BrowseResult *result) override;
    void browserItem(BrowserItemResult *result) override;
    void executeBrowserItem(BrowserActionInfo *info) override;

private:
    PluginTimer *m_pluginTimer5sec = nullptr;
    PluginTimer *m_pluginTimer60sec = nullptr;

    QHash<ThingId, Sonos *> m_setupSonosConnections;
    QHash<Thing *, Sonos *> m_sonosConnections;
    QList<QByteArray> m_householdIds;

    QByteArray m_sonosConnectionAccessToken;
    QByteArray m_sonosConnectionRefreshToken;

    QHash<QUuid, QPointer<ThingActionInfo> > m_pendingActions;
    QHash<QUuid, BrowseResult *> m_pendingBrowseResult;
    QHash<QUuid, BrowserItemResult *> m_pendingBrowserItemResult;
    QHash<QUuid, BrowserActionInfo  *> m_pendingBrowserExecution;

    const QString m_browseFavoritesPrefix = "/favorites";

private slots:
    void onConnectionChanged(bool connected);
    void onAuthenticationStatusChanged(bool authenticated);

    void onHouseholdIdsReceived(QList<QString> householdIds);
    void onFavoritesReceived(QUuid requestId, const QString &householdId, QList<Sonos::FavoriteObject> favorites);
    void onPlaylistsReceived(const QString &householdId, QList<Sonos::PlaylistObject> playlists);
    void onPlaylistSummaryReceived(const QString &householdId, Sonos::PlaylistSummaryObject playlistSummary);

    void onGroupsReceived(const QString &householdId, QList<Sonos::GroupObject> groupIds);
    void onPlayBackStatusReceived(const QString &groupId, Sonos::PlayBackObject playBack);
    void onMetadataStatusReceived(const QString &groupId, Sonos::MetadataStatus metaDataStatus);
    void onVolumeReceived(const QString &groupId, Sonos::VolumeObject groupVolume);
    void onActionExecuted(QUuid actionId, bool success);
};

#endif // INTEGRATIONPLUGINSONOS_H
