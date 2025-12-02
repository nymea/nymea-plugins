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

#ifndef INTEGRATIONPLUGINDENON_H
#define INTEGRATIONPLUGINDENON_H

#include "heos.h"
#include "avrconnection.h"
#include "plugintimer.h"
#include "integrations/integrationplugin.h"
#include "network/zeroconf/zeroconfservicebrowser.h"
#include "network/zeroconf/zeroconfserviceentry.h"

#include <QProcess>
#include <QPair>
#include <QHash>
#include <QObject>
#include <QPointer>
#include <QHostAddress>
#include <QNetworkReply>
#include <QUuid>

class IntegrationPluginDenon : public IntegrationPlugin
{
    Q_OBJECT

    Q_PLUGIN_METADATA(IID "io.nymea.IntegrationPlugin" FILE "integrationplugindenon.json")
    Q_INTERFACES(IntegrationPlugin)

public:
    explicit IntegrationPluginDenon();
    void init() override;

    void discoverThings(ThingDiscoveryInfo *info) override;

    void startPairing(ThingPairingInfo *info) override;
    void confirmPairing(ThingPairingInfo *info, const QString &username, const QString &secret) override;

    void setupThing(ThingSetupInfo *info) override;
    void postSetupThing(Thing *thing) override;
    void executeAction(ThingActionInfo *info) override;
    void thingRemoved(Thing *thing) override;

    void browseThing(BrowseResult *result) override;
    void browserItem(BrowserItemResult *result) override;
    void executeBrowserItem(BrowserActionInfo *info) override;
    void executeBrowserItemAction(BrowserItemActionInfo *info) override;

private:
    PluginTimer *m_pluginTimer = nullptr;
    ZeroConfServiceBrowser *m_serviceBrowser = nullptr;

    QHash<ThingId, AvrConnection*> m_avrConnections;
    QHash<ThingId, Heos*> m_heosConnections;
    QHash<ThingId, Heos*> m_unfinishedHeosConnections;
    QHash<Heos *, ThingPairingInfo *> m_unfinishedHeosPairings;

    QHash<AvrConnection*, ThingSetupInfo*> m_asyncAvrSetups;
    QHash<Heos*, ThingSetupInfo*> m_asyncHeosSetups;

    QHash<int, Thing *> m_playerIds;
    QHash<int, Thing *> m_discoveredPlayerIds;
    QHash<const Action *, int> m_asyncActions;
    QUrl m_notificationUrl;

    QHash<int, ThingActionInfo*> m_heosPendingActions;
    QHash<QUuid, ThingActionInfo*> m_avrPendingActions;

    QHash<Heos*, BrowseResult*> m_pendingGetSourcesRequest;
    QHash<QString, BrowseResult*> m_pendingBrowseResult;    // QString = containerId or sourceId
    QHash<int, BrowserActionInfo*> m_pendingBrowserActions;
    QHash<int, BrowserItemActionInfo*> m_pendingBrowserItemActions;
    QHash<QString, MediaObject> m_mediaObjects;

    QHash<int, GroupObject> m_groupBuffer;
    QHash<int, HeosPlayer *> m_playerBuffer;

    QHash<QString, QHostAddress> m_heosIpAddresses;

    Heos *createHeosConnection(const QHostAddress &address);
    QHostAddress findAvrById(const QString &id);

private slots:
    void onPluginTimer();

    void onHeosConnectionChanged(bool status);
    void onHeosPlayersChanged();
    void onHeosPlayersReceived(QList<HeosPlayer *> players);
    void onHeosPlayerInfoRecieved(HeosPlayer *player);
    void onHeosPlayStateReceived(int playerId, PLAYER_STATE state);
    void onHeosShuffleModeReceived(int playerId, bool shuffle);
    void onHeosRepeatModeReceived(int playerId, REPEAT_MODE repeatMode);
    void onHeosMuteStatusReceived(int playerId, bool mute);
    void onHeosVolumeStatusReceived(int playerId, int volume);
    void onHeosNowPlayingMediaStatusReceived(int playerId, const QString &sourceId, const QString &artist, const QString &album, const QString &song, const QString &artwork);
    void onHeosMusicSourcesReceived(quint32 sequenceNumber, QList<MusicSourceObject> musicSources);

    void onHeosBrowseRequestReceived(quint32 sequenceNumber, const QString &sourceId, const QString &containerId, QList<MusicSourceObject> musicSources, QList<MediaObject> mediaItems);
    void onHeosBrowseErrorReceived(const QString &sourceId, const QString &containerId, int errorId, const QString &errorMessage);
    void onHeosPlayerNowPlayingChanged(int playerId);
    void onHeosPlayerQueueChanged(int playerId);
    void onHeosGroupsReceived(QList<GroupObject> groups);
    void onHeosGroupsChanged();
    void onHeosUserChanged(bool signedIn, const QString &userName);
    void onHeosDiscoveryFinished();

    void onAvrConnectionChanged(bool status);
    void onAvrSocketError();
    void onAvrCommandExecuted(const QUuid &commandId, bool success);

    void onAvrVolumeChanged(int volume);
    void onAvrChannelChanged(const QString &channel);
    void onAvrMuteChanged(bool mute);
    void onAvrPowerChanged(bool power);
    void onAvrSurroundModeChanged(const QString &surroundMode);
    void onAvrSongChanged(const QString &song);
    void onAvrArtistChanged(const QString &artist);
    void onAvrAlbumChanged(const QString &album);
    void onAvrPlayBackModeChanged(AvrConnection::PlayBackMode mode);
    void onAvrBassLevelChanged(int level);
    void onAvrTrebleLevelChanged(int level);
    void onAvrToneControlEnabledChanged(bool enabled);

    void onPluginConfigurationChanged(const ParamTypeId &paramTypeId, const QVariant &value);
};

#endif // INTEGRATIONPLUGINDENON_H
