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

    enum PlayMode {
      Repeat,
        RepeatOne,
        Shuffle,
        Crossfade
    };

    /*
     * Represents a Sonos household.*/
    struct HouseholdObject {
        QString id;         //Identifies a Sonos household.
        QString name;       //A user-displayable name of the Sonos household
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
    /*
     * A single music track or audio file. Tracks are identified by type,
     * which determines the key values for the object types included.
     * The following fields are shared by all types of tracks. */
    struct TrackObject
    {
        bool canCrossfade;
        bool canSkip;
        int durationMillis;
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
        //TODO
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

    explicit Sonos(NetworkAccessManager *networkManager, const QByteArray &accessToken, QObject *parent = nullptr);

    void setAccessToken(const QByteArray &accessToken);

    void getHouseholds();

    void cancelAudioClip();
    void loadAudioClip();

    void getFavorites();
    void loadFavorite();

    void getGroups();
    void createGroup();
    void modifyGroupMembers();
    void setGroupMembers();

    //group volume
    void getGroupVolume(int groupId);           //Get the volume and mute state of a group.
    void setGroupVolume(int groupId, int volume);           //Set group volume to a specific level and unmute the group if muted.
    void setGroupMute(int groupId, bool mute);    //Mute and unmute the group.
    void setGroupRelativeVolume(int groupId, int volumeDelta); 	//Increase or decrease group volume.

    //playback
    void getPlaybackStatus();
    void loadLineIn();
    void play();
    void pause();
    void seek();
    void seekRelative();
    void setPlayModes();
    void skipToNextTrack();
    void skipToPreviousTrack();
    void togglePlayPause();

    //playbackMetadata

    // playerVolume
    void getPlayerVolume(int playerId);
    void setPlayerVolume(int playerId, int volume);
    void setPlayerRelativeVolume(int playerId, int volumeDelta);
    void setPlayerMute(int playerId, bool mute);

    //Playlists API namespace
    void getPlaylist();
    void getPlaylists();
    void loadPlaylist();

    //Settings
    void getPlayerSettings();
    void setPlayerSettings();

private:
    QByteArray m_baseAuthorizationUrl = "api.sonos.com/login/v3/oauth";
    QByteArray m_baseControlUrl = "api.ws.sonos.com/control/api/v1";
    QByteArray m_accessToken;

    NetworkAccessManager *m_networkManager = nullptr;

private slots:


signals:

};

#endif // SONOS_H
