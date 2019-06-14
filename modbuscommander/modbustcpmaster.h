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
#include <QtSerialBus>
#include <QTimer>

class ModbusTCPMaster : public QObject
{
    Q_OBJECT
public:
    explicit ModbusTCPMaster(QString ipAddress, int port, QObject *parent = nullptr);
    ~ModbusTCPMaster();

    bool connectDevice();

    bool getCoil(int slaveAddress, int registerAddress);
    bool getDiscreteInput(int slaveAddress, int registerAddress);
    bool getInputRegister(int slaveAddress, int registerAddress);
    bool getHoldingRegister(int slaveAddress, int registerAddress);

    bool setCoil(int slaveAddress, int registerAddress, bool status);
    bool setHoldingRegister(int slaveAddress, int registerAddress, int data);

    QString ipv4Address();
    int port();
    bool setIPv4Address(QString ipAddress);
    bool setPort(int port);


private:
    QTimer *m_reconnectTimer = nullptr;
    QModbusTcpClient *m_modbusTcpClient;

    QString m_IPv4Address;
    int m_port;

private slots:
    void onReplyFinished();
    void onReplyErrorOccured(QModbusDevice::Error error);
    void onReconnectTimer();

    void onModbusErrorOccurred(QModbusDevice::Error error);
    void onModbusStateChanged(QModbusDevice::State state);

signals:
    void connectionStateChanged(bool status);
    void receivedCoil(int slaveAddress, int modbusRegister, bool value);
    void receivedDiscreteInput(int slaveAddress, int modbusRegister, bool value);
    void receivedHoldingRegister(int slaveAddress, int modbusRegister, int value);
    void receivedInputRegister(int slaveAddress, int modbusRegister, int value);
};

#endif // MODBUSTCPMASTER_H
