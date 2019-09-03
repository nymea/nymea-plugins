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
#include <QJsonObject>
#include <QJsonArray>

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

        connectionChanged(true);
        // Check HTTP status code
        if (status != 200 || reply->error() != QNetworkReply::NoError) {
            qCWarning(dcSonos()) << "Request error:" << status << reply->errorString();
            return;
        }

        //qDebug(dcSonos()) << "Received response from Sonos" << reply->readAll();
        QJsonDocument data = QJsonDocument::fromJson(reply->readAll());
        if (!data.isObject()) {
            qDebug(dcSonos()) << "Household ID: Recieved invalide JSON object";
            return;
        }
        QList<QString> households;
        QJsonArray jsonArray = data["households"].toArray();
        foreach (const QJsonValue & value, jsonArray) {
            QJsonObject obj = value.toObject();
            qDebug(dcSonos()) << "Household ID received:" << obj["id"].toString();
            households.append(obj["id"].toString());
        }
        emit householdIdsReceived(households);
    });
}

QUuid Sonos::cancelAudioClip()
{
    QUuid actionId = QUuid::createUuid();
    return actionId;
}

QUuid Sonos::loadAudioClip()
{
    QUuid actionId = QUuid::createUuid();
    return actionId;
}

void Sonos::getFavorites()
{

}

QUuid Sonos::loadFavorite()
{
    QUuid actionId = QUuid::createUuid();
    return actionId;
}

void Sonos::getGroups(const QString &householdId)
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

        //qDebug(dcSonos()) << "Received response from Sonos" << reply->readAll();
        QJsonDocument data = QJsonDocument::fromJson(reply->readAll());
        if (!data.isObject())
            return;

        if (!data["groups"].isArray())
            return;

        QJsonArray array = data["groups"].toArray();
        QList<GroupObject> groupObjects;
        foreach (const QJsonValue & value, array) {
            QJsonObject obj = value.toObject();
            qDebug(dcSonos()) << "Group ID received:" << obj["id"].toString();
            GroupObject group;
            group.groupId = obj["id"].toString();
            group.displayName = obj["name"].toString();
            groupObjects.append(group);
        }
        emit groupsReceived(groupObjects);
    });
}

QUuid Sonos::createGroup(const QString &householdId, QList<QString> playerIds)
{
    Q_UNUSED(householdId)
    Q_UNUSED(playerIds)
    QUuid actionId = QUuid::createUuid();
    return actionId;
}

QUuid Sonos::modifyGroupMembers()
{
    QUuid actionId = QUuid::createUuid();
    return actionId;
}

QUuid Sonos::setGroupMembers(const QString &groupId)
{
    Q_UNUSED(groupId)
    QUuid actionId = QUuid::createUuid();
    return actionId;
}

void Sonos::getGroupVolume(const QString &groupId)
{
    QNetworkRequest request;
    request.setHeader(QNetworkRequest::KnownHeaders::ContentTypeHeader, "application/json");
    request.setRawHeader("Authorization", "Bearer " + m_accessToken);
    request.setRawHeader("X-Sonos-Api-Key", m_apiKey);
    request.setUrl(QUrl(m_baseControlUrl + "/groups/" + groupId + "/groupVolume"));
    QNetworkReply *reply = m_networkManager->get(request);
    connect(reply, &QNetworkReply::finished, this, [reply, groupId, this] {
        reply->deleteLater();
        int status = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();

        // Check HTTP status code
        if (status != 200 || reply->error() != QNetworkReply::NoError) {
            qCWarning(dcSonos()) << "Request error:" << status << reply->errorString();
            return;
        }

        //qDebug(dcSonos()) << "Received response from Sonos" << reply->readAll();
        QJsonDocument data = QJsonDocument::fromJson(reply->readAll());
        if (!data.isObject())
            return;

        GroupVolumeObject groupVolume;

        groupVolume.volume = data["volume"].toInt();
        groupVolume.muted  = data["muted"].toBool();
        groupVolume.fixed  = data["fixed"].toBool();

        emit volumeReceived(groupId, groupVolume);
    });
}

QUuid Sonos::setGroupVolume(const QString &groupId, int volume)
{
    QNetworkRequest request;
    request.setHeader(QNetworkRequest::KnownHeaders::ContentTypeHeader, "application/json");
    request.setRawHeader("Authorization", "Bearer " + m_accessToken);
    request.setRawHeader("X-Sonos-Api-Key", m_apiKey);
    request.setUrl(QUrl(m_baseControlUrl + "/groups/" + groupId + "/groupVolume"));
    QUuid actionId = QUuid::createUuid();

    QJsonObject object;
    object.insert("volume", QJsonValue::fromVariant(volume));
    QJsonDocument doc(object);

    QNetworkReply *reply = m_networkManager->post(request, doc.toBinaryData());
    connect(reply, &QNetworkReply::finished, this, [reply, actionId, this] {
        reply->deleteLater();
        int status = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();

        // Check HTTP status code
        if (status != 200 || reply->error() != QNetworkReply::NoError) {
            qCWarning(dcSonos()) << "Request error:" << status << reply->errorString();
            emit actionExecuted(actionId, false);
            return;
        }
        emit actionExecuted(actionId, true);
    });
    return actionId;
}

QUuid Sonos::setGroupMute(const QString &groupId, bool mute)
{
    Q_UNUSED(groupId)
    Q_UNUSED(mute)

    QNetworkRequest request;
    request.setHeader(QNetworkRequest::KnownHeaders::ContentTypeHeader, "application/json");
    request.setRawHeader("Authorization", "Bearer " + m_accessToken);
    request.setRawHeader("X-Sonos-Api-Key", m_apiKey);
    request.setUrl(QUrl(m_baseControlUrl + "/groups/" + groupId + "/groupVolume/mute"));
    QUuid actionId = QUuid::createUuid();

    QJsonObject object;
    object.insert("muted", QJsonValue::fromVariant(mute));
    QJsonDocument doc(object);

    QNetworkReply *reply = m_networkManager->post(request, doc.toBinaryData());
    connect(reply, &QNetworkReply::finished, this, [reply, actionId, this] {
        reply->deleteLater();
        int status = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();

        // Check HTTP status code
        if (status != 200 || reply->error() != QNetworkReply::NoError) {
            qCWarning(dcSonos()) << "Request error:" << status << reply->errorString();
            emit actionExecuted(actionId, false);
            return;
        }
        emit actionExecuted(actionId, true);
    });
    return actionId;
}

QUuid Sonos::setGroupRelativeVolume(const QString &groupId, int volumeDelta)
{
    QNetworkRequest request;
    request.setHeader(QNetworkRequest::KnownHeaders::ContentTypeHeader, "application/json");
    request.setRawHeader("Authorization", "Bearer " + m_accessToken);
    request.setRawHeader("X-Sonos-Api-Key", m_apiKey);
    request.setUrl(QUrl(m_baseControlUrl + "/groups/" + groupId + "/groupVolume/relative"));
    QUuid actionId = QUuid::createUuid();

    QJsonObject object;
    object.insert("volumeDelta", QJsonValue::fromVariant(volumeDelta));
    QJsonDocument doc(object);

    QNetworkReply *reply = m_networkManager->post(request, doc.toBinaryData());
    connect(reply, &QNetworkReply::finished, this, [reply, actionId, this] {
        reply->deleteLater();
        int status = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();

        // Check HTTP status code
        if (status != 200 || reply->error() != QNetworkReply::NoError) {
            qCWarning(dcSonos()) << "Request error:" << status << reply->errorString();
            emit actionExecuted(actionId, false);
            return;
        }
        emit actionExecuted(actionId, true);
    });
    return actionId;
}

void Sonos::getGroupPlaybackStatus(const QString &groupId)
{
    QNetworkRequest request;
    request.setHeader(QNetworkRequest::KnownHeaders::ContentTypeHeader, "application/json");
    request.setRawHeader("Authorization", "Bearer " + m_accessToken);
    request.setRawHeader("X-Sonos-Api-Key", m_apiKey);
    request.setUrl(QUrl(m_baseControlUrl + "/groups/" + groupId + "/playback"));
    QNetworkReply *reply = m_networkManager->get(request);
    connect(reply, &QNetworkReply::finished, this, [reply, this, groupId] {
        reply->deleteLater();
        int status = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();

        // Check HTTP status code
        if (status != 200 || reply->error() != QNetworkReply::NoError) {
            qCWarning(dcSonos()) << "Request error:" << status << reply->errorString();
            return;
        }

        QJsonDocument data = QJsonDocument::fromJson(reply->readAll());
        if (!data.isObject())
            return;

        PlayBackObject playBackObject;
        QJsonObject object = data.object();
        playBackObject.itemId = object["itemId"].toString();
        playBackObject.positionMillis = object["positionMillis"].toInt();
        playBackObject.previousItemId = object["previousItemId"].toInt();
        playBackObject.previousPositionMillis = object["previousPositionMillis"].toInt();
        QString playBackState = object["playbackState"].toString();
        if (playBackState.contains("BUFFERING")) {
            playBackObject.playbackState = PlayBackStateBuffering;
        } else if (playBackState.contains("IDLE")) {
            playBackObject.playbackState = PlayBackStateIdle;
        } else if (playBackState.contains("PAUSE")) {
            playBackObject.playbackState = PlayBackStatePause;
        } else if (playBackState.contains("PLAYING")) {
            playBackObject.playbackState = PlayBackStatePlaying;
        }
        playBackObject.isDucking = object["isDucking"].toBool();
        playBackObject.queueVersion = object["queueVersion"].toString();
        emit playBackStatusReceived(groupId, playBackObject);
    });
}

QUuid Sonos::groupLoadLineIn(const QString &groupId)
{
    Q_UNUSED(groupId)
    QUuid actionId = QUuid::createUuid();
    return actionId;
}

QUuid Sonos::groupPlay(const QString &groupId)
{
    QNetworkRequest request;
    request.setHeader(QNetworkRequest::KnownHeaders::ContentTypeHeader, "application/json");
    request.setRawHeader("Authorization", "Bearer " + m_accessToken);
    request.setRawHeader("X-Sonos-Api-Key", m_apiKey);
    request.setUrl(QUrl(m_baseControlUrl + "/groups/" + groupId + "/playback/play"));
    QUuid actionId = QUuid::createUuid();

    QNetworkReply *reply = m_networkManager->post(request, "");
    connect(reply, &QNetworkReply::finished, this, [reply, actionId, this] {
        reply->deleteLater();
        int status = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();

        // Check HTTP status code
        if (status != 200 || reply->error() != QNetworkReply::NoError) {
            qCWarning(dcSonos()) << "Request error:" << status << reply->errorString();
            emit actionExecuted(actionId, false);
            return;
        }
        emit actionExecuted(actionId, true);
    });
    return actionId;
}

QUuid Sonos::groupPause(const QString &groupId)
{
    QNetworkRequest request;
    request.setHeader(QNetworkRequest::KnownHeaders::ContentTypeHeader, "application/json");
    request.setRawHeader("Authorization", "Bearer " + m_accessToken);
    request.setRawHeader("X-Sonos-Api-Key", m_apiKey);
    request.setUrl(QUrl(m_baseControlUrl + "/groups/" + groupId + "/playback/pause"));
    QUuid actionId = QUuid::createUuid();

    QNetworkReply *reply = m_networkManager->post(request, "");
    connect(reply, &QNetworkReply::finished, this, [reply, actionId, this] {
        reply->deleteLater();
        int status = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();

        // Check HTTP status code
        if (status != 200 || reply->error() != QNetworkReply::NoError) {
            qCWarning(dcSonos()) << "Request error:" << status << reply->errorString();
            emit actionExecuted(actionId, false);
            return;
        }
        emit actionExecuted(actionId, true);
    });
    return actionId;
}

QUuid Sonos::groupSeek(const QString &groupId, int possitionMillis)
{
    Q_UNUSED(groupId)
    QNetworkRequest request;
    request.setHeader(QNetworkRequest::KnownHeaders::ContentTypeHeader, "application/json");
    request.setRawHeader("Authorization", "Bearer " + m_accessToken);
    request.setRawHeader("X-Sonos-Api-Key", m_apiKey);
    request.setUrl(QUrl(m_baseControlUrl + "/groups/" + groupId + "/playback/seek"));
    QUuid actionId = QUuid::createUuid();

    QJsonObject object;
    object.insert("positionMillis", QJsonValue::fromVariant(possitionMillis));
    QJsonDocument doc(object);

    QNetworkReply *reply = m_networkManager->post(request, doc.toBinaryData());
    connect(reply, &QNetworkReply::finished, this, [reply, actionId, this] {
        reply->deleteLater();
        int status = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();

        // Check HTTP status code
        if (status != 200 || reply->error() != QNetworkReply::NoError) {
            qCWarning(dcSonos()) << "Request error:" << status << reply->errorString();
            emit actionExecuted(actionId, false);
            return;
        }
        emit actionExecuted(actionId, true);
    });
    return actionId;
}

QUuid Sonos::groupSeekRelative(const QString &groupId, int deltaMillis)
{
    Q_UNUSED(groupId)
    QNetworkRequest request;
    request.setHeader(QNetworkRequest::KnownHeaders::ContentTypeHeader, "application/json");
    request.setRawHeader("Authorization", "Bearer " + m_accessToken);
    request.setRawHeader("X-Sonos-Api-Key", m_apiKey);
    request.setUrl(QUrl(m_baseControlUrl + "/groups/" + groupId + "/playback/seekRelative"));
    QUuid actionId = QUuid::createUuid();

    QJsonObject object;
    object.insert("deltaMillis", QJsonValue::fromVariant(deltaMillis));
    QJsonDocument doc(object);

    QNetworkReply *reply = m_networkManager->post(request, doc.toBinaryData());
    connect(reply, &QNetworkReply::finished, this, [reply, actionId, this] {
        reply->deleteLater();
        int status = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();

        // Check HTTP status code
        if (status != 200 || reply->error() != QNetworkReply::NoError) {
            qCWarning(dcSonos()) << "Request error:" << status << reply->errorString();
            emit actionExecuted(actionId, false);
            return;
        }
        emit actionExecuted(actionId, true);
    });
    return actionId;
}

QUuid Sonos::groupSetPlayModes(const QString &groupId, PlayMode playMode)
{
    QNetworkRequest request;
    request.setHeader(QNetworkRequest::KnownHeaders::ContentTypeHeader, "application/json");
    request.setRawHeader("Authorization", "Bearer " + m_accessToken);
    request.setRawHeader("X-Sonos-Api-Key", m_apiKey);
    request.setUrl(QUrl(m_baseControlUrl + "/groups/" + groupId + "/playback/playMode"));
    QUuid actionId = QUuid::createUuid();

    QJsonObject object;
    QJsonObject playModesObject;
    playModesObject["repeat"] = playMode.repeat;
    playModesObject["repeatOne"] = playMode.repeatOne;
    playModesObject["crossfade"] = playMode.crossfade;
    playModesObject["shuffle"] = playMode.shuffle;
    object.insert("playModes", playModesObject);
    QJsonDocument doc(object);

    QNetworkReply *reply = m_networkManager->post(request, doc.toBinaryData());
    connect(reply, &QNetworkReply::finished, this, [reply, actionId, this] {
        reply->deleteLater();
        int status = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();

        // Check HTTP status code
        if (status != 200 || reply->error() != QNetworkReply::NoError) {
            qCWarning(dcSonos()) << "Request error:" << status << reply->errorString();
            emit actionExecuted(actionId, false);
            return;
        }
        emit actionExecuted(actionId, true);
    });
    return actionId;
}

QUuid Sonos::groupSetShuffle(const QString &groupId, bool shuffle)
{
    QNetworkRequest request;
    request.setHeader(QNetworkRequest::KnownHeaders::ContentTypeHeader, "application/json");
    request.setRawHeader("Authorization", "Bearer " + m_accessToken);
    request.setRawHeader("X-Sonos-Api-Key", m_apiKey);
    request.setUrl(QUrl(m_baseControlUrl + "/groups/" + groupId + "/playback/playMode"));
    QUuid actionId = QUuid::createUuid();

    QJsonObject object;
    QJsonObject playModesObject;
    playModesObject["shuffle"] = shuffle;
    object.insert("playModes", playModesObject);
    QJsonDocument doc(object);

    QNetworkReply *reply = m_networkManager->post(request, doc.toBinaryData());
    connect(reply, &QNetworkReply::finished, this, [reply, actionId, this] {
        reply->deleteLater();
        int status = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();

        // Check HTTP status code
        if (status != 200 || reply->error() != QNetworkReply::NoError) {
            qCWarning(dcSonos()) << "Request error:" << status << reply->errorString();
            emit actionExecuted(actionId, false);
            return;
        }
        emit actionExecuted(actionId, true);
    });
    return actionId;
}

QUuid Sonos::groupSetRepeat(const QString &groupId, RepeatMode repeatMode)
{
    QNetworkRequest request;
    request.setHeader(QNetworkRequest::KnownHeaders::ContentTypeHeader, "application/json");
    request.setRawHeader("Authorization", "Bearer " + m_accessToken);
    request.setRawHeader("X-Sonos-Api-Key", m_apiKey);
    request.setUrl(QUrl(m_baseControlUrl + "/groups/" + groupId + "/playback/playMode"));
    QUuid actionId = QUuid::createUuid();

    QJsonObject object;
    QJsonObject playModesObject;
    if (repeatMode == RepeatModeAll) {
        playModesObject["repeat"] = true;
        playModesObject["repeatOne"] = false;
    } else if (repeatMode == RepeatModeOne) {
        playModesObject["repeat"] = false;
        playModesObject["repeatOne"] = true;
    } else if (repeatMode == RepeatModeAll) {
        playModesObject["repeat"] = false;
        playModesObject["repeatOne"] = false;
    }
    object.insert("playModes", playModesObject);
    QJsonDocument doc(object);

    QNetworkReply *reply = m_networkManager->post(request, doc.toBinaryData());
    connect(reply, &QNetworkReply::finished, this, [reply, actionId, this] {
        reply->deleteLater();
        int status = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();

        // Check HTTP status code
        if (status != 200 || reply->error() != QNetworkReply::NoError) {
            qCWarning(dcSonos()) << "Request error:" << status << reply->errorString();
            emit actionExecuted(actionId, false);
            return;
        }
        emit actionExecuted(actionId, true);
    });
    return actionId;
}

QUuid Sonos::groupSetCrossfade(const QString &groupId, bool crossfade)
{
    QNetworkRequest request;
    request.setHeader(QNetworkRequest::KnownHeaders::ContentTypeHeader, "application/json");
    request.setRawHeader("Authorization", "Bearer " + m_accessToken);
    request.setRawHeader("X-Sonos-Api-Key", m_apiKey);
    request.setUrl(QUrl(m_baseControlUrl + "/groups/" + groupId + "/playback/playMode"));
    QUuid actionId = QUuid::createUuid();

    QJsonObject object;
    QJsonObject playModesObject;
    playModesObject["crossfade"] = crossfade;
    object.insert("playModes", playModesObject);
    QJsonDocument doc(object);

    QNetworkReply *reply = m_networkManager->post(request, doc.toBinaryData());
    connect(reply, &QNetworkReply::finished, this, [reply, actionId, this] {
        reply->deleteLater();
        int status = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();

        // Check HTTP status code
        if (status != 200 || reply->error() != QNetworkReply::NoError) {
            qCWarning(dcSonos()) << "Request error:" << status << reply->errorString();
            emit actionExecuted(actionId, false);
            return;
        }
        emit actionExecuted(actionId, true);
    });
    return actionId;
}

QUuid Sonos::groupSkipToNextTrack(const QString &groupId)
{
    QNetworkRequest request;
    request.setHeader(QNetworkRequest::KnownHeaders::ContentTypeHeader, "application/json");
    request.setRawHeader("Authorization", "Bearer " + m_accessToken);
    request.setRawHeader("X-Sonos-Api-Key", m_apiKey);
    request.setUrl(QUrl(m_baseControlUrl + "/groups/" + groupId + "/playback/skipToNextTrack"));
    QUuid actionId = QUuid::createUuid();

    QNetworkReply *reply = m_networkManager->post(request, "");
    connect(reply, &QNetworkReply::finished, this, [reply, actionId, this] {
        reply->deleteLater();
        int status = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();

        // Check HTTP status code
        if (status != 200 || reply->error() != QNetworkReply::NoError) {
            qCWarning(dcSonos()) << "Request error:" << status << reply->errorString();
            actionExecuted(actionId, false);
            return;
        }
        actionExecuted(actionId, true);
    });
    return actionId;
}

QUuid Sonos::groupSkipToPreviousTrack(const QString &groupId)
{
    QNetworkRequest request;
    request.setHeader(QNetworkRequest::KnownHeaders::ContentTypeHeader, "application/json");
    request.setRawHeader("Authorization", "Bearer " + m_accessToken);
    request.setRawHeader("X-Sonos-Api-Key", m_apiKey);
    request.setUrl(QUrl(m_baseControlUrl + "/groups/" + groupId + "/playback/skipToPreviousTrack"));
    QUuid actionId = QUuid::createUuid();

    QNetworkReply *reply = m_networkManager->post(request, "");
    connect(reply, &QNetworkReply::finished, this, [reply, actionId, this] {
        reply->deleteLater();
        int status = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();

        // Check HTTP status code
        if (status != 200 || reply->error() != QNetworkReply::NoError) {
            qCWarning(dcSonos()) << "Request error:" << status << reply->errorString();
            actionExecuted(actionId, false);
            return;
        }
        actionExecuted(actionId, true);
    });
    return actionId;
}

QUuid Sonos::groupTogglePlayPause(const QString &groupId)
{
    QNetworkRequest request;
    request.setHeader(QNetworkRequest::KnownHeaders::ContentTypeHeader, "application/json");
    request.setRawHeader("Authorization", "Bearer " + m_accessToken);
    request.setRawHeader("X-Sonos-Api-Key", m_apiKey);
    request.setUrl(QUrl(m_baseControlUrl + "/groups/" + groupId + "/playback/togglePlayPause"));
    QUuid actionId = QUuid::createUuid();

    QNetworkReply *reply = m_networkManager->post(request, "");
    connect(reply, &QNetworkReply::finished, this, [reply, actionId, this] {
        reply->deleteLater();
        int status = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();

        // Check HTTP status code
        if (status != 200 || reply->error() != QNetworkReply::NoError) {
            qCWarning(dcSonos()) << "Request error:" << status << reply->errorString();
            actionExecuted(actionId, false);
            return;
        }
        actionExecuted(actionId, true);
    });
    return actionId;
}

void Sonos::getGroupMetadataStatus(const QString &groupId)
{
    QNetworkRequest request;
    request.setHeader(QNetworkRequest::KnownHeaders::ContentTypeHeader, "application/json");
    request.setRawHeader("Authorization", "Bearer " + m_accessToken);
    request.setRawHeader("X-Sonos-Api-Key", m_apiKey);
    request.setUrl(QUrl(m_baseControlUrl + "/groups/" + groupId + "/playbackMetadata"));
    QNetworkReply *reply = m_networkManager->get(request);
    connect(reply, &QNetworkReply::finished, this, [reply, groupId, this] {
        reply->deleteLater();
        int status = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();

        // Check HTTP status code
        if (status != 200 || reply->error() != QNetworkReply::NoError) {
            qCWarning(dcSonos()) << "Request error:" << status << reply->errorString();
            return;
        }

        QJsonDocument data = QJsonDocument::fromJson(reply->readAll());
        if (!data.isObject())
            return;

        MetadataStatus metaDataStatus;
        QJsonObject object = data.object();

        if (object.contains("container")) {
            ContainerObject container;
            QJsonObject containerObject = object["container"].toObject();
            container.name = containerObject["name"].toString();
            container.type = containerObject["type"].toString();
            container.imageUrl = containerObject["imageUrl"].toString();
            if (containerObject.contains("service")) {
                ServiceObject service;
                QJsonObject serviceObject = containerObject.value("artist").toObject();
                service.name = serviceObject["name"].toString();
                container.service = service;
            }
            if (containerObject.contains("id")) {
                qDebug(dcSonos()) << "Item ID" << containerObject.value("id").toString();
            }
            metaDataStatus.container = container;
        }

        if (object.contains("currentItem")) {
            QJsonObject currentItemObject = object["currentItem"].toObject();
            ItemObject currentItem;
            if (currentItemObject.contains("track")) {
                TrackObject track;
                QJsonObject trackObject = currentItemObject["track"].toObject();

                if (trackObject.contains("artist")) {
                    ArtistObject artist;
                    QJsonObject artistObject = trackObject["artist"].toObject();
                    artist.name = artistObject["name"].toString();
                    //qDebug(dcSonos()) << "Track object contains artist" << artist.name;
                    track.artist = artist;
                }
                if (trackObject.contains("album")) {
                    AlbumObject album;
                    QJsonObject albumObject = trackObject["album"].toObject();
                    album.name = albumObject["name"].toString();
                    //qDebug(dcSonos()) << "Track object contains album" << album.name;
                    track.album = album;
                }
                if (trackObject.contains("service")) {
                    ServiceObject service;
                    QJsonObject serviceObject = trackObject["service"].toObject();
                    service.name = serviceObject["name"].toString();
                    //qDebug(dcSonos()) << "Track object contains service" << service.name;
                    track.service = service;
                }
                if (trackObject.contains("id")) {
                    qDebug(dcSonos()) << "Item ID" << trackObject.value("id").toString();
                }

                track.type = trackObject["type"].toString();
                track.name = trackObject["name"].toString();
                track.imageUrl = trackObject["imageUrl"].toString();
                track.trackNumber = trackObject["trackNumber"].toInt();
                track.durationMillis = trackObject["durationMillis"].toInt();

                currentItem.track = track;
            }
            metaDataStatus.currentItem = currentItem;
        }

        if (object.contains("nextItem")) {
            ItemObject nextItem;
            QJsonObject nextItemObject = object["nextItem"].toObject();
            if (nextItemObject.contains("track")) {
                TrackObject track;
                QJsonObject trackObject = nextItemObject.value("track").toObject();

                if (trackObject.contains("artist")) {
                    ArtistObject artist;
                    QJsonObject artistObject = trackObject.value("artist").toObject();
                    artist.name = artistObject["name"].toString();
                    //qDebug(dcSonos()) << "Track object contains artist" << artist.name;
                    track.artist = artist;
                }
                if (trackObject.contains("album")) {
                    AlbumObject album;
                    QJsonObject albumObject = trackObject.value("album").toObject();
                    album.name = albumObject["name"].toString();
                    //qDebug(dcSonos()) << "Track object contains album" << album.name;
                    track.album = album;
                }
                if (trackObject.contains("service")) {
                    ServiceObject service;
                    QJsonObject serviceObject = trackObject.value("service").toObject();
                    service.name = serviceObject["name"].toString();
                    //qDebug(dcSonos()) << "Track object contains service" << service.name;
                    track.service = service;
                }
                if (trackObject.contains("id")) {
                    qDebug(dcSonos()) << "Item ID" << trackObject.value("id").toString();
                }
                track.type = trackObject["type"].toString();
                track.name = trackObject["name"].toString();
                track.imageUrl = trackObject["imageUrl"].toString();
                track.trackNumber = trackObject["trackNumber"].toInt();
                track.durationMillis = trackObject["durationMillis"].toInt();

                nextItem.track = track;
            }
            metaDataStatus.nextItem = nextItem;
        }
        emit metadataStatusReceived(groupId, metaDataStatus);
    });
}

void Sonos::getPlayerVolume(const QByteArray &playerId)
{
    Q_UNUSED(playerId)
}

QUuid Sonos::setPlayerVolume(const QByteArray &playerId, int volume)
{
    Q_UNUSED(playerId)
    Q_UNUSED(volume)
    QUuid actionId = QUuid::createUuid();
    return actionId;
}

QUuid Sonos::setPlayerRelativeVolume(const QByteArray &playerId, int volumeDelta)
{
    Q_UNUSED(playerId)
    Q_UNUSED(volumeDelta)
    QUuid actionId = QUuid::createUuid();
    return actionId;
}

QUuid Sonos::setPlayerMute(const QByteArray &playerId, bool mute)
{
    Q_UNUSED(playerId)
    Q_UNUSED(mute)
    QUuid actionId = QUuid::createUuid();
    return actionId;
}

void Sonos::getPlaylist()
{

}

void Sonos::getPlaylists()
{

}

QUuid Sonos::loadPlaylist()
{
    QUuid actionId = QUuid::createUuid();
    return actionId;
}

void Sonos::getPlayerSettings()
{

}

QUuid Sonos::setPlayerSettings()
{
    QUuid actionId = QUuid::createUuid();
    return actionId;
}

