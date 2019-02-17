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

ModbusTCPMaster::ModbusTCPMaster(QHostAddress IPv4Address, int port, QObject *parent) :
    QObject(parent),
    m_IPv4Address(IPv4Address),
    m_port(port)
{
    createInterface();
}

ModbusTCPMaster::~ModbusTCPMaster()
{
    if (m_mb != NULL) {
        modbus_close(m_mb);
    }
    modbus_free(m_mb);
}

bool ModbusTCPMaster::createInterface() {
    // TCP connction to target device
    qDebug(dcModbusCommander()) << "Setting up TCP connecion" << m_IPv4Address.toString() << "port:" << m_port;
    const char *address = m_IPv4Address.toString().toLatin1().data();
    m_mb = modbus_new_tcp(address, m_port);

    if(m_mb == nullptr){
        qCWarning(dcModbusCommander()) << "Error modbus TCP: " << modbus_strerror(errno) ;
        return false;
    }

    // Extend the timeout to 3 seconds
    struct timeval response_timeout;
    response_timeout.tv_sec = 3;
    response_timeout.tv_usec = 0;
    modbus_set_response_timeout(m_mb, &response_timeout);

    if(modbus_connect(m_mb) == -1){
        qCWarning(dcModbusCommander()) << "Error connecting modbus:" << modbus_strerror(errno) ;
        return false;
    }
    return true;
}

int ModbusTCPMaster::port()
{
    return m_port;
}

bool ModbusTCPMaster::setIPv4Address(QHostAddress ipv4Address)
{
    m_IPv4Address = ipv4Address;
    if (!createInterface()) {
        return false;
    }
    return true;
}

bool ModbusTCPMaster::setPort(int port)
{
    m_port = port;
    if (!createInterface()) {
        return false;
    }
    return true;
}

QHostAddress ModbusTCPMaster::ipv4Address()
{
    return m_IPv4Address;
}

bool ModbusTCPMaster::setCoil(int slaveAddress, int coilAddress, bool status)
{
    if (m_mb == nullptr) {
        if (!createInterface())
            return false;
    }

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

bool ModbusTCPMaster::setRegister(int slaveAddress, int registerAddress, int data)
{
    if (m_mb == nullptr) {
        if (!createInterface())
            return false;
    }
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

bool ModbusTCPMaster::getCoil(int slaveAddress, int coilAddress, bool *result)
{
    if (m_mb == nullptr) {
        if (!createInterface())
            return false;
    }

    if(modbus_set_slave(m_mb, slaveAddress) == -1){
        qCWarning(dcModbusCommander()) << "Error setting slave ID" << slaveAddress << "Reason:" << modbus_strerror(errno) ;
        return false;
    }

    uint8_t status;
    if (modbus_read_bits(m_mb, coilAddress, 1, &status) == -1){
        qCWarning(dcModbusCommander()) << "Could not read bits" << coilAddress << "Reason:"<< modbus_strerror(errno);
        return false;
    }
    *result = (bool)status;
    return true;
}

bool ModbusTCPMaster::getRegister(int slaveAddress, int registerAddress, int *result)
{
    uint16_t data;

    if (m_mb == nullptr) {
        if (!createInterface())
            return false;
    }

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
