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

#ifndef DOORBIRD_H
#define DOORBIRD_H

#include <QObject>
#include <QHostAddress>
#include <QNetworkAccessManager>
#include <QUuid>
#include <QImage>

class Doorbird : public QObject
{
    Q_OBJECT
public:
    explicit Doorbird(const QHostAddress &address, QObject *parent = nullptr);

    enum EventType {
        Doorbell,
        Motion,
        Input,
        Rfid
    };
    enum FavoriteType {
        Http,
        Sip
    };

    struct FavoriteObject {
        FavoriteType type;
        QString title;
        QUrl value;
        int id;
    };

    QHostAddress address();
    void setAddress(const QHostAddress &address);
    QUuid getSession(const QString &username, const QString &password);
    QUuid openDoor(int value);
    QUuid lightOn();
    QUuid liveVideoRequest();
    QUuid liveImageRequest();
    QUuid historyImageRequest(int index);

    QUuid liveAudioReceive();
    QUuid liveAudioTransmit();
    QUuid infoRequest();

    QUuid listFavorites();
    QUuid addFavorite(FavoriteType type, const QString &name, const QUrl &url, int id);
    QUuid deleteFavorite();

    QUuid listSchedules();
    QUuid addScheduleEntry(EventType event, int favoriteNumber, bool enabled);
    QUuid deleteScheduleEntry();

    QUuid restart();

    void connectToEventMonitor();
private:
    QHostAddress m_address;
    QNetworkAccessManager *m_networkAccessManager = nullptr;
    QByteArray m_readBuffer;

    QList<QNetworkReply *> m_networkRequests;
    QList<QNetworkReply *> m_pendingAuthentications;

    QString m_username;
    QString m_password;

signals:
    void deviceConnected(bool status);
    void requestSent(QUuid requestId, bool success);

    void eventReveiced(EventType eventType, bool status);
    void favoritesReceived(QList<Doorbird::FavoriteObject> favourites);

    void sessionIdReceived(const QString &sessionId);
    void liveImageReceived(QImage image);

};

#endif // DOORBIRD_H
