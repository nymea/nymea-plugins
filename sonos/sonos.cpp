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
#include <QUrlQuery>

Sonos::Sonos(NetworkAccessManager *networkmanager,  const QByteArray &clientKey,  const QByteArray &clientSecret, QObject *parent) :
    QObject(parent),
    m_clientKey(clientKey),
    m_clientSecret(clientSecret),
    m_networkManager(networkmanager)
{
    if(!m_tokenRefreshTimer) {
        m_tokenRefreshTimer = new QTimer(this);
        m_tokenRefreshTimer->setSingleShot(true);
        connect(m_tokenRefreshTimer, &QTimer::timeout, this, &Sonos::onRefreshTimeout);
    }
}

QUrl Sonos::getLoginUrl(const QUrl &redirectUrl)
{
    if (m_clientKey.isEmpty()) {
        qWarning(dcSonos()) << "Client key not defined!";
        return QUrl("");
    }

    if (redirectUrl.isEmpty()){
        qWarning(dcSonos()) << "No redirect uri defined!";
    }
    m_redirectUri = QUrl::toPercentEncoding(redirectUrl.toString());

    QUrl url("https://api.sonos.com/login/v3/oauth");
    QUrlQuery queryParams;
    queryParams.addQueryItem("client_id", m_clientKey);
    queryParams.addQueryItem("redirect_uri", m_redirectUri);
    queryParams.addQueryItem("response_type", "code");
    queryParams.addQueryItem("scope", "playback-control-all");
    queryParams.addQueryItem("state", QUuid::createUuid().toString());
    url.setQuery(queryParams);

    return url;
}

QByteArray Sonos::accessToken()
{
    return m_accessToken;
}

QByteArray Sonos::refreshToken()
{
    return m_refreshToken;
}

void Sonos::getHouseholds()
{
    QNetworkRequest request;
    request.setHeader(QNetworkRequest::KnownHeaders::ContentTypeHeader, "application/json");
    request.setRawHeader("Authorization", "Bearer " + m_accessToken);
    request.setRawHeader("X-Sonos-Api-Key", m_clientKey);
    request.setUrl(QUrl(m_baseControlUrl + "/households"));
    QNetworkReply *reply = m_networkManager->get(request);
    qDebug(dcSonos()) << "Sending request" << request.url() << request.rawHeaderList() << request.rawHeader("Authorization");
    connect(reply, &QNetworkReply::finished, this, [reply, this] {
        reply->deleteLater();
        int status = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();

        // Check HTTP status code
        if (status != 200 || reply->error() != QNetworkReply::NoError) {
            if (reply->error() == QNetworkReply::HostNotFoundError) {
                emit connectionChanged(false);
            }
            if (status == 400 || status == 401) {
                emit authenticationStatusChanged(false);
            }
            qCWarning(dcSonos()) << "Request error:" << status << reply->errorString();
            return;
        }
        emit connectionChanged(true);
        emit authenticationStatusChanged(true);

        QJsonParseError error;
        QJsonDocument data = QJsonDocument::fromJson(reply->readAll(), &error);
        if (error.error != QJsonParseError::NoError) {
            qDebug(dcSonos()) << "Household ID: Recieved invalide JSON object";
            return;
        }
        QList<QString> households;
        foreach (const QVariant &variant, data.toVariant().toMap().value("households").toList()) {
            QVariantMap obj = variant.toMap();
            //qDebug(dcSonos()) << "Household ID received:" << obj["id"].toString();
            households.append(obj["id"].toString());
        }
        emit householdIdsReceived(households);
    });
}


QUuid Sonos::loadFavorite(const QString &groupId, const QString &favouriteId)
{
    QNetworkRequest request;
    request.setHeader(QNetworkRequest::KnownHeaders::ContentTypeHeader, "application/json");
    request.setRawHeader("Authorization", "Bearer " + m_accessToken);
    request.setRawHeader("X-Sonos-Api-Key", m_clientKey);
    request.setUrl(QUrl(m_baseControlUrl + "/groups/" + groupId + "/favorites"));
    QUuid actionId = QUuid::createUuid();

    QJsonObject object;
    object.insert("favoriteId", favouriteId);
    object.insert("playOnCompletion", true);
    QJsonDocument doc(object);
    qDebug(dcSonos()) << "Sending request" << doc.toJson();

    QNetworkReply *reply = m_networkManager->post(request, doc.toJson(QJsonDocument::Compact));
    connect(reply, &QNetworkReply::finished, this, [reply, actionId, this] {
        reply->deleteLater();
        int status = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();

        // Check HTTP status code
        if (status != 200 || reply->error() != QNetworkReply::NoError) {
            if (reply->error() == QNetworkReply::HostNotFoundError) {
                emit connectionChanged(false);
            }
            if (status == 400 || status == 401) {
                emit authenticationStatusChanged(false);
            }
            emit actionExecuted(actionId, false);
            qCWarning(dcSonos()) << "Request error:" << status << reply->errorString();
            return;
        }
        emit connectionChanged(true);
        emit authenticationStatusChanged(true);
        emit actionExecuted(actionId, true);
    });
    return actionId;
}

QUuid Sonos::getFavorites(const QString &householdId)
{
    QNetworkRequest request;
    request.setHeader(QNetworkRequest::KnownHeaders::ContentTypeHeader, "application/json");
    request.setRawHeader("Authorization", "Bearer " + m_accessToken);
    request.setRawHeader("X-Sonos-Api-Key", m_clientKey);
    request.setUrl(QUrl(m_baseControlUrl + "/households/" + householdId + "/favorites"));
    QUuid requestId = QUuid::createUuid();

    QNetworkReply *reply = m_networkManager->get(request);
    connect(reply, &QNetworkReply::finished, this, [reply, requestId, householdId, this] {
        reply->deleteLater();
        int status = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();

        // Check HTTP status code
        if (status != 200 || reply->error() != QNetworkReply::NoError) {
            if (reply->error() == QNetworkReply::HostNotFoundError) {
                emit connectionChanged(false);
            }
            if (status == 400 || status == 401) {
                emit authenticationStatusChanged(false);
            }
            qCWarning(dcSonos()) << "Request error:" << status << reply->errorString();
            return;
        }
        emit connectionChanged(true);
        emit authenticationStatusChanged(true);

        QJsonParseError error;
        QJsonDocument data = QJsonDocument::fromJson(reply->readAll(), &error);
        if (error.error != QJsonParseError::NoError) {
            qCWarning(dcSonos()) << "Invalid json received from server";
            return;
        }

        if (!data.toVariant().toMap().contains("items"))
            return;

        QVariantList array = data.toVariant().toMap().value("items").toList();
        //qDebug(dcSonos()) << "Favorites received:" << data.toJson();

        QList<FavoriteObject> favorites;
        foreach (const QVariant &variant, array) {
            QVariantMap itemObject = variant.toMap();
            FavoriteObject favorite;
            favorite.id = itemObject["id"].toString();
            favorite.name = itemObject["name"].toString();
            favorite.description = itemObject["description"].toString();
            favorite.imageUrl = itemObject["imageUrl"].toString();
            favorites.append(favorite);
        }
        emit favoritesReceived(requestId, householdId, favorites);
    });
    return requestId;
}


void Sonos::getGroups(const QString &householdId)
{
    QNetworkRequest request;
    request.setHeader(QNetworkRequest::KnownHeaders::ContentTypeHeader, "application/json");
    request.setRawHeader("Authorization", "Bearer " + m_accessToken);
    request.setRawHeader("X-Sonos-Api-Key", m_clientKey);
    request.setUrl(QUrl(m_baseControlUrl + "/households/" + householdId + "/groups"));
    QNetworkReply *reply = m_networkManager->get(request);
    connect(reply, &QNetworkReply::finished, this, [reply, householdId, this] {
        reply->deleteLater();
        int status = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();

        // Check HTTP status code
        if (status != 200 || reply->error() != QNetworkReply::NoError) {
            if (reply->error() == QNetworkReply::HostNotFoundError) {
                emit connectionChanged(false);
            }
            if (status == 400 || status == 401) {
                emit authenticationStatusChanged(false);
            }
            qCWarning(dcSonos()) << "Request error:" << status << reply->errorString();
            return;
        }
        emit connectionChanged(true);
        emit authenticationStatusChanged(true);

        //qDebug(dcSonos()) << "Received response from Sonos" << reply->readAll();
        QJsonParseError error;
        QJsonDocument data = QJsonDocument::fromJson(reply->readAll(), &error);
        if (error.error != QJsonParseError::NoError)
            return;

        if (!data.toVariant().toMap().contains("groups"))
            return;

        QVariantList array = data.toVariant().toMap().value("groups").toList();
        QList<GroupObject> groupObjects;
        foreach (const QVariant &value, array) {
            QVariantMap obj = value.toMap();
            //qDebug(dcSonos()) << "Group ID received:" << obj["id"].toString();
            GroupObject group;
            group.groupId = obj["id"].toString();
            group.displayName = obj["name"].toString();
            group.CoordinatorId = obj["coordinatorId"].toString();
            group.playbackState = obj["playbackState"].toString();
            QVariantList players = obj.value("playerIds").toList();
            foreach (const QVariant &value, players) {
              group.playerIds.append(value.toByteArray());
            }
            groupObjects.append(group);
        }
        emit groupsReceived(householdId, groupObjects);
    });
}

void Sonos::getGroupVolume(const QString &groupId)
{
    QNetworkRequest request;
    request.setHeader(QNetworkRequest::KnownHeaders::ContentTypeHeader, "application/json");
    request.setRawHeader("Authorization", "Bearer " + m_accessToken);
    request.setRawHeader("X-Sonos-Api-Key", m_clientKey);
    request.setUrl(QUrl(m_baseControlUrl + "/groups/" + groupId + "/groupVolume"));
    QNetworkReply *reply = m_networkManager->get(request);
    connect(reply, &QNetworkReply::finished, this, [reply, groupId, this] {
        reply->deleteLater();
        int status = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();

        // Check HTTP status code
        if (status != 200 || reply->error() != QNetworkReply::NoError) {
            if (reply->error() == QNetworkReply::HostNotFoundError) {
                emit connectionChanged(false);
            }
            if (status == 400 || status == 401) {
                emit authenticationStatusChanged(false);
            }
            qCWarning(dcSonos()) << "Request error:" << status << reply->errorString();
            return;
        }
        emit connectionChanged(true);
        emit authenticationStatusChanged(true);

        //qDebug(dcSonos()) << "Received response from Sonos" << reply->readAll();
        QJsonParseError error;
        QJsonDocument data = QJsonDocument::fromJson(reply->readAll(), &error);
        if (error.error != QJsonParseError::NoError) {
            qCWarning(dcSonos()) << "JSON Parse error" << error.errorString();
            return;
        }

        VolumeObject volume;

        QVariantMap variant = data.toVariant().toMap();
        volume.volume = variant["volume"].toInt();
        volume.muted  = variant["muted"].toBool();
        volume.fixed  = variant["fixed"].toBool();

        emit volumeReceived(groupId, volume);
    });
}

QUuid Sonos::setGroupVolume(const QString &groupId, int volume)
{
    QNetworkRequest request;
    request.setHeader(QNetworkRequest::KnownHeaders::ContentTypeHeader, "application/json");
    request.setRawHeader("Authorization", "Bearer " + m_accessToken);
    request.setRawHeader("X-Sonos-Api-Key", m_clientKey);
    request.setUrl(QUrl(m_baseControlUrl + "/groups/" + groupId + "/groupVolume"));
    QUuid actionId = QUuid::createUuid();

    QJsonObject object;
    object.insert("volume", volume);
    QJsonDocument doc(object);
    qDebug(dcSonos()) << "Set volume:" << groupId << doc.toJson(QJsonDocument::Compact);

    QNetworkReply *reply = m_networkManager->post(request, doc.toJson(QJsonDocument::Compact));
    connect(reply, &QNetworkReply::finished, this, [reply, actionId, groupId, this] {
        reply->deleteLater();
        int status = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();

        // Check HTTP status code
        if (status != 200 || reply->error() != QNetworkReply::NoError) {
            if (reply->error() == QNetworkReply::HostNotFoundError) {
                emit connectionChanged(false);
            }
            if (status == 400 || status == 401) {
                emit authenticationStatusChanged(false);
            }
            emit actionExecuted(actionId, false);
            qCWarning(dcSonos()) << "Request error:" << status << reply->errorString();
            return;
        }
        emit connectionChanged(true);
        emit authenticationStatusChanged(true);
        emit actionExecuted(actionId, true);
        getGroupVolume(groupId);
    });
    return actionId;
}

QUuid Sonos::setGroupMute(const QString &groupId, bool mute)
{
    QNetworkRequest request;
    request.setHeader(QNetworkRequest::KnownHeaders::ContentTypeHeader, "application/json");
    request.setRawHeader("Authorization", "Bearer " + m_accessToken);
    request.setRawHeader("X-Sonos-Api-Key", m_clientKey);
    request.setUrl(QUrl(m_baseControlUrl + "/groups/" + groupId + "/groupVolume/mute"));
    QUuid actionId = QUuid::createUuid();

    QJsonObject object;
    object.insert("muted", mute);
    QJsonDocument doc(object);

    qDebug(dcSonos()) << "Set mute:" << groupId << doc.toJson(QJsonDocument::Compact);

    QNetworkReply *reply = m_networkManager->post(request, doc.toJson(QJsonDocument::Compact));
    connect(reply, &QNetworkReply::finished, this, [reply, actionId, groupId, this] {
        reply->deleteLater();
        int status = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();

        // Check HTTP status code
        if (status != 200 || reply->error() != QNetworkReply::NoError) {
            if (reply->error() == QNetworkReply::HostNotFoundError) {
                emit connectionChanged(false);
            }
            if (status == 400 || status == 401) {
                emit authenticationStatusChanged(false);
            }
            emit actionExecuted(actionId, false);
            qCWarning(dcSonos()) << "Request error:" << status << reply->errorString();
            return;
        }
        emit connectionChanged(true);
        emit authenticationStatusChanged(true);
        emit actionExecuted(actionId, true);
        getGroupVolume(groupId);
    });
    return actionId;
}

QUuid Sonos::setGroupRelativeVolume(const QString &groupId, int volumeDelta)
{
    QNetworkRequest request;
    request.setHeader(QNetworkRequest::KnownHeaders::ContentTypeHeader, "application/json");
    request.setRawHeader("Authorization", "Bearer " + m_accessToken);
    request.setRawHeader("X-Sonos-Api-Key", m_clientKey);
    request.setUrl(QUrl(m_baseControlUrl + "/groups/" + groupId + "/groupVolume/relative"));
    QUuid actionId = QUuid::createUuid();

    QJsonObject object;
    object.insert("volumeDelta", QJsonValue::fromVariant(volumeDelta));
    QJsonDocument doc(object);

    qDebug(dcSonos()) << "Relative volume:" << groupId << volumeDelta;

    QNetworkReply *reply = m_networkManager->post(request, doc.toJson(QJsonDocument::Compact));
    connect(reply, &QNetworkReply::finished, this, [reply, actionId, groupId, this] {
        reply->deleteLater();
        int status = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();

        // Check HTTP status code
        if (status != 200 || reply->error() != QNetworkReply::NoError) {
            if (reply->error() == QNetworkReply::HostNotFoundError) {
                emit connectionChanged(false);
            }
            if (status == 400 || status == 401) {
                emit authenticationStatusChanged(false);
            }
            emit actionExecuted(actionId, false);
            qCWarning(dcSonos()) << "Request error:" << status << reply->errorString();
            return;
        }
        emit connectionChanged(true);
        emit authenticationStatusChanged(true);
        emit actionExecuted(actionId, true);
        getGroupVolume(groupId);
    });
    return actionId;
}

void Sonos::getGroupPlaybackStatus(const QString &groupId)
{
    QNetworkRequest request;
    request.setHeader(QNetworkRequest::KnownHeaders::ContentTypeHeader, "application/json");
    request.setRawHeader("Authorization", "Bearer " + m_accessToken);
    request.setRawHeader("X-Sonos-Api-Key", m_clientKey);
    request.setUrl(QUrl(m_baseControlUrl + "/groups/" + groupId + "/playback"));
    QNetworkReply *reply = m_networkManager->get(request);
    connect(reply, &QNetworkReply::finished, this, [reply, this, groupId] {
        reply->deleteLater();
        int status = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();

        // Check HTTP status code
        if (status != 200 || reply->error() != QNetworkReply::NoError) {
            if (reply->error() == QNetworkReply::HostNotFoundError) {
                emit connectionChanged(false);
            }
            if (status == 400 || status == 401) {
                emit authenticationStatusChanged(false);
            }
            qCWarning(dcSonos()) << "Request error:" << status << reply->errorString();
            return;
        }
        emit connectionChanged(true);
        emit authenticationStatusChanged(true);

        QJsonDocument data = QJsonDocument::fromJson(reply->readAll());
        if (!data.isObject())
            return;

        PlayBackObject playBack;
        QJsonObject object = data.object();
        playBack.itemId = object["itemId"].toString();
        playBack.positionMillis = object["positionMillis"].toInt();
        playBack.previousItemId = object["previousItemId"].toInt();
        playBack.previousPositionMillis = object["previousPositionMillis"].toInt();
        QString playBackState = object["playbackState"].toString();
        if (playBackState.contains("BUFFERING")) {
            playBack.playbackState = PlayBackStateBuffering;
        } else if (playBackState.contains("IDLE")) {
            playBack.playbackState = PlayBackStateIdle;
        } else if (playBackState.contains("PAUSE")) {
            playBack.playbackState = PlayBackStatePause;
        } else if (playBackState.contains("PLAYING")) {
            playBack.playbackState = PlayBackStatePlaying;
        }
        playBack.isDucking = object["isDucking"].toBool();
        playBack.queueVersion = object["queueVersion"].toString();
        if (object.contains("playModes")) {
            PlayMode playMode;
            QJsonObject playModeObject = object["playModes"].toObject();
            playMode.repeat = playModeObject["repeat"].toBool();
            playMode.repeatOne = playModeObject["repeatOne"].toBool();
            playMode.crossfade = playModeObject["crossfade"].toBool();
            playMode.shuffle = playModeObject["shuffle"].toBool();
            playBack.playMode = playMode;
        }
        emit playBackStatusReceived(groupId, playBack);
    });
}

QUuid Sonos::groupLoadLineIn(const QString &groupId)
{
    qDebug(dcSonos()) << "Load line in:" << groupId;
    QNetworkRequest request;
    request.setHeader(QNetworkRequest::KnownHeaders::ContentTypeHeader, "application/json");
    request.setRawHeader("Authorization", "Bearer " + m_accessToken);
    request.setRawHeader("X-Sonos-Api-Key", m_clientKey);
    request.setUrl(QUrl(m_baseControlUrl + "/groups/" + groupId + "/playback/lineIn"));
    QUuid actionId = QUuid::createUuid();

    QNetworkReply *reply = m_networkManager->post(request, "");
    connect(reply, &QNetworkReply::finished, this, [reply, actionId, groupId, this] {
        reply->deleteLater();
        int status = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();

        // Check HTTP status code
        if (status != 200 || reply->error() != QNetworkReply::NoError) {
            if (reply->error() == QNetworkReply::HostNotFoundError) {
                emit connectionChanged(false);
            }
            if (status == 400 || status == 401) {
                emit authenticationStatusChanged(false);
            }
            emit actionExecuted(actionId, false);
            qCWarning(dcSonos()) << "Request error:" << status << reply->errorString();
            return;
        }
        emit connectionChanged(true);
        emit authenticationStatusChanged(true);
        emit actionExecuted(actionId, true);
        getGroupVolume(groupId);
    });
    return actionId;
}

QUuid Sonos::groupPlay(const QString &groupId)
{
    QNetworkRequest request;
    request.setHeader(QNetworkRequest::KnownHeaders::ContentTypeHeader, "application/json");
    request.setRawHeader("Authorization", "Bearer " + m_accessToken);
    request.setRawHeader("X-Sonos-Api-Key", m_clientKey);
    request.setUrl(QUrl(m_baseControlUrl + "/groups/" + groupId + "/playback/play"));
    QUuid actionId = QUuid::createUuid();

    qDebug(dcSonos()) << "Play:" << groupId;

    QNetworkReply *reply = m_networkManager->post(request, "");
    connect(reply, &QNetworkReply::finished, this, [reply, actionId, groupId, this] {
        reply->deleteLater();
        int status = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();

        // Check HTTP status code
        if (status != 200 || reply->error() != QNetworkReply::NoError) {
            if (reply->error() == QNetworkReply::HostNotFoundError) {
                emit connectionChanged(false);
            }
            if (status == 400 || status == 401) {
                emit authenticationStatusChanged(false);
            }
            emit actionExecuted(actionId, false);
            qCWarning(dcSonos()) << "Request error:" << status << reply->errorString();
            return;
        }
        emit connectionChanged(true);
        emit authenticationStatusChanged(true);
        emit actionExecuted(actionId, true);
        getGroupPlaybackStatus(groupId);
    });
    return actionId;
}

QUuid Sonos::groupPause(const QString &groupId)
{
    QNetworkRequest request;
    request.setHeader(QNetworkRequest::KnownHeaders::ContentTypeHeader, "application/json");
    request.setRawHeader("Authorization", "Bearer " + m_accessToken);
    request.setRawHeader("X-Sonos-Api-Key", m_clientKey);
    request.setUrl(QUrl(m_baseControlUrl + "/groups/" + groupId + "/playback/pause"));
    QUuid actionId = QUuid::createUuid();

    qDebug(dcSonos()) << "Pause:" << groupId;

    QNetworkReply *reply = m_networkManager->post(request, "");
    connect(reply, &QNetworkReply::finished, this, [reply, actionId, groupId, this] {
        reply->deleteLater();
        int status = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();

        // Check HTTP status code
        if (status != 200 || reply->error() != QNetworkReply::NoError) {
            if (reply->error() == QNetworkReply::HostNotFoundError) {
                emit connectionChanged(false);
            }
            if (status == 400 || status == 401) {
                emit authenticationStatusChanged(false);
            }
            emit actionExecuted(actionId, false);
            qCWarning(dcSonos()) << "Request error:" << status << reply->errorString();
            return;
        }
        emit connectionChanged(true);
        emit authenticationStatusChanged(true);
        emit actionExecuted(actionId, true);
        getGroupPlaybackStatus(groupId);
    });
    return actionId;
}

QUuid Sonos::groupSeek(const QString &groupId, int possitionMillis)
{
    Q_UNUSED(groupId)
    QNetworkRequest request;
    request.setHeader(QNetworkRequest::KnownHeaders::ContentTypeHeader, "application/json");
    request.setRawHeader("Authorization", "Bearer " + m_accessToken);
    request.setRawHeader("X-Sonos-Api-Key", m_clientKey);
    request.setUrl(QUrl(m_baseControlUrl + "/groups/" + groupId + "/playback/seek"));
    QUuid actionId = QUuid::createUuid();

    QJsonObject object;
    object.insert("positionMillis", QJsonValue::fromVariant(possitionMillis));
    QJsonDocument doc(object);

    QNetworkReply *reply = m_networkManager->post(request, doc.toJson(QJsonDocument::Compact));
    connect(reply, &QNetworkReply::finished, this, [reply, actionId, this] {
        reply->deleteLater();
        int status = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();

        // Check HTTP status code
        if (status != 200 || reply->error() != QNetworkReply::NoError) {
            if (reply->error() == QNetworkReply::HostNotFoundError) {
                emit connectionChanged(false);
            }
            if (status == 400 || status == 401) {
                emit authenticationStatusChanged(false);
            }
            emit actionExecuted(actionId, false);
            qCWarning(dcSonos()) << "Request error:" << status << reply->errorString();
            return;
        }
        emit connectionChanged(true);
        emit authenticationStatusChanged(true);
        emit actionExecuted(actionId, true);
    });
    return actionId;
}

QUuid Sonos::groupSeekRelative(const QString &groupId, int deltaMillis)
{
    QNetworkRequest request;
    request.setHeader(QNetworkRequest::KnownHeaders::ContentTypeHeader, "application/json");
    request.setRawHeader("Authorization", "Bearer " + m_accessToken);
    request.setRawHeader("X-Sonos-Api-Key", m_clientKey);
    request.setUrl(QUrl(m_baseControlUrl + "/groups/" + groupId + "/playback/seekRelative"));
    QUuid actionId = QUuid::createUuid();

    QJsonObject object;
    object.insert("deltaMillis", QJsonValue::fromVariant(deltaMillis));
    QJsonDocument doc(object);

    QNetworkReply *reply = m_networkManager->post(request, doc.toJson(QJsonDocument::Compact));
    connect(reply, &QNetworkReply::finished, this, [reply, actionId, this] {
        reply->deleteLater();
        int status = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();

        // Check HTTP status code
        if (status != 200 || reply->error() != QNetworkReply::NoError) {
            if (reply->error() == QNetworkReply::HostNotFoundError) {
                emit connectionChanged(false);
            }
            if (status == 400 || status == 401) {
                emit authenticationStatusChanged(false);
            }
            emit actionExecuted(actionId, false);
            qCWarning(dcSonos()) << "Request error:" << status << reply->errorString();
            return;
        }
        emit connectionChanged(true);
        emit authenticationStatusChanged(true);
        emit actionExecuted(actionId, true);
    });
    return actionId;
}

QUuid Sonos::groupSetPlayModes(const QString &groupId, PlayMode playMode)
{
    QNetworkRequest request;
    request.setHeader(QNetworkRequest::KnownHeaders::ContentTypeHeader, "application/json");
    request.setRawHeader("Authorization", "Bearer " + m_accessToken);
    request.setRawHeader("X-Sonos-Api-Key", m_clientKey);
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

    QNetworkReply *reply = m_networkManager->post(request, doc.toJson(QJsonDocument::Compact));
    connect(reply, &QNetworkReply::finished, this, [reply, actionId, groupId, this] {
        reply->deleteLater();
        int status = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();

        // Check HTTP status code
        if (status != 200 || reply->error() != QNetworkReply::NoError) {
            if (reply->error() == QNetworkReply::HostNotFoundError) {
                emit connectionChanged(false);
            }
            if (status == 400 || status == 401) {
                emit authenticationStatusChanged(false);
            }
            emit actionExecuted(actionId, false);
            qCWarning(dcSonos()) << "Request error:" << status << reply->errorString();
            return;
        }
        emit connectionChanged(true);
        emit authenticationStatusChanged(true);
        emit actionExecuted(actionId, true);
        getGroupPlaybackStatus(groupId);
    });
    return actionId;
}

QUuid Sonos::groupSetShuffle(const QString &groupId, bool shuffle)
{
    QNetworkRequest request;
    request.setHeader(QNetworkRequest::KnownHeaders::ContentTypeHeader, "application/json");
    request.setRawHeader("Authorization", "Bearer " + m_accessToken);
    request.setRawHeader("X-Sonos-Api-Key", m_clientKey);
    request.setUrl(QUrl(m_baseControlUrl + "/groups/" + groupId + "/playback/playMode"));
    QUuid actionId = QUuid::createUuid();

    QJsonObject object;
    QJsonObject playModesObject;
    playModesObject["shuffle"] = shuffle;
    object.insert("playModes", playModesObject);
    QJsonDocument doc(object);

    QNetworkReply *reply = m_networkManager->post(request, doc.toJson(QJsonDocument::Compact));
    connect(reply, &QNetworkReply::finished, this, [reply, actionId, groupId, this] {
        reply->deleteLater();
        int status = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();

        // Check HTTP status code
        if (status != 200 || reply->error() != QNetworkReply::NoError) {
            if (reply->error() == QNetworkReply::HostNotFoundError) {
                emit connectionChanged(false);
            }
            if (status == 400 || status == 401) {
                emit authenticationStatusChanged(false);
            }
            emit actionExecuted(actionId, false);
            qCWarning(dcSonos()) << "Request error:" << status << reply->errorString();
            return;
        }
        emit connectionChanged(true);
        emit authenticationStatusChanged(true);
        emit actionExecuted(actionId, true);
        getGroupPlaybackStatus(groupId);
    });
    return actionId;
}

QUuid Sonos::groupSetRepeat(const QString &groupId, RepeatMode repeatMode)
{
    QNetworkRequest request;
    request.setHeader(QNetworkRequest::KnownHeaders::ContentTypeHeader, "application/json");
    request.setRawHeader("Authorization", "Bearer " + m_accessToken);
    request.setRawHeader("X-Sonos-Api-Key", m_clientKey);
    request.setUrl(QUrl(m_baseControlUrl + "/groups/" + groupId + "/playback/playMode"));
    QUuid actionId = QUuid::createUuid();

    QJsonObject object;
    QJsonObject playModesObject;
    if (repeatMode == RepeatModeAll) {
        qDebug(dcSonos()) << "Setting repeat mode all";
        playModesObject["repeat"] = true;
        playModesObject["repeatOne"] = false;
    } else if (repeatMode == RepeatModeOne) {
        qDebug(dcSonos()) << "Setting repeat mode one";
        playModesObject["repeat"] = false;
        playModesObject["repeatOne"] = true;
    } else if (repeatMode == RepeatModeNone) {
        qDebug(dcSonos()) << "Setting repeat mode none";
        playModesObject["repeat"] = false;
        playModesObject["repeatOne"] = false;
    }
    object.insert("playModes", playModesObject);
    QJsonDocument doc(object);

    QNetworkReply *reply = m_networkManager->post(request, doc.toJson(QJsonDocument::Compact));
    connect(reply, &QNetworkReply::finished, this, [reply, actionId, groupId, this] {
        reply->deleteLater();
        int status = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();

        // Check HTTP status code
        if (status != 200 || reply->error() != QNetworkReply::NoError) {
            if (reply->error() == QNetworkReply::HostNotFoundError) {
                emit connectionChanged(false);
            }
            if (status == 400 || status == 401) {
                emit authenticationStatusChanged(false);
            }
            emit actionExecuted(actionId, false);
            qCWarning(dcSonos()) << "Request error:" << status << reply->errorString();
            return;
        }
        emit connectionChanged(true);
        emit authenticationStatusChanged(true);
        emit actionExecuted(actionId, true);
        getGroupPlaybackStatus(groupId);
    });
    return actionId;
}

QUuid Sonos::groupSetCrossfade(const QString &groupId, bool crossfade)
{
    QNetworkRequest request;
    request.setHeader(QNetworkRequest::KnownHeaders::ContentTypeHeader, "application/json");
    request.setRawHeader("Authorization", "Bearer " + m_accessToken);
    request.setRawHeader("X-Sonos-Api-Key", m_clientKey);
    request.setUrl(QUrl(m_baseControlUrl + "/groups/" + groupId + "/playback/playMode"));
    QUuid actionId = QUuid::createUuid();

    QJsonObject object;
    QJsonObject playModesObject;
    playModesObject["crossfade"] = crossfade;
    object.insert("playModes", playModesObject);
    QJsonDocument doc(object);

    QNetworkReply *reply = m_networkManager->post(request, doc.toJson(QJsonDocument::Compact));
    connect(reply, &QNetworkReply::finished, this, [reply, actionId, groupId, this] {
        reply->deleteLater();
        int status = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();

        // Check HTTP status code
        if (status != 200 || reply->error() != QNetworkReply::NoError) {
            if (reply->error() == QNetworkReply::HostNotFoundError) {
                emit connectionChanged(false);
            }
            if (status == 400 || status == 401) {
                emit authenticationStatusChanged(false);
            }
            emit actionExecuted(actionId, false);
            qCWarning(dcSonos()) << "Request error:" << status << reply->errorString();
            return;
        }
        emit connectionChanged(true);
        emit authenticationStatusChanged(true);
        emit actionExecuted(actionId, true);
        getGroupPlaybackStatus(groupId);
    });
    return actionId;
}

QUuid Sonos::groupSkipToNextTrack(const QString &groupId)
{
    QNetworkRequest request;
    request.setHeader(QNetworkRequest::KnownHeaders::ContentTypeHeader, "application/json");
    request.setRawHeader("Authorization", "Bearer " + m_accessToken);
    request.setRawHeader("X-Sonos-Api-Key", m_clientKey);
    request.setUrl(QUrl(m_baseControlUrl + "/groups/" + groupId + "/playback/skipToNextTrack"));
    QUuid actionId = QUuid::createUuid();

    QNetworkReply *reply = m_networkManager->post(request, "");
    connect(reply, &QNetworkReply::finished, this, [reply, actionId, groupId, this] {
        reply->deleteLater();
        int status = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();

        // Check HTTP status code
        if (status != 200 || reply->error() != QNetworkReply::NoError) {
            if (reply->error() == QNetworkReply::HostNotFoundError) {
                emit connectionChanged(false);
            }
            if (status == 400 || status == 401) {
                emit authenticationStatusChanged(false);
            }
            emit actionExecuted(actionId, false);
            qCWarning(dcSonos()) << "Request error:" << status << reply->errorString();
            return;
        }
        emit connectionChanged(true);
        emit authenticationStatusChanged(true);
        emit actionExecuted(actionId, true);
        getGroupMetadataStatus(groupId);
    });
    return actionId;
}

QUuid Sonos::groupSkipToPreviousTrack(const QString &groupId)
{
    QNetworkRequest request;
    request.setHeader(QNetworkRequest::KnownHeaders::ContentTypeHeader, "application/json");
    request.setRawHeader("Authorization", "Bearer " + m_accessToken);
    request.setRawHeader("X-Sonos-Api-Key", m_clientKey);
    request.setUrl(QUrl(m_baseControlUrl + "/groups/" + groupId + "/playback/skipToPreviousTrack"));
    QUuid actionId = QUuid::createUuid();

    QNetworkReply *reply = m_networkManager->post(request, "");
    connect(reply, &QNetworkReply::finished, this, [reply, actionId, groupId, this] {
        reply->deleteLater();
        int status = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();

        // Check HTTP status code
        if (status != 200 || reply->error() != QNetworkReply::NoError) {
            if (reply->error() == QNetworkReply::HostNotFoundError) {
                emit connectionChanged(false);
            }
            if (status == 400 || status == 401) {
                emit authenticationStatusChanged(false);
            }
            emit actionExecuted(actionId, false);
            qCWarning(dcSonos()) << "Request error:" << status << reply->errorString();
            return;
        }
        emit connectionChanged(true);
        emit authenticationStatusChanged(true);
        emit actionExecuted(actionId, true);
        getGroupMetadataStatus(groupId);
    });
    return actionId;
}

QUuid Sonos::groupTogglePlayPause(const QString &groupId)
{
    QNetworkRequest request;
    request.setHeader(QNetworkRequest::KnownHeaders::ContentTypeHeader, "application/json");
    request.setRawHeader("Authorization", "Bearer " + m_accessToken);
    request.setRawHeader("X-Sonos-Api-Key", m_clientKey);
    request.setUrl(QUrl(m_baseControlUrl + "/groups/" + groupId + "/playback/togglePlayPause"));
    QUuid actionId = QUuid::createUuid();

    QNetworkReply *reply = m_networkManager->post(request, "");
    connect(reply, &QNetworkReply::finished, this, [reply, actionId, groupId, this] {
        reply->deleteLater();
        int status = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();

        // Check HTTP status code
        if (status != 200 || reply->error() != QNetworkReply::NoError) {
            if (reply->error() == QNetworkReply::HostNotFoundError) {
                emit connectionChanged(false);
            }
            if (status == 400 || status == 401) {
                emit authenticationStatusChanged(false);
            }
            emit actionExecuted(actionId, false);
            qCWarning(dcSonos()) << "Request error:" << status << reply->errorString();
            return;
        }
        emit connectionChanged(true);
        emit authenticationStatusChanged(true);
        emit actionExecuted(actionId, true);
        getGroupPlaybackStatus(groupId);
    });
    return actionId;
}

void Sonos::getGroupMetadataStatus(const QString &groupId)
{
    QNetworkRequest request;
    request.setHeader(QNetworkRequest::KnownHeaders::ContentTypeHeader, "application/json");
    request.setRawHeader("Authorization", "Bearer " + m_accessToken);
    request.setRawHeader("X-Sonos-Api-Key", m_clientKey);
    request.setUrl(QUrl(m_baseControlUrl + "/groups/" + groupId + "/playbackMetadata"));
    QNetworkReply *reply = m_networkManager->get(request);
    connect(reply, &QNetworkReply::finished, this, [reply, groupId, this] {
        reply->deleteLater();
        int status = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();

        // Check HTTP status code
        if (status != 200 || reply->error() != QNetworkReply::NoError) {
            if (reply->error() == QNetworkReply::HostNotFoundError) {
                emit connectionChanged(false);
            }
            if (status == 400 || status == 401) {
                emit authenticationStatusChanged(false);
            }
            qCWarning(dcSonos()) << "Request error:" << status << reply->errorString();
            return;
        }
        emit connectionChanged(true);
        emit authenticationStatusChanged(true);

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
                //TODO parse ID
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
                    //TODO parse id
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
                    //TODO parse id
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
    QNetworkRequest request;
    request.setHeader(QNetworkRequest::KnownHeaders::ContentTypeHeader, "application/json");
    request.setRawHeader("Authorization", "Bearer " + m_accessToken);
    request.setRawHeader("X-Sonos-Api-Key", m_clientKey);
    request.setUrl(QUrl(m_baseControlUrl + "/players/" + playerId + "/playerVolume"));
    QNetworkReply *reply = m_networkManager->get(request);
    connect(reply, &QNetworkReply::finished, this, [reply, playerId, this] {
        reply->deleteLater();
        int status = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();

        // Check HTTP status code
        if (status != 200 || reply->error() != QNetworkReply::NoError) {
            if (reply->error() == QNetworkReply::HostNotFoundError) {
                emit connectionChanged(false);
            }
            if (status == 400 || status == 401) {
                emit authenticationStatusChanged(false);
            }
            qCWarning(dcSonos()) << "Request error:" << status << reply->errorString();
            return;
        }
        emit connectionChanged(true);
        emit authenticationStatusChanged(true);

        //qDebug(dcSonos()) << "Received response from Sonos" << reply->readAll();
        QJsonParseError error;
        QJsonDocument data = QJsonDocument::fromJson(reply->readAll(), &error);
        if (error.error != QJsonParseError::NoError) {
            qCWarning(dcSonos()) << "Json parse error" << error.errorString();
            return;
        }

        VolumeObject volume;
        QVariantMap variant = data.toVariant().toMap();
        volume.volume = variant["volume"].toInt();
        volume.muted  = variant["muted"].toBool();
        volume.fixed  = variant["fixed"].toBool();
        emit playerVolumeReceived(playerId, volume);
    });
}

QUuid Sonos::setPlayerVolume(const QByteArray &playerId, int volume)
{
    QNetworkRequest request;
    request.setHeader(QNetworkRequest::KnownHeaders::ContentTypeHeader, "application/json");
    request.setRawHeader("Authorization", "Bearer " + m_accessToken);
    request.setRawHeader("X-Sonos-Api-Key", m_clientKey);
    request.setUrl(QUrl(m_baseControlUrl + "/players/" + playerId + "/playerVolume"));
    QUuid actionId = QUuid::createUuid();

    qDebug(dcSonos()) << "Setting volume:" << playerId << volume;

    QJsonObject object;
    object.insert("volume", QJsonValue::fromVariant(volume));
    QJsonDocument doc(object);

    QNetworkReply *reply = m_networkManager->post(request, doc.toJson(QJsonDocument::Compact));
    connect(reply, &QNetworkReply::finished, this, [reply, actionId, playerId, this] {
        reply->deleteLater();
        int status = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();

        // Check HTTP status code
        if (status != 200 || reply->error() != QNetworkReply::NoError) {
            if (reply->error() == QNetworkReply::HostNotFoundError) {
                emit connectionChanged(false);
            }
            if (status == 400 || status == 401) {
                emit authenticationStatusChanged(false);
            }
            emit actionExecuted(actionId, false);
            qCWarning(dcSonos()) << "Request error:" << status << reply->errorString();
            return;
        }
        emit connectionChanged(true);
        emit authenticationStatusChanged(true);
        emit actionExecuted(actionId, true);
        getPlayerVolume(playerId);
    });
    return actionId;
}

QUuid Sonos::setPlayerRelativeVolume(const QByteArray &playerId, int volumeDelta)
{
    QNetworkRequest request;
    request.setHeader(QNetworkRequest::KnownHeaders::ContentTypeHeader, "application/json");
    request.setRawHeader("Authorization", "Bearer " + m_accessToken);
    request.setRawHeader("X-Sonos-Api-Key", m_clientKey);
    request.setUrl(QUrl(m_baseControlUrl + "/players/" + playerId + "/playerVolume/relative"));
    QUuid actionId = QUuid::createUuid();

    QJsonObject object;
    object.insert("volumeDelta", QJsonValue::fromVariant(volumeDelta));
    QJsonDocument doc(object);

    QNetworkReply *reply = m_networkManager->post(request, doc.toJson(QJsonDocument::Compact));
    connect(reply, &QNetworkReply::finished, this, [reply, actionId, playerId, this] {
        reply->deleteLater();
        int status = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();

        // Check HTTP status code
        if (status != 200 || reply->error() != QNetworkReply::NoError) {
            if (reply->error() == QNetworkReply::HostNotFoundError) {
                emit connectionChanged(false);
            }
            if (status == 400 || status == 401) {
                emit authenticationStatusChanged(false);
            }
            emit actionExecuted(actionId, false);
            qCWarning(dcSonos()) << "Request error:" << status << reply->errorString();
            return;
        }
        emit connectionChanged(true);
        emit authenticationStatusChanged(true);
        emit actionExecuted(actionId, true);
        getPlayerVolume(playerId);
    });
    return actionId;
}

QUuid Sonos::setPlayerMute(const QByteArray &playerId, bool mute)
{
    QNetworkRequest request;
    request.setHeader(QNetworkRequest::KnownHeaders::ContentTypeHeader, "application/json");
    request.setRawHeader("Authorization", "Bearer " + m_accessToken);
    request.setRawHeader("X-Sonos-Api-Key", m_clientKey);
    request.setUrl(QUrl(m_baseControlUrl + "/players/" + playerId + "/playerVolume"));
    QUuid actionId = QUuid::createUuid();

    QJsonObject object;
    object.insert("muted", QJsonValue::fromVariant(mute));
    QJsonDocument doc(object);

    QNetworkReply *reply = m_networkManager->post(request, doc.toJson(QJsonDocument::Compact));
    connect(reply, &QNetworkReply::finished, this, [reply, actionId, playerId, this] {
        reply->deleteLater();
        int status = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();

        // Check HTTP status code
        if (status != 200 || reply->error() != QNetworkReply::NoError) {
            if (reply->error() == QNetworkReply::HostNotFoundError) {
                emit connectionChanged(false);
            }
            if (status == 400 || status == 401) {
                emit authenticationStatusChanged(false);
            }
            emit actionExecuted(actionId, false);
            qCWarning(dcSonos()) << "Request error:" << status << reply->errorString();
            return;
        }
        emit connectionChanged(true);
        emit authenticationStatusChanged(true);
        emit actionExecuted(actionId, true);
        getPlayerVolume(playerId);
    });
    return actionId;
}

void Sonos::getPlaylist(const QString &householdId, const QString &playlistId)
{
    QNetworkRequest request;
    request.setHeader(QNetworkRequest::KnownHeaders::ContentTypeHeader, "application/json");
    request.setRawHeader("Authorization", "Bearer " + m_accessToken);
    request.setRawHeader("X-Sonos-Api-Key", m_clientKey);
    request.setUrl(QUrl(m_baseControlUrl + "/households/" + householdId + "/playlists/getPlaylist"));


    QJsonObject object;
    object["playlistId"] = playlistId;
    QJsonDocument doc(object);

    QNetworkReply *reply = m_networkManager->post(request, doc.toJson(QJsonDocument::Compact));
    connect(reply, &QNetworkReply::finished, this, [reply, householdId, this] {
        reply->deleteLater();
        int status = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();

        // Check HTTP status code
        if (status != 200 || reply->error() != QNetworkReply::NoError) {
            if (reply->error() == QNetworkReply::HostNotFoundError) {
                emit connectionChanged(false);
            }
            if (status == 400 || status == 401) {
                emit authenticationStatusChanged(false);
            }
            qCWarning(dcSonos()) << "Request error:" << status << reply->errorString();
            return;
        }
        emit connectionChanged(true);
        emit authenticationStatusChanged(true);

        //qDebug(dcSonos()) << "Received response from Sonos" << reply->readAll();
        QJsonParseError error;
        QJsonDocument data = QJsonDocument::fromJson(reply->readAll(), &error);
        if (error.error != QJsonParseError::NoError) {
            qCWarning(dcSonos()) << "Json parse error" << error.errorString();
            return;
        }

        QVariantMap variant = data.toVariant().toMap();

        if (!variant.contains("tracks"))
            return;

        PlaylistSummaryObject playlist;
        QVariantList array = variant["tracks"].toList();
        foreach (const QVariant &value, array) {
            QVariantMap itemObject = value.toMap();
            PlaylistTrackObject track;
            track.name = itemObject["name"].toString();
            track.album = itemObject["album"].toString();
            track.artist= itemObject["artist"].toString();
            playlist.tracks.append(track);
        }
        emit playlistSummaryReceived(householdId, playlist);
    });
}

void Sonos::getPlaylists(const QString &householdId)
{
    QNetworkRequest request;
    request.setHeader(QNetworkRequest::KnownHeaders::ContentTypeHeader, "application/json");
    request.setRawHeader("Authorization", "Bearer " + m_accessToken);
    request.setRawHeader("X-Sonos-Api-Key", m_clientKey);
    request.setUrl(QUrl(m_baseControlUrl + "/households/" + householdId + "/playlists"));
    QNetworkReply *reply = m_networkManager->get(request);
    connect(reply, &QNetworkReply::finished, this, [reply, householdId, this] {
        reply->deleteLater();
        int status = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();

        // Check HTTP status code
        if (status != 200 || reply->error() != QNetworkReply::NoError) {
            if (reply->error() == QNetworkReply::HostNotFoundError) {
                emit connectionChanged(false);
            }
            if (status == 400 || status == 401) {
                emit authenticationStatusChanged(false);
            }
            qCWarning(dcSonos()) << "Request error:" << status << reply->errorString();
            return;
        }
        emit connectionChanged(true);
        emit authenticationStatusChanged(true);

        //qDebug(dcSonos()) << "Received response from Sonos" << reply->readAll();
        QJsonParseError error;
        QJsonDocument data = QJsonDocument::fromJson(reply->readAll(), &error);
        if (!data.isObject()) {
            qCWarning(dcSonos()) << "Json parse error:" << error.errorString();
            return;
        }

        if (!data.toVariant().toMap().contains("playlists"))
            return;

        QVariantList array = data.toVariant().toMap().value("playlists").toList();
        QList<PlaylistObject> playlists;
        foreach (const QVariant &value, array) {
            QVariantMap itemObject = value.toMap();
            PlaylistObject playlist;
            playlist.id = itemObject["id"].toString();
            playlist.name = itemObject["name"].toString();
            playlist.type = itemObject["type"].toString();
            playlist.trackCount = itemObject["trackCount"].toString();
            playlists.append(playlist);
        }
        emit playlistsReceived(householdId, playlists);
    });
}

QUuid Sonos::loadPlaylist(const QString &groupId, const QString &playlistId)
{
    QNetworkRequest request;
    request.setHeader(QNetworkRequest::KnownHeaders::ContentTypeHeader, "application/json");
    request.setRawHeader("Authorization", "Bearer " + m_accessToken);
    request.setRawHeader("X-Sonos-Api-Key", m_clientKey);
    request.setUrl(QUrl(m_baseControlUrl + "/groups/" + groupId + "/playlists"));
    QUuid actionId = QUuid::createUuid();

    QJsonObject object;
    object["playlistId"] = playlistId;
    QJsonDocument doc(object);

    QNetworkReply *reply = m_networkManager->post(request, doc.toJson(QJsonDocument::Compact));
    connect(reply, &QNetworkReply::finished, this, [reply, actionId, this] {
        reply->deleteLater();
        int status = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();

        // Check HTTP status code
        if (status != 200 || reply->error() != QNetworkReply::NoError) {
            if (reply->error() == QNetworkReply::HostNotFoundError) {
                emit connectionChanged(false);
            }
            if (status == 400 || status == 401) {
                emit authenticationStatusChanged(false);
            }
            emit actionExecuted(actionId, false);
            qCWarning(dcSonos()) << "Request error:" << status << reply->errorString();
            return;
        }
        emit connectionChanged(true);
        emit authenticationStatusChanged(true);
        emit actionExecuted(actionId, true);
    });
    return actionId;
}

void Sonos::getPlayerSettings(const QString &playerId)
{
    QNetworkRequest request;
    request.setHeader(QNetworkRequest::KnownHeaders::ContentTypeHeader, "application/json");
    request.setRawHeader("Authorization", "Bearer " + m_accessToken);
    request.setRawHeader("X-Sonos-Api-Key", m_clientKey);
    request.setUrl(QUrl(m_baseControlUrl + "/players/" + playerId + "/settings/player"));
    QNetworkReply *reply = m_networkManager->get(request);
    connect(reply, &QNetworkReply::finished, this, [reply, playerId, this] {
        reply->deleteLater();
        int status = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();

        // Check HTTP status code
        if (status != 200 || reply->error() != QNetworkReply::NoError) {
            if (reply->error() == QNetworkReply::HostNotFoundError) {
                emit connectionChanged(false);
            }
            if (status == 400 || status == 401) {
                emit authenticationStatusChanged(false);
            }
            qCWarning(dcSonos()) << "Request error:" << status << reply->errorString();
            return;
        }
        emit connectionChanged(true);
        emit authenticationStatusChanged(true);

        QJsonParseError error;
        QJsonDocument jsonDoc = QJsonDocument::fromJson(reply->readAll(), &error);
        if (error.error != QJsonParseError::NoError) {
            qCWarning(dcSonos()) << "Json parse error" << error.errorString();
            return;
        }

        QVariantMap data = jsonDoc.toVariant().toMap();
        PlayerSettingsObject playerSettings;
        playerSettings.monoMode = data["monoMode"].toBool();
        playerSettings.volumeMode = data["volumeMode"].toString();
        playerSettings.wifiDisabled =  data["wifiDisable"].toBool();
        playerSettings.volumeScalingFactor =  data["wifiDisable"].toDouble();
        emit playerSettingsRecieved(playerId, playerSettings);
    });
}

QUuid Sonos::setPlayerSettings(const QString &playerId, PlayerSettingsObject settings)
{
    QNetworkRequest request;
    request.setHeader(QNetworkRequest::KnownHeaders::ContentTypeHeader, "application/json");
    request.setRawHeader("Authorization", "Bearer " + m_accessToken);
    request.setRawHeader("X-Sonos-Api-Key", m_clientKey);
    request.setUrl(QUrl(m_baseControlUrl + "/players/" + playerId + "/settings/player"));
    QUuid actionId = QUuid::createUuid();

    QJsonObject object;
    object["volumeMode"] = settings.volumeMode;
    object["volumeScalingFactor"] = settings.volumeScalingFactor;
    object["monoMode"] = settings.monoMode;
    object["wifiDisable"] = settings.wifiDisabled;
    QJsonDocument doc(object);

    QNetworkReply *reply = m_networkManager->post(request, doc.toJson(QJsonDocument::Compact));
    connect(reply, &QNetworkReply::finished, this, [reply, actionId, playerId, this] {
        reply->deleteLater();
        int status = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();

        // Check HTTP status code
        if (status != 200 || reply->error() != QNetworkReply::NoError) {
            if (reply->error() == QNetworkReply::HostNotFoundError) {
                emit connectionChanged(false);
            }
            if (status == 400 || status == 401) {
                emit authenticationStatusChanged(false);
            }
            emit actionExecuted(actionId, false);
            qCWarning(dcSonos()) << "Request error:" << status << reply->errorString();
            return;
        }
        emit connectionChanged(true);
        emit authenticationStatusChanged(true);
        emit actionExecuted(actionId, true);
        getPlayerSettings(playerId);
    });
    return actionId;
}

void Sonos::onRefreshTimeout()
{
    qCDebug(dcSonos) << "Refresh authentication token";
    getAccessTokenFromRefreshToken(m_refreshToken);
}


void Sonos::getAccessTokenFromRefreshToken(const QByteArray &refreshToken)
{
    if (refreshToken.isEmpty()) {
        qWarning(dcSonos()) << "No refresh token given!";
        emit authenticationStatusChanged(false);
        return;
    }

    QUrl url(m_baseAuthorizationUrl);
    QUrlQuery query;
    query.clear();
    query.addQueryItem("grant_type", "refresh_token");
    query.addQueryItem("refresh_token", refreshToken);
    url.setQuery(query);

    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/x-www-form-urlencoded; charset=UTF-8");

    QByteArray auth = QByteArray(m_clientKey + ':' + m_clientSecret).toBase64(QByteArray::Base64Encoding | QByteArray::KeepTrailingEquals);
    request.setRawHeader("Authorization", QString("Basic %1").arg(QString(auth)).toUtf8());

    QNetworkReply *reply = m_networkManager->post(request, QByteArray());
    connect(reply, &QNetworkReply::finished, this, [this, reply](){
        reply->deleteLater();

        QJsonDocument jsonDoc = QJsonDocument::fromJson(reply->readAll());
        int status = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
        if (status != 200 || reply->error() != QNetworkReply::NoError) {
            if(jsonDoc.toVariant().toMap().contains("error_description")) {
                qWarning(dcSonos()) << "Access token error:" << jsonDoc.toVariant().toMap().value("error_description").toString();
            }
            emit authenticationStatusChanged(false);
            return;
        }
        if(!jsonDoc.toVariant().toMap().contains("access_token")) {
            emit authenticationStatusChanged(false);
            return;
        }
        m_accessToken = jsonDoc.toVariant().toMap().value("access_token").toByteArray();

        if (jsonDoc.toVariant().toMap().contains("expires_in")) {
            int expireTime = jsonDoc.toVariant().toMap().value("expires_in").toInt();
            qCDebug(dcSonos()) << "Access token expires at" << QDateTime::currentDateTime().addSecs(expireTime).toString();
            if (!m_tokenRefreshTimer) {
                qWarning(dcSonos()) << "Access token refresh timer not initialized";
                return;
            }
            m_tokenRefreshTimer->start((expireTime - 20) * 1000);
        }
        emit authenticationStatusChanged(true);;
    });
}

void Sonos::getAccessTokenFromAuthorizationCode(const QByteArray &authorizationCode)
{
    // Obtaining access token
    if(authorizationCode.isEmpty())
        qWarning(dcSonos) << "No auhtorization code given!";
    if(m_clientKey.isEmpty())
        qWarning(dcSonos) << "Client key not set!";
    if(m_clientSecret.isEmpty())
        qWarning(dcSonos) << "Client secret not set!";

    QUrl url = QUrl(m_baseAuthorizationUrl);
    QUrlQuery query;
    query.clear();
    query.addQueryItem("grant_type", "authorization_code");
    query.addQueryItem("code", authorizationCode);
    query.addQueryItem("redirect_uri", m_redirectUri);
    url.setQuery(query);

    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/x-www-form-urlencoded;charset=utf-8");

    QByteArray auth = QByteArray(m_clientKey + ':' + m_clientSecret).toBase64(QByteArray::Base64Encoding | QByteArray::KeepTrailingEquals);
    request.setRawHeader("Authorization", QString("Basic %1").arg(QString(auth)).toUtf8());

    QNetworkReply *reply = m_networkManager->post(request, QByteArray());
    connect(reply, &QNetworkReply::finished, this, [this, reply](){
        reply->deleteLater();
        QJsonDocument jsonDoc = QJsonDocument::fromJson(reply->readAll());

        int status = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
        switch (status){
        case 400:
            if(!jsonDoc.toVariant().toMap().contains("error")) {
                if(jsonDoc.toVariant().toMap().value("error").toString() == "invalid_client") {
                    qWarning(dcSonos()) << "Client token provided doesn’t correspond to client that generated auth code.";
                }
                if(jsonDoc.toVariant().toMap().value("error").toString() == "invalid_redirect_uri") {
                    qWarning(dcSonos()) << "Missing redirect_uri parameter.";
                }
                if(jsonDoc.toVariant().toMap().value("error").toString() == "invalid_code") {
                    qWarning(dcSonos()) << "Expired authorization code.";
                }
            }
            return;
        case 401:
            qWarning(dcSonos()) << "Client does not have permission to use this API.";
            return;
        case 405:
            qWarning(dcSonos()) << "Wrong HTTP method used.";
            return;
        default:
            break;
        }
        qCDebug(dcSonos()) << "Sonos accessToken reply:" << this << reply->error() << reply->errorString() << jsonDoc.toJson();
        if(!jsonDoc.toVariant().toMap().contains("access_token") || !jsonDoc.toVariant().toMap().contains("refresh_token") ) {
            emit authenticationStatusChanged(false);
            return;
        }
        qCDebug(dcSonos()) << "Access token:" << jsonDoc.toVariant().toMap().value("access_token").toString();
        m_accessToken = jsonDoc.toVariant().toMap().value("access_token").toByteArray();

        qCDebug(dcSonos()) << "Refresh token:" << jsonDoc.toVariant().toMap().value("refresh_token").toString();
        m_refreshToken = jsonDoc.toVariant().toMap().value("refresh_token").toByteArray();

        if (jsonDoc.toVariant().toMap().contains("expires_in")) {
            int expireTime = jsonDoc.toVariant().toMap().value("expires_in").toInt();
            qCDebug(dcSonos()) << "expires at" << QDateTime::currentDateTime().addSecs(expireTime).toString();
            if (!m_tokenRefreshTimer) {
                qWarning(dcSonos()) << "Token refresh timer not initialized";
                emit authenticationStatusChanged(false);
                return;
            }
            m_tokenRefreshTimer->start((expireTime - 20) * 1000);
        }
        emit authenticationStatusChanged(true);
    });
}
