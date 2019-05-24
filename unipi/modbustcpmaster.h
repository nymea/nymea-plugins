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
    explicit ModbusTCPMaster(QHostAddress IPv4Address, int port, QObject *parent = nullptr);
    ~ModbusTCPMaster();

    bool createInterface();
    bool isConnected();

    bool getCoil(int coilAddress, bool *result);
    bool getRegister(int registerAddress, int *result);
    bool setCoil(int coilAddress, bool status);
    bool setRegister(int registerAddress, int data);
    QHostAddress ipv4Address();
    int port();
    bool setIPv4Address(QHostAddress IPv4Address);
    bool setPort(int port);

private:
    modbus_t *m_mb;

    QHostAddress m_IPv4Address;
    int m_port;
};

#endif // MODBUSTCPMASTER_H
