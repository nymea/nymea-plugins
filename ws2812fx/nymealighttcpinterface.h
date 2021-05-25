/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*
* Copyright 2013 - 2021, nymea GmbH
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

#ifndef NYMEALIGHTTCPINTERFACE_H
#define NYMEALIGHTTCPINTERFACE_H

#include <QTimer>
#include <QObject>
#include <QTcpSocket>
#include <QHostAddress>

#include "nymealightinterface.h"

class NymeaLightTcpInterface : public NymeaLightInterface
{
    Q_OBJECT
public:
    explicit NymeaLightTcpInterface(const QHostAddress &address, QObject *parent = nullptr);
    ~NymeaLightTcpInterface() override = default;

    bool open() override;
    void close() override;
    bool available() override;
    void sendData(const QByteArray &data) override;

    void setAddress(const QHostAddress &address);
    QHostAddress address() const;

private:

    QHostAddress m_address;
    QTimer *m_reconnectTimer = nullptr;
    QTcpSocket *m_socket = nullptr;
    QByteArray m_buffer;

private slots:
    void onReadyRead();
    void onStateChanged(QAbstractSocket::SocketState state);
    void onError(QAbstractSocket::SocketError);
};

#endif // NYMEALIGHTTCPINTERFACE_H
