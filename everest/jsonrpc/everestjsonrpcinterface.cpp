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

#include "everestjsonrpcinterface.h"
#include "extern-plugininfo.h"

EverestJsonRpcInterface::EverestJsonRpcInterface(QObject *parent)
    : QObject{parent}
{
    m_webSocket = new QWebSocket("nymea-client", QWebSocketProtocol::Version13, this);

    connect(m_webSocket, &QWebSocket::disconnected, this, &EverestJsonRpcInterface::onDisconnected);
    connect(m_webSocket, &QWebSocket::textMessageReceived, this, &EverestJsonRpcInterface::onTextMessageReceived);
    connect(m_webSocket, &QWebSocket::binaryMessageReceived, this, &EverestJsonRpcInterface::onBinaryMessageReceived);
    connect(m_webSocket, &QWebSocket::stateChanged, this, &EverestJsonRpcInterface::onStateChanged);
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    connect(m_webSocket, &QWebSocket::errorOccurred, this, &EverestJsonRpcInterface::onError);
#else
    connect(m_webSocket, SIGNAL(error(QAbstractSocket::SocketError)), this, SLOT(onError(QAbstractSocket::SocketError)));
#endif

}

EverestJsonRpcInterface::~EverestJsonRpcInterface()
{
    disconnectServer();
}

QUrl EverestJsonRpcInterface::serverUrl() const
{
    return m_serverUrl;
}

void EverestJsonRpcInterface::sendData(const QByteArray &data)
{
    m_webSocket->sendTextMessage(QString::fromUtf8(data));
}

void EverestJsonRpcInterface::connectServer(const QUrl &serverUrl)
{
    if (m_connected) {
        m_connected = false;
        emit connectedChanged(m_connected);
        m_webSocket->close();
    }

    m_serverUrl = serverUrl;

    qCDebug(dcEverest()) << "Connecting to" << m_serverUrl.toString();
    m_webSocket->open(m_serverUrl);
}

void EverestJsonRpcInterface::disconnectServer()
{
    qCDebug(dcEverest()) << "Disconnecting from" << m_serverUrl.toString();
    m_webSocket->close();
}

void EverestJsonRpcInterface::onDisconnected()
{
    qCDebug(dcEverest()) << "Disconnected from" << m_webSocket->requestUrl().toString() << m_webSocket->closeReason();

    if (m_connected) {
        m_connected = false;
        emit connectedChanged(m_connected);
    }
}

void EverestJsonRpcInterface::onError(QAbstractSocket::SocketError error)
{
    qCDebug(dcEverest()) << "Socket error occurred" << error << m_webSocket->errorString();
    if (error == QAbstractSocket::ConnectionRefusedError)
        emit connectedChanged(false);

}

void EverestJsonRpcInterface::onStateChanged(QAbstractSocket::SocketState state)
{
    qCDebug(dcEverest()) << "Socket state changed" << state;

    switch (state) {
    case QAbstractSocket::ConnectedState:
        qCDebug(dcEverest()) << "Connected with" << m_webSocket->requestUrl().toString();
        if (!m_connected) {
            m_connected = true;
            emit connectedChanged(m_connected);
        }
        break;
    default:
        if (m_connected) {
            m_connected = false;
            emit connectedChanged(m_connected);
        }
        break;
    }
}

void EverestJsonRpcInterface::onTextMessageReceived(const QString &message)
{
    emit dataReceived(message.toUtf8());
}

void EverestJsonRpcInterface::onBinaryMessageReceived(const QByteArray &message)
{
    emit dataReceived(message);
}
