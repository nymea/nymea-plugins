/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*
* Copyright 2013 - 2025, nymea GmbH
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

#include "soundtouch.h"

#include <hardwaremanager.h>
#include <integrations/thing.h>

SoundTouch::SoundTouch(NetworkAccessManager *networkAccessManager, QString ipAddress, QObject *parent) :
    QObject(parent),
    m_networkAccessManager(networkAccessManager),
    m_ipAddress(ipAddress)
{
    //THIS CODE IS LEFT INTENTIONALLY
    //m_websocket = new QWebSocket();
    //connect(m_websocket, &QWebSocket::connected, this, &SoundTouch::onWebsocketConnected);
    //connect(m_websocket, &QWebSocket::disconnected, this, &SoundTouch::onWebsocketDisconnected);
    //connect(m_websocket, &QWebSocket::textMessageReceived, this, &SoundTouch::onWebsocketMessageReceived);
    //QUrl url;
    //url.setHost(m_ipAddress);
    //url.setScheme("ws");
    //url.setPort(8080);
    //qCDebug(dcBose()) << "Connecting websocket to" << url;
    //TODO missing websocket subprotocol "gabbo"
    //QWebsockets doesn't support subprotocols
    //m_websocket->open(url);
}

QUuid SoundTouch::getInfo()
{
    QUuid requestId;
    if (!m_getRepliesPending) {
        requestId = sendGetRequest("/info");
    } else {
        if (!m_getRequestQueue.contains("/info"))
            m_getRequestQueue.append("/info");
    }
    return requestId;
}

QUuid SoundTouch::getVolume()
{
    QUuid requestId;
    if (!m_getRepliesPending) {
        requestId = sendGetRequest("/volume");
    } else {
        if (!m_getRequestQueue.contains("/volume"))
            m_getRequestQueue.append("/volume");
    }
    return requestId;
}

QUuid SoundTouch::getNowPlaying()
{
    QUuid requestId;
    if (!m_getRepliesPending) {
        requestId = sendGetRequest("/now_playing");
    } else {
        if (!m_getRequestQueue.contains("/now_playing"))
            m_getRequestQueue.append("/now_playing");
    }
    return requestId;
}

QUuid SoundTouch::getBass()
{
    QUuid requestId;
    if (!m_getRepliesPending) {
        requestId = sendGetRequest("/bass");
    } else {
        if (!m_getRequestQueue.contains("/bass"))
            m_getRequestQueue.append("/bass");
    }
    return requestId;
}

QUuid SoundTouch::getGroup()
{
    QUuid requestId;
    if (!m_getRepliesPending) {
        requestId = sendGetRequest("/getGroup");
    } else {
        if (!m_getRequestQueue.contains("/getGroup"))
            m_getRequestQueue.append("/getGroup");
    }
    return requestId;
}

QUuid SoundTouch::getSources()
{
    QUuid requestId;
    if (!m_getRepliesPending) {
        requestId = sendGetRequest("/sources");
    } else {
        if (!m_getRequestQueue.contains("/sources"))
            m_getRequestQueue.append("/sources");
    }
    return requestId;
}

QUuid SoundTouch::getZone()
{
    QUuid requestId;
    if (!m_getRepliesPending)  {
        requestId = sendGetRequest("/getZone");
    } else {
        if (!m_getRequestQueue.contains("/getZone"))
            m_getRequestQueue.append("/getZone");
    }
    return requestId;
}

QUuid SoundTouch::getPresets()
{
    QUuid requestId;
    if (!m_getRepliesPending)  {
        requestId = sendGetRequest("/presets");
    } else {
        if (!m_getRequestQueue.contains("/presets"))
            m_getRequestQueue.append("/presets");
    }
    return requestId;
}


QUuid SoundTouch::getBassCapabilities()
{
    QUuid requestId;
    if (!m_getRepliesPending)  {
        requestId = sendGetRequest("/bassCapabilities");
    } else {
        if (!m_getRequestQueue.contains("/bassCapabilities"))
            m_getRequestQueue.append("/bassCapabilities");
    }
    return requestId;
}

QUuid SoundTouch::setKey(KEY_VALUE keyValue, bool pressed)
{
    QUuid requestId = QUuid::createUuid();
    QUrl url;
    url.setHost(m_ipAddress);
    url.setScheme("http");
    url.setPort(m_port);
    url.setPath("/key");
    QByteArray content;
    QXmlStreamWriter xml(&content);
    xml.writeStartDocument("1.0");
    xml.writeStartElement("key");
    if (pressed) {
        xml.writeAttribute("state", "press");
    } else {
        xml.writeAttribute("state", "release");
    }

    xml.writeAttribute("sender", "Gabbo");
    switch (keyValue){
    case KEY_VALUE_PLAY:
        xml.writeCharacters("PLAY");
        break;
    case KEY_VALUE_STOP:
        xml.writeCharacters("STOP");
        break;
    case KEY_VALUE_PAUSE:
        xml.writeCharacters("PAUSE");
        break;
    case KEY_VALUE_PLAY_PAUSE:
        xml.writeCharacters("PLAY_PAUSE");
        break;
    case KEY_VALUE_POWER:
        xml.writeCharacters("POWER");
        break;
    case KEY_VALUE_NEXT_TRACK:
        xml.writeCharacters("NEXT_TRACK");
        break;
    case KEY_VALUE_PREV_TRACK:
        xml.writeCharacters("PREV_TRACK");
        break;
    case KEY_VALUE_BOOKMARK:
        xml.writeCharacters("BOOKMARK");
        break;
    case KEY_VALUE_AUX_INPUT:
        xml.writeCharacters("AUX_INPUT");
        break;
    case KEY_VALUE_REPEAT_ALL:
        xml.writeCharacters("REPEAT_ALL");
        break;
    case KEY_VALUE_REPEAT_ONE:
        xml.writeCharacters("REPEAT_ONE");
        break;
    case KEY_VALUE_REPEAT_OFF:
        xml.writeCharacters("REPEAT_OFF");
        break;
    case KEY_VALUE_ADD_FAVORITE:
        xml.writeCharacters("ADD_FAVORITE");
        break;
    case KEY_VALUE_MUTE:
        xml.writeCharacters("MUTE");
        break;
    case KEY_VALUE_SHUFFLE_ON:
        xml.writeCharacters("SHUFFLE_ON");
        break;
    case KEY_VALUE_SHUFFLE_OFF:
        xml.writeCharacters("SHUFFLE_OFF");
        break;
    case KEY_VALUE_PRESET_1:
        xml.writeCharacters("PRESET_1");
        break;
    case KEY_VALUE_PRESET_2:
        xml.writeCharacters("PRESET_2");
        break;
    case KEY_VALUE_PRESET_3:
        xml.writeCharacters("PRESET_3");
        break;
    case KEY_VALUE_PRESET_4:
        xml.writeCharacters("PRESET_4");
        break;
    case KEY_VALUE_PRESET_5:
        xml.writeCharacters("PRESET_5");
        break;
    case KEY_VALUE_PRESET_6:
        xml.writeCharacters("PRESET_6");
        break;
    default:
        qCWarning(dcBose()) << "key not yet implemented";
        return QUuid();
    }

    xml.writeEndElement(); //key
    xml.writeEndDocument();
    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/xml");
    QNetworkReply *reply = m_networkAccessManager->post(request, content);
    connect(reply, &QNetworkReply::finished, this, [requestId, reply, this] {
        reply->deleteLater();
        emitRequestStatus(requestId, reply);
    });

    //Power button needs to be released immediatelly after beeing pressed
    if (keyValue == KEY_VALUE_POWER && pressed) {
        QUrl url;
        url.setHost(m_ipAddress);
        url.setScheme("http");
        url.setPort(m_port);
        url.setPath("/key");
        QByteArray content;
        QXmlStreamWriter xml(&content);
        xml.writeStartDocument("1.0");
        xml.writeStartElement("key");
        xml.writeAttribute("state", "release");
        xml.writeAttribute("sender", "Gabbo");
        xml.writeCharacters("POWER");
        xml.writeEndElement(); //key
        xml.writeEndDocument();
        QNetworkRequest request(url);
        request.setHeader(QNetworkRequest::ContentTypeHeader, "application/xml");
        QNetworkReply *reply = m_networkAccessManager->post(request, content);
        connect(reply, &QNetworkReply::finished, this, [reply] {reply->deleteLater();});
    }
    return requestId;
}


QUuid SoundTouch::setVolume(int volume)
{
    QUuid requestId = QUuid::createUuid();
    QUrl url;
    url.setHost(m_ipAddress);
    url.setScheme("http");
    url.setPort(m_port);
    url.setPath("/volume");
    QByteArray content = ("<?xml version=\"1.0\" ?>");
    content.append("<volume>");
    content.append(QByteArray::number(volume));
    content.append("</volume>");
    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/xml");
    QNetworkReply *reply = m_networkAccessManager->post(request, content);
    connect(reply, &QNetworkReply::finished, this, [requestId, reply, this] {
        reply->deleteLater();
        emitRequestStatus(requestId, reply);
    });
    return requestId;
}

QUuid SoundTouch::setSource(ContentItemObject contentItem)
{
    QUuid requestId = QUuid::createUuid();
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
    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/xml");
    QNetworkReply *reply = m_networkAccessManager->post(request, content);
    connect(reply, &QNetworkReply::finished, this, [requestId, reply, this] {
        reply->deleteLater();
        emitRequestStatus(requestId, reply);
    });
    return requestId;
}

QUuid SoundTouch::setZone(ZoneObject zone)
{
    QUuid requestId = QUuid::createUuid();
    QUrl url;
    url.setHost(m_ipAddress);
    url.setScheme("http");
    url.setPort(m_port);
    url.setPath("/setZone");
    QByteArray content;
    QXmlStreamWriter xml(&content);
    xml.writeStartDocument("1.0");
    xml.writeStartElement("zone");
    xml.writeAttribute("master", zone.deviceID);
    foreach (MemberObject member, zone.members){
        xml.writeTextElement("member", member.deviceID);
        xml.writeAttribute("ipaddress", member.ipAddress);
    }
    xml.writeEndElement(); //zone
    xml.writeEndDocument();
    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/xml");
    QNetworkReply *reply = m_networkAccessManager->post(request, content);
    connect(reply, &QNetworkReply::finished, this, [requestId, reply, this] {
        reply->deleteLater();
        emitRequestStatus(requestId, reply);
    });
    return requestId;
}

QUuid SoundTouch::addZoneSlave(ZoneObject zone)
{
    QUuid requestId = QUuid::createUuid();
    QUrl url;
    url.setHost(m_ipAddress);
    url.setScheme("http");
    url.setPort(m_port);
    url.setPath("/addZoneSlave");
    QByteArray content;
    QXmlStreamWriter xml(&content);
    xml.writeStartDocument("1.0");
    xml.writeStartElement("zone");
    xml.writeAttribute("master", zone.deviceID);
    foreach (MemberObject member, zone.members){
        xml.writeTextElement("member", member.deviceID);
        xml.writeAttribute("ipaddress", member.ipAddress);
    }
    xml.writeEndElement(); //zone
    xml.writeEndDocument();
    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/xml");
    QNetworkReply *reply = m_networkAccessManager->post(request, content);
    connect(reply, &QNetworkReply::finished, this, [requestId, reply, this] {
        reply->deleteLater();
        emitRequestStatus(requestId, reply);
    });
    return requestId;
}

QUuid SoundTouch::removeZoneSlave(ZoneObject zone)
{
    QUuid requestId = QUuid::createUuid();
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
    foreach (MemberObject member, zone.members){
        xml.writeTextElement("member", member.deviceID);
        xml.writeAttribute("ipaddress", member.ipAddress);
    }
    xml.writeEndElement(); //zone
    xml.writeEndDocument();
    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/xml");
    QNetworkReply *reply = m_networkAccessManager->post(request, content);
    connect(reply, &QNetworkReply::finished, this, [requestId, reply, this] {
        reply->deleteLater();
        emitRequestStatus(requestId, reply);
    });
    return requestId;
}

QUuid SoundTouch::setBass(int volume)
{
    QUuid requestId = QUuid::createUuid();
    QUrl url;
    url.setHost(m_ipAddress);
    url.setScheme("http");
    url.setPort(m_port);
    url.setPath("/bass");
    QByteArray content = ("<?xml version=\"1.0\" ?>");
    content.append("<bass>");
    content.append(QByteArray::number(volume));
    content.append("</bass>");
    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/xml");
    QNetworkReply *reply = m_networkAccessManager->post(request, content);
    connect(reply, &QNetworkReply::finished, this, [requestId, reply, this] {
        reply->deleteLater();
        emitRequestStatus(requestId, reply);
    });
    return requestId;
}

QUuid SoundTouch::setName(const QString &name)
{
    QUuid requestId = QUuid::createUuid();
    QUrl url;
    url.setHost(m_ipAddress);
    url.setScheme("http");
    url.setPort(m_port);
    url.setPath("/name");
    QByteArray content = ("<?xml version=\"1.0\" ?>");
    content.append("<name>");
    content.append(name.toUtf8());
    content.append("</name>");
    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/xml");
    QNetworkReply *reply = m_networkAccessManager->post(request, content);
    connect(reply, &QNetworkReply::finished, this, [requestId, reply, this] {
        reply->deleteLater();
        emitRequestStatus(requestId, reply);
    });
    return requestId;
}

QUuid SoundTouch::setSpeaker(PlayInfoObject playInfo)
{
    QUuid requestId = QUuid::createUuid();
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
    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/xml");
    QNetworkReply *reply = m_networkAccessManager->post(request, content);
    connect(reply, &QNetworkReply::finished, this, [requestId, reply, this] {
        reply->deleteLater();
        emitRequestStatus(requestId, reply);
    });
    return requestId;
}

void SoundTouch::onWebsocketConnected()
{
    qCDebug(dcBose()) << "Bose websocket connected";
    emit connectionChanged(true);
}

void SoundTouch::onWebsocketDisconnected()
{
    qCDebug(dcBose()) << "Bose websocket disconnected";
    emit connectionChanged(false);
    QTimer::singleShot(5000, this, [this](){
        QUrl url;
        url.setHost(m_ipAddress);
        url.setScheme("ws");
        url.setPort(8080);
        m_websocket->open(url);
    });
}

void SoundTouch::onWebsocketMessageReceived(QString message)
{
    qCDebug(dcBose()) << "Websocket message received:" << message;
    //TODO as soon as QWebSocket supports sub-protocols
}

QUuid SoundTouch::sendGetRequest(QString path)
{
    QUuid requestId = QUuid::createUuid();
    QUrl url;
    url.setHost(m_ipAddress);
    url.setScheme("http");
    url.setPort(m_port);
    url.setPath(path);
    //qCDebug(dcBose()) << "Sending request" << url;
    QNetworkRequest request = QNetworkRequest(url);

    QNetworkReply *reply = m_networkAccessManager->get(request);
    m_getRepliesPending = true;
    connect(reply, &QNetworkReply::finished, this, [requestId, reply, this] {
        reply->deleteLater();
        int status = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();

        if (m_getRequestQueue.isEmpty()) {
            m_getRepliesPending = false;
        } else {
            sendGetRequest(m_getRequestQueue.takeFirst());
        }
        if (reply->error() != QNetworkReply::NoError) {
            emit requestExecuted(requestId, false);
            emit connectionChanged(false);
            qCWarning(dcBose()) << "Request error" << reply->errorString();
            return;
        }
        emit connectionChanged(true);
        // Check HTTP status code
        if (status != 200) {
            qCWarning(dcBose()) << "Request error:" << reply->errorString() << "request:"  << reply->url().path();
            emit requestExecuted(requestId, false);
            return;
        }
        emit requestExecuted(requestId, true);
        QByteArray data = reply->readAll();
        parseData(requestId, data);
    });
    return requestId;
}

void SoundTouch::emitRequestStatus(const QUuid &requestId, QNetworkReply *reply)
{
    int status = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
    // Check HTTP status code
    if (reply->error() != QNetworkReply::NoError) {
        emit connectionChanged(false);
        emit requestExecuted(requestId, false);
        qCWarning(dcBose()) << "Request error:" << reply->errorString() << "request:"  << reply->url().path();
        return;
    }
    emit connectionChanged(true);
    if (status != 200) {
        emit requestExecuted(requestId, false);
        return;
    }
    QByteArray data = reply->readAll();
    qCDebug(dcBose()) << "Request status" << data;
    QXmlStreamReader xml;
    xml.addData(data);

    if (xml.readNextStartElement()) {
        if (xml.name() == QString("status")) {
            //QString status = xml.readElementText();
            emit requestExecuted(requestId, true);
        } else if (xml.name() == QString("errors")) {
            emit requestExecuted(requestId, false);
            QString deviceId;
            if(xml.attributes().hasAttribute("deviceID")) {
                deviceId = xml.attributes().value("deviceID").toString();
            }
            while(xml.readNextStartElement()){
                if(xml.name() == QString("error")){
                    ErrorObject error;
                    error.deviceId = deviceId;
                    error.error = xml.readElementText();
                    if(xml.attributes().hasAttribute("value")) {
                        error.value = xml.attributes().value("value").toInt();
                    }
                    if(xml.attributes().hasAttribute("name")) {
                        error.name = xml.attributes().value("name").toString();
                    }
                    if(xml.attributes().hasAttribute("severity")) {
                        error.severity = xml.attributes().value("severity").toString();
                    }
                    emit errorReceived(error);
                }
            }
        }
    }
}

void SoundTouch::parseData(const QUuid &requestId, const QByteArray &data)
{
    QXmlStreamReader xml;
    xml.addData(data);

    if (xml.readNextStartElement()) {
        if (xml.name() == QString("info")) {
            InfoObject info;
            if(xml.attributes().hasAttribute("deviceID")) {
                //qCDebug(dcBose()) << "Device ID" << xml.attributes().value("deviceID").toString();
                info.deviceID = xml.attributes().value("deviceID").toString();
            }
            while(xml.readNextStartElement()){
                if(xml.name() == QString("name")){
                    //qCDebug(dcBose()) << "name" << xml.readElementText();
                    info.name =  xml.readElementText();
                } else if(xml.name() == QString("type")){
                    //qCDebug(dcBose()) << "type" << xml.readElementText();
                    info.type =  xml.readElementText();
                } else if(xml.name() == QString("components")){
                    //qCDebug(dcBose()) << "components element";
                    while(xml.readNextStartElement()){
                        if(xml.name() == QString("component")){
                            ComponentObject component;
                            while(xml.readNextStartElement()){
                                if(xml.name() == QString("softwareVersion")){
                                    //qCDebug(dcBose()) << "Software version" << xml.readElementText();
                                    component.softwareVersion = xml.readElementText();
                                } else if(xml.name() == QString("serialNumber")) {
                                    //qCDebug(dcBose()) << "Serialnumber" << xml.readElementText();
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
                } else if(xml.name() == QString("networkInfo")){
                    while (xml.readNextStartElement()) {
                        if (xml.name() == QString("macAddress")) {
                            info.networkInfo.macAddress = xml.readElementText();
                        } else if(xml.name() == QString("ipAddress")) {
                            info.networkInfo.ipAddress = xml.readElementText();
                        } else {
                            xml.skipCurrentElement();
                        }
                    }
                }  else {
                    xml.skipCurrentElement();
                }
            }
            emit infoReceived(requestId, info);
        } else if (xml.name() == QString("nowPlaying")) {
            NowPlayingObject nowPlaying;
            if(xml.attributes().hasAttribute("deviceID")) {
                //qCDebug(dcBose()) << "Device ID" << xml.attributes().value("deviceID").toString();
                nowPlaying.deviceID = xml.attributes().value("deviceID").toString();
            }
            if(xml.attributes().hasAttribute("source")) {
                //qCDebug(dcBose()) << "Source" << xml.attributes().value("source").toString();
                nowPlaying.source = xml.attributes().value("source").toString();
            }
            if(xml.attributes().hasAttribute("sourceAccount")) {
                //qCDebug(dcBose()) << "Source Account" << xml.attributes().value("sourceAccount").toString();
                nowPlaying.sourceAccount = xml.attributes().value("sourceAccount").toString();
            }
            while(xml.readNextStartElement()){
                if (xml.name() == QString("track")) {
                    //qCDebug(dcBose()) << "track" << xml.readElementText();
                    nowPlaying.track = xml.readElementText();
                } else if(xml.name() == QString("artist")) {
                    //qCDebug(dcBose()) << "artist" << xml.readElementText();
                    nowPlaying.artist = xml.readElementText();
                } else if(xml.name() == QString("album")) {
                    //qCDebug(dcBose()) << "album" << xml.readElementText();
                    nowPlaying.album = xml.readElementText();
                } else if(xml.name() == QString("genre")) {
                    //qCDebug(dcBose()) << "genre" << xml.readElementText();
                    nowPlaying.genre = xml.readElementText();
                } else if(xml.name() == QString("rating")) {
                    //qCDebug(dcBose()) << "rating" << xml.readElementText();
                    nowPlaying.rating = xml.readElementText();
                } else if(xml.name() == QString("stationName")) {
                    //qCDebug(dcBose()) << "Station name" << xml.readElementText();
                    nowPlaying.stationName = xml.readElementText();
                } else if(xml.name() == QString("art")) {
                    ArtObject art;
                    if(xml.attributes().hasAttribute("artImageStatus")) {
                        QString artStatus = xml.attributes().value("artImageStatus").toString().toUpper();
                        //ART_STATUS: INVALID, SHOW_DEFAULT_IMAGE, DOWNLOADING, IMAGE_PRESENT
                        //qCDebug(dcBose()) << "Art Image status" << artStatus;
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
                }else if(xml.name() == QString("playStatus")) {
                    QString playStatus = xml.readElementText();
                    //qCDebug(dcBose()) << "Play Status" << playStatus;
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
                } else if(xml.name() == QString("shuffleSetting")) {
                    QString shuffle = xml.readElementText().toUpper();
                    //qCDebug(dcBose()) << "Shuffle Setting" << shuffle;
                    if (shuffle == "SHUFFLE_ON") {
                        nowPlaying.shuffleSetting = SHUFFLE_STATUS_SHUFFLE_ON;
                    } else {
                        nowPlaying.shuffleSetting = SHUFFLE_STATUS_SHUFFLE_OFF;
                    }
                }else if(xml.name() == QString("repeatSetting")) {
                    QString repeat = xml.readElementText().toUpper();
                    //qCDebug(dcBose()) << "Repeat Setting" << repeat;
                    //Modes: REPEAT_OFF, REPEAT_ALL, REPEAT_ONE
                    if (repeat == "REPEAT_OFF") {
                        nowPlaying.repeatSettings = REPEAT_STATUS_REPEAT_OFF;
                    } else if (repeat == "REPEAT_ONE") {
                        nowPlaying.repeatSettings = REPEAT_STATUS_REPEAT_ONE;
                    } else if (repeat == "REPEAT_ALL") {
                        nowPlaying.repeatSettings = REPEAT_STATUS_REPEAT_ALL;
                    }
                } else if(xml.name() == QString("streamType")) {
                    QString streamType = xml.readElementText().toUpper();
                    //qCDebug(dcBose()) << "Stream Type" << streamType;
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
                } else if(xml.name() == QString("stationLocation")) {
                    nowPlaying.stationLocation = xml.readElementText();
                } else {
                    xml.skipCurrentElement();
                }
            }
            emit nowPlayingReceived(requestId, nowPlaying);
        } else if (xml.name() == QString("volume")) {
            VolumeObject volumeObject;
            if(xml.attributes().hasAttribute("deviceID")) {
                //qCDebug(dcBose()) << "Device ID" << xml.attributes().value("deviceID").toString();
                volumeObject.deviceID = xml.attributes().value("deviceID").toString();
            }
            while(xml.readNextStartElement()){
                if(xml.name() == QString("targetvolume")){
                    //qCDebug(dcBose()) << "Target volume" << xml.readElementText();
                    volumeObject.targetVolume = xml.readElementText().toInt();
                }else if(xml.name() == QString("actualvolume")){
                    //qCDebug(dcBose()) << "Actual volume" << xml.readElementText();
                    volumeObject.actualVolume = xml.readElementText().toInt();
                }else if(xml.name() == QString("muteenabled")){
                    //qCDebug(dcBose()) << "Mute enabled" << xml.readElementText();
                    volumeObject.muteEnabled = ( xml.readElementText().toUpper() == "TRUE" ); //TODO convert from "false" to bool
                }else {
                    xml.skipCurrentElement();
                }
            }
            emit volumeReceived(requestId, volumeObject);
        } else if (xml.name() == QString("sources")) {
            SourcesObject sourcesObject;
            if(xml.attributes().hasAttribute("deviceID")) {
                //qCDebug(dcBose()) << "Device ID" << xml.attributes().value("deviceID").toString();
                sourcesObject.deviceId = xml.attributes().value("deviceID").toString();
            }
            while(xml.readNextStartElement()){
                if(xml.name() == QString("sourceItem")){
                    SourceItemObject sourceItem;
                    if(xml.attributes().hasAttribute("source")) {
                        //qCDebug(dcBose()) << "Source" << xml.attributes().value("source").toString();
                        sourceItem.source = xml.attributes().value("source").toString();
                    }
                    if(xml.attributes().hasAttribute("sourceAccount")) {
                        //qCDebug(dcBose()) << "Source Account" << xml.attributes().value("sourceAccount").toString();
                        sourceItem.sourceAccount = xml.attributes().value("sourceAccount").toString();
                    }
                    if(xml.attributes().hasAttribute("status")) {
                        QString status = xml.attributes().value("status").toString().toUpper(); //UNAVAILABLE, READY
                        //qCDebug(dcBose()) << "status" << status;
                        if (status == "READY") {
                            sourceItem.status = SOURCE_STATUS_READY;
                        } else {
                            sourceItem.status = SOURCE_STATUS_UNAVAILABLE;
                        }
                    }
                    if(xml.attributes().hasAttribute("isLocal")) {
                        //qCDebug(dcBose()) << "is Local" << xml.attributes().value("isLocal").toString();
                        sourceItem.isLocal = ( xml.attributes().value("isLocal").toString().toUpper() == "TRUE" );
                    }
                    if(xml.attributes().hasAttribute("multiroomallowed")) {
                        //qCDebug(dcBose) << "multiroom allowed" << xml.attributes().value("multiroomallowed").toString();
                        sourceItem.multiroomallowed = ( xml.attributes().value("multiroomallowed").toString().toUpper() == "TRUE" );
                    }
                    sourceItem.displayName = xml.readElementText();
                    sourcesObject.sourceItems.append(sourceItem);
                }else {
                    xml.skipCurrentElement();
                }
            }
            emit sourcesReceived(requestId, sourcesObject);
        } else if (xml.name() == QString("bass")) {
            BassObject bassObject;
            if(xml.attributes().hasAttribute("deviceID")) {
                //qCDebug(dcBose()) << "Device ID" << xml.attributes().value("deviceID").toString();
                bassObject.deviceID = xml.attributes().value("deviceID").toString();
            }
            while(xml.readNextStartElement()){
                if(xml.name() == QString("targetbass")){
                    //qCDebug(dcBose()) << "Target bas" << xml.readElementText();
                    bassObject.targetBass = xml.readElementText().toInt();
                } else if(xml.name() == QString("actualbass")){
                    //qCDebug(dcBose()) << "Actual bass" << xml.readElementText();
                    bassObject.actualBass = xml.readElementText().toInt();
                } else {
                    xml.skipCurrentElement();
                }
            }
            emit bassReceived(requestId, bassObject);
        } else if (xml.name() == QString("bassCapabilities")) {
            BassCapabilitiesObject bassCapabilities;
            if(xml.attributes().hasAttribute("deviceID")) {
                bassCapabilities.deviceID = xml.attributes().value("deviceID").toString();
            }
            while(xml.readNextStartElement()){
                if(xml.name() == QString("bassAvailable")){
                    //qCDebug(dcBose()) << "BassAvailable" << xml.readElementText();
                    bassCapabilities.bassAvailable = ( xml.readElementText().toUpper() == "TRUE" );
                } else if(xml.name() == QString("bassMin")){
                    //qCDebug(dcBose()) << "bass Min" << xml.readElementText();
                    bassCapabilities.bassMin = xml.readElementText().toInt();
                } else if(xml.name() == QString("bassMax")){
                    //qCDebug(dcBose()) << "bass Max" << xml.readElementText();
                    bassCapabilities.bassMax = xml.readElementText().toInt();
                } else if(xml.name() == QString("bassDefault")){
                    //qCDebug(dcBose()) << "bass default" << xml.readElementText();
                    bassCapabilities.bassDefault = xml.readElementText().toInt();
                }else {
                    xml.skipCurrentElement();
                }
            }
            emit bassCapabilitiesReceived(requestId, bassCapabilities);
        } else if (xml.name() == QString("presets")) {
            QList<PresetObject> presets;
            qCDebug(dcBose()) << "Presets";
            while(xml.readNextStartElement()){
                if(xml.name() == QString("preset")){
                    PresetObject preset;
                    if(xml.attributes().hasAttribute("id")) {
                        preset.presetId = xml.attributes().value("id").toInt();
                    }
                    if(xml.attributes().hasAttribute("createdOn")) {
                        preset.createdOn= xml.attributes().value("createdOn").toULong();
                    }
                    if(xml.attributes().hasAttribute("updatedOn")) {
                        preset.updatedOn = xml.attributes().value("updatedOn").toULong();
                    }
                    qCDebug(dcBose()) << "Preset" << preset.presetId;
                    while(xml.readNextStartElement()){

                        if (xml.name() == QString("ContentItem")) {
                            if(xml.attributes().hasAttribute("source")) {
                                preset.ContentItem.source = xml.attributes().value("source").toString();
                            }
                            if(xml.attributes().hasAttribute("location")) {
                                preset.ContentItem.location = xml.attributes().value("location").toString();
                            }
                            if(xml.attributes().hasAttribute("sourceAccount")) {
                                preset.ContentItem.sourceAccount = xml.attributes().value("sourceAccount").toString();
                            }

                            while(xml.readNextStartElement()){
                                 if (xml.name() == QString("itemName")) {
                                    preset.ContentItem.itemName = xml.readElementText();
                                 } else if (xml.name() == QString("containerArt")){
                                    preset.ContentItem.containerArt = xml.readElementText();
                                } else {
                                     qCWarning(dcBose()) << "Presets: unhandled XML element" << xml.name();
                                     xml.skipCurrentElement();
                                }
                            }

                        } else {
                            qCWarning(dcBose()) << "Presets: unhandled XML element" << xml.name();
                            xml.skipCurrentElement();
                        }
                    }
                    presets.append(preset);
                } else {
                    qCWarning(dcBose()) << "Presets: unhandled XML element" << xml.name();
                    xml.skipCurrentElement();
                }
            }
            emit presetsReceived(requestId, presets);

        } else if (xml.name() == QString("group")) {
            GroupObject group;
            if(xml.attributes().hasAttribute("deviceID")) {
                group.id = xml.attributes().value("id").toString();
            }
            while(xml.readNextStartElement()){
                if(xml.name() == QString("name")) {
                    group.name = xml.readElementText();
                } else if(xml.name() == QString("masterDeviceId")) {
                    group.masterDeviceId = xml.readElementText();
                } else if(xml.name() == QString("roles")) {
                    //group.roles = xml.readElementText().toInt();
                } else if(xml.name() == QString("status")){
                    QString groupStatus = xml.readElementText();
                    //qCDebug(dcBose()) << "Group role" << groupStatus;
                    //group.status = xml.readElementText();
                }else {
                    xml.skipCurrentElement();
                }
            }
            emit groupReceived(requestId, group);
        } else if (xml.name() == QString("zone")) {
            ZoneObject zone;
            if(xml.attributes().hasAttribute("master")) {
                zone.deviceID = xml.attributes().value("master").toString();
            }
            while(xml.readNextStartElement()){
                MemberObject member;
                if(xml.name() == QString("member")) {
                    if(xml.attributes().hasAttribute("ipaddress")) {
                        member.ipAddress = xml.attributes().value("ipaddress").toString();
                    }
                    member.deviceID = xml.readElementText();
                } else {
                    xml.skipCurrentElement();
                }
                zone.members.append(member);
            }
            emit zoneReceived(requestId, zone);
        }
        else {
            xml.skipCurrentElement();
        }
    }
}
