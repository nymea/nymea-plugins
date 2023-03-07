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

#include "kodiconnection.h"
#include "kodijsonhandler.h"
#include "extern-plugininfo.h"

#include <QPixmap>

KodiConnection::KodiConnection(const QHostAddress &hostAddress, int port, QObject *parent) :
    QObject(parent),
    m_hostAddress(hostAddress),
    m_port(port),
    m_connected(false)
{
    m_socket = new QTcpSocket(this);

    connect(m_socket, &QTcpSocket::connected, this, &KodiConnection::onConnected);
    connect(m_socket, &QTcpSocket::disconnected, this, &KodiConnection::onDisconnected);
    connect(m_socket, SIGNAL(error(QAbstractSocket::SocketError)), this, SLOT(onError(QAbstractSocket::SocketError)));
    connect(m_socket, &QTcpSocket::readyRead, this, &KodiConnection::readData);
}

void KodiConnection::connectKodi()
{
    if (m_socket->state() == QAbstractSocket::ConnectingState) {
        qCDebug(dcKodi) << "Aready connecting... skipping request";
        return;
    }
    m_socket->connectToHost(m_hostAddress, m_port);
}

void KodiConnection::disconnectKodi()
{
    m_socket->close();
}

QHostAddress KodiConnection::hostAddress() const
{
    return m_hostAddress;
}

void KodiConnection::setHostAddress(const QHostAddress &address)
{
    m_hostAddress = address;
}

int KodiConnection::port() const
{
    return m_port;
}

void KodiConnection::setPort(int port)
{
    m_port = port;
}


bool KodiConnection::connected()
{
    return m_connected;
}

void KodiConnection::onConnected()
{
    qCDebug(dcKodi) << "connected successfully to" << hostAddress().toString() << port();
    m_connected = true;
    emit connectionStatusChanged();
}

void KodiConnection::onDisconnected()
{
    qCDebug(dcKodi) << "disconnected from" << hostAddress().toString() << port();
    m_connected = false;
    emit connectionStatusChanged();
}

void KodiConnection::onError(QAbstractSocket::SocketError socketError)
{
    qCWarning(dcKodi) << "socket error:" << socketError << m_socket->errorString() << "this" << this;
    m_connected = false;
    emit connectionStatusChanged();
}

void KodiConnection::readData()
{
    QByteArray data = m_socket->readAll();

    QStringList commandList = QString(data).split("}{");
    for(int i = 0; i < commandList.count(); ++i) {
        QString command = commandList.at(i);
        if(command.isEmpty()) {
            continue;
        }
        if(i < commandList.count() - 1) {
            command.append("}");
        }
        if(i > 0) {
            command.prepend("{");
        }
        emit dataReady(command.toUtf8());
    }
}

void KodiConnection::sendData(const QByteArray &message)
{
    m_socket->write(message);
}

