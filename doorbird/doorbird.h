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

#ifndef DOORBIRD_H
#define DOORBIRD_H

#include <QObject>
#include <QHostAddress>
#include <QNetworkAccessManager>
#include <QUuid>
#include <QImage>

#include "network/networkaccessmanager.h"

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
    QNetworkAccessManager *m_networkAccessManager;
    QByteArray m_readBuffer;

    QList<QNetworkReply *> m_networkRequests;
    QList<QNetworkReply *> m_pendingAuthentications;

    QString m_username;
    QString m_password;

signals:
    void deviceConnected(bool status);
    void requestSent(QUuid requestId, bool success);

    void eventReveiced(EventType eventType, bool status);
    void favoritesReceived(QList<FavoriteObject> favourites);

    void sessionIdReceived(const QString &sessionId);
    void liveImageReceived(QImage image);

};

#endif // DOORBIRD_H
