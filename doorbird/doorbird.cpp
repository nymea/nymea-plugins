// SPDX-License-Identifier: GPL-3.0-or-later

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*
* Copyright (C) 2013 - 2024, nymea GmbH
* Copyright (C) 2024 - 2025, chargebyte austria GmbH
*
* This file is part of nymea-plugins.
*
* nymea-plugins is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* (at your option) any later version.
*
* nymea-plugins is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
* General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with nymea-plugins. If not, see <https://www.gnu.org/licenses/>.
*
* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

#include "doorbird.h"
#include "extern-plugininfo.h"

#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QAuthenticator>
#include <QTimer>
#include <QImage>
#include <QUrl>
#include <QUrlQuery>

Doorbird::Doorbird(const QHostAddress &address, QObject *parent) :
    QObject(parent),
    m_address(address)
{
    m_networkAccessManager = new QNetworkAccessManager(this);
}

QHostAddress Doorbird::address()
{
    return m_address;
}

void Doorbird::setAddress(const QHostAddress &address)
{
    m_address = address;
}

QUuid Doorbird::getSession(const QString &username, const QString &password)
{
    QUrl url;
    url.setHost(m_address.toString());
    url.setScheme("http");
    url.setPath("/bha-api/getsession.cgi");
    url.setUserName(username);
    url.setPassword(password);
    QNetworkRequest request(url);
    qCDebug(dcDoorBird) << "Sending request:" << request.url();
    QNetworkReply *reply = m_networkAccessManager->get(request);
    QUuid requestId = QUuid::createUuid();

    connect(reply, &QNetworkReply::finished, reply, &QNetworkReply::deleteLater);
    connect(reply, &QNetworkReply::finished, this, [this, reply, requestId](){

        if (reply->error() != QNetworkReply::NoError) {
            qCWarning(dcDoorBird) << "Error DoorBird thing:" << reply->errorString();
            emit requestSent(requestId, false);
            return;
        }
        emit requestSent(requestId, true);
        QByteArray data = reply->readAll();
        QJsonParseError error;
        QJsonDocument jsonDoc = QJsonDocument::fromJson(data, &error);
        if (error.error != QJsonParseError::NoError) {
            qCWarning(dcDoorBird()) << "Error parsing json:" << data;
            return;
        }
        QVariantMap map = jsonDoc.toVariant().toMap().value("BHA").toMap();
        if (map.contains("SESSIONID")) {
            QString sessionId = map.value("SESSIONID").toString();
            qCDebug(dcDoorBird) << "Got sessionId" << sessionId;
            emit sessionIdReceived(sessionId);
        }
    });

    return requestId;
}

QUuid Doorbird::openDoor(int value)
{
    QNetworkRequest request(QString("http://%1/bha-api/open-door.cgi?r=%2").arg(m_address.toString()).arg(QString::number(value)));
    qCDebug(dcDoorBird) << "Sending request:" << request.url();
    QNetworkReply *reply = m_networkAccessManager->get(request);
    QUuid requestId = QUuid::createUuid();
    connect(reply, &QNetworkReply::finished, reply, &QNetworkReply::deleteLater);
    connect(reply, &QNetworkReply::finished, this, [this, reply, requestId](){

        if (reply->error() != QNetworkReply::NoError) {
            qCWarning(dcDoorBird) << "Error opening DoorBird "  << reply->error() << reply->errorString();
            emit requestSent(requestId, false);
            return;
        }
        qCDebug(dcDoorBird) << "DoorBird unlatched:";
        emit requestSent(requestId, true);
    });
    return requestId;
}

QUuid Doorbird::lightOn()
{
    QNetworkRequest request(QString("http://%1/bha-api/light-on.cgi").arg(m_address.toString()));
    qCDebug(dcDoorBird) << "Sending request:" << request.url();
    QNetworkReply *reply = m_networkAccessManager->get(request);
    QUuid requestId = QUuid::createUuid();
    connect(reply, &QNetworkReply::finished, reply, &QNetworkReply::deleteLater);
    connect(reply, &QNetworkReply::finished, this, [this, reply, requestId](){

        if (reply->error() != QNetworkReply::NoError) {
            qCWarning(dcDoorBird) << "Error light on"  << reply->error() << reply->errorString();
            emit requestSent(requestId, false);
            return;
        }
        qCDebug(dcDoorBird) << "DoorBird light on:";
        emit requestSent(requestId, true);
    });
    return requestId;
}

QUuid Doorbird::liveVideoRequest()
{
    QNetworkRequest request(QString("http://%1/bha-api/video.cgi").arg(m_address.toString()));
    qCDebug(dcDoorBird) << "Sending request:" << request.url();
    QNetworkReply *reply = m_networkAccessManager->get(request);
    QUuid requestId = QUuid::createUuid();
    connect(reply, &QNetworkReply::finished, reply, &QNetworkReply::deleteLater);
    connect(reply, &QNetworkReply::finished, this, [this, reply, requestId](){

        if (reply->error() != QNetworkReply::NoError) {
            qCWarning(dcDoorBird) << "Error live video request" << reply->error() << reply->errorString();
            emit requestSent(requestId, false);
            return;
        }
        qCDebug(dcDoorBird) << "DoorBird live video request" ;
        emit requestSent(requestId, true);
    });
    return requestId;
}

QUuid Doorbird::liveImageRequest()
{
    QNetworkRequest request(QString("http://%1/bha-api/image.cgi").arg(m_address.toString()));
    qCDebug(dcDoorBird) << "Sending request:" << request.url();
    QNetworkReply *reply = m_networkAccessManager->get(request);
    QUuid requestId = QUuid::createUuid();
    connect(reply, &QNetworkReply::finished, reply, &QNetworkReply::deleteLater);
    connect(reply, &QNetworkReply::finished, this, [this, reply, requestId](){

        if (reply->error() != QNetworkReply::NoError) {
            qCWarning(dcDoorBird) << "Error live image request"  << reply->error() << reply->errorString();
            emit requestSent(requestId, false);
            return;
        }

        QByteArrayList tokens = reply->readAll().split('\n');
        QImage image = QImage::fromData(tokens.last());
        //image.save("testfile");
        emit liveImageReceived(image);
        qCDebug(dcDoorBird) << "DoorBird live image request:";
        emit requestSent(requestId, true);
    });
    return requestId;
}

QUuid Doorbird::historyImageRequest(int index)
{
    QUrl url(QString("http://%1/bha-api/history.cgi").arg(m_address.toString()));
    QUrlQuery query;
    query.addQueryItem("index", QString::number(index));
    url.setQuery(query);

    qCDebug(dcDoorBird) << "Sending request:" << url;
    QNetworkReply *reply = m_networkAccessManager->get(QNetworkRequest(url));
    QUuid requestId = QUuid::createUuid();
    connect(reply, &QNetworkReply::finished, reply, &QNetworkReply::deleteLater);
    connect(reply, &QNetworkReply::finished, this, [this, reply, requestId](){

        if (reply->error() != QNetworkReply::NoError) {
            qCWarning(dcDoorBird) << "Error history image request" << reply->error() << reply->errorString();
            emit requestSent(requestId, false);
            return;
        }
        qCDebug(dcDoorBird) << "DoorBird history image received:";
        emit requestSent(requestId, true);
    });
    return requestId;
}

QUuid Doorbird::liveAudioReceive()
{
    QNetworkRequest request(QString("http://%1/bha-api/audio-receive.cgi").arg(m_address.toString()));
    qCDebug(dcDoorBird) << "Sending request:" << request.url();
    QNetworkReply *reply = m_networkAccessManager->get(request);
    QUuid requestId = QUuid::createUuid();
    connect(reply, &QNetworkReply::finished, reply, &QNetworkReply::deleteLater);
    connect(reply, &QNetworkReply::finished, this, [this, reply, requestId](){

        if (reply->error() != QNetworkReply::NoError) {
            qCWarning(dcDoorBird) << "Error live audio receive";
            emit requestSent(requestId, false);
            return;
        }
        qCDebug(dcDoorBird) << "DoorBird live audio receive";
        emit requestSent(requestId, true);
    });
    return requestId;
}

QUuid Doorbird::liveAudioTransmit()
{
    return QUuid::createUuid();
}

QUuid Doorbird::infoRequest()
{
    QNetworkRequest request(QString("http://%1/bha-api/info.cgi").arg(m_address.toString()));
    qCDebug(dcDoorBird) << "Sending request:" << request.url();
    QNetworkReply *reply = m_networkAccessManager->get(request);
    QUuid requestId = QUuid::createUuid();
    connect(reply, &QNetworkReply::finished, reply, &QNetworkReply::deleteLater);
    connect(reply, &QNetworkReply::finished, this, [this, reply, requestId](){

        if (reply->error() != QNetworkReply::NoError) {
            qCWarning(dcDoorBird) << "Error DoorBird" << reply->error() << reply->errorString();
            emit requestSent(requestId, false);
            return;
        }
        qCDebug(dcDoorBird) << "DoorBird info:" << reply->readAll() ;
        emit requestSent(requestId, true);
    });
    return requestId;
}

QUuid Doorbird::listFavorites()
{
    QNetworkRequest request(QString("http://%1/bha-api/favorites.cgi").arg(m_address.toString()));
    qCDebug(dcDoorBird) << "Sending request:" << request.url();
    QNetworkReply *reply = m_networkAccessManager->get(request);
    QUuid requestId = QUuid::createUuid();
    connect(reply, &QNetworkReply::finished, reply, &QNetworkReply::deleteLater);
    connect(reply, &QNetworkReply::finished, this, [this, reply, requestId](){

        if (reply->error() != QNetworkReply::NoError) {
            qCWarning(dcDoorBird) << "Error DoorBird device" << reply->error() << reply->errorString();
            emit requestSent(requestId, false);
            return;
        }
        emit requestSent(requestId, true);
    });
    return requestId;
}

QUuid Doorbird::addFavorite(FavoriteType type, const QString &name, const QUrl &commandUrl, int id)
{
    QUrl url(QString("http://%1/bha-api/favorites.cgi").arg(m_address.toString()));
    QUrlQuery query;
    query.addQueryItem("action", "save");
    if (type == FavoriteType::Http) {
        query.addQueryItem("type", "http");
    } else {
        query.addQueryItem("type", "sip");
    }
    query.addQueryItem("title", name);
    query.addQueryItem("value", commandUrl.toString());
    query.addQueryItem("id", QString::number(id));
    url.setQuery(query);

    QNetworkReply *reply = m_networkAccessManager->get(QNetworkRequest(url));
    QUuid requestId = QUuid::createUuid();
    connect(reply, &QNetworkReply::finished, reply, &QNetworkReply::deleteLater);
    connect(reply, &QNetworkReply::finished, this, [this, reply, requestId](){

        if (reply->error() != QNetworkReply::NoError) {
            qCWarning(dcDoorBird) << "Error DoorBird device" << reply->error() << reply->errorString();
            emit requestSent(requestId, false);
            return;
        }
        emit requestSent(requestId, true);
    });
    return requestId;
}

QUuid Doorbird::listSchedules()
{
    QNetworkRequest request(QString("http://%1/bha-api/schedule.cgi").arg(m_address.toString()));
    qCDebug(dcDoorBird) << "Sending request:" << request.url();
    QNetworkReply *reply = m_networkAccessManager->get(request);
    QUuid requestId = QUuid::createUuid();
    connect(reply, &QNetworkReply::finished, reply, &QNetworkReply::deleteLater);
    connect(reply, &QNetworkReply::finished, this, [this, reply, requestId](){

        if (reply->error() != QNetworkReply::NoError) {
            qCWarning(dcDoorBird) << "Error DoorBird device" << reply->error() << reply->errorString();
            emit requestSent(requestId, false);
            return;
        }
        emit requestSent(requestId, true);
    });
    return requestId;
}

QUuid Doorbird::restart()
{
    QNetworkRequest request(QString("http://%1/bha-api/restart.cgi").arg(m_address.toString()));
    qCDebug(dcDoorBird) << "Sending request:" << request.url();
    QNetworkReply *reply = m_networkAccessManager->get(request);
    QUuid requestId = QUuid::createUuid();
    connect(reply, &QNetworkReply::finished, reply, &QNetworkReply::deleteLater);
    connect(reply, &QNetworkReply::finished, this, [this, reply, requestId](){

        if (reply->error() != QNetworkReply::NoError) {
            qCWarning(dcDoorBird) << "Error restarting DoorBird device" << reply;
            emit requestSent(requestId, false);
            return;
        }
        qCDebug(dcDoorBird) << "DoorBird restarting";
        emit requestSent(requestId, true);
    });
    return requestId;
}

void Doorbird::connectToEventMonitor()
{
    qCDebug(dcDoorBird) << "Starting monitoring";

    QNetworkRequest request(QString("http://%1/bha-api/monitor.cgi?ring=doorbell,motionsensor").arg(m_address.toString()));
    QNetworkReply *reply = m_networkAccessManager->get(request);

    connect(reply, &QNetworkReply::downloadProgress, this, [this, reply](qint64 bytesReceived, qint64 bytesTotal){
        Q_UNUSED(bytesReceived)
        Q_UNUSED(bytesTotal);

        emit deviceConnected(true);
        m_readBuffer.append(reply->readAll());
        qCDebug(dcDoorBird) << "Event received" << m_readBuffer;

        // Input data looks like:
        // "--ioboundary\r\nContent-Type: text/plain\r\n\r\ndoorbell:H\r\n\r\n"

        while (!m_readBuffer.isEmpty()) {
            // find next --ioboundary
            QString boundary = QStringLiteral("--ioboundary");
            int startIndex = m_readBuffer.indexOf(boundary.toUtf8());
            if (startIndex == -1) {
                qCWarning(dcDoorBird) << "No meaningful data in buffer:" << m_readBuffer;
                if (m_readBuffer.size() > 1024) {
                    qCWarning(dcDoorBird) << "Buffer size > 1KB and still no meaningful data. Discarding buffer...";
                    m_readBuffer.clear();
                }
                // Assuming we don't have enough data yet...
                return;
            }

            QByteArray contentType = QByteArrayLiteral("Content-Type: text/plain");
            int contentTypeIndex = m_readBuffer.indexOf(contentType);
            if (contentTypeIndex == -1) {
                qCWarning(dcDoorBird) << "Cannot find Content-Type in buffer:" << m_readBuffer;
                if (m_readBuffer.size() > startIndex + 50) {
                    qCWarning(dcDoorBird) << boundary << "found but unexpected data follows. Skipping this element...";
                    m_readBuffer.remove(0, startIndex + boundary.length());
                    continue;
                }
                // Assuming we don't have enough data yet...
                return;
            }

            // At this point we have the boundary and Content-Type. Remove all of that and take the entire string to either end or next boundary
            m_readBuffer.remove(0, contentTypeIndex + contentType.length());
            int nextStartIndex = m_readBuffer.indexOf(boundary.toUtf8());
            QByteArray data;
            if (nextStartIndex == -1) {
                data = m_readBuffer;
                m_readBuffer.clear();
            } else {
                data = m_readBuffer.left(nextStartIndex);
                m_readBuffer.remove(0, nextStartIndex);
            }

            QString message = data.trimmed();
            QStringList parts = message.split(":");
            if (parts.count() != 2) {
                qCWarning(dcDoorBird) << "Message has invalid format:" << message << "Expected device:state";
                continue;
            }
            if (parts.first() == "doorbell") {
                if (parts.at(1) == "H") {
                    qCDebug(dcDoorBird) << "Doorbell ringing!";
                    emit eventReveiced(EventType::Doorbell, true);
                } else {
                    emit eventReveiced(EventType::Doorbell, false);
                }
            } else if (parts.first() == "motionsensor") {
                if (parts.at(1) == "H") {
                    qCDebug(dcDoorBird) << "Motion sensor detected a person";
                    emit eventReveiced(EventType::Motion, true);
                } else {
                    emit eventReveiced(EventType::Motion, false);
                }
            } else {
                qCWarning(dcDoorBird) << "Unhandled DoorBird data:" << message;
            }
        }
        qCDebug(dcDoorBird()) << "Event read buffer size" << m_readBuffer.size();
    });

    connect(reply, &QNetworkReply::finished, reply, &QNetworkReply::deleteLater);
    connect(reply, &QNetworkReply::finished, this, [this, reply]() {

        emit deviceConnected(false);
        m_readBuffer.clear();
        qCDebug(dcDoorBird) << "Monitor request finished:" << reply->error();
        qCDebug(dcDoorBird) << "    - Trying to reconnect in 5 seconds";
        QTimer::singleShot(2000, this, [this] {
            qCDebug(dcDoorBird) << "    - Reconnecting now";
            connectToEventMonitor();
        });
    });
}
