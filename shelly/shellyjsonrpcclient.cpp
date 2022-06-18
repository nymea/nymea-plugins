#include "shellyjsonrpcclient.h"

#include <QJsonDocument>
#include <QTimer>
#include <QLoggingCategory>
Q_DECLARE_LOGGING_CATEGORY(dcShelly)

ShellyRpcReply::ShellyRpcReply(int id, QObject *parent):
    QObject(parent),
    m_id(id)
{
    QTimer::singleShot(10000, this, [this]{finished(StatusTimeout, QVariantMap());});
    connect(this, &ShellyRpcReply::finished, this, &ShellyRpcReply::deleteLater);
}

int ShellyRpcReply::id() const
{
    return m_id;
}

ShellyJsonRpcClient::ShellyJsonRpcClient(QObject *parent)
    : QObject(parent)
{
    m_socket = new QWebSocket("nymea", QWebSocketProtocol::VersionLatest, this);
    connect(m_socket, &QWebSocket::stateChanged, this, &ShellyJsonRpcClient::stateChanged);

    connect(m_socket, &QWebSocket::textMessageReceived, this, &ShellyJsonRpcClient::onTextMessageReceived);
}

void ShellyJsonRpcClient::open(const QHostAddress &address)
{
    QUrl url;
    url.setScheme("ws");
    url.setHost(address.toString());
    url.setPath("/rpc");
    m_socket->open(url);
}

ShellyRpcReply *ShellyJsonRpcClient::sendRequest(const QString &method)
{
    int id = m_currentId++;

    QVariantMap data;
    data.insert("id", id);
    data.insert("src", "nymea");
    data.insert("method", method);

    ShellyRpcReply *reply = new ShellyRpcReply(id, this);
    connect(reply, &ShellyRpcReply::finished, this, [this, id]{
        m_pendingReplies.remove(id);
    });
    m_pendingReplies.insert(id, reply);

    qCDebug(dcShelly) << "Sending request" << QJsonDocument::fromVariant(data).toJson();
    m_socket->sendTextMessage(QJsonDocument::fromVariant(data).toJson(QJsonDocument::Compact));

    return reply;
}

void ShellyJsonRpcClient::onTextMessageReceived(const QString &message)
{
    qCDebug(dcShelly) << "Text message received from shelly:" << message;

    QJsonParseError error;
    QVariantMap data = QJsonDocument::fromJson(message.toUtf8(), &error).toVariant().toMap();
    if (error.error != QJsonParseError::NoError) {
        qCWarning(dcShelly()) << "Error parsing data from shelly";
        m_socket->close(QWebSocketProtocol::CloseCodeBadOperation);
        return;
    }

    if (data.value("method").toString() == "NotifyStatus") {
        emit notificationReceived(data.value("params").toMap());
        return;
    }

    int id = data.value("id").toInt();
    ShellyRpcReply *reply = m_pendingReplies.take(id);

    if (!reply) {
        qCDebug(dcShelly()) << "Received a message which is neither a notification nor a reply to a request:" << message;
        return;
    }

    reply->finished(ShellyRpcReply::StatusSuccess, data.value("result").toMap());
}
