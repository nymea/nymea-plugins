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

#ifndef HEOS_H
#define HEOS_H

#include <QObject>
#include <QHostAddress>
#include <QTcpSocket>

#include "heosplayer.h"

class Heos : public QObject
{
    Q_OBJECT
public:
    enum HeosPlayerState {
        Play = 0,
        Pause = 1,
        Stop = 2
    };

    enum HeosRepeatMode {
        Off = 0,
        One = 1,
        All = 2
    };

    explicit Heos(const QHostAddress &hostAddress, QObject *parent = nullptr);
    ~Heos();

    void setAddress(QHostAddress address);
    QHostAddress getAddress();
    bool connected();
    void connectHeos();

    void getPlayers();
    HeosPlayer *getPlayer(int playerId);
    void getPlayerState(int playerId);
    void setPlayerState(int playerId, HeosPlayerState state);
    void getVolume(int playerId);
    void setVolume(int playerId, int volume);
    void getMute(int playerId);
    void setMute(int playerId, bool state);
    void setPlayMode(int playerId, HeosRepeatMode repeatMode, bool shuffle); //shuffle and repead mode
    void getPlayMode(int playerId);
    void playNext(int playerId);
    void playPrevious(int playerId);
    void getNowPlayingMedia(int playerId);
    void registerForChangeEvents(bool state);
    void sendHeartbeat();

private:
    bool m_connected = false;
    bool m_eventRegistered = false;
    QHostAddress m_hostAddress;
    QTcpSocket *m_socket = nullptr;
    QHash<int, HeosPlayer*> m_heosPlayers;
    void setConnected(const bool &connected);

signals:
    void playerDiscovered(HeosPlayer *heosPlayer);
    void connectionStatusChanged();

    void playStateReceived(int playerId, HeosPlayerState state);
    void shuffleModeReceived(int playerId, bool shuffle);
    void repeatModeReceived(int playerId, HeosRepeatMode repeatMode);
    void muteStatusReceived(int playerId, bool mute);
    void volumeStatusReceived(int playerId, int volume);
    void nowPlayingMediaStatusReceived(int playerId, QString source, QString artist, QString album, QString Song, QString artwork);

private slots:
    void onConnected();
    void onDisconnected();
    void onError(QAbstractSocket::SocketError socketError);
    void readData();
};


#endif // HEOS_H
