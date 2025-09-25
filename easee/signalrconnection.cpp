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

#include "signalrconnection.h"

#include <QNetworkRequest>
#include <QWebSocket>
#include <QJsonDocument>
#include <QNetworkReply>
#include <QUrlQuery>
#include <QTimer>

#include "extern-plugininfo.h"

SignalRConnection::SignalRConnection(const QUrl &url, const QByteArray &accessToken, NetworkAccessManager *nam, QObject *parent)
    : QObject{parent},
      m_url(url),
      m_accessToken(accessToken),
      m_nam(nam)
{
    m_socket = new QWebSocket();
    typedef void (QWebSocket:: *errorSignal)(QAbstractSocket::SocketError);
    connect(m_socket, static_cast<errorSignal>(&QWebSocket::error), this, [](QAbstractSocket::SocketError error){
        qCWarning(dcEasee) << "SingalR: Error in websocket:" << error;
    });
    connect(m_socket, &QWebSocket::stateChanged, this, [=](QAbstractSocket::SocketState state){
        qCDebug(dcEasee) << "SingalR: Websocket state changed" << state;

        if (state == QAbstractSocket::ConnectedState) {
            qCDebug(dcEasee) << "SingalR: Websocket connected";

            QVariantMap handshake;
            handshake.insert("protocol", "json");
            handshake.insert("version", 1);
            QByteArray data = encode(handshake);
            qCDebug(dcEasee) << "Sending handshake" << data;
            m_socket->sendTextMessage(data);
            m_watchdog->start();
        } else if (QAbstractSocket::UnconnectedState) {
            m_watchdog->stop();
            QTimer::singleShot(5000, this, [=](){
                connectToHost();
            });
        }

    });
    connect(m_socket, &QWebSocket::binaryMessageReceived, this, [](const QByteArray &message){
        qCDebug(dcEasee) << "SingalR: Binary message received" << message;
    });
    connect(m_socket, &QWebSocket::textMessageReceived, this, [=](const QString &message){
        qCDebug(dcEasee) << "SingalR: Text message received" << message;

        QStringList messages = message.split(QByteArray::fromHex("1E"));

        foreach (const QString &msg, messages) {
            if (msg.isEmpty()) {
                continue;
            }
//            qCDebug(dcEasee()) << "Received message:" << msg;

            QJsonParseError error;
            QJsonDocument jsonDoc = QJsonDocument::fromJson(msg.toUtf8(), &error);
            if (error.error != QJsonParseError::NoError) {
                qCWarning(dcEasee()) << "SingalR: Unable to parse message from SignalR socket" << error.errorString() << msg;
                continue;
            }

            if (m_waitingForHandshakeReply && jsonDoc.toVariant().toMap().isEmpty()) {
                m_waitingForHandshakeReply = false;
                qCDebug(dcEasee()) << "SingalR: Handshake reply received.";
                emit connectionStateChanged(true);
                return;
            }

            QVariantMap map = jsonDoc.toVariant().toMap();
            switch (map.value("type").toUInt()) {
            case 1:
                emit dataReceived(map);
                break;
            case 3:
                // Silencing acks to our requests
                qCDebug(dcEasee()) << "SingalR: Message ACK received:" << map;
                break;
            case 6:
                // Resetting watchdog
                m_watchdog->start();
                break;
            default:
                qCWarning(dcEasee()) << "SingalR: Unhandled SingalR message type" << map;
            }
        }
    });

    connectToHost();

    m_watchdog = new QTimer(this);
    m_watchdog->setInterval(30000);
    connect(m_watchdog, &QTimer::timeout, this, [=](){
        qCWarning(dcEasee()) << "SingalR: Watchdog triggered! Reconnecting web socket stream...";
        m_socket->close();
        connectToHost();
    });
}

void SignalRConnection::subscribe(const QString &chargerId)
{
    QVariantMap map;
    map.insert("type", 1);
    map.insert("invocationId", QUuid::createUuid());
    map.insert("target", "SubscribeWithCurrentState");
    map.insert("arguments", QVariantList{chargerId, true});
    qCDebug(dcEasee) << "SingalR: subscribing to" << chargerId;
    m_socket->sendTextMessage(encode(map));
}

bool SignalRConnection::connected() const
{
    return m_socket->state() == QAbstractSocket::ConnectedState;
}

void SignalRConnection::updateToken(const QByteArray &accessToken)
{
    m_accessToken = accessToken;
    m_socket->close();
    connectToHost();
}

QByteArray SignalRConnection::encode(const QVariantMap &message)
{
    return QJsonDocument::fromVariant(message).toJson(QJsonDocument::Compact).append(QByteArray::fromHex("1E"));
}

void SignalRConnection::connectToHost()
{
    QUrl negotiationUrl = m_url;
    negotiationUrl.setScheme("https");
    negotiationUrl.setPath(negotiationUrl.path() + "/negotiate");
    QNetworkRequest negotiateRequest(negotiationUrl);
    negotiateRequest.setRawHeader("Authorization", "Bearer " + m_accessToken);
    qCDebug(dcEasee()) << "SingalR: Negotiating:" << negotiationUrl << negotiateRequest.rawHeader("Authorization");
    QNetworkReply *negotiantionReply = m_nam->post(negotiateRequest, QByteArray());
    connect(negotiantionReply, &QNetworkReply::finished, this, [=](){
        if (negotiantionReply->error() != QNetworkReply::NoError) {
            qCWarning(dcEasee()) << "SingalR: Unable to neotiate SignalR channel:" << negotiantionReply->error();
            QTimer::singleShot(5000, this, [=](){connectToHost();});
            return;
        }
        QByteArray data = negotiantionReply->readAll();
        qCDebug(dcEasee) << "SingalR: Negotiation reply" << data;
        QJsonParseError error;
        QJsonDocument jsonDoc = QJsonDocument::fromJson(data, &error);
        if (error.error != QJsonParseError::NoError) {
            qCWarning(dcEasee()) << "SingalR: Unable to parse json from negoatiate endpoint" << error.errorString() << data;
            QTimer::singleShot(5000, this, [=](){connectToHost();});
            return;
        }

        QVariantMap map = jsonDoc.toVariant().toMap();
        QString connectionId = map.value("connectionId").toString();


        QUrl wsUrl = m_url;
        wsUrl.setScheme("wss");
        QUrlQuery query;
        query.addQueryItem("id", connectionId);
        wsUrl.setQuery(query);
        QNetworkRequest request(wsUrl);
        request.setRawHeader("Authorization", "Bearer " + m_accessToken);
        qCDebug(dcEasee()) << "SingalR: Connecting websocket:" << wsUrl.toString();
        m_waitingForHandshakeReply = true;
#if QT_VERSION >= QT_VERSION_CHECK(5,6,0)
        m_socket->open(request);
#else
        qCWarning(dcEasee()) << "This plugin requires at least Qt 5.6 to establish a signal R connection. Updating values won't work.";
#endif

    });

}

