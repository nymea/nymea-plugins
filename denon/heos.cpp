/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *                                                                         *
 *  Copyright (C) 2019 Bernhard Trinnes <bernhard.trinnes@guh.io>          *
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

#include "heos.h"
#include "extern-plugininfo.h"
#include <QJsonArray>
#include <QJsonObject>
#include <QJsonDocument>
#include <QUrlQuery>

Heos::Heos(const QHostAddress &hostAddress, QObject *parent) :
    QObject(parent),
    m_hostAddress(hostAddress)
{
    m_socket = new QTcpSocket(this);

    connect(m_socket, &QTcpSocket::connected, this, &Heos::onConnected);
    connect(m_socket, &QTcpSocket::disconnected, this, &Heos::onDisconnected);
    connect(m_socket, &QTcpSocket::readyRead, this, &Heos::readData);
    connect(m_socket, SIGNAL(error(QAbstractSocket::SocketError)), this, SLOT(onError(QAbstractSocket::SocketError)));
}


void Heos::getPlayers()
{
    QByteArray cmd = "heos://player/get_players\r\n";
    m_socket->write(cmd);
}

void Heos::registerForChangeEvents(bool state)
{
    QByteArray query;

    if (state) {
        query = "?enable=on";
    } else {
        query = "?enable=off";
    }
    QByteArray cmd = "heos://system/register_for_change_events" + query + "\r\n";
    qCDebug(dcDenon) << "Register for change events:" << cmd;
    m_socket->write(cmd);
}

void Heos::sendHeartbeat()
{
    QByteArray cmd = "heos://system/heart_beat\r\n";
    m_socket->write(cmd);
}

void Heos::getVolume(int playerId)
{
    QByteArray cmd = "heos://player/get_volume?pid=" + QVariant(playerId).toByteArray() + "\r\n";
    m_socket->write(cmd);
}

void Heos::setVolume(int playerId, int volume)
{
    QByteArray cmd = "heos://player/set_volume?pid=" + QVariant(playerId).toByteArray() + "&level=" + QVariant(volume).toByteArray() + "\r\n";
    qCDebug(dcDenon) << "Set volume:" << cmd;
    m_socket->write(cmd);
}

void Heos::getMute(int playerId)
{
    QByteArray cmd = "heos://player/get_mute?pid=" + QVariant(playerId).toByteArray() + "\r\n";
    m_socket->write(cmd);
}

void Heos::setMute(int playerId, bool state)
{
    QByteArray stateQuery;
    if(state) {
        stateQuery = "&state=on";
    } else {
        stateQuery = "&state=off";
    }
    QByteArray cmd = "heos://player/set_mute?pid=" + QVariant(playerId).toByteArray() + stateQuery + "\r\n";
    qCDebug(dcDenon) << "Set mute:" << cmd;
    m_socket->write(cmd);
}

void Heos::setPlayerState(int playerId, HeosPlayerState state)
{
    QByteArray playerStateQuery;

    if (state == HeosPlayerState::Play){
        playerStateQuery = "&state=play";
    } else if (state == HeosPlayerState::Pause){
        playerStateQuery = "&state=pause";
    } else if (state == HeosPlayerState::Stop){
        playerStateQuery = "&state=stop";
    }

    QByteArray cmd = "heos://player/set_play_state?pid=" + QVariant(playerId).toByteArray() + playerStateQuery + "\r\n";
    qCDebug(dcDenon) << "Set play mode:" << cmd;
    m_socket->write(cmd);
}

void Heos::getPlayerState(int playerId)
{
    QByteArray cmd = "heos://player/get_play_state?pid=" + QVariant(playerId).toByteArray() + "\r\n";
    m_socket->write(cmd);
}


void Heos::setPlayMode(int playerId, HeosRepeatMode repeatMode, bool shuffle)
{
    QByteArray repeatModeQuery;

    if (repeatMode == HeosRepeatMode::Off) {
        repeatModeQuery = "&repeat=off";
    } else if (repeatMode == HeosRepeatMode::One) {
        repeatModeQuery = "&repeat=on_one";
    } else if (repeatMode == HeosRepeatMode::All) {
        repeatModeQuery = "&repeat=on_all";
    }

    QByteArray shuffleQuery;
    if (shuffle) {
        shuffleQuery = "&shuffle=on";
    } else {
        shuffleQuery = "&shuffle=off";
    }

    QByteArray cmd = "heos://player/set_play_mode?pid=" + QVariant(playerId).toByteArray() + repeatModeQuery + shuffleQuery + "\r\n";
    qCDebug(dcDenon) << "Set play mode:" << cmd;
    m_socket->write(cmd);
}

void Heos::getPlayMode(int playerId)
{
    QByteArray cmd = "heos://player/get_play_mode?pid=" + QVariant(playerId).toByteArray() + "\r\n";
    m_socket->write(cmd);
}

void Heos::playNext(int playerId)
{
    QByteArray cmd = "heos://player/play_next?pid=" + QVariant(playerId).toByteArray() + "\r\n";
    qCDebug(dcDenon) << "Play next:" << cmd;
    m_socket->write(cmd);
}

void Heos::playPrevious(int playerId)
{
    QByteArray cmd = "heos://player/play_previous?pid=" + QVariant(playerId).toByteArray() + "\r\n";
    qCDebug(dcDenon) << "Play previous:" << cmd;
    m_socket->write(cmd);
}

void Heos::getNowPlayingMedia(int playerId)
{
    QByteArray cmd = "heos://player/get_now_playing_media?pid=" + QVariant(playerId).toByteArray() + "\r\n";
    m_socket->write(cmd);
}

Heos::~Heos()
{
    m_socket->close();
}

bool Heos::connected()
{
    return m_connected;
}

void Heos::connectHeos()
{
    if (m_socket->state() == QAbstractSocket::ConnectingState) {
        return;
    }
    m_socket->connectToHost(m_hostAddress, 1255);
}

HeosPlayer *Heos::getPlayer(int playerId)
{
    return m_heosPlayers.value(playerId);
}

void Heos::onConnected()
{
    qCDebug(dcDenon()) << "connected successfully to" << m_hostAddress.toString();
    setConnected(true);
}

void Heos::onDisconnected()
{
    qCDebug(dcDenon()) << "disconnected from" << m_hostAddress.toString();
    setConnected(false);
}

void Heos::onError(QAbstractSocket::SocketError socketError)
{
    qCWarning(dcDenon) << "socket error:" << socketError << m_socket->errorString();
}

void Heos::readData()
{
    int playerId;
    QByteArray data;
    QJsonParseError error;

    while (m_socket->canReadLine()) {
        data = m_socket->readLine();
        //qDebug(dcDenon) << data;
        QJsonDocument jsonDoc = QJsonDocument::fromJson(data, &error);
        if (error.error != QJsonParseError::NoError) {
            qCWarning(dcDenon) << "failed to parse json :" << error.errorString();
            return;
        }

        QVariantMap dataMap = jsonDoc.toVariant().toMap();
        if (dataMap.contains("heos")) {
            QString command = dataMap.value("heos").toMap().value("command").toString();
            if (command.contains("register_for_change_events")) {
                QString enabled = dataMap.value("heos").toMap().value("message").toString();
                if (enabled.contains("off")) {
                    qDebug(dcDenon) << "Events are disabled";
                    m_eventRegistered = false;
                } else {
                    qDebug(dcDenon) << "Events are enabled";
                    m_eventRegistered = true;
                }

            } else if (command.contains("get_players")) {
                QVariantList payloadVariantList = jsonDoc.toVariant().toMap().value("payload").toList();

                foreach (const QVariant &payloadEntryVariant, payloadVariantList) {
                    playerId = payloadEntryVariant.toMap().value("pid").toInt();
                    if(!m_heosPlayers.contains(playerId)){
                        QString serialNumber = payloadEntryVariant.toMap().value("serial").toString();
                        QString name = payloadEntryVariant.toMap().value("name").toString();
                        HeosPlayer *heosPlayer = new HeosPlayer(playerId, name, serialNumber, this);
                        m_heosPlayers.insert(playerId, heosPlayer);
                        emit playerDiscovered(heosPlayer);
                    }
                }

            } else {
                QUrlQuery message(dataMap.value("heos").toMap().value("message").toString());
                if (message.hasQueryItem("pid")) {
                    playerId = message.queryItemValue("pid").toInt();
                }

                if (command.contains("get_player_info")) {
                    //update heos player info
                }

                if (command.contains("get_now_playing_media")) {

                    QString artist = dataMap.value("payload").toMap().value("artist").toString();
                    QString song = dataMap.value("payload").toMap().value("song").toString();
                    QString artwork = dataMap.value("payload").toMap().value("image_url").toString();
                    QString album = dataMap.value("payload").toMap().value("album").toString();
                    QString source;
                    switch (dataMap.value("payload").toMap().value("sid").toInt()) {
                    case 1:
                        source = "Pandora";
                        break;
                    case 2:
                        source = "Rhapsody";
                        break;
                    case 3:
                        source = "TuneIn";
                        break;
                    case 4:
                        source = "Spotify";
                        break;
                    case 5:
                        source = "Deezer";
                        break;
                    case 6:
                        source = "Napster";
                        break;
                    case 7:
                        source = "iHeartRadio";
                        break;
                    case 8:
                        source = "Sirius XM";
                        break;
                    case 9:
                        source = "Soundcloud";
                        break;
                    case 10:
                        source = "Tidal";
                        break;
                    case 11:
                        source = "Unknown";
                        break;
                    case 12:
                        source = "Rdio";
                        break;
                    case 13:
                        source = "Amazon Music";
                        break;
                    case 14:
                        source = "Unknown";
                        break;
                    case 15:
                        source = "Moodmix";
                        break;
                    case 16:
                        source = "Juke";
                        break;
                    case 17:
                        source = "Unkown";
                        break;
                    case 18:
                        source = "QQMusic";
                        break;
                    case 1024:
                        source = "USB Media/DLNA Servers";
                        break;
                    case 1025:
                        source = "HEOS Playlists";
                        break;
                    case 1026:
                        source = "HEOS History";
                        break;
                    case 1027:
                        source = "HEOS aux inputs";
                        break;
                    case 1028:
                        source = "HEOS aux inputs";
                        break;
                    default:
                        source = "Unknown";
                    };
                    emit nowPlayingMediaStatusReceived(playerId, source, artist, album, song, artwork);
                }

                if (command.contains("get_play_state") || command.contains("set_play_state")) {
                    if (message.hasQueryItem("state")) {
                        HeosPlayerState playState = HeosPlayerState::Stop;
                        if (message.queryItemValue("state").contains("play")) {
                            playState = HeosPlayerState::Play;
                        } else if (message.queryItemValue("state").contains("pause")) {
                            playState = HeosPlayerState::Pause;
                        } else if (message.queryItemValue("state").contains("stop")) {
                            playState = HeosPlayerState::Stop;
                        }
                        emit playStateReceived(playerId, playState);
                    }
                }

                if (command.contains("get_volume") || command.contains("set_volume")) {
                    if (message.hasQueryItem("level")) {
                        int volume = message.queryItemValue("level").toInt();
                        emit volumeStatusReceived(playerId, volume);
                    }
                }

                if (command.contains("get_mute") || command.contains("set_mute")) {
                    if (message.hasQueryItem("state")) {
                        QString state = message.queryItemValue("state");
                        if (state.contains("on")) {
                            emit muteStatusReceived(playerId, true);
                        } else {
                            emit muteStatusReceived(playerId, false);
                        }
                    }
                }

                if (command.contains("get_play_mode") || command.contains("set_play_mode")) {
                    if (message.hasQueryItem("shuffle") && message.hasQueryItem("repeat")) {
                        bool shuffle;
                        if (message.queryItemValue("shuffle").contains("on")){
                            shuffle = true;
                        } else {
                            shuffle = false;
                        }
                        emit shuffleModeReceived(playerId, shuffle);

                        HeosRepeatMode repeatMode = HeosRepeatMode::Off;
                        if (message.queryItemValue("repeat").contains("on_all")){
                            repeatMode = HeosRepeatMode::All;
                        } else if (message.queryItemValue("repeat").contains("on_one")){
                            repeatMode = HeosRepeatMode::One;
                        } else  if (message.queryItemValue("repeat").contains("off")){
                            repeatMode = HeosRepeatMode::Off;
                        }
                        emit repeatModeReceived(playerId, repeatMode);
                    }
                }

                if (command.contains("player_state_changed")) {
                    if (message.hasQueryItem("state")) {
                        HeosPlayerState playState = HeosPlayerState::Stop;
                        if (message.queryItemValue("state").contains("play")) {
                            playState = HeosPlayerState::Play;
                        } else if (message.queryItemValue("state").contains("pause")) {
                            playState = HeosPlayerState::Pause;
                        } else if (message.queryItemValue("state").contains("stop")) {
                            playState = HeosPlayerState::Stop;
                        }
                        emit playStateReceived(playerId, playState);
                    }
                }

                if (command.contains("player_volume_changed")) {
                    qDebug() << "Volume Changed";
                    if (message.hasQueryItem("level")) {
                        int volume = message.queryItemValue("level").toInt();
                        emit volumeStatusReceived(playerId, volume);
                    }
                    if (message.hasQueryItem("mute")) {
                        bool mute;
                        if (message.queryItemValue("mute").contains("on")) {
                            mute = true;
                        } else {
                            mute = false;
                        }
                        emit muteStatusReceived(playerId, mute);
                    }
                }

                if (command.contains("repeat_mode_changed")) {

                    if (message.hasQueryItem("repeat")) {
                        HeosRepeatMode repeatMode = HeosRepeatMode::Off;
                        if (message.queryItemValue("repeat").contains("on_all")){
                            repeatMode = HeosRepeatMode::All;
                        } else if (message.queryItemValue("repeat").contains("on_one")){
                            repeatMode = HeosRepeatMode::One;
                        } else  if (message.queryItemValue("repeat").contains("off")){
                            repeatMode = HeosRepeatMode::Off;
                        }
                        emit repeatModeReceived(playerId, repeatMode);
                    }
                }

                if (command.contains("shuffle_mode_changed")) {

                    if (message.hasQueryItem("shuffle")) {
                        bool shuffle;
                        if (message.queryItemValue("shuffle").contains("on")){
                            shuffle = true;
                        } else {
                            shuffle = false;
                        }
                        emit shuffleModeReceived(playerId, shuffle);
                    }
                }

                if (command.contains("player_now_playing_changed")) {
                    getNowPlayingMedia(playerId);
                }
            }
        }
    }
}

void Heos::setConnected(const bool &connected)
{
    m_connected = connected;
    emit connectionStatusChanged();
}
