/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *                                                                         *
 *  Copyright (C) 2019 Bernhard Trinnes <bernhard.trinnes@nymea.io>        *
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

    bool isValid();
    QHostAddress serverAddress();
    int serverPort();

    void setPort(int port);
    void setServerAddress(const QHostAddress &address);


private:
    QTcpServer *m_tcpServer = nullptr;
    QTcpSocket *m_socket = nullptr;

signals:
    void newPendingConnection();
    void commandReceived(QByteArray message);
    void connectionChanged(bool connected);

private slots:
    void newConnection();
    void onDisconnected();
    void readData();
    void onError(QAbstractSocket::SocketError error);
};

#endif // TCPSERVER_H
