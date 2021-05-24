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

#include "nymealighttcpinterface.h"
#include "extern-plugininfo.h"

#include <QDataStream>

NymeaLightTcpInterface::NymeaLightTcpInterface(const QHostAddress &address, QObject *parent) :
    NymeaLightInterface(parent),
    m_address(address)
{
    qCDebug(dcWs2812fx()) << "NymeaLightTcpInterface: Creating TCP connection" << m_address;
    m_socket = new QTcpSocket(this);

    connect(m_socket, &QTcpSocket::stateChanged, this, &NymeaLightTcpInterface::onStateChanged);
    connect(m_socket, &QTcpSocket::readyRead, this, &NymeaLightTcpInterface::onReadyRead);
    connect(m_socket, SIGNAL(error(QAbstractSocket::SocketError)), this, SLOT(onError(QAbstractSocket::SocketError)));

    m_reconnectTimer = new QTimer(this);
    m_reconnectTimer->setInterval(5000);
    m_reconnectTimer->setSingleShot(false);
    connect(m_reconnectTimer, &QTimer::timeout, this, [=](){
        if (m_socket->isOpen()) {
            m_reconnectTimer->stop();
        } else {
            open();
        }
    });
}

bool NymeaLightTcpInterface::open()
{
    m_socket->connectToHost(m_address, 1080);
    return true;
}

void NymeaLightTcpInterface::close()
{
    m_socket->close();
}

bool NymeaLightTcpInterface::available()
{
    return m_socket->isOpen();
}

void NymeaLightTcpInterface::sendData(const QByteArray &data)
{
    m_socket->write(data);
    m_socket->flush();
}

void NymeaLightTcpInterface::setAddress(const QHostAddress &address)
{
    m_address = address;
}

QHostAddress NymeaLightTcpInterface::address() const
{
    return m_address;
}

void NymeaLightTcpInterface::onReadyRead()
{
    QByteArray data;

    while (m_socket->canReadLine()) {
        data = m_socket->readLine();

        if (!m_buffer.isEmpty() && m_buffer.length() >= 3) {
            emit dataReceived(m_buffer);
        }
        m_buffer.clear();
    }
}

void NymeaLightTcpInterface::onStateChanged(QAbstractSocket::SocketState state)
{
    if (state == QAbstractSocket::SocketState::ConnectedState) {
        emit availableChanged(true);
    } else if (state == QAbstractSocket::SocketState::UnconnectedState) {
        emit availableChanged(false);
        m_reconnectTimer->start();
    }
}

void NymeaLightTcpInterface::onError(QAbstractSocket::SocketError)
{
    qCWarning(dcWs2812fx()) << "NymeaLightTcpInterface: Tcp socket error" << m_socket->errorString();
}
