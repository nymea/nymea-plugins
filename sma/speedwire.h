/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*
* Copyright 2013 - 2021, nymea GmbH
* Contact: contact@nymea.io
*
* This file is part of nymea.
* This project including source code and documentation is protected by
* copyright law, and remains the property of nymea GmbH. All rights, including
* reproduction, publication, editing and translation, are reserved. The use of
* this project is subject to the terms of a license agreement to be concluded
* with nymea GmbH in accordance with the terms of use of nymea GmbH, available
* under https://nymea.io/license
*
* GNU Lesser General Public License Usage
* Alternatively, this project may be redistributed and/or modified under the
* terms of the GNU Lesser General Public License as published by the Free
* Software Foundation; version 3. This project is distributed in the hope that
* it will be useful, but WITHOUT ANY WARRANTY; without even the implied
* warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
* Lesser General Public License for more details.
*
* You should have received a copy of the GNU Lesser General Public License
* along with this project. If not, see <https://www.gnu.org/licenses/>.
*
* For any further details and any questions please contact us under
* contact@nymea.io or see our FAQ/Licensing Information on
* https://nymea.io/license/faq
*
* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

#ifndef SPEEDWIRE_H
#define SPEEDWIRE_H

#include <QObject>
#include <QDebug>
#include <QDataStream>
#include <QHostAddress>
#include <QHash>

class Speedwire
{
    Q_GADGET
public:
    enum Command {
        CommandIdentify = 0x00000201,
        CommandQueryStatus = 0x51800200,
        CommandQueryAc = 0x51000200,
        CommandQueryDc = 0x53800200,
        CommandQueryEnergy = 0x54000200,
        CommandQueryDevice = 0x58000200,
        CommandQueryDeviceResponse = 58000201,
        CommandLogin = 0xfffd040c,
        CommandLogout = 0xfffd010e,
        CommandLoginResponse = 0x0ffdf40d
    };
    Q_ENUM(Command)

    enum ProtocolId {
        ProtocolIdUnknown = 0x0000,
        ProtocolIdMeter = 0x6069,
        ProtocolIdInverter = 0x6065,
        ProtocolIdDiscoveryResponse = 0x0001,
        ProtocolIdDiscovery = 0xffff
    };
    Q_ENUM(ProtocolId)

    enum DeviceClass {
        DeviceClassUnknown = 0x0000,
        DeviceClassAllDevices = 0x1f40,
        DeviceClassSolarInverter = 0x1f41,
        DeviceClassWindTurbine = 0x1f42,
        DeviceClassBatteryInverter = 0x1f47,
        DeviceClassConsumer = 0x1f61,
        DeviceClassSensorSystem = 0x1f80,
        DeviceClassElectricityMeter = 0x1f81,
        DeviceClassCommunicationProduct = 0x1fc0
    };
    Q_ENUM(DeviceClass)

    class Header
    {
    public:
        Header() = default;
        quint32 smaSignature = 0;
        quint16 headerLength = 0;
        quint16 tagType = 0;
        quint16 tagVersion = 0;
        quint16 group = 0;
        quint16 payloadLength = 0;
        quint16 smaNet2Version = 0;
        ProtocolId protocolId = ProtocolIdUnknown;

        inline bool isValid() const {
            return smaSignature == Speedwire::smaSignature() && protocolId != ProtocolIdUnknown;
        }
    };

    typedef struct InverterPackage {
        quint8 wordCount = 0;
        quint8 control = 0;
        quint16 destinationModelId = 0;
        quint32 destinationSerialNumber = 0;
        quint16 destinationControl = 0;
        quint16 sourceModelId = 0;
        quint32 sourceSerialNumber = 0;
        quint16 sourceControl = 0;
        quint16 errorCode = 0;
        quint16 fragmentId = 0;
        quint16 packetId = 0;
        quint32 command = 0;
    } InverterPackage;

    Speedwire() = default;

    //static QHash<quint16, QString> deviceTypes = { {0x0000, "Unknwon"} };

    static quint16 port() { return  9522; }
    static QHostAddress multicastAddress() { return QHostAddress("239.12.255.254"); }
    static quint32 smaSignature() { return  0x534d4100; }
    static quint16 tag0() { return 0x02a0; }
    static quint16 tagVersion() { return 0; }
    static quint16 smaNet2Version() { return 0x0010; }

    static QString getModelName(quint16 modelIdentifier) {
        switch (modelIdentifier) {
        case 9015: return "SB 700";
        case 9016: return "SB 700U";
        case 9017: return "SB 1100";
        case 9018: return "SB 1100U";
        case 9019: return "SB 1100LV";
        case 9020: return "SB 1700";
        case 9021: return "SB 1900TLJ";
        case 9022: return "SB 2100TL";
        case 9023: return "SB 2500";
        case 9024: return "SB 2800";
        case 9025: return "SB 2800i";
        case 9026: return "SB 3000";
        case 9027: return "SB 3000US";
        case 9028: return "SB 3300";
        case 9029: return "SB 3300U";
        case 9030: return "SB 3300TL";
        case 9031: return "SB 3300TL HC";
        case 9032: return "SB 3800";
        case 9033: return "SB 3800U";
        case 9034: return "SB 4000US";
        case 9035: return "SB 4200TL";
        case 9036: return "SB 4200TL HC";
        case 9037: return "SB 5000TL";
        case 9038: return "SB 5000TLW";
        case 9039: return "SB 5000TL HC";
        case 9066: return "SB 1200";
        case 9067: return "STP 10000TL-10";
        case 9068: return "STP 12000TL-10";
        case 9069: return "STP 15000TL-10";
        case 9070: return "STP 17000TL-10";
        case 9084: return "WB 3600TL-20";
        case 9085: return "WB 5000TL-20";
        case 9086: return "SB 3800US-10";
        case 9098: return "STP 5000TL-20";
        case 9099: return "STP 6000TL-20";
        case 9100: return "STP 7000TL-20";
        case 9101: return "STP 8000TL-10";
        case 9102: return "STPcase 9000TL-20";
        case 9103: return "STP 8000TL-20";
        case 9104: return "SB 3000TL-JP-21";
        case 9105: return "SB 3500TL-JP-21";
        case 9106: return "SB 4000TL-JP-21";
        case 9107: return "SB 4500TL-JP-21";
        case 9108: return "SCSMC";
        case 9109: return "SB 1600TL-10";
        case 9131: return "STP 20000TL-10";
        case 9139: return "STP 20000TLHE-10";
        case 9140: return "STP 15000TLHE-10";
        case 9157: return "Sunny Island 2012";
        case 9158: return "Sunny Island 2224";
        case 9159: return "Sunny Island 5048";
        case 9160: return "SB 3600TL-20";
        case 9168: return "SC630HE-11";
        case 9169: return "SC500HE-11";
        case 9170: return "SC400HE-11";
        case 9171: return "WB 3000TL-21";
        case 9172: return "WB 3600TL-21";
        case 9173: return "WB 4000TL-21";
        case 9174: return "WB 5000TL-21";
        case 9175: return "SC 250";
        case 9176: return "SMA Meteo Station";
        case 9177: return "SB 240-10";
        case 9179: return "Multigate-10";
        case 9180: return "Multigate-US-10";
        case 9181: return "STP 20000TLEE-10";
        case 9182: return "STP 15000TLEE-10";
        case 9183: return "SB 2000TLST-21";
        case 9184: return "SB 2500TLST-21";
        case 9185: return "SB 3000TLST-21";
        case 9186: return "WB 2000TLST-21";
        case 9187: return "WB 2500TLST-21";
        case 9188: return "WB 3000TLST-21";
        case 9189: return "WTP 5000TL-20";
        case 9190: return "WTP 6000TL-20";
        case 9191: return "WTP 7000TL-20";
        case 9192: return "WTP 8000TL-20";
        case 9193: return "WTPcase 9000TL-20";
        case 9254: return "Sunny Island 3324";
        case 9255: return "Sunny Island 4.0M";
        case 9256: return "Sunny Island 4248";
        case 9257: return "Sunny Island 4248U";
        case 9258: return "Sunny Island 4500";
        case 9259: return "Sunny Island 4548U";
        case 9260: return "Sunny Island 5.4M";
        case 9261: return "Sunny Island 5048U";
        case 9262: return "Sunny Island 6048U";
        case 9278: return "Sunny Island 3.0M";
        case 9279: return "Sunny Island 4.4M";
        case 9281: return "STP 10000TL-20";
        case 9282: return "STP 11000TL-20";
        case 9283: return "STP 12000TL-20";
        case 9284: return "STP 20000TL-30";
        case 9285: return "STP 25000TL-30";
        case 9301: return "SB1.5-1VL-40";
        case 9302: return "SB2.5-1VL-40";
        case 9303: return "SB2.0-1VL-40";
        case 9304: return "SB5.0-1SP-US-40";
        case 9305: return "SB6.0-1SP-US-40";
        case 9306: return "SB8.0-1SP-US-40";
        case 9307: return "Energy Meter";
        default: return "Unknown";
        }
    };

    //  Multicast device discovery request packet, according to SMA documentation.
    //  However, this does not seem to be supported anymore with version 3.x devices
    //        0x53, 0x4d, 0x41, 0x00, 0x00, 0x04, 0x02, 0xa0,     // sma signature, tag0
    //        0xff, 0xff, 0xff, 0xff, 0x00, 0x00, 0x00, 0x20,     // 0xffffffff group, 0x0000 length, 0x0020 "SMA Net ?", Version ?
    //        0x00, 0x00, 0x00, 0x00                              // 0x0000 protocol, 0x00 #long words, 0x00 ctrl

    // Unicast device discovery request packet, according to SMA documentation
    //        0x53, 0x4d, 0x41, 0x00, 0x00, 0x04, 0x02, 0xa0,     // sma signature, tag0
    //        0x00, 0x00, 0x00, 0x01, 0x00, 0x26, 0x00, 0x10,     // 0x26 length, 0x0010 "SMA Net 2", Version 0
    //        0x60, 0x65, 0x09, 0xa0, 0xff, 0xff, 0xff, 0xff,     // 0x6065 protocol, 0x09 #long words, 0xa0 ctrl, 0xffff dst susyID any, 0xffffffff dst serial any
    //        0xff, 0xff, 0x00, 0x00, 0x7d, 0x00, 0x52, 0xbe,     // 0x0000 dst cntrl, 0x007d src susy id, 0x3a28be52 src serial
    //        0x28, 0x3a, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,     // 0x0000 src cntrl, 0x0000 error code, 0x0000 fragment ID
    //        0x01, 0x80, 0x00, 0x02, 0x00, 0x00, 0x00, 0x00,     // 0x8001 packet ID
    //        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    //        0x00, 0x00

    static QByteArray discoveryDatagramMulticast() { return QByteArray::fromHex("534d4100000402a0ffffffff0000002000000000"); }
    static QByteArray discoveryResponseDatagram() { return QByteArray::fromHex("534d4100000402A000000001000200000001"); }
    static QByteArray discoveryDatagramUnicast() { return QByteArray::fromHex("534d4100000402a00000000100260010606509a0ffffffffffff00007d0052be283a000000000000018000020000000000000000000000000000"); }

    static Speedwire::Header parseHeader(QDataStream &stream) {
        stream.setByteOrder(QDataStream::BigEndian);
        Header header;
        quint16 protocolId;
        stream >> header.smaSignature >> header.headerLength;
        stream >> header.tagType >> header.tagVersion >> header.group;
        stream >> header.payloadLength >> header.smaNet2Version;
        stream >> protocolId;
        header.protocolId = static_cast<ProtocolId>(protocolId);
        return header;
    };

    static Speedwire::InverterPackage parseInverterPackage(QDataStream &stream) {
        // Make sure the data stream is little endian
        stream.setByteOrder(QDataStream::LittleEndian);
        InverterPackage package;
        stream >> package.wordCount;
        stream >> package.control;
        stream >> package.destinationModelId;
        stream >> package.destinationSerialNumber;
        stream >> package.destinationControl;
        stream >> package.sourceModelId;
        stream >> package.sourceSerialNumber;
        stream >> package.sourceControl;
        stream >> package.errorCode;
        stream >> package.fragmentId;
        stream >> package.packetId;
        stream >> package.command;
        return package;
    };
};

inline QDebug operator<<(QDebug debug, const Speedwire::Header &header)
{
    debug.nospace() << "SpeedwireHeader(" << header.protocolId << ", payload size: " << header.payloadLength << ", group: " << header.payloadLength << ")";
    return debug.maybeSpace();
}

inline QDebug operator<<(QDebug debug, const Speedwire::InverterPackage &package)
{
    debug.nospace() << "InverterPackage(" << package.sourceSerialNumber;
    debug.nospace() << ", Model ID: " << package.sourceModelId;
    debug.nospace() << ", command: " << package.command;
    debug.nospace() << ", error: " << package.errorCode;
    debug.nospace() << ", fragment: " << package.fragmentId;
    debug.nospace() << ", package ID: " << package.fragmentId;
    debug.nospace()  << ")";
    return debug.maybeSpace();
}


#endif // SPEEDWIRE_H
