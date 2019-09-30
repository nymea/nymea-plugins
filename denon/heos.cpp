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

#include "heos.h"
#include "extern-plugininfo.h"
#include <QJsonArray>
#include <QJsonObject>
#include <QJsonDocument>
#include <QUrlQuery>
#include <QTimer>

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

Heos::~Heos()
{
    m_socket->close();
}

void Heos::connectHeos()
{
    if (m_socket->state() == QAbstractSocket::ConnectingState) {
        return;
    }
    m_socket->connectToHost(m_hostAddress, 1255);
}

/*
 *  SYSTEM COMMANDS
 */
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

void Heos::getUserAccount()
{
    QByteArray cmd = "heos://system/check_account\r\n";
    m_socket->write(cmd);
}

void Heos::setUserAccount(QString userName, QString password)
{
    QByteArray cmd = "heos://system/sign_in?un=" + userName.toLocal8Bit() + "&pw=" + password.toLocal8Bit() + "\r\n";
    m_socket->write(cmd);
}

void Heos::logoutUserAccount()
{
    QByteArray cmd = "heos://system/sign_out\r\n";
    m_socket->write(cmd);
}

void Heos::rebootSpeaker()
{
    QByteArray cmd = "heos://system/reboot\r\n";
    m_socket->write(cmd);
}

void Heos::prettifyJsonResponse(bool enable)
{
    QByteArray cmd = "heos://system/prettify_json_response?enable=";
    if (enable) {
        cmd.append("on=\r\n");
    } else {
        cmd.append("off=\r\n");
    }
    m_socket->write(cmd);
}


/*
 *  PLAYER COMMANDS
 */

void Heos::getNowPlayingMedia(int playerId)
{
    QByteArray cmd = "heos://player/get_now_playing_media?pid=" + QVariant(playerId).toByteArray() + "\r\n";
    m_socket->write(cmd);
}

HeosPlayer *Heos::getPlayer(int playerId)
{
    return m_heosPlayers.value(playerId);
}

void Heos::getPlayers()
{
    QByteArray cmd = "heos://player/get_players\r\n";
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

void Heos::setPlayerState(int playerId, PLAYER_STATE state)
{
    QByteArray playerStateQuery;

    if (state == PLAYER_STATE_PLAY){
        playerStateQuery = "&state=play";
    } else if (state == PLAYER_STATE_PAUSE){
        playerStateQuery = "&state=pause";
    } else if (state == PLAYER_STATE_STOP){
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


void Heos::setPlayMode(int playerId, REPEAT_MODE repeatMode, bool shuffle)
{
    QByteArray repeatModeQuery;

    if (repeatMode == REPEAT_MODE_OFF) {
        repeatModeQuery = "&repeat=off";
    } else if (repeatMode == REPEAT_MODE_ONE) {
        repeatModeQuery = "&repeat=on_one";
    } else if (repeatMode == REPEAT_MODE_ALL) {
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

void Heos::getQueue(int playerId)
{
    QByteArray cmd = "heos://player/get_queue?pid=" + QVariant(playerId).toByteArray() + "\r\n";
    m_socket->write(cmd);
}

/*
 *  GROUP COMMANDS
 */
void Heos::getGroups()
{
    QByteArray cmd = "heos://group/get_groups\r\n";
    m_socket->write(cmd);
}

void Heos::getGroupInfo(int groupId)
{
    QByteArray cmd = "heos://group/get_group_info?gid=" + QVariant(groupId).toByteArray() + "\r\n";
    m_socket->write(cmd);
}

void Heos::getGroupVolume(int groupId)
{
    QByteArray cmd = "heos://group/get_volume?gid=" + QVariant(groupId).toByteArray() + "\r\n";
    m_socket->write(cmd);
}

void Heos::getGroupMute(int groupId)
{
    QByteArray cmd = "heos://group/get_mute?gid=" + QVariant(groupId).toByteArray() + "\r\n";
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

void Heos::volumeUp(int playerId, int step)
{
    QByteArray cmd = "heos://player/volume_up?pid=" + QVariant(playerId).toByteArray() + "&step=" + QVariant(step).toByteArray() + "\r\n";
    qCDebug(dcDenon) << "Volume up:" << cmd;
    m_socket->write(cmd);
}

void Heos::volumeDown(int playerId, int step)
{
    QByteArray cmd = "heos://player/volume_down?pid=" + QVariant(playerId).toByteArray() + "&step=" + QVariant(step).toByteArray() + "\r\n";
    qCDebug(dcDenon) << "Volume down:" << cmd;
    m_socket->write(cmd);
}

void Heos::clearQueue(int playerId)
{
    QByteArray cmd = "heos://player/clear_queue?pid=" + QVariant(playerId).toByteArray() + "\r\n";
    qCDebug(dcDenon) << "clear queue:" << cmd;
    m_socket->write(cmd);
}

void Heos::moveQueue(int playerId, int sourcQueueId, int destinationQueueId)
{
    QByteArray cmd("heos://player/move_queue_item?");
    QUrlQuery queryParams;
    queryParams.addQueryItem("pid", QString::number(playerId));
    queryParams.addQueryItem("sqid", QString::number(sourcQueueId));
    queryParams.addQueryItem("dqid", QString::number(destinationQueueId));
    cmd.append(queryParams.toString());
    cmd.append("\r\n");
    qCDebug(dcDenon) << "moving queue:" << cmd;
    m_socket->write(cmd);
}

void Heos::checkForFirmwareUpdate(int playerId)
{
    QByteArray cmd = "heos://player/check_update?pid=" + QVariant(playerId).toByteArray() + "\r\n";
    qCDebug(dcDenon) << "Check firmware update:" << cmd;
    m_socket->write(cmd);
}

void Heos::setGroupVolume(int groupId, bool volume)
{
    QByteArray cmd = "heos://group/set_volume?gid=" + QVariant(groupId).toByteArray() + "&level=" + QVariant(volume).toByteArray() + "\r\n";
    qCDebug(dcDenon) << "Volume up:" << cmd;
    m_socket->write(cmd);
}

void Heos::setGroupMute(int groupId, bool mute)
{
    QByteArray cmd = "heos://group/set_mute?gid=" + QVariant(groupId).toByteArray() + "&state=";
    if (mute) {
        cmd.append("on\r\n");
    } else {
        cmd.append("off\r\n");
    }
    m_socket->write(cmd);
}

void Heos::toggleGroupMute(int groupId)
{
    QByteArray cmd = "heos://group/toggle_mute?gid=" + QVariant(groupId).toByteArray() + "\r\n";
    qCDebug(dcDenon) << "Volume up:" << cmd;
    m_socket->write(cmd);
}

void Heos::groupVolumeUp(int groupId, int step)
{
    QByteArray cmd = "heos://group/volume_up?pid=" + QVariant(groupId).toByteArray() + "&step=" + QVariant(step).toByteArray() + "\r\n";
    qCDebug(dcDenon) << "Group volume up:" << cmd;
    m_socket->write(cmd);
}

void Heos::groupVolumeDown(int groupId, int step)
{
    QByteArray cmd = "heos://group/volume_down?pid=" + QVariant(groupId).toByteArray() + "&step=" + QVariant(step).toByteArray() + "\r\n";
    qCDebug(dcDenon) << "Group volume up:" << cmd;
    m_socket->write(cmd);
}

void Heos::getMusicSources()
{
    QByteArray cmd = "heos://browse/get_music_sources\r\n";
    m_socket->write(cmd);
}

void Heos::getSourceInfo(SOURCE_ID sourceId)
{
    QByteArray cmd = " heos://browse/get_source_info?sid=" + QVariant(sourceId).toByteArray() + "\r\n";
    qCDebug(dcDenon) << "Get source info:" << cmd;
    m_socket->write(cmd);
}

void Heos::getSearchCriteria(SOURCE_ID sourceId)
{
    QByteArray cmd = "heos://browse/get_search_criteria?sid=" + QVariant(sourceId).toByteArray() + "\r\n";
    qCDebug(dcDenon) << "Get search criteria:" << cmd;
    m_socket->write(cmd);
}

void Heos::browseSource(const QString &sourceId)
{
    QByteArray cmd = "heos://browse/browse?sid=" + sourceId.toUtf8() + "\r\n";
    qCDebug(dcDenon) << "Browse source:" << cmd;
    m_socket->write(cmd);
}

void Heos::browseSourceContainers(const QString &sourceId, const QString &containerId)
{
    QByteArray cmd = "heos://browse/browse?";
    QUrlQuery queryParams;
    queryParams.addQueryItem("sid", sourceId);
    queryParams.addQueryItem("cid", containerId);
    cmd.append(queryParams.toString());
    cmd.append("\r\n");
    qCDebug(dcDenon) << "playing station:" << cmd;
    m_socket->write(cmd);
}

void Heos::playStation(int playerId, const QString &sourceId, const QString &containerId, const QString &mediaId, const QString &stationName)
{
    QByteArray cmd("heos://browse/play_stream?");
    QUrlQuery queryParams;
    queryParams.addQueryItem("pid", QString::number(playerId));
    queryParams.addQueryItem("sid", sourceId);
    queryParams.addQueryItem("cid", containerId);
    queryParams.addQueryItem("mid", mediaId);
    queryParams.addQueryItem("name", stationName);
    cmd.append(queryParams.toString());
    cmd.append("\r\n");
    qCDebug(dcDenon) << "playing station:" << cmd;
    m_socket->write(cmd);
}

void Heos::playPresetStation(int playerId, int presetNumber)
{
    QByteArray cmd("heos://browse/play_preset?");
    QUrlQuery queryParams;
    queryParams.addQueryItem("pid", QString::number(playerId));
    queryParams.addQueryItem("preset", QString::number(presetNumber));
    cmd.append(queryParams.toString());
    cmd.append("\r\n");
    qCDebug(dcDenon) << "playing preset station:" << cmd;
    m_socket->write(cmd);
}

void Heos::playInputSource(int playerId, const QString &inputName)
{
    QByteArray cmd("heos://browse/play_input?");
    QUrlQuery queryParams;
    queryParams.addQueryItem("pid", QString::number(playerId));
    queryParams.addQueryItem("input", inputName);
    cmd.append(queryParams.toString());
    cmd.append("\r\n");
    qCDebug(dcDenon) << "playing input source:" << cmd;
    m_socket->write(cmd);
}

void Heos::playUrl(int playerId, const QUrl &mediaUrl)
{
    QByteArray cmd("heos://browse/play_stream?");
    QUrlQuery queryParams;
    queryParams.addQueryItem("pid", QString::number(playerId));
    queryParams.addQueryItem("url", mediaUrl.toString());
    cmd.append(queryParams.toString());
    cmd.append("\r\n");
    qCDebug(dcDenon) << "playing url:" << cmd;
    m_socket->write(cmd);
}

/* This command is used to perform the following actions:
 * Create new group: Creates new group. First player id in the list is group leader.
 * Adds or delete players from the group. First player id should be the group leader id.
 * Ungroup all players in the group
 * Ungroup players. Player id (pid) should be the group leader id.
 */
//void Heos::setGroup()
//{
//}



void Heos::onConnected()
{
    qCDebug(dcDenon()) << "connected successfully to" << m_hostAddress.toString();
    emit connectionStatusChanged(true);
}

void Heos::onDisconnected()
{
    qCDebug(dcDenon()) << "Disconnected from" << m_hostAddress.toString() << "try reconnecting in 5 seconds";
    QTimer::singleShot(5000, this, [this](){
        connectHeos();
    });
    emit connectionStatusChanged(false);
}

void Heos::onError(QAbstractSocket::SocketError socketError)
{
    qCWarning(dcDenon) << "socket error:" << socketError << m_socket->errorString();
}

void Heos::readData()
{
    int playerId = 0;
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
            bool success = dataMap.value("heos").toMap().value("result").toString().contains("success");
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
                    SOURCE_ID sourceId = SOURCE_ID(dataMap.value("payload").toMap().value("sid").toInt());
                    emit nowPlayingMediaStatusReceived(playerId, sourceId, artist, album, song, artwork);
                }

                if (command.contains("get_play_state") || command.contains("set_play_state")) {
                    if (message.hasQueryItem("state")) {
                        PLAYER_STATE playState =  PLAYER_STATE_STOP;
                        if (message.queryItemValue("state").contains("play")) {
                            playState =  PLAYER_STATE_PLAY;
                        } else if (message.queryItemValue("state").contains("pause")) {
                            playState = PLAYER_STATE_PAUSE;
                        } else if (message.queryItemValue("state").contains("stop")) {
                            playState =  PLAYER_STATE_STOP;
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

                        REPEAT_MODE repeatMode = REPEAT_MODE_OFF;
                        if (message.queryItemValue("repeat").contains("on_all")){
                            repeatMode = REPEAT_MODE_ALL;
                        } else if (message.queryItemValue("repeat").contains("on_one")){
                            repeatMode = REPEAT_MODE_ONE;
                        } else  if (message.queryItemValue("repeat").contains("off")){
                            repeatMode = REPEAT_MODE_OFF;
                        }
                        emit repeatModeReceived(playerId, repeatMode);
                    }
                }

                if (command.contains("player_state_changed")) {
                    if (message.hasQueryItem("state")) {
                        PLAYER_STATE playState = PLAYER_STATE_STOP;
                        if (message.queryItemValue("state").contains("play")) {
                            playState = PLAYER_STATE_PLAY;
                        } else if (message.queryItemValue("state").contains("pause")) {
                            playState = PLAYER_STATE_PAUSE;
                        } else if (message.queryItemValue("state").contains("stop")) {
                            playState = PLAYER_STATE_STOP;
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
                        REPEAT_MODE repeatMode = REPEAT_MODE_OFF;
                        if (message.queryItemValue("repeat").contains("on_all")){
                            repeatMode = REPEAT_MODE_ALL;
                        } else if (message.queryItemValue("repeat").contains("on_one")){
                            repeatMode = REPEAT_MODE_ONE;
                        } else  if (message.queryItemValue("repeat").contains("off")){
                            repeatMode = REPEAT_MODE_OFF;
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
                if (command.contains("play_stream")) {
                    if (success) {
                        qDebug(dcDenon()) << "Playing Url";
                    }
                }
                if (command.contains("get_music_sources") || command.contains("get_source_info")) {
                    QVariantList payloadVariantList = jsonDoc.toVariant().toMap().value("payload").toList();
                    QList<MusicSourceObject> musicSources;
                    foreach (const QVariant &payloadEntryVariant, payloadVariantList) {
                        MusicSourceObject source;
                        source.name = payloadEntryVariant.toMap().value("name").toString();
                        source.image_url = payloadEntryVariant.toMap().value("image_url").toString();
                        source.type = payloadEntryVariant.toMap().value("type").toString();
                        source.sourceId = payloadEntryVariant.toMap().value("sid").toInt();
                        source.available = payloadEntryVariant.toMap().value("available").toString().contains("true");
                        source.serviceUsername = payloadEntryVariant.toMap().value("service_username").toString();
                        musicSources.append(source);
                    }
                    emit musicSourcesReceived(musicSources);
                }
                if (command.contains("browse/browse")) {
                    QVariantList payloadVariantList = jsonDoc.toVariant().toMap().value("payload").toList();
                    QList<MusicSourceObject> musicSources;
                    QList<MediaObject> mediaItems;

                    foreach (const QVariant &payloadEntryVariant, payloadVariantList) {
                        QString type = payloadEntryVariant.toMap().value("type").toString();
                       if (type == "source") {
                           MusicSourceObject source;
                           source.name = payloadEntryVariant.toMap().value("name").toString();
                           source.image_url = payloadEntryVariant.toMap().value("image_url").toString();
                           source.type = payloadEntryVariant.toMap().value("type").toString();
                           source.sourceId = payloadEntryVariant.toMap().value("sid").toInt();
                           source.available = payloadEntryVariant.toMap().value("available").toString().contains("true");
                           source.serviceUsername = payloadEntryVariant.toMap().value("service_username").toString();
                           musicSources.append(source);
                        } else if (type == "container" || type == "album" || type == "song") {
                            MediaObject media;
                            media.name = payloadEntryVariant.toMap().value("name").toString();
                            media.imageUrl = payloadEntryVariant.toMap().value("image_url").toString();
                            mediaItems.append(media);
                        }
                       if (!mediaItems.isEmpty())
                            emit mediaItemsReceived(mediaItems);

                       if (!musicSources.isEmpty())
                            emit musicSourcesReceived(musicSources);
                    }
                }
            }
        }
    }
}
