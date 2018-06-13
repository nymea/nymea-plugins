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

    if (device->deviceClassId() == serialPortOutputDeviceClassId) {
        QString interface = device->paramValue(serialPortOutputSerialPortParamTypeId).toString();

        if (!m_serialPortCommanders.contains(interface)) {

            QSerialPort *serialPort = new QSerialPort(interface, this);
            if(!serialPort)
                return DeviceManager::DeviceSetupStatusFailure;

            serialPort->setBaudRate(device->paramValue(serialPortInputBaudRateParamTypeId).toInt());
            serialPort->setDataBits(QSerialPort::DataBits(device->paramValue(serialPortInputDataBitsParamTypeId).toInt()));
            serialPort->setParity(QSerialPort::Parity(device->paramValue(serialPortInputParityParamTypeId).toInt()));
            serialPort->setStopBits(QSerialPort::StopBits(device->paramValue(serialPortInputStopBitsParamTypeId).toInt()));

            if (!serialPort->open(QIODevice::ReadWrite)) {
                qCWarning(dcSerialPortCommander()) << "Could not open serial port" << interface << serialPort->errorString();
                return DeviceManager::DeviceSetupStatusFailure;
            }

            qCDebug(dcSerialPortCommander()) << "Setup successfully serial port" << interface;

            SerialPortCommander *serialPortCommander = new SerialPortCommander(serialPort, this);
            serialPortCommander->addOutputDevice(device);
            m_serialPortCommanders.insert(interface, serialPortCommander);

        } else {
            SerialPortCommander *serialPortCommander = m_serialPortCommanders.value(interface);
            if (serialPortCommander->hasOutputDevice())
                return DeviceManager::DeviceSetupStatusFailure;
            serialPortCommander->addOutputDevice(device);

        }
        return DeviceManager::DeviceSetupStatusSuccess;

    } else if (device->deviceClassId() == serialPortInputDeviceClassId) {
        QString interface = device->paramValue(serialPortInputSerialPortParamTypeId).toString();

        if (!m_serialPortCommanders.contains(interface)) {

            QSerialPort *serialPort = new QSerialPort(interface, this);
            if(!serialPort)
                return DeviceManager::DeviceSetupStatusFailure;

            serialPort->setBaudRate(device->paramValue(serialPortInputBaudRateParamTypeId).toInt());
            serialPort->setDataBits(QSerialPort::DataBits(device->paramValue(serialPortInputDataBitsParamTypeId).toInt()));
            serialPort->setParity(QSerialPort::Parity(device->paramValue(serialPortInputParityParamTypeId).toInt()));
            serialPort->setStopBits(QSerialPort::StopBits(device->paramValue(serialPortInputStopBitsParamTypeId).toInt()));

            if (!serialPort->open(QIODevice::ReadWrite)) {
                qCWarning(dcSerialPortCommander()) << "Could not open serial port" << interface << serialPort->errorString();
                return DeviceManager::DeviceSetupStatusFailure;
            }
            qCDebug(dcSerialPortCommander()) << "Setup successfully serial port" << interface;
            SerialPortCommander *serialPortCommander = new SerialPortCommander(serialPort, this);
            connect(serialPortCommander, SIGNAL(commandReceived(Device *)), this, SLOT(onCommandReceived(Device *)));
            serialPortCommander->addInputDevice(device);
            m_serialPortCommanders.insert(interface, serialPortCommander);

        } else {
            SerialPortCommander *serialPortCommander = m_serialPortCommanders.value(interface);
            connect(serialPortCommander, SIGNAL(commandReceived(Device *)), this, SLOT(onCommandReceived(Device *)));
            serialPortCommander->addInputDevice(device);
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
        if (m_serialPortCommanders.contains(port.portName())){
            SerialPortCommander *serialPortCommander = m_serialPortCommanders.value(port.portName());
            QSerialPort *serialPort = serialPortCommander->serialPort();
            QString description = "Note, this serial port is also used by another device";
            DeviceDescriptor descriptor(deviceClassId, serialPort->portName(), description);
            ParamList parameters;

            if (deviceClassId == serialPortInputDeviceClassId) {
                //take the params from the already existing in/output device
                parameters.append(Param(serialPortInputSerialPortParamTypeId, serialPort->portName()));
                parameters.append(Param(serialPortInputBaudRateParamTypeId, serialPort->baudRate()));
                parameters.append(Param(serialPortInputDataBitsParamTypeId, serialPort->dataBits()));
                parameters.append(Param(serialPortInputFlowControlParamTypeId, serialPort->flowControl()));
                parameters.append(Param(serialPortInputStopBitsParamTypeId, serialPort->stopBits()));
                parameters.append(Param(serialPortInputParityParamTypeId, serialPort->parity()));
            }

            if (deviceClassId == serialPortOutputDeviceClassId) {
                if (serialPortCommander->hasOutputDevice()){
                    //only one output per port is allowed
                    continue;
                }
                //take the params from the already existing input device
                parameters.append(Param(serialPortOutputSerialPortParamTypeId, serialPort->portName()));
                parameters.append(Param(serialPortOutputBaudRateParamTypeId, serialPort->baudRate()));
                parameters.append(Param(serialPortOutputDataBitsParamTypeId, serialPort->dataBits()));
                parameters.append(Param(serialPortOutputFlowControlParamTypeId, serialPort->flowControl()));
                parameters.append(Param(serialPortOutputStopBitsParamTypeId, serialPort->stopBits()));
                parameters.append(Param(serialPortOutputParityParamTypeId, serialPort->parity()));
            }

            descriptor.setParams(parameters);
            deviceDescriptors.append(descriptor);
        } else {
            //Serial port is not yet used, create now a new one
            qCDebug(dcSerialPortCommander()) << "Found serial port:" << port.portName();
            QString description = port.manufacturer() + " " + port.description();
            DeviceDescriptor descriptor(deviceClassId, port.portName(), description);
            ParamList parameters;

            if (deviceClassId == serialPortInputDeviceClassId) {
                parameters.append(Param(serialPortInputSerialPortParamTypeId, port.portName()));
            }

            if (deviceClassId == serialPortOutputDeviceClassId) {
                parameters.append(Param(serialPortOutputSerialPortParamTypeId, port.portName()));
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
    if (device->deviceClassId() == serialPortOutputDeviceClassId ) {

        if (action.actionTypeId() == serialPortOutputTriggerActionTypeId) {

            QString interface = device->paramValue(serialPortInputSerialPortParamTypeId).toString();
            SerialPortCommander *serialPortCommander = m_serialPortCommanders.value(interface);
            serialPortCommander->sendCommand(action.param(serialPortOutputOutputDataAreaParamTypeId).value().toByteArray());

            return DeviceManager::DeviceErrorNoError;
        }
        return DeviceManager::DeviceErrorActionTypeNotFound;
    }
    return DeviceManager::DeviceErrorDeviceClassNotFound;
}


void DevicePluginSerialPortCommander::deviceRemoved(Device *device)
{
    QString interface;
    SerialPortCommander *serialPortCommander;

    if (device->deviceClassId() == serialPortInputDeviceClassId) {

        interface = device->paramValue(serialPortInputSerialPortParamTypeId).toString();
        serialPortCommander = m_serialPortCommanders.value(interface);
        serialPortCommander->removeInputDevice(device);
        if (serialPortCommander->isEmpty()) {
            m_serialPortCommanders.remove(interface);
            serialPortCommander->deleteLater();
        }
    }

    if (device->deviceClassId() == serialPortOutputDeviceClassId) {
        interface = device->paramValue(serialPortOutputSerialPortParamTypeId).toString();
        serialPortCommander = m_serialPortCommanders.value(interface);
        serialPortCommander->removeOutputDevice();
        if (serialPortCommander->isEmpty()) {
            m_serialPortCommanders.remove(interface);
            serialPortCommander->deleteLater();
        }
    }
}


void DevicePluginSerialPortCommander::onCommandReceived(Device *device)
{
    emitEvent(Event(serialPortInputTriggeredEventTypeId, device->id()));
}
