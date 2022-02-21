/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*
* Copyright 2013 - 2022, nymea GmbH
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

#ifndef SPEEDWIREINTERFACE_H
#define SPEEDWIREINTERFACE_H

#include <QObject>
#include <QUdpSocket>
#include <QDataStream>

#include "speedwire.h"

class SpeedwireInterface : public QObject
{
    Q_OBJECT
public:
    enum DeviceType {
        DeviceTypeUnknown,
        DeviceTypeMeter,
        DeviceTypeInverter
    };
    Q_ENUM(DeviceType)

    explicit SpeedwireInterface(const QHostAddress &address, bool multicast, QObject *parent = nullptr);
    ~SpeedwireInterface();

    bool initialize();
    void deinitialize();

    bool initialized() const;

    quint16 sourceModelId() const;
    quint32 sourceSerialNumber() const;

public slots:
    void sendData(const QByteArray &data);

signals:
    void dataReceived(const QByteArray &data);

private:
    QUdpSocket *m_socket = nullptr;
    QHostAddress m_address;
    quint16 m_port = Speedwire::port();
    QHostAddress m_multicastAddress = Speedwire::multicastAddress();
    bool m_multicast = false;
    bool m_initialized = false;

    // Requester
    quint16 m_sourceModelId = 0x007d;
    quint32 m_sourceSerialNumber = 0x3a28be52;

private slots:
    void readPendingDatagrams();
    void onSocketError(QAbstractSocket::SocketError error);
    void onSocketStateChanged(QAbstractSocket::SocketState socketState);

};


#endif // SPEEDWIREINTERFACE_H
