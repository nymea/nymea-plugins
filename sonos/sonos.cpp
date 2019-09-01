/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *                                                                         *
 *  Copyright (C) 2019 Bernhard Trinnes <bernhard.trinnes@nymea.io>        *
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

#include "sonos.h"
#include "extern-plugininfo.h"

#include <QJsonDocument>

Sonos::Sonos(NetworkAccessManager *networkmanager, const QByteArray &accessToken, QObject *parent) :
    QObject(parent),
    m_accessToken(accessToken),
    m_networkManager(networkmanager)
{
}

void Sonos::setAccessToken(const QByteArray &accessToken)
{
    m_accessToken = accessToken;
}

void Sonos::getHouseholds()
{
    QNetworkRequest request;
    request.setHeader(QNetworkRequest::KnownHeaders::ContentTypeHeader, "application/json");
    request.setRawHeader("Authorization", "Bearer " + m_accessToken);
    request.setRawHeader("X-Sonos-Api-Key", m_apiKey);
    request.setUrl(QUrl(m_baseControlUrl + "/households"));
    QNetworkReply *reply = m_networkManager->get(request);
    qDebug(dcSonos()) << "Sending request" << request.url() << request.rawHeaderList() << request.rawHeader("Authorization");
    connect(reply, &QNetworkReply::finished, this, [reply, this] {
        reply->deleteLater();
        int status = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();

        // Check HTTP status code
        if (status != 200 || reply->error() != QNetworkReply::NoError) {
            qCWarning(dcSonos()) << "Request error:" << status << reply->errorString();
            return;
        }

        qDebug(dcSonos()) << "Received response from Sonos" << reply->readAll();
        /*QJsonDocument data = reply->readAll();
        if (!data.isObject())
            return;
        QList<HouseholdObject> households;
        emit householdObjectsReceived(households);*/
    });
}

void Sonos::cancelAudioClip()
{

}

void Sonos::loadAudioClip()
{

}

void Sonos::getFavorites()
{

}

void Sonos::loadFavorite()
{

}

void Sonos::getGroups(const QByteArray &householdId)
{
    QNetworkRequest request;
    request.setHeader(QNetworkRequest::KnownHeaders::ContentTypeHeader, "application/json");
    request.setRawHeader("Authorization", "Bearer " + m_accessToken);
    request.setRawHeader("X-Sonos-Api-Key", m_apiKey);
    request.setUrl(QUrl(m_baseControlUrl + "/households/" + householdId + "/groups"));
    QNetworkReply *reply = m_networkManager->get(request);
    connect(reply, &QNetworkReply::finished, this, [reply, this] {
        reply->deleteLater();
        int status = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();

        // Check HTTP status code
        if (status != 200 || reply->error() != QNetworkReply::NoError) {
            qCWarning(dcSonos()) << "Request error:" << status << reply->errorString();
            return;
        }

        qDebug(dcSonos()) << "Received response from Sonos" << reply->readAll();
        /*QJsonDocument data = reply->readAll();
        if (!data.isObject())
            return;

        QList<HouseholdObject> households;
        emit householdObjectsReceived(households);*/
    });
}

void Sonos::createGroup(const QByteArray &householdId, QList<QByteArray> playerIds)
{
    Q_UNUSED(householdId)
    Q_UNUSED(playerIds)
}

void Sonos::modifyGroupMembers()
{

}

void Sonos::setGroupMembers(const QByteArray &groupId)
{
    Q_UNUSED(groupId)
}

void Sonos::getGroupVolume(const QByteArray &groupId)
{
    Q_UNUSED(groupId)
}

void Sonos::setGroupVolume(const QByteArray &groupId, int volume)
{
    Q_UNUSED(groupId)
    Q_UNUSED(volume)
}

void Sonos::setGroupMute(const QByteArray &groupId, bool mute)
{
    Q_UNUSED(groupId)
    Q_UNUSED(mute)
}

void Sonos::setGroupRelativeVolume(const QByteArray &groupId, int volumeDelta)
{
    Q_UNUSED(groupId)
    Q_UNUSED(volumeDelta)
}

void Sonos::getGroupPlaybackStatus(const QByteArray &groupId)
{
    Q_UNUSED(groupId)
}

void Sonos::groupLoadLineIn(const QByteArray &groupId)
{
    Q_UNUSED(groupId)

}

void Sonos::groupPlay(const QByteArray &groupId)
{
    Q_UNUSED(groupId)
}

void Sonos::groupPause(const QByteArray &groupId)
{
    Q_UNUSED(groupId)
}

void Sonos::groupSeek(const QByteArray &groupId)
{
    Q_UNUSED(groupId)
}

void Sonos::groupSeekRelative(const QByteArray &groupId, int millis)
{
    Q_UNUSED(groupId)
    Q_UNUSED(millis)
}

void Sonos::groupSetPlayModes(const QByteArray &groupId, PlayMode playMode)
{
    Q_UNUSED(groupId)
    Q_UNUSED(playMode)
}

void Sonos::groupSetShuffle(const QByteArray &groupId, bool shuffle)
{
    Q_UNUSED(groupId)
    Q_UNUSED(shuffle)
}

void Sonos::groupSetRepeat(const QByteArray &groupId, RepeatMode repeatMode)
{
    Q_UNUSED(groupId)
    Q_UNUSED(repeatMode)
}

void Sonos::groupSetCrossfade(const QByteArray &groupId, bool crossfade)
{
    Q_UNUSED(groupId)
    Q_UNUSED(crossfade)
}

void Sonos::groupSkipToNextTrack(const QByteArray &groupId)
{
    Q_UNUSED(groupId)
}

void Sonos::groupSkipToPreviousTrack(const QByteArray &groupId)
{
    Q_UNUSED(groupId)
}

void Sonos::groupTogglePlayPause(const QByteArray &groupId)
{
    Q_UNUSED(groupId)
}

void Sonos::getPlayerVolume(const QByteArray &playerId)
{
    Q_UNUSED(playerId)
}

void Sonos::setPlayerVolume(const QByteArray &playerId, int volume)
{
    Q_UNUSED(playerId)
    Q_UNUSED(volume)
}

void Sonos::setPlayerRelativeVolume(const QByteArray &playerId, int volumeDelta)
{
    Q_UNUSED(playerId)
    Q_UNUSED(volumeDelta)
}

void Sonos::setPlayerMute(const QByteArray &playerId, bool mute)
{
    Q_UNUSED(playerId)
    Q_UNUSED(mute)
}

void Sonos::getPlaylist()
{

}

void Sonos::getPlaylists()
{

}

void Sonos::loadPlaylist()
{

}

void Sonos::getPlayerSettings()
{

}

void Sonos::setPlayerSettings()
{

}

