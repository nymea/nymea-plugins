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

#include "nymealightserialinterface.h"
#include "extern-plugininfo.h"

#include <QDataStream>

NymeaLightSerialInterface::NymeaLightSerialInterface(const QString &name, QObject *parent) :
    NymeaLightInterface(parent),
    m_serialPortName(name)
{
    m_serialPort = new QSerialPort(m_serialPortName, this);
    m_serialPort->setBaudRate(115200);
    m_serialPort->setDataBits(QSerialPort::DataBits::Data8);
    m_serialPort->setParity(QSerialPort::Parity::NoParity);
    m_serialPort->setStopBits(QSerialPort::StopBits::OneStop);
    m_serialPort->setFlowControl(QSerialPort::FlowControl::NoFlowControl);

    connect(m_serialPort, &QSerialPort::readyRead, this, &NymeaLightSerialInterface::onReadyRead);
    connect(m_serialPort, SIGNAL(error(QSerialPort::SerialPortError)), this, SLOT(onSerialError(QSerialPort::SerialPortError)));

    m_reconnectTimer = new QTimer(this);
    m_reconnectTimer->setInterval(5000);
    m_reconnectTimer->setSingleShot(false);
    connect(m_reconnectTimer, &QTimer::timeout, this, [=](){
        if (m_serialPort->isOpen()) {
            m_reconnectTimer->stop();
            return;
        } else {
            if (open()) {
                m_reconnectTimer->stop();
            }
        }
    });
}

bool NymeaLightSerialInterface::open()
{
    bool serialPortFound = false;
    foreach(const QSerialPortInfo &serialPortInfo, QSerialPortInfo::availablePorts()) {
        if (serialPortInfo.systemLocation() == m_serialPortName) {
            serialPortFound = true;
            break;
        }
    }

    // Prevent repeating warnings...
    if (!serialPortFound) {
        if (!m_reconnectTimer->isActive()) {
            m_reconnectTimer->start();
        }
        return false;
    }

    // The serial port is available...lt's try to open it
    if (!m_serialPort->open(QIODevice::ReadWrite)) {
        qCWarning(dcWs2812fx()) << "Could not open serial port" << m_serialPort->portName() << m_serialPort->errorString();
        m_reconnectTimer->start();
        return false;
    }

    emit availableChanged(true);
    return true;
}

void NymeaLightSerialInterface::close()
{
    m_serialPort->close();
    emit availableChanged(false);
}

bool NymeaLightSerialInterface::available()
{
    return m_serialPort->isOpen();
}

void NymeaLightSerialInterface::sendData(const QByteArray &data)
{
    // Stream bytes using SLIP
    QByteArray message;
    QDataStream stream(&message, QIODevice::WriteOnly);
    stream << static_cast<quint8>(SlipProtocolEnd);

    for (int i = 0; i < data.length(); i++) {
        quint8 dataByte = data.at(i);
        switch (dataByte) {
        case SlipProtocolEnd:
            stream << static_cast<quint8>(SlipProtocolEsc);
            stream << static_cast<quint8>(SlipProtocolTransposedEnd);
            break;
        case SlipProtocolEsc:
            stream << static_cast<quint8>(SlipProtocolEsc);
            stream << static_cast<quint8>(SlipProtocolTransposedEsc);
            break;
        default:
            stream << dataByte;
            break;
        }
    }
    stream << static_cast<quint8>(SlipProtocolEnd);

    qCDebug(dcWs2812fx()) << "UART -->" << message.toHex();
    m_serialPort->write(message);
    m_serialPort->flush();
}

void NymeaLightSerialInterface::onReadyRead()
{
    QByteArray data = m_serialPort->readAll();
    for (int i = 0; i < data.length(); i++) {
        quint8 receivedByte = data.at(i);

        if (m_protocolEscaping) {
            switch (receivedByte) {
            case SlipProtocolTransposedEnd:
                m_buffer.append(static_cast<quint8>(SlipProtocolEnd));
                m_protocolEscaping = false;
                break;
            case SlipProtocolTransposedEsc:
                m_buffer.append(static_cast<quint8>(SlipProtocolEsc));
                m_protocolEscaping = false;
                break;
            default:
                // SLIP protocol violation...received escape, but it is not an escaped byte
                break;
            }
        }

        switch (receivedByte) {
        case SlipProtocolEnd:
            // We are done with this package, process it and reset the buffer
            if (!m_buffer.isEmpty() && m_buffer.length() >= 3) {
                qCDebug(dcWs2812fx()) << "UART <--" << m_buffer.toHex();
                emit dataReceived(m_buffer);
            }
            m_buffer.clear();
            m_protocolEscaping = false;
            break;
        case SlipProtocolEsc:
            // The next byte will be escaped, lets wait for it
            m_protocolEscaping = true;
            break;
        default:
            // Nothing special, just add to buffer
            m_buffer.append(receivedByte);
            break;
        }
    }
}

void NymeaLightSerialInterface::onSerialError(QSerialPort::SerialPortError error)
{
    if (error != QSerialPort::NoError && m_serialPort->isOpen()) {
        qCWarning(dcWs2812fx()) << "Serial port error:" << error << m_serialPort->errorString();
        m_reconnectTimer->start();
        m_serialPort->close();
        emit availableChanged(false);
    }
}
