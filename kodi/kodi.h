/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *                                                                         *
 *  Copyright (C) 2015 Simon Stürz <simon.stuerz@guh.io>                   *
 *                                                                         *
 *  This file is part of guh.                                              *
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

    explicit Kodi(const QHostAddress &hostAddress, const int &port = 9090, QObject *parent = 0);

    QHostAddress hostAddress() const;
    int port() const;

    bool connected() const;

    // propertys
    void setMuted(const bool &muted, const ActionId &actionId);
    bool muted() const;

    void setVolume(const int &volume, const ActionId &actionId);
    int volume() const;

    // actions
    void showNotification(const QString &message, const int &displayTime, const QString &notificationType, const ActionId &actionId);
    void pressButton(const QString &button, const ActionId &actionId);
    void systemCommand(const QString &command, const ActionId &actionId);
    void videoLibrary(const QString &command, const ActionId &actionId);
    void audioLibrary(const QString &command, const ActionId &actionId);

    void update();
    void checkVersion();

    void connectKodi();
    void disconnectKodi();

private:
    KodiConnection *m_connection;
    KodiJsonHandler *m_jsonHandler;
    bool m_muted;
    int m_volume;
    int m_activePlayerCount = 0; // if it's > 0, there is something playing (either music or video or slideshow)

signals:
    void connectionStatusChanged();
    void stateChanged();
    void actionExecuted(const ActionId &actionId, const bool &success);
    void updateDataReceived(const QVariantMap &data);
    void versionDataReceived(const QVariantMap &data);
    void playbackStatusChanged(const QString &playbackState);

private slots:
    void onVolumeChanged(const int &volume, const bool &muted);
    void onUpdateFinished(const QVariantMap &data);
    void activePlayersChanged(const QVariantList &data);
    void playerPropertiesReceived(const QVariantMap &properties);
};

#endif // KODI_H
