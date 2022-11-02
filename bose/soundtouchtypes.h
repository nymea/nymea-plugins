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

#ifndef SOUNDTOUCHTYPES_H
#define SOUNDTOUCHTYPES_H

#include "extern-plugininfo.h"

enum SHUFFLE_STATUS {
    SHUFFLE_STATUS_SHUFFLE_OFF,
    SHUFFLE_STATUS_SHUFFLE_ON
};

enum REPEAT_STATUS { 	//The state of repeat.
    REPEAT_STATUS_REPEAT_OFF,
    REPEAT_STATUS_REPEAT_ALL,
    REPEAT_STATUS_REPEAT_ONE
};

enum STREAM_STATUS {     //The type of music container that is streaming.
    STREAM_STATUS_TRACK_ONDEMAND,
    STREAM_STATUS_RADIO_STREAMING,
    STREAM_STATUS_RADIO_TRACKS,
    STREAM_STATUS_NO_TRANSPORT_CONTROLS
};

enum SOURCE_STATUS {	//The availability of an audio source
    SOURCE_STATUS_UNAVAILABLE,
    SOURCE_STATUS_READY
};

enum PLAY_STATUS {	//The state of the audio stream
    PLAY_STATUS_PLAY_STATE,
    PLAY_STATUS_PAUSE_STATE,
    PLAY_STATUS_STOP_STATE,
    PLAY_STATUS_BUFFERING_STATE
};

enum ART_STATUS {	//The state of an image
    ART_STATUS_INVALID,
    ART_STATUS_SHOW_DEFAULT_IMAGE,
    ART_STATUS_DOWNLOADING,
    ART_STATUS_IMAGE_PRESENT
};

enum KEY_VALUE {	//An enumeration of virtual thing buttons that may be pressed
    KEY_VALUE_PLAY,
    KEY_VALUE_PAUSE,
    KEY_VALUE_PLAY_PAUSE,
    KEY_VALUE_STOP,
    KEY_VALUE_PREV_TRACK,
    KEY_VALUE_NEXT_TRACK,
    KEY_VALUE_POWER,
    KEY_VALUE_MUTE,
    KEY_VALUE_AUX_INPUT,
    KEY_VALUE_SHUFFLE_ON,
    KEY_VALUE_SHUFFLE_OFF,
    KEY_VALUE_REPEAT_ONE,
    KEY_VALUE_REPEAT_ALL,
    KEY_VALUE_REPEAT_OFF,
    KEY_VALUE_ADD_FAVORITE,
    KEY_VALUE_REMOVE_FAVORITE,
    KEY_VALUE_THUMBS_UP,
    KEY_VALUE_THUMBS_DOWN,
    KEY_VALUE_BOOKMARK,
    KEY_VALUE_PRESET_1,
    KEY_VALUE_PRESET_2,
    KEY_VALUE_PRESET_3,
    KEY_VALUE_PRESET_4,
    KEY_VALUE_PRESET_5,
    KEY_VALUE_PRESET_6
};

enum KEY_STATE {	//The state of the virtual key press
    KEY_STATE_PRESS,
    KEY_STATE_RELEASE
};

enum GROUP_LOC {
    GROUP_LOC_LEFT,
    GROUP_LOC_RIGHT
};

struct ComponentObject {
    QString softwareVersion;            //Element. The firmware version of the component.
    QString serialNumber;               //Element. The serial number of the component.
};

struct NetworkInfoObject {
    QString macAddress;                 //Element. The MAC Address of the component.
    QString ipAddress;                  //Element. The IP Address of the product.
};

struct InfoObject {
    QString deviceID;                   //Attribute. Unique identifier of the product.
    QString name;                       //Element. The user-set product name. You can change this with /name.
    QString type;                       //Element. The Bose-defined name for the product. Ex: SoundTouch 10.
    QList<ComponentObject> components; 	//Element. A container for all component objects. See component object.
    NetworkInfoObject networkInfo;      //Element. This object describes the product's current connection information. See networkInfo object.
};

struct ArtObject {
    ART_STATUS artStatus = ART_STATUS_INVALID;
    QString url;
};

struct TimeObject {
    int total;                  //Attribute. Tells you the current time the track has played, in seconds.
    int length;                 //Content. The length of the track, in seconds.
};
struct ContentItemObject {
    QString source;             //Attribute. The type or name of the service playing. To determine if the product is in standby mode, check if source == STANDBY.
    QString location;           //Attribute. READ-ONLY. This attribute is used by Bose to point to the content the user is accessing. You can save it to access content later, but do not attempt to generate your own locations.
    QString sourceAccount;      //Attribute. The user-account associated with this source.
    bool isPresetable = false;  //Attribute. TRUE if the source can be set as one of the six (6) SoundTouch presets.
    QString itemName;           //Element. The album, station, playlist, song, phone, etc. name depending on the source.
    QString containerArt;       //URL
};

struct ConnectionStatusInfoObject {
    QString status; 	//Attribute. The connection status.
    QString deviceName; //Attribute. The name of the Bluetooth source.
};

struct NowPlayingObject {
    QString deviceID;           //Attribute. Unique identifier of the product.
    QString source;             //Attribute. The type or name of the service playing. To determine if the product is in standby mode, check if source == STANDBY.
    QString sourceAccount;      //Attribute. The account associated with this source.
    ContentItemObject ContentItem;        //Element. This object describes the content container that is playing on the product. You should treat this object as an opaque blob, for use in /select.
    QString track;              //Element. The track name.
    QString artist;             //Eement. The artist name.
    QString album;              //Element. The album name.
    QString genre;              //Element. The music genre.
    QString rating;             //Element. The user rating of the song.
    QString stationName;        //Element. The station or playlist name.
    ArtObject art;              //Element. This object provides information for source content art. See art object
    TimeObject time;            //Element. This object provides information for current and total track time. See time object
    bool skipEnabled;           //Element. If tag is present, the /key value NEXT_TRACK is valid.
    bool skipPreviousEnabled;	//Element. If tag is present, the /key value PREV_TRACK is valid.
    bool favoriteEnabled;       //Element. If tag is present, the /key values ADD_FAVORITE and REMOVE_FAVORITE are valid.
    bool isFavorit;             //Element. If tag is present, the track or station has been favorited by the user.
    bool rateEnabled;           //Element. If tag is present, the track can be rated by the user using /key THUMBS_UP and THUMBS_DOWN.
    //QString rating;               //Element. UP, DOWN, or NONE rating. If tag isn't present, the track has not been rated by the user.
    PLAY_STATUS playStatus;         //Element. This indicates whether the audio product is currently playing, paused, in standby, or in some other state.
    SHUFFLE_STATUS shuffleSetting;	//Element. The shuffle state. If this tag is present, the /key values SHUFFLE_ON and SHUFFLE_OFF are valid.
    REPEAT_STATUS repeatSettings;  	//Element. The repeat state. If this tag is present, the /key values REPEAT_OFF, REPEAT_ONE, and REPEAT_ALL are valid.
    STREAM_STATUS streamType; 		//Element. The type of music that is streaming.
    QString stationLocation;        //Element. The location of the source ex: "Internet Only".
    ConnectionStatusInfoObject ConnectionStatusInfo;       	//Element. This object describes the connection status to the Bluetooth source. See ConnectionStatusInfo object.
};

struct SourceItemObject {
    QString source;             //Attribute. The name of the source.
    QString sourceAccount;      //Attribute. The user account associated with the source.
    SOURCE_STATUS status;       //Attribute. Indicates whether the source is available.
    bool isLocal;               //Attribute. TRUE if a local source (AUX or BLUETOOTH)
    bool multiroomallowed;      //Attribute. TRUE if the source can be rebroadcast in a multi-room zone.
    QString displayName;        //Content. The user-facing name of the source.
};

struct SourcesObject {
    QString deviceId;
    QList<SourceItemObject> sourceItems;
};

struct VolumeObject {
    QString deviceID; 	//Attribute. Unique identifier of the product.
    int targetVolume;	//Element. The volume the product is trying to reach, 0 to 100 inclusive. Bigger is louder.
    int actualVolume;	//Element. The current product volume, 0 to 100 inclusive. Bigger is louder.
    bool muteEnabled; 	//Element. TRUE if the product is muted.
};

struct MemberObject {
    QString ipAddress;      //Attribute. The IP address of the product.
    QString deviceID;       //The deviceID unique identifier of the product.
};

struct ZoneObject {
    QString deviceID;               //Attribute. The deviceID unique identifier of the master product.
    QList<MemberObject> members;    //Element. This object describes products in the zone There is an object for each product.
};

struct BassCapabilitiesObject {
    QString deviceID; 	//Attribute. Unique identifier of the product.
    bool bassAvailable;	//Element. TRUE if the bass of the advice is adjustable.
    int bassMin;        //Element. The minimum value the bass can be set to.
    int bassMax;        //Element. The maximum value the bass can be set to.
    int bassDefault;	//Element. The default bass value.
};

struct BassObject {
    QString deviceID;   //Attribute. Unique identifier of the product.
    int targetBass;     //Element. The bass level the product is trying to reach.
    int actualBass;     //Element. The current bass level.
};

struct PresetObject {
    int presetId;                   //Attribute. The number of the SoundTouch preset, 1-6 inclusive.
    uint64_t createdOn;             //Attribute. A timestamp of when the preset was originally created.
    uint64_t updatedOn;             //Attribute. A timestamp of the last time the preset was changed.
    ContentItemObject ContentItem;  //Attribute. An object describing the source. See /now_playing for full documentation on this object.
};

struct GroupRoleObject {
    QString deviceID;           //Unique identifier of the product.
    GROUP_LOC role;             //The user-set location of the product. LEFT or RIGHT.
    QString ipAddress;          //The IP Address of the product.
};

struct RolesObject {
    GroupRoleObject groupRole;  //This object describes a product in the group.
};

struct GroupObject {
    QString id;                     //Attribute. A unique ID for the group.
    QString name;                   //Element. A user-set name for the group.
    QString masterDeviceId;         //Element. The unique identifier of the master product in the group.
    QList<RolesObject> roles;              //Element. This object describes the products in the group and their location (left/right).
    PLAY_STATUS  status;            //Element. The state of the stereo pair group.
};

struct PlayInfoObject {
    QString appKey;     //Element. An authorization key used to identify the client application. Apply for an app key by creating an app here.
    QString url;        //Element. A fully qualified, web hosted stream URL. The URL should include the 'http://' prefix as well as a stream suffix ('mp3', ...) for proper playback.
    QString services;   //Element. This indicates the service providing the notification. This text will appear on the thing display (when available) and the SoundTouch application screen.
    QString reason;     //Element. This indicates the reason for the notification. This text will appear on the thing display (when available) and the SoundTouch application screen. If a reason string is not provided, the field with be blank.
    QString message;    //Element. This indicates further details about the notification. This text will appear on the thing display (when available) and the SoundTouch application screen. If a message string is not provided, the field with be blank.
    int volume;         //Element. This indicates the desired volume level while playing the notification. The value represents a percentage (0 to 100) of the full audible range of the speaker thing. A value less than 10 or greater than 70 will result in an error and not play the notification. Upon completion of the notification, the speaker volume will return to its original value. If not present, the notification will play at the existing volume level.
};

struct ErrorObject{
    QString deviceId;
    int value;
    QString name;
    QString severity;
    QString error;
};

#endif // SOUNDTOUCHTYPES_H

