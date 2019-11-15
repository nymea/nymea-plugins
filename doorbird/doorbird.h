/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *                                                                         *
 *  Copyright (C) 2019 Bernhard Trinnes <bernhard.trinnes@nymea.io>        *
 *  Copyright (C) 2019 Michael Zanetti <michael.zanetti@nymea.io>          *
 *                                                                         *
 *  This file is part of nymea.                                            *
 *                                                                         *
 *  nymea is free software: you can redistribute it and/or modify          *
 *  it under the terms of the GNU General Public License as published by   *
 *  the Free Software Foundation, version 2 of the License.                *
 *                                                                         *
 *  nymea is distributed in the hope that it will be useful,               *
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of         *
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the           *
 *  GNU General Public License for more details.                           *
 *                                                                         *
 *  You should have received a copy of the GNU General Public License      *
 *  along with nymea. If not, see <http://www.gnu.org/licenses/>.          *
 *                                                                         *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

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
    explicit Doorbird(const QHostAddress &address, const QString &username, const QString &password, QObject *parent = nullptr);

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

    QUuid getSession();
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
    QNetworkAccessManager *m_networkAccessManager;
    QByteArray m_readBuffer;

    QHostAddress m_address;
    QList<QNetworkReply *> m_networkRequests;

    QString m_username;
    QString m_password;

    QByteArray sessionId;

signals:
    void deviceConnected(bool status);
    void requestSent(QUuid requestId, bool success);

    void eventReveiced(EventType eventType, bool status);
    void favoritesReceived(QList<FavoriteObject> favourites);

    void liveImageReceived(QImage image);

public slots:
    void onUdpBroadcast(const QByteArray &data);
};

#endif // DOORBIRD_H
