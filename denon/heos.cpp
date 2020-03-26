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

#if QT_VERSION >= QT_VERSION_CHECK(5, 10, 0)
#include <QRandomGenerator>
#endif

Heos::Heos(const QHostAddress &hostAddress, QObject *parent) :
    QObject(parent),
    m_hostAddress(hostAddress)
{
    m_socket = new QTcpSocket(this);

    connect(m_socket, &QTcpSocket::stateChanged, this, &Heos::onStateChanged);
    connect(m_socket, &QTcpSocket::readyRead, this, &Heos::readData);
    connect(m_socket, SIGNAL(error(QAbstractSocket::SocketError)), this, SLOT(onError(QAbstractSocket::SocketError)));

    m_reconnectTimer = new QTimer(this);
    m_reconnectTimer->setInterval(5000);
    connect(m_reconnectTimer, &QTimer::timeout, this, [this]{
        connectDevice();
    });
}

Heos::~Heos()
{
    m_socket->close();
}

void Heos::connectDevice()
{
    if (m_socket->state() == QAbstractSocket::ConnectingState) {
       return;
    }
    m_socket->connectToHost(m_hostAddress, 1255);
}

bool Heos::connected()
{
    return m_socket->isOpen();
}


void Heos::disconnectDevice()
{
    m_socket->close();
}


/********************************
 *        PLAYER COMMANDS
 ********************************/
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

/********************************
 *        PLAYER COMMANDS
 ********************************/
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
void Heos::getNowPlayingMedia(int playerId)
{
    QByteArray cmd = "heos://player/get_now_playing_media?pid=" + QVariant(playerId).toByteArray() + "\r\n";
    m_socket->write(cmd);
}

void Heos::getPlayers()
{
    QByteArray cmd = "heos://player/get_players\r\n";
    m_socket->write(cmd);
}

void Heos::getPlayerInfo(int playerId)
{
    QByteArray cmd = "heos://player/get_player_info?pid=" + QVariant(playerId).toByteArray() + "\r\n";
    qCDebug(dcDenon) << "Get player info:" << cmd;
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

/********************************
 *        GROUP COMMANDS
 ********************************/
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

void Heos::setGroup(QList<int> playerIds)
{
    QByteArray cmd = "heos://group/set_group?pid=";
    foreach(int playerId, playerIds) {
        cmd.append(QVariant(playerId).toByteArray());
        cmd.append(',');
    }
    cmd.resize(cmd.size()-1); //remove last ','
    cmd.append("\r\n");
    qCDebug(dcDenon) << "Set group:" << cmd;
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


/********************************
 *       BROWSE COMMANDS
 ********************************/
quint32 Heos::getMusicSources()
{
    quint32 sequence = createRandomNumber();
    QByteArray cmd = "heos://browse/get_music_sources?";
    QUrlQuery queryParams;
    queryParams.addQueryItem("SEQUENCE", QString::number(sequence));
    cmd.append(queryParams.toString());
    cmd.append("\r\n");
    qCDebug(dcDenon) << "Get music sources:" << cmd;
    m_socket->write(cmd);
    return sequence;
}

quint32 Heos::getSourceInfo(const QString &sourceId)
{
    quint32 sequence = createRandomNumber();
    QByteArray cmd = "heos://browse/get_source_info?";
    QUrlQuery queryParams;
    queryParams.addQueryItem("sid", sourceId);
    queryParams.addQueryItem("SEQUENCE", QString::number(sequence));
    cmd.append(queryParams.toString());
    cmd.append("\r\n");
    qCDebug(dcDenon) << "Get source info:" << cmd;
    m_socket->write(cmd);
    return sequence;
}

quint32 Heos::getSearchCriteria(const QString &sourceId)
{
    quint32 sequence = createRandomNumber();;
    QByteArray cmd = "heos://browse/get_search_criteria?";
    QUrlQuery queryParams;
    queryParams.addQueryItem("sid", sourceId);
    cmd.append(queryParams.toString());
    cmd.append("\r\n");
    qCDebug(dcDenon) << "Get search criteria:" << cmd;
    m_socket->write(cmd);
    return sequence;
}

quint32 Heos::browseSource(const QString &sourceId)
{
    quint32 sequence = createRandomNumber();
    QByteArray cmd = "heos://browse/browse?";
    QUrlQuery queryParams;
    queryParams.addQueryItem("sid", sourceId);
    queryParams.addQueryItem("SEQUENCE", QString::number(sequence));
    cmd.append(queryParams.toString());
    cmd.append("\r\n");
    qCDebug(dcDenon) << "Browse source:" << cmd;
    m_socket->write(cmd);
    return sequence;
}

quint32 Heos::browseSourceContainers(const QString &sourceId, const QString &containerId)
{
    quint32 sequence = createRandomNumber();;
    QByteArray cmd = "heos://browse/browse?";
    QUrlQuery queryParams;
    queryParams.addQueryItem("sid", sourceId);
    queryParams.addQueryItem("cid", containerId);
    queryParams.addQueryItem("SEQUENCE", QString::number(sequence));
    cmd.append(queryParams.toString());
    cmd.append("\r\n");
    qCDebug(dcDenon) << "Browsing container:" << cmd;
    m_socket->write(cmd);
    return sequence;
}

quint32 Heos::playStation(int playerId, const QString &sourceId, const QString &containerId, const QString &mediaId, const QString &stationName)
{
    quint32 sequence = createRandomNumber();;
    QByteArray cmd("heos://browse/play_stream?");
    QUrlQuery queryParams;
    queryParams.addQueryItem("pid", QString::number(playerId));
    if (!sourceId.isEmpty()) {
        queryParams.addQueryItem("sid", sourceId);
    }
    if (!containerId.isEmpty()) {
        queryParams.addQueryItem("cid", containerId);
    }
    if (!mediaId.isEmpty()) {
        queryParams.addQueryItem("mid", mediaId);
    }
    if (!stationName.isEmpty()) {
        queryParams.addQueryItem("name", stationName);
    }
    queryParams.addQueryItem("SEQUENCE", QString::number(sequence));
    cmd.append(queryParams.toString());
    cmd.append("\r\n");
    qCDebug(dcDenon) << "playing station:" << cmd;
    m_socket->write(cmd);
    return sequence;
}

quint32 Heos::playPresetStation(int playerId, int presetNumber)
{
    quint32 sequence = createRandomNumber();
    QByteArray cmd("heos://browse/play_preset?");
    QUrlQuery queryParams;
    queryParams.addQueryItem("pid", QString::number(playerId));
    queryParams.addQueryItem("preset", QString::number(presetNumber));
    queryParams.addQueryItem("SEQUENCE", QString::number(sequence));
    cmd.append(queryParams.toString());
    cmd.append("\r\n");
    qCDebug(dcDenon) << "playing preset station:" << cmd;
    m_socket->write(cmd);
    return sequence;
}

quint32 Heos::playInputSource(int playerId, const QString &inputName)
{
    quint32 sequence = createRandomNumber();
    QByteArray cmd("heos://browse/play_input?");
    QUrlQuery queryParams;
    queryParams.addQueryItem("pid", QString::number(playerId));
    queryParams.addQueryItem("input", inputName);
    queryParams.addQueryItem("SEQUENCE", QString::number(sequence));
    cmd.append(queryParams.toString());
    cmd.append("\r\n");
    qCDebug(dcDenon) << "playing input source:" << cmd;
    m_socket->write(cmd);
    return sequence;
}

quint32 Heos::playUrl(int playerId, const QUrl &mediaUrl)
{
    quint32 sequence = createRandomNumber();
    QByteArray cmd("heos://browse/play_stream?");
    QUrlQuery queryParams;
    queryParams.addQueryItem("pid", QString::number(playerId));
    queryParams.addQueryItem("url", mediaUrl.toString());
    queryParams.addQueryItem("SEQUENCE", QString::number(sequence));
    cmd.append(queryParams.toString());
    cmd.append("\r\n");
    qCDebug(dcDenon) << "playing url:" << cmd;
    m_socket->write(cmd);
    return sequence;
}

void Heos::onStateChanged(QAbstractSocket::SocketState state)
{
    switch (state) {
    case QAbstractSocket::ConnectedState:
        qCDebug(dcDenon()) << "connected successfully to" << m_hostAddress.toString();
        m_reconnectTimer->stop();
        emit connectionStatusChanged(true);
        break;
    case QAbstractSocket::ConnectingState:
        break;
    case QAbstractSocket::UnconnectedState:
        m_reconnectTimer->start();
        qCDebug(dcDenon()) << "Disconnected from" << m_hostAddress.toString() << "try reconnecting in 5 seconds";
        emit connectionStatusChanged(false);
        break;
    default:
        emit connectionStatusChanged(false);
    }
}

quint32 Heos::addContainerToQueue(int playerId, const QString &sourceId, const QString &containerId, ADD_CRITERIA addCriteria)
{
    quint32 sequence = createRandomNumber();
    QByteArray cmd("heos://browse/add_to_queue?");
    QUrlQuery queryParams;
    queryParams.addQueryItem("pid", QString::number(playerId));
    queryParams.addQueryItem("sid", sourceId);
    queryParams.addQueryItem("cid", containerId);
    queryParams.addQueryItem("aid", QString::number(addCriteria));
    queryParams.addQueryItem("SEQUENCE", QString::number(sequence));
    cmd.append(queryParams.toString());
    cmd.append("\r\n");
    qCDebug(dcDenon) << "Adding to queue:" << cmd;
    m_socket->write(cmd);
    return sequence;
}

void Heos::onError(QAbstractSocket::SocketError socketError)
{
    qCWarning(dcDenon) << "socket error:" << socketError << m_socket->errorString();
}

void Heos::readData()
{
    QByteArray data;
    QJsonParseError error;

    while (m_socket->canReadLine()) {
        data = m_socket->readLine();
        QJsonDocument jsonDoc = QJsonDocument::fromJson(data, &error);
        if (error.error != QJsonParseError::NoError) {
            qCWarning(dcDenon) << "failed to parse json :" << error.errorString();
            return;
        }
        QVariantMap dataMap = jsonDoc.toVariant().toMap();
        if (dataMap.contains("heos")) {
            QString command = dataMap.value("heos").toMap().value("command").toString();
            QUrlQuery message(dataMap.value("heos").toMap().value("message").toString());
            bool success = false;
            if(dataMap.value("heos").toMap().contains("result")) {
                //If the message doesn't contain result it is an event message
                success = dataMap.value("heos").toMap().value("result").toString().contains("success");
                if (!success) {
                    qCWarning(dcDenon()) << "Command:" << command << "was not successfull. Message:" << message.toString();
                    if (command == "system/sign_in") {
                        emit userChanged(false, "");
                    }
                }
            }
            /*
             * 4.1 System Commands
             *  4.1.1 Register for Change Events
             *  4.1.2 HEOS Account Check
             *  4.1.3 HEOS Account Sign In
             *  4.1.4 HEOS Account Sign Out
             *  4.1.5 HEOS System Heart Beat
             *  4.1.6 HEOS Speaker Reboot
             *  4.1.7 Prettify JSON response
            */
            if (command.startsWith("system")) {
                if (command.contains("register_for_change_events")) {
                    QString enabled = message.queryItemValue("enabled");
                    if (enabled.contains("off")) {
                        qDebug(dcDenon) << "Events are disabled";
                        m_eventRegistered = false;
                        emit systemEventsEnabled(false);
                    } else {
                        qDebug(dcDenon) << "Events are enabled";
                        m_eventRegistered = true;
                        emit systemEventsEnabled(true);
                    }

                } else if (command.contains("check_account")) {

                    qDebug(dcDenon()) << "System command check_account:" << message.toString();
                    bool signedIn;
                    QString username = "";
                    if (message.hasQueryItem("signed_in")){
                        signedIn = true;
                        username = message.queryItemValue("un");
                    } else {
                        signedIn = false;
                    }
                    emit userChanged(signedIn, username);

                } else if (command.contains("sign_in")) {

                    qDebug(dcDenon()) << "System command sign_in:" << message.toString();

                    if (message.hasQueryItem("signed_in")) {
                        QString username = message.queryItemValue("un");
                        emit userChanged(true, username);

                    } // otherwise it will be command under process and we will wait for the event
                } else if (command.contains("sign_out")) {

                    qDebug(dcDenon()) << "System command sign_out:" << message.toString();
                    emit userChanged(false, "");

                } else if (command.contains("heart_beat")) {

                } else if (command.contains("reboot")) {

                } else if (command.contains("prettify_json_response")) {

                }  else {
                    qDebug(dcDenon) << "Unhandled Heos system command" << command;
                }
                /* 4.2 Player Commands
             *  4.2.1 Get Players
             *  4.2.2 Get Player Info
             *  4.2.3 Get Play State
             *  4.2.4 Set Play State
             *  4.2.5 Get Now Playing Media
             *  4.2.6 Get Volume
             *  4.2.7 Set Volume
             *  4.2.8 Volume Up
             *  4.2.9 Volume Down
             *  4.2.10 Get Mute
             *  4.2.11 Set Mute
             *  4.2.12 Toggle Mute
             *  4.2.13 Get Play Mode
             *  4.2.14 Set Play Mode
             *  4.2.15 Get Queue
             *  4.2.16 Play Queue Item
             *  4.2.17 Remove Item(s) from Queue
             *  4.2.18 Save Queue as Playlist
             *  4.2.19 Clear Queue
             *  4.2.20 Move Queue
             *  4.2.21 Play Next
             *  4.2.22 Play Previous
             *  4.2.23 Set QuickSelect [LS AVR Only]
             *  4.2.24 Play QuickSelect [LS AVR Only]
             *  4.2.25 Get QuickSelects [LS AVR Only]
             *  4.2.26 Check for Firmware Update
             */
            } else if (command.startsWith("player")) {
                int playerId = 0;
                if (message.hasQueryItem("pid")) {
                    playerId = message.queryItemValue("pid").toInt();
                }

                if (command.contains("get_players")) {
                    QVariantList payloadVariantList = jsonDoc.toVariant().toMap().value("payload").toList();
                    QList<HeosPlayer *> players;
                    foreach (const QVariant &payloadEntryVariant, payloadVariantList) {
                        HeosPlayer *player = new HeosPlayer(payloadEntryVariant.toMap().value("pid").toInt());
                        player->setSerialNumber(payloadEntryVariant.toMap().value("serial").toString());
                        player->setName(payloadEntryVariant.toMap().value("name").toString());
                        getPlayerInfo(player->playerId());
                        players.append(player);
                    }
                    emit playersRecieved(players);

                } else if (command.contains("get_player_info")) {
                    //update heos player info
                    int pid = dataMap.value("payload").toMap().value("pid").toInt();
                    HeosPlayer *player = new HeosPlayer(pid);
                    player->setName(dataMap.value("payload").toMap().value("name").toString());
                    if (dataMap.value("payload").toMap().contains("gid")) {
                        player->setGroupId(dataMap.value("payload").toMap().value("gid").toInt());
                    } else {
                        player->setGroupId(-1); //no group assigned
                    }
                    player->setPlayerModel(dataMap.value("payload").toMap().value("model").toString());
                    player->setPlayerVersion(dataMap.value("payload").toMap().value("version").toString());
                    player->setLineOut(dataMap.value("payload").toMap().value("lineout").toString());
                    player->setControl(dataMap.value("payload").toMap().value("control").toString());
                    player->setSerialNumber(dataMap.value("payload").toMap().value("serial").toString());
                    player->setNetwork(dataMap.value("payload").toMap().value("network").toString());
                    emit playerInfoRecieved(player);

                } else if (command.contains("get_now_playing_media")) {

                    QString artist = dataMap.value("payload").toMap().value("artist").toString();
                    QString song = dataMap.value("payload").toMap().value("song").toString();
                    QString artwork = dataMap.value("payload").toMap().value("image_url").toString();
                    QString album = dataMap.value("payload").toMap().value("album").toString();
                    QString sourceId = dataMap.value("payload").toMap().value("sid").toString();
                    qDebug(dcDenon) << "Now playing" << playerId << sourceId << artist << album << song;
                    emit nowPlayingMediaStatusReceived(playerId, sourceId, artist, album, song, artwork);

                } else if (command.contains("get_play_state") || command.contains("set_play_state")) {
                    if (message.hasQueryItem("state")) {
                        PLAYER_STATE playState =  PLAYER_STATE_STOP;
                        if (message.queryItemValue("state").contains("play")) {
                            playState =  PLAYER_STATE_PLAY;
                        } else if (message.queryItemValue("state").contains("pause")) {
                            playState = PLAYER_STATE_PAUSE;
                        } else if (message.queryItemValue("state").contains("stop")) {
                            playState =  PLAYER_STATE_STOP;
                        }
                        emit playerPlayStateReceived(playerId, playState);
                    }
                } else if (command.contains("get_volume") || command.contains("set_volume")) {
                    if (message.hasQueryItem("level")) {
                        int volume = message.queryItemValue("level").toInt();
                        emit playerVolumeReceived(playerId, volume);
                    }
                } else if (command.contains("get_mute") || command.contains("set_mute")) {
                    if (message.hasQueryItem("state")) {
                        QString state = message.queryItemValue("state");
                        if (state.contains("on")) {
                            emit playerMuteStatusReceived(playerId, true);
                        } else {
                            emit playerMuteStatusReceived(playerId, false);
                        }
                    }
                } else if (command.contains("get_play_mode") || command.contains("set_play_mode")) {
                    if (message.hasQueryItem("shuffle") && message.hasQueryItem("repeat")) {
                        bool shuffle;
                        if (message.queryItemValue("shuffle").contains("on")){
                            shuffle = true;
                        } else {
                            shuffle = false;
                        }
                        emit playerShuffleModeReceived(playerId, shuffle);

                        REPEAT_MODE repeatMode = REPEAT_MODE_OFF;
                        if (message.queryItemValue("repeat").contains("on_all")){
                            repeatMode = REPEAT_MODE_ALL;
                        } else if (message.queryItemValue("repeat").contains("on_one")){
                            repeatMode = REPEAT_MODE_ONE;
                        } else  if (message.queryItemValue("repeat").contains("off")){
                            repeatMode = REPEAT_MODE_OFF;
                        }
                        emit playerRepeatModeReceived(playerId, repeatMode);
                    }
                } else if (command.contains("check_update")) {
                    QVariantMap payloadVariantMap = jsonDoc.toVariant().toMap().value("payload").toMap();
                    bool updateExist = payloadVariantMap.value("update").toString().contains("exist");
                    emit playerUpdateAvailable(playerId, updateExist);
                } else {
                    qDebug(dcDenon) << "Unhandled Heos group command" << command;
                }
                /*
             * 4.3 Group Commands
             *  4.3.1 Get Groups
             *  4.3.2 Get Group Info
             *  4.3.3 Set Group
             *  4.3.4 Get Group Volume
             *  4.3.5 Set Group Volume
             *  4.2.6 Group Volume Up
             *  4.2.7 Group Volume Down
             *  4.3.8 Get Group Mute
             *  4.3.9 Set Group Mute
             *  4.3.10 Toggle Group Mute
             */
            } else if (command.startsWith("group")) {
                int groupId = 0;
                if (message.hasQueryItem("gid")) {
                    qDebug(dcDenon) << "Group id" << message.queryItemValue("gid");
                    groupId = message.queryItemValue("gid").toInt();
                }
                if (command.contains("get_groups")) {
                    QVariantList payloadVariantList = jsonDoc.toVariant().toMap().value("payload").toList();
                    QList<GroupObject> groups;
                    foreach (const QVariant &payloadEntryVariant, payloadVariantList) {
                        GroupObject group;
                        group.groupId = payloadEntryVariant.toMap().value("gid").toInt();
                        group.name = payloadEntryVariant.toMap().value("name").toString();
                        if (!payloadEntryVariant.toMap().value("players").toList().isEmpty()) {
                            QVariantList playerlist = payloadEntryVariant.toMap().value("players").toList();
                            foreach (const QVariant &playerVariant, playerlist) {
                                PlayerObject player;
                                player.name = playerVariant.toMap().value("name").toString();
                                player.playerId = playerVariant.toMap().value("pid").toInt();
                                group.players.append(player);
                            }
                        }
                        groups.append(group);
                    }
                    emit groupsReceived(groups);
                } else if (command.contains("get_group_info")) {
                    QVariantMap payloadVariantMap = jsonDoc.toVariant().toMap().value("payload").toMap();
                    GroupObject group;
                    group.groupId = payloadVariantMap.value("gid").toInt();
                    group.name = payloadVariantMap.value("name").toString();
                    if (!payloadVariantMap.value("players").toList().isEmpty()) {
                        QVariantList playerlist = payloadVariantMap.value("players").toList();
                        foreach (const QVariant &playerVariant, playerlist) {
                            PlayerObject player;
                            player.name = playerVariant.toMap().value("name").toString();
                            player.playerId = playerVariant.toMap().value("pid").toInt();
                            group.players.append(player);
                        }
                    }
                    emit groupInfoReceived(group);

                } else if (command.contains("set_group")) {
                    if (message.hasQueryItem("gid")) {

                        int groupId = message.queryItemValue("gid").toInt();
                        QString groupName = message.queryItemValue("name");
                        emit setGroupReceived(groupId, groupName);
                    } else {
                        //No group Id so it must have been an ungoup request
                        int playerId = message.queryItemValue("pid").toInt();
                        emit deleteGroupReceived(playerId);
                    }

                } else if (command.contains("get_volume") || command.contains("set_volume")) {

                    if (message.hasQueryItem("level")) {
                        int volume = message.queryItemValue("level").toInt();
                        emit groupVolumeReceived(groupId, volume);
                    }

                } else if (command.contains("volume_up") || command.contains("volume_down")) {

                } else if (command.contains("get_mute") || command.contains("set_mute")) {

                    if (message.hasQueryItem("state")) {
                        QString state = message.queryItemValue("state");
                        if (state.contains("on")) {
                            emit playerMuteStatusReceived(groupId, true);
                        } else {
                            emit playerMuteStatusReceived(groupId, false);
                        }
                    }
                } else if (command.contains("toggle_mute")) {

                } else {
                    qDebug(dcDenon) << "Unhandled Heos group command" << command;
                }


                /* 4.4 Browse Commands
                                                  *  4.4.1 Get Music Sources                         - "command": "browse/get_music_sources"
                                                  *  4.4.2 Get Source Info                           - "command": "browse/get_source_info"
                                                  *  4.4.3 Browse Source                             - "command": "browse/browse",
                                                  *  4.4.4 Browse Source Containers                  - "command": "browse/browse",
                                                  *  4.4.5 Get Source Search Criteria                - "command": "browse/get_search_criteria"
                                                  *  4.4.6 Search                                    - "command": "browse/search"
                                                  *  4.4.7 Play Station                              - "command": "browse/play_stream"
                                                  *  4.4.8 Play Preset Station                       - "command": "browse/play_preset"
                                                  *  4.4.9 Play Input source                         - "command": "browse/play_input"
                                                  *  4.4.10 Play URL                                 - "command": "browse/play_stream "
                                                  *  4.4.11 Add Container to Queue with Options      - "command": "browse/add_to_queue"
                                                  *  4.4.12 Add Track to Queue with Options          - "command": "browse/add_to_queue"
                                                  *  4.4.14 Rename HEOS Playlist                     - "command": "browse/rename_playlist"
                                                  *  4.4.15 Delete HEOS Playlist                     - "command": "browse/delete_playlist "
                                                  *  4.4.17 Retrieve Album Metadata                  - "command": "browse/retrieve_metadata",
                                                 */
            } else if (command.startsWith("browse") || command.startsWith(" browse")) {

                quint32 sequenceNumber = 0;
                if (message.hasQueryItem("SEQUENCE")) {
                    qDebug(dcDenon) << "Sequence number" << message.queryItemValue("SEQUENCE");
                    sequenceNumber = message.queryItemValue("SEQUENCE").toUInt();
                }

                if (command.contains("get_music_sources") || command.contains("get_source_info")) {
                    qDebug(dcDenon()) << "Get music source request response received" << command;
                    QVariantList payloadVariantList = jsonDoc.toVariant().toMap().value("payload").toList();
                    QList<MusicSourceObject> musicSources;
                    if (success) {
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
                        emit musicSourcesReceived(sequenceNumber, musicSources);
                    }

                } else if (command.contains("browse/browse")) {
                    qDebug(dcDenon()) << "Browse response:"  << jsonDoc.toVariant().toMap().value("payload");
                    QVariantList payloadVariantList = jsonDoc.toVariant().toMap().value("payload").toList();
                    QString sourceId;
                    QString containerId;
                    if (message.hasQueryItem("sid")) {
                        sourceId = message.queryItemValue("sid");
                    }
                    if (message.hasQueryItem("cid")) {
                        containerId = message.queryItemValue("cid");
                    }

                    if (message.toString().contains("command under process")){
                        qDebug(dcDenon()) << "Browse command is beeing processed";
                        return;
                    }
                    if (success) {
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
                                qDebug(dcDenon()) << "Source" << source.name << source.type << source.sourceId << payloadEntryVariant.toMap().value("sid");
                                //source.available = payloadEntryVariant.toMap().value("available").toString().contains("true");
                                //source.serviceUsername = payloadEntryVariant.toMap().value("service_username").toString();
                                musicSources.append(source);
                            } else {
                                MediaObject media;
                                qDebug(dcDenon()) << "Media Item" << payloadEntryVariant.toMap().value("mid").toString() << payloadEntryVariant.toMap().value("cid").toString();
                                media.name = payloadEntryVariant.toMap().value("name").toString();
                                if (payloadEntryVariant.toMap().contains("cid")) {
                                    media.containerId = payloadEntryVariant.toMap().value("cid").toString();
                                } else {
                                    media.containerId = message.queryItemValue("cid");
                                }
                                media.mediaId = payloadEntryVariant.toMap().value("mid").toString();
                                media.imageUrl = payloadEntryVariant.toMap().value("image_url").toString();
                                media.isPlayable = payloadEntryVariant.toMap().value("playable").toString().contains("yes");
                                media.isContainer = payloadEntryVariant.toMap().value("container").toString().contains("yes");
                                media.sourceId = sourceId;
                                if (type == "artist") {
                                    media.mediaType = MEDIA_TYPE_ARTIST;
                                } else if (type == "song") {
                                    media.mediaType = MEDIA_TYPE_SONG;
                                } else if (type == "genre") {
                                    media.mediaType = MEDIA_TYPE_GENRE;
                                } else if (type == "station") {
                                    media.mediaType = MEDIA_TYPE_STATION;
                                } else if (type == "album") {
                                    media.mediaType = MEDIA_TYPE_ALBUM;
                                } else if (type == "container") {
                                    media.mediaType = MEDIA_TYPE_CONTAINER;
                                }
                                mediaItems.append(media);
                            }
                        }
                        emit browseRequestReceived(sequenceNumber, sourceId, containerId, musicSources, mediaItems);
                    }
                    else {
                        int errorId = message.queryItemValue("eid").toInt();
                        QString text = message.queryItemValue("text");
                        emit browseErrorReceived(sourceId, containerId, errorId, text);
                    }
                } else if (command.contains("play_preset")) {

                } else if (command.contains("play_input")) {
                    qCDebug(dcDenon()) << "Command play input";

                } else if (command.contains("add_to_queue")) {

                } else if (command.contains("rename_playlist")) {

                } else if (command.contains("delete_playlist")) {

                } else if (command.contains("retrieve_metadata")) {

                } else {
                    qDebug(dcDenon) << "Unhandled Heos browse command" << command;
                }


                /*
             * 5. Change Events (Unsolicited Responses) 5.1 Sources Changed
             * 5.2 Players Changed
             * 5.3 Group Changed
             * 5.4 Player State Changed
             * 5.5 Player Now Playing Changed
             * 5.6 Player Now Playing Progress
             * 5.7 Player Playback Error
             * 5.8 Player Queue Changed
             * 5.9 Player Volume Changed
             * 5.10 Player Repeat Mode Changed
             * 5.11 Player Shuffle Mode Changed
             * 5.12 Group Volume Changed
             * 5.13 User Changed
             */
            } else if (command.startsWith("event")) {
                if (command.contains("sources_changed")) {
                    emit sourcesChanged();

                } else if (command.contains("players_changed")) {
                    emit playersChanged();

                } else if (command.contains("groups_changed")) {
                    emit groupsChanged();

                } else if (command.contains("player_state_changed")) {
                    qDebug(dcDenon()) << "Player state changed";
                    if (message.hasQueryItem("pid")) {
                        int playerId = message.queryItemValue("pid").toInt();
                        if (message.hasQueryItem("state")) {
                            PLAYER_STATE playState = PLAYER_STATE_STOP;
                            if (message.queryItemValue("state").contains("play")) {
                                playState = PLAYER_STATE_PLAY;
                            } else if (message.queryItemValue("state").contains("pause")) {
                                playState = PLAYER_STATE_PAUSE;
                            } else if (message.queryItemValue("state").contains("stop")) {
                                playState = PLAYER_STATE_STOP;
                            }
                            emit playerPlayStateReceived(playerId, playState);
                        }
                    }
                } else if (command.contains("player_now_playing_changed")) {
                    qDebug(dcDenon()) << "Player now playing changed, player id:" << message.queryItemValue("pid").toInt();
                    if (message.hasQueryItem("pid")) {
                        int playerId = message.queryItemValue("pid").toInt();
                        emit playerNowPlayingChanged(playerId);
                    }
                } else if (command.contains("player_now_playing_progress")) {
                    //qDebug(dcDenon()) << "Player now playing progress";
                    if (message.hasQueryItem("pid")) {
                        int playerId = message.queryItemValue("pid").toInt();
                        int currentPossition = message.queryItemValue("cur_pos").toInt();
                        int duration = message.queryItemValue("duration").toInt();
                        emit playerNowPlayingProgressReceived(playerId, currentPossition, duration);
                    }
                } else if (command.contains("player_playback_error")) {
                    qDebug(dcDenon) << "Player playback error";
                    int playerId = 0;
                    if (message.hasQueryItem("pid")) {
                        playerId = message.queryItemValue("pid").toInt();
                        QString errorMessage = message.queryItemValue("error");
                        emit playerPlaybackErrorReceived(playerId, errorMessage);
                    }
                } else if (command.contains("player_queue_changed")) {
                    qDebug(dcDenon()) << "Player queue Changed";
                    int playerId = 0;
                    if (message.hasQueryItem("pid")) {
                        playerId = message.queryItemValue("pid").toInt();
                        emit playerQueueChanged(playerId);
                    }
                } else if (command.contains("player_volume_changed")) {
                    qDebug(dcDenon()) << "Event player volume Changed";
                    int playerId = 0;
                    if (message.hasQueryItem("pid")) {
                        playerId = message.queryItemValue("pid").toInt();

                        if (message.hasQueryItem("level")) {
                            int volume = message.queryItemValue("level").toInt();
                            emit playerVolumeReceived(playerId, volume);
                        }
                        if (message.hasQueryItem("mute")) {
                            bool mute;
                            if (message.queryItemValue("mute").contains("on")) {
                                mute = true;
                            } else {
                                mute = false;
                            }
                            emit playerMuteStatusReceived(playerId, mute);
                        }
                    }
                } else if (command.contains("repeat_mode_changed")) {
                    qDebug(dcDenon()) << "Repeat mode Changed";
                    int playerId = 0;
                    if (message.hasQueryItem("pid")) {
                        playerId = message.queryItemValue("pid").toInt();

                        if (message.hasQueryItem("repeat")) {
                            REPEAT_MODE repeatMode = REPEAT_MODE_OFF;
                            if (message.queryItemValue("repeat").contains("on_all")){
                                repeatMode = REPEAT_MODE_ALL;
                            } else if (message.queryItemValue("repeat").contains("on_one")){
                                repeatMode = REPEAT_MODE_ONE;
                            } else  if (message.queryItemValue("repeat").contains("off")){
                                repeatMode = REPEAT_MODE_OFF;
                            }
                            emit playerRepeatModeReceived(playerId, repeatMode);
                        }
                    }
                } else if (command.contains("shuffle_mode_changed")) {
                    qDebug(dcDenon()) << "Shuffle mode Changed";
                    int playerId = 0;
                    if (message.hasQueryItem("pid")) {
                        playerId = message.queryItemValue("pid").toInt();

                        if (message.hasQueryItem("shuffle")) {
                            bool shuffle;
                            if (message.queryItemValue("shuffle").contains("on")){
                                shuffle = true;
                            } else {
                                shuffle = false;
                            }
                            emit playerShuffleModeReceived(playerId, shuffle);
                        }
                    }
                } else if (command.contains("group_volume_changed")) {
                    qDebug(dcDenon()) << "Event group volume Changed";
                    int playerId = 0;
                    if (message.hasQueryItem("gid")) {
                        playerId = message.queryItemValue("gid").toInt();

                        if (message.hasQueryItem("level")) {
                            int volume = message.queryItemValue("level").toInt();
                            emit groupVolumeReceived(playerId, volume);
                        }
                        if (message.hasQueryItem("mute")) {
                            bool mute;
                            if (message.queryItemValue("mute").contains("on")) {
                                mute = true;
                            } else {
                                mute = false;
                            }
                            emit groupMuteStatusReceived(playerId, mute);
                        }
                    }
                } else if (command.contains("user_changed")) {

                    qDebug(dcDenon()) << "Event user changed" << message.toString();
                    bool signedIn;
                    QString username;
                    if (message.hasQueryItem("signed_out")){
                        signedIn = false;
                    } else {
                        signedIn = true;
                        username = message.queryItemValue("un");
                    }
                    emit userChanged(signedIn, username);
                } else {
                    qDebug(dcDenon) << "Unhandled Heos event";
                }
            } else {
                qDebug(dcDenon) << "Unhandled Heos category" << command;
            }
        }
    }
}

quint32 Heos::createRandomNumber()
{
#if QT_VERSION >= QT_VERSION_CHECK(5, 10, 0)
    return QRandomGenerator::global()->generate();
#else
    return qrand();
#endif
}


