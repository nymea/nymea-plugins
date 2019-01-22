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

/*!
    \page serialportcommander.html
    \title Serial Port Commander
    \brief Plug-In to send and receive strings over a serial port.

    \ingroup plugins
    \ingroup nymea-plugins

    \chapter Plugin properties
    Following JSON file contains the definition and the description of all available \l{DeviceClass}{DeviceClasses}
    and \l{Vendor}{Vendors} of this \l{DevicePlugin}.

    For more details how to read this JSON file please check out the documentation for \l{The plugin JSON File}.

    \quotefile plugins/deviceplugins/serialportcommander/devicepluginserialportcommander.json
*/

#include "devicepluginserialportcommander.h"
#include "plugininfo.h"

DevicePluginSerialPortCommander::DevicePluginSerialPortCommander()
{
}

DeviceManager::DeviceSetupStatus DevicePluginSerialPortCommander::setupDevice(Device *device)
{
    if (device->deviceClassId() == serialPortCommanderDeviceClassId) {
        QString interface = device->paramValue(serialPortCommanderDeviceSerialPortParamTypeId).toString();

        if (!m_usedInterfaces.contains(interface)) {

            QSerialPort *serialPort = new QSerialPort(interface, this);
            if(!serialPort)
                return DeviceManager::DeviceSetupStatusFailure;

            serialPort->setBaudRate(device->paramValue(serialPortCommanderDeviceBaudRateParamTypeId).toInt());
            serialPort->setDataBits(QSerialPort::DataBits(device->paramValue(serialPortCommanderDeviceDataBitsParamTypeId).toInt()));

            if (device->paramValue(serialPortCommanderDeviceParityParamTypeId).toString().contains("Even")) {
                serialPort->setParity(QSerialPort::Parity::EvenParity );
            } else if (device->paramValue(serialPortCommanderDeviceParityParamTypeId).toString().contains("Odd")) {
                serialPort->setParity(QSerialPort::Parity::OddParity );
            } else if (device->paramValue(serialPortCommanderDeviceParityParamTypeId).toString().contains("Space")) {
                serialPort->setParity(QSerialPort::Parity::SpaceParity );
            } else if (device->paramValue(serialPortCommanderDeviceParityParamTypeId).toString().contains("Mark")) {
                serialPort->setParity(QSerialPort::Parity::MarkParity );
            } else {
                serialPort->setParity(QSerialPort::Parity::NoParity);
            }

            serialPort->setStopBits(QSerialPort::StopBits(device->paramValue(serialPortCommanderDeviceStopBitsParamTypeId).toInt()));

            if (device->paramValue(serialPortCommanderDeviceFlowControlParamTypeId).toString().contains("Hardware")) {
                serialPort->setFlowControl(QSerialPort::FlowControl::HardwareControl);
            } else if (device->paramValue(serialPortCommanderDeviceFlowControlParamTypeId).toString().contains("Software")) {
                serialPort->setFlowControl(QSerialPort::FlowControl::SoftwareControl);
            } else {
                serialPort->setFlowControl(QSerialPort::FlowControl::NoFlowControl);
            }

            if (!serialPort->open(QIODevice::ReadWrite)) {
                qCWarning(dcSerialPortCommander()) << "Could not open serial port" << interface << serialPort->errorString();
                return DeviceManager::DeviceSetupStatusFailure;
            }

            connect(serialPort, SIGNAL(error(QSerialPort::SerialPortError)), this, SLOT(onSerialError(QSerialPort::SerialPortError)));
            connect(serialPort, SIGNAL(readyRead()), this, SLOT(onReadyRead()));
            connect(serialPort, SIGNAL(baudRateChanged(qint32, QSerialPort::Direction)), this, SLOT(onBaudRateChanged(qint32, QSerialPort::Direction)));
            connect(serialPort, SIGNAL(parityChanged(QSerialPort::Parity)), this, SLOT(onParityChanged(QSerialPort::Parity)));
            connect(serialPort, SIGNAL(dataBitsChanged(QSerialPort::DataBits)), this, SLOT(onDataBitsChanged(QSerialPort::DataBits)));
            connect(serialPort, SIGNAL(stopBitsChanged(QSerialPort::StopBits)), this, SLOT(onStopBitsChanged(QSerialPort::StopBits)));
            connect(serialPort, SIGNAL(flowControlChanged(QSerialPort::FlowControl)), this, SLOT(onFlowControlChanged(QSerialPort::FlowControl)));
            qCDebug(dcSerialPortCommander()) << "Setup successfully serial port" << interface;
            m_usedInterfaces.append(interface);
            m_serialPorts.insert(device, serialPort);
        } else {
            return DeviceManager::DeviceSetupStatusFailure;
        }
        return DeviceManager::DeviceSetupStatusSuccess;
    }
    return DeviceManager::DeviceSetupStatusFailure;
}


DeviceManager::DeviceError DevicePluginSerialPortCommander::discoverDevices(const DeviceClassId &deviceClassId, const ParamList &params)
{
    Q_UNUSED(params)
    // Create the list of available serial interfaces
    QList<DeviceDescriptor> deviceDescriptors;

    Q_FOREACH(QSerialPortInfo port, QSerialPortInfo::availablePorts()) {
        if (m_usedInterfaces.contains(port.portName())){
            //device already in use
            qCDebug(dcSerialPortCommander()) << "Found serial port that is already used:" << port.portName();
        } else {
            //Serial port is not yet used, create now a new one
            qCDebug(dcSerialPortCommander()) << "Found serial port:" << port.portName();
            QString description = port.manufacturer() + " " + port.description();
            DeviceDescriptor descriptor(deviceClassId, port.portName(), description);
            ParamList parameters;
            parameters.append(Param(serialPortCommanderDeviceSerialPortParamTypeId, port.portName()));
            descriptor.setParams(parameters);
            deviceDescriptors.append(descriptor);
        }
    }
    emit devicesDiscovered(deviceClassId, deviceDescriptors);
    return DeviceManager::DeviceErrorAsync;
}


DeviceManager::DeviceError DevicePluginSerialPortCommander::executeAction(Device *device, const Action &action)
{
    if (device->deviceClassId() == serialPortCommanderDeviceClassId ) {

        if (action.actionTypeId() == serialPortCommanderTriggerActionTypeId) {

            QSerialPort *serialPort = m_serialPorts.value(device);
            serialPort->write(action.param(serialPortCommanderTriggerActionOutputDataParamTypeId).value().toByteArray());

            return DeviceManager::DeviceErrorNoError;
        }
        return DeviceManager::DeviceErrorActionTypeNotFound;
    }
    return DeviceManager::DeviceErrorDeviceClassNotFound;
}


void DevicePluginSerialPortCommander::deviceRemoved(Device *device)
{
    if (device->deviceClassId() == serialPortCommanderDeviceClassId) {

        m_usedInterfaces.removeAll(device->paramValue(serialPortCommanderDeviceSerialPortParamTypeId).toString());
        QSerialPort *serialPort = m_serialPorts.take(device);
        serialPort->close();
        serialPort->deleteLater();
    }
}

void DevicePluginSerialPortCommander::onReadyRead()
{
    QSerialPort *serialPort =  static_cast<QSerialPort*>(sender());
    Device *device = m_serialPorts.key(serialPort);

    QByteArray data;
    while (!serialPort->atEnd()) {
        data = serialPort->read(100);
    }
    qDebug(dcSerialPortCommander()) << "Message received" << data;

    device->setStateValue(serialPortCommanderInputDataStateTypeId, data);
    emitEvent(Event(serialPortCommanderTriggeredEventTypeId, device->id()));
}

void DevicePluginSerialPortCommander::onSerialError(QSerialPort::SerialPortError error)
{
    qCWarning(dcSerialPortCommander) << "Serial Port error happened:" << error;
}

void DevicePluginSerialPortCommander::onBaudRateChanged(qint32 baudRate, QSerialPort::Direction direction)
{
    Q_UNUSED(direction)
    QSerialPort *serialPort =  static_cast<QSerialPort*>(sender());
    Device *device = m_serialPorts.key(serialPort);
    device->setParamValue(serialPortCommanderDeviceBaudRateParamTypeId, baudRate);
}

void DevicePluginSerialPortCommander::onParityChanged(QSerialPort::Parity parity)
{
    QSerialPort *serialPort =  static_cast<QSerialPort*>(sender());
    Device *device = m_serialPorts.key(serialPort);
    device->setParamValue(serialPortCommanderDeviceParityParamTypeId, parity);
}

void DevicePluginSerialPortCommander::onDataBitsChanged(QSerialPort::DataBits dataBits)
{
    QSerialPort *serialPort =  static_cast<QSerialPort*>(sender());
    Device *device = m_serialPorts.key(serialPort);
    device->setParamValue(serialPortCommanderDeviceDataBitsParamTypeId, dataBits);
}

void DevicePluginSerialPortCommander::onStopBitsChanged(QSerialPort::StopBits stopBits)
{
    QSerialPort *serialPort =  static_cast<QSerialPort*>(sender());
    Device *device = m_serialPorts.key(serialPort);
    device->setParamValue(serialPortCommanderDeviceStopBitsParamTypeId, stopBits);
}

void DevicePluginSerialPortCommander::onFlowControlChanged(QSerialPort::FlowControl flowControl)
{
    QSerialPort *serialPort =  static_cast<QSerialPort*>(sender());
    Device *device = m_serialPorts.key(serialPort);
    device->setParamValue(serialPortCommanderDeviceFlowControlParamTypeId, flowControl);
}

