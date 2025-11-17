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

#ifndef SIGNALRCONNECTION_H
#define SIGNALRCONNECTION_H

#include <QObject>
#include <QNetworkAccessManager>
#include <QWebSocket>
#include <QTimer>
#include <network/networkaccessmanager.h>

class SignalRConnection : public QObject
{
    Q_OBJECT
public:
    explicit SignalRConnection(const QUrl &url, const QByteArray &accessToken, NetworkAccessManager *nam, QObject *parent = nullptr);

    void subscribe(const QString &chargerId);
    bool connected() const;

    void updateToken(const QByteArray &accessToken);

signals:
    void connectionStateChanged(bool connected);
    void dataReceived(const QVariantMap &data);

private:
    QByteArray encode(const QVariantMap &message);

private slots:
    void connectToHost();

private:
    QUrl m_url;
    QByteArray m_accessToken;
    NetworkAccessManager *m_nam = nullptr;
    QWebSocket *m_socket = nullptr;
    QTimer *m_watchdog = nullptr;

    bool m_waitingForHandshakeReply = false;
};

#endif // SIGNALRCONNECTION_H
