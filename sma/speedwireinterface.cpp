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

#include "speedwireinterface.h"
#include "extern-plugininfo.h"

SpeedwireInterface::SpeedwireInterface(const QHostAddress &address, bool multicast, QObject *parent) :
    QObject(parent),
    m_address(address),
    m_multicast(multicast)
{

    qCDebug(dcSma()) << "SpeedwireInterface: Create interface for" << address.toString() << (multicast ? "multicast" : "unicast");
    m_socket = new QUdpSocket(this);
    connect(m_socket, &QUdpSocket::readyRead, this, &SpeedwireInterface::readPendingDatagrams);
    connect(m_socket, &QUdpSocket::stateChanged, this, &SpeedwireInterface::onSocketStateChanged);
    connect(m_socket, SIGNAL(error(QAbstractSocket::SocketError)),this, SLOT(onSocketError(QAbstractSocket::SocketError)));
}

SpeedwireInterface::~SpeedwireInterface()
{
    deinitialize();
}

bool SpeedwireInterface::initialize()
{
    if (!m_socket->bind(QHostAddress::AnyIPv4, m_port, QAbstractSocket::ShareAddress | QAbstractSocket::ReuseAddressHint)) {
        qCWarning(dcSma()) << "SpeedwireInterface: Initialization failed. Could not bind to port" << m_port;
        return false;
    }

    if (m_multicast && !m_socket->joinMulticastGroup(m_multicastAddress)) {
        qCWarning(dcSma()) << "SpeedwireInterface: Initialization failed. Could not join multicast group" << m_multicastAddress.toString() << m_socket->errorString();
        return false;
    }

    qCDebug(dcSma()) << "SpeedwireInterface: Interface initialized successfully.";
    m_initialized = true;
    return m_initialized;
}

void SpeedwireInterface::deinitialize()
{
    if (m_initialized) {
        if (m_multicast) {
            if (!m_socket->leaveMulticastGroup(m_multicastAddress)) {
                qCWarning(dcSma()) << "SpeedwireInterface: Failed to leave multicast group" << m_multicastAddress.toString();
            }
        }

        m_socket->close();
        m_initialized = false;
    }
}

bool SpeedwireInterface::initialized() const
{
    return m_initialized;
}

quint16 SpeedwireInterface::sourceModelId() const
{
    return m_sourceModelId;
}

quint32 SpeedwireInterface::sourceSerialNumber() const
{
    return m_sourceSerialNumber;
}

void SpeedwireInterface::sendData(const QByteArray &data)
{
    qCDebug(dcSma()) << "SpeedwireInterface: -->" << m_address.toString() << m_port << data.toHex();
    if (m_socket->writeDatagram(data, m_address, m_port) < 0) {
        qCWarning(dcSma()) << "SpeedwireInterface: failed to send data" << m_socket->errorString();
    }
}

void SpeedwireInterface::readPendingDatagrams()
{
    QByteArray datagram;
    QHostAddress senderAddress;
    quint16 senderPort;

    while (m_socket->hasPendingDatagrams()) {
        datagram.resize(m_socket->pendingDatagramSize());
        m_socket->readDatagram(datagram.data(), datagram.size(), &senderAddress, &senderPort);

        // Process only data coming from our target address
        if (senderAddress != m_address)
            continue;

        qCDebug(dcSma()) << "SpeedwireInterface: Received data from" << QString("%1:%2").arg(senderAddress.toString()).arg(senderPort);
        //qCDebug(dcSma()) << "SpeedwireInterface: " << datagram.toHex();
        emit dataReceived(datagram);
    }
}

void SpeedwireInterface::onSocketError(QAbstractSocket::SocketError error)
{
    qCDebug(dcSma()) << "SpeedwireInterface: Socket error" << error;
}

void SpeedwireInterface::onSocketStateChanged(QAbstractSocket::SocketState socketState)
{
    qCDebug(dcSma()) << "SpeedwireInterface: Socket state changed" << socketState;
}
