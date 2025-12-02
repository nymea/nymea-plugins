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

#ifndef BLUOS_H
#define BLUOS_H

#include <QUuid>
#include <QTimer>
#include <QObject>
#include <QHostAddress>

#include <integrations/thing.h>
#include <network/networkaccessmanager.h>

class BluOS : public QObject
{
    Q_OBJECT
public:

    enum PlaybackCommand {
        Play,
        Pause,
        Stop,
        Skip,
        Back
    };

    enum RepeatMode {
        All,
        One,
        None
    };

    enum PlaybackState {
        Playing,
        Paused,
        Stopped,
        Connecting,
        Streaming
    };

    struct StatusResponse {
        QString Album;
        QString Artist;
        QString Name;
        QString Title;
        QString Service;
        QUrl ServiceIcon;
        PlaybackState State;
        QUrl StationUrl;
        int Volume;
        bool Mute;
        RepeatMode Repeat;
        bool Shuffle;
        QUrl Image;
        QString Group;
    };

    struct Preset {
        QString Name;
        int Id;
        QString Url;
    };

    struct Source {
        //<item image="/images/ci_myplaylists.png" browseKey="playlists" text="Playlists" type="link"/>
        QString Image;
        QString BrowseKey;
        QString Text;
        QString Type;
    };

    explicit BluOS(NetworkAccessManager *networkManager, QHostAddress hostAddress, int port, QObject *parent = nullptr);

    int port();
    QHostAddress hostAddress();
    
    // Status Queries
    void getStatus();
    
    // Volume Control
    QUuid setVolume(uint volume);
    QUuid setMute(bool mute);

    // Playback Control
    QUuid play();
    QUuid pause();
    QUuid stop();
    QUuid skip();
    QUuid back();
    QUuid setShuffle(bool shuffle);
    QUuid setRepeat(RepeatMode repeatMode);
    
    // Presets
    QUuid listPresets();
    QUuid loadPreset(int preset); //1 for next preset, -1 for previous preset
    
    // Content Browsing
    QUuid getSources();
    QUuid browseSource(const QString &key);
    
    // Player Grouping
    QUuid addGroupPlayer(QHostAddress address, int port); //adds player as slave
    QUuid removeGroupPlayer(QHostAddress address, int port);
    
private:
    NetworkAccessManager *m_networkManager = nullptr;
    QHostAddress m_hostAddress;
    int m_port;

    QUuid playBackControl(PlaybackCommand command);
    bool parseState(const QByteArray &state);

signals:
    void connectionChanged(bool connected);
    void actionExecuted(const QUuid &actionId, bool success);

    void statusReceived(const BluOS::StatusResponse &status);
    void volumeReceived(int volume, bool mute);
    void shuffleStateReceived(bool state);
    void repeatModeReceived(BluOS::RepeatMode mode);

    void presetsReceived(const QUuid &requestId, const QList<BluOS::Preset> &presets);
    void sourcesReceived(const QUuid &requestId, const QList<BluOS::Source> &sources);
    void browseResultReceived(const QUuid &requestId, const QList<BluOS::Source> &sources);
};
#endif // BLUOS_H
