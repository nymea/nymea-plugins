/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *                                                                         *
 *  Copyright (C) 2019 Bernhard Trinnes <bernhard.trinnes@nymea.io         *
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

#ifndef SOUNDTOUCH_H
#define SOUNDTOUCH_H

#include <QObject>
#include <QXmlStreamReader>
#include <QtWebSockets/QtWebSockets>

#include "extern-plugininfo.h"
#include "soundtouchtypes.h"

#include "hardwaremanager.h"
#include "network/networkaccessmanager.h"


class SoundTouch : public QObject
{
    Q_OBJECT
public:
    explicit SoundTouch(NetworkAccessManager *networkAccessManager, QString ipAddress, QObject *parent = nullptr);

    void getInfo();             //Get basic information about a product.
    void getVolume();           //Get the current volume and mute status of a product.
    void getNowPlaying();       //Get information about what's playing on a product.
    void getBass();             //Get the bass level of a product, if supported.
    void getGroup();            //Get the current left/right stereo pair configuration of a product.
    void getSources();          //Get available sources for a product.
    void getZone();             //Get the current multiroom zone state of a product.
    void getPresets();          //Get information related to the user's SoundTouch presets.
    void getBassCapabilities(); //Get whether a product supports reducing bass.

    void setKey(KEY_VALUE keyValue);                //Start and control playback on a product.
    void setVolume(int volume);                     //Set the volume of a product.
    void setSource(ContentItemObject contentItem);  //Select a product's source.
    void setZone(ZoneObject zone);                  //Create a zone of synced products.
    void addZoneSlave(ZoneObject zone);             //Add one or more slave product(s) to a multiroom zone.
    void removeZoneSlave(ZoneObject zone);          //Remove one or more slave product(s) from a multiroom zone.
    void setBass(int volume);                       //Set the bass level of a product, if supported.*/
    void setName(QString name);                     //Set the products user-facing name.
    void setSpeaker(PlayInfoObject playInfo);       //initiate playback of a specified network-accessible audio file on a Bose SoundTouch product.


private:
    enum RequestType {
        Get,
        Post
    };

    void sendGetRequest(QString path);
    //get calls are getting queued to don't overstrain the device
    //post calls must get sent immediately
    //if an get call of the same URL is already in the queu the new one will be ignored
    QList<QString> m_getRequestQueue;
    bool m_getRepliesPending = false;

    NetworkAccessManager *m_networkAccessManager = nullptr;
    QString m_ipAddress;
    int m_port = 8090;
    QWebSocket *m_websocket = nullptr;

signals:
    void connectionChanged(bool connected);

    void infoReceived(InfoObject info);
    void nowPlayingReceived(NowPlayingObject nowPlaying);
    void volumeReceived(VolumeObject volume);
    void sourcesReceived(SourcesObject sources);
    void zoneReceived(ZoneObject);
    void bassCapabilitiesReceived(BassCapabilitiesObject bassCapabilities);
    void bassReceived(BassObject bass);
    void presetsReceived(PresetObject presets);
    void groupReceived(GroupObject group);


private slots:
    void onWebsocketConnected();
    void onWebsocketDisconnected();
    void onWebsocketMessageReceived(QString message);
    void onRestRequestFinished();
    void onRestRequestError(QNetworkReply::NetworkError error);
};

#endif // SOUNDTOUCH_H
