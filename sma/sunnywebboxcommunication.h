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

#ifndef SUNNYWEBBOXCOMMUNICATION_H
#define SUNNYWEBBOXCOMMUNICATION_H

#include <QObject>
#include <QUdpSocket>

class SunnyWebBoxCommunication : public QObject
{
    Q_OBJECT
public:
    explicit SunnyWebBoxCommunication(QObject *parent = nullptr);

    QString sendMessage(const QHostAddress &address, const QString &procedure);
    QString sendMessage(const QHostAddress &address, const QString &procedure, const QJsonObject &params);

private:
    int m_port =  34268;
    bool m_messageResponsePending = false;
    QUdpSocket *m_udpSocket;
    QHash<QHostAddress, QList<QByteArray>> m_messageQueue;

    void datagramReceived(const QHostAddress &address, const QByteArray &data);

signals:
    void socketConnected(bool connected);
    void messageReceived(const QHostAddress &address, const QString &messageId, const QString &messageType, const QVariantMap &result);

};

#endif // SUNNYWEBBOXCOMMUNICATION_H
