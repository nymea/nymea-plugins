#include "solarmanmodbus.h"
#include "solarmanmodbusreply.h"

#include <QDataStream>
#include <QHostAddress>
#include <QTimer>
#include <QLoggingCategory>
#include <QDateTime>
#include <QTimeZone>


Q_DECLARE_LOGGING_CATEGORY(dcSolarman)

#define MOCK_DATA 0

quint8 START_OF_MESSAGE = 0xA5;
quint8 END_OF_MESSAGE = 0x15;
quint8 FRAME_TYPE = 0x02;
quint16 requestCode = 0x4510;
quint16 responseCode = 0x1510;

SolarmanModbus::SolarmanModbus(QObject *parent)
    : QObject{parent}
{
    m_socket = new QTcpSocket(this);
    connect(m_socket, &QTcpSocket::stateChanged, this, [=](QAbstractSocket::SocketState state){
        qCDebug(dcSolarman()) << "Socket state changed:" << state;
        if (state == QAbstractSocket::ConnectedState) {
            emit connectedChanged(true);
        } else if (state == QAbstractSocket::UnconnectedState){
            emit connectedChanged(false);
        }
    }, Qt::QueuedConnection); // Otherwise socket->isOpen() may stll be true if the user reconnects right away.

    connect(m_socket, &QTcpSocket::readyRead, this, [this](){
        QByteArray data = m_socket->readAll();
        processData(data);
    });
}

void SolarmanModbus::connectToHost(const QHostAddress &host, quint16 port, const QString &serial)
{
    if (m_serial != serial || m_host != host || m_port != port) {
        m_serial = serial;
        m_host = host;
        m_port = port;
        m_socket->close();
    }

    if (!m_socket->isOpen()) {
        qCDebug(dcSolarman()) << "Connecting to" << host.toString() << port;
        m_socket->connectToHost(host, port);
    } else {
        qCDebug(dcSolarman()) << "Still connected. Not reconnecting";
    }
}

void SolarmanModbus::disconnectFromHost()
{
    m_serial.clear();
    m_socket->disconnectFromHost();
    emit connectedChanged(false);
}

bool SolarmanModbus::isConnected() const
{
    return m_socket->isOpen();
}

SolarmanModbusReply *SolarmanModbus::readRegisters(int slaveId, quint16 startRegister, quint16 endRegister, FunctionCode functionCode)
{

#if MOCK_DATA == 1
    quint8 requestId = 0;
    SolarmanModbusReply *reply = new SolarmanModbusReply(requestId, slaveId, startRegister, endRegister, functionCode, this);

    QByteArray mockData = QByteArray::fromHex(
                // Late afternoon
                "a5f30010150033fdc5fff40201aaab35004b0600001e2dba620103e0010002013232303230393036353500010000120c070000000112020700000bb800000101004b0000003c160807111210000000000abe07081450128e000000000000139c002c000000000000000000640000000000010000000000010000000000010000000000000000000000000000000000000004001100000000042a00000011000000000000042a0000000000000938000000000000000000001388000000000000000000000000042e00000000000014d20000000000000000000000000000000000000000000000000000000000000000000000000174001c0000000000356b15"
                // End of day
//                "a5f3001015002bfdc5fff402011a2900005a0600006adcef620103e0010002013232303230393036353500010000120c070000000112020700000bb800000101004b0000003c160807141d08000000000abe07081450128e000000000000139c002c000000000000000000640000000000010000000000010000000000010000000000000000000000000000000000000004001300000000042c00000013000000000000042c0000000000000924000000000000000000001388000000000000000000000000001e000000000000106800000000000000000000000000000000000000000000000000000000000000000000000001460000000000008ce6c615"
                );
    m_pendingReplies.append(reply);
    QMetaObject::invokeMethod(this, "processData", Qt::QueuedConnection, Q_ARG(QByteArray, mockData));
#else

    quint8 requestId = m_requestIdCounter++;
    SolarmanModbusReply *reply = new SolarmanModbusReply(requestId, slaveId, startRegister, endRegister, functionCode, this);

    if (!m_socket->isOpen()) {
        reply->finish(false);
        return reply;
    }
    m_pendingReplies.append(reply);
    connect(reply, &SolarmanModbusReply::finished, this, [this, reply](){
        m_pendingReplies.removeAll(reply);
    });
    QByteArray request = createRequest(requestId, slaveId, startRegister, endRegister, functionCode, m_serial);
    qCDebug(dcSolarman) << "Requesting:" << request.toHex();
    m_socket->write(request);
#endif

    return reply;
}

void SolarmanModbus::processData(const QByteArray &data)
{
    qCDebug(dcSolarman) << "Data received:" << data.toHex();

    if (!validateChecksum(data)) {
        qCWarning(dcSolarman()) << "Checksum verification failed for payload:" << data.toHex();
        return;
    }

    QDataStream stream(data);
    stream.setByteOrder(QDataStream::LittleEndian);
    quint8 startOfMessage;
    stream >> startOfMessage;
    if (startOfMessage != START_OF_MESSAGE) {
        qCWarning(dcSolarman()) << "Skipping message... No Start of message found.";
        return;
    }
    quint16 payloadLength;
    stream >> payloadLength;
    qCDebug(dcSolarman()) << "Payload length:" << payloadLength;
    quint16 commandCode;
    stream >> commandCode;

    quint8 requestId;
    stream >> requestId;
    quint8 packetCounter;
    stream >> packetCounter; // Probably for deduplication, dunno, just counts up to 0x6a (100) and resets to 1...


    if (commandCode != responseCode) {
        qCWarning(dcSolarman()) << "Unknown packet received:" << data.toHex();
        disconnectFromHost();
        return;
        QByteArray rsp;
        QDataStream rs(&rsp, QIODevice::WriteOnly);
        stream.setByteOrder(QDataStream::LittleEndian);
        rs << static_cast<quint8>(START_OF_MESSAGE);
        rs << static_cast<quint16>(1); // len
        rs << static_cast<quint16>(0x1710);
        rs << requestId;
        rs << packetCounter;
        QByteArray serialHex = getSerialHex(m_serial);
        qDebug() << "Serial hex:" << serialHex.toHex();
        rs.writeRawData(serialHex.data(), serialHex.length());
        rs << static_cast<quint8>(0);
        rs << createChecksum(rsp);
        rs << static_cast<quint8>(END_OF_MESSAGE);
        m_socket->write(rsp);
        return;
    }


    char serialStr[4];
    stream.readRawData(serialStr, 4);
    QByteArray serial(serialStr, 4);

    if (serial != getSerialHex(m_serial)) {
        qCWarning(dcSolarman()) << "Serial number does not match:" << serial.toHex() << getSerialHex(m_serial).toHex();
        return;
    }

    // Skipping 10 unknown characters
    quint8 frameType;
    stream >> frameType;

    quint8 sensorType;
    stream >> sensorType;

    quint32 deliveryTime;
    stream >> deliveryTime;

    quint32 powerOnTime;
    stream >> powerOnTime;
//    char unknownStr[10];
//    stream.readRawData(unknownStr, 10);

    quint32 timestamp;
    stream >> timestamp;
    qCDebug(dcSolarman()) << "Device time:" << QDateTime::fromMSecsSinceEpoch((qulonglong)timestamp * 1000, QTimeZone::utc());

    quint8 slaveId;
    stream >> slaveId;

    quint8 rt;
    stream >> rt;
    FunctionCode functionCode = static_cast<FunctionCode>(rt);

    quint8 registersLength;
    stream >> registersLength;

    char dataStr[registersLength];
    stream.readRawData(dataStr, registersLength);
    QByteArray registers(dataStr, registersLength);

    quint16 modbusCRC;
    stream >> modbusCRC;
    // While the checksum on the outer package is quite crappy (I've seen it colliding easily), we still don't bother checking the modbus CRC if that matches...

    qCDebug(dcSolarman()) << "Reply received: SlaveID" << slaveId << functionCode << "payload len" << registersLength << "poweruptime" << powerOnTime << "deliverytime" << deliveryTime << "frametype" << frameType;

    foreach (SolarmanModbusReply *reply, m_pendingReplies) {
        if (reply->requestId() == requestId) {
            reply->finish(true, registers);
        }
    }
}

QByteArray SolarmanModbus::createRequest(quint8 requestId, quint8 slaveId, quint16 startRegister, quint16 endRegister, FunctionCode functionCode, const QString &serialNumber) {


    QByteArray packet;
    QDataStream stream(&packet, QIODevice::WriteOnly);
    stream.setByteOrder(QDataStream::LittleEndian);
    stream << START_OF_MESSAGE;    
    QByteArray modbusPayload = createModbusPayload(slaveId, startRegister, endRegister, functionCode);
    stream << static_cast<quint16>(15 + modbusPayload.length());
//    stream.writeRawData((char*)CONTROL_CODE, 2);
    stream << requestCode;
    stream << requestId;
    stream << requestId;//static_cast<quint8>(0x00); // This seems to be a static counter counting up unrelated on both ends, but the inverter doesn't care if we don't use it.
    QByteArray serialHex = getSerialHex(serialNumber);
    qDebug() << "Serial hex:" << serialHex.toHex();
    stream.writeRawData(serialHex.data(), serialHex.length());
    stream << FRAME_TYPE;
    stream << static_cast<quint16>(0x0000); // Sensor type
    stream << static_cast<quint32>(0x00000000); // Deliverytime
    stream << static_cast<quint32>(0x00000000); // PowerOnTime
    stream << static_cast<quint32>(QDateTime::currentDateTimeUtc().toMSecsSinceEpoch() / 1000);
    stream.writeRawData(modbusPayload.data(), modbusPayload.length());
    stream << createChecksum(packet);
    stream << END_OF_MESSAGE;
    return packet;
}

QByteArray SolarmanModbus::createModbusPayload(quint8 slaveId, quint16 startRegister, quint16 endRegister, FunctionCode functionCode)
{
    quint16 registerCount = endRegister - startRegister + 1;

    QByteArray data;
    QDataStream stream(&data, QIODevice::WriteOnly);
    stream << static_cast<quint8>(slaveId);
    stream << static_cast<quint8>(functionCode);
    stream << startRegister;
    stream << registerCount;
    quint16 crc = createModbusCRC(data);
    stream.setByteOrder(QDataStream::LittleEndian);
//    stream.setByteOrder(QDataStream::BigEndian);
    stream << crc;
    return data;
}

quint16 SolarmanModbus::createModbusCRC(const QByteArray &data)
{
    quint16 poly = 0xA001;
    QDataStream stream(data);
    quint16 crc = 0xFFFF;

    while (!stream.atEnd()) {
        quint8 byte;
        stream >> byte;
        crc ^= byte;
        for (int i = 0; i < 8; i++) {
            if (crc & 0x0001) {
                crc = (crc >> 1) ^ poly;
            } else {
                crc = crc >> 1;
            }
        }
    }
    return crc;
}

QByteArray SolarmanModbus::getSerialHex(const QString &serialNumber) {
    QByteArray serialHex = QByteArray::fromHex(QByteArray::number(serialNumber.toLongLong(), 16));
    QByteArray reversed;
    reversed.reserve(serialHex.size());
    for (int i = serialHex.size() - 1; i >= 0; i--) {
        reversed.append(serialHex.at(i));
    }
    return reversed;
}

quint8 SolarmanModbus::createChecksum(const QByteArray &data)
{
    quint16 checksum = 0;
    for (int i = 1; i < data.length(); i++) {
        checksum += (quint8)data.at(i);
    }
    return checksum;
}

bool SolarmanModbus::validateChecksum(const QByteArray &packet) {
    quint16 checksum = 0;
    // Don't include the checksum and END OF MESSAGE (-2)
    for (int i = 1; i < packet.length() - 2; i++) {
        checksum += packet[i];
    }
    quint8 final = static_cast<quint8>(checksum);
    return final == (quint8)packet.at(packet.length() - 2);
}
