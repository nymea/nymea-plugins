/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *                                                                         *
 *  Copyright (C) 2017 Bernhard Trinnes <bernhard.trinnes@guh.io>          *
 *                                                                         *
 *  This file is part of guh.                                              *
 *                                                                         *
 *  Guh is free software: you can redistribute it and/or modify            *
 *  it under the terms of the GNU General Public License as published by   *
 *  the Free Software Foundation, version 2 of the License.                *
 *                                                                         *
 *  Guh is distributed in the hope that it will be useful,                 *
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of         *
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the           *
 *  GNU General Public License for more details.                           *
 *                                                                         *
 *  You should have received a copy of the GNU General Public License      *
 *  along with guh. If not, see <http://www.gnu.org/licenses/>.            *
 *                                                                         *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

#ifndef TCPSERVER_H
#define TCPSERVER_H

#include <QObject>
#include <QTcpSocket>
#include <QTcpServer>

class TcpServer : public QObject
{
    Q_OBJECT
public:
    explicit TcpServer(const QHostAddress address, const int &port, QObject *parent);
    explicit TcpServer(const int &port, QObject *parent = 0);
    ~TcpServer();

    bool isValid();
    QHostAddress serverAddress();
    int serverPort();

    void setPort(int port);
    void setServerAddress(const QHostAddress &address);


private:
    QTcpServer *m_tcpServer;
    QTcpSocket *m_socket;

signals:
    void newPendingConnection();
    void textMessageReceived(QByteArray message);
    void connected();
    void disconnected();

public slots:
    void newConnection();
    void onDisconnected();
    void readData();
};

#endif // TCPSERVER_H
