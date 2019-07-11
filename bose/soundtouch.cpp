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

#include "soundtouch.h"
#include "hardwaremanager.h"
#include "devices/device.h"
#include "network/networkaccessmanager.h"

SoundTouch::SoundTouch(NetworkAccessManager *networkAccessManager, QString ipAddress, QObject *parent) :
    QObject(parent),
    m_networkAccessManager(networkAccessManager),
    m_ipAddress(ipAddress)
{
    m_websocket = new QWebSocket();
    connect(m_websocket, &QWebSocket::connected, this, &SoundTouch::onWebsocketConnected);
    connect(m_websocket, &QWebSocket::disconnected, this, &SoundTouch::onWebsocketDisconnected);
    connect(m_websocket, &QWebSocket::textMessageReceived, this, &SoundTouch::onWebsocketMessageReceived);
    QUrl url;
    url.setHost(m_ipAddress);
    url.setScheme("ws");
    url.setPort(8080);
    qDebug(dcBose) << "Connecting websocket to" << url;
    //TODO missing websocket subprotocol "gabbo"
    //Seems QWebsockets doesn't support subprotocols
    m_websocket->open(url);
}

void SoundTouch::getInfo()
{
    QUrl url;
    url.setHost(m_ipAddress);
    url.setScheme("http");
    url.setPort(m_port);
    url.setPath("/info");
    //qDebug(dcBose) << "Sending request" << url;
    QNetworkReply *reply = m_networkAccessManager->get(QNetworkRequest(url));
    connect(reply, &QNetworkReply::finished, this, &SoundTouch::onRestRequestFinished);
}

void SoundTouch::getVolume()
{
    QUrl url;
    url.setHost(m_ipAddress);
    url.setScheme("http");
    url.setPort(m_port);
    url.setPath("/volume");
    //qDebug(dcBose) << "Sending request" << url;
    QNetworkReply *reply = m_networkAccessManager->get(QNetworkRequest(url));
    connect(reply, &QNetworkReply::finished, this, &SoundTouch::onRestRequestFinished);
}

void SoundTouch::getNowPlaying()
{
    QUrl url;
    url.setHost(m_ipAddress);
    url.setScheme("http");
    url.setPort(m_port);
    url.setPath("/now_playing");
    //qDebug(dcBose) << "Sending request" << url;
    QNetworkReply *reply = m_networkAccessManager->get(QNetworkRequest(url));
    connect(reply, &QNetworkReply::finished, this, &SoundTouch::onRestRequestFinished);
}

void SoundTouch::getBass()
{
    QUrl url;
    url.setHost(m_ipAddress);
    url.setScheme("http");
    url.setPort(m_port);
    url.setPath("/bass");
    //qDebug(dcBose) << "Sending request" << url;
    QNetworkReply *reply = m_networkAccessManager->get(QNetworkRequest(url));
    connect(reply, &QNetworkReply::finished, this, &SoundTouch::onRestRequestFinished);
}

void SoundTouch::getGroup()
{
    QUrl url;
    url.setHost(m_ipAddress);
    url.setScheme("http");
    url.setPort(m_port);
    url.setPath("/group");
    qDebug(dcBose) << "Sending request" << url;
    QNetworkReply *reply = m_networkAccessManager->get(QNetworkRequest(url));
    connect(reply, &QNetworkReply::finished, this, &SoundTouch::onRestRequestFinished);
}

void SoundTouch::getSources()
{
    QUrl url;
    url.setHost(m_ipAddress);
    url.setScheme("http");
    url.setPort(m_port);
    url.setPath("/sources");
    //qDebug(dcBose) << "Sending request" << url;
    QNetworkReply *reply = m_networkAccessManager->get(QNetworkRequest(url));
    connect(reply, &QNetworkReply::finished, this, &SoundTouch::onRestRequestFinished);
}

void SoundTouch::getZone()
{
    QUrl url;
    url.setHost(m_ipAddress);
    url.setScheme("http");
    url.setPort(m_port);
    url.setPath("/getZone");
    qDebug(dcBose) << "Sending request" << url;
    QNetworkReply *reply = m_networkAccessManager->get(QNetworkRequest(url));
    connect(reply, &QNetworkReply::finished, this, &SoundTouch::onRestRequestFinished);
}

void SoundTouch::getPresets()
{
    QUrl url;
    url.setHost(m_ipAddress);
    url.setScheme("http");
    url.setPort(m_port);
    url.setPath("/presets");
    qDebug(dcBose) << "Sending request" << url;
    QNetworkReply *reply = m_networkAccessManager->get(QNetworkRequest(url));
    connect(reply, &QNetworkReply::finished, this, &SoundTouch::onRestRequestFinished);
}

void SoundTouch::setKey(KEY_VALUE keyValue)
{
    QUrl url;
    url.setHost(m_ipAddress);
    url.setScheme("http");
    url.setPort(m_port);
    url.setPath("/key");
    QByteArray content = ("<?xml version=\"1.0\" ?>");
    content.append("<key state=\"press\" sender=\"Gabbo\">");
    switch (keyValue){
    case KEY_VALUE_PLAY:
        content.append("PLAY");
        break;
    case KEY_VALUE_STOP:
        content.append("STOP");
        break;
    case KEY_VALUE_PAUSE:
        content.append("PAUSE");
        break;
    case KEY_VALUE_PLAY_PAUSE:
        content.append("PLAY_PAUSE");
        break;
    case KEY_VALUE_POWER:
        content.append("POWER");
        break;
    case KEY_VALUE_NEXT_TRACK:
        content.append("NEXT_TRACK");
        break;
    case KEY_VALUE_PREV_TRACK:
        content.append("PREV_TRACK");
        break;
    case KEY_VALUE_BOOKMARK:
        content.append("BOOKMARK");
        break;
    case KEY_VALUE_AUX_INPUT:
        content.append("AUX_INPUT");
        break;
    case KEY_VALUE_REPEAT_ALL:
        content.append("REPEAT_ALL");
        break;
    case KEY_VALUE_REPEAT_ONE:
        content.append("REPEAT_ONE");
        break;
    case KEY_VALUE_REPEAT_OFF:
        content.append("REPEAT_OFF");
        break;
    case KEY_VALUE_ADD_FAVORITE:
        content.append("ADD_FAVORITE");
        break;
    case KEY_VALUE_MUTE:
        content.append("MUTE");
        break;
    case KEY_VALUE_SHUFFLE_ON:
        content.append("SHUFFLE_ON");
        break;
    case KEY_VALUE_SHUFFLE_OFF:
        content.append("SHUFFLE_OFF");
        break;
    default:
        qWarning(dcBose) << "key not yet implemented";
        return;
    }
    content.append("</key>");
    qDebug(dcBose) << "Sending request" << url << content;
    QNetworkReply *reply = m_networkAccessManager->post(QNetworkRequest(url), content);
    connect(reply, &QNetworkReply::finished, this, &SoundTouch::onRestRequestFinished);

    if (keyValue == KEY_VALUE_POWER) {
        QUrl url;
        url.setHost(m_ipAddress);
        url.setScheme("http");
        url.setPort(m_port);
        url.setPath("/key");
        QByteArray content = ("<?xml version=\"1.0\" ?>");
        content.append("<key state=\"release\" sender=\"Gabbo\">");
        content.append("POWER");
        content.append("</key>");
        //qDebug(dcBose) << "Sending request" << url << content;
        QNetworkReply *reply = m_networkAccessManager->post(QNetworkRequest(url), content);
        connect(reply, &QNetworkReply::finished, this, &SoundTouch::onRestRequestFinished);
    }
}

void SoundTouch::setVolume(int volume)
{
    QUrl url;
    url.setHost(m_ipAddress);
    url.setScheme("http");
    url.setPort(m_port);
    url.setPath("/volume");
    QByteArray content = ("<?xml version=\"1.0\" ?>");
    content.append("<volume>");
    content.append(QByteArray::number(volume));
    content.append("</volume>");
    //qDebug(dcBose) << "Sending request" << url << content;
    QNetworkReply *reply = m_networkAccessManager->post(QNetworkRequest(url), content);
    connect(reply, &QNetworkReply::finished, this, &SoundTouch::onRestRequestFinished);
}

void SoundTouch::setSource(ContentItemObject contentItem)
{
    QUrl url;
    url.setHost(m_ipAddress);
    url.setScheme("http");
    url.setPort(m_port);
    url.setPath("/select"); //Select source
    QByteArray content;
    QXmlStreamWriter xml(&content);
    xml.writeStartDocument();
    xml.writeStartElement("ContentItem");
    xml.writeAttribute("source", contentItem.source);
    xml.writeAttribute("sourceAccount", contentItem.sourceAccount);
    xml.writeEndElement(); //ContentItem
    xml.writeEndDocument();
    qDebug(dcBose) << "Sending request" << url << content;

    //QNetworkReply *reply = m_networkAccessManager->post(QNetworkRequest(url), content);
    //connect(reply, &QNetworkReply::finished, this, &SoundTouch::onRestRequestFinished);
}

void SoundTouch::setZone(ZoneObject zone)
{
    QUrl url;
    url.setHost(m_ipAddress);
    url.setScheme("http");
    url.setPort(m_port);
    url.setPath("/setZone");
    QByteArray content;
    QXmlStreamWriter xml(&content);
    xml.writeStartDocument();
    xml.writeStartElement("zone");
    xml.writeAttribute("master", zone.deviceID);
    xml.writeTextElement("member", zone.member.deviceID);
    xml.writeAttribute("ipaddress", zone.member.ipAddress);
    xml.writeEndElement(); //zone
    xml.writeEndDocument();
    qDebug(dcBose) << "Sending request" << url << content;
    //QNetworkReply *reply = m_networkAccessManager->post(QNetworkRequest(url), content);
    //connect(reply, &QNetworkReply::finished, this, &SoundTouch::onRestRequestFinished);
}

void SoundTouch::addZoneSlave(ZoneObject zone)
{
    QUrl url;
    url.setHost(m_ipAddress);
    url.setScheme("http");
    url.setPort(m_port);
    url.setPath("/addZoneSlave");
    QByteArray content;
    QXmlStreamWriter xml(&content);
    xml.writeStartDocument();
    xml.writeStartElement("zone");
    xml.writeAttribute("master", zone.deviceID);
    xml.writeTextElement("member", zone.member.deviceID);
    xml.writeAttribute("ipaddress", zone.member.ipAddress);
    xml.writeEndElement(); //zone
    xml.writeEndDocument();
    qDebug(dcBose) << "Sending request" << url << content;
    //QNetworkReply *reply = m_networkAccessManager->post(QNetworkRequest(url), content);
    //connect(reply, &QNetworkReply::finished, this, &SoundTouch::onRestRequestFinished);
}

void SoundTouch::removeZoneSlave(ZoneObject zone)
{
    QUrl url;
    url.setHost(m_ipAddress);
    url.setScheme("http");
    url.setPort(m_port);
    url.setPath("/removeZoneSlave");
    QByteArray content;
    QXmlStreamWriter xml(&content);
    xml.writeStartDocument();
    xml.writeStartElement("zone");
    xml.writeAttribute("master", zone.deviceID);
    xml.writeTextElement("member", zone.member.deviceID);
    xml.writeAttribute("ipaddress", zone.member.ipAddress);
    xml.writeEndElement(); //zone
    xml.writeEndDocument();
    qDebug(dcBose) << "Sending request" << url << content;
    //QNetworkReply *reply = m_networkAccessManager->post(QNetworkRequest(url), content);
    //connect(reply, &QNetworkReply::finished, this, &SoundTouch::onRestRequestFinished);
}

void SoundTouch::setBass(int volume)
{
    QUrl url;
    url.setHost(m_ipAddress);
    url.setScheme("http");
    url.setPort(m_port);
    url.setPath("/bass");
    QByteArray content = ("<?xml version=\"1.0\" ?>");
    content.append("<bass>");
    content.append(QByteArray::number(volume));
    content.append("</bass>");
    qDebug(dcBose) << "Sending request" << url << content;
    QNetworkReply *reply = m_networkAccessManager->post(QNetworkRequest(url), content);
    connect(reply, &QNetworkReply::finished, this, &SoundTouch::onRestRequestFinished);
}

void SoundTouch::setName(QString name)
{
    QUrl url;
    url.setHost(m_ipAddress);
    url.setScheme("http");
    url.setPort(m_port);
    url.setPath("/name");
    QByteArray content = ("<?xml version=\"1.0\" ?>");
    content.append("<name>");
    content.append(name);
    content.append("</name>");
    qDebug(dcBose) << "Sending request" << url << content;
    QNetworkReply *reply = m_networkAccessManager->post(QNetworkRequest(url), content);
    connect(reply, &QNetworkReply::finished, this, &SoundTouch::onRestRequestFinished);
}

void SoundTouch::setSpeaker(PlayInfoObject playInfo)
{
    QUrl url;
    url.setHost(m_ipAddress);
    url.setScheme("http");
    url.setPort(m_port);
    url.setPath("/speaker");
    QByteArray content;
    QXmlStreamWriter xml(&content);
    xml.writeStartDocument();
    xml.writeStartElement("play_info");
    xml.writeTextElement("app_key", playInfo.appKey);
    xml.writeTextElement("url", playInfo.url);
    xml.writeTextElement("service", playInfo.services);
    xml.writeTextElement("reason", playInfo.reason);
    xml.writeTextElement("message", playInfo.message);
    xml.writeTextElement("volume", QString::number(playInfo.volume));
    xml.writeEndElement(); //play_info
    xml.writeEndDocument();
    qDebug(dcBose) << "Sending request" << url << content;
    //QNetworkReply *reply = m_networkAccessManager->post(QNetworkRequest(url), content);
    //connect(reply, &QNetworkReply::finished, this, &SoundTouch::onRestRequestFinished);
}

void SoundTouch::onWebsocketConnected()
{
    qDebug(dcBose) << "Bose websocket connected";
    emit connectionChanged(true);
}

void SoundTouch::onWebsocketDisconnected()
{
    qDebug(dcBose) << "Bose websocket disconnected";
    emit connectionChanged(false);
    QTimer::singleShot(5000, [this](){
        QUrl url;
        url.setHost(m_ipAddress);
        url.setScheme("ws");
        url.setPort(8080);
        m_websocket->open(url);
    });
}


void SoundTouch::onRestRequestFinished() {

    QNetworkReply *reply = static_cast<QNetworkReply *>(sender());
    reply->deleteLater();

    QByteArray data = reply->readAll();
    int status = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();

    //qDebug(dcBose) << data;
    // Check HTTP status code
    if (status != 200 || reply->error() != QNetworkReply::NoError) {
        qCWarning(dcBose()) << "Request error:" << status << reply->errorString();
    }

    QXmlStreamReader xml;
    xml.addData(data);

    if (xml.readNextStartElement()) {
        if (xml.name() == "info") {
            InfoObject info;
            qDebug(dcBose) << "Info Request";
            if(xml.attributes().hasAttribute("deviceID")) {
                qDebug(dcBose) << "Device ID" << xml.attributes().value("deviceID").toString();
                info.deviceID = xml.attributes().value("deviceID").toString();
            }
            while(xml.readNextStartElement()){
                if(xml.name() == "name"){
                    //qDebug(dcBose) << "name" << xml.readElementText();
                    info.name =  xml.readElementText();
                } else if(xml.name() == "type"){
                    //qDebug(dcBose) << "type" << xml.readElementText();
                    info.type =  xml.readElementText();
                } else if(xml.name() == "components"){
                    //qDebug(dcBose) << "components element";
                    while(xml.readNextStartElement()){
                        if(xml.name() == "component"){
                            ComponentObject component;
                            while(xml.readNextStartElement()){
                                if(xml.name() == "softwareVersion"){
                                    //qDebug(dcBose) << "Software version" << xml.readElementText();
                                    component.softwareVersion = xml.readElementText();
                                } else if(xml.name() == "serialNumber") {
                                    //qDebug(dcBose) << "Serialnumber" << xml.readElementText();
                                    component.serialNumber = xml.readElementText();
                                } else {
                                    xml.skipCurrentElement();
                                }
                            }
                            info.components.append(component);
                        } else {
                            xml.skipCurrentElement();
                        }
                    }
                } else if(xml.name() == "networkInfo"){
                    qDebug(dcBose) << "network Info";
                    while (xml.readNextStartElement()) {
                        if (xml.name() == "macAddress") {
                            qDebug(dcBose) << "macAddress" << xml.readElementText();
                        } else if(xml.name() == "ipAddress") {
                            qDebug(dcBose) << "ipAddress" << xml.readElementText();
                        } else {
                            xml.skipCurrentElement();
                        }
                    }
                }  else {
                    xml.skipCurrentElement();
                }
            }
            emit infoReceived(info);
        } else if (xml.name() == "nowPlaying") {
            NowPlayingObject nowPlaying;
            if(xml.attributes().hasAttribute("deviceID")) {
                //qDebug(dcBose) << "Device ID" << xml.attributes().value("deviceID").toString();
                nowPlaying.deviceID = xml.attributes().value("deviceID").toString();
            }
            if(xml.attributes().hasAttribute("source")) {
                //qDebug(dcBose) << "Source" << xml.attributes().value("source").toString();
                nowPlaying.source = xml.attributes().value("source").toString();
            }
            if(xml.attributes().hasAttribute("sourceAccount")) {
                //qDebug(dcBose) << "Source Account" << xml.attributes().value("sourceAccount").toString();
                nowPlaying.sourceAccount = xml.attributes().value("sourceAccount").toString();
            }
            while(xml.readNextStartElement()){
                if (xml.name() == "track") {
                    //qDebug(dcBose) << "track" << xml.readElementText();
                    nowPlaying.track = xml.readElementText();
                } else if(xml.name() == "artist") {
                    //qDebug(dcBose) << "artist" << xml.readElementText();
                    nowPlaying.artist = xml.readElementText();
                } else if(xml.name() == "album") {
                    //qDebug(dcBose) << "album" << xml.readElementText();
                    nowPlaying.album = xml.readElementText();
                } else if(xml.name() == "genre") {
                    //qDebug(dcBose) << "genre" << xml.readElementText();
                    nowPlaying.genre = xml.readElementText();
                } else if(xml.name() == "rating") {
                    //qDebug(dcBose) << "rating" << xml.readElementText();
                    nowPlaying.rating = xml.readElementText();
                } else if(xml.name() == "stationName") {
                    //qDebug(dcBose) << "Station name" << xml.readElementText();
                    nowPlaying.stationName = xml.readElementText();
                } else if(xml.name() == "art") {
                    ArtObject art;
                    if(xml.attributes().hasAttribute("artImageStatus")) {
                        QString artStatus = xml.attributes().value("artImageStatus").toString().toUpper();
                        //ART_STATUS: INVALID, SHOW_DEFAULT_IMAGE, DOWNLOADING, IMAGE_PRESENT
                        //qDebug(dcBose) << "Art Image status" << artStatus;
                        if (artStatus == "INVALID") {
                            art.artStatus = ART_STATUS_INVALID;
                        } else if (artStatus == "SHOW_DEFAULT_IMAGE") {
                            art.artStatus = ART_STATUS_SHOW_DEFAULT_IMAGE;
                        }  else if (artStatus == "DOWNLOADING") {
                            art.artStatus = ART_STATUS_DOWNLOADING;
                        }  else if (artStatus == "IMAGE_PRESENT") {
                            art.artStatus = ART_STATUS_IMAGE_PRESENT;
                        }
                    }
                    nowPlaying.art.url = xml.readElementText();
                }else if(xml.name() == "playStatus") {
                    QString playStatus = xml.readElementText();
                    //qDebug(dcBose) << "Play Status" << playStatus;
                    //Modes: PLAY_STATE, PAUSE_STATE, STOP_STATE, BUFFERING_STATE
                    if (playStatus == "PLAY_STATE") {
                        nowPlaying.playStatus = PLAY_STATUS_PLAY_STATE;
                    } else if (playStatus == "PAUSE_STATE") {
                        nowPlaying.playStatus = PLAY_STATUS_PAUSE_STATE;
                    } else if (playStatus == "STOP_STATE") {
                        nowPlaying.playStatus = PLAY_STATUS_STOP_STATE;
                    } else if (playStatus == "BUFFERING_STATE") {
                        nowPlaying.playStatus = PLAY_STATUS_BUFFERING_STATE;
                    }
                } else if(xml.name() == "shuffleSetting") {
                    QString shuffle = xml.readElementText().toUpper();
                    //qDebug(dcBose) << "Shuffle Setting" << shuffle;
                    if (shuffle == "SHUFFLE_ON") {
                        nowPlaying.shuffleSetting = SHUFFLE_STATUS_SHUFFLE_ON;
                    } else {
                        nowPlaying.shuffleSetting = SHUFFLE_STATUS_SHUFFLE_OFF;
                    }
                }else if(xml.name() == "repeatSetting") {
                    QString repeat = xml.readElementText().toUpper();
                    //qDebug(dcBose) << "Repeat Setting" << repeat;
                    //Modes: REPEAT_OFF, REPEAT_ALL, REPEAT_ONE
                    if (repeat == "REPEAT_OFF") {
                        nowPlaying.repeatSettings = REPEAT_STATUS_REPEAT_OFF;
                    } else if (repeat == "REPEAT_ONE") {
                        nowPlaying.repeatSettings = REPEAT_STATUS_REPEAT_ONE;
                    } else if (repeat == "REPEAT_ALL") {
                        nowPlaying.repeatSettings = REPEAT_STATUS_REPEAT_ALL;
                    }
                } else if(xml.name() == "streamType") {
                    QString streamType = xml.readElementText().toUpper();
                    //qDebug(dcBose) << "Stream Type" << streamType;
                    //Types: TRACK_ONDEMAND, RADIO_STREAMING, RADIO_TRACKS, NO_TRANSPORT_CONTROLS
                    if (streamType == "RADIO_TRACKS") {
                        nowPlaying.streamType = STREAM_STATUS_RADIO_TRACKS;
                    } else if (streamType == "TRACK_ONDEMAND") {
                        nowPlaying.streamType = STREAM_STATUS_TRACK_ONDEMAND;
                    } else if (streamType == "RADIO_STREAMING") {
                        nowPlaying.streamType = STREAM_STATUS_RADIO_STREAMING;
                    } else if (streamType == "NO_TRANSPORT_CONTROLS") {
                        nowPlaying.streamType = STREAM_STATUS_NO_TRANSPORT_CONTROLS;
                    };
                } else if(xml.name() == "stationLocation") {
                    nowPlaying.stationLocation = xml.readElementText();
                } else {
                    xml.skipCurrentElement();
                }
            }

            emit nowPlayingReceived(nowPlaying);
        } else if (xml.name() == "volume") {
            VolumeObject volumeObject;
            if(xml.attributes().hasAttribute("deviceID")) {
                //qDebug(dcBose) << "Device ID" << xml.attributes().value("deviceID").toString();
                volumeObject.deviceID = xml.attributes().value("deviceID").toString();
            }
            while(xml.readNextStartElement()){
                if(xml.name() == "targetvolume"){
                    //qDebug(dcBose) << "Target volume" << xml.readElementText();
                    volumeObject.targetVolume = xml.readElementText().toInt();
                }else if(xml.name() == "actualvolume"){
                    //qDebug(dcBose) << "Actual volume" << xml.readElementText();
                    volumeObject.actualVolume = xml.readElementText().toInt();
                }else if(xml.name() == "muteenabled"){
                    //qDebug(dcBose) << "Mute enabled" << xml.readElementText();
                    volumeObject.muteEnabled = ( xml.readElementText().toUpper() == "TRUE" ); //TODO convert from "false" to bool
                }else {
                    xml.skipCurrentElement();
                }
            }
            emit volumeReceived(volumeObject);
        } else if (xml.name() == "sources") {
            SourcesObject sourcesObject;
            if(xml.attributes().hasAttribute("deviceID")) {
                //qDebug(dcBose) << "Device ID" << xml.attributes().value("deviceID").toString();
                sourcesObject.deviceId = xml.attributes().value("deviceID").toString();
            }
            while(xml.readNextStartElement()){
                if(xml.name() == "sourceItem"){
                    SourceItemObject sourceItem;
                    if(xml.attributes().hasAttribute("source")) {
                        //qDebug(dcBose) << "Source" << xml.attributes().value("source").toString();
                        sourceItem.source = xml.attributes().value("source").toString();
                    } else if(xml.attributes().hasAttribute("sourceAccount")) {
                        //qDebug(dcBose) << "Source Account" << xml.attributes().value("sourceAccount").toString();
                        sourceItem.sourceAccount = xml.attributes().value("sourceAccount").toString();
                    } else if(xml.attributes().hasAttribute("status")) {
                        QString status = xml.attributes().value("status").toString().toUpper(); //UNAVAILABLE, READY
                        //qDebug(dcBose) << "status" << status;
                        if (status == "READY") {
                            sourceItem.status = SOURCE_STATUS_READY;
                        } else {
                            sourceItem.status = SOURCE_STATUS_UNAVAILABLE;
                        }
                    } else if(xml.attributes().hasAttribute("isLocal")) {
                        //qDebug(dcBose) << "is Local" << xml.attributes().value("isLocal").toString();
                        sourceItem.isLocal = ( xml.attributes().value("isLocal").toString().toUpper() == "TRUE" );
                    } else if(xml.attributes().hasAttribute("multiroomallowed")) {
                        //Debug(dcBose) << "multiroom allowed" << xml.attributes().value("multiroomallowed").toString();
                        sourceItem.multiroomallowed = ( xml.attributes().value("multiroomallowed").toString().toUpper() == "TRUE" );
                    }
                    sourceItem.displayName = xml.readElementText();
                    sourcesObject.sourceItems.append(sourceItem);
                }else {
                    xml.skipCurrentElement();
                }
            }
            emit sourcesReceived(sourcesObject);
        } else {
            xml.skipCurrentElement();
        }
    }
}


void SoundTouch::onWebsocketMessageReceived(QString message)
{
    qDebug(dcBose) << "Websocket message received:" << message;
}
