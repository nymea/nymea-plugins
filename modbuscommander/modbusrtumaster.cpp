/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *                                                                         *
 *  Copyright (C) 2019 Bernhard Trinnes <bernhard.trinnes@nymea.io>        *
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

#include "modbusrtumaster.h"
#include "extern-plugininfo.h"

#include <QSerialPortInfo>

ModbusRTUMaster::ModbusRTUMaster(QString serialPort, int baudrate, QSerialPort::Parity parity, int dataBits, int stopBits, QObject *parent) :
    QObject(parent)
{
    m_modbusRtuSerialMaster = new QModbusRtuSerialMaster(this);
    m_modbusRtuSerialMaster->setConnectionParameter(QModbusDevice::SerialPortNameParameter, serialPort);
    m_modbusRtuSerialMaster->setConnectionParameter(QModbusDevice::SerialBaudRateParameter, baudrate);
    m_modbusRtuSerialMaster->setConnectionParameter(QModbusDevice::SerialDataBitsParameter, dataBits);
    m_modbusRtuSerialMaster->setConnectionParameter(QModbusDevice::SerialStopBitsParameter, stopBits);
    m_modbusRtuSerialMaster->setConnectionParameter(QModbusDevice::SerialParityParameter, parity);
    //m_modbusRtuSerialMaster->setTimeout(100);
    //m_modbusRtuSerialMaster->setNumberOfRetries(1);
    connect(m_modbusRtuSerialMaster, &QModbusTcpClient::stateChanged, this, &ModbusRTUMaster::onModbusStateChanged);
    connect(m_modbusRtuSerialMaster, &QModbusRtuSerialMaster::errorOccurred, this, &ModbusRTUMaster::onModbusErrorOccurred);

    m_reconnectTimer = new QTimer(this);
    m_reconnectTimer->setSingleShot(true);
    connect(m_reconnectTimer, &QTimer::timeout, this, &ModbusRTUMaster::onReconnectTimer);
}


ModbusRTUMaster::~ModbusRTUMaster()
{
    if (!m_modbusRtuSerialMaster) {
        m_modbusRtuSerialMaster->disconnectDevice();
        m_modbusRtuSerialMaster->deleteLater();
    }
    if (!m_reconnectTimer) {
        m_reconnectTimer->stop();
        m_reconnectTimer->deleteLater();
    }
}

bool ModbusRTUMaster::connectDevice()
{
    qDebug(dcModbusCommander()) << "Setting up TCP connecion";

    if (!m_modbusRtuSerialMaster)
        return false;

    return m_modbusRtuSerialMaster->connectDevice();
}

QString ModbusRTUMaster::serialPort()
{
    return m_modbusRtuSerialMaster->connectionParameter(QModbusDevice::SerialPortNameParameter).toString();
}

void ModbusRTUMaster::onReplyFinished()
{
    QModbusReply *reply = qobject_cast<QModbusReply *>(sender());
    if (!reply)
        return;
    int modbusAddress = 0;

    if (reply->error() == QModbusDevice::NoError) {
        const QModbusDataUnit unit = reply->result();

        for (int i = 0; i < static_cast<int>(unit.valueCount()); i++) {
            //qCDebug(dcUniPi()) << "Start Address:" << unit.startAddress() << "Register Type:" << unit.registerType() << "Value:" << unit.value(i);
            modbusAddress = unit.startAddress() + i;

            switch (unit.registerType()) {
            case QModbusDataUnit::RegisterType::Coils:
                emit receivedCoil(reply->serverAddress(), modbusAddress, unit.value(i));
                break;
            case QModbusDataUnit::RegisterType::DiscreteInputs:
                emit receivedDiscreteInput(reply->serverAddress(), modbusAddress, unit.value(i));
                break;
            case QModbusDataUnit::RegisterType::InputRegisters:
                emit receivedInputRegister(reply->serverAddress(), modbusAddress, unit.value(i));
                break;
            case QModbusDataUnit::RegisterType::HoldingRegisters:
                emit receivedHoldingRegister(reply->serverAddress(), modbusAddress, unit.value(i));
                break;
            case QModbusDataUnit::RegisterType::Invalid:
                break;
            }
        }

    } else if (reply->error() == QModbusDevice::ProtocolError) {
        qCWarning(dcModbusCommander()) << "Read response error:" << reply->errorString() << reply->rawResult().exceptionCode();
    } else {
        qCWarning(dcModbusCommander()) << "Read response error:" << reply->error();
    }
    reply->deleteLater();
}

void ModbusRTUMaster::onReplyErrorOccured(QModbusDevice::Error error)
{
    qCWarning(dcModbusCommander()) << "Modbus replay error:" << error;
    QModbusReply *reply = qobject_cast<QModbusReply *>(sender());
    if (!reply)
        return;
    reply->finished(); //to make sure it will be deleted
}

void ModbusRTUMaster::onReconnectTimer()
{
    if(!m_modbusRtuSerialMaster->connectDevice()) {
        m_reconnectTimer->start(10000);
    }
}

bool ModbusRTUMaster::readCoil(int slaveAddress, int registerAddress)
{
    if (!m_modbusRtuSerialMaster)
        return false;

    QModbusDataUnit request = QModbusDataUnit(QModbusDataUnit::RegisterType::Coils, registerAddress, 1);

    if (QModbusReply *reply = m_modbusRtuSerialMaster->sendReadRequest(request, slaveAddress)) {
        if (!reply->isFinished()) {
            connect(reply, &QModbusReply::finished, this, &ModbusRTUMaster::onReplyFinished);
            connect(reply, &QModbusReply::errorOccurred, this, &ModbusRTUMaster::onReplyErrorOccured);
            QTimer::singleShot(200, reply, SLOT(deleteLater()));
        } else {
            delete reply; // broadcast replies return immediately
        }
    } else {
        qCWarning(dcModbusCommander()) << "Read error: " << m_modbusRtuSerialMaster->errorString();
    }
    return true;
}

bool ModbusRTUMaster::writeCoil(int slaveAddress, int registerAddress, bool value)
{
    if (!m_modbusRtuSerialMaster)
        return false;

    QModbusDataUnit request = QModbusDataUnit(QModbusDataUnit::RegisterType::Coils, registerAddress, 1);
    request.setValue(0, static_cast<uint16_t>(value));

    if (QModbusReply *reply = m_modbusRtuSerialMaster->sendWriteRequest(request, slaveAddress)) {
        if (!reply->isFinished()) {
            connect(reply, &QModbusReply::finished, this, &ModbusRTUMaster::onReplyFinished);
            connect(reply, &QModbusReply::errorOccurred, this, &ModbusRTUMaster::onReplyErrorOccured);
            QTimer::singleShot(200, reply, SLOT(deleteLater()));
        } else {
            delete reply; // broadcast replies return immediately
        }
    } else {
        qCWarning(dcModbusCommander()) << "Read error: " << m_modbusRtuSerialMaster->errorString();
    }
    return true;
}

bool ModbusRTUMaster::writeHoldingRegister(int slaveAddress, int registerAddress, int value)
{
    if (!m_modbusRtuSerialMaster)
        return false;

    QModbusDataUnit request = QModbusDataUnit(QModbusDataUnit::RegisterType::HoldingRegisters, registerAddress, 1);
    request.setValue(0, static_cast<uint16_t>(value));

    if (QModbusReply *reply = m_modbusRtuSerialMaster->sendWriteRequest(request, slaveAddress)) {
        if (!reply->isFinished()) {
            connect(reply, &QModbusReply::finished, this, &ModbusRTUMaster::onReplyFinished);
            connect(reply, &QModbusReply::errorOccurred, this, &ModbusRTUMaster::onReplyErrorOccured);
            QTimer::singleShot(200, reply, SLOT(deleteLater()));
        } else {
            delete reply; // broadcast replies return immediately
        }
    } else {
        qCWarning(dcModbusCommander()) << "Read error: " << m_modbusRtuSerialMaster->errorString();
    }
    return true;
}

bool ModbusRTUMaster::readDiscreteInput(int slaveAddress, int registerAddress)
{
    if (!m_modbusRtuSerialMaster)
        return false;

    QModbusDataUnit request = QModbusDataUnit(QModbusDataUnit::RegisterType::DiscreteInputs, registerAddress, 1);

    if (QModbusReply *reply = m_modbusRtuSerialMaster->sendReadRequest(request, slaveAddress)) {
        if (!reply->isFinished()) {
            connect(reply, &QModbusReply::finished, this, &ModbusRTUMaster::onReplyFinished);
            connect(reply, &QModbusReply::errorOccurred, this, &ModbusRTUMaster::onReplyErrorOccured);
            QTimer::singleShot(200, reply, SLOT(deleteLater()));
        } else {
            delete reply; // broadcast replies return immediately
        }
    } else {
        qCWarning(dcModbusCommander()) << "Read error: " << m_modbusRtuSerialMaster->errorString();
    }
    return true;
}

bool ModbusRTUMaster::readInputRegister(int slaveAddress, int registerAddress)
{
    if (!m_modbusRtuSerialMaster)
        return false;

    QModbusDataUnit request = QModbusDataUnit(QModbusDataUnit::RegisterType::InputRegisters, registerAddress, 1);

    if (QModbusReply *reply = m_modbusRtuSerialMaster->sendReadRequest(request, slaveAddress)) {
        if (!reply->isFinished()) {
            connect(reply, &QModbusReply::finished, this, &ModbusRTUMaster::onReplyFinished);
            connect(reply, &QModbusReply::errorOccurred, this, &ModbusRTUMaster::onReplyErrorOccured);
            QTimer::singleShot(200, reply, SLOT(deleteLater()));
        } else {
            delete reply; // broadcast replies return immediately
        }
    } else {
        qCWarning(dcModbusCommander()) << "Read error: " << m_modbusRtuSerialMaster->errorString();
    }
    return true;
}

bool ModbusRTUMaster::readHoldingRegister(int slaveAddress, int registerAddress)
{
    if (!m_modbusRtuSerialMaster)
        return false;

    QModbusDataUnit request = QModbusDataUnit(QModbusDataUnit::RegisterType::HoldingRegisters, registerAddress, 1);

    if (QModbusReply *reply = m_modbusRtuSerialMaster->sendReadRequest(request, slaveAddress)) {
        if (!reply->isFinished()) {
            connect(reply, &QModbusReply::finished, this, &ModbusRTUMaster::onReplyFinished);
            connect(reply, &QModbusReply::errorOccurred, this, &ModbusRTUMaster::onReplyErrorOccured);
            QTimer::singleShot(200, reply, SLOT(deleteLater()));
        } else {
            delete reply; // broadcast replies return immediately
        }
    } else {
        qCWarning(dcModbusCommander()) << "Read error: " << m_modbusRtuSerialMaster->errorString();
    }
    return true;
}


void ModbusRTUMaster::onModbusErrorOccurred(QModbusDevice::Error error)
{
    qCWarning(dcModbusCommander()) << "An error occured" << error;
}


void ModbusRTUMaster::onModbusStateChanged(QModbusDevice::State state)
{
    bool connected = (state != QModbusDevice::UnconnectedState);
    if (!connected) {
        //try to reconnect in 10 seconds
        m_reconnectTimer->start(10000);
    }
    emit connectionStateChanged(connected);
}
