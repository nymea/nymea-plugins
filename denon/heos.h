/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*
* Copyright 2013 - 2020, nymea GmbH
* Contact: contact@nymea.io
*
* This file is part of nymea.
* This project including source code and documentation is protected by
* copyright law, and remains the property of nymea GmbH. All rights, including
* reproduction, publication, editing and translation, are reserved. The use of
* this project is subject to the terms of a license agreement to be concluded
* with nymea GmbH in accordance with the terms of use of nymea GmbH, available
* under https://nymea.io/license
*
* GNU Lesser General Public License Usage
* Alternatively, this project may be redistributed and/or modified under the
* terms of the GNU Lesser General Public License as published by the Free
* Software Foundation; version 3. This project is distributed in the hope that
* it will be useful, but WITHOUT ANY WARRANTY; without even the implied
* warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
* Lesser General Public License for more details.
*
* You should have received a copy of the GNU Lesser General Public License
* along with this project. If not, see <https://www.gnu.org/licenses/>.
*
* For any further details and any questions please contact us under
* contact@nymea.io or see our FAQ/Licensing Information on
* https://nymea.io/license/faq
*
* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

#ifndef HEOS_H
#define HEOS_H

#include <QObject>
#include <QHostAddress>
#include <QTcpSocket>

#include "heosplayer.h"
#include "heostypes.h"

#include "integrations/thing.h"
#include "types/mediabrowseritem.h"
#include "integrations/browseresult.h"
#include "integrations/browseritemresult.h"

class Heos : public QObject
{
    Q_OBJECT
public:

    explicit Heos(const QHostAddress &hostAddress, QObject *parent = nullptr);
    ~Heos();

    void connectDevice();
    void disconnectDevice();
    bool connected();

    void setAddress(QHostAddress address);
    QHostAddress getAddress();

    // Heos system commands
    void registerForChangeEvents(bool state);   //By default HEOS speaker does not send Change events. Controller needs to send this command with enable=on when it is ready to receive unsolicit responses from CLI. Please refer to "Driver Initialization" section regarding when to register for change events.
    void sendHeartbeat();
    void getUserAccount();                      //returns current user name in its message field if the user is currently singed in.
    void setUserAccount(QString userName, QString password);
    void logoutUserAccount();
    void rebootSpeaker();                       //Using this command controllers can reboot HEOS thing. This command can only be used to reboot the HEOS thing to which the controller is connected through CLI port.
    void prettifyJsonResponse(bool enable);     //Helper command to prettify JSON response when user is running CLI controller through telnet.

    //Player Get Calls
    void getPlayers();                          //get a list of players associated with this heos master
    void getPlayerInfo(int playerId);
    void getPlayerState(int playerId);
    void getVolume(int playerId);
    void getNowPlayingMedia(int playerId);
    void getMute(int playerId);
    void getPlayMode(int playerId);
    void getQueue(int playerId);

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

    //Group Get Calls
    void getGroups();
    void getGroupInfo(int groupId);
    void getGroupVolume(int groupId);
    void getGroupMute(int groupId);

    //Group Set Calls
    void setGroupVolume(int groupId, bool volume);
    void setGroupMute(int groupId, bool mute);
    void setGroup(QList<int> playerIds);
    void deleteGroup(int leadPlayerId);
    void toggleGroupMute(int groupId);
    void groupVolumeUp(int groupId, int step = 5);
    void groupVolumeDown(int groupId, int step = 5);

    //Browse Get Commands
    quint32 getMusicSources();
    quint32 getSourceInfo(const QString &sourceId);
    quint32 getSearchCriteria(const QString &sourceId);
    quint32 browseSource(const QString &sourceId);
    quint32 browseSourceContainers(const QString &sourceId, const QString &containerId);
    quint32 addContainerToQueue(int playerId, const QString &sourceId, const QString &containerId, ADD_CRITERIA addCriteria);
    // Controllers can add custom argument SEQUENCE=<number> in browse commands to associate command and response.

    //Play commands
    quint32 playStation(int playerId, const QString &sourceId, const QString &containerId, const QString &mediaId, const QString &stationName);
    quint32 playPresetStation(int playerId, int presetNumber);
    quint32 playInputSource(int playerId, const QString &inputName); //Validity of Inputs depends on the type of source HEOS devie
    quint32 playUrl(int playerId, const QUrl &url);

private:
    bool m_eventRegistered = false;
    QHostAddress m_hostAddress;
    QTcpSocket *m_socket = nullptr;
    void setConnected(const bool &connected);

signals:
    void connectionStatusChanged(bool status);
    void systemEventsEnabled(bool status);

    void playersChanged();
    void playersRecieved(QList<HeosPlayer *> heosPlayers);
    void playerInfoRecieved(HeosPlayer *heosPlayers);
    void playerQueueChanged(int playerId);
    void playerPlayStateReceived(int playerId, PLAYER_STATE state);
    void playerShuffleModeReceived(int playerId, bool shuffle);
    void playerRepeatModeReceived(int playerId, REPEAT_MODE repeatMode);
    void playerMuteStatusReceived(int playerId, bool mute);
    void playerVolumeReceived(int playerId, int volume);
    void playerUpdateAvailable(int playerId, bool exist); // Callback of Check for Firmware Update
    void playerPlaybackErrorReceived(int playerId, const QString &message); //Error string represents error type. Controller can directly display the error string to the user.
    void playerNowPlayingProgressReceived(int playerId, int currentPosition, int duration);
    void playerNowPlayingChanged(int playerId);

    void groupsReceived(QList<GroupObject> groups);      // Callback of getGroups()
    void groupInfoReceived(GroupObject group);
    void groupVolumeReceived(int groupId, int volume);
    void groupMuteStatusReceived(int groupId, bool mute);
    void groupsChanged();

    void setGroupReceived(int groupId, const QString &groupName);   // Callback of setGroup()
    void deleteGroupReceived(int leadPlayerId);                     // Callback of deleteGroup();

    void sourcesChanged();
    void nowPlayingMediaStatusReceived(int playerId, const QString &sourceId, const QString &artist, const QString &album, const QString &song, const QString &artwork);

    void musicSourcesReceived(quint32 sequenceNumber, QList<MusicSourceObject> musicSources); //callback of getMusicSource, not associated to a playerId
    void browseRequestReceived(quint32 sequenceNumber, const QString &sourceId, const QString &containerId, QList<MusicSourceObject> musicSources, QList<MediaObject> mediaItems); //callback of browseSource
    void browseErrorReceived(const QString &sourceId, const QString &containerId, int errorId, const QString &errorMessage);
    void userChanged(bool signedIn, const QString &userName);

private slots:
    void onConnected();
    void onDisconnected();
    void onError(QAbstractSocket::SocketError socketError);
    void readData();
};


#endif // HEOS_H
