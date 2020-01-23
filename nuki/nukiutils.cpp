/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*
* Copyright 2013 - 2020, nymea GmbH
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

#include "nukiutils.h"
#include "extern-plugininfo.h"

#include <QtEndian>
#include <QDataStream>

QString NukiUtils::convertByteToHexString(const quint8 &byte)
{
    QString hexString(QStringLiteral("0x%1"));
    hexString = hexString.arg(byte, 2, 16, QLatin1Char('0'));
    return hexString.toStdString().data();
}

QString NukiUtils::convertByteArrayToHexString(const QByteArray &byteArray)
{
    QString hexString;
    for (int i = 0; i < byteArray.count(); i++) {
        hexString.append(convertByteToHexString(static_cast<quint8>(byteArray.at(i))));
        if (i != byteArray.count() - 1) {
            hexString.append(" ");
        }
    }

    return hexString.toStdString().data();
}

QString NukiUtils::convertByteArrayToHexStringCompact(const QByteArray &byteArray)
{
    QString hexString;
    for (int i = 0; i < byteArray.length(); i++) {
        hexString.append(QString("%1").arg(static_cast<unsigned char>(byteArray.at(i)), 2, 16, QLatin1Char('0')));
    }

    return hexString;
}

QString NukiUtils::convertUint16ToHexString(const quint16 &value)
{
    QByteArray data;
    QDataStream stream(&data, QIODevice::WriteOnly);
    stream << value;

    return QString("0x%1").arg(convertByteArrayToHexString(data).remove(" ").remove("0x"));
}

QByteArray NukiUtils::converUint32ToByteArrayLittleEndian(const quint32 &value)
{
    QByteArray data;
    QDataStream stream(&data, QIODevice::WriteOnly);
    stream.setByteOrder(QDataStream::LittleEndian);
    stream << value;
    Q_ASSERT_X(data.length() == 4, "data converting", "Could not convert quint32 value to byte array (little endian)");
    return data;
}

QByteArray NukiUtils::converUint16ToByteArrayLittleEndian(const quint16 &value)
{
    QByteArray data;
    QDataStream stream(&data, QIODevice::WriteOnly);
    stream.setByteOrder(QDataStream::LittleEndian);
    stream << value;
    Q_ASSERT_X(data.length() == 2, "data converting", "Could not convert quint16 value to byte array (little endian)");
    return data;
}

quint16 NukiUtils::convertByteArrayToUint16BigEndian(const QByteArray &littleEndianByteArray)
{
    Q_ASSERT_X(littleEndianByteArray.length() == 2, "data converting", "Could not convert byte array (little endian) to quint16 value. Invalid size of byte array.");

    quint16 value = 0;
    QByteArray data(littleEndianByteArray);
    QDataStream stream(&data, QIODevice::ReadOnly);
    stream.setByteOrder(QDataStream::LittleEndian);
    stream >> value;
    return value;
}

quint32 NukiUtils::convertByteArrayToUint32BigEndian(const QByteArray &littleEndianByteArray)
{
    Q_ASSERT_X(littleEndianByteArray.length() == 4, "data converting", "Could not convert byte array (little endian) to quint32 value. Invalid size of byte array.");

    quint32 value = 0;
    QByteArray data(littleEndianByteArray);
    QDataStream stream(&data, QIODevice::ReadOnly);
    stream.setByteOrder(QDataStream::LittleEndian);
    stream >> value;
    return value;
}

quint16 NukiUtils::calculateCrc(const QByteArray &data)
{
    quint16 crcValue = 0xffff;
    quint16 polynom = 0x1021;

    for (int byte = 0; byte < data.length(); ++byte) {
        crcValue ^= (static_cast<quint8>(data.at(byte)) << 8);
        for (quint8 bit = 8; bit > 0; --bit) {
            if (crcValue & 0x8000) {
                crcValue = (crcValue << 1) ^ polynom;
            } else {
                crcValue = (crcValue << 1);
            }
        }
    }

    return crcValue;
}

bool NukiUtils::validateMessageCrc(const QByteArray &message)
{
    quint16 crcValue = 0;
    QByteArray crcValueRaw = message.right(2);
    QDataStream stream(&crcValueRaw, QIODevice::ReadOnly);
    stream.setByteOrder(QDataStream::LittleEndian);
    stream >> crcValue;

    QByteArray content = message.left(message.count() - 2);

    quint16 calculatedCrcValue = calculateCrc(content);
    if (crcValue != calculatedCrcValue) {
        qCWarning(dcNuki()) << "CRC CCITT validation failed:" << crcValue << "!=" << calculatedCrcValue;
        return false;
    }

    return true;
}

QByteArray NukiUtils::createRequestMessageForUnencrypted(NukiUtils::Command command, const QByteArray &payload)
{
    /* Note: build a message for unencrypted communication with paring service
     *      2 Bytes: command identifier (LittleEndian)
     *      n Bytes: pyload (raw bytes)
     *      2 Bytes: crc (LittleEndian)
     */
    QByteArray message;
    QDataStream stream(&message, QIODevice::WriteOnly);
    stream.setByteOrder(QDataStream::LittleEndian);
    stream << static_cast<quint16>(command);

    // Write payload
    for (int i = 0; i < payload.length(); i++) {
        stream << static_cast<quint8>(payload.at(i));
    }

    stream << NukiUtils::calculateCrc(message);
    return message;
}

QByteArray NukiUtils::createRequestMessageForUnencryptedForEncryption(quint32 authenticationId, NukiUtils::Command command, const QByteArray &payload)
{
    /* Note: build a message for encrypted communication with key turner service. This represents the unencrypted PDATA
     *      4 Bytes: authentication ID (LittleEndian)
     *      2 Bytes: command identifier (LittleEndian)
     *      n Bytes: pyxload (raw bytes)
     *      2 Bytes: crc (LittleEndian)
     */

    QByteArray message;
    QDataStream stream(&message, QIODevice::WriteOnly);
    stream.setByteOrder(QDataStream::LittleEndian);
    stream << authenticationId;
    stream << static_cast<quint16>(command);

    // Write payload
    for (int i = 0; i < payload.length(); i++) {
        stream << static_cast<quint8>(payload.at(i));
    }

    stream << NukiUtils::calculateCrc(message);
    return message;
}
