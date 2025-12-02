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

#ifndef SOUNDTOUCH_H
#define SOUNDTOUCH_H

#include <QObject>
#include <QXmlStreamReader>
#include <QtWebSockets/QtWebSockets>
#include <QUuid>

#include "extern-plugininfo.h"
#include "soundtouchtypes.h"

#include <network/networkaccessmanager.h>

class SoundTouch : public QObject
{
    Q_OBJECT
public:
    explicit SoundTouch(NetworkAccessManager *networkAccessManager, QString ipAddress, QObject *parent = nullptr);

    QUuid getInfo();             //Get basic information about a product.
    QUuid getVolume();           //Get the current volume and mute status of a product.
    QUuid getNowPlaying();       //Get information about what's playing on a product.
    QUuid getBass();             //Get the bass level of a product, if supported.
    QUuid getGroup();            //Get the current left/right stereo pair configuration of a product.
    QUuid getZone();             //Get the current multiroom zone state of a product.
    QUuid getBassCapabilities(); //Get whether a product supports reducing bass.
    QUuid getSources();          //Get available sources for a product.
    QUuid getPresets();          //Get information related to the user's SoundTouch presets.

    //ACTIONS
    QUuid setKey(KEY_VALUE keyValue, bool pressed);  //Start and control playback on a product.
    QUuid setVolume(int volume);                     //Set the volume of a product.
    QUuid setSource(ContentItemObject contentItem);  //Select a product's source.
    QUuid setZone(ZoneObject zone);                  //Create a zone of synced products.
    QUuid addZoneSlave(ZoneObject zone);             //Add one or more slave product(s) to a multiroom zone.
    QUuid removeZoneSlave(ZoneObject zone);          //Remove one or more slave product(s) from a multiroom zone.
    QUuid setBass(int volume);                       //Set the bass level of a product, if supported.*/
    QUuid setName(const QString &name);                     //Set the products user-facing name.
    QUuid setSpeaker(PlayInfoObject playInfo);       //initiate playback of a specified network-accessible audio file on a Bose SoundTouch product.

private:
    QUuid sendGetRequest(QString path);
    //Get calls are getting queued to don't overstrain the thing
    //Post calls must be sent immediately
    //If an get call of the same URL is already in the queu the new one will be ignored
    QList<QString> m_getRequestQueue;
    bool m_getRepliesPending = false;

    NetworkAccessManager *m_networkAccessManager = nullptr;
    QString m_ipAddress;
    int m_port = 8090;
    QWebSocket *m_websocket = nullptr;
    void emitRequestStatus(const QUuid &requestId, QNetworkReply *reply); //returns the status, -1 in case of error
    void parseData(const QUuid &requestId, const QByteArray &data);

signals:
    void connectionChanged(bool connected);

    void infoReceived(const QUuid &requestId, InfoObject info);
    void nowPlayingReceived(const QUuid &requestId, NowPlayingObject nowPlaying);
    void volumeReceived(const QUuid &requestId, VolumeObject volume);
    void sourcesReceived(const QUuid &requestId, SourcesObject sources);
    void zoneReceived(const QUuid &requestId, ZoneObject);
    void bassCapabilitiesReceived(const QUuid &requestId, BassCapabilitiesObject bassCapabilities);
    void bassReceived(const QUuid &requestId, BassObject bass);
    void presetsReceived(const QUuid &requestId, QList<PresetObject> presets);
    void groupReceived(const QUuid &requestId, GroupObject group);

    void requestExecuted(const QUuid &requestId, bool success);
    void errorReceived(ErrorObject error);

private slots:
    void onWebsocketConnected();
    void onWebsocketDisconnected();
    void onWebsocketMessageReceived(QString message);
};

#endif // SOUNDTOUCH_H
