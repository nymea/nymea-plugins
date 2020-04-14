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

#ifndef SONOS_H
#define SONOS_H

#include <QObject>
#include <QTimer>

#include "network/networkaccessmanager.h"
#include "integrations/thing.h"

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

    /* Represents a Sonos household.*/
    struct GroupObject {
        QString CoordinatorId;     //Player acting as the group coordinator for the group
        QString groupId;           //The ID of the group.
        QString playbackState;        //The playback state corresponding to the group.
        QList<QByteArray> playerIds;  //The IDs of the primary players in the group.
        QString displayName;          //The display name for the group, such as “Living Room” or “Kitchen + 2”.
    };

    struct VolumeObject {
        int volume; //Group volume as an integer between 0 and 100, inclusive.
        bool muted; //A value indicating whether or not the group is muted
        bool fixed; //A value indicating whether or not the group volume is fixed or changeable.
    };

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
    struct FavoriteObject {
        QString id;
        QString name;
        QString description;
        QString imageUrl;
        ServiceObject service;
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
        QString volumeMode;
        double volumeScalingFactor;
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
    struct TrackObject {
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
    struct ItemObject {
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

    struct PlaylistTrackObject {
        QString name;
        QString artist;
        QString album;
    };

    struct PlaylistSummaryObject {
        QString id;
        QString name;
        QString type;
        QList<PlaylistTrackObject> tracks;
    };

    explicit Sonos(NetworkAccessManager *networkManager, const QByteArray &clientId,  const QByteArray &clientSecret, QObject *parent = nullptr);

    QUrl getLoginUrl(const QUrl &redirectUrl);
    QByteArray accessToken();
    QByteArray refreshToken();
    void getAccessTokenFromRefreshToken(const QByteArray &refreshToken);
    void getAccessTokenFromAuthorizationCode(const QByteArray &authorizationCode);

    void getHouseholds();
    QUuid getFavorites(const QString &householdId);
    void getGroups(const QString &householdId);

    QUuid loadFavorite(const QString &groupId, const QString &faveriteId);

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
    void getPlaylists(const QString &householdId);
    void getPlaylist(const QString &householdId, const QString &playlistId);
    QUuid loadPlaylist(const QString &groupId, const QString &playlistId);

    //Settings
    void getPlayerSettings(const QString &playerId);
    QUuid setPlayerSettings(const QString &playerId, PlayerSettingsObject settings);

private:
    QByteArray m_baseAuthorizationUrl = "https://api.sonos.com/login/v3/oauth/access";
    QByteArray m_baseControlUrl = "https://api.ws.sonos.com/control/api/v1";
    QByteArray m_clientKey;
    QByteArray m_clientSecret;

    QByteArray m_accessToken;
    QByteArray m_refreshToken;
    QByteArray m_redirectUri;

    NetworkAccessManager *m_networkManager = nullptr;
    QTimer *m_tokenRefreshTimer = nullptr;
private slots:
    void onRefreshTimeout();

signals:
    void connectionChanged(bool connected);
    void authenticationStatusChanged(bool authenticated);

    void householdIdsReceived(QList<QString> householdIds);
    void favoritesReceived(QUuid requestId, const QString &householdId, QList<FavoriteObject> favorites);
    void playlistsReceived(const QString &householdId, QList<PlaylistObject> playlists);
    void groupsReceived(const QString &householdId, QList<GroupObject> groups);
    void playlistSummaryReceived(const QString &householdId, PlaylistSummaryObject playlistSummary);

    void playBackStatusReceived(const QString &groupId, PlayBackObject playBack);
    void metadataStatusReceived(const QString &groupId, MetadataStatus metaDataStatus);
    void volumeReceived(const QString &groupId, VolumeObject groupVolume);

    void playerVolumeReceived(const QString &playerId, VolumeObject playerVolume);
    void playerSettingsRecieved(const QString &playerId, PlayerSettingsObject playerSettings);
    void actionExecuted(QUuid actionId, bool success);
};
#endif // SONOS_H
