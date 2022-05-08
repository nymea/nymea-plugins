#include "owletclient.h"
#include "owlettransport.h"
#include "extern-plugininfo.h"

#include <QJsonDocument>
#include <QTimer>

OwletClient::OwletClient(OwletTransport *transport, QObject *parent) :
    QObject(parent),
    m_transport(transport)
{
    transport->setParent(this);

    m_commandTimeoutTimer.setInterval(5000);
    connect(&m_commandTimeoutTimer, &QTimer::timeout, this, [=](){
        if (m_pendingCommandId != -1) {
            QVariantMap params;
            params.insert("error", "TimeoutError");
            emit replyReceived(m_pendingCommandId, params);
            m_pendingCommandId = -1;
            sendNextRequest();
        }
    });

    connect(m_transport, &OwletTransport::dataReceived, this, &OwletClient::dataReceived);
    connect(m_transport, &OwletTransport::error, this, &OwletClient::error);
    connect(m_transport, &OwletTransport::connectedChanged, this, [=](bool isConnected){
        if (isConnected) {
            emit connected();
        } else {
            emit disconnected();
        }
    });
}

bool OwletClient::isConnected() const
{
    return m_transport->connected();
}

OwletTransport *OwletClient::transport() const
{
    return m_transport;
}

int OwletClient::sendCommand(const QString &method, const QVariantMap &params)
{
    if (!m_transport->connected()) {
        qCWarning(dcOwlet()) << "Not connected to owlet. Not sending command.";
        return -1;
    }

    quint16 id = ++m_commandId;

    QVariantMap packet;
    packet.insert("id", id);
    packet.insert("method", method);
    packet.insert("params", params);
    qCDebug(dcOwlet) << "Sending command" << qUtf8Printable(QJsonDocument::fromVariant(packet).toJson());
    Command cmd;
    cmd.id = id;
    cmd.payload = packet;
    m_commandQueue.append(cmd);
    sendNextRequest();
    return id;
}

void OwletClient::dataReceived(const QByteArray &data)
{
    m_receiveBuffer.append(data);

    int splitIndex = m_receiveBuffer.indexOf("}\n{") + 1;
    if (splitIndex <= 0) {
        splitIndex = m_receiveBuffer.length();
    }
    QJsonParseError error;
    QJsonDocument jsonDoc = QJsonDocument::fromJson(m_receiveBuffer.left(splitIndex), &error);
    if (error.error != QJsonParseError::NoError) {
        qCWarning(dcOwlet) << "Could not parse json data from nymea" << m_receiveBuffer.left(splitIndex) << error.errorString();
        return;
    }
    m_receiveBuffer = m_receiveBuffer.right(m_receiveBuffer.length() - splitIndex - 1);
    if (!m_receiveBuffer.isEmpty()) {
        staticMetaObject.invokeMethod(this, "dataReceived", Qt::QueuedConnection, Q_ARG(QByteArray, QByteArray()));
    }

    QVariantMap packet = jsonDoc.toVariant().toMap();

    if (packet.contains("notification")) {
        qCDebug(dcOwlet()) << "Notification received:" << qUtf8Printable(QJsonDocument::fromVariant(packet).toJson());
        emit notificationReceived(packet.value("notification").toString(), packet.value("params").toMap());
    } else if (packet.contains("id")) {
        qCDebug(dcOwlet()) << "Reply received:" << qUtf8Printable(QJsonDocument::fromVariant(packet).toJson());
        int id = packet.value("id").toInt();
        if (id == m_pendingCommandId) {
            m_pendingCommandId = -1;
            sendNextRequest();
        }
        emit replyReceived(id, packet.value("params").toMap());
    } else {
        qCWarning(dcOwlet) << "Unhandled data from owlet" << qUtf8Printable(jsonDoc.toJson(QJsonDocument::Indented));
    }
}

void OwletClient::sendNextRequest()
{
    if (m_commandQueue.isEmpty()) {
        return;
    }
    if (m_pendingCommandId != -1) {
        return;
    }
    Command cmd = m_commandQueue.takeFirst();
    m_pendingCommandId = cmd.id;
    m_transport->sendData(QJsonDocument::fromVariant(cmd.payload).toJson(QJsonDocument::Compact));
    m_commandTimeoutTimer.start();
}
