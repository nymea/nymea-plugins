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

    \quotefile plugins/deviceplugins/networkdetector/devicepluginnetworkdetector.json
*/

#include "devicepluginmodbuscommander.h"
#include "plugininfo.h"


DevicePluginModbusCommander::DevicePluginModbusCommander()
{
}

DevicePluginModbusCommander::~DevicePluginModbusCommander()
{
    hardwareManager()->pluginTimerManager()->unregisterTimer(m_refreshTimer);
}

void DevicePluginModbusCommander::init()
{
    // Refresh timer for TCP read
    int refreshTime = configValue(modbusCommanderPluginUpdateIntervalParamTypeId).toInt();
    m_refreshTimer = hardwareManager()->pluginTimerManager()->registerTimer(refreshTime);
    connect(m_refreshTimer, &PluginTimer::timeout, this, &DevicePluginModbusCommander::onRefreshTimer);
}


DeviceManager::DeviceSetupStatus DevicePluginModbusCommander::setupDevice(Device *device)
{
    if (device->deviceClassId() == modbusTCPInterfaceDeviceClassId) {
        QHostAddress ipAddress = QHostAddress(device->paramValue(modbusTCPInterfaceDeviceIpv4addressParamTypeId).toString());
        int port = device->paramValue(modbusTCPInterfaceDevicePortParamTypeId).toInt();

        foreach(ModbusTCPMaster *modbusTCPMaster, m_modbusTCPMasters.values())
        {
            if ((modbusTCPMaster->ipv4Address() == ipAddress) && (modbusTCPMaster->port() == port)){
                m_modbusTCPMasters.insert(device, modbusTCPMaster);
                return DeviceManager::DeviceSetupStatusSuccess;
            }
        }
        ModbusTCPMaster *modbus = new ModbusTCPMaster(ipAddress, port, this);
        m_modbusTCPMasters.insert(device, modbus);
        return DeviceManager::DeviceSetupStatusSuccess;
    }

    if (device->deviceClassId() == modbusTCPWriteDeviceClassId) {

        return DeviceManager::DeviceSetupStatusSuccess;
    }

    if (device->deviceClassId() == modbusTCPReadDeviceClassId) {

        return DeviceManager::DeviceSetupStatusSuccess;
    }

    if (device->deviceClassId() == modbusRTUInterfaceDeviceClassId) {

        QString serialPort = device->paramValue(modbusRTUInterfaceDeviceSerialPortParamTypeId).toString();
        int baudrate = device->paramValue(modbusRTUInterfaceDeviceBaudRateParamTypeId).toInt();
        int stopBits = device->paramValue(modbusRTUInterfaceDeviceStopBitsParamTypeId).toInt();
        int dataBits = device->paramValue(modbusRTUInterfaceDeviceDataBitsParamTypeId).toInt();
        QString parity;
        if (device->paramValue(modbusRTUInterfaceDeviceSerialPortParamTypeId).toString().contains("No")) {
            parity = "N";
        } else if (device->paramValue(modbusRTUInterfaceDeviceSerialPortParamTypeId).toString().contains("Even")) {
            parity = "E";
        } else if (device->paramValue(modbusRTUInterfaceDeviceSerialPortParamTypeId).toString().contains("Odd")) {
            parity = "O";
        }

        ModbusRTUMaster *modbus = new ModbusRTUMaster(serialPort, baudrate, parity, dataBits, stopBits, this);
        m_modbusRTUMasters.insert(device, modbus);
        return DeviceManager::DeviceSetupStatusSuccess;
    }

    if (device->deviceClassId() == modbusRTUWriteDeviceClassId) {

        return DeviceManager::DeviceSetupStatusSuccess;
    }

    if (device->deviceClassId() == modbusRTUReadDeviceClassId) {

        return DeviceManager::DeviceSetupStatusSuccess;
    }

    return DeviceManager::DeviceSetupStatusFailure;
}

DeviceManager::DeviceError DevicePluginModbusCommander::discoverDevices(const DeviceClassId &deviceClassId, const ParamList &params)
{
    Q_UNUSED(params)
    // Create the list of available serial interfaces
    QList<DeviceDescriptor> deviceDescriptors;

    if (deviceClassId == modbusRTUInterfaceDeviceClassId) {
        Q_FOREACH(QSerialPortInfo port, QSerialPortInfo::availablePorts()) {
            if (m_usedSerialPorts.contains(port.portName())){
                //device already in use
                qCDebug(dcModbusCommander()) << "Found serial port that is already used:" << port.portName();
            } else {
                //Serial port is not yet used, create now a new one
                qCDebug(dcModbusCommander()) << "Found serial port:" << port.portName();
                QString description = port.manufacturer() + " " + port.description();
                DeviceDescriptor descriptor(deviceClassId, port.portName(), description);
                ParamList parameters;
                parameters.append(Param(modbusRTUInterfaceDeviceSerialPortParamTypeId, port.portName()));
                descriptor.setParams(parameters);
                deviceDescriptors.append(descriptor);
            }
        }
        emit devicesDiscovered(deviceClassId, deviceDescriptors);
        return DeviceManager::DeviceErrorAsync;
    }

    if (deviceClassId == modbusRTUWriteDeviceClassId) {
        Q_FOREACH(Device *interfaceDevice, myDevices()){
            if (interfaceDevice->deviceClassId() == modbusRTUInterfaceDeviceClassId) {
                DeviceDescriptor descriptor(deviceClassId, interfaceDevice->name(), interfaceDevice->paramValue(modbusRTUInterfaceDeviceSerialPortParamTypeId).toString());
                ParamList parameters;
                parameters.append(Param(modbusRTUWriteDeviceModbusRTUInterfaceParamTypeId, interfaceDevice->name()));
                descriptor.setParams(parameters);
                deviceDescriptors.append(descriptor);
            }
        }
        emit devicesDiscovered(deviceClassId, deviceDescriptors);
        return DeviceManager::DeviceErrorAsync;
    }

    if (deviceClassId == modbusRTUReadDeviceClassId) {
        Q_FOREACH(Device *interfaceDevice, myDevices()){
            if (interfaceDevice->deviceClassId() == modbusRTUInterfaceDeviceClassId) {
                DeviceDescriptor descriptor(deviceClassId, interfaceDevice->name(), interfaceDevice->paramValue(modbusRTUInterfaceDeviceSerialPortParamTypeId).toString());
                ParamList parameters;
                parameters.append(Param(modbusRTUReadDeviceModbusRTUInterfaceParamTypeId, interfaceDevice->name()));
                descriptor.setParams(parameters);
                deviceDescriptors.append(descriptor);
            }
        }
        emit devicesDiscovered(deviceClassId, deviceDescriptors);
        return DeviceManager::DeviceErrorAsync;
    }

    if (deviceClassId == modbusTCPReadDeviceClassId) {
        Q_FOREACH(Device *interfaceDevice, myDevices()){
            if (interfaceDevice->deviceClassId() == modbusTCPInterfaceDeviceClassId) {
                DeviceDescriptor descriptor(deviceClassId, interfaceDevice->name(), interfaceDevice->paramValue(modbusTCPInterfaceDeviceIpv4addressParamTypeId).toString() + "Port: " + interfaceDevice->paramValue(modbusTCPInterfaceDevicePortParamTypeId).toString());
                ParamList parameters;
                parameters.append(Param(modbusTCPReadDeviceModbusTCPInterfaceParamTypeId, interfaceDevice->name()));
                descriptor.setParams(parameters);
                deviceDescriptors.append(descriptor);
            }
        }
        emit devicesDiscovered(deviceClassId, deviceDescriptors);
        return DeviceManager::DeviceErrorAsync;
    }

    if (deviceClassId == modbusTCPWriteDeviceClassId) {
        Q_FOREACH(Device *interfaceDevice, myDevices()){
            if (interfaceDevice->deviceClassId() == modbusTCPInterfaceDeviceClassId) {
                DeviceDescriptor descriptor(deviceClassId, interfaceDevice->name(), interfaceDevice->paramValue(modbusRTUInterfaceDeviceSerialPortParamTypeId).toString());
                ParamList parameters;
                parameters.append(Param(modbusTCPWriteDeviceModbusTCPInterfaceParamTypeId, interfaceDevice->name()));
                descriptor.setParams(parameters);
                deviceDescriptors.append(descriptor);
            }
        }
        emit devicesDiscovered(deviceClassId, deviceDescriptors);
        return DeviceManager::DeviceErrorAsync;
    }
    return DeviceManager::DeviceErrorDeviceClassNotFound;
}

void DevicePluginModbusCommander::postSetupDevice(Device *device)
{
    if ((device->deviceClassId() == modbusTCPReadDeviceClassId) || (device->deviceClassId() == modbusRTUReadDeviceClassId)) {
        readData(device);
    }

    if ((device->deviceClassId() == modbusTCPWriteDeviceClassId) || (device->deviceClassId() == modbusRTUWriteDeviceClassId)){
        readData(device);
    }
}


DeviceManager::DeviceError DevicePluginModbusCommander::executeAction(Device *device, const Action &action)
{
    if (device->deviceClassId() == modbusTCPWriteDeviceClassId) {

        if (action.actionTypeId() == modbusTCPWriteDataActionTypeId) {
            writeData(device, action);
            return DeviceManager::DeviceErrorNoError;
        }
        return DeviceManager::DeviceErrorActionTypeNotFound;
    }
    if (device->deviceClassId() == modbusRTUWriteDeviceClassId) {

        if (action.actionTypeId() == modbusRTUWriteDataActionTypeId) {
            writeData(device, action);
            return DeviceManager::DeviceErrorNoError;
        }
        return DeviceManager::DeviceErrorActionTypeNotFound;
    }
    return DeviceManager::DeviceErrorDeviceClassNotFound;
}


void DevicePluginModbusCommander::deviceRemoved(Device *device)
{
    if (device->deviceClassId() == modbusTCPInterfaceDeviceClassId) {
        ModbusTCPMaster *modbus = m_modbusTCPMasters.take(device);
        modbus->deleteLater();
    }

    if (device->deviceClassId() == modbusRTUInterfaceDeviceClassId) {
        m_usedSerialPorts.removeAll(device->paramValue(modbusRTUInterfaceDeviceSerialPortParamTypeId).toString());
        ModbusRTUMaster *modbus = m_modbusRTUMasters.take(device);
        modbus->deleteLater();
    }
}

void DevicePluginModbusCommander::onRefreshTimer()
{
    foreach (Device *device, myDevices()) {
        if ((device->deviceClassId() == modbusTCPReadDeviceClassId) || (device->deviceClassId() == modbusRTUReadDeviceClassId)) {
            readData(device);
        }
    }
}

void DevicePluginModbusCommander::readData(Device *device) {

    if (device->deviceClassId() == modbusTCPReadDeviceClassId) {
        Device *parent = myDevices().findById(device->parentId());
        ModbusTCPMaster *modbus = m_modbusTCPMasters.value(parent);
        int registerAddress = device->paramValue(modbusTCPReadDeviceRegisterAddressParamTypeId).toInt();
        int slaveAddress = device->paramValue(modbusTCPReadDeviceSlaveAddressParamTypeId).toInt();

        if (device->paramValue(modbusTCPReadDeviceRegisterTypeParamTypeId) == "coil"){
            bool data;
            if (modbus->getCoil(slaveAddress, registerAddress, &data)) {
                device->setStateValue(modbusTCPReadReadDataStateTypeId, data);
                device->setStateValue(modbusTCPReadConnectedStateTypeId, true);
            } else {
                device->setStateValue(modbusTCPReadConnectedStateTypeId, false);
            }
        } else if (device->paramValue(modbusTCPReadDeviceRegisterTypeParamTypeId) == "register") {
            int data;
            if (modbus->getRegister(slaveAddress, registerAddress, &data)) {
                device->setStateValue(modbusTCPReadReadDataStateTypeId, data);
                device->setStateValue(modbusTCPReadConnectedStateTypeId, true);
            } else {
                device->setStateValue(modbusTCPReadConnectedStateTypeId, false);
            }
        }
    }

    if (device->deviceClassId() == modbusRTUReadDeviceClassId) {
        Device *parent = myDevices().findById(device->parentId());
        ModbusRTUMaster *modbus = m_modbusRTUMasters.value(parent);
        int slaveAddress = device->paramValue(modbusRTUReadDeviceSlaveAddressParamTypeId).toInt();
        int registerAddress = device->paramValue(modbusRTUReadDeviceRegisterAddressParamTypeId).toInt();

        if (device->paramValue(modbusRTUReadDeviceRegisterTypeParamTypeId) == "coil"){
            bool data;
            if (modbus->getCoil(slaveAddress, registerAddress, &data)){
                device->setStateValue(modbusRTUReadReadDataStateTypeId, data);
                device->setStateValue(modbusRTUReadConnectedStateTypeId, true);
            } else {
                device->setStateValue(modbusRTUReadConnectedStateTypeId, false);
            }
        } else if (device->paramValue(modbusRTUReadDeviceRegisterTypeParamTypeId) == "register") {
            int data;
            if (modbus->getRegister(slaveAddress, registerAddress, &data)) {
                device->setStateValue(modbusRTUReadReadDataStateTypeId, data);
                device->setStateValue(modbusRTUReadConnectedStateTypeId, true);
            } else {
                device->setStateValue(modbusRTUReadConnectedStateTypeId, false);
            }
        }
    }
}

void DevicePluginModbusCommander::writeData(Device *device, Action action) {

    if (device->deviceClassId() == modbusTCPWriteDeviceClassId) {
        Device *parent = myDevices().findById(device->parentId());
        ModbusTCPMaster *modbus = m_modbusTCPMasters.value(parent);
        int registerAddress = device->paramValue(modbusTCPWriteDeviceRegisterAddressParamTypeId).toInt();
        int slaveAddress = device->paramValue(modbusTCPWriteDeviceSlaveAddressParamTypeId).toInt();

        if (device->paramValue(modbusTCPWriteDeviceRegisterTypeParamTypeId).toString() == "coil") {
            bool data = action.param(modbusTCPWriteDataActionDataParamTypeId).value().toBool();
            modbus->setCoil(slaveAddress, registerAddress, data);

        } else if (device->paramValue(modbusTCPWriteDeviceRegisterTypeParamTypeId).toString() == "register") {
            int data = action.param(modbusTCPWriteDataActionDataParamTypeId).value().toInt();
            modbus->setRegister(slaveAddress, registerAddress, data);
        }
    }

    if (device->deviceClassId() == modbusRTUWriteDeviceClassId) {
        Device *parent = myDevices().findById(device->parentId());

        ModbusRTUMaster *modbus = m_modbusRTUMasters.value(parent);
        int registerAddress = device->paramValue(modbusTCPWriteDeviceRegisterAddressParamTypeId).toInt();
        int slaveAddress = device->paramValue(modbusRTUWriteDeviceSlaveAddressParamTypeId).toInt();

        if (device->paramValue(modbusRTUWriteDeviceRegisterTypeParamTypeId).toString() == "coil") {
            bool data = action.param(modbusRTUWriteDataActionDataParamTypeId).value().toBool();
            modbus->setCoil(slaveAddress, registerAddress, data);

        } else if (device->paramValue(modbusRTUWriteDeviceRegisterTypeParamTypeId).toString() == "register") {
            int data = action.param(modbusRTUWriteDataActionDataParamTypeId).value().toInt();
            modbus->setRegister(slaveAddress, registerAddress, data);
        }
    }
}
