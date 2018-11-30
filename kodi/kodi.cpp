/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *                                                                         *
 *  Copyright (C) 2015 Simon Stürz <simon.stuerz@guh.io>                   *
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

#include "kodi.h"
#include <QDebug>
#include "extern-plugininfo.h"
#include <QUrl>

Kodi::Kodi(const QHostAddress &hostAddress, const int &port, QObject *parent) :
    QObject(parent),
    m_muted(false),
    m_volume(-1)
{
    m_connection = new KodiConnection(hostAddress, port, this);
    connect (m_connection, &KodiConnection::connectionStatusChanged, this, &Kodi::connectionStatusChanged);

    m_jsonHandler = new KodiJsonHandler(m_connection, this);
    connect(m_jsonHandler, &KodiJsonHandler::notificationReceived, this, &Kodi::processNotification);
    connect(m_jsonHandler, &KodiJsonHandler::replyReceived, this, &Kodi::processResponse);
}

QHostAddress Kodi::hostAddress() const
{
    return m_connection->hostAddress();
}

int Kodi::port() const
{
    return m_connection->port();
}

bool Kodi::connected() const
{
    return m_connection->connected();
}

int Kodi::setMuted(const bool &muted)
{
    QVariantMap params;
    params.insert("mute", muted);

    return m_jsonHandler->sendData("Application.SetMute", params);
}

bool Kodi::muted() const
{
    return m_muted;
}

int Kodi::setVolume(const int &volume)
{
    QVariantMap params;
    params.insert("volume", volume);

    return m_jsonHandler->sendData("Application.SetVolume", params);
}

int Kodi::volume() const
{
    return m_volume;
}

int Kodi::setShuffle(bool shuffle)
{
    QVariantMap params;
    params.insert("playerid", m_activePlayer);
    params.insert("shuffle", shuffle);
    return m_jsonHandler->sendData("Player.SetShuffle", params);
}

int Kodi::setRepeat(const QString &repeat)
{
    QVariantMap params;
    params.insert("playerid", m_activePlayer);
    params.insert("repeat", repeat);
    return m_jsonHandler->sendData("Player.SetRepeat", params);
}

int Kodi::showNotification(const QString &message, const int &displayTime, const QString &notificationType)
{
    QVariantMap params;
    params.insert("title", "nymea notification");
    params.insert("message", message);
    params.insert("displaytime", displayTime);
    params.insert("image", notificationType);

    return m_jsonHandler->sendData("GUI.ShowNotification", params);
}

int Kodi::pressButton(const QString &button)
{
    QVariantMap params;
    params.insert("action", button);
    return m_jsonHandler->sendData("Input.ExecuteAction", params);
}

int Kodi::systemCommand(const QString &command)
{
    QString method;
    if (command == "hibernate") {
        method = "Hibernate";
    } else if (command == "reboot") {
        method = "Reboot";
    } else if (command == "shutdown") {
        method = "Shutdown";
    } else if (command == "suspend") {
        method = "Suspend";
    } else {
        // already checkt with allowed values
    }

    return m_jsonHandler->sendData("System." + method, QVariantMap());
}

int Kodi::videoLibrary(const QString &command)
{
    QString method;
    if (command == "scan") {
        method = "Scan";
    } else if (command == "clean") {
        method = "Clean";
    } else {
        // already checkt with allowed values
    }

    return m_jsonHandler->sendData("VideoLibrary." + method, QVariantMap());
}

int Kodi::audioLibrary(const QString &command)
{
    QString method;
    if (command == "scan") {
        method = "Scan";
    } else if (command == "clean") {
        method = "Clean";
    } else {
        // already checkt with allowed values
    }

    return m_jsonHandler->sendData("AudioLibrary." + method, QVariantMap());
}

void Kodi::update()
{
    QVariantMap params;
    QVariantList properties;
    properties.append("volume");
    properties.append("muted");
    properties.append("name");
    properties.append("version");
    params.insert("properties", properties);

    m_jsonHandler->sendData("Application.GetProperties", params);

    params.clear();
    m_jsonHandler->sendData("Player.GetActivePlayers", params);
}

void Kodi::checkVersion()
{
    m_jsonHandler->sendData("JSONRPC.Version", QVariantMap());
}

void Kodi::connectKodi()
{
    m_connection->connectKodi();
}

void Kodi::disconnectKodi()
{
    m_connection->disconnectKodi();
}

void Kodi::onVolumeChanged(const int &volume, const bool &muted)
{
    if (m_volume != volume || m_muted != muted) {
        m_volume = volume;
        m_muted = muted;
        emit stateChanged();
    }
}

void Kodi::onUpdateFinished(const QVariantMap &data)
{
    qCDebug(dcKodi()) << "update finished:" << data;
    if (data.contains("volume")) {
        m_volume = data.value("volume").toInt();
    }
    if (data.contains("muted")) {
        m_muted = data.value("muted").toBool();
    }
    emit stateChanged();
}

void Kodi::activePlayersChanged(const QVariantList &data)
{
    qCDebug(dcKodi()) << "active players changed" << data.count() << data;
    m_activePlayerCount = data.count();
    if (m_activePlayerCount == 0) {
        onPlaybackStatusChanged("Stopped");
        return;
    }
    m_activePlayer = data.first().toMap().value("playerid").toInt();
    qCDebug(dcKodi) << "Active Player changed:" << m_activePlayer << data.first().toMap().value("type").toString();
    emit activePlayerChanged(data.first().toMap().value("type").toString());

    updatePlayerProperties();
}

void Kodi::playerPropertiesReceived(const QVariantMap &properties)
{
    qCDebug(dcKodi()) << "player props received" << properties;

    if (m_activePlayerCount > 0) {
        if (properties.value("speed").toDouble() > 0) {
            onPlaybackStatusChanged("Playing");
        } else {
            onPlaybackStatusChanged("Paused");
        }
    }

    emit shuffleChanged(properties.value("shuffled").toBool());
    emit repeatChanged(properties.value("repeat").toString());

}

void Kodi::mediaMetaDataReceived(const QVariantMap &data)
{
    QVariantMap item = data.value("item").toMap();

    QString title = item.value("title").toString();
    QString artist;
    QString collection;
    if (item.value("type").toString() == "song") {
        artist = !item.value("artist").toList().isEmpty() ? item.value("artist").toList().first().toString() : "";
        collection = item.value("album").toString();
    } else if (item.value("type").toString() == "episode") {
        collection = item.value("showtitle").toString();
    } else if (item.value("type").toString() == "unknown") {
        artist = item.value("channel").toString();
        collection = item.value("showtitle").toString();
    } else if (item.value("type").toString() == "channel") {
        artist = item.value("channel").toString();
        collection = item.value("showtitle").toString();
    } else if (item.value("type").toString() == "movie") {
        artist = item.value("director").toStringList().join(", ");
        collection = item.value("year").toString();
    }

    QString artwork = item.value("thumbnail").toString();
    if (artwork.isEmpty()) {
        artwork = item.value("fanart").toString();
    }
    qCDebug(dcKodi) << "title:" << title << artwork;
    emit mediaMetadataChanged(title, artist, collection, artwork);
}

void Kodi::onPlaybackStatusChanged(const QString &playbackState)
{
    if (playbackState != "Stopped") {
        updateMetadata();
    } else {
        emit mediaMetadataChanged(QString(), QString(), QString(), QString());
    }
    emit playbackStatusChanged(playbackState);
}

void Kodi::processNotification(const QString &method, const QVariantMap &params)
{
    qCDebug(dcKodi) << "got notification" << method << params;

    if (method == "Application.OnVolumeChanged") {
        QVariantMap data = params.value("data").toMap();
        onVolumeChanged(data.value("volume").toInt(), data.value("muted").toBool());
    } else if (method == "Player.OnPlay" || method == "Player.OnResume") {
        emit activePlayersChanged(QVariantList() << params.value("data").toMap().value("player"));
        onPlaybackStatusChanged("Playing");
    } else if (method == "Player.OnPause") {
        emit playbackStatusChanged("Paused");
    } else if (method == "Player.OnStop") {
        emit playbackStatusChanged("Stopped");
        emit activePlayersChanged(QVariantList());
    }
}

void Kodi::processResponse(int id, const QString &method, const QVariantMap &response)
{

    qCDebug(dcKodi) << "response received:" << method << response;

    if (response.contains("error")) {
        //qCDebug(dcKodi) << QJsonDocument::fromVariant(response).toJson();
        qCWarning(dcKodi) << "got error response for request " << method << ":" << response.value("error").toMap().value("message").toString();
        emit actionExecuted(id, false);
        return;
    }

    if (method == "Application.GetProperties") {
        //qCDebug(dcKodi) << "got update response" << reply.method();
        emit updateDataReceived(response.value("result").toMap());
        return;
    }

    if (method == "JSONRPC.Version") {
        qCDebug(dcKodi) << "got version response" << method;
        emit versionDataReceived(response.value("result").toMap());
        return;
    }

    if (method == "Player.GetActivePlayers") {
        qCDebug(dcKodi) << "Active players changed" << response;
        emit activePlayersChanged(response.value("result").toList());
        return;
    }

    if (method == "Player.GetProperties") {
        qCDebug(dcKodi) << "Player properties received" << response;
        playerPropertiesReceived(response.value("result").toMap());
        return;
    }

    if (method == "Player.GetItem") {
        qCDebug(dcKodi) << "Played item received" << response;
        emit mediaMetaDataReceived(response.value("result").toMap());
        return;
    }

    if (method == "Player.SetShuffle" || method == "Player.SetRepeat") {
        updatePlayerProperties();
    }

    emit actionExecuted(id, true);

    qCDebug(dcKodi()) << "unhandled reply" << method << response;
}

void Kodi::updatePlayerProperties()
{
    QVariantMap params;
    params.insert("playerid", m_activePlayer);
    QVariantList properties;
    properties << "speed" << "shuffled" << "repeat";
    params.insert("properties", properties);
    m_jsonHandler->sendData("Player.GetProperties", params);
}

void Kodi::updateMetadata()
{
    QVariantMap params;
    params.insert("playerid", m_activePlayer);
    QVariantList fields;
    fields << "title" << "artist" << "album" << "director" << "thumbnail" << "showtitle" << "fanart" << "channel" << "year";
    params.insert("properties", fields);
    m_jsonHandler->sendData("Player.GetItem", params);
}
