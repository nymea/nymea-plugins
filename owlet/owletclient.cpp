#include "owletclient.h"

#include "extern-plugininfo.h"

#include <QJsonDocument>
#include <QTimer>

OwletClient::OwletClient(QObject *parent) : QObject(parent)
{

}

void OwletClient::connectToHost(const QHostAddress &address, int port)
{
    if (m_socket) {
        m_socket->abort();
        m_socket->deleteLater();
    }

    m_socket = new QTcpSocket(this);
    connect(m_socket, &QTcpSocket::connected, this, [this](){
        emit connected();
    });
    connect(m_socket, &QTcpSocket::disconnected, this, [this, address, port](){
        qCDebug(dcOwlet()) << "Disconnected from owleet";
        emit disconnected();
        QTimer::singleShot(1000, this, [=]{
            connectToHost(address, port);
        });

    });
    typedef void (QTcpSocket:: *errorSignal)(QAbstractSocket::SocketError);
    connect(m_socket, static_cast<errorSignal>(&QTcpSocket::error), this, [this](){
        qCDebug(dcOwlet()) << "Error in owlet communication";
        emit error();
    });

    connect(m_socket, &QTcpSocket::readyRead, this, [this](){
        dataReceived(m_socket->readAll());
    });
    m_socket->connectToHost(address, port);
}

int OwletClient::sendCommand(const QString &method, const QVariantMap &params)
{
    if (!m_socket) {
        qCWarning(dcOwlet()) << "Not connected to owlet. Not sending command.";
        return -1;
    }

    int id = ++m_commandId;

    QVariantMap packet;
    packet.insert("id", id);
    packet.insert("method", method);
    packet.insert("params", params);
    m_socket->write(QJsonDocument::fromVariant(packet).toJson(QJsonDocument::Compact));
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
