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
#include <QtSerialBus>
#include <QSerialPort>
#include <QTimer>

class ModbusRTUMaster : public QObject
{
    Q_OBJECT
public:
    explicit ModbusRTUMaster(QString serialPort, int baudrate, QSerialPort::Parity parity, int dataBits, int stopBits, QObject *parent = nullptr);
    ~ModbusRTUMaster();

    bool connectDevice();

    bool readCoil(int slaveAddress, int registerAddress);
    bool readDiscreteInput(int slaveAddress, int registerAddress);
    bool readInputRegister(int slaveAddress, int registerAddress);
    bool readHoldingRegister(int slaveAddress, int registerAddress);

    bool writeCoil(int slaveAddress, int registerAddress, bool status);
    bool writeHoldingRegister(int slaveAddress, int registerAddress, int data);

    QString serialPort();

private:
    QModbusRtuSerialMaster *m_modbusRtuSerialMaster;
    QTimer *m_reconnectTimer = nullptr;

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

#endif // MODBUSRTUMASTER_H
