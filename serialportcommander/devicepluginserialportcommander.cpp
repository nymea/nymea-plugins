/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*
* Copyright 2013 - 2020, nymea GmbH
* Contact: contact@nymea.io
*
* This file is part of nymea.
* This project including source code and documentation is protected by
* copyright law, and remains the property of nymea GmbH. All rights, including
* reproduction, publication, editing and translation, are reserved. The use of
* this project is subject to the terms of a license agreement to be concluded
* with nymea GmbH in accordance with the terms of use of nymea GmbH, available
* under https://nymea.io/license
*
* GNU Lesser General Public License Usage
* Alternatively, this project may be redistributed and/or modified under the
* terms of the GNU Lesser General Public License as published by the Free
* Software Foundation; version 3. This project is distributed in the hope that
* it will be useful, but WITHOUT ANY WARRANTY; without even the implied
* warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
* Lesser General Public License for more details.
*
* You should have received a copy of the GNU Lesser General Public License
* along with this project. If not, see <https://www.gnu.org/licenses/>.
*
* For any further details and any questions please contact us under
* contact@nymea.io or see our FAQ/Licensing Information on
* https://nymea.io/license/faq
*
* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

#include "devicepluginserialportcommander.h"
#include "plugininfo.h"

DevicePluginSerialPortCommander::DevicePluginSerialPortCommander()
{
}


void DevicePluginSerialPortCommander::discoverDevices(DeviceDiscoveryInfo *info)
{
    // Create the list of available serial interfaces

    foreach(QSerialPortInfo port, QSerialPortInfo::availablePorts()) {

        qCDebug(dcSerialPortCommander()) << "Found serial port:" << port.portName();
        QString description = port.manufacturer() + " " + port.description();
        DeviceDescriptor deviceDescriptor(info->deviceClassId(), port.portName(), description);
        ParamList parameters;
        foreach (Device *existingDevice, myDevices()) {
            if (existingDevice->paramValue(serialPortCommanderDeviceSerialPortParamTypeId).toString() == port.portName()) {
                deviceDescriptor.setDeviceId(existingDevice->id());
                break;
            }
        }
        parameters.append(Param(serialPortCommanderDeviceSerialPortParamTypeId, port.portName()));
        deviceDescriptor.setParams(parameters);
        info->addDeviceDescriptor(deviceDescriptor);
    }

    info->finish(Device::DeviceErrorNoError);
}


void DevicePluginSerialPortCommander::setupDevice(DeviceSetupInfo *info)
{
    Device *device = info->device();

    if(!m_reconnectTimer) {
        m_reconnectTimer = new QTimer(this);
        m_reconnectTimer->setSingleShot(true);
        m_reconnectTimer->setInterval(5000);

        connect(m_reconnectTimer, &QTimer::timeout, this, &DevicePluginSerialPortCommander::onReconnectTimer);
    }

    if (device->deviceClassId() == serialPortCommanderDeviceClassId) {
        QString interface = device->paramValue(serialPortCommanderDeviceSerialPortParamTypeId).toString();
        QSerialPort *serialPort = new QSerialPort(interface, this);

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
            serialPort->deleteLater();
            //: Error setting up device
            return info->finish(Device::DeviceErrorHardwareFailure, QT_TR_NOOP("Could not open serial port."));
        }

        connect(serialPort, SIGNAL(error(QSerialPort::SerialPortError)), this, SLOT(onSerialError(QSerialPort::SerialPortError)));
        connect(serialPort, SIGNAL(readyRead()), this, SLOT(onReadyRead()));
        connect(serialPort, SIGNAL(baudRateChanged(qint32, QSerialPort::Directions)), this, SLOT(onBaudRateChanged(qint32, QSerialPort::Directions)));
        connect(serialPort, SIGNAL(parityChanged(QSerialPort::Parity)), this, SLOT(onParityChanged(QSerialPort::Parity)));
        connect(serialPort, SIGNAL(dataBitsChanged(QSerialPort::DataBits)), this, SLOT(onDataBitsChanged(QSerialPort::DataBits)));
        connect(serialPort, SIGNAL(stopBitsChanged(QSerialPort::StopBits)), this, SLOT(onStopBitsChanged(QSerialPort::StopBits)));
        connect(serialPort, SIGNAL(flowControlChanged(QSerialPort::FlowControl)), this, SLOT(onFlowControlChanged(QSerialPort::FlowControl)));
        m_serialPorts.insert(device, serialPort);
        device->setStateValue(serialPortCommanderConnectedStateTypeId, true);
    }
    return info->finish(Device::DeviceErrorNoError);
}


void DevicePluginSerialPortCommander::executeAction(DeviceActionInfo *info)
{
    Device *device = info->device();
    Action action = info->action();


    if (action.actionTypeId() == serialPortCommanderTriggerActionTypeId) {

        QSerialPort *serialPort = m_serialPorts.value(device);
        qint64 size = serialPort->write(action.param(serialPortCommanderTriggerActionOutputDataParamTypeId).value().toByteArray());
        if(size != action.param(serialPortCommanderTriggerActionOutputDataParamTypeId).value().toByteArray().length()) {
            return info->finish(Device::DeviceErrorHardwareFailure, QT_TR_NOOP("Error writing to serial port."));
        }
        return info->finish(Device::DeviceErrorNoError);
    }
    info->finish(Device::DeviceErrorActionTypeNotFound);
}


void DevicePluginSerialPortCommander::deviceRemoved(Device *device)
{
    if (device->deviceClassId() == serialPortCommanderDeviceClassId) {

        QSerialPort *serialPort = m_serialPorts.take(device);
        if (serialPort) {
            if (serialPort->isOpen()){
                serialPort->flush();
                serialPort->close();
            }
            serialPort->deleteLater();
        }
    }

    if (myDevices().empty()) {
        m_reconnectTimer->stop();
        m_reconnectTimer->deleteLater();
    }
}

void DevicePluginSerialPortCommander::onReadyRead()
{
    QSerialPort *serialPort =  static_cast<QSerialPort*>(sender());
    Device *device = m_serialPorts.key(serialPort);

    QByteArray data;
    while (!serialPort->atEnd()) {
        data.append(serialPort->read(100));
    }
    qDebug(dcSerialPortCommander()) << "Message received" << data;

    Event event(serialPortCommanderTriggeredEventTypeId, device->id());
    ParamList parameters;
    parameters.append(Param(serialPortCommanderTriggeredEventInputDataParamTypeId, data));
    event.setParams(parameters);
    emitEvent(event);
}

void DevicePluginSerialPortCommander::onSerialError(QSerialPort::SerialPortError error)
{
    QSerialPort *serialPort =  static_cast<QSerialPort*>(sender());
    Device *device = m_serialPorts.key(serialPort);

    if (error != QSerialPort::NoError && serialPort->isOpen()) {
        qCCritical(dcSerialPortCommander()) << "Serial port error:" << error << serialPort->errorString();
        m_reconnectTimer->start();
        serialPort->close();
        device->setStateValue(serialPortCommanderConnectedStateTypeId, false);
    }
}

void DevicePluginSerialPortCommander::onBaudRateChanged(qint32 baudRate, QSerialPort::Directions direction)
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

void DevicePluginSerialPortCommander::onReconnectTimer()
{
    foreach(Device *device, myDevices()) {
        if (!device->stateValue(serialPortCommanderConnectedStateTypeId).toBool()) {
            QSerialPort *serialPort =  m_serialPorts.value(device);
            if (serialPort) {
                if (serialPort->open(QSerialPort::ReadWrite)) {
                    device->setStateValue(serialPortCommanderConnectedStateTypeId, true);
                } else {
                    device->setStateValue(serialPortCommanderConnectedStateTypeId, false);
                    m_reconnectTimer->start();
                }
            }
        }
    }
}

