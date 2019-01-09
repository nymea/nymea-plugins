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

#include "devicepluginserialportcommander.h"
#include "plugininfo.h"

DevicePluginSerialPortCommander::DevicePluginSerialPortCommander()
{
}

void DevicePluginSerialPortCommander::init()
{
}


DeviceManager::DeviceSetupStatus DevicePluginSerialPortCommander::setupDevice(Device *device)
{

    if (device->deviceClassId() == serialPortCommanderDeviceClassId) {
        QString interface = device->paramValue(serialPortCommanderDeviceSerialPortParamTypeId).toString();

        if (!m_serialPorts.contains(interface)) {

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

            qCDebug(dcSerialPortCommander()) << "Setup successfully serial port" << interface;
            m_serialPorts.insert(interface, serialPort);

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
        if (m_serialPorts.contains(port.portName())){
            //device already in use
              qCDebug(dcSerialPortCommander()) << "Found serial port that is already used:" << port.portName();
        } else {
            //Serial port is not yet used, create now a new one
            qCDebug(dcSerialPortCommander()) << "Found serial port:" << port.portName();
            QString description = port.manufacturer() + " " + port.description();
            DeviceDescriptor descriptor(deviceClassId, port.portName(), description);
            ParamList parameters;

            if (deviceClassId == serialPortCommanderDeviceClassId) {
                parameters.append(Param(serialPortCommanderDeviceSerialPortParamTypeId, port.portName()));
            }

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

            QString interface = device->paramValue(serialPortCommanderDeviceSerialPortParamTypeId).toString();
            QSerialPort *serialPort = m_serialPorts.value(interface);
            serialPort->write(action.param(serialPortCommanderTriggerActionOutputDataParamTypeId).value().toByteArray());

            return DeviceManager::DeviceErrorNoError;
        }
        return DeviceManager::DeviceErrorActionTypeNotFound;
    }
    return DeviceManager::DeviceErrorDeviceClassNotFound;
}


void DevicePluginSerialPortCommander::deviceRemoved(Device *device)
{
    QString interface;
    QSerialPort *serialPort;

    if (device->deviceClassId() == serialPortCommanderDeviceClassId) {

        interface = device->paramValue(serialPortCommanderDeviceSerialPortParamTypeId).toString();
        serialPort = m_serialPorts.value(interface);
        serialPort->close();
        m_serialPorts.remove(interface);
        serialPort->deleteLater();
    }
}


void DevicePluginSerialPortCommander::onCommandReceived(Device *device)
{
    emitEvent(Event(serialPortCommanderTriggeredEventTypeId, device->id()));
}
