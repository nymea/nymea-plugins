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

#include "kecontactdatalayer.h"
#include "extern-plugininfo.h"

KeContactDataLayer::KeContactDataLayer(QObject *parent) : QObject(parent)
{
    qCDebug(dcKeba()) << "KeContactDataLayer: Creating UDP socket";
    m_udpSocket = new QUdpSocket(this);
    connect(m_udpSocket, &QUdpSocket::readyRead, this, &KeContactDataLayer::readPendingDatagrams);
    connect(m_udpSocket, &QUdpSocket::stateChanged, this, &KeContactDataLayer::onSocketStateChanged);
#if QT_VERSION >= QT_VERSION_CHECK(6,0,0)
    connect(m_udpSocket, &QUdpSocket::errorOccurred, this, &KeContactDataLayer::onSocketError);
#else
    connect(m_udpSocket, SIGNAL(error(QAbstractSocket::SocketError)),this, SLOT(onSocketError(QAbstractSocket::SocketError)));
#endif
}

KeContactDataLayer::~KeContactDataLayer()
{
    qCDebug(dcKeba()) << "KeContactDataLayer: Deleting UDP socket";
}

bool KeContactDataLayer::init()
{
    m_udpSocket->close();
    m_initialized = false;

    if (!m_udpSocket->bind(QHostAddress::AnyIPv4, m_port, QAbstractSocket::ShareAddress)) {
        qCWarning(dcKeba()) << "KeContactDataLayer: Cannot bind to port" << m_port;
        return false;
    }

    m_initialized = true;
    return true;
}

bool KeContactDataLayer::initialized() const
{
    return m_initialized;
}

void KeContactDataLayer::write(const QHostAddress &address, const QByteArray &data)
{
    qCDebug(dcKeba()) << "KeContactDataLayer: -->" << address.toString() << data;
    m_udpSocket->writeDatagram(data, address, m_port);
}

void KeContactDataLayer::readPendingDatagrams()
{
    QUdpSocket *socket= qobject_cast<QUdpSocket*>(sender());

    QByteArray datagram;
    QHostAddress senderAddress;
    quint16 senderPort;

    while (socket->hasPendingDatagrams()) {
        datagram.resize(socket->pendingDatagramSize());
        socket->readDatagram(datagram.data(), datagram.size(), &senderAddress, &senderPort);
        qCDebug(dcKeba()) << "KeContactDataLayer: <--" << senderAddress.toString() << datagram;
        emit datagramReceived(senderAddress, datagram);
    }
}

void KeContactDataLayer::onSocketError(QAbstractSocket::SocketError error)
{
    qCWarning(dcKeba()) << "KeContactDataLayer: Socket error" << error;
}

void KeContactDataLayer::onSocketStateChanged(QAbstractSocket::SocketState socketState)
{
    qCDebug(dcKeba()) << "KeContactDataLayer: Socket state changed" << socketState;
}
