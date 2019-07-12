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
#include "heostypes.h"

class Heos : public QObject
{
    Q_OBJECT
public:

    explicit Heos(const QHostAddress &hostAddress, QObject *parent = nullptr);
    ~Heos();

    void connectHeos();
    void setAddress(QHostAddress address);
    QHostAddress getAddress();
    HeosPlayer *getPlayer(int playerId);

    // Heos system commands
    void registerForChangeEvents(bool state);   //By default HEOS speaker does not send Change events. Controller needs to send this command with enable=on when it is ready to receive unsolicit responses from CLI. Please refer to "Driver Initialization" section regarding when to register for change events.
    void sendHeartbeat();
    void getUserAccount();                      //returns current user name in its message field if the user is currently singed in.
    void setUserAccount(QString userName, QString password);
    void logoutUserAccount();
    void rebootSpeaker();                       //Using this command controllers can reboot HEOS device. This command can only be used to reboot the HEOS device to which the controller is connected through CLI port.
    void prettifyJsonResponse(bool enable);     //Helper command to prettify JSON response when user is running CLI controller through telnet.

    //Player Get Calls
    void getPlayers();                          //get a list of players associated with this heos master
    void getPlayerState(int playerId);
    void getVolume(int playerId);
    void getNowPlayingMedia(int playerId);
    void getMute(int playerId);
    void getPlayMode(int playerId);
    void getQueue(int playerId);

    //Group Get Calls
    void getGroups();
    void getGroupInfo(int groupId);
    void getGroupVolume(int groupId);
    void getGroupMute(int groupId);

    //Player Set Calls
    void setPlayerState(int playerId, PLAYER_STATE state);
    void setVolume(int playerId, int volume); //Player volume level 0 to 100
    void setMute(int playerId, bool mute);
    void setPlayMode(int playerId, REPEAT_MODE repeatMode, bool shuffle); //shuffle and repead mode
    void playNext(int playerId);
    void playPrevious(int playerId);
    void volumeUp(int playerId, int step = 5); //steps 0-10
    void volumeDown(int playerId, int step = 5); //steps 0-10
    void clearQueue(int playerId);
    void moveQueue(int playerId, int sourcQueueId, int destinationQueueId);
    void checkForFirmwareUpdate(int playerId);

    //Group Set Calls
    void setGroupVolume(int groupId, bool volume);
    void setGroupMute(int groupId, bool mute);
    void toggleGroupMute(int groupId);
    void groupVolumeUp(int groupId, int step = 5);
    void groupVolumeDown(int groupId, int step = 5);

    //Browse Get Commands
    void getMusicSources();
    void getSourceInfo(SOURCE_ID sourceId);
    void getSearchCriteria(SOURCE_ID sourceId);
    void browseSource(SOURCE_ID sourceId);
    //void search();


private:
    bool m_eventRegistered = false;
    QHostAddress m_hostAddress;
    QTcpSocket *m_socket = nullptr;
    QHash<int, HeosPlayer*> m_heosPlayers;
    void setConnected(const bool &connected);

signals:
    void playerDiscovered(HeosPlayer *heosPlayer);
    void connectionStatusChanged(bool status);

    void playStateReceived(int playerId, PLAYER_STATE state);
    void shuffleModeReceived(int playerId, bool shuffle);
    void repeatModeReceived(int playerId, REPEAT_MODE repeatMode);
    void muteStatusReceived(int playerId, bool mute);
    void volumeStatusReceived(int playerId, int volume);
    void nowPlayingMediaStatusReceived(int playerId, SOURCE_ID source, QString artist, QString album, QString Song, QString artwork);

private slots:
    void onConnected();
    void onDisconnected();
    void onError(QAbstractSocket::SocketError socketError);
    void readData();
};


#endif // HEOS_H
