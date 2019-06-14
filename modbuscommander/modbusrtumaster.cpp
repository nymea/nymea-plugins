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

ModbusRTUMaster::ModbusRTUMaster(QString serialPort, int baudrate, QString parity, int dataBits, int stopBits, QObject *parent) :
    QObject(parent),
    m_serialPort(serialPort),
    m_baudrate(baudrate),
    m_parity(parity),
    m_dataBits(dataBits),
    m_stopBits(stopBits)
{
    QModbusRtuSerialMaster *modbusRTUMaster = new QModbusRtuSerialMaster(this);
    modbusRTUMaster->setConnectionParameter(QModbusDevice::SerialPortNameParameter, serialPort);
    modbusRTUMaster->setConnectionParameter(QModbusDevice::SerialBaudRateParameter, baudrate);
    modbusRTUMaster->setConnectionParameter(QModbusDevice::SerialDataBitsParameter, 8);
    modbusRTUMaster->setConnectionParameter(QModbusDevice::SerialStopBitsParameter, 1);
    //modbusRTUMaster->setTimeout(100);
    //modbusRTUMaster->setNumberOfRetries(1);
}

ModbusRTUMaster::~ModbusRTUMaster()
{

}

QString ModbusRTUMaster::serialPort()
{
    return m_serialPort;
}

bool ModbusRTUMaster::createInterface()
{
    //Setting up a RTU interface
    qDebug(dcModbusCommander()) << "Setting up a RTU interface" << m_serialPort << "baud:" << m_baudrate << "parity:" << m_parity << "stop bits:" << m_stopBits << "data bits:" << m_dataBits ;

    if (!isConnected())
        return false;

    char parity;
    if (m_parity.size() == 1) {
        parity = m_parity.toUtf8().at(0);
    } else {
        qCWarning(dcModbusCommander()) << "Error parity wrong: " << m_parity;
        return false;
    }

    char *port = m_serialPort.toLatin1().data();
    m_mb = modbus_new_rtu(port, m_baudrate, parity, m_dataBits, m_stopBits);

    if(m_mb == nullptr){
        qCWarning(dcModbusCommander()) << "Error modbus RTU: " << modbus_strerror(errno);
        return false;
    }

    if(modbus_connect(m_mb) == -1){
        qCWarning(dcModbusCommander()) << "Error connecting modbus:" << modbus_strerror(errno) << port;
        return false;
    }
    return true;
}

bool ModbusRTUMaster::isConnected()
{
    Q_FOREACH(QSerialPortInfo port, QSerialPortInfo::availablePorts()) {

        if (m_serialPort == port.systemLocation()) {

            if(modbus_connect(m_mb) == -1){
                return false;
            }
            return true;
        }
    }
    return false;
}

bool ModbusRTUMaster::setCoil(int slaveAddress, int coilAddress, bool status)
{
    if (!isConnected())
        return false;

    if(modbus_set_slave(m_mb, slaveAddress) == -1){
        qCWarning(dcModbusCommander()) << "Error setting slave ID" << slaveAddress << "Reason:" << modbus_strerror(errno) ;
        return false;
    }

    if (modbus_write_bit(m_mb, coilAddress, status) == -1) {
        qCWarning(dcModbusCommander()) << "Could not write Coil" << coilAddress << "Reason:" << modbus_strerror(errno);
        return false;
    }
    return true;
}

bool ModbusRTUMaster::setRegister(int slaveAddress, int registerAddress, int data)
{
    if (!m_mb) {
        if (!createInterface())
            return false;
    }

    if (!isConnected())
        return false;

    if(modbus_set_slave(m_mb, slaveAddress) == -1){
        qCWarning(dcModbusCommander()) << "Error setting slave ID" << slaveAddress << "Reason:" << modbus_strerror(errno) ;
        return false;
    }

    if (modbus_write_register(m_mb, registerAddress, data) == -1) {
        qCWarning(dcModbusCommander()) << "Could not write Register" << registerAddress << "Reason:" << modbus_strerror(errno);
        return false;
    }
    return true;
}

bool ModbusRTUMaster::getCoil(int slaveAddress, int coilAddress)
{
    if (!m_mb) {
        if (!createInterface())
            return false;
    }

    return true;
}

bool ModbusRTUMaster::getRegister(int slaveAddress, int registerAddress, int *result)
{
    uint16_t data;

    if (!m_mb) {
        if (!createInterface())
            return false;
    }

    if (!isConnected())
        return false;

    if(modbus_set_slave(m_mb, slaveAddress) == -1){
        qCWarning(dcModbusCommander()) << "Error setting slave ID" << slaveAddress << "Reason:" << modbus_strerror(errno) ;
        return false;
    }

    if (modbus_read_registers(m_mb, registerAddress, 1, &data) == -1){
        qCWarning(dcModbusCommander()) << "Could not read register" << registerAddress << "Reason:" << modbus_strerror(errno);
        return false;
    }
    *result = data;
    return true;
}
