/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *                                                                         *
 *  Copyright (C) 2015 Simon Stürz <simon.stuerz@nymea.io>                 *
 *  Copyright (C) 2019 Bernhard Trinnes <bernhard.trinnes@nymea.io>        *
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

#ifndef AVRCONNECTION_H
#define AVRCONNECTION_H

#include <QObject>
#include <QTcpSocket>
#include <QHostAddress>

class AvrConnection : public QObject
{
    Q_OBJECT
public:
    explicit AvrConnection(const QHostAddress &hostAddress, const int &port = 23, QObject *parent = nullptr);
    ~AvrConnection();

    void connect();
    void disconnect();

    QHostAddress hostAddress() const;
    int port() const;
    bool connected();

    void getAllStatus();
    void getChannel();
    void getVolume();
    void getMute();
    void getPower();
    void getSurroundMode();

    void setChannel(const QByteArray &channel);
    void setVolume(int volume);
    void setMute(bool mute);
    void setPower(bool power);
    void setSurroundMode(const QByteArray &surroundMode);

    void increaseVolume();
    void decreaseVolume();
private:
    QTcpSocket *m_socket = nullptr;
    QHostAddress m_hostAddress;
    int m_port;

    void sendCommand(const QByteArray &message);

private slots:
    void onConnected();
    void onDisconnected();
    void onError(QAbstractSocket::SocketError socketError);
    void readData();

signals:
    void socketErrorOccured(QAbstractSocket::SocketError socketError);
    void connectionStatusChanged(bool status);
    void volumeChanged(int volume);
    void muteChanged(bool mute);
    void channelChanged(const QByteArray &channel);
    void powerChanged(bool power);
    void surroundModeChanged(const QByteArray &surroundMode);
};

#endif // AVRCONNECTION_H
