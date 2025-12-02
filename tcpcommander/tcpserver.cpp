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

#include "tcpserver.h"
#include "extern-plugininfo.h"

#include <QNetworkInterface>

TcpServer::TcpServer(const QHostAddress address, const quint16 &port, QObject *parent) :
    QObject(parent)
{
    m_tcpServer = new QTcpServer(this);
    connect(m_tcpServer, &QTcpServer::newConnection, this, &TcpServer::newConnection);
    qCDebug(dcTCPCommander()) << "TCP Server on Port: " << port << "Address: " << address.toString();
    if (!m_tcpServer->listen(address, port)) {
        qCWarning(dcTCPCommander()) << "Unable to start the server: " << m_tcpServer->errorString();
        return;
    }
}

TcpServer::TcpServer(const quint16 &port, QObject *parent) :
    QObject(parent)
{
    m_tcpServer = new QTcpServer(this);
    connect(m_tcpServer, &QTcpServer::newConnection, this, &TcpServer::newConnection);
    qCDebug(dcTCPCommander()) << "TCP Server on Port: " << port;
    if (!m_tcpServer->listen(QHostAddress::Any, port)) {
        qCWarning(dcTCPCommander()) << "Unable to start the server: " << m_tcpServer->errorString();
        return;
    }
}

TcpServer::~TcpServer()
{
}

bool TcpServer::isValid() const
{
    return m_tcpServer->isListening();
}

bool TcpServer::confirmCommands() const
{
    return m_confirmCommands;
}

void TcpServer::setConfirmCommands(bool confirmCommands)
{
    m_confirmCommands = confirmCommands;
}

QHostAddress TcpServer::serverAddress()
{
    return m_tcpServer->serverAddress();
}

int TcpServer::serverPort()
{
    return m_tcpServer->serverPort();
}

int TcpServer::connectionCount() const
{
    return m_clients.count();
}

bool TcpServer::sendCommand(const QString &clientIp, const QByteArray &data)
{
    bool success = false;
    QHostAddress address(clientIp);
    bool broadcast = false;
    if (address == QHostAddress(QHostAddress::AnyIPv4) || address == QHostAddress(QHostAddress::Broadcast))
        broadcast = true;

    foreach (QTcpSocket *client, m_clients) {
        if (broadcast || client->peerAddress() == address) {
            qint64 len = client->write(data);
            if (len == data.length()) {
                success = true;
            }
        }
    }

    return success;
}

void TcpServer::newConnection()
{
    qCDebug(dcTCPCommander()) << "TCP Server new Connection request";
    QTcpSocket *socket = m_tcpServer->nextPendingConnection();
    socket->flush();

    m_clients.append(socket);
    emit connectionCountChanged(m_clients.count());
    connect(socket, &QTcpSocket::disconnected, this, &TcpServer::onDisconnected);
    connect(socket, &QTcpSocket::readyRead, this, &TcpServer::readData);
#if QT_VERSION >= QT_VERSION_CHECK(5, 15, 0)
    connect(socket, &QTcpSocket::errorOccurred, this, &TcpServer::onError);
#else
    connect(socket, SIGNAL(error(QAbstractSocket::SocketError)), this, SLOT(onError(QAbstractSocket::SocketError)));
#endif
}

void TcpServer::onDisconnected()
{
    QTcpSocket *client = qobject_cast<QTcpSocket*>(sender());
    qCDebug(dcTCPCommander()) << "TCP client disconnected";
    m_clients.removeAll(client);
    emit connectionCountChanged(m_clients.count());
}

void TcpServer::readData()
{
    QTcpSocket *socket = static_cast<QTcpSocket *>(sender());
    QByteArray data = socket->readAll();
    qCDebug(dcTCPCommander()) << "TCP Server data received: " << data;
    if (m_confirmCommands) {
        socket->write("OK\n");
    }

    emit commandReceived(socket->peerAddress().toString(), data);
}

void TcpServer::onError(QAbstractSocket::SocketError error)
{
    QTcpSocket *socket = static_cast<QTcpSocket *>(sender());
    qCWarning(dcTCPCommander()) << "Socket Error" << socket->errorString() << error;
}
