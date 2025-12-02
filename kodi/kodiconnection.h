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

#ifndef KODICONNECTION_H
#define KODICONNECTION_H

#include <QObject>
#include <QTcpSocket>
#include <QHostAddress>
#include <QJsonDocument>

class KodiConnection : public QObject
{
    Q_OBJECT
public:
    explicit KodiConnection(const QHostAddress &hostAddress, int port = 9090, QObject *parent = nullptr);

    void connectKodi();
    void disconnectKodi();

    QHostAddress hostAddress() const;
    void setHostAddress(const QHostAddress &address);

    int port() const;
    void setPort(int port);

    bool connected() const;

private:
    QTcpSocket *m_socket = nullptr;

    QHostAddress m_hostAddress;
    int m_port;
    bool m_connected = false;

private slots:
    void onConnected();
    void onDisconnected();
    void onError(QAbstractSocket::SocketError socketError);
    void readData();

signals:
    void connectionStatusChanged();
    void dataReady(const QByteArray &data);

public slots:
    void sendData(const QByteArray &message);

};

#endif // KODICONNECTION_H
