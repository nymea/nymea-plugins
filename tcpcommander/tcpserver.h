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

#ifndef TCPSERVER_H
#define TCPSERVER_H

#include <QObject>
#include <QTcpSocket>
#include <QTcpServer>

class TcpServer : public QObject
{
    Q_OBJECT
public:
    explicit TcpServer(const QHostAddress address, const quint16 &port, QObject *parent = nullptr);
    explicit TcpServer(const quint16 &port, QObject *parent = nullptr);
    ~TcpServer();


    QHostAddress serverAddress();
    int serverPort();

    bool isValid() const;

    bool confirmCommands() const;
    void setConfirmCommands(bool confirmCommands);

    int connectionCount() const;

    bool sendCommand(const QString &clientIp, const QByteArray &data);

signals:
    void newPendingConnection();
    void commandReceived(const QString &clientIp, const QByteArray &message);
    void connectionCountChanged(int connections);

private slots:
    void newConnection();
    void onDisconnected();
    void readData();
    void onError(QAbstractSocket::SocketError error);

private:
    QTcpServer *m_tcpServer = nullptr;
    bool m_confirmCommands = false;
    QList<QTcpSocket*> m_clients;

};

#endif // TCPSERVER_H
