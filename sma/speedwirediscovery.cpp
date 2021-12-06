#include "speedwirediscovery.h"
#include "extern-plugininfo.h"

#include <QDataStream>
#include <speedwirediscovery.h>

SpeedwireDiscovery::SpeedwireDiscovery(NetworkDeviceDiscovery *networkDeviceDiscovery, QObject *parent) :
    QObject(parent),
    m_networkDeviceDiscovery(networkDeviceDiscovery)
{
    // More details: https://github.com/RalfOGit/libspeedwire/


    //    //! Multicast device discovery request packet, according to SMA documentation.
    //    //! However, this does not seem to be supported anymore with version 3.x devices
    //    const unsigned char multicast_request[] = {
    //        0x53, 0x4d, 0x41, 0x00, 0x00, 0x04, 0x02, 0xa0,     // sma signature, tag0
    //        0xff, 0xff, 0xff, 0xff, 0x00, 0x00, 0x00, 0x20,     // 0xffffffff group, 0x0000 length, 0x0020 "SMA Net ?", Version ?
    //        0x00, 0x00, 0x00, 0x00                              // 0x0000 protocol, 0x00 #long words, 0x00 ctrl
    //    };

    //    //! Unicast device discovery request packet, according to SMA documentation
    //    const unsigned char unicast_request[] = {
    //        0x53, 0x4d, 0x41, 0x00, 0x00, 0x04, 0x02, 0xa0,     // sma signature, tag0
    //        0x00, 0x00, 0x00, 0x01, 0x00, 0x26, 0x00, 0x10,     // 0x26 length, 0x0010 "SMA Net 2", Version 0
    //        0x60, 0x65, 0x09, 0xa0, 0xff, 0xff, 0xff, 0xff,     // 0x6065 protocol, 0x09 #long words, 0xa0 ctrl, 0xffff dst susyID any, 0xffffffff dst serial any
    //        0xff, 0xff, 0x00, 0x00, 0x7d, 0x00, 0x52, 0xbe,     // 0x0000 dst cntrl, 0x007d src susy id, 0x3a28be52 src serial
    //        0x28, 0x3a, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,     // 0x0000 src cntrl, 0x0000 error code, 0x0000 fragment ID
    //        0x01, 0x80, 0x00, 0x02, 0x00, 0x00, 0x00, 0x00,     // 0x8001 packet ID
    //        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    //        0x00, 0x00
    //    };

    //    // Request: 534d4100000402a00000000100260010 606509a0 ffffffffffff0000 7d0052be283a0000 000000000180 00020000 00000000 00000000 00000000  => command = 0x00000200, first = 0x00000000; last = 0x00000000; trailer = 0x00000000
    //    // Response 534d4100000402a000000001004e0010 606513a0 7d0052be283a00c0 7a01842a71b30000 000000000180 01020000 00000000 00000000 00030000 00ff0000 00000000 01007a01 842a71b3 00000a00 0c000000 00000000 00000000 01010000 00000000








    //    qCDebug(dcSma()) << "SpeedwireDiscovery: Create speed wire interface for multicast" << m_multicastAddress.toString() << "on port" << m_port;
    //    QByteArray exampleData = QByteArray::fromHex("534d4100000402a000000001024400106069010e714369aee618a41600010400000000000001080000000021391229100002040000004415000208000000001575a137d800030400000000000003080000000003debed0e800040400000017c6000408000000001008c2070000090400000000000009080000000027c77bed20000a04000000481d000a08000000001722823410000d0400000003b00015040000000000001508000000000d1e1e0e3000160400000015120016080000000006c5a2d8b800170400000000000017080000000001bd6f680000180400000007990018080000000004def712b8001d040000000000001d08000000000eeefaafd0001e040000001666001e0800000000074b38bf88001f040000000a300020040000037bcb00210400000003ad0029040000000000002908000000000a9b1afec8002a040000001a81002a08000000000803e62b88002b040000000000002b080000000001511459b8002c0400000006d5002c0800000000052c8455b80031040000000000003108000000000cf83b37100032040000001b5f0032080000000008a6e257f80033040000000c3f003404000003747900350400000003c8003d040000000000003d08000000000a53d0ba08003e040000001482003e080000000007800fd188003f040000000000003f080000000001185820c8004004000000095800400800000000064563b1900045040000000000004508000000000d26d3eae0004604000000168900460800000000082b4fc5a80047040000000a440048040000037ed1004904000000038e900000000102085200000000");
    //    processDatagram(QHostAddress("127.0.0.1"), m_port, exampleData);

    m_multicastSocket = new QUdpSocket(this);
    connect(m_multicastSocket, &QUdpSocket::readyRead, this, &SpeedwireDiscovery::readPendingDatagramsMulticast);
    connect(m_multicastSocket, &QUdpSocket::stateChanged, this, &SpeedwireDiscovery::onSocketStateChanged);
    connect(m_multicastSocket, SIGNAL(error(QAbstractSocket::SocketError)),this, SLOT(onSocketError(QAbstractSocket::SocketError)));

    m_unicastSocket = new QUdpSocket(this);
    connect(m_unicastSocket, &QUdpSocket::readyRead, this, &SpeedwireDiscovery::readPendingDatagramsUnicast);
    connect(m_unicastSocket, &QUdpSocket::stateChanged, this, &SpeedwireDiscovery::onSocketStateChanged);
    connect(m_unicastSocket, SIGNAL(error(QAbstractSocket::SocketError)),this, SLOT(onSocketError(QAbstractSocket::SocketError)));

    m_discoveryTimer.setInterval(1000);
    m_discoveryTimer.setSingleShot(false);
    connect(&m_discoveryTimer, &QTimer::timeout, this, &SpeedwireDiscovery::sendDiscoveryRequest);
}

SpeedwireDiscovery::~SpeedwireDiscovery()
{
    if (m_initialized) {
        if (!m_multicastSocket->leaveMulticastGroup(m_multicastAddress)) {
            qCWarning(dcSma()) << "SpeedwireDiscovery: Failed to leave multicast group" << m_multicastAddress.toString();
        }

        m_multicastSocket->close();
    }
}

bool SpeedwireDiscovery::initialize()
{
    m_multicastSocket->close();
    m_initialized = false;

    // Setup multicast socket
    if (!m_multicastSocket->bind(QHostAddress::AnyIPv4, m_port, QAbstractSocket::ShareAddress | QAbstractSocket::ReuseAddressHint)) {
        qCWarning(dcSma()) << "SpeedwireDiscovery: Initialization failed. Could not bind multicast socket to port" << m_port << m_multicastSocket->errorString();
        return false;
    }

    if (!m_multicastSocket->joinMulticastGroup(m_multicastAddress)) {
        qCWarning(dcSma()) << "SpeedwireDiscovery: Initialization failed. Could not join multicast group" << m_multicastAddress.toString() << m_multicastSocket->errorString();
        return false;
    }

    // Setup unicast socket
    if (!m_unicastSocket->bind(QHostAddress::AnyIPv4, m_port, QAbstractSocket::ShareAddress | QAbstractSocket::ReuseAddressHint)) {
        qCWarning(dcSma()) << "SpeedwireDiscovery: Initialization failed. Could not bind to port" << m_port << m_multicastSocket->errorString();
        return false;
    }

    qCDebug(dcSma()) << "SpeedwireDiscovery: Interface initialized successfully.";
    m_initialized = true;
    return m_initialized;
}

bool SpeedwireDiscovery::initialized() const
{
    return m_initialized;
}

bool SpeedwireDiscovery::startDiscovery()
{
    // 1. Discover all network devices
    // 2. Send upd multicast and unicast messages to verify if it is a SMA speedwire device

    if (m_discoveryRunning)
        return true;

    if (!m_initialized) {
        qCDebug(dcSma()) << "SpeedwireDiscovery: Failed to start discovery because the socket has not been initialized successfully.";
        return false;
    }

    // CLean up
    m_results.clear();
    m_networkDeviceInfos.clear();

    qCDebug(dcSma()) << "SpeedwireDiscovery: Start discovering network...";
    NetworkDeviceDiscoveryReply *discoveryReply = m_networkDeviceDiscovery->discover();
    connect(discoveryReply, &NetworkDeviceDiscoveryReply::finished, this, [=](){
        qCDebug(dcSma()) << "Discovery finished. Found" << discoveryReply->networkDeviceInfos().count() << "devices";
        m_networkDeviceInfos = discoveryReply->networkDeviceInfos();

        foreach (const NetworkDeviceInfo &networkDeviceInfo, discoveryReply->networkDeviceInfos()) {
            // 2. Send unicast to all results and start requesting on multicast address
            sendUnicastDiscoveryRequest(networkDeviceInfo.address());
        }

        startMulticastDiscovery();
    });

    return true;
}

bool SpeedwireDiscovery::discoveryRunning() const
{
    return m_discoveryRunning;
}

QList<SpeedwireDiscovery::SpeedwireDiscoveryResult> SpeedwireDiscovery::discoveryResult() const
{
    return m_results.values();
}

void SpeedwireDiscovery::startMulticastDiscovery()
{
    // Start sending multicast messages
    sendDiscoveryRequest();

    m_discoveryRunning = true;
    QTimer::singleShot(5000, this, &SpeedwireDiscovery::onDiscoveryProcessFinished);

    m_discoveryTimer.start();
}

void SpeedwireDiscovery::sendUnicastDiscoveryRequest(const QHostAddress &targetHostAddress)
{
    if (m_unicastSocket->writeDatagram(m_discoveryDatagramUnicast, targetHostAddress, m_port) < 0) {
        qCWarning(dcSma()) << "SpeedwireDiscovery: Failed to send unicast discovery datagram to address" << targetHostAddress.toString();
        return;
    }

    qCDebug(dcSma()) << "SpeedwireDiscovery: Sent successfully the discovery request to unicast address" << targetHostAddress.toString();
}

void SpeedwireDiscovery::readPendingDatagramsMulticast()
{
    QUdpSocket *socket = qobject_cast<QUdpSocket *>(sender());

    QByteArray datagram;
    QHostAddress senderAddress;
    quint16 senderPort;

    while (socket->hasPendingDatagrams()) {
        datagram.resize(socket->pendingDatagramSize());
        socket->readDatagram(datagram.data(), datagram.size(), &senderAddress, &senderPort);
        qCDebug(dcSma()) << "SpeedwireDiscovery: Received multicast data from" << QString("%1:%2").arg(senderAddress.toString()).arg(senderPort);
        //qCDebug(dcSma()) << "SpeedwireDiscovery: " << datagram.toHex();
        processDatagram(senderAddress, senderPort, datagram);
    }
}

void SpeedwireDiscovery::readPendingDatagramsUnicast()
{
    QUdpSocket *socket = qobject_cast<QUdpSocket *>(sender());

    QByteArray datagram;
    QHostAddress senderAddress;
    quint16 senderPort;

    while (socket->hasPendingDatagrams()) {
        datagram.resize(socket->pendingDatagramSize());
        socket->readDatagram(datagram.data(), datagram.size(), &senderAddress, &senderPort);
        qCDebug(dcSma()) << "SpeedwireDiscovery: Received unicast data from" << QString("%1:%2").arg(senderAddress.toString()).arg(senderPort);
        //qCDebug(dcSma()) << "SpeedwireDiscovery: " << datagram.toHex();
        processDatagram(senderAddress, senderPort, datagram);
    }
}


void SpeedwireDiscovery::onSocketError(QAbstractSocket::SocketError error)
{
    qCDebug(dcSma()) << "SpeedwireDiscovery: Socket error" << error;
}

void SpeedwireDiscovery::onSocketStateChanged(QAbstractSocket::SocketState socketState)
{
    qCDebug(dcSma()) << "SpeedwireDiscovery: Socket state changed" << socketState;
}

void SpeedwireDiscovery::processDatagram(const QHostAddress &senderAddress, quint16 senderPort, const QByteArray &datagram)
{
    // Check min size of SMA datagrams
    if (datagram.size() < 18) {
        qCDebug(dcSma()) << "SpeedwireDiscovery: Received datagram is to short to be a SMA speedwire message. Ignoring data...";
        return;
    }

    // Ignore discovery requests
    if (datagram == m_discoveryDatagramMulticast || datagram == m_discoveryDatagramUnicast)
        return;

    QDataStream stream(datagram);
    stream.setByteOrder(QDataStream::BigEndian);

    SpeedwireInterface::SpeedwireHeader header = SpeedwireInterface::parseHeader(stream);
    if (!header.isValid()) {
        qCWarning(dcSma()) << "SpeedwireDiscovery: Datagram header is not valid. Ignoring data...";
        return;
    }
    // Example data:
    // 534d4100 0004 02a0 0000 0001 0244 0010 6069 010e 7143 69ae     e618a416 00010400000000000001080000000021391229100002040000004415000208000000001575a137d800030400000000000003080000000003debed0e800040400000017c6000408000000001008c2070000090400000000000009080000000027c77bed20000a04000000481d000a08000000001722823410000d0400000003b00015040000000000001508000000000d1e1e0e3000160400000015120016080000000006c5a2d8b800170400000000000017080000000001bd6f680000180400000007990018080000000004def712b8001d040000000000001d08000000000eeefaafd0001e040000001666001e0800000000074b38bf88001f040000000a300020040000037bcb00210400000003ad0029040000000000002908000000000a9b1afec8002a040000001a81002a08000000000803e62b88002b040000000000002b080000000001511459b8002c0400000006d5002c0800000000052c8455b80031040000000000003108000000000cf83b37100032040000001b5f0032080000000008a6e257f80033040000000c3f003404000003747900350400000003c8003d040000000000003d08000000000a53d0ba08003e040000001482003e080000000007800fd188003f040000000000003f080000000001185820c8004004000000095800400800000000064563b1900045040000000000004508000000000d26d3eae0004604000000168900460800000000082b4fc5a80047040000000a440048040000037ed1004904000000038e900000000102085200000000



    qCDebug(dcSma()) << "SpeedwireDiscovery: ======================= Header";
    qCDebug(dcSma()) << "SpeedwireDiscovery: Length:" << header.headerLength;
    qCDebug(dcSma()) << "SpeedwireDiscovery: Tag0:" << header.tagType;
    qCDebug(dcSma()) << "SpeedwireDiscovery: Tag0 version:" << header.tagVersion;
    qCDebug(dcSma()) << "SpeedwireDiscovery: Group:" << header.group << (header.group == 1 ? "(default group)" : "");
    qCDebug(dcSma()) << "SpeedwireDiscovery: Data length:" << header.payloadLength << datagram.length();
    qCDebug(dcSma()) << "SpeedwireDiscovery: SMA Net 2 Version" << header.smaNet2Version;
    qCDebug(dcSma()) << "SpeedwireDiscovery: Protocol ID" << header.protocolId;

    if (header.protocolId == SpeedwireInterface::ProtocolIdDiscoveryResponse) {
        qCDebug(dcSma()) << "SpeedwireDiscovery: Received discovery response from" << QString("%1:%2").arg(senderAddress.toString()).arg(senderPort);
        //              Response: 534d4100 0004 02a0 0000 0001 0002 0000 0001
        // "192.168.178.25:9522" "534d4100 0004 02a0 0000 0001 0002 0000 0001 0004 0010 0001 0003 0004 0020 0000 0001 0004 0030 c0a8 b219 0004 0040 0000 0000 0002 0070 ef0c 00000000"
        // "192.168.178.22:9522" "534d4100 0004 02a0 0000 0001 0002 0000 0001 0004 0010 0001 0001 0004 0020 0000 0001 0004 0030 c0a8 b216 0004 0040 0000 0001 00000000"

        if (!datagram.startsWith(m_discoveryResponseDatagram)) {
            qCWarning(dcSma()) << "SpeedwireDiscovery: Received discovery reply but the message start does not match the required schema. Ignoring data...";
            return;
        }

        if (!m_results.contains(senderAddress)) {
            qCDebug(dcSma()) << "SpeedwireDiscovery: --> Found SMA device on" << senderAddress.toString();
            if (!m_networkDeviceInfos.hasHostAddress(senderAddress)) {
                qCWarning(dcSma()) << "SpeedwireDiscovery: Found SMA using UDP discovery but the host is not in the network discovery result list. Not adding to results" << senderAddress.toString();
                return;
            }


            SpeedwireDiscoveryResult result;
            result.address = senderAddress;
            if (m_networkDeviceInfos.hasHostAddress(senderAddress)) {
                result.networkDeviceInfo = m_networkDeviceInfos.get(senderAddress);
            }
            result.deviceType = SpeedwireInterface::DeviceTypeUnknown;
            m_results.insert(senderAddress, result);
        } else {
            if (m_networkDeviceInfos.hasHostAddress(senderAddress)) {
                m_results[senderAddress].networkDeviceInfo = m_networkDeviceInfos.get(senderAddress);
            }
        }
        return;
    }

    // We received SMA data, let's parse depending on the protocol id

    if (header.protocolId == SpeedwireInterface::ProtocolIdMeter) {
        // Example: 010e 714369ae
        quint16 modelId;
        quint32 serialNumber;
        stream >> modelId >> serialNumber;
        qCDebug(dcSma()) << "SpeedwireDiscovery: ======================= Meter identifier";
        qCDebug(dcSma()) << "SpeedwireDiscovery: Model ID:" << modelId;
        qCDebug(dcSma()) << "SpeedwireDiscovery: Serial number:" << serialNumber;

        if (!m_results.contains(senderAddress)) {
            SpeedwireDiscoveryResult result;
            result.address = senderAddress;
            result.deviceType = SpeedwireInterface::DeviceTypeMeter;
            m_results.insert(senderAddress, result);
        }

        if (m_networkDeviceInfos.hasHostAddress(senderAddress)) {
            m_results[senderAddress].networkDeviceInfo = m_networkDeviceInfos.get(senderAddress);
        }

        m_results[senderAddress].modelId = modelId;
        m_results[senderAddress].serialNumber = serialNumber;
    } else if (header.protocolId == SpeedwireInterface::ProtocolIdInverter) {
        quint16 modelId;
        quint32 serialNumber;
        stream >> modelId >> serialNumber;
        qCDebug(dcSma()) << "SpeedwireDiscovery: ======================= Inverter identifier";
        qCDebug(dcSma()) << "SpeedwireDiscovery: Model ID:" << modelId;
        qCDebug(dcSma()) << "SpeedwireDiscovery: Serial number:" << serialNumber;

        if (!m_results.contains(senderAddress)) {
            SpeedwireDiscoveryResult result;
            result.address = senderAddress;
            result.deviceType = SpeedwireInterface::DeviceTypeInverter;
            m_results.insert(senderAddress, result);
        }

        if (m_networkDeviceInfos.hasHostAddress(senderAddress)) {
            m_results[senderAddress].networkDeviceInfo = m_networkDeviceInfos.get(senderAddress);
        }

        m_results[senderAddress].modelId = modelId;
        m_results[senderAddress].serialNumber = serialNumber;
    } else {
        qCWarning(dcSma()) << "SpeedwireDiscovery: Unhandled data received" << datagram.toHex();
        return;
    }

}

void SpeedwireDiscovery::sendDiscoveryRequest()
{
    if (m_multicastSocket->writeDatagram(m_discoveryDatagramMulticast, m_multicastAddress, m_port) < 0) {
        qCWarning(dcSma()) << "SpeedwireDiscovery: Failed to send discovery datagram to multicast address" << m_multicastAddress.toString();
        return;
    }

    qCDebug(dcSma()) << "SpeedwireDiscovery: Sent successfully the discovery request to multicast address" << m_multicastAddress.toString();
}

void SpeedwireDiscovery::onDiscoveryProcessFinished()
{
    qCDebug(dcSma()) << "SpeedwireDiscovery: Discovey finished. Found" << m_results.count() << "SMA devices in the network";
    m_discoveryTimer.stop();
    m_discoveryRunning = false;

    foreach (const SpeedwireDiscoveryResult &result, m_results) {
        qCDebug(dcSma()) << "SpeedwireDiscovery: ============================================";
        qCDebug(dcSma()) << "SpeedwireDiscovery: Device type:" << result.deviceType;
        qCDebug(dcSma()) << "SpeedwireDiscovery: Address:" << result.address.toString();
        qCDebug(dcSma()) << "SpeedwireDiscovery: Hostname:" << result.networkDeviceInfo.hostName();
        qCDebug(dcSma()) << "SpeedwireDiscovery: MAC:" << result.networkDeviceInfo.macAddress();
        qCDebug(dcSma()) << "SpeedwireDiscovery: MAC manufacturer:" << result.networkDeviceInfo.macAddressManufacturer();
        qCDebug(dcSma()) << "SpeedwireDiscovery: Model ID:" << result.modelId;
        qCDebug(dcSma()) << "SpeedwireDiscovery: Serial number:" << result.serialNumber;
    }


    emit discoveryFinished();
}
