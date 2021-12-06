#include "speedwireinterface.h"
#include "extern-plugininfo.h"

SpeedwireInterface::SpeedwireInterface(const QHostAddress &address, bool multicast, QObject *parent) :
    QObject(parent),
    m_address(address),
    m_multicast(multicast)
{
    m_socket = new QUdpSocket(this);
    connect(m_socket, &QUdpSocket::readyRead, this, &SpeedwireInterface::readPendingDatagrams);
    connect(m_socket, &QUdpSocket::stateChanged, this, &SpeedwireInterface::onSocketStateChanged);
    connect(m_socket, SIGNAL(error(QAbstractSocket::SocketError)),this, SLOT(onSocketError(QAbstractSocket::SocketError)));



    // Request  534d4100000402a00000000100260010 606509a0 7a01842a71b30001 7d0042be283a0001 000000000480 00020058 001e8200 ff208200 00000000 =>  query device type
    // Response 534d4100000402a000000001009e0010 606527a0 7d0042be283a00a1 7a01842a71b30001 000000000480 01020058 01000000 03000000 011e8210 6f89e95f 534e3a20 33303130 35333831 31360000 00000000 00000000 00000000 00000000
    //                                                                                                                              011f8208 6f89e95f 411f0001 feffff00 00000000 00000000 00000000 00000000 00000000 00000000  => 1f41 solar inverter
    //                                                                                                                              01208208 6f89e95f 96240000 80240000 81240001 82240000 feffff00 00000000 00000000 00000000 00000000
    // Request  534d4100000402a00000000100260010 606509a0 7a01842a71b30001 7d0042be283a0001 000000000480 00028053 001e2500 ff1e2500 00000000 =>  query spot dc power
    // Response 534d4100000402a000000001005e0010 606517a0 7d0042be283a00a1 7a01842a71b30001 000000000480 01028053 00000000 01000000 011e2540 61a7e95f 57000000 57000000 57000000 57000000 01000000
    //                                                                                                                              021e2540 61a7e95f 5e000000 5e000000 5e000000 5e000000 01000000 00000000
    // Request  534d4100000402a00000000100260010 606509a0 7a01842a71b30001 7d0042be283a0001 000000000580 00028053 001f4500 ff214500 00000000 =>  query spot dc voltage/current
    // Response 534d4100000402a00000000100960010 606525a0 7d0042be283a00a1 7a01842a71b30001 000000000580 01028053 02000000 05000000 011f4540 61a7e95f 05610000 05610000 05610000 05610000 01000000
    //                                                                                                                              021f4540 61a7e95f 505b0000 505b0000 505b0000 505b0000 01000000
    //                                                                                                                              01214540 61a7e95f 60010000 60010000 60010000 60010000 01000000
    //                                                                                                                              02214540 61a7e95f 95010000 95010000 95010000 95010000 01000000 00000000
    // Request  534d4100000402a00000000100260010 606509a0 7a01842a71b30001 7d0042be283a0001 000000000680 00020051 00404600 ff424600 00000000 =>  query spot ac power
    // Response 534d4100000402a000000001007a0010 60651ea0 7d0042be283a00a1 7a01842a71b30001 000000000680 01020051 09000000 0b000000 01404640 61a7e95f 38000000 38000000 38000000 38000000 01000000
    //                                                                                                                              01414640 61a7e95f 37000000 37000000 37000000 37000000 01000000
    //                                                                                                                              01424640 61a7e95f 39000000 39000000 39000000 39000000 01000000 00000000
    // Request  534d4100000402a00000000100260010 606509a0 7a01842a71b30001 7d0042be283a0001 000000000780 00020051 00484600 ff554600 00000000 =>  query spot ac voltage/current
    // Response 534d4100000402a000000001013e0010 60654fa0 7d0042be283a00a1 7a01842a71b30001 000000000780 01020051 0c000000 15000000 01484600 61a7e95f 5a590000 5a590000 5a590000 5a590000 01000000
    //                                                                                                                              01494600 61a7e95f cf590000 cf590000 cf590000 cf590000 01000000
    //                                                                                                                              014a4600 61a7e95f 7a590000 7a590000 7a590000 7a590000 01000000
    //                                                                                                                              014b4600 61a7e95f f19a0000 f19a0000 f19a0000 f19a0000 01000000
    //                                                                                                                              014c4600 61a7e95f 3c9b0000 3c9b0000 3c9b0000 3c9b0000 01000000
    //                                                                                                                              014d4600 61a7e95f 189b0000 189b0000 189b0000 189b0000 01000000
    //                                                                                                                              014e4600 51a7e95f 1d000000 1d000000 1d000000 1d000000 01000000
    //                                                                                                                              01534640 61a7e95f 24010000 24010000 24010000 24010000 01000000
    //                                                                                                                              01544640 61a7e95f 1e010000 1e010000 1e010000 1e010000 01000000
    //                                                                                                                              01554640 61a7e95f 23010000 23010000 23010000 23010000 01000000 00000000
    // Request  534d4100000402a00000000100260010 606509a0 7a01842a71b30001 7d0042be283a0001 000000000980 00028051 00482100 ff482100 00000000 =>  query device status
    // Response 534d4100000402a000000001004e0010 606513a0 7d0042be283a00a1 7a01842a71b30001 000000000980 01028051 00000000 00000000 01482108 59c5e95f 33010001 feffff00 00000000 00000000 00000000 00000000 00000000 00000000 00000000
    // Request  534d4100000402a00000000100260010 606509a0 7a01842a71b30001 7d0042be283a0001 000000000a80 00028051 00644100 ff644100 00000000 =>  query grid relay status
    // Response 534d4100000402a000000001004e0010 606513a0 7d0042be283a00a1 7a01842a71b30001 000000000a80 01028051 07000000 07000000 01644108 59c5e95f 33000001 37010000 fdffff00 feffff00 00000000 00000000 00000000 00000000 00000000
}

SpeedwireInterface::~SpeedwireInterface()
{
    deinitialize();
}

bool SpeedwireInterface::initialize()
{
    if (!m_socket->bind(QHostAddress::AnyIPv4, m_port, QAbstractSocket::ShareAddress | QAbstractSocket::ReuseAddressHint)) {
        qCWarning(dcSma()) << "SpeedwireInterface: Initialization failed. Could not bind to port" << m_port;
        return false;
    }

    if (m_multicast && !m_socket->joinMulticastGroup(m_multicastAddress)) {
        qCWarning(dcSma()) << "SpeedwireDiscovery: Initialization failed. Could not join multicast group" << m_multicastAddress.toString() << m_socket->errorString();
        return false;
    }

    qCDebug(dcSma()) << "SpeedwireInterface: Interface initialized successfully.";
    m_initialized = true;
    return m_initialized;
}

void SpeedwireInterface::deinitialize()
{
    if (m_initialized) {
        if (m_multicast) {
            if (!m_socket->leaveMulticastGroup(m_multicastAddress)) {
                qCWarning(dcSma()) << "SpeedwireDiscovery: Failed to leave multicast group" << m_multicastAddress.toString();
            }

            m_socket->close();
            m_initialized = false;
        }
    }
}

bool SpeedwireInterface::initialized() const
{
    return m_initialized;
}

quint16 SpeedwireInterface::sourceModelId() const
{
    return m_sourceModelId;
}

quint32 SpeedwireInterface::sourceSerialNumber() const
{
    return m_sourceSerialNumber;
}

SpeedwireInterface::SpeedwireHeader SpeedwireInterface::parseHeader(QDataStream &stream)
{
    SpeedwireHeader header;
    quint16 protocolId;
    stream >> header.smaSignature >> header.headerLength;
    stream >> header.tagType >> header.tagVersion >> header.group;
    stream >> header.payloadLength >> header.smaNet2Version;
    stream >> protocolId;
    header.protocolId = static_cast<ProtocolId>(protocolId);
    return header;
}

void SpeedwireInterface::sendData(const QByteArray &data)
{
    m_socket->writeDatagram(data, m_address, m_port);
}

void SpeedwireInterface::readPendingDatagrams()
{
    QByteArray datagram;
    QHostAddress senderAddress;
    quint16 senderPort;

    while (m_socket->hasPendingDatagrams()) {
        datagram.resize(m_socket->pendingDatagramSize());
        m_socket->readDatagram(datagram.data(), datagram.size(), &senderAddress, &senderPort);

        // Process only data coming from our target address
        if (senderAddress != m_address)
            continue;

        qCDebug(dcSma()) << "SpeedwireInterface: Received data from" << QString("%1:%2").arg(senderAddress.toString()).arg(senderPort);
        qCDebug(dcSma()) << "SpeedwireInterface: " << datagram.toHex();
        emit dataReceived(datagram);
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
