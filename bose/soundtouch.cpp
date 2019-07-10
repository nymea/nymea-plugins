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
    m_websocket->open(url);
}

void SoundTouch::getInfo()
{
    QUrl url;
    url.setHost(m_ipAddress);
    url.setScheme("http");
    url.setPort(m_port);
    url.setPath("/info");
    qDebug(dcBose) << "Sending request" << url;
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
    qDebug(dcBose) << "Sending request" << url;
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
    qDebug(dcBose) << "Sending request" << url;
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
    qDebug(dcBose) << "Sending request" << url;
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

}

void SoundTouch::getZone()
{

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
        qDebug(dcBose) << "Sending request" << url << content;
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
    qDebug(dcBose) << "REST message received:" << data;
    int status = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();

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
                    qDebug(dcBose) << "name" << xml.readElementText();
                    info.name =  xml.readElementText();
                } else if(xml.name() == "type"){
                    qDebug(dcBose) << "type" << xml.readElementText();
                    info.type =  xml.readElementText();
                } else if(xml.name() == "components"){
                    qDebug(dcBose) << "components element";
                    while(xml.readNextStartElement()){
                        if(xml.name() == "component"){
                            while(xml.readNextStartElement()){
                                if(xml.name() == "softwareVersion"){
                                    qDebug(dcBose) << "Software version" << xml.readElementText();
                                } else if(xml.name() == "serialNumber") {
                                    qDebug(dcBose) << "Serialnumber" << xml.readElementText();
                                } else {
                                    xml.skipCurrentElement();
                                }
                            }
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
        } else if (xml.name() == "now_playing") {
            NowPlayingObject nowPlaying;

            emit
        }
    }

    /*while (!xml.atEnd()) {
                      if (xml.readNext()) {
                          if (xml.tokenType() == TokenType::)
                          qDebug(dcBose) << "element" << xml.tokenString() << xml.readElementText();
                      }

                      if (xml.hasError()) {
                          qCWarning(dcBose()) << "Error when parsing XML response";
                      }
                  }*/


    //}

}


void SoundTouch::onWebsocketMessageReceived(QString message)
{
    qDebug(dcBose) << "Websocket message received:" << message;
}
