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
    \page modbuscommander.html
    \title Modbus Commander
    \brief Plugin to write and read Modbus TCP coils and registers.

    \ingroup plugins
    \ingroup nymea-plugins


    This plugin allows to write and read modbus registers and coils.

    \underline{NOTE}: the library \c libmodbus has to be installed.

    \chapter Plugin properties
    Following JSON file contains the definition and the description of all available \l{DeviceClass}{DeviceClasses}
    and \l{Vendor}{Vendors} of this \l{DevicePlugin}.

    For more details how to read this JSON file please check out the documentation for \l{The plugin JSON File}.

    \quotefile plugins/deviceplugins/modbuscommander/devicepluginmodbuscommander.json
*/

#include "devicepluginmodbuscommander.h"
#include "plugininfo.h"

#include <QSerialPort>

DevicePluginModbusCommander::DevicePluginModbusCommander()
{
}

void DevicePluginModbusCommander::init()
{
    connect(this, &DevicePluginModbusCommander::configValueChanged, this, &DevicePluginModbusCommander::onPluginConfigurationChanged);
}


Device::DeviceSetupStatus DevicePluginModbusCommander::setupDevice(Device *device)
{
    if (device->deviceClassId() == modbusTCPClientDeviceClassId) {
        QString ipAddress = device->paramValue(modbusTCPClientDeviceIpv4addressParamTypeId).toString();
        int port = device->paramValue(modbusTCPClientDevicePortParamTypeId).toInt();

        foreach (ModbusTCPMaster *modbusTCPMaster, m_modbusTCPMasters.values()) {
            if ((modbusTCPMaster->ipv4Address() == ipAddress) && (modbusTCPMaster->port() == port)){
                m_modbusTCPMasters.insert(device, modbusTCPMaster);
                return Device::DeviceSetupStatusSuccess;
            }
        }

        ModbusTCPMaster *modbusTCPMaster = new ModbusTCPMaster(ipAddress, port, this);
        connect(modbusTCPMaster, &ModbusTCPMaster::connectionStateChanged, this, &DevicePluginModbusCommander::onConnectionStateChanged);

        connect(modbusTCPMaster, &ModbusTCPMaster::receivedCoil, this, &DevicePluginModbusCommander::onReceivedCoil);
        connect(modbusTCPMaster, &ModbusTCPMaster::receivedDiscreteInput, this, &DevicePluginModbusCommander::onReceivedDiscreteInput);
        connect(modbusTCPMaster, &ModbusTCPMaster::receivedHoldingRegister, this, &DevicePluginModbusCommander::onReceivedHoldingRegister);
        connect(modbusTCPMaster, &ModbusTCPMaster::receivedInputRegister, this, &DevicePluginModbusCommander::onReceivedInputRegister);

        m_modbusTCPMasters.insert(device, modbusTCPMaster);
        return Device::DeviceSetupStatusSuccess;
    }


    if (device->deviceClassId() == modbusRTUClientDeviceClassId) {

        QString serialPort = device->paramValue(modbusRTUClientDeviceSerialPortParamTypeId).toString();
        int baudrate = device->paramValue(modbusRTUClientDeviceBaudRateParamTypeId).toInt();
        int stopBits = device->paramValue(modbusRTUClientDeviceStopBitsParamTypeId).toInt();
        int dataBits = device->paramValue(modbusRTUClientDeviceDataBitsParamTypeId).toInt();
        QSerialPort::Parity parity = QSerialPort::Parity::NoParity;
        if (device->paramValue(modbusRTUClientDeviceParityParamTypeId).toString().contains("No")) {
            parity = QSerialPort::Parity::NoParity;
        } else if (device->paramValue(modbusRTUClientDeviceParityParamTypeId).toString().contains("Even")) {
            parity = QSerialPort::Parity::EvenParity;
        } else if (device->paramValue(modbusRTUClientDeviceParityParamTypeId).toString().contains("Odd")) {
            parity = QSerialPort::Parity::OddParity;
        }

        ModbusRTUMaster *modbusRTUMaster = new ModbusRTUMaster(serialPort, baudrate, parity, dataBits, stopBits, this);
        connect(modbusRTUMaster, &ModbusRTUMaster::connectionStateChanged, this, &DevicePluginModbusCommander::onConnectionStateChanged);

        connect(modbusRTUMaster, &ModbusRTUMaster::receivedCoil, this, &DevicePluginModbusCommander::onReceivedCoil);
        connect(modbusRTUMaster, &ModbusRTUMaster::receivedDiscreteInput, this, &DevicePluginModbusCommander::onReceivedDiscreteInput);
        connect(modbusRTUMaster, &ModbusRTUMaster::receivedHoldingRegister, this, &DevicePluginModbusCommander::onReceivedHoldingRegister);
        connect(modbusRTUMaster, &ModbusRTUMaster::receivedInputRegister, this, &DevicePluginModbusCommander::onReceivedInputRegister);

        m_modbusRTUMasters.insert(device, modbusRTUMaster);
        return Device::DeviceSetupStatusSuccess;
    }

    if (device->deviceClassId() == coilDeviceClassId) {

        device->setParentId(device->paramValue(coilDeviceParentIdParamTypeId).toString());
        return Device::DeviceSetupStatusSuccess;
    }

    if (device->deviceClassId() == discreteInputDeviceClassId) {

        device->setParentId(device->paramValue(discreteInputDeviceParentIdParamTypeId).toString());
        return Device::DeviceSetupStatusSuccess;
    }

    if (device->deviceClassId() == holdingRegisterDeviceClassId) {
        device->setParentId(device->paramValue(holdingRegisterDeviceParentIdParamTypeId).toString());
        return Device::DeviceSetupStatusSuccess;
    }

    if (device->deviceClassId() == inputRegisterDeviceClassId) {
        device->setParentId(device->paramValue(inputRegisterDeviceParentIdParamTypeId).toString());
        return Device::DeviceSetupStatusSuccess;
    }
    return Device::DeviceSetupStatusFailure;
}

Device::DeviceError DevicePluginModbusCommander::discoverDevices(const DeviceClassId &deviceClassId, const ParamList &params)
{
    Q_UNUSED(params)
    // Create the list of available serial Clients
    QList<DeviceDescriptor> deviceDescriptors;

    if (deviceClassId == modbusRTUClientDeviceClassId) {
        Q_FOREACH(QSerialPortInfo port, QSerialPortInfo::availablePorts()) {
            //Serial port is not yet used, create now a new one
            qCDebug(dcModbusCommander()) << "Found serial port:" << port.systemLocation();
            QString description = port.manufacturer() + " " + port.description();
            DeviceDescriptor deviceDescriptor(deviceClassId, port.portName(), description);
            ParamList parameters;
            QString serialPort = port.systemLocation();
            foreach (Device *existingDevice, myDevices()) {
                if (existingDevice->paramValue(modbusRTUClientDeviceSerialPortParamTypeId).toString() == serialPort) {
                    deviceDescriptor.setDeviceId(existingDevice->id());
                    break;
                }
            }
            parameters.append(Param(modbusRTUClientDeviceSerialPortParamTypeId, serialPort));
            deviceDescriptor.setParams(parameters);
            deviceDescriptors.append(deviceDescriptor);
        }
        emit devicesDiscovered(deviceClassId, deviceDescriptors);
        return Device::DeviceErrorAsync;
    }

    if (deviceClassId == discreteInputDeviceClassId) {
        Q_FOREACH(Device *ClientDevice, myDevices()){
            if (ClientDevice->deviceClassId() == modbusTCPClientDeviceClassId) {
                DeviceDescriptor descriptor(deviceClassId, ClientDevice->name(), ClientDevice->paramValue(modbusTCPClientDeviceIpv4addressParamTypeId).toString() + "Port: " + ClientDevice->paramValue(modbusTCPClientDevicePortParamTypeId).toString());
                ParamList parameters;
                parameters.append(Param(discreteInputDeviceParentIdParamTypeId, ClientDevice->id()));
                descriptor.setParams(parameters);
                deviceDescriptors.append(descriptor);
            }
            if (ClientDevice->deviceClassId() == modbusRTUClientDeviceClassId) {
                DeviceDescriptor descriptor(deviceClassId, ClientDevice->name(), ClientDevice->paramValue(modbusRTUClientDeviceSerialPortParamTypeId).toString());
                ParamList parameters;
                parameters.append(Param(discreteInputDeviceParentIdParamTypeId, ClientDevice->id()));
                descriptor.setParams(parameters);
                deviceDescriptors.append(descriptor);
            }
        }
        emit devicesDiscovered(deviceClassId, deviceDescriptors);
        return Device::DeviceErrorAsync;
    }

    if (deviceClassId == coilDeviceClassId) {
        Q_FOREACH(Device *ClientDevice, myDevices()){
            if (ClientDevice->deviceClassId() == modbusTCPClientDeviceClassId) {
                DeviceDescriptor descriptor(deviceClassId, ClientDevice->name(), ClientDevice->paramValue(modbusTCPClientDeviceIpv4addressParamTypeId).toString() + "Port: " + ClientDevice->paramValue(modbusTCPClientDevicePortParamTypeId).toString());
                ParamList parameters;
                parameters.append(Param(coilDeviceParentIdParamTypeId, ClientDevice->id()));
                descriptor.setParams(parameters);
                deviceDescriptors.append(descriptor);
            }
            if (ClientDevice->deviceClassId() == modbusRTUClientDeviceClassId) {
                DeviceDescriptor descriptor(deviceClassId, ClientDevice->name(), ClientDevice->paramValue(modbusRTUClientDeviceSerialPortParamTypeId).toString());
                ParamList parameters;
                parameters.append(Param(coilDeviceParentIdParamTypeId, ClientDevice->id()));
                descriptor.setParams(parameters);
                deviceDescriptors.append(descriptor);
            }
        }
        emit devicesDiscovered(deviceClassId, deviceDescriptors);
        return Device::DeviceErrorAsync;
    }

    if (deviceClassId == holdingRegisterDeviceClassId) {
        Q_FOREACH(Device *ClientDevice, myDevices()){
            if (ClientDevice->deviceClassId() == modbusTCPClientDeviceClassId) {
                DeviceDescriptor descriptor(deviceClassId, ClientDevice->name(), ClientDevice->paramValue(modbusTCPClientDeviceIpv4addressParamTypeId).toString() + "Port: " + ClientDevice->paramValue(modbusTCPClientDevicePortParamTypeId).toString());
                ParamList parameters;
                parameters.append(Param(holdingRegisterDeviceParentIdParamTypeId, ClientDevice->id()));
                descriptor.setParams(parameters);
                deviceDescriptors.append(descriptor);
            }
            if (ClientDevice->deviceClassId() == modbusRTUClientDeviceClassId) {
                DeviceDescriptor descriptor(deviceClassId, ClientDevice->name(), ClientDevice->paramValue(modbusRTUClientDeviceSerialPortParamTypeId).toString());
                ParamList parameters;
                parameters.append(Param(holdingRegisterDeviceParentIdParamTypeId, ClientDevice->id()));
                descriptor.setParams(parameters);
                deviceDescriptors.append(descriptor);
            }
        }
        emit devicesDiscovered(deviceClassId, deviceDescriptors);
        return Device::DeviceErrorAsync;
    }

    if (deviceClassId == inputRegisterDeviceClassId) {
        Q_FOREACH(Device *ClientDevice, myDevices()){
            if (ClientDevice->deviceClassId() == modbusTCPClientDeviceClassId) {
                DeviceDescriptor descriptor(deviceClassId, ClientDevice->name(), ClientDevice->paramValue(modbusTCPClientDeviceIpv4addressParamTypeId).toString() + "Port: " + ClientDevice->paramValue(modbusTCPClientDevicePortParamTypeId).toString());
                ParamList parameters;
                parameters.append(Param(inputRegisterDeviceParentIdParamTypeId, ClientDevice->id()));
                descriptor.setParams(parameters);
                deviceDescriptors.append(descriptor);
            }
            if (ClientDevice->deviceClassId() == modbusRTUClientDeviceClassId) {
                DeviceDescriptor descriptor(deviceClassId, ClientDevice->name(), ClientDevice->paramValue(modbusRTUClientDeviceSerialPortParamTypeId).toString());
                ParamList parameters;
                parameters.append(Param(inputRegisterDeviceParentIdParamTypeId, ClientDevice->id()));
                descriptor.setParams(parameters);
                deviceDescriptors.append(descriptor);
            }
        }
        emit devicesDiscovered(deviceClassId, deviceDescriptors);
        return Device::DeviceErrorAsync;
    }
    return Device::DeviceErrorDeviceClassNotFound;
}

void DevicePluginModbusCommander::postSetupDevice(Device *device)
{
    if (!m_refreshTimer) {
        // Refresh timer for TCP read
        int refreshTime = configValue(modbusCommanderPluginUpdateIntervalParamTypeId).toInt();
        m_refreshTimer = hardwareManager()->pluginTimerManager()->registerTimer(refreshTime);
        connect(m_refreshTimer, &PluginTimer::timeout, this, &DevicePluginModbusCommander::onRefreshTimer);
    }

    if ((device->deviceClassId() == coilDeviceClassId) ||
            (device->deviceClassId() == discreteInputDeviceClassId) ||
            (device->deviceClassId() == holdingRegisterDeviceClassId) ||
            (device->deviceClassId() == inputRegisterDeviceClassId)) {
        readRegister(device);
    }
}


Device::DeviceError DevicePluginModbusCommander::executeAction(Device *device, const Action &action)
{
    if (device->deviceClassId() == coilDeviceClassId) {

        if (action.actionTypeId() == coilValueActionTypeId) {
            writeRegister(device, action);

            return Device::DeviceErrorNoError;
        }
        return Device::DeviceErrorActionTypeNotFound;
    }

    if (device->deviceClassId() == holdingRegisterDeviceClassId) {

        if (action.actionTypeId() == holdingRegisterValueActionTypeId) {
            writeRegister(device, action);
            return Device::DeviceErrorNoError;
        }
        return Device::DeviceErrorActionTypeNotFound;
    }
    return Device::DeviceErrorDeviceClassNotFound;
}


void DevicePluginModbusCommander::deviceRemoved(Device *device)
{
    if (device->deviceClassId() == modbusTCPClientDeviceClassId) {
        ModbusTCPMaster *modbus = m_modbusTCPMasters.take(device);
        modbus->deleteLater();
    }

    if (device->deviceClassId() == modbusRTUClientDeviceClassId) {
        ModbusRTUMaster *modbus = m_modbusRTUMasters.take(device);
        modbus->deleteLater();
    }

    if (myDevices().empty()) {
        m_refreshTimer->stop();
        hardwareManager()->pluginTimerManager()->unregisterTimer(m_refreshTimer);
    }
}

void DevicePluginModbusCommander::onRefreshTimer()
{
    foreach (Device *device, myDevices()) {
        if ((device->deviceClassId() == coilDeviceClassId) ||
                (device->deviceClassId() == discreteInputDeviceClassId) ||
                (device->deviceClassId() == holdingRegisterDeviceClassId) ||
                (device->deviceClassId() == inputRegisterDeviceClassId)) {
            readRegister(device);
        }
    }
}

void DevicePluginModbusCommander::onPluginConfigurationChanged(const ParamTypeId &paramTypeId, const QVariant &value)
{
    // Check refresh schedule
    if (paramTypeId == modbusCommanderPluginUpdateIntervalParamTypeId) {;
        if (m_refreshTimer) {
            int refreshTime = value.toInt();
            m_refreshTimer->stop();
            m_refreshTimer->startTimer(refreshTime);
        }
    }
}

void DevicePluginModbusCommander::onConnectionStateChanged(bool status)
{
    auto modbus = sender();

    if (m_modbusRTUMasters.values().contains(static_cast<ModbusRTUMaster *>(modbus))) {
        Device *device = m_modbusRTUMasters.key(static_cast<ModbusRTUMaster *>(modbus));
        device->setStateValue(modbusRTUClientConnectedStateTypeId, status);
    }

    if (m_modbusTCPMasters.values().contains(static_cast<ModbusTCPMaster *>(modbus))) {
        Device *device = m_modbusTCPMasters.key(static_cast<ModbusTCPMaster *>(modbus));
        device->setStateValue(modbusTCPClientConnectedStateTypeId, status);
    }
}

void DevicePluginModbusCommander::onReceivedCoil(int slaveAddress, int modbusRegister, bool value)
{
    auto modbus = sender();

    if (m_modbusRTUMasters.values().contains(static_cast<ModbusRTUMaster *>(modbus))) {
        Device *parentDevice = m_modbusRTUMasters.key(static_cast<ModbusRTUMaster *>(modbus));
        foreach(Device *device, myDevices().filterByParam(coilDeviceParentIdParamTypeId, parentDevice->id())) {
            if ((device->paramValue(coilDeviceSlaveAddressParamTypeId) == slaveAddress) && (device->paramValue(coilDeviceRegisterAddressParamTypeId) == modbusRegister))
                device->setStateValue(coilValueStateTypeId, value);
        }
    }

    if (m_modbusTCPMasters.values().contains(static_cast<ModbusTCPMaster *>(modbus))) {
        Device *parentDevice = m_modbusTCPMasters.key(static_cast<ModbusTCPMaster *>(modbus));
        foreach(Device *device, myDevices().filterByParam(coilDeviceParentIdParamTypeId, parentDevice->id())) {
            if ((device->paramValue(coilDeviceSlaveAddressParamTypeId) == slaveAddress) && (device->paramValue(coilDeviceRegisterAddressParamTypeId) == modbusRegister))
                device->setStateValue(coilValueStateTypeId, value);
        }
    }
}

void DevicePluginModbusCommander::onReceivedDiscreteInput(int slaveAddress, int modbusRegister, bool value)
{
    auto modbus = sender();

    if (m_modbusRTUMasters.values().contains(static_cast<ModbusRTUMaster *>(modbus))) {
        Device *parentDevice = m_modbusRTUMasters.key(static_cast<ModbusRTUMaster *>(modbus));
        foreach(Device *device, myDevices().filterByParam(discreteInputDeviceParentIdParamTypeId, parentDevice->id())) {
            if ((device->paramValue(discreteInputDeviceSlaveAddressParamTypeId) == slaveAddress) && (device->paramValue(discreteInputDeviceRegisterAddressParamTypeId) == modbusRegister))
                device->setStateValue(discreteInputValueStateTypeId, value);
        }
    }

    if (m_modbusTCPMasters.values().contains(static_cast<ModbusTCPMaster *>(modbus))) {
        Device *parentDevice = m_modbusTCPMasters.key(static_cast<ModbusTCPMaster *>(modbus));
        foreach(Device *device, myDevices().filterByParam(discreteInputDeviceParentIdParamTypeId, parentDevice->id())) {
            if ((device->paramValue(discreteInputDeviceSlaveAddressParamTypeId) == slaveAddress) && (device->paramValue(discreteInputDeviceRegisterAddressParamTypeId) == modbusRegister))
                device->setStateValue(discreteInputValueStateTypeId, value);
        }
    }
}

void DevicePluginModbusCommander::onReceivedHoldingRegister(int slaveAddress, int modbusRegister, int value)
{
    auto modbus = sender();

    if (m_modbusRTUMasters.values().contains(static_cast<ModbusRTUMaster *>(modbus))) {
        Device *parentDevice = m_modbusRTUMasters.key(static_cast<ModbusRTUMaster *>(modbus));
        foreach(Device *device, myDevices().filterByParam(holdingRegisterDeviceParentIdParamTypeId, parentDevice->id())) {
            if ((device->paramValue(holdingRegisterDeviceSlaveAddressParamTypeId) == slaveAddress) && (device->paramValue(holdingRegisterDeviceRegisterAddressParamTypeId) == modbusRegister))
                device->setStateValue(holdingRegisterValueStateTypeId, value);
        }
    }

    if (m_modbusTCPMasters.values().contains(static_cast<ModbusTCPMaster *>(modbus))) {
        Device *parentDevice = m_modbusTCPMasters.key(static_cast<ModbusTCPMaster *>(modbus));
        foreach(Device *device, myDevices().filterByParam(holdingRegisterDeviceParentIdParamTypeId, parentDevice->id())) {
            if ((device->paramValue(holdingRegisterDeviceSlaveAddressParamTypeId) == slaveAddress) && (device->paramValue(holdingRegisterDeviceRegisterAddressParamTypeId) == modbusRegister))
                device->setStateValue(holdingRegisterValueStateTypeId, value);
        }
    }
}

void DevicePluginModbusCommander::onReceivedInputRegister(int slaveAddress, int modbusRegister, int value)
{
    auto modbus = sender();

    if (m_modbusRTUMasters.values().contains(static_cast<ModbusRTUMaster *>(modbus))) {
        Device *parentDevice = m_modbusRTUMasters.key(static_cast<ModbusRTUMaster *>(modbus));
        foreach(Device *device, myDevices().filterByParam(inputRegisterDeviceParentIdParamTypeId, parentDevice->id())) {
            if ((device->paramValue(inputRegisterDeviceSlaveAddressParamTypeId) == slaveAddress) && (device->paramValue(inputRegisterDeviceRegisterAddressParamTypeId) == modbusRegister))
                device->setStateValue(inputRegisterValueStateTypeId, value);
        }
    }

    if (m_modbusTCPMasters.values().contains(static_cast<ModbusTCPMaster *>(modbus))) {
        Device *parentDevice = m_modbusTCPMasters.key(static_cast<ModbusTCPMaster *>(modbus));
        foreach(Device *device, myDevices().filterByParam(inputRegisterDeviceParentIdParamTypeId, parentDevice->id())) {
            if ((device->paramValue(inputRegisterDeviceSlaveAddressParamTypeId) == slaveAddress) && (device->paramValue(inputRegisterDeviceRegisterAddressParamTypeId) == modbusRegister))
                device->setStateValue(inputRegisterValueStateTypeId, value);
        }
    }
}

void DevicePluginModbusCommander::readRegister(Device *device)
{
    Device *parent = myDevices().findById(device->parentId());

    if (!parent)
        return;

    int registerAddress;
    int slaveAddress;

    if (parent->deviceClassId() == modbusTCPClientDeviceClassId) {

        ModbusTCPMaster *modbus = m_modbusTCPMasters.value(parent);
        if (device->deviceClassId() == coilDeviceClassId) {
            registerAddress = device->paramValue(coilDeviceRegisterAddressParamTypeId).toInt();
            slaveAddress = device->paramValue(coilDeviceSlaveAddressParamTypeId).toInt();
            modbus->readCoil(slaveAddress, registerAddress);
        }
        if (device->deviceClassId() == discreteInputDeviceClassId) {
            registerAddress = device->paramValue(discreteInputDeviceRegisterAddressParamTypeId).toInt();
            slaveAddress = device->paramValue(discreteInputDeviceSlaveAddressParamTypeId).toInt();
            modbus->readDiscreteInput(slaveAddress, registerAddress);
        }
        if (device->deviceClassId() == holdingRegisterDeviceClassId) {
            registerAddress = device->paramValue(holdingRegisterDeviceRegisterAddressParamTypeId).toInt();
            slaveAddress = device->paramValue(holdingRegisterDeviceSlaveAddressParamTypeId).toInt();
            modbus->readDiscreteInput(slaveAddress, registerAddress);
        }
        if (device->deviceClassId() == inputRegisterDeviceClassId) {
            registerAddress = device->paramValue(discreteInputDeviceRegisterAddressParamTypeId).toInt();
            slaveAddress = device->paramValue(discreteInputDeviceSlaveAddressParamTypeId).toInt();
            modbus->readDiscreteInput(slaveAddress, registerAddress);
        }
    }

    if (parent->deviceClassId() == modbusRTUClientDeviceClassId) {

        ModbusRTUMaster *modbus = m_modbusRTUMasters.value(parent);
        if (device->deviceClassId() == coilDeviceClassId) {
            registerAddress = device->paramValue(coilDeviceRegisterAddressParamTypeId).toInt();
            slaveAddress = device->paramValue(coilDeviceSlaveAddressParamTypeId).toInt();
            modbus->readCoil(slaveAddress, registerAddress);
        }
        if (device->deviceClassId() == discreteInputDeviceClassId) {
            registerAddress = device->paramValue(discreteInputDeviceRegisterAddressParamTypeId).toInt();
            slaveAddress = device->paramValue(discreteInputDeviceSlaveAddressParamTypeId).toInt();
            modbus->readDiscreteInput(slaveAddress, registerAddress);
        }
        if (device->deviceClassId() == holdingRegisterDeviceClassId) {
            registerAddress = device->paramValue(holdingRegisterDeviceRegisterAddressParamTypeId).toInt();
            slaveAddress = device->paramValue(holdingRegisterDeviceSlaveAddressParamTypeId).toInt();
            modbus->readDiscreteInput(slaveAddress, registerAddress);
        }
        if (device->deviceClassId() == inputRegisterDeviceClassId) {
            registerAddress = device->paramValue(discreteInputDeviceRegisterAddressParamTypeId).toInt();
            slaveAddress = device->paramValue(discreteInputDeviceSlaveAddressParamTypeId).toInt();
            modbus->readDiscreteInput(slaveAddress, registerAddress);
        }
    }
}

void DevicePluginModbusCommander::writeRegister(Device *device, Action action)
{
    Device *parent = myDevices().findById(device->parentId());
    int registerAddress;
    int slaveAddress;

    if (parent->deviceClassId() == modbusTCPClientDeviceClassId) {
        ModbusTCPMaster *modbus = m_modbusTCPMasters.value(parent);
        if (device->deviceClassId() == coilDeviceClassId) {
            registerAddress = device->paramValue(coilDeviceRegisterAddressParamTypeId).toInt();
            slaveAddress = device->paramValue(coilDeviceSlaveAddressParamTypeId).toInt();
            modbus->writeCoil(slaveAddress, registerAddress, action.param(coilValueActionValueParamTypeId).value().toBool());
        }
        if (device->deviceClassId() == holdingRegisterDeviceClassId) {
            registerAddress = device->paramValue(holdingRegisterDeviceRegisterAddressParamTypeId).toInt();
            slaveAddress = device->paramValue(holdingRegisterDeviceSlaveAddressParamTypeId).toInt();
            modbus->writeHoldingRegister(slaveAddress, registerAddress, action.param(holdingRegisterValueActionValueParamTypeId).value().toInt());
        }
    }

    if (parent->deviceClassId() == modbusRTUClientDeviceClassId) {
        ModbusRTUMaster *modbus = m_modbusRTUMasters.value(parent);
        if (device->deviceClassId() == coilDeviceClassId) {
            registerAddress = device->paramValue(coilDeviceRegisterAddressParamTypeId).toInt();
            slaveAddress = device->paramValue(coilDeviceSlaveAddressParamTypeId).toInt();
            modbus->writeCoil(slaveAddress, registerAddress, action.param(coilValueActionValueParamTypeId).value().toBool());
        }
        if (device->deviceClassId() == holdingRegisterDeviceClassId) {
            registerAddress = device->paramValue(holdingRegisterDeviceRegisterAddressParamTypeId).toInt();
            slaveAddress = device->paramValue(holdingRegisterDeviceSlaveAddressParamTypeId).toInt();
            modbus->writeHoldingRegister(slaveAddress, registerAddress, action.param(holdingRegisterValueActionValueParamTypeId).value().toInt());
        }
    }
}
