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


#include "lifxlan.h"
#include "extern-plugininfo.h"

#include <QColor>

LifxLan::LifxLan(const QHostAddress &address, quint16 port, QObject *parent) :
    QObject(parent),
    m_host(address),
    m_port(port)
{
    m_clientId = qrand();

    m_socket = new QUdpSocket(this);

    m_socket->setSocketOption(QAbstractSocket::MulticastTtlOption, QVariant(1));
    m_socket->setSocketOption(QAbstractSocket::MulticastLoopbackOption, QVariant(1));
}

LifxLan::~LifxLan()
{
    if (m_socket) {
        m_socket->waitForBytesWritten(1000);
        m_socket->close();
    }
}

bool LifxLan::enable()
{
    // Bind udp socket and join multicast group
    if(!m_socket->bind(QHostAddress::AnyIPv4, m_port, QUdpSocket::ShareAddress)){
        qCWarning(dcLifx()) << "could not bind to port" << m_port;
        delete m_socket;
        m_socket = nullptr;
        return false;
    }

    if(!m_socket->joinMulticastGroup(QHostAddress("239.255.255.250"))){
        qCWarning(dcLifx()) << "could not join multicast group";
        delete m_socket;
        m_socket = nullptr;
        return false;
    }
    connect(m_socket, &QUdpSocket::readyRead, this, &LifxLan::onReadyRead);
    return true;
}

void LifxLan::setHostAddress(const QHostAddress &address)
{
    m_host = address;
}

void LifxLan::setPort(quint16 port)
{
    m_port = port;
}

int LifxLan::setColorTemperature(uint mirad, uint msFadeTime)
{
    Q_UNUSED(mirad)
    Q_UNUSED(msFadeTime)
    int requestId = qrand();
    Message message;
    sendMessage(message);
    return requestId;
}

int LifxLan::setColor(QColor color, uint msFadeTime)
{
    Q_UNUSED(color)
    Q_UNUSED(msFadeTime)
    int requestId = qrand();
    Message message;
    //TODO create LAN message
    sendMessage(message);
    return requestId;
}

int LifxLan::setBrightness(uint percentage, uint msFadeTime)
{
    Q_UNUSED(percentage)
    Q_UNUSED(msFadeTime)
    int requestId = qrand();
    Message message;
    sendMessage(message);
        //TODO create LAN message
    return requestId;
}

int LifxLan::setPower(bool power, uint msFadeTime)
{
    Q_UNUSED(power)
    Q_UNUSED(msFadeTime)
    int requestId = qrand();
    Message message;
    sendMessage(message);
        //TODO create LAN message
    return requestId;
}

void LifxLan::sendMessage(const LifxLan::Message &message)
{
    QByteArray header;
    // -- FRAME --
    // Protocol number: must be 1024 (decimal)
    quint16 protocol = 1024;
    protocol |= (0x0001 << 4);                 //Message includes a target address: must be one (1)
    protocol |= (message.frame.Tagged << 5);   // Determines usage of the Frame Address target field
    protocol &= ~(0x0003);                     // Message origin indicator: must be zero (0)
    header.append(protocol >> 8);
    header.append(protocol & 0xff);

    //Source identifier: unique value set by the client, used by responses
    header.append(m_clientId);

    // -- FRAME ADDRESS --
    //Target - frame address starts with 64 bits


    //ADD RESERVED SECTION a reserved section of 48 bits (6 bytes)
    //header.append(6, '\x00'); //that must be all zeros.

    //ADD ACK and RES
    //header.append(2, '\x01');

    //ADD SEQUENCE NUMBER 1Byte
    //header.append(m_sequenceNumber++);

    //Protocol header. which begins with 64 reserved bits (8 bytes). Set these all to zero.
    //header.append(8, '\x00'); //that must be all zeros.

    //ADD MESSAGE TYPE
    //header.append(static_cast<uint16_t>(LightMessages::SetColor));

    // Finally another reserved field of 16 bits (2 bytes).
    //header.append(2, '\x00');

    //ADD SIZE
    //header.append(((static_cast<uint16_t>(header.length()+1) & 0xff00) >> 8));
    //header.append((static_cast<uint16_t>(header.length()+1) & 0x00ff));

    //Finally another reserved field of 16 bits (2 bytes).
    //header.append(2, '\x00');

    QByteArray fullMessage;
    //fullMessage = QByteArray::fromHex("0x310000340000000000000000000000000000000000000000000000000000000066000000005555FFFFFFFFAC0D00040000"); // test message - set all lights green
    //std::reverse(fullMessage.begin(), fullMessage.end());
    m_socket->writeDatagram(fullMessage, m_host, m_port);
}

void LifxLan::onStateChanged(QAbstractSocket::SocketState state)
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

void LifxLan::onReadyRead()
{
    QByteArray data = m_socket->readAll();
    qCDebug(dcLifx()) << "Message received" << data;
}
