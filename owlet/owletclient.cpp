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

    int id = ++m_commandId;

    QVariantMap packet;
    packet.insert("id", id);
    packet.insert("method", method);
    packet.insert("params", params);
    m_transport->sendData(QJsonDocument::fromVariant(packet).toJson(QJsonDocument::Compact));
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
        //        qWarning() << "Could not parse json data from nymea" << m_receiveBuffer.left(splitIndex) << error.errorString();
        return;
    }
    //    qDebug() << "received response" << qUtf8Printable(jsonDoc.toJson(QJsonDocument::Indented));
    m_receiveBuffer = m_receiveBuffer.right(m_receiveBuffer.length() - splitIndex - 1);
    if (!m_receiveBuffer.isEmpty()) {
        staticMetaObject.invokeMethod(this, "dataReceived", Qt::QueuedConnection, Q_ARG(QByteArray, QByteArray()));
    }

    QVariantMap packet = jsonDoc.toVariant().toMap();

    if (packet.contains("notification")) {
        qCDebug(dcOwlet()) << "Notification received:" << packet;
        emit notificationReceived(packet.value("notification").toString(), packet.value("params").toMap());
    } else if (packet.contains("id")) {
        qCDebug(dcOwlet()) << "reply received:" << packet;
        int id = packet.value("id").toInt();
        emit replyReceived(id, packet.value("params").toMap());
    }
}
