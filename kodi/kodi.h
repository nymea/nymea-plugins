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

#ifndef KODI_H
#define KODI_H

#include <QObject>
#include <QHostAddress>

#include "kodiconnection.h"
#include "kodijsonhandler.h"

class Kodi : public QObject
{
    Q_OBJECT
public:

    explicit Kodi(const QHostAddress &hostAddress, const int &port = 9090, QObject *parent = nullptr);

    QHostAddress hostAddress() const;
    int port() const;

    bool connected() const;

    // propertys
    int setMuted(const bool &muted);
    bool muted() const;

    int setVolume(const int &volume);
    int volume() const;

    int setShuffle(bool shuffle);
    int setRepeat(const QString &repeat);

    // actions
    int showNotification(const QString &message, const int &displayTime, const QString &notificationType);
    int pressButton(const QString &button);
    int systemCommand(const QString &command);
    int videoLibrary(const QString &command);
    int audioLibrary(const QString &command);

    void update();
    void checkVersion();

    void connectKodi();
    void disconnectKodi();

signals:
    void connectionStatusChanged();
    void stateChanged();
    void activePlayerChanged(const QString &playerType);
    void actionExecuted(int actionId, const bool &success);
    void updateDataReceived(const QVariantMap &data);
    void versionDataReceived(const QVariantMap &data);
    void playbackStatusChanged(const QString &playbackState);
    void mediaMetadataChanged(const QString &title, const QString &artist, const QString &collection, const QString &artwork);
    void shuffleChanged(bool shuffle);
    void repeatChanged(const QString &repeat);

private slots:
    void onVolumeChanged(const int &volume, const bool &muted);
    void onUpdateFinished(const QVariantMap &data);
    void activePlayersChanged(const QVariantList &data);
    void playerPropertiesReceived(const QVariantMap &properties);
    void mediaMetaDataReceived(const QVariantMap &data);
    void onPlaybackStatusChanged(const QString &plabackState);

    void processNotification(const QString &method, const QVariantMap &params);
    void processResponse(int id, const QString &method, const QVariantMap &response);

    void updatePlayerProperties();
    void updateMetadata();

private:
    KodiConnection *m_connection;
    KodiJsonHandler *m_jsonHandler;
    bool m_muted;
    int m_volume;
    int m_activePlayerCount = 0; // if it's > 0, there is something playing (either music or video or slideshow)
    int m_activePlayer = -1;

};

#endif // KODI_H
