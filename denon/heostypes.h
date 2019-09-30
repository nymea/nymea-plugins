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

#ifndef HEOSTYPES_H
#define HEOSTYPES_H

#include "extern-plugininfo.h"

enum NETWORK_TYPE {
    NETWORK_TYPE_WIRED,
    NETWORK_TYPE_WIFI
} ;

enum LINEOUT_LEVEL_TYPE {
    LINEOUT_LEVEL_TYPE_VARIABLE = 1,
    LINEOUT_LEVEL_TYP_FIXED = 2
};

enum CONTROL_TYPE {
    CONTROL_TYPE_NONE    = 1,
    CONTROL_TYPE_IR      = 2,
    CONTROL_TYPE_TRIGGER = 3,
    CONTROL_TYPE_NETWORK = 4
};

enum PLAYER_STATE {
    PLAYER_STATE_PLAY,
    PLAYER_STATE_PAUSE,
    PLAYER_STATE_STOP
};

enum NOW_PLAYING_OPTIONS {
    NOW_PLAYING_OPTION_THUMBS_UP = 11,
    NOW_PLAYING_OPTION_THUMBS_DOWN = 12,
    NOW_PLAYING_OPTION_ADD_STATION_TO_HEOS_FAVOURITES = 19
};

enum REPEAT_MODE {
    REPEAT_MODE_OFF,
    REPEAT_MODE_ONE,
    REPEAT_MODE_ALL
};

enum PLAYER_ROLE {
    PLAYER_ROLE_LEADER,
    PLAYER_ROLE_MEMBER
};

enum BROWSE_OPTION {
    BROWSE_OPTION_ADD_TRACK_TO_LIBRARY = 1,
    BROWSE_OPTION_ADD_ALBUM_TO_LIBRARY = 2,
    BROWSE_OPTION_ADD_STATION_TO_LIBRARY = 3,
    BROWSE_OPTION_ADD_PLAYLIST_TO_LIBRARY = 4,
    BROWSE_OPTION_REMOVE_TRACK_FROM_LIBRARY = 5,
    BROWSE_OPTION_REMOVE_ALBUM_FROM_LIBRARY = 6,
    BROWSE_OPTION_REMOVE_STATION_FROM_LIBRARY = 7,
    BROWSE_OPTION_REMOVE_PLAYLIST_FROM_LIBRARY = 8,
    BROWSE_OPTION_CREATE_NEW_STATION = 13,
    BROWSE_OPTION_ADD_HEOS_FAVORITES = 19
};

enum SEARCH_CRITERIA {      // criteria id returned by 'get_search_criteria' command
    SEARCH_CRITERIA_ARTIST,
    SEARCH_CRITERIA_ALBUM,
    SEARCH_CRITERIA_SONG,
    SEARCH_CRITERIA_STATION
};

enum MEDIA_TYPE {
    MEDIA_TYPE_SONG,
    MEDIA_TYPE_STATION,
    MEDIA_TYPE_GENRE,
    MEDIA_TYPE_ARTIST,
    MEDIA_TYPE_ALBUM,
    MEDIA_TYPE_CONTAINER
};

enum SOURCE_ID {
    SOURCE_ID_PANDORA = 1,
    SOURCE_ID_RHAPSODY,
    SOURCE_ID_TUNEIN,
    SOURCE_ID_SPOTIFY,
    SOURCE_ID_DEEZER,
    SOURCE_ID_NAPSTER,
    SOURCE_ID_IHEARTRADIO,
    SOURCE_ID_SIRIUS_XM,
    SOURCE_ID_SOUNDCLOUD,
    SOURCE_ID_TIDAL,
    SOURCE_ID_FUTURE_SERVICE_1,
    SOURCE_ID_RDIO,
    SOURCE_ID_AMAZON_MUSIC,
    SOURCE_ID_FUTURE_SERVICE_2,
    SOURCE_ID_MOODMIX,
    SOURCE_ID_JUKE,
    SOURCE_ID_FUTURE_SERVICE_3,
    SOURCE_ID_QQMUSIC = 18,
    SOURCE_ID_LOCAL_MEDIA = 1024,
    SOURCE_ID_HEOS_PLAYLIST = 1025,
    SOURCE_ID_HEOS_HISTORY = 1026,
    SOURCE_ID_HEOS_AUX = 1027,
    SOURCE_ID_HEOS_FAVORITES = 1028
};

struct SearchObject {
    int sourceId;                   //Source id returned by 'get_music_sources' command
    QString searchString;           //String for search limited to 128 unicode characters and may contain '*' for wildcard if supported by search criteria id
    SEARCH_CRITERIA searchCriteria; //Search criteria id returned by 'get_search_criteria' command
    int count;                      //Total number of items available in the container. NOTE: count value of '0' indicates unknown container size. Controllers needs to query until the return payload is empty (returned attribute is 0).
    int range;                      //Range is start and end record index to return. Range parameter is optional. Omitting range parameter returns all records up to a maximum of 50/100 records per response. The default maximum number of records depend on the service type.
    int returned;                   //Number of items returned in current response
};

struct MusicSourceObject {
    QString name;
    QString image_url;
    QString type;
    int sourceId;
    bool available;
    QString serviceUsername;
};

struct PlayerObject {
    QString name;
    int playerId;
    PLAYER_ROLE role;
};

struct GroupObject {
    QString name;
    int groupId;
    QList<PLAYER_ROLE> role;
};

struct SourceContainersObject {
    int sourceId;
    int containerId;
    int range;
    int count;
};

struct MediaObject {
    MEDIA_TYPE mediaType;
    bool isContainer;
    bool isPlayable;
    QString name;
    QString imageUrl;
    QString containerId;
    QString mediaId;
};

struct heosPlayer {

};

struct heosGroup {

};

#endif // HEOSTYPES_H
