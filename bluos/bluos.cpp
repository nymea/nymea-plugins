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

#include "bluos.h"
#include "extern-plugininfo.h"

#include <QNetworkRequest>
#include <QNetworkReply>
#include <QUrlQuery>
#include <QXmlStreamReader>

BluOS::BluOS(NetworkAccessManager *networkmanager,  QHostAddress hostAddress, int port,  QObject *parent) :
    QObject(parent),
    m_hostAddress(hostAddress),
    m_port(port),
    m_networkManager(networkmanager)
{

}

int BluOS::port()
{
    return m_port;
}

QHostAddress BluOS::hostAddress()
{
    return m_hostAddress;
}

void BluOS::getStatus()
{
    QUrl url;
    url.setScheme("http");
    url.setHost(m_hostAddress.toString());
    url.setPort(m_port);
    url.setPath("/Status");
    QNetworkReply *reply = m_networkManager->get(QNetworkRequest(url));
    connect(reply, &QNetworkReply::finished, reply, &QNetworkReply::deleteLater);
    connect(reply, &QNetworkReply::finished, this, [reply, this] {
        int status = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
        // Check HTTP status code
        if (status != 200 || reply->error() != QNetworkReply::NoError) {
            if (reply->error() == QNetworkReply::HostNotFoundError) {
                emit connectionChanged(false);
            }
            qCWarning(dcBluOS()) << "Request error:" << status << reply->errorString();
            return;
        }
        emit connectionChanged(true);
        QByteArray data = reply->readAll();
        //qCDebug(dcBluOS()) << "Get Status:" << data;
        parseState(data);
    });
    return;
}

QUuid BluOS::setVolume(uint volume)
{
    QUuid requestId = QUuid::createUuid();

    QUrlQuery query;
    query.addQueryItem("level", QString::number(volume));
    query.addQueryItem("tell_slaves", "off");

    QUrl url;
    url.setScheme("http");
    url.setHost(m_hostAddress.toString());
    url.setPort(m_port);
    url.setPath("/Volume");
    url.setQuery(query);
    QNetworkReply *reply = m_networkManager->get(QNetworkRequest(url));
    connect(reply, &QNetworkReply::finished, reply, &QNetworkReply::deleteLater);
    connect(reply, &QNetworkReply::finished, this, [requestId, reply, this] {

        int status = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();

        // Check HTTP status code
        if (status != 200 || reply->error() != QNetworkReply::NoError) {
            if (reply->error() == QNetworkReply::HostNotFoundError) {
                emit connectionChanged(false);
            }
            emit actionExecuted(requestId, false);
            qCWarning(dcBluOS()) << "Request error:" << status << reply->errorString();
            return;
        }
        emit connectionChanged(true);

        QXmlStreamReader xml;
        xml.addData(reply->readAll());
        if (xml.hasError()) {
            qCDebug(dcBluOS()) << "XML Error:" << xml.errorString();
        }
        int volume = 0;
        bool mute = false;
        if (xml.readNextStartElement()) {
            if (xml.name() == "volume") {
                if(xml.attributes().hasAttribute("mute")) {
                    mute = xml.attributes().value("mute").toInt();
                }
                volume = xml.readElementText().toInt();
            }
        }
        emit volumeReceived(volume, mute);
        emit actionExecuted(requestId, true);
    });
    return requestId;
}

QUuid BluOS::setMute(bool mute)
{
    QUuid requestId = QUuid::createUuid();

    QUrlQuery query;
    query.addQueryItem("mute", QString::number(mute));
    query.addQueryItem("tell_slaves", "off");

    QUrl url;
    url.setScheme("http");
    url.setHost(m_hostAddress.toString());
    url.setPort(m_port);
    url.setPath("/Volume");
    url.setQuery(query);

    QNetworkReply *reply = m_networkManager->get(QNetworkRequest(url));
    connect(reply, &QNetworkReply::finished, reply, &QNetworkReply::deleteLater);
    connect(reply, &QNetworkReply::finished, this, [requestId, reply, this] {

        int status = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();

        // Check HTTP status code
        if (status != 200 || reply->error() != QNetworkReply::NoError) {
            if (reply->error() == QNetworkReply::HostNotFoundError) {
                emit connectionChanged(false);
            }
            emit actionExecuted(requestId, false);
            qCWarning(dcBluOS()) << "Request error:" << status << reply->errorString();
            return;
        }
        emit connectionChanged(true);
        emit actionExecuted(requestId, true);
    });

    return requestId;
}

QUuid BluOS::play()
{
    return playBackControl(PlaybackCommand::Play);
}

QUuid BluOS::pause()
{
    return playBackControl(PlaybackCommand::Pause);
}

QUuid BluOS::stop()
{
    return playBackControl(PlaybackCommand::Stop);
}

QUuid BluOS::back()
{
    return playBackControl(PlaybackCommand::Back);
}

QUuid BluOS::setShuffle(bool shuffle)
{
    Q_UNUSED(shuffle)
    QUuid requestId = QUuid::createUuid();

    QUrl url;
    url.setScheme("http");
    url.setHost(m_hostAddress.toString());
    url.setPort(m_port);
    url.setPath("/Shuffle");
    QUrlQuery query;
    query.addQueryItem("state", QString::number(shuffle));
    url.setQuery(query);
    QNetworkReply *reply = m_networkManager->get(QNetworkRequest(url));
    connect(reply, &QNetworkReply::finished, reply, &QNetworkReply::deleteLater);
    connect(reply, &QNetworkReply::finished, this, [requestId, reply, this] {

        int status = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();

        // Check HTTP status code
        if (status != 200 || reply->error() != QNetworkReply::NoError) {
            if (reply->error() == QNetworkReply::HostNotFoundError) {
                emit connectionChanged(false);
            }
            emit actionExecuted(requestId, false);
            qCWarning(dcBluOS()) << "Request error:" << status << reply->errorString();
            return;
        }
        emit connectionChanged(true);
        emit actionExecuted(requestId, true);

        QXmlStreamReader xml;
        xml.addData(reply->readAll());
        if (xml.hasError()) {
            qCDebug(dcBluOS()) << "XML Error:" << xml.errorString();
            return;
        }
        if (xml.readNextStartElement()) {
            if (xml.attributes().hasAttribute("shuffle")) {
                bool shuffle = RepeatMode(xml.attributes().value("shuffle").toInt());
                emit shuffleStateReceived(shuffle);
            }
        }
    });
    return requestId;
}

QUuid BluOS::setRepeat(RepeatMode repeatMode)
{
    Q_UNUSED(repeatMode)
    QUuid requestId = QUuid::createUuid();

    QUrl url;
    url.setScheme("http");
    url.setHost(m_hostAddress.toString());
    url.setPort(m_port);
    url.setPath("/Repeat");
    QUrlQuery query;
    query.addQueryItem("state", QString::number(repeatMode));
    url.setQuery(query);
    QNetworkReply *reply = m_networkManager->get(QNetworkRequest(url));
    connect(reply, &QNetworkReply::finished, reply, &QNetworkReply::deleteLater);
    connect(reply, &QNetworkReply::finished, this, [requestId, reply, this] {

        int status = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();

        // Check HTTP status code
        if (status != 200 || reply->error() != QNetworkReply::NoError) {
            if (reply->error() == QNetworkReply::HostNotFoundError) {
                emit connectionChanged(false);

            }
            emit actionExecuted(requestId, false);
            qCWarning(dcBluOS()) << "Request error:" << status << reply->errorString();
            return;
        }
        emit connectionChanged(true);
        emit actionExecuted(requestId, true);

        QXmlStreamReader xml;
        xml.addData(reply->readAll());
        if (xml.hasError()) {
            qCDebug(dcBluOS()) << "XML Error:" << xml.errorString();
            return;
        }
        if (xml.readNextStartElement()) {
            if (xml.name() == "playlist") {
                if (xml.attributes().hasAttribute("repeat")) {
                    RepeatMode mode = RepeatMode(xml.attributes().value("repeat").toInt());
                    emit repeatModeReceived(mode);
                }
            }
        }
    });
    return requestId;
}

QUuid BluOS::listPresets()
{
    QUuid requestId = QUuid::createUuid();

    QUrl url;
    url.setScheme("http");
    url.setHost(m_hostAddress.toString());
    url.setPort(m_port);
    url.setPath("/Presets");
    QNetworkReply *reply = m_networkManager->get(QNetworkRequest(url));
    connect(reply, &QNetworkReply::finished, reply, &QNetworkReply::deleteLater);
    connect(reply, &QNetworkReply::finished, this, [requestId, reply, this] {

        int status = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();

        // Check HTTP status code
        if (status != 200 || reply->error() != QNetworkReply::NoError) {
            if (reply->error() == QNetworkReply::HostNotFoundError) {
                emit connectionChanged(false);
            }

            qCWarning(dcBluOS()) << "Request error:" << status << reply->errorString();
            return;
        }
        emit connectionChanged(true);

        QByteArray data = reply->readAll();
        QXmlStreamReader xml;
        xml.addData(data);
        if (xml.hasError()) {
            qCDebug(dcBluOS()) << "XML Error:" << xml.errorString();
            return;
        }
        QList<Preset> presetList;
        if (xml.readNextStartElement()) {
            if (xml.name() == "presets") {
                while(xml.readNextStartElement()){
                    if(xml.name() == "preset"){
                        Preset preset;
                        if (xml.attributes().hasAttribute("id")) {
                            preset.Id = xml.attributes().value("id").toInt();
                        }
                        if (xml.attributes().hasAttribute("name")) {
                            preset.Name = xml.attributes().value("name").toString();
                        }
                        if (xml.attributes().hasAttribute("url")) {
                            preset.Url = xml.attributes().value("url").toString();
                        }
                        qCDebug(dcBluOS()) << "Preset text" << xml.readElementText(); //apparently the text must be read so the xml parser recognises the next element
                        presetList.append(preset);
                    } else {
                        xml.skipCurrentElement();
                    }
                }
            }
        }
        emit presetsReceived(requestId, presetList);
    });
    return requestId;
}

QUuid BluOS::loadPreset(int preset)
{
    QUuid requestId = QUuid::createUuid();

    QUrl url;
    url.setScheme("http");
    url.setHost(m_hostAddress.toString());
    url.setPort(m_port);
    url.setPath("/Preset");
    QUrlQuery query;
    query.addQueryItem("id", QString::number(preset));
    url.setQuery(query);
    qCDebug(dcBluOS()) << "Loading preset" << url.toString();
    QNetworkReply *reply = m_networkManager->get(QNetworkRequest(url));
    connect(reply, &QNetworkReply::finished, reply, &QNetworkReply::deleteLater);
    connect(reply, &QNetworkReply::finished, this, [requestId, reply, this] {

        int status = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();

        // Check HTTP status code
        if (status != 200 || reply->error() != QNetworkReply::NoError) {
            if (reply->error() == QNetworkReply::HostNotFoundError) {
                emit connectionChanged(false);
            }
            emit actionExecuted(requestId, false);
            qCWarning(dcBluOS()) << "Request error:" << status << reply->errorString();
            return;
        }
        emit connectionChanged(true);
        emit actionExecuted(requestId, true);
    });
    return requestId;
}

QUuid BluOS::getSources()
{
    QUuid requestId = QUuid::createUuid();

    QUrl url;
    url.setScheme("http");
    url.setHost(m_hostAddress.toString());
    url.setPort(m_port);
    url.setPath("/Browse");
    QNetworkReply *reply = m_networkManager->get(QNetworkRequest(url));
    connect(reply, &QNetworkReply::finished, reply, &QNetworkReply::deleteLater);
    connect(reply, &QNetworkReply::finished, this, [requestId, reply, this] {

        int status = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();

        // Check HTTP status code
        if (status != 200 || reply->error() != QNetworkReply::NoError) {
            if (reply->error() == QNetworkReply::HostNotFoundError) {
                emit connectionChanged(false);
            }
            qCWarning(dcBluOS()) << "Request error:" << status << reply->errorString();
            return;
        }
        emit connectionChanged(true);
        QByteArray data = reply->readAll();
        qCDebug(dcBluOS()) << "Sources: " << data;
        QXmlStreamReader xml;
        xml.addData(data);
        if (xml.hasError()) {
            qCDebug(dcBluOS()) << "XML Error:" << xml.errorString();
            return;
        }
        QList<Source> sourceList;
        if (xml.readNextStartElement()) {
            if (xml.name() == "browse") {
                while(xml.readNextStartElement()){
                    if(xml.name() == "item"){
                        Source source;
                        if (xml.attributes().hasAttribute("text")) {
                            source.Text = xml.attributes().value("text").toString();
                        }
                        if (xml.attributes().hasAttribute("type")) {
                            source.Type = xml.attributes().value("type").toString();
                        }
                        if (xml.attributes().hasAttribute("browseKey")) {
                            source.BrowseKey = xml.attributes().value("browseKey").toString();
                        }
                        if (xml.attributes().hasAttribute("image")) {
                            source.Image = xml.attributes().value("image").toString();
                        }
                        qCDebug(dcBluOS()) << "Source text" << xml.readElementText();
                        sourceList.append(source);
                    } else {
                        xml.skipCurrentElement();
                    }
                }
            }
        }
        emit sourcesReceived(requestId, sourceList);
    });
    return requestId;
}

QUuid BluOS::browseSource(const QString &key)
{
    QUuid requestId = QUuid::createUuid();

    QUrl url;
    url.setScheme("http");
    url.setHost(m_hostAddress.toString());
    url.setPort(m_port);
    url.setPath("/Browse");
    QUrlQuery query;
    query.addQueryItem("key", key);
    url.setQuery(query);
    QNetworkReply *reply = m_networkManager->get(QNetworkRequest(url));
    connect(reply, &QNetworkReply::finished, reply, &QNetworkReply::deleteLater);
    connect(reply, &QNetworkReply::finished, this, [requestId, reply, this] {

        int status = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();

        // Check HTTP status code
        if (status != 200 || reply->error() != QNetworkReply::NoError) {
            if (reply->error() == QNetworkReply::HostNotFoundError) {
                emit connectionChanged(false);
            }
            qCWarning(dcBluOS()) << "Request error:" << status << reply->errorString();
            return;
        }
        emit connectionChanged(true);
        QByteArray data = reply->readAll();
        qCDebug(dcBluOS()) << "Browse result: " << data;
        QXmlStreamReader xml;
        xml.addData(data);
        if (xml.hasError()) {
            qCDebug(dcBluOS()) << "XML Error:" << xml.errorString();
            return;
        }
        QList<Source> sourceList;
        if (xml.readNextStartElement()) {
            if (xml.name() == "browse") {
                while(xml.readNextStartElement()){
                    if(xml.name() == "item"){
                        Source source;
                        if (xml.attributes().hasAttribute("text")) {
                            source.Text = xml.attributes().value("text").toString();
                        }
                        if (xml.attributes().hasAttribute("type")) {
                            source.Type = xml.attributes().value("type").toString();
                        }
                        if (xml.attributes().hasAttribute("browseKey")) {
                            source.BrowseKey = xml.attributes().value("browseKey").toString();
                        }
                        if (xml.attributes().hasAttribute("image")) {
                            source.Image = xml.attributes().value("image").toString();
                        }
                        qCDebug(dcBluOS()) << "Source text" << xml.readElementText();
                        sourceList.append(source);
                    } else {
                        xml.skipCurrentElement();
                    }
                }
            }
        }
        emit sourcesReceived(requestId, sourceList);
    });
    return requestId;
}

QUuid BluOS::addGroupPlayer(QHostAddress address, int port)
{
    Q_UNUSED(address)
    Q_UNUSED(port)
    QUuid requestId = QUuid::createUuid();

    QUrl url;
    url.setScheme("http");
    url.setHost(m_hostAddress.toString());
    url.setPort(m_port);
    url.setPath("/AddSlave");
    QUrlQuery query;
    query.addQueryItem("slave", address.toString());
    query.addQueryItem("port", QString::number(port));
    url.setQuery(query);
    QNetworkReply *reply = m_networkManager->get(QNetworkRequest(url));
    connect(reply, &QNetworkReply::finished, reply, &QNetworkReply::deleteLater);
    connect(reply, &QNetworkReply::finished, this, [reply, this] {

        int status = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();

        // Check HTTP status code
        if (status != 200 || reply->error() != QNetworkReply::NoError) {
            if (reply->error() == QNetworkReply::HostNotFoundError) {
                emit connectionChanged(false);
            }

            qCWarning(dcBluOS()) << "Request error:" << status << reply->errorString();
            return;
        }
        emit connectionChanged(true);
    });
    return requestId;
}

QUuid BluOS::removeGroupPlayer(QHostAddress address, int port)
{
    Q_UNUSED(address)
    Q_UNUSED(port)
    QUuid requestId = QUuid::createUuid();

    QUrl url;
    url.setScheme("http");
    url.setHost(m_hostAddress.toString());
    url.setPort(m_port);
    url.setPath("/RemoveSlave");
    QUrlQuery query;
    query.addQueryItem("slave", address.toString());
    query.addQueryItem("port", QString::number(port));
    url.setQuery(query);
    QNetworkReply *reply = m_networkManager->get(QNetworkRequest(url));
    connect(reply, &QNetworkReply::finished, reply, &QNetworkReply::deleteLater);
    connect(reply, &QNetworkReply::finished, this, [reply, this] {

        int status = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();

        // Check HTTP status code
        if (status != 200 || reply->error() != QNetworkReply::NoError) {
            if (reply->error() == QNetworkReply::HostNotFoundError) {
                emit connectionChanged(false);
            }

            qCWarning(dcBluOS()) << "Request error:" << status << reply->errorString();
            return;
        }
        emit connectionChanged(true);
    });
    return requestId;
}

QUuid BluOS::skip()
{
    return playBackControl(PlaybackCommand::Skip);
}

QUuid BluOS::playBackControl(BluOS::PlaybackCommand command)
{
    QUuid requestId = QUuid::createUuid();
    QUrl url;
    url.setScheme("http");
    url.setHost(m_hostAddress.toString());
    url.setPort(m_port);
    switch (command) {
    case PlaybackCommand::Play:
        url.setPath("/Play");
        break;
    case PlaybackCommand::Pause:
        url.setPath("/Pause");
        break;
    case PlaybackCommand::Stop:
        url.setPath("/Stop");
        break;
    case PlaybackCommand::Back:
        url.setPath("/Back");
        break;
    case PlaybackCommand::Skip:
        url.setPath("/Skip");
        break;
    }
    QNetworkRequest request;
    request.setUrl(url);
    QNetworkReply *reply = m_networkManager->get(request);
    qCDebug(dcBluOS()) << "Sending request" << request.url();
    connect(reply, &QNetworkReply::finished, reply, &QNetworkReply::deleteLater);
    connect(reply, &QNetworkReply::finished, this, [requestId, reply, this] {

        int status = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();

        // Check HTTP status code
        if (status != 200 || reply->error() != QNetworkReply::NoError) {
            if (reply->error() == QNetworkReply::HostNotFoundError) {
                emit connectionChanged(false);
            }
            emit actionExecuted(requestId, false);
            qCWarning(dcBluOS()) << "Request error:" << status << reply->errorString();
            return;
        }
        emit connectionChanged(true);

        QByteArray data = reply->readAll();
        parseState(data);
    });
    return requestId;
}

bool BluOS::parseState(const QByteArray &state)
{
    QXmlStreamReader xml;
    xml.addData(state);
    if (xml.hasError()) {
        qCDebug(dcBluOS()) << "XML Error:" << xml.errorString();
        return false;
    }

    StatusResponse statusResponse;
    if (xml.readNextStartElement()) {
        if (xml.name() == "status") {
            while(xml.readNextStartElement()){
                if(xml.name() == "artist"){
                    statusResponse.Artist = xml.readElementText();
                } else if(xml.name() == "album"){
                    statusResponse.Album = xml.readElementText();
                } else if(xml.name() == "name"){
                    statusResponse.Name = xml.readElementText();
                } else if(xml.name() == "service"){
                    statusResponse.Service = xml.readElementText();
                } else if(xml.name() == "serviceIcon"){
                    statusResponse.ServiceIcon = xml.readElementText();
                } else if(xml.name() == "shuffle"){
                    statusResponse.Shuffle = xml.readElementText().toInt();
                } else if(xml.name() == "repeat"){
                    statusResponse.Shuffle = xml.readElementText().toInt();
                } else if(xml.name() == "state"){
                    QString playback = xml.readElementText();
                    if (playback == "play") {
                        statusResponse.State = PlaybackState::Playing;
                    } else if (playback == "pause") {
                        statusResponse.State = PlaybackState::Paused;
                    } else if (playback == "stop") {
                        statusResponse.State = PlaybackState::Stopped;
                    } else if (playback == "connecting") {
                        statusResponse.State = PlaybackState::Connecting;
                    } else if (playback == "stream") {
                        statusResponse.State = PlaybackState::Streaming;
                    }  else {
                        statusResponse.State = PlaybackState::Stopped;
                        qCWarning(dcBluOS()) << "State response, unhandled playback mode" << playback;
                    }
                } else if(xml.name() == "volume"){
                    statusResponse.Volume = xml.readElementText().toInt();
                } else if(xml.name() == "mute"){
                    statusResponse.Mute = xml.readElementText().toInt();
                } else if(xml.name() == "image") {
                    statusResponse.Image = xml.readElementText();
                } else if(xml.name() == "title1") {
                    statusResponse.Title = xml.readElementText();
                } else if(xml.name() == "group") {
                    statusResponse.Group = xml.readElementText();
                } else {
                    xml.skipCurrentElement();
                }
            }
        }
    }
    emit statusReceived(statusResponse);
    return true;
}
