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

#ifndef SONOS_H
#define SONOS_H

#include <QObject>

#include "network/networkaccessmanager.h"
#include "devices/device.h"

class Sonos : public QObject
{
    Q_OBJECT
public:

    enum RepeatMode {
        RepeatModeOne,
        RepeatModeAll,
        RepeatModeNone
    };

    enum PlayBackState {
        PlayBackStateBuffering,
        PlayBackStateIdle,
        PlayBackStatePause,
        PlayBackStatePlaying
    };

    struct PlayMode {
        bool repeat;
        bool repeatOne;
        bool shuffle;
        bool crossfade;
    };

    struct PlayerObject {

    };

    /* Represents a Sonos household.*/
    struct GroupObject {
        QString CoordinatorId;     //Player acting as the group coordinator for the group
        QString groupId;           //The ID of the group.
        QString playbackState;        //The playback state corresponding to the group.
        QList<QByteArray> playerIds;  //The IDs of the primary players in the group.
        QString displayName;          //The display name for the group, such as “Living Room” or “Kitchen + 2”.
    };

    struct GroupVolumeObject {
        int volume; //Group volume as an integer between 0 and 100, inclusive.
        bool muted; //A value indicating whether or not the group is muted
        bool fixed; //A value indicating whether or not the group volume is fixed or changeable.
    };
    /*
     * Represents a Sonos household.*/
    /*struct HouseholdObject {
        QString id;         //Identifies a Sonos household.
        QString name;       //A user-displayable name of the Sonos household
    };*/

    /*
     * The music service identifier or a pseudo-service identifier in the case of local library. */
    struct ServiceObject
    {
        QString id;
        QString name;
        QString imageUrl;
    };

    /*
    * Describes a Sonos favorite in the household.
    * You can see favorites in the My Sonos tab in the app. The following are not considered  */
    struct FavouriteObject {
        QString id;
        QString name;
        QString description;
        QString imageUrl;
    };

    struct PlaylistObject
    {
        QString id;
        QString name;
        QString type;
        QString trackCount;
    };

    struct PlayerSettingsObject
    {
        int volumeMode;
        float volumeScalingFactor;
        bool monoMode;
        bool wifiDisabled;
    };


    /* The music object identifier for the item in a music service.
     * This identifies the content within a music service, the music service,
     * and the account associated with the content. */
    struct MusicObjectId {
        QString serviceId;
        QString objectId;
        QString accountId;
    };

    struct TrackPoliciesObject {
        bool canCrossfade;
        bool canResume;
        bool canSeek;
        bool canSkip;
        bool canSkipBack;
        bool canSkipToItem;
        bool isVisible;
    };

    /* The artist of the track. */
    struct ArtistObject
    {
        QString name;
        QString imageUrl;
        MusicObjectId id;
        // tags enum
    };

    struct AlbumObject
    {
        QString name;
        ArtistObject artist;
    };

    struct ContainerObject
    {
        QString name;
        QString type;
        MusicObjectId id;
        ServiceObject service;
        QString imageUrl;
        //tags enum
    };

    struct PlayBackObject {
      QString itemId;
      bool isDucking;
      PlayBackState playbackState;
      PlayMode playMode;
      uint positionMillis;
      QString previousItemId;
      uint previousPositionMillis;
      QString queueVersion;
    };

    /*
     * A single music track or audio file. Tracks are identified by type,
     * which determines the key values for the object types included.
     * The following fields are shared by all types of tracks. */
    struct TrackObject
    {
        QString type;
        QString name;
        QString imageUrl;
        int trackNumber;
        bool canCrossfade;
        bool canSkip;
        int durationMillis;
        ArtistObject artist;
        AlbumObject album;
        ServiceObject service;
    };

    /* An item in a queue. Used for cloud queue tracks and radio stations that have track-like data
     * for the currently playing content. For example, the currentItem and nextItem parameters in the
     * metadataStatus event are item object types.*/
    struct ItemObject
    {
        QString itemId;
        TrackObject track;
        bool deleted;
        TrackPoliciesObject policies;
    };

    struct MetadataStatus {
        ContainerObject container;
        ItemObject currentItem;
        ItemObject nextItem;
    };

    explicit Sonos(NetworkAccessManager *networkManager, const QByteArray &accessToken, QObject *parent = nullptr);

    void setAccessToken(const QByteArray &accessToken);

    void getHouseholds();
    void getFavorites();
    void getGroups(const QString &householdId);

    QUuid cancelAudioClip();
    QUuid loadAudioClip();
    QUuid loadFavorite();

    QUuid createGroup(const QString &householdId, QList<QString> playerIds);
    QUuid modifyGroupMembers();
    QUuid setGroupMembers(const QString &groupId);

    //Group volume
    void getGroupVolume(const QString &groupId);                             //Get the volume and mute state of a group.
    //Group volume actions
    QUuid setGroupVolume(const QString &groupId, int volume);                //Set group volume to a specific level and unmute the group if muted.
    QUuid setGroupMute(const QString &groupId, bool mute);                   //Mute and unmute the group.
    QUuid setGroupRelativeVolume(const QString &groupId, int volumeDelta); 	//Increase or decrease group volume.

    //group playback
    void getGroupPlaybackStatus(const QString &groupId);

    //Group playback actions
    QUuid groupLoadLineIn(const QString &groupId);
    QUuid groupPlay(const QString &groupId);
    QUuid groupPause(const QString &groupId);
    QUuid groupSeek(const QString &groupId, int possitionMillis);
    QUuid groupSeekRelative(const QString &groupId, int deltaMillis);
    QUuid groupSetPlayModes(const QString &groupId, PlayMode playMode);
    QUuid groupSetShuffle(const QString &groupId, bool shuffle);
    QUuid groupSetRepeat(const QString &groupId, RepeatMode repeatMode);
    QUuid groupSetCrossfade(const QString &groupId, bool crossfade);
    QUuid groupSkipToNextTrack(const QString &groupId);
    QUuid groupSkipToPreviousTrack(const QString &groupId);
    QUuid groupTogglePlayPause(const QString &groupId);

    //playbackMetadata
    void getGroupMetadataStatus(const QString &groupId);

    // playerVolume
    void getPlayerVolume(const QByteArray &playerId);
    QUuid setPlayerVolume(const QByteArray &playerId, int volume);
    QUuid setPlayerRelativeVolume(const QByteArray &playerId, int volumeDelta);
    QUuid setPlayerMute(const QByteArray &playerId, bool mute);

    //Playlists API namespace
    void getPlaylist();
    void getPlaylists();
    QUuid loadPlaylist();

    //Settings
    void getPlayerSettings();
    QUuid setPlayerSettings();

private:
    QByteArray m_baseAuthorizationUrl = "https://api.sonos.com/login/v3/oauth";
    QByteArray m_baseControlUrl = "https://api.ws.sonos.com/control/api/v1";
    QByteArray m_apiKey = "b15cbf8c-a39c-47aa-bd93-635a96e9696c";
    QByteArray m_accessToken;

    NetworkAccessManager *m_networkManager = nullptr;
private slots:


signals:
    void connectionChanged(bool connected);
    void householdIdsReceived(QList<QString> householdIds);
    void groupsReceived(QList<GroupObject> groups);

    void playBackStatusReceived(const QString &groupId, PlayBackObject playBack);
    void metadataStatusReceived(const QString &groupId, MetadataStatus metaDataStatus);
    void volumeReceived(const QString &groupId, GroupVolumeObject groupVolume);

    void actionExecuted(QUuid actionId,bool success);

};

#endif // SONOS_H
