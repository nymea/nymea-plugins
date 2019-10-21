/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *                                                                         *
 *  Copyright (C) 2019 Bernhard Trinnes <bernhard.trinnes@nymea.io         *
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

#ifndef DEVICEPLUGINSONOS_H
#define DEVICEPLUGINSONOS_H

#include "devices/deviceplugin.h"
#include "plugintimer.h"
#include "sonos.h"

#include <QHash>
#include <QDebug>

class DevicePluginSonos : public DevicePlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "io.nymea.DevicePlugin" FILE "devicepluginsonos.json")
    Q_INTERFACES(DevicePlugin)

public:
    explicit DevicePluginSonos();
    ~DevicePluginSonos() override;

    void setupDevice(DeviceSetupInfo *info) override;
    void startPairing(DevicePairingInfo *info) override;
    void confirmPairing(DevicePairingInfo *info, const QString &username, const QString &secret) override;

    void postSetupDevice(Device *device) override;
    void startMonitoringAutoDevices() override;
    void deviceRemoved(Device *device) override;
    void executeAction(DeviceActionInfo *info) override;

    void browseDevice(BrowseResult *result) override;
    void browserItem(BrowserItemResult *result) override;
    void executeBrowserItem(BrowserActionInfo *info) override;

private:
    PluginTimer *m_pluginTimer5sec = nullptr;
    PluginTimer *m_pluginTimer60sec = nullptr;

    QHash<DeviceId, Sonos *> m_setupSonosConnections;
    QHash<Device *, Sonos *> m_sonosConnections;
    QList<QByteArray> m_householdIds;

    QByteArray m_sonosConnectionAccessToken;
    QByteArray m_sonosConnectionRefreshToken;

    QHash<QUuid, QPointer<DeviceActionInfo> > m_pendingActions;
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

#endif // DEVICEPLUGINSONOS_H
