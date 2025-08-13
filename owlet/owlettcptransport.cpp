#include "owlettcptransport.h"
#include "extern-plugininfo.h"

#include <QTimer>

OwletTcpTransport::OwletTcpTransport(const QHostAddress &hostAddress, quint16 port, QObject *parent) :
    OwletTransport(parent),
    m_socket(new QTcpSocket(this)),
    m_hostAddress(hostAddress),
    m_port(port)
{
    connect(m_socket, &QTcpSocket::connected, this, [=](){
        emit connectedChanged(true);
    });
    connect(m_socket, &QTcpSocket::disconnected, this, [=](){
        qCDebug(dcOwlet()) << "TCP transport: Disconnected from owlet" << QString("%1:%2").arg(m_hostAddress.toString()).arg(m_port);
        emit connectedChanged(false);
        QTimer::singleShot(1000, this, &OwletTcpTransport::connectTransport);
    });

    typedef void (QTcpSocket:: *errorSignal)(QAbstractSocket::SocketError);
    connect(m_socket, static_cast<errorSignal>(&QTcpSocket::error), this, [this](){
        qCDebug(dcOwlet()) << "TCP transport: Error in owlet communication" << m_socket->errorString();
        emit error();
    });

    connect(m_socket, &QTcpSocket::readyRead, this, [this](){
        emit dataReceived(m_socket->readAll());
    });
}

bool OwletTcpTransport::connected() const
{
    return m_socket->state() == QAbstractSocket::ConnectedState;
}

void OwletTcpTransport::sendData(const QByteArray &data)
{
    m_socket->write(data);
}

void OwletTcpTransport::connectTransport()
{
    qCDebug(dcOwlet()) << "Connecting to" << m_hostAddress << m_port;
    m_socket->connectToHost(m_hostAddress, m_port);
}

void OwletTcpTransport::disconnectTransport()
{
    m_socket->close();
}
