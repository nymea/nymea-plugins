/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *                                                                         *
 *  Copyright (C) 2019 Simon Stürz <simon.stuerz@nymea.io>                 *
 *                                                                         *
 *  This file is part of nymea.                                            *
 *                                                                         *
 *  This library is free software; you can redistribute it and/or          *
 *  modify it under the terms of the GNU Lesser General Public             *
 *  License as published by the Free Software Foundation; either           *
 *  version 2.1 of the License, or (at your option) any later version.     *
 *                                                                         *
 *  This library is distributed in the hope that it will be useful,        *
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of         *
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU      *
 *  Lesser General Public License for more details.                        *
 *                                                                         *
 *  You should have received a copy of the GNU Lesser General Public       *
 *  License along with this library; If not, see                           *
 *  <http://www.gnu.org/licenses/>.                                        *
 *                                                                         *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

#include "honeywellscanner.h"
#include "extern-plugininfo.h"

#include <QDataStream>

HoneywellScanner::HoneywellScanner(const QString &serialPortName, qint32 baudrate, QObject *parent) :
    QObject(parent),
    m_serialPortName(serialPortName),
    m_baudrate(baudrate)
{
    m_reconnectTimer = new QTimer(this);
    m_reconnectTimer->setSingleShot(true);
    m_reconnectTimer->setInterval(2000);

    connect(m_reconnectTimer, &QTimer::timeout, this, &HoneywellScanner::onReconnectTimeout);
}

bool HoneywellScanner::connected() const
{
    return m_connected;
}

void HoneywellScanner::init()
{
    if (!m_serialPort->isOpen())
        return;

    qCDebug(dcHoneywellScanner()) << "Execute: Init";
//    m_asyncCommand = "PAP232;232BAD9,WRD2";
//    sendData(buildCommand(m_asyncCommand));
}

void HoneywellScanner::setConnected(bool connected)
{
    if (m_connected == connected)
        return;

    m_connected = connected;
    emit connectedChanged(m_connected);

    if (!m_connected) {
        m_reconnectTimer->start();
        m_asyncCommand.clear();
    } else {
        init();
    }
}

void HoneywellScanner::sendData(const QByteArray &data)
{
    qCDebug(dcHoneywellScanner()) << "-->" << data;
    m_serialPort->write(data);
}

QByteArray HoneywellScanner::buildCommand(const QString &command)
{
    return QByteArray("\x16m\r").append(command.toUtf8()).append(".");
}

void HoneywellScanner::onReconnectTimeout()
{
    if (m_serialPort && !m_serialPort->isOpen()) {
        if (!m_serialPort->open(QSerialPort::ReadWrite)) {
            setConnected(false);
            m_reconnectTimer->start();
        } else {
            qCDebug(dcHoneywellScanner()) << "The sensor is connected again" << m_serialPortName << m_baudrate;
            setConnected(true);
        }
    }
}

void HoneywellScanner::onReadyRead()
{
    QByteArray data = m_serialPort->readAll();
    qCDebug(dcHoneywellScanner()) << "<--" << data;

    if (m_asyncCommand.isEmpty()) {
        emit codeScanned(QString::fromUtf8(data).trimmed());
    } else {
        quint8 responseCode = 0;
        if (data.size() < 2) {
            responseCode = static_cast<quint8>(data.at(data.length() - 1));
        } else {
            responseCode = static_cast<quint8>(data.at(data.length() - 2));
        }

        // Note: https://en.cppreference.com/w/cpp/language/ascii
        QString responseCodeString;
        switch (responseCode) {
        case 0x06:
            responseCodeString = "ACK";
            break;
        case 0x05:
            responseCodeString = "ENQ";
            break;
        case 0x15:
            responseCodeString = "NAK";
            break;
        default:
            responseCodeString = "Unknown response code";
            break;
        }
        qCDebug(dcHoneywellScanner()) << "Received response for command" << m_asyncCommand << responseCodeString;
        m_asyncCommand.clear();
    }
}

void HoneywellScanner::onError(const QSerialPort::SerialPortError &error)
{
    if (error != QSerialPort::NoError && m_serialPort->isOpen()) {
        qCWarning(dcHoneywellScanner()) << "Serial port error:" << error << m_serialPort->errorString();
        m_reconnectTimer->start();
        m_serialPort->close();
        setConnected(false);
    }
}

void HoneywellScanner::enable()
{
    qCDebug(dcHoneywellScanner()) << "Enable scanner interface";
    if (m_serialPort) {
        delete m_serialPort;
        m_serialPort = nullptr;
    }

    setConnected(false);

    m_serialPort = new QSerialPort(m_serialPortName, this);
    m_serialPort->setBaudRate(m_baudrate);
    m_serialPort->setDataBits(QSerialPort::Data8);
    m_serialPort->setParity(QSerialPort::NoParity);
    m_serialPort->setFlowControl(QSerialPort::NoFlowControl);

    connect(m_serialPort, &QSerialPort::readyRead, this, &HoneywellScanner::onReadyRead);
    connect(m_serialPort, SIGNAL(error(QSerialPort::SerialPortError)), this, SLOT(onError(QSerialPort::SerialPortError)));

    if (!m_serialPort->open(QSerialPort::ReadWrite)) {
        qCWarning(dcHoneywellScanner()) << "Could not open serial port" << m_serialPortName << m_baudrate;
        m_reconnectTimer->start();
        return;
    }

    qCDebug(dcHoneywellScanner()) << "Interface enabled successfully on" << m_serialPortName << m_baudrate;
    setConnected(true);
}

void HoneywellScanner::disable()
{
    qCDebug(dcHoneywellScanner()) << "Disable scanner interface";
    if (!m_serialPort)
        return;

    if (m_serialPort->isOpen())
        m_serialPort->close();

    delete m_serialPort;
    m_serialPort = nullptr;
    setConnected(false);
    qCDebug(dcHoneywellScanner()) << "Scanner interface disabled";
}

bool HoneywellScanner::reset()
{
    qCDebug(dcHoneywellScanner()) << "Execute: Reset";
    if (!m_serialPort->isOpen())
        return false;

    m_asyncCommand = "RESET";
    sendData(QByteArray("\x16n,reset\r."));
    return true;
}

bool HoneywellScanner::configureDefaults()
{
    if (!m_serialPort->isOpen())
        return false;

    qCDebug(dcHoneywellScanner()) << "Execute: Defaults";
    m_asyncCommand = "DEFALT";
    sendData(buildCommand(m_asyncCommand));
    return true;
}

bool HoneywellScanner::configurePowerUpBeep(bool enabled)
{
    if (!m_serialPort->isOpen())
        return false;

    qCDebug(dcHoneywellScanner()) << "Execute: Power up beep" << (enabled ? "enabled": "disabled");
    if (enabled) {
        m_asyncCommand = "BEPPWR1";
        sendData(buildCommand(m_asyncCommand));
    } else {
        m_asyncCommand = "BEPPWR0";
        sendData(buildCommand(m_asyncCommand));
    }

    return true;
}

bool HoneywellScanner::configureLedPower(bool power)
{
    if (!m_serialPort->isOpen())
        return false;

    qCDebug(dcHoneywellScanner()) << "Execute: LED power" << (power ? "enabled": "disabled");
    if (power) {
        m_asyncCommand = "SCNLED1";
        sendData(buildCommand(m_asyncCommand));
    } else {
        m_asyncCommand = "SCNLED0";
        sendData(buildCommand(m_asyncCommand));
    }

    return true;
}

bool HoneywellScanner::configureTrigger(bool enabled)
{
    if (!m_serialPort->isOpen())
        return false;

    qCDebug(dcHoneywellScanner()) << "Execute:" << (enabled ? "Trigger": "Untrigger");
    if (enabled) {
        sendData(buildCommand(QByteArray("\x16t\r.")));
    } else {
        sendData(buildCommand(QByteArray("\x16u\r.")));
    }

    return true;
}

bool HoneywellScanner::configureMode(HoneywellScanner::Mode mode)
{
    if (!m_serialPort->isOpen())
        return false;

    switch (mode) {
    case ModeManual:
        m_asyncCommand = "TRGMOD0";
        sendData(buildCommand(m_asyncCommand));
        break;
    case ModePresentation:
        m_asyncCommand = "TRGMOD3";
        sendData(buildCommand(m_asyncCommand));
        break;
    }
    return true;
}
