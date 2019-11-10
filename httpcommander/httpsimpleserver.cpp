/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *                                                                         *
 *  Copyright (C) 2015 Simon Stürz <simon.stuerz@guh.io>                   *
 *  Copyright (C) 2014 Michael Zanetti <michael_zanetti@gmx.net>           *
 *                                                                         *
 *  This file is part of nymea.                                            *
 *                                                                         *
 *  This library is free software; you can redistribute it and/or          *
 *  modify it under the terms of the GNU Lesser General Public             *
 *  License as published by the Free Software Foundation; either           *
 *  version 2.1 of the License, or (at your option) any later version.     *
 *                                                                         *
 *  This library is distributed in the hope that it will be useful,        *
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of         *
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU      *
 *  Lesser General Public License for more details.                        *
 *                                                                         *
 *  You should have received a copy of the GNU Lesser General Public       *
 *  License along with this library; If not, see                           *
 *  <http://www.gnu.org/licenses/>.                                        *
 *                                                                         *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

#include "httpsimpleserver.h"

#include "types/statetype.h"
#include "extern-plugininfo.h"

#include <QTcpSocket>
#include <QDebug>
#include <QDateTime>
#include <QUrlQuery>
#include <QRegExp>
#include <QStringList>

HttpSimpleServer::HttpSimpleServer(QObject *parent):
    QTcpServer(parent)
{
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
    QTcpSocket* s = new QTcpSocket(this);
    connect(s, SIGNAL(readyRead()), this, SLOT(readClient()));
    connect(s, SIGNAL(disconnected()), this, SLOT(discardClient()));
    s->setSocketDescriptor(socket);

}

void HttpSimpleServer::readClient()
{
    // This slot is called when the client sent data to the server. The
    // server looks if it was a get request and sends a very simple HTML
    // document back.
    QTcpSocket* socket = static_cast<QTcpSocket*>(sender());
    if (socket->canReadLine()) {
        QByteArray data = socket->readLine();
        QStringList tokens = QString(data).split(QRegExp("[ \r\n][ \r\n]*"));
        QUrl url("http://foo.bar" + tokens[1]);
        QUrlQuery query(url);

        if (tokens[0] == "GET") {
            QTextStream os(socket);
            os.setAutoDetectUnicode(true);
            os << generateHeader();
            socket->close();

            if (socket->state() == QTcpSocket::UnconnectedState)
                delete socket;
        } else if (tokens[0] == "PUT") {

        } else if (tokens[0] == "POST") {

        } else if (tokens[0] == "DELETE") {

        }
    }
}

void HttpSimpleServer::discardClient()
{
    QTcpSocket* socket = static_cast<QTcpSocket*>(sender());
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
