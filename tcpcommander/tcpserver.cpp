/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *                                                                         *
 *  Copyright (C) 2017 Bernhard Trinnes <bernhard.trinnes@guh.io>          *
 *                                                                         *
 *  This file is part of guh.                                              *
 *                                                                         *
 *  Guh is free software: you can redistribute it and/or modify            *
 *  it under the terms of the GNU General Public License as published by   *
 *  the Free Software Foundation, version 2 of the License.                *
 *                                                                         *
 *  Guh is distributed in the hope that it will be useful,                 *
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of         *
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the           *
 *  GNU General Public License for more details.                           *
 *                                                                         *
 *  You should have received a copy of the GNU General Public License      *
 *  along with guh. If not, see <http://www.gnu.org/licenses/>.            *
 *                                                                         *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

#include "tcpserver.h"
#include "extern-plugininfo.h"
#include <QNetworkInterface>


TcpServer::TcpServer(const QHostAddress address, const int &port, QObject *parent) :
    QObject(parent)
{
    m_tcpServer = new QTcpServer(this);
    connect(m_tcpServer, &QTcpServer::newConnection, this, &TcpServer::newConnection);
    qDebug(dcTCPCommander()) << "TCP Server on Port: " << port << "Address: " << address.toString();
    if (!m_tcpServer->listen(address, port)) {
        qDebug(dcTCPCommander()) << "Unable to start the server: " << m_tcpServer->errorString();
        return;
    }
}


TcpServer::TcpServer(const int &port, QObject *parent) :
    QObject(parent)
{

    m_tcpServer = new QTcpServer(this);
    connect(m_tcpServer, &QTcpServer::newConnection, this, &TcpServer::newConnection);
    qDebug(dcTCPCommander()) << "TCP Server on Port: " << port;
    if (!m_tcpServer->listen(QHostAddress::Any, port)) {
        qDebug(dcTCPCommander()) << "Unable to start the server: " << m_tcpServer->errorString();
        return;
    }
}

TcpServer::~TcpServer()
{
}

bool TcpServer::isValid()
{
    return m_tcpServer->isListening();
}

QHostAddress TcpServer::serverAddress()
{
    return m_tcpServer->serverAddress();
}

int TcpServer::serverPort()
{
    return m_tcpServer->serverPort();
}

void TcpServer::newConnection()
{
    qDebug(dcTCPCommander()) << "TCP Server new Connection request";
    m_socket = m_tcpServer->nextPendingConnection();
    m_socket->write("Hello client");
    m_socket->flush();

    emit connected();
    connect(m_socket, &QTcpSocket::disconnected, this, &TcpServer::onDisconnected);
    connect(m_socket, &QTcpSocket::readyRead, this, &TcpServer::readData);
    // Note: error signal will be interpreted as function, not as signal in C++11
    //connect(m_socket, SIGNAL(error(QAbstractSocket::SocketError)), this, SLOT(onError(QAbstractSocket::SocketError)));
}


void TcpServer::onDisconnected()
{
    qDebug(dcTCPCommander()) << "TCP Server connection aborted";
    disconnect(m_socket, &QTcpSocket::disconnected, this, &TcpServer::onDisconnected);
    disconnect(m_socket, &QTcpSocket::readyRead, this, &TcpServer::readData);
    m_socket->deleteLater();
    emit disconnected();
}

void TcpServer::readData()
{
    QByteArray data = m_socket->readAll();
    qDebug(dcTCPCommander()) << "TCP Server data received: " << data;
    emit textMessageReceived(data);

}
