/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*
* Copyright 2013 - 2023, nymea GmbH
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

#include "evboxport.h"

#include <QDataStream>

#include "extern-plugininfo.h"


#include <QTimer>

#define STX 0x02
#define ETX 0x03



EVBoxPort::EVBoxPort(const QString &portName, QObject *parent)
    : QObject{parent}
{
    m_serialPort = new QSerialPort(portName, this);
    m_serialPort->setBaudRate(QSerialPort::Baud38400);
    m_serialPort->setDataBits(QSerialPort::Data8);
    m_serialPort->setStopBits(QSerialPort::OneStop);
    m_serialPort->setParity(QSerialPort::NoParity);

    connect(m_serialPort, &QSerialPort::readyRead, this, &EVBoxPort::onReadyRead);

    connect(m_serialPort, static_cast<void(QSerialPort::*)(QSerialPort::SerialPortError)>(&QSerialPort::error), this, [=](){
        qCWarning(dcEVBox()) << "Serial Port error" << m_serialPort->error() << m_serialPort->errorString();
        if (m_serialPort->error() != QSerialPort::NoError) {
            if (m_serialPort->isOpen()) {
                m_serialPort->close();
            }
            emit closed();

            QTimer::singleShot(1000, this, [=](){
                open();
            });
        }
    });

    // As per spec we need to wait at least 100ms after anything happens on the bus
    m_waitTimer.setSingleShot(true);
    m_waitTimer.setInterval(150);
    connect(&m_waitTimer, &QTimer::timeout, this, &EVBoxPort::processQueue);
}

bool EVBoxPort::open()
{
    if (m_serialPort->open(QSerialPort::ReadWrite)) {
        emit opened();
        return true;
    }
    return false;
}

bool EVBoxPort::isOpen()
{
    return m_serialPort->isOpen();
}

void EVBoxPort::close()
{
    m_serialPort->close();
}

void EVBoxPort::sendCommand(Command command, quint16 timeout, quint16 maxChargingCurrent, const QString &serial)
{
    CommandWrapper cmd;
    cmd.command = command;
    cmd.timeout = timeout;
    cmd.maxChargingCurrent = maxChargingCurrent;
    cmd.serial = serial;

    m_commandQueue.enqueue(cmd);

    processQueue();
}

void EVBoxPort::onReadyRead()
{
    m_waitTimer.start();

    m_inputBuffer.append(m_serialPort->readAll());

    QByteArray packet;
    QDataStream inputStream(m_inputBuffer);
    QDataStream outputStream(&packet, QIODevice::WriteOnly);
    bool startFound = false, endFound = false;

    while (!inputStream.atEnd()) {
        quint8 byte;
        inputStream >> byte;
        if (!startFound) {
            if (byte == STX) {
                startFound = true;
                continue;
            } else {
                qCWarning(dcEVBox()) << "Discarding byte not matching start of frame 0x" + QString::number(byte, 16);
                continue;
            }
        }

        if (byte == ETX) {
            endFound = true;
            break;
        }

        outputStream << byte;
    }

    if (startFound && endFound) {
        m_inputBuffer.remove(0, packet.length() + 2);
    } else {
        qCDebug(dcEVBox()) << "Data is incomplete... Waiting for more...";
        return;
    }

    if (packet.length() < 2) { // In practice it'll be longer, but let's make sure we won't crash checking the checksum on erraneous data
        qCWarning(dcEVBox()) << "Packet is too short. Discarding packet...";
        return;
    }

    qCDebug(dcEVBox()) << "<--" << packet;

    processDataPacket(packet);

}

void EVBoxPort::processDataPacket(const QByteArray &packet)
{
    // The data is a mess of hex and dec values... We'll read the stream as hex but have to convert some back to decimal.
    QDataStream stream(QByteArray::fromHex(packet));

    quint8 from, to, commandId, chargeBoxModuleCount;
    quint16 minPollInterval, maxChargingCurrent, minChargingCurrent, chargingCurrentL1, chargingCurrentL2, chargingCurrentL3, cosinePhiL1, cosinePhiL2, cosinePhiL3, voltageL1, voltageL2, voltageL3;
    quint32 totalEnergyConsumed;

    stream >> from >> to >> commandId;

    // Command is transmitted in decimal
    Command command = static_cast<Command>(QString::number(commandId, 16).toInt());


    QString serial;

    if (command == Command68) {
        quint32 serialNumber;
        stream >> serialNumber;
        serial = QString::number(serialNumber, 16);

        stream >> minPollInterval >> maxChargingCurrent >> chargeBoxModuleCount;

        if (chargeBoxModuleCount > 0) {
            stream >> minChargingCurrent
                    >> chargingCurrentL1
                    >> chargingCurrentL2
                    >> chargingCurrentL3
                    >> cosinePhiL1
                    >> cosinePhiL2
                    >> cosinePhiL3
                    >> totalEnergyConsumed
                    >> voltageL1
                    >> voltageL2
                    >> voltageL3;
        } else {
            qCDebug(dcEVBox()) << "No chargebox module data in packet!";
            emit shortResponseReceived(command, serial);
            return;
        }

    } else if (command == Command69) {

        stream >> minPollInterval >> maxChargingCurrent >> chargeBoxModuleCount;

        if (chargeBoxModuleCount > 0) {
            stream >> minChargingCurrent
                    >> chargingCurrentL1
                    >> chargingCurrentL2
                    >> chargingCurrentL3
                    >> cosinePhiL1
                    >> cosinePhiL2
                    >> cosinePhiL3
                    >> totalEnergyConsumed;
        } else {
            qCDebug(dcEVBox()) << "No chargebox module data in packet!";
            emit shortResponseReceived(command, serial);
            return;
        }
    } else {
        qCWarning(dcEVBox()) << "Unknown command id. Cannot process packet.";
        return;
    }

    qCDebug(dcEVBox()) << "Data packet received: From:" << from
                       << "To:" << to
                       << "Command:" << command
                       << "Serial:" << serial
                       << "MinAmpere:" << minChargingCurrent
                       << "MaxAmpere:" << maxChargingCurrent
                       << "AmpereL1" << chargingCurrentL1
                       << "AmpereL2" << chargingCurrentL2
                       << "AmpereL3" << chargingCurrentL3
                       << "Total" << totalEnergyConsumed;
    emit responseReceived(command, serial, minChargingCurrent, maxChargingCurrent, chargingCurrentL1, chargingCurrentL2, chargingCurrentL3, totalEnergyConsumed);

}

void EVBoxPort::processQueue()
{
    if (m_commandQueue.isEmpty()) {
        return;
    }
    if (m_waitTimer.isActive()) {
        qCDebug(dcEVBox()) << "Line is busy. Waiting...";
        return;
    }

    CommandWrapper cmd = m_commandQueue.takeFirst();

    QByteArray commandData;

    commandData += "80"; // Dst addr
    commandData += "A0"; // Sender address
    commandData += QString::number(cmd.command);

    qCDebug(dcEVBox()) << "Sending command" << cmd.command << "to" << cmd.serial << "MaxCurrent:" << cmd.maxChargingCurrent;

    if (cmd.command == Command68) {
        if (cmd.serial.length() != 8) {
            qCCritical(dcEVBox()) << "Serial must be 8 characters. Cannot send command...";
            processQueue();
            return;
        }
        commandData += cmd.serial;
        // The content of the “information module” is 16 bytes in size and not defined. ¯\_(ツ)_/¯
        commandData += "00112233445566778899AABBCCDDEEFF";

    } else if (cmd.command == Command69) {
        qCDebug(dcEVBox()) << "Using command 69";

    }

    commandData += QString("%1").arg(cmd.maxChargingCurrent * 10, 4, 10, QChar('0'));
    commandData += QString("%1").arg(cmd.maxChargingCurrent * 10, 4, 10, QChar('0'));
    commandData += QString("%1").arg(cmd.maxChargingCurrent * 10, 4, 10, QChar('0'));
    commandData += QString("%1").arg(cmd.timeout, 4, 10, QChar('0'));
    // If we fail to refresh the wallbox after the timeout, it shall turn off, which is what we'll use as default
    // when we don't know what its set to (as we can't read it).
    // Hence we do *not* cache the power and maxChargingCurrent states for this one
    commandData += QString("%1").arg(6, 4, 10, QChar('0'));
    commandData += QString("%1").arg(6, 4, 10, QChar('0'));
    commandData += QString("%1").arg(6, 4, 10, QChar('0'));

    commandData += createChecksum(commandData);

    QByteArray data;
    QDataStream stream(&data, QIODevice::WriteOnly);
    stream << static_cast<quint8>(STX);
    stream.writeRawData(commandData.data(), commandData.length());
    stream << static_cast<quint8>(ETX);

    qCDebug(dcEVBox()) << "-->" << data; // << "Hex:" << data.toHex();

    qint64 count = m_serialPort->write(data);
    if (count != data.length()) {
        qCWarning(dcEVBox()) << "Error writing data to serial port:" << m_serialPort->errorString();
    }

    m_waitTimer.start();
}

QByteArray EVBoxPort::createChecksum(const QByteArray &data) const
{
    QDataStream checksumStream(data);
    quint8 sum = 0;
    quint8 xOr = 0;
    while (!checksumStream.atEnd()) {
        quint8 byte;
        checksumStream >> byte;
        sum += byte;
        xOr ^= byte;
    }
    return QString("%1%2").arg(sum,2,16, QChar('0')).arg(xOr,2,16, QChar('0')).toUpper().toLocal8Bit();
}
