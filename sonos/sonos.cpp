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
    request.setRawHeader("Authorization", "Bearer" + m_accessToken);
    request.setUrl(QUrl(m_baseControlUrl + "/households"));
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

void Sonos::getGroups()
{

}

void Sonos::createGroup()
{

}

void Sonos::modifyGroupMembers()
{

}

void Sonos::setGroupMembers()
{

}

void Sonos::getGroupVolume(int groupId)
{
    Q_UNUSED(groupId)
}

void Sonos::setGroupVolume(int groupId, int volume)
{
 Q_UNUSED(groupId)
     Q_UNUSED(volume)
}

void Sonos::setGroupMute(int groupId, bool mute)
{
 Q_UNUSED(groupId)
     Q_UNUSED(mute)
}

void Sonos::setGroupRelativeVolume(int groupId, int volumeDelta)
{
 Q_UNUSED(groupId)
     Q_UNUSED(volumeDelta)
}

void Sonos::getPlaybackStatus()
{

}

void Sonos::loadLineIn()
{

}

void Sonos::play()
{

}

void Sonos::pause()
{

}

void Sonos::seek()
{

}

void Sonos::seekRelative()
{

}

void Sonos::setPlayModes()
{

}

void Sonos::skipToNextTrack()
{

}

void Sonos::skipToPreviousTrack()
{

}

void Sonos::togglePlayPause()
{

}

void Sonos::getPlayerVolume(int playerId)
{
  Q_UNUSED(playerId)
}

void Sonos::setPlayerVolume(int playerId, int volume)
{
 Q_UNUSED(playerId)
     Q_UNUSED(volume)
}

void Sonos::setPlayerRelativeVolume(int playerId, int volumeDelta)
{
 Q_UNUSED(playerId)
     Q_UNUSED(volumeDelta)
}

void Sonos::setPlayerMute(int playerId, bool mute)
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

