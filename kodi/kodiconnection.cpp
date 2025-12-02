// SPDX-License-Identifier: GPL-3.0-or-later

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*
* Copyright (C) 2013 - 2024, nymea GmbH
* Copyright (C) 2024 - 2025, chargebyte austria GmbH
*
* This file is part of nymea-plugins.
*
* nymea-plugins is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* (at your option) any later version.
*
* nymea-plugins is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
* General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with nymea-plugins. If not, see <https://www.gnu.org/licenses/>.
*
* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

#include "kodiconnection.h"
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
#if QT_VERSION >= QT_VERSION_CHECK(5, 15, 0)
    connect(m_socket, &QTcpSocket::errorOccurred, this, &KodiConnection::onError);
#else
    connect(m_socket, SIGNAL(error(QAbstractSocket::SocketError)), this, SLOT(onError(QAbstractSocket::SocketError)));
#endif
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


bool KodiConnection::connected() const
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
    for (int i = 0; i < commandList.count(); ++i) {
        QString command = commandList.at(i);
        if (command.isEmpty()) {
            continue;
        }
        if (i < commandList.count() - 1) {
            command.append("}");
        }
        if (i > 0) {
            command.prepend("{");
        }
        emit dataReady(command.toUtf8());
    }
}

void KodiConnection::sendData(const QByteArray &message)
{
    m_socket->write(message);
}

