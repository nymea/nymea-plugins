#include "tcpsocket.h"
#include "extern-plugininfo.h"

TcpSocket::TcpSocket(const QHostAddress address, const quint16 &port, QObject *parent) :
    QObject(parent),
    m_port(port),
    m_address(address)
{
    m_tcpSocket = new QTcpSocket(this);
    connect(m_tcpSocket, &QTcpSocket::connected, this, &TcpSocket::onConnected);
    connect(m_tcpSocket, &QTcpSocket::disconnected, this, &TcpSocket::onDisconnected);
    connect(m_tcpSocket, &QTcpSocket::bytesWritten, this, &TcpSocket::onBytesWritten);
    connect(m_tcpSocket, SIGNAL(error(QAbstractSocket::SocketError)), this, SLOT(onTcpSocketError(QAbstractSocket::SocketError)));
}

void TcpSocket::sendCommand(QByteArray command)
{
    if (m_pendingCommands.isEmpty()) {
        m_pendingCommands.append(command);
        m_tcpSocket->abort();
        m_tcpSocket->connectToHost(m_address, m_port);
    } else {
        m_pendingCommands.append(command);
    }
}

void TcpSocket::connectionTest()
{
    QTcpSocket *testSocket = new QTcpSocket(this);
    connect(testSocket, &QTcpSocket::connected, this,[this, testSocket] {
        emit  connectionTestFinished(true);
        testSocket->deleteLater();
    });
    connect(testSocket, static_cast<void (QTcpSocket::*) (QAbstractSocket::SocketError)>(&QTcpSocket::error), this, [this, testSocket] {
        emit connectionTestFinished(false);
        testSocket->deleteLater();
    });
    testSocket->connectToHost(m_address, m_port);
}

void TcpSocket::onConnected()
{
    qDebug(dcTCPCommander()) << "Socket connected" ;
    if (!m_pendingCommands.isEmpty()) {
        QByteArray data = m_pendingCommands.takeLast();
        qDebug(dcTCPCommander()) << "Writing data:" <<  data;
        m_tcpSocket->write(data + "\n");
    } else {
        m_tcpSocket->disconnectFromHost();
    }
    emit connectionChanged(true);
}

void TcpSocket::onDisconnected()
{
    qDebug(dcTCPCommander()) << "Socket disconnected" ;
    emit connectionChanged(false);
}


void TcpSocket::onBytesWritten()
{
    emit commandSent(true);
    if (!m_pendingCommands.isEmpty()){
        m_tcpSocket->write(m_pendingCommands.takeFirst());
    } else {
        m_tcpSocket->close();
    }
}

void TcpSocket::onError(QAbstractSocket::SocketError error)
{
    qWarning(dcTCPCommander()) << "Socket Error" << m_tcpSocket->errorString();

    switch (error) {
        case QAbstractSocket::RemoteHostClosedError:
            break;
        case QAbstractSocket::HostNotFoundError:
            break;
        case QAbstractSocket::ConnectionRefusedError:
            break;
        default:
        ;
    }
    emit commandSent(false);
    emit connectionChanged(false);

    m_pendingCommands.clear(); //undefined socket state needs to clear command buffer.

    if (m_tcpSocket->isOpen()) {
        m_tcpSocket->disconnectFromHost();
    }
}
