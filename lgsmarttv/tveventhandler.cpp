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

#include "tveventhandler.h"
#include "extern-plugininfo.h"

TvEventHandler::TvEventHandler(const QHostAddress &host, quint16 eventHandlerPort, QObject *parent) :
    QTcpServer(parent),
    m_host(host),
    m_port(eventHandlerPort),
    m_expectingData(false)
{
    if (!listen(QHostAddress::AnyIPv4, m_port)) {
        qCWarning(dcLgSmartTv()) << "EventHandler: Could not listen for events on" << m_port;
    } else {
        qCDebug(dcLgSmartTv()) << "EventHandler: listening on port" << m_port;
    }
}

uint TvEventHandler::getFreePort()
{
    uint port = 9000;
    QTcpServer testServer;
    for (int i = 0; i < 100; i++) {
        if (testServer.listen(QHostAddress::AnyIPv4, port)) {
            qCDebug(dcLgSmartTv()) << "EventHandler: found free port" << port;
            testServer.close();
            return port;
        } else {
            qCDebug(dcLgSmartTv()) << "EventHandler: port not free" << port << "picking next one...";
        }
    }

    return port;
}

void TvEventHandler::incomingConnection(qintptr socket)
{
    QTcpSocket* tcpSocket = new QTcpSocket(this);
    tcpSocket->setSocketDescriptor(socket);
    qCDebug(dcLgSmartTv()) << "EventHandler: incoming connection" << tcpSocket->peerAddress().toString() << tcpSocket->peerName();
    connect(tcpSocket, &QTcpSocket::readyRead, this, &TvEventHandler::readClient);
    connect(tcpSocket, &QTcpSocket::disconnected, this, &TvEventHandler::onDisconnected);
}

void TvEventHandler::readClient()
{
    QTcpSocket* socket = (QTcpSocket*)sender();

    // Reject everything, except data from the TV
    if(socket->peerAddress() != m_host){
        socket->close();
        socket->deleteLater();
        qCWarning(dcLgSmartTv()) << "EventHandler: rejecting connection from " << socket->peerAddress().toString();
        return;
    }


    // the tv sends first the header (POST /udap/api/.... HTTP/1.1)
    // in the scond package the tv sends the information (xml format)
    while (!socket->atEnd()){
        QByteArray data = socket->readAll();

        // check if we got information
        if (data.startsWith("<?xml") && m_expectingData){
            m_expectingData = false;

            // Answere with OK
            QTextStream textStream(socket);
            textStream.setAutoDetectUnicode(true);
            textStream << "HTTP/1.0 200 OK\r\n"
                          "Content-Type: text/html; charset=\"utf-8\"\r\n"
                          "User-Agent: UDAP/2.0 nymea\r\n"
                       << "Date: " << QDateTime::currentDateTime().toString() << "\n";

            emit eventOccured(data);
        }

        // check if we got header
        if (data.startsWith("POST") && !m_expectingData) {
            m_expectingData = true;
            QStringList tokens = QString(data).split(QRegExp("[ \r\n][ \r\n]*"));
            qCDebug(dcLgSmartTv()) << "EventHandler: event occured" << "http://" << m_host.toString() << ":" << m_port << tokens[1];
        }
    }
}

void TvEventHandler::onDisconnected()
{
    QTcpSocket* socket = (QTcpSocket*)sender();
    qCDebug(dcLgSmartTv()) << "EventHandler: client disconnected" << socket->peerAddress();
    socket->deleteLater();
}

