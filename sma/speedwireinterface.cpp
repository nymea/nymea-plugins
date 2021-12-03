#include "speedwireinterface.h"
#include "extern-plugininfo.h"

#include <QTimer>

SpeedwireInterface::SpeedwireInterface(QObject *parent) :
    QObject(parent)
{
    m_socket = new QUdpSocket(this);
    connect(m_socket, &QUdpSocket::readyRead, this, &SpeedwireInterface::readPendingDatagrams);
    connect(m_socket, &QUdpSocket::stateChanged, this, &SpeedwireInterface::onSocketStateChanged);
    connect(m_socket, SIGNAL(error(QAbstractSocket::SocketError)),this, SLOT(onSocketError(QAbstractSocket::SocketError)));
}

SpeedwireInterface::~SpeedwireInterface()
{
    if (m_initialized) {
        if (!m_socket->leaveMulticastGroup(m_multicastAddress)) {
            qCWarning(dcSma()) << "SpeedwireInterface: Failed to leave multicast group" << m_multicastAddress.toString();
        }

        m_socket->close();
    }
}

bool SpeedwireInterface::initialize()
{
    // If we already initialized the socket, we are done
    if (m_initialized)
        return true;

    m_socket->close();
    m_initialized = false;

    if (!m_socket->bind(QHostAddress::AnyIPv4, m_port, QAbstractSocket::ShareAddress)) {
        qCWarning(dcSma()) << "SpeedwireInterface: Cannot bind to port" << m_port << m_socket->errorString();
        return false;
    }

    if (!m_socket->joinMulticastGroup(m_multicastAddress)) {
        qCWarning(dcSma()) << "SpeedwireInterface: Failed to join multicast group" << m_multicastAddress.toString() << m_socket->errorString();
        return false;
    }

    m_initialized = true;
    return true;
}

bool SpeedwireInterface::initialized() const
{
    return m_initialized;
}

bool SpeedwireInterface::startDiscovery()
{
    if (m_discoveryRunning)
        return true;

    qCDebug(dcSma()) << "SpeedwireInterface: Start discovering network...";
    if (!m_initialized) {
        qCDebug(dcSma()) << "SpeedwireInterface: Failed to start discovery because the socket has not been initialized successfully.";
        return false;
    }

    // Discovery message
    QByteArray discoveryDatagram = QByteArray::fromHex("534d4100000402a0ffffffff0000002000000000");
    if (m_socket->write(discoveryDatagram) < 0) {
        qCWarning(dcSma()) << "SpeedwireInterface: Failed to send discovery datagram to multicast address" << m_multicastAddress.toString();
        return false;
    }

    m_discoveryRunning = true;
    QTimer::singleShot(10000, this, [=](){
        qCDebug(dcSma()) << "SpeedwireInterface: Discovey finished.";
        m_discoveryRunning = false;
        emit discoveryFinished();
    });

    return true;
}

bool SpeedwireInterface::discoveryRunning() const
{
    return m_discoveryRunning;
}

void SpeedwireInterface::readPendingDatagrams()
{
    QUdpSocket *socket= qobject_cast<QUdpSocket *>(sender());

    QByteArray datagram;
    QHostAddress senderAddress;
    quint16 senderPort;

    while (socket->hasPendingDatagrams()) {
        datagram.resize(socket->pendingDatagramSize());
        socket->readDatagram(datagram.data(), datagram.size(), &senderAddress, &senderPort);
        qCDebug(dcSma()) << "SpeedwireInterface: Data received from" << senderAddress.toString() << datagram.toHex();
        //emit datagramReceived(senderAddress, datagram);
    }
}

void SpeedwireInterface::onSocketError(QAbstractSocket::SocketError error)
{
    qCDebug(dcSma()) << "SpeedwireInterface: Socket error" << error;
}

void SpeedwireInterface::onSocketStateChanged(QAbstractSocket::SocketState socketState)
{
    qCDebug(dcSma()) << "SpeedwireInterface: Socket state changed" << socketState;
}
