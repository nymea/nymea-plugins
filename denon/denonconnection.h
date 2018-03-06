/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *                                                                         *
 *  Copyright (C) 2015 Simon Stürz <simon.stuerz@guh.io>                   *
 *  Copyright (C) 2016 Bernhard Trinnes <bernhard.trinnes@guh.guru>        *
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

#ifndef DENONCONNECTION_H
#define DENONCONNECTION_H

#include <QObject>
#include <QTcpSocket>
#include <QHostAddress>

class DenonConnection : public QObject
{
    Q_OBJECT
public:
    explicit DenonConnection(const QHostAddress &hostAddress, const int &port = 23, QObject *parent = 0);
    ~DenonConnection();

    void connectDenon();
    void disconnectDenon();

    QHostAddress hostAddress() const;
    int port() const;

    bool connected();

    void sendData(const QByteArray &message);

private:
    QTcpSocket *m_socket;

    QHostAddress m_hostAddress;
    int m_port;
    bool m_connected;

private slots:
    void onConnected();
    void onDisconnected();
    void onError(QAbstractSocket::SocketError socketError);
    void readData();

    void setConnected(const bool &connected);

signals:
    void socketErrorOccured(QAbstractSocket::SocketError socketError);
    void connectionStatusChanged();
    void dataReady(const QByteArray &data);

};

#endif // DENONCONNECTION_H
