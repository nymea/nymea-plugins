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
        qCWarning(dcEasee) << "Error in websocket:" << error;
    });
    connect(m_socket, &QWebSocket::stateChanged, this, [=](QAbstractSocket::SocketState state){
        qCDebug(dcEasee) << "Websocket state changed" << state;

        if (state == QAbstractSocket::ConnectedState) {
            qCDebug(dcEasee) << "Websocket connected";

            QVariantMap handshake;
            handshake.insert("protocol", "json");
            handshake.insert("version", 1);
            QByteArray data = encode(handshake);
            qCDebug(dcEasee) << "Sending handshake" << data;
            m_socket->sendTextMessage(data);
        } else if (QAbstractSocket::UnconnectedState) {
            QTimer::singleShot(5000, this, [=](){
                connectToHost();
            });
        }

    });
    connect(m_socket, &QWebSocket::binaryMessageReceived, this, [](const QByteArray &message){
        qCDebug(dcEasee) << "Binary message received" << message;
    });
    connect(m_socket, &QWebSocket::textMessageReceived, this, [=](const QString &message){
        QStringList messages = message.split(QByteArray::fromHex("1E"));

        foreach (const QString &msg, messages) {
            if (msg.isEmpty()) {
                continue;
            }
//            qCDebug(dcEasee()) << "Received message:" << msg;

            QJsonParseError error;
            QJsonDocument jsonDoc = QJsonDocument::fromJson(msg.toUtf8(), &error);
            if (error.error != QJsonParseError::NoError) {
                qCWarning(dcEasee()) << "Unable to parse message from SignalR socket" << error.errorString() << msg;
                continue;
            }

            if (m_waitingForHandshakeReply && jsonDoc.toVariant().toMap().isEmpty()) {
                m_waitingForHandshakeReply = false;
                qCDebug(dcEasee()) << "Handshake reply received.";
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
                qCDebug(dcEasee()) << "Message ACK received:" << map;
            case 6:
                // Silencing pings
                break;
            default:
                qCWarning(dcEasee()) << "Unhandled signalr message type" << map;
            }
        }
    });

    connectToHost();
}

void SignalRConnection::subscribe(const QString &chargerId)
{
    QVariantMap map;
    map.insert("type", 1);
    map.insert("invocationId", QUuid::createUuid());
    map.insert("target", "SubscribeWithCurrentState");
    map.insert("arguments", QVariantList{chargerId, true});
    qCDebug(dcEasee) << "subscribing to" << chargerId;
    m_socket->sendTextMessage(encode(map));
}

bool SignalRConnection::connected() const
{
    return m_socket->state() == QAbstractSocket::ConnectedState;
}

void SignalRConnection::updateToken(const QByteArray &accessToken)
{
    m_accessToken = accessToken;
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
    qCDebug(dcEasee()) << "Negotiating:" << negotiationUrl << negotiateRequest.rawHeader("Authorization");
    QNetworkReply *negotiantionReply = m_nam->post(negotiateRequest, QByteArray());
    connect(negotiantionReply, &QNetworkReply::finished, this, [=](){
        if (negotiantionReply->error() != QNetworkReply::NoError) {
            qCWarning(dcEasee()) << "Unable to neotiate SignalR channel:" << negotiantionReply->error();
            return;
        }
        QByteArray data = negotiantionReply->readAll();
        qCDebug(dcEasee) << "Negotiation reply" << data;
        QJsonParseError error;
        QJsonDocument jsonDoc = QJsonDocument::fromJson(data, &error);
        if (error.error != QJsonParseError::NoError) {
            qCWarning(dcEasee()) << "Unable to parse json from negoatiate endpoint" << error.errorString() << data;
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
        qCDebug(dcEasee()) << "Connecting websocket:" << wsUrl.toString();
        m_waitingForHandshakeReply = true;
#if QT_VERSION >= QT_VERSION_CHECK(5,6,0)
        m_socket->open(request);
#else
        qCWarning(dcEasee()) << "This plugin requires at least Qt 5.6 to establish a signal R connection. Updating values won't work.";
#endif

    });

}

