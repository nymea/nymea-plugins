/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *                                                                         *
 *  Copyright (C) 2020 Bernhard Trinnes <bernhard.trinnes@nymea.io>        *
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

#ifndef SSDP_H
#define SSDP_H

#include <QObject>

#include <QUdpSocket>
#include <QUrl>
#include <QTimer>
#include <QUdpSocket>
#include <QHostAddress>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QNetworkAccessManager>

class Ssdp : public QObject
{
    Q_OBJECT
public:
    explicit Ssdp(QObject *parent = nullptr);
    ~Ssdp();
    bool enable();
    void discover();
    bool disable();
private:
    QUdpSocket *m_socket = nullptr;
    QUdpSocket *m_socket2 = nullptr;
    QHostAddress m_host;
    quint16 m_port;

    QTimer *m_notificationTimer = nullptr;

    QNetworkAccessManager *m_networkAccessManager = nullptr;

    //QList<UpnpDiscoveryRequest *> m_discoverRequests;
   // QHash<QNetworkReply*, UpnpDeviceDescriptor> m_informationRequestList;

    bool m_available = false;
    bool m_enabled = false;

signals:
    void discovered(const QString &address, int port, int id, const QString &model);

private slots:
    void error(QAbstractSocket::SocketError error);
    void readData();
    void readData2();
};

#endif // SSDP_H
