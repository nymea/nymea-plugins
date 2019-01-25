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

#ifndef MODBUSRTUMASTER_H
#define MODBUSRTUMASTER_H

#include <QObject>
#include <modbus/modbus.h>

class ModbusRTUMaster : public QObject
{
    Q_OBJECT
public:
    explicit ModbusRTUMaster(QString serialPort, int baudrate, QString parity, int dataBits, int stopBits, QObject *parent = 0);
    ~ModbusRTUMaster();

    bool getCoil(int slaveAddress, int coilAddress);
    int getRegister(int slaveAddress, int registerAddress);
    void setCoil(int slaveAddress, int coilAddress, bool status);
    void setRegister(int slaveAddress, int registerAddress, int data);
    void reconnect(int slaveAddress, int address);

    QString serialPort();
    bool connected();

private:
     modbus_t *m_mb;
     QString m_serialPort;
     int m_baudrate;
     QString m_parity;
     int m_dataBits;
     int m_stopBits;

signals:

public slots:
};

#endif // MODBUSRTUMASTER_H
