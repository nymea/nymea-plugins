/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*
* Copyright 2013 - 2020, nymea GmbH
* Contact: contact@nymea.io
*
* This file is part of nymea.
* This project including source code and documentation is protected by copyright law, and
* remains the property of nymea GmbH. All rights, including reproduction, publication,
* editing and translation, are reserved. The use of this project is subject to the terms of a
* license agreement to be concluded with nymea GmbH in accordance with the terms
* of use of nymea GmbH, available under https://nymea.io/license
*
* GNU Lesser General Public License Usage
* This project may also contain libraries licensed under the open source software license GNU GPL v.3.
* Alternatively, this project may be redistributed and/or modified under the terms of the GNU
* Lesser General Public License as published by the Free Software Foundation; version 3.
* this project is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;
* without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
* See the GNU Lesser General Public License for more details.
*
* You should have received a copy of the GNU Lesser General Public License along with this project.
* If not, see <https://www.gnu.org/licenses/>.
*
* For any further details and any questions please contact us under contact@nymea.io
* or see our FAQ/Licensing Information on https://nymea.io/license/faq
*
* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

#include "lifx.h"
#include "extern-plugininfo.h"

#include <QColor>
#include <QRandomGenerator>

Lifx::Lifx(QObject *parent) :
    QObject(parent)
{
    m_reconnectTimer = new QTimer(this);
    m_reconnectTimer->setSingleShot(true);
    connect(m_reconnectTimer, &QTimer::timeout, this, &Lifx::onReconnectTimer);
}

Lifx::~Lifx()
{
    if (m_socket) {
        m_socket->waitForBytesWritten(1000);
        m_socket->close();
    }
}

bool Lifx::enable()
{
    // Clean up
    if (m_socket) {
        delete m_socket;
        m_socket = nullptr;
    }

    // Bind udp socket and join multicast group
    m_socket = new QUdpSocket(this);
    m_port = 56700;
    m_host = QHostAddress("239.255.255.250");

    m_socket->setSocketOption(QAbstractSocket::MulticastTtlOption,QVariant(1));
    m_socket->setSocketOption(QAbstractSocket::MulticastLoopbackOption,QVariant(1));

    if(!m_socket->bind(QHostAddress::AnyIPv4, m_port, QUdpSocket::ShareAddress)){
        qCWarning(dcLifx()) << "could not bind to port" << m_port;
        delete m_socket;
        m_socket = nullptr;
        return false;
    }

    if(!m_socket->joinMulticastGroup(m_host)){
        qCWarning(dcLifx()) << "could not join multicast group" << m_host;
        delete m_socket;
        m_socket = nullptr;
        return false;
    }
    connect(m_socket, SIGNAL(error(QAbstractSocket::SocketError)), this, SLOT(error(QAbstractSocket::SocketError)));
    connect(m_socket, &QUdpSocket::readyRead, this, &Lifx::onReadyRead);
    return true;
}

void Lifx::discoverDevices()
{
    Message message;
    sendMessage(message);
}

int Lifx::setColorTemperature(int mirad, int msFadeTime)
{
    Q_UNUSED(mirad)
    Q_UNUSED(msFadeTime)
    int requestId = static_cast<int>(QRandomGenerator::global()->generate());

    return requestId;
}

int Lifx::setColor(QColor color, int msFadeTime)
{
    Q_UNUSED(color)
    Q_UNUSED(msFadeTime)
    int requestId = static_cast<int>(QRandomGenerator::global()->generate());

    return requestId;
}

int Lifx::setBrightness(int percentage, int msFadeTime)
{
    Q_UNUSED(percentage)
    Q_UNUSED(msFadeTime)
    int requestId = static_cast<int>(QRandomGenerator::global()->generate());
    Message message;
    sendMessage(message);
    return requestId;
}

int Lifx::setPower(bool power, int msFadeTime)
{
    Q_UNUSED(power)
    Q_UNUSED(msFadeTime)
    int requestId = static_cast<int>(QRandomGenerator::global()->generate());

    return requestId;
}

int Lifx::flash()
{
    int requestId = static_cast<int>(QRandomGenerator::global()->generate());

    return requestId;
}

int Lifx::flash15s()
{
    int requestId = static_cast<int>(QRandomGenerator::global()->generate());

    return requestId;
}

void Lifx::sendMessage(const Lifx::Message &message)
{
    QByteArray data;
    // -- FRAME --
    // Protocol number: must be 1024 (decimal)
    quint16 protocol = 1024;
    protocol |= (0x0001 << 4); //Message includes a target address: must be one (1)
    protocol |= (message.frame.Tagged << 5);   // Determines usage of the Frame Address target field
    protocol &= ~(0x0003); // Message origin indicator: must be zero (0)
    data.append(protocol >> 8);
    data.append(protocol & 0xff);

    //Source identifier: unique value set by the client, used by responses
    data.append("nyma");

    // -- FRAME ADDRESS --
    //Target - frame address starts with 64 bits


    //ADD RESERVED SECTION a reserved section of 48 bits (6 bytes)
    data.append(6, '0');

    //ADD ACK and RES

    //ADD SEQUENCE NUMBER 1Byte
    data.append(m_sequenceNumber + 1);

    //ADD MESSAGE TYPE

    //ADD SIZE
    data.append(((static_cast<uint16_t>(data.length()+1) & 0xff00) >> 8));
    data.append((static_cast<uint16_t>(data.length()+1) & 0x00ff));

    //Finally another reserved field of 16 bits (2 bytes).
    data.append(2, '0');
    data = QByteArray::fromHex("0x310000340000000000000000000000000000000000000000000000000000000066000000005555FFFFFFFFAC0D00040000");
    m_socket->writeDatagram(data, m_host, m_port);
    Q_UNUSED(message)
}

void Lifx::onStateChanged(QAbstractSocket::SocketState state)
{
    switch (state) {
    case QAbstractSocket::SocketState::ConnectedState:
        emit connectionChanged(true);
        break;
    case QAbstractSocket::SocketState::UnconnectedState:
        m_reconnectTimer->start(10 * 1000);
        emit connectionChanged(false);
        break;
    default:
        emit connectionChanged(false);
        break;
    }
}

void Lifx::onReadyRead()
{
    QByteArray data = m_socket->readAll();
    qCDebug(dcLifx()) << "Message received" << data;
}

void Lifx::onReconnectTimer()
{

}
