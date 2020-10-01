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
