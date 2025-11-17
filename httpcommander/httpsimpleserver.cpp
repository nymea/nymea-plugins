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

#include "httpsimpleserver.h"

#include "extern-plugininfo.h"

#include <QTcpSocket>
#include <QDebug>
#include <QDateTime>
#include <QUrlQuery>
#include <QRegularExpression>
#include <QStringList>

HttpSimpleServer::HttpSimpleServer(quint16 port, QObject *parent):
    QTcpServer(parent)
{
    listen(QHostAddress::Any, port);
}

HttpSimpleServer::~HttpSimpleServer()
{
    close();
}

void HttpSimpleServer::incomingConnection(qintptr socket)
{
    // When a new client connects, the server constructs a QTcpSocket and all
    // communication with the client is done over this QTcpSocket. QTcpSocket
    // works asynchronously, this means that all the communication is done
    // in the two slots readClient() and discardClient().
    QTcpSocket* tcpSocket = new QTcpSocket(this);
    connect(tcpSocket, &QTcpSocket::readyRead, this, &HttpSimpleServer::readClient);
    connect(tcpSocket, &QTcpSocket::disconnected, this, &HttpSimpleServer::discardClient);
    tcpSocket->setSocketDescriptor(socket);

}

void HttpSimpleServer::readClient()
{
    // This slot is called when the client sent data to the server. The
    // server looks if it was a get request and sends a very simple HTML
    // document back.
    QTcpSocket *tcpSocket = static_cast<QTcpSocket*>(sender());
    if (tcpSocket->canReadLine()) {
        QByteArray data = tcpSocket->readAll();
        QStringList tokens = QString(data).split(QRegularExpression("[ \r\n][ \r\n]*"));
        qCDebug(dcHttpCommander()) << "Http Request, type" << tokens[0] << "path" << tokens[1] << "body" << tokens.last();

        if ((tokens[0] == "GET")      ||
            (tokens[0] == "PUT")  ||
            (tokens[0] == "POST") ||
            (tokens[0] == "DELETE")) {

            QTextStream os(tcpSocket);
            os.setAutoDetectUnicode(true);
            os << generateHeader();
            tcpSocket->close();

            if (tcpSocket->state() == QTcpSocket::UnconnectedState)
                delete tcpSocket;

            emit requestReceived(tokens[0], tokens[1], tokens.last());
        }
    }
}

void HttpSimpleServer::discardClient()
{
    QTcpSocket *socket = static_cast<QTcpSocket *>(sender());
    socket->deleteLater();
}

QString HttpSimpleServer::generateHeader()
{
    QString contentHeader(
        "HTTP/1.0 200 Ok\r\n"
        "Content-Type: text/html; charset=\"utf-8\"\r\n"
        "\r\n"
        );
    return contentHeader;
}
