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

class Doorbird : public QObject
{
    Q_OBJECT
public:
    explicit Doorbird(const QHostAddress &address, const QString &username, const QString &password, QObject *parent = nullptr);

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
    QUuid addFavorite();
    QUuid deleteFavorite();

    QUuid listSchedules();
    QUuid daddScheduleEntry();
    QUuid deleteScheduleEntry();

    QUuid restart();

    void connectToEventMonitor();
private:
    QNetworkAccessManager *m_networkAccessManager;
    QList<QByteArray> m_readBuffers;

    QHostAddress m_address;
    QList<QNetworkReply *> m_networkRequests;

    QString m_username;
    QString m_password;

    QByteArray sessionId;

signals:
    void requestSent(QUuid requestId, bool success);

public slots:
};

#endif // DOORBIRD_H
