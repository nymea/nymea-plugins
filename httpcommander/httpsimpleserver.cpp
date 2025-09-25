/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*
* Copyright 2013 - 2025, nymea GmbH
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
