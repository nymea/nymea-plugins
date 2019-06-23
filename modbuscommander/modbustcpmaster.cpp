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

#include "modbustcpmaster.h"
#include "extern-plugininfo.h"

ModbusTCPMaster::ModbusTCPMaster(QString IPv4Address, int port, QObject *parent) :
    QObject(parent)
{
    m_modbusTcpClient = new QModbusTcpClient(this);
    m_modbusTcpClient->setConnectionParameter(QModbusDevice::NetworkPortParameter, port);
    m_modbusTcpClient->setConnectionParameter(QModbusDevice::NetworkAddressParameter, IPv4Address);
    //m_modbusTcpClient->setTimeout(100);
    //m_modbusTcpClient->setNumberOfRetries(1);

    connect(m_modbusTcpClient, &QModbusTcpClient::stateChanged, this, &ModbusTCPMaster::onModbusStateChanged);
    connect(m_modbusTcpClient, &QModbusRtuSerialMaster::errorOccurred, this, &ModbusTCPMaster::onModbusErrorOccurred);

    m_reconnectTimer = new QTimer(this);
    m_reconnectTimer->setSingleShot(true);
    connect(m_reconnectTimer, &QTimer::timeout, this, &ModbusTCPMaster::onReconnectTimer);
}

ModbusTCPMaster::~ModbusTCPMaster()
{
    if (!m_modbusTcpClient) {
        m_modbusTcpClient->disconnectDevice();
        m_modbusTcpClient->deleteLater();
    }
    if (!m_reconnectTimer) {
        m_reconnectTimer->stop();
        m_reconnectTimer->deleteLater();
    }
}

bool ModbusTCPMaster::connectDevice() {
    // TCP connction to target device
    qDebug(dcModbusCommander()) << "Setting up TCP connecion";

    if (!m_modbusTcpClient)
        return false;

    return m_modbusTcpClient->connectDevice();
}

int ModbusTCPMaster::port()
{
    return m_modbusTcpClient->connectionParameter(QModbusDevice::NetworkPortParameter).toInt();
}

bool ModbusTCPMaster::setIPv4Address(QString ipv4Address)
{
    m_modbusTcpClient->setConnectionParameter(QModbusDevice::NetworkAddressParameter, ipv4Address);
    return connectDevice();
}

bool ModbusTCPMaster::setPort(int port)
{
    m_modbusTcpClient->setConnectionParameter(QModbusDevice::NetworkPortParameter, port);
    return connectDevice();
}

void ModbusTCPMaster::onReplyFinished()
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

void ModbusTCPMaster::onReplyErrorOccured(QModbusDevice::Error error)
{
    qCWarning(dcModbusCommander()) << "Modbus replay error:" << error;
     QModbusReply *reply = qobject_cast<QModbusReply *>(sender());
     if (!reply)
         return;
     reply->finished(); //to make sure it will be deleted
}

void ModbusTCPMaster::onReconnectTimer()
{
    if(!m_modbusTcpClient->connectDevice()) {
        m_reconnectTimer->start(10000);
    }
}

QString ModbusTCPMaster::ipv4Address()
{
    return m_modbusTcpClient->connectionParameter(QModbusDevice::NetworkAddressParameter).toString();
}

bool ModbusTCPMaster::readCoil(int slaveAddress, int registerAddress)
{
    if (!m_modbusTcpClient)
        return false;

    QModbusDataUnit request = QModbusDataUnit(QModbusDataUnit::RegisterType::Coils, registerAddress, 1);

    if (QModbusReply *reply = m_modbusTcpClient->sendReadRequest(request, slaveAddress)) {
        if (!reply->isFinished()) {
            connect(reply, &QModbusReply::finished, this, &ModbusTCPMaster::onReplyFinished);
            connect(reply, &QModbusReply::errorOccurred, this, &ModbusTCPMaster::onReplyErrorOccured);
            QTimer::singleShot(200, reply, SLOT(deleteLater()));
        } else {
            delete reply; // broadcast replies return immediately
        }
    } else {
        qCWarning(dcModbusCommander()) << "Read error: " << m_modbusTcpClient->errorString();
    }
    return true;
}

bool ModbusTCPMaster::writeHoldingRegister(int slaveAddress, int registerAddress, int value)
{
    if (!m_modbusTcpClient)
        return false;

    QModbusDataUnit request = QModbusDataUnit(QModbusDataUnit::RegisterType::HoldingRegisters, registerAddress, 1);
    request.setValue(0, static_cast<uint16_t>(value));

    if (QModbusReply *reply = m_modbusTcpClient->sendWriteRequest(request, slaveAddress)) {
        if (!reply->isFinished()) {
            connect(reply, &QModbusReply::finished, this, &ModbusTCPMaster::onReplyFinished);
            connect(reply, &QModbusReply::errorOccurred, this, &ModbusTCPMaster::onReplyErrorOccured);
            QTimer::singleShot(200, reply, SLOT(deleteLater()));
        } else {
            delete reply; // broadcast replies return immediately
        }
    } else {
        qCWarning(dcModbusCommander()) << "Read error: " << m_modbusTcpClient->errorString();
    }
    return true;
}

bool ModbusTCPMaster::readDiscreteInput(int slaveAddress, int registerAddress)
{
    if (!m_modbusTcpClient)
        return false;

    QModbusDataUnit request = QModbusDataUnit(QModbusDataUnit::RegisterType::Coils, registerAddress, 1);

    if (QModbusReply *reply = m_modbusTcpClient->sendReadRequest(request, slaveAddress)) {
        if (!reply->isFinished()) {
            connect(reply, &QModbusReply::finished, this, &ModbusTCPMaster::onReplyFinished);
            connect(reply, &QModbusReply::errorOccurred, this, &ModbusTCPMaster::onReplyErrorOccured);
            QTimer::singleShot(200, reply, SLOT(deleteLater()));
        } else {
            delete reply; // broadcast replies return immediately
        }
    } else {
        qCWarning(dcModbusCommander()) << "Read error: " << m_modbusTcpClient->errorString();
    }
    return true;
}

bool ModbusTCPMaster::readInputRegister(int slaveAddress, int registerAddress)
{
    if (!m_modbusTcpClient)
        return false;

    QModbusDataUnit request = QModbusDataUnit(QModbusDataUnit::RegisterType::InputRegisters, registerAddress, 1);

    if (QModbusReply *reply = m_modbusTcpClient->sendReadRequest(request, slaveAddress)) {
        if (!reply->isFinished()) {
            connect(reply, &QModbusReply::finished, this, &ModbusTCPMaster::onReplyFinished);
            connect(reply, &QModbusReply::errorOccurred, this, &ModbusTCPMaster::onReplyErrorOccured);
            QTimer::singleShot(200, reply, SLOT(deleteLater()));
        } else {
            delete reply; // broadcast replies return immediately
        }
    } else {
        qCWarning(dcModbusCommander()) << "Read error: " << m_modbusTcpClient->errorString();
    }
    return true;
}

bool ModbusTCPMaster::readHoldingRegister(int slaveAddress, int registerAddress)
{
    if (!m_modbusTcpClient)
        return false;

    QModbusDataUnit request = QModbusDataUnit(QModbusDataUnit::RegisterType::HoldingRegisters, registerAddress, 1);

    if (QModbusReply *reply = m_modbusTcpClient->sendReadRequest(request, slaveAddress)) {
        if (!reply->isFinished()) {
            connect(reply, &QModbusReply::finished, this, &ModbusTCPMaster::onReplyFinished);
            connect(reply, &QModbusReply::errorOccurred, this, &ModbusTCPMaster::onReplyErrorOccured);
            QTimer::singleShot(200, reply, SLOT(deleteLater()));
        } else {
            delete reply; // broadcast replies return immediately
        }
    } else {
        qCWarning(dcModbusCommander()) << "Read error: " << m_modbusTcpClient->errorString();
    }
    return true;
}

bool ModbusTCPMaster::writeCoil(int slaveAddress, int registerAddress, bool value)
{
    if (!m_modbusTcpClient)
        return false;

    QModbusDataUnit request = QModbusDataUnit(QModbusDataUnit::RegisterType::Coils, registerAddress, 1);
    request.setValue(0, static_cast<uint16_t>(value));

    if (QModbusReply *reply = m_modbusTcpClient->sendWriteRequest(request, slaveAddress)) {
        if (!reply->isFinished()) {
            connect(reply, &QModbusReply::finished, this, &ModbusTCPMaster::onReplyFinished);
            connect(reply, &QModbusReply::errorOccurred, this, &ModbusTCPMaster::onReplyErrorOccured);
            QTimer::singleShot(200, reply, SLOT(deleteLater()));
        } else {
            delete reply; // broadcast replies return immediately
        }
    } else {
        qCWarning(dcModbusCommander()) << "Read error: " << m_modbusTcpClient->errorString();
    }
    return true;
}


void ModbusTCPMaster::onModbusErrorOccurred(QModbusDevice::Error error)
{
    qCWarning(dcModbusCommander()) << "An error occured" << error;
}


void ModbusTCPMaster::onModbusStateChanged(QModbusDevice::State state)
{
    bool connected = (state != QModbusDevice::UnconnectedState);
    if (!connected) {
        //try to reconnect in 10 seconds
        m_reconnectTimer->start(10000);
    }
    emit connectionStateChanged(connected);
}
