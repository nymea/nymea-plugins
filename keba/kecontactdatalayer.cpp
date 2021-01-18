#include "kecontactdatalayer.h"
#include "extern-plugininfo.h"

KeContactDataLayer::KeContactDataLayer(QObject *parent) : QObject(parent)
{
    m_udpSocket = new QUdpSocket(this);
    connect(m_udpSocket, &QUdpSocket::readyRead, this, &KeContactDataLayer::readPendingDatagrams);
}

bool KeContactDataLayer::init()
{
    if (!m_udpSocket->bind(QHostAddress::AnyIPv4, m_port, QAbstractSocket::ShareAddress)) {
        qCWarning(dcKebaKeContact()) << "Cannot bind to port" << m_port;
        return false;
    }
    return true;
}

void KeContactDataLayer::write(const QHostAddress &address, const QByteArray &data)
{
    m_udpSocket->writeDatagram(data, address, m_port);
}

void KeContactDataLayer::readPendingDatagrams()
{
    QUdpSocket *socket= qobject_cast<QUdpSocket*>(sender());

    QByteArray datagram;
    QHostAddress senderAddress;
    quint16 senderPort;

    while (socket->hasPendingDatagrams()) {

        datagram.resize(socket->pendingDatagramSize());
        socket->readDatagram(datagram.data(), datagram.size(), &senderAddress, &senderPort);
        qCDebug(dcKebaKeContact()) << "Data received" << datagram << senderAddress;
        emit datagramReceived(senderAddress, datagram);
    }
}
