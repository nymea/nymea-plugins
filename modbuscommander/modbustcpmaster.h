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

#ifndef MODBUSTCPMASTER_H
#define MODBUSTCPMASTER_H

#include <QObject>
#include <QHostAddress>
#include <modbus/modbus.h>

class ModbusTCPMaster : public QObject
{
    Q_OBJECT
public:
    explicit ModbusTCPMaster(QHostAddress IPv4Address, int port, int slaveAddress, QObject *parent = 0);
    ~ModbusTCPMaster();

    bool getCoil(int coilAddress);
    int getRegister(int registerAddress);
    void setCoil(int coilAddress, bool status);
    void setRegister(int registerAddress, int data);
    void reconnect(int address);
    QHostAddress ipv4Address();
    int port();
    int slaveAddress();
    bool connected();
private:
     modbus_t *m_mb;
     QHostAddress m_IPv4Address;
     int m_port;
     int m_slaveAddress;

signals:

public slots:
};

#endif // MODBUSTCPMASTER_H
