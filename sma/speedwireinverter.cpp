#include "speedwireinverter.h"
#include "extern-plugininfo.h"

#include <QDateTime>

SpeedwireInverter::SpeedwireInverter(const QHostAddress &address, quint16 modelId, quint32 serialNumber, QObject *parent) :
    QObject(parent),
    m_address(address),
    m_modelId(modelId),
    m_serialNumber(serialNumber)
{

    m_interface = new SpeedwireInterface(m_address, true, this);
    connect(m_interface, &SpeedwireInterface::dataReceived, this, &SpeedwireInverter::processData);

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

bool SpeedwireInverter::initialize()
{
    return m_interface->initialize();
}

bool SpeedwireInverter::initialized() const
{
    return m_interface->initialized();
}

void SpeedwireInverter::sendLoginRequest(const QString &password, bool loginAsUser)
{
    // Request  534d4100000402a000000001003a0010 60650ea0 7a01842a71b30001 7d0042be283a0001 000000000280 0c04fdff 07000000 84030000 00d8e85f 00000000 c1c1c1c18888888888888888 00000000   => login command = 0xfffd040c, first = 0x00000007 (user 7, installer a), last = 0x00000384 (hier timeout), time = 0x5fdf9ae8, 0x00000000, pw 12 bytes

    // Response 534d4100000402a000000001002e0010 60650be0 7d0042be283a0001 7a01842a71b30001 000000000280 0d04fdff 07000000 84030000 00d8e85f 00000000 00000000 => login OK
    // Response 534d4100000402a000000001002e0010 60650be0 7d0042be283a0001 7a01842a71b30001 000100000280 0d04fdff 07000000 84030000 fddbe85f 00000000 00000000 => login INVALID PASSWORD
    // command  0xfffd040c => 0x400 set?  0x00c bytecount=12?

    // Build the header
    QByteArray header = QByteArray::fromHex("534d4100000402a000000001003a001060650ea0");

    // The payload is little endian encoded
    QByteArray payload;
    QDataStream payloadStream(&payload, QIODevice::WriteOnly);
    payloadStream.setByteOrder(QDataStream::LittleEndian);

    // Destination
    payloadStream << m_modelId << m_serialNumber << static_cast<quint16>(0x0100);

    // Source
    payloadStream << m_interface->sourceModelId() << m_interface->sourceSerialNumber() << static_cast<quint16>(0x0100);

    // Packet information
    quint16 errorCode = 0;
    quint16 fragmentId = 0;
    quint16 packetId = m_packetId++ | 0x8000;
    payloadStream << errorCode << fragmentId << packetId;

    // Command
    payloadStream << static_cast<quint32>(CommandQueryLogin);

    // User type: 7 = user, a = installer
    payloadStream << (loginAsUser ? static_cast<quint32>(0x00000007) : static_cast<quint32>(0x0000000a));

    // Timeout
    payloadStream << static_cast<quint32>(900); // 1s

    // Current time
    payloadStream << static_cast<quint32>(QDateTime::currentMSecsSinceEpoch() / 1000.0);

    // Zeros
    payloadStream << static_cast<quint32>(0);

    // Password
    QByteArray passwordData = password.toUtf8();
    QByteArray encodedPassword(12, loginAsUser ? 0x88 : 0xBB);
    for (int i = 0; i < password.count(); i++) {
        encodedPassword[i] = passwordData.at(i) + (loginAsUser ? 0x88 : 0xBB);
    }

    for (int i = 0; i < encodedPassword.count(); i++) {
        payloadStream << static_cast<quint8>(encodedPassword.at(i));
    }

    // End of data
    payloadStream << static_cast<quint32>(0);

    QByteArray data = header + payload;
    qCDebug(dcSma()) << "Inverter: Send login request" << data.toHex();
    m_interface->sendData(data);

    // 534d4100000402a000000001003a001060650ea0 7a01 842a71b3 0001 7d00 42be283a 0001 000000000280 0c04fdff 07000000 84030000 00d8e85f 00000000 c1c1c1c18888888888888888 00000000   => login command = 0xfffd040c, first = 0x00000007 (user 7, installer a), last = 0x00000384 (hier timeout), time = 0x5fdf9ae8, 0x00000000, pw 12 bytes
    // 534d4100000402a000000001003a001060650ea0 9013 be52007d 0001 7d00 42be283a 0001 000000000280 0c04fdff 07000000 84030000 cc96b061 00000000 c1c1c1c18888888888888888 00000000
    // 534d4100000402a000000001003a001060650ea0 9013 be52007d 0001 7d00 42be283a 0001 000000000180 0c04fdff 07000000 84030000 ae9db061 00000000 c1c1c1c18888888888888888 00000000

}

void SpeedwireInverter::querySoftwareVersion()
{
    // Request  534d4100000402a00000000100260010 606509a0 7a01 842a71b30001 7d00 42be283a0001 000000000380 00020058 00348200 ff348200 00000000 =>  query software version
    // Response 534d4100000402a000000001004e0010 606513a0 7d00 42be283a00a1 7a01 842a71b30001 000000000380 01020058 0a000000 0a000000 01348200 2ae5e65f 00000000 00000000 feffffff feffffff 040a1003 040a1003 00000000 00000000 00000000  code = 0x00823401    3 (BCD).10 (BCD).10 (BIN) Typ R (Enum)

    // ========= header

    // == 534d4100000402a00000000100260010

    // 534d4100 : SMA\0 signature
    // 0004 : header length
    // 02a0 : Tag0 type
    // 0000 : Tag version
    // 0001 : Group
    // 0026 : payload length
    // 0010 : SMA Net 2 version

    // == 606509a0

    // 6065 : inverter protocol id
    // 09 : length of long words = payload length / 4
    // a0 : control ?



    // ========= payload (big endian)

    // == 7a01842a71b30001

    // 7a01 : destination model id
    // 842a71b3 : destination serial number
    // 0001 : destination control field

    // == 7d0042be283a0001

    // 7d00 : source model id
    // 42be283a: source serial number
    // 0001 : source control field

    // == 0000 0000 0380 00020058

    // 0000 : error code
    // 0000 : fragment id
    // 0380 : packet id

    // 00020058 : command id = CommandQueryDevice
    // 00348200 : first register
    // ff348200 : last register

    // 00000000 : end of data

    qCDebug(dcSma()) << "Inverter: Query software version...";

    // The payload is little endian encoded
    QByteArray payload;
    QDataStream payloadStream(&payload, QIODevice::WriteOnly);
    payloadStream.setByteOrder(QDataStream::LittleEndian);

    // Destination
    payloadStream << m_modelId << m_serialNumber << static_cast<quint16>(0x1000);

    // Source
    payloadStream << m_interface->sourceModelId() << m_interface->sourceSerialNumber() << static_cast<quint16>(0x1000);

    // Packet information
    quint16 errorCode = 0;
    quint16 fragmentId = 0;
    m_packetId += 1;
    quint16 packetId = m_packetId | 0x8000;
    payloadStream << errorCode << fragmentId << packetId;

    // Request information
    payloadStream << static_cast<quint32>(CommandQueryDevice);
    payloadStream << static_cast<quint32>(0x00823400); // Software version first
    payloadStream << static_cast<quint32>(0x008234ff); // Software version last

    // End of data
    payloadStream << static_cast<quint32>(0);

    // Build the header
    QByteArray header = QByteArray::fromHex("534d4100000402a00000000100260010606509a0");

    QByteArray data = header + payload;
    m_interface->sendData(data);
}

void SpeedwireInverter::queryDeviceType()
{
    // Request  534d4100000402a00000000100260010 606509a0 7a01842a71b30001 7d0042be283a0001 000000000480 00020058 001e8200 ff208200 00000000 =>  query device type
    // Response 534d4100000402a000000001009e0010 606527a0 7d0042be283a00a1 7a01842a71b30001 000000000480 01020058 01000000 03000000 011e8210 6f89e95f 534e3a20 33303130 35333831 31360000 00000000 00000000 00000000 00000000
    //                                                                                                                              011f8208 6f89e95f 411f0001 feffff00 00000000 00000000 00000000 00000000 00000000 00000000  => 1f41 solar inverter
    //                                                                                                                              01208208 6f89e95f 96240000 80240000 81240001 82240000 feffff00 00000000 00000000 00000000 00000000
}


void SpeedwireInverter::processData(const QByteArray &data)
{
    qCDebug(dcSma()) << "Inverter: data received" << data.toHex();

}
