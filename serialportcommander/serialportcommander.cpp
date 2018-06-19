/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *                                                                         *
 *  Copyright (C) 2017 Bernhard Trinnes  <bernhard.trinnes@guh.io>         *
 *                                                                         *
 *  This file is part of guh.                                              *
 *                                                                         *
 *  Guh is free software: you can redistribute it and/or modify            *
 *  it under the terms of the GNU General Public License as published by   *
 *  the Free Software Foundation, version 2 of the License.                *
 *                                                                         *
 *  Guh is distributed in the hope that it will be useful,                 *
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of         *
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the           *
 *  GNU General Public License for more details.                           *
 *                                                                         *
 *  You should have received a copy of the GNU General Public License      *
 *  along with guh. If not, see <http://www.gnu.org/licenses/>.            *
 *                                                                         *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

#include "serialportcommander.h"

SerialPortCommander::SerialPortCommander(QSerialPort *serialPort, QObject *parent) :
    QObject(parent),
    m_serialPort(serialPort)
{
    connect(m_serialPort, SIGNAL(error(QSerialPort::SerialPortError)), this, SLOT(onSerialError(QSerialPort::SerialPortError)));
    connect(m_serialPort, SIGNAL(readyRead()), this, SLOT(onReadyRead()));
    connect(m_serialPort, SIGNAL(baudRateChanged(qint32, QSerialPort::Direction)), this, SLOT(onBaudRateChanged(qint32, QSerialPort::Direction)));
    connect(m_serialPort, SIGNAL(parityChanged(QSerialPort::Parity)), this, SLOT(onParityChanged(QSerialPort::Parity)));
    connect(m_serialPort, SIGNAL(dataBitsChanged(QSerialPort::DataBits)), this, SLOT(onDataBitsChanged(QSerialPort::DataBits)));
    connect(m_serialPort, SIGNAL(stopBitsChanged(QSerialPort::StopBits)), this, SLOT(onStopBitsChanged(QSerialPort::StopBits)));
    connect(m_serialPort, SIGNAL(flowControlChanged(QSerialPort::FlowControl)), this, SLOT(onFlowControlChanged(QSerialPort::FlowControl)));
}


SerialPortCommander::~SerialPortCommander()
{
    m_serialPort->close();
    m_serialPort->deleteLater();
}


void SerialPortCommander::addOutputDevice(Device* device)
{
    m_outputDevice = device;
}


void SerialPortCommander::removeOutputDevice()
{
    m_outputDevice = nullptr;
}


void SerialPortCommander::addInputDevice(Device* device)
{
    m_inputDevices.append(device);
}


void SerialPortCommander::removeInputDevice(Device* device)
{
    m_inputDevices.removeAll(device);
}


bool SerialPortCommander::isEmpty()
{
    return(!hasOutputDevice() && m_inputDevices.empty());
}


bool SerialPortCommander::hasOutputDevice()
{
    if (m_outputDevice == nullptr) {
        return false;
    } else {
        return true;
    }
}


QSerialPort * SerialPortCommander::serialPort()
{
    return m_serialPort;
}


Device * SerialPortCommander::outputDevice()
{
    return m_outputDevice;
}


void SerialPortCommander::onReadyRead()
{
    QByteArray data;
    while (!m_serialPort->atEnd()) {
        data = m_serialPort->read(100);
    }
    qDebug(dcSerialPortCommander()) << "Message received" << data;

    foreach (Device *device, m_inputDevices) {
        if (device->paramValue(serialPortInputComparisonTypeParamTypeId).toString() == "Is exactly") {
            if (data == device->paramValue(serialPortInputInputCommandParamTypeId)) {
                emit commandReceived(device);
            }
        } else if (device->paramValue(serialPortInputComparisonTypeParamTypeId).toString() == "Contains") {
            if (data.contains(device->paramValue(serialPortInputInputCommandParamTypeId).toByteArray())) {
                emit commandReceived(device);
            }
        } else if (device->paramValue(serialPortInputComparisonTypeParamTypeId) == "Contains not") {
            if (!data.contains(device->paramValue(serialPortInputInputCommandParamTypeId).toByteArray())) {
               emit commandReceived(device);
            }
        } else if (device->paramValue(serialPortInputComparisonTypeParamTypeId) == "Starts with") {
            if (data.startsWith(device->paramValue(serialPortInputInputCommandParamTypeId).toByteArray())) {
                emit commandReceived(device);
            }
        } else if (device->paramValue(serialPortInputComparisonTypeParamTypeId) == "Ends with") {
            if (data.endsWith(device->paramValue(serialPortInputInputCommandParamTypeId).toByteArray())) {
                emit commandReceived(device);
            }
        }
    }
}

void SerialPortCommander::onSerialError(QSerialPort::SerialPortError error)
{
    Q_UNUSED(error);
}

void SerialPortCommander::onBaudRateChanged(qint32 baudRate, QSerialPort::Direction direction)
{
    Q_UNUSED(direction);
    foreach(Device *device, m_inputDevices) {
        device->setParamValue(serialPortInputBaudRateParamTypeId, baudRate);
    }
    if(m_outputDevice != nullptr)
        m_outputDevice->setParamValue(serialPortOutputBaudRateParamTypeId, baudRate);
}

void SerialPortCommander::onParityChanged(QSerialPort::Parity parity)
{
    foreach(Device *device, m_inputDevices) {
        device->setParamValue(serialPortInputParityParamTypeId, parity); //TODO Strings not int
    }
    if(m_outputDevice != nullptr)
        m_outputDevice->setParamValue(serialPortOutputBaudRateParamTypeId, parity);
}

void SerialPortCommander::onDataBitsChanged(QSerialPort::DataBits dataBits)
{
    foreach(Device *device, m_inputDevices) {
        device->setParamValue(serialPortInputDataBitsParamTypeId, dataBits);
    }
    if(m_outputDevice != nullptr)
        m_outputDevice->setParamValue(serialPortOutputDataBitsParamTypeId, dataBits);
}

void SerialPortCommander::onStopBitsChanged(QSerialPort::StopBits stopBits)
{
    foreach(Device *device, m_inputDevices) {
        device->setParamValue(serialPortInputStopBitsParamTypeId, stopBits);
    }
    if(m_outputDevice != nullptr)
        m_outputDevice->setParamValue(serialPortOutputStopBitsParamTypeId, stopBits);
}

void SerialPortCommander::onFlowControlChanged(QSerialPort::FlowControl flowControl)
{
    //foreach(Device *device, m_inputDevices) { //TODO enum to stringau
        //device->setParamValue(serialPortInputFlowControlParamTypeId, QVariant::fromValue(QSerialPort::FlowControl).value<QString>());
    //}
    if(m_outputDevice != nullptr)
        m_outputDevice->setParamValue(serialPortOutputFlowControlParamTypeId, flowControl);
}

void SerialPortCommander::sendCommand(QByteArray data)
{
    m_serialPort->write(data);
}
