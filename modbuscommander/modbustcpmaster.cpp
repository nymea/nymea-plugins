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

ModbusTCPMaster::ModbusTCPMaster(QHostAddress IPv4Address, int port, int slaveAddress, QObject *parent) :
    QObject(parent),
    m_IPv4Address(IPv4Address),
    m_port(port),
    m_slaveAddress(slaveAddress)
{
    // TCP connction to target device
    qDebug(dcModbusCommander()) << "Setting up TCP connecion" << m_IPv4Address.toString() << "port:" << m_port;
    const char *address = m_IPv4Address.toString().toLatin1().data();
    m_mb = modbus_new_tcp(address, m_port);

    if(m_mb == NULL){
        qCWarning(dcModbusCommander()) << "Error modbus TCP: " << modbus_strerror(errno) ;
       this->deleteLater();
        return;
    }

    struct timeval response_timeout;

    response_timeout.tv_sec = 3;
    response_timeout.tv_usec = 0;
    modbus_set_response_timeout(m_mb, &response_timeout);

    if(modbus_connect(m_mb) == -1){
        qCWarning(dcModbusCommander()) << "Error connecting modbus:" << modbus_strerror(errno) ;
        return;
    }

    if(modbus_set_slave(m_mb, m_slaveAddress) == -1){
        qCWarning(dcModbusCommander()) << "Error setting slave ID" << m_slaveAddress << "Reason:" << modbus_strerror(errno) ;
        //free((void*)(m_mb));
        this->deleteLater();
        return;
    }
}

ModbusTCPMaster::~ModbusTCPMaster()
{
    if (m_mb != NULL) {
        modbus_close(m_mb);
    }
     modbus_free(m_mb);
}

int ModbusTCPMaster::port()
{
    return m_port;
}

QHostAddress ModbusTCPMaster::ipv4Address()
{
    return m_IPv4Address;
}

int ModbusTCPMaster::slaveAddress()
{
    return m_slaveAddress;
}

bool ModbusTCPMaster::connected()
{
    if (!m_mb){
        return false;
    }
    // Check if already connected
    if (modbus_read_input_registers(m_mb, 1, 1, NULL) == -1) { //Register address 1 bloody workaround
        return false;
    } else {
        return true;
    }
}

void ModbusTCPMaster::reconnect(int registerAddress)
{
    if (!m_mb){
        return;
    }
    // Check if already connected
    if (modbus_read_input_registers(m_mb, registerAddress, 1, NULL) == -1) {
        qDebug(dcModbusCommander()) << "Could not read register" << registerAddress << "Reason:" << modbus_strerror(errno) ;
        // Try to connect to device
        if (modbus_connect(m_mb) == -1) {
            qCWarning(dcModbusCommander()) << "Connection failed: " << modbus_strerror(errno);
            return;
        }else{
            // recheck the connection
            if (modbus_read_input_registers(m_mb, registerAddress, 1, NULL) == -1)
                return;
        }
    }
}

void ModbusTCPMaster::setCoil(int coilAddress, bool status)
{
    if (!m_mb) {
        return;
    }

    if (modbus_write_bit(m_mb, coilAddress, status) == -1)
        qCWarning(dcModbusCommander()) << "Could not write Coil" << coilAddress << "Reason:" << modbus_strerror(errno);
}

void ModbusTCPMaster::setRegister(int registerAddress, int data)
{
    if (!m_mb) {
        return;
    }

    if (modbus_write_register(m_mb, registerAddress, data) == -1)
        qCWarning(dcModbusCommander()) << "Could not write Register" << registerAddress << "Reason:" << modbus_strerror(errno);
}

bool ModbusTCPMaster::getCoil(int coilAddress)
{
    if (!m_mb){
        return false;
    }

    uint8_t bits;
    if (modbus_read_bits(m_mb, coilAddress, 1, &bits) == -1){
        qCWarning(dcModbusCommander()) << "Could not read bits" << coilAddress << "Reason:"<< modbus_strerror(errno);
    }
    return bits;
}

int ModbusTCPMaster::getRegister(int registerAddress)
{
    uint16_t reg;

    if (!m_mb){
        return 0;
    }

    if (modbus_read_registers(m_mb, registerAddress, 1, &reg) == -1){
        qCWarning(dcModbusCommander()) << "Could not read register" << registerAddress << "Reason:" << modbus_strerror(errno);
    }
    return reg;
}
