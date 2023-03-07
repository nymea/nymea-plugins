/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*
* Copyright 2013 - 2020, nymea GmbH
* Contact: contact@nymea.io
*
* This file is part of nymea.
* This project including source code and documentation is protected by
* copyright law, and remains the property of nymea GmbH. All rights, including
* reproduction, publication, editing and translation, are reserved. The use of
* this project is subject to the terms of a license agreement to be concluded
* with nymea GmbH in accordance with the terms of use of nymea GmbH, available
* under https://nymea.io/license
*
* GNU Lesser General Public License Usage
* Alternatively, this project may be redistributed and/or modified under the
* terms of the GNU Lesser General Public License as published by the Free
* Software Foundation; version 3. This project is distributed in the hope that
* it will be useful, but WITHOUT ANY WARRANTY; without even the implied
* warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
* Lesser General Public License for more details.
*
* You should have received a copy of the GNU Lesser General Public License
* along with this project. If not, see <https://www.gnu.org/licenses/>.
*
* For any further details and any questions please contact us under
* contact@nymea.io or see our FAQ/Licensing Information on
* https://nymea.io/license/faq
*
* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

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

    bool connected();

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

signals:
    void connectionStatusChanged();
    void dataReady(const QByteArray &data);

public slots:
    void sendData(const QByteArray &message);



};

#endif // KODICONNECTION_H
