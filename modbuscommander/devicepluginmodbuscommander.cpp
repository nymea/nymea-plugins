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
    m_refreshTimer = hardwareManager()->pluginTimerManager()->registerTimer(60);
    connect(m_refreshTimer, &PluginTimer::timeout, this, &DevicePluginModbusCommander::onRefreshTimer);
}


DeviceManager::DeviceSetupStatus DevicePluginModbusCommander::setupDevice(Device *device)
{
    if (device->deviceClassId() == modbusTCPWriteDeviceClassId) {
        QHostAddress ipAddress = QHostAddress(device->paramValue(modbusTCPWriteDeviceIpv4addressParamTypeId).toString());
        int slaveAddress = device->paramValue(modbusTCPWriteDeviceSlaveAddressParamTypeId).toInt();
        int port = device->paramValue(modbusTCPWriteDevicePortParamTypeId).toInt();

        foreach(ModbusTCPMaster *modbusTCPMaster, m_modbusSockets.values())
        {
            if ((modbusTCPMaster->ipv4Address() == ipAddress) && (modbusTCPMaster->port() == port)){
                m_modbusSockets.insert(device, modbusTCPMaster);
                return DeviceManager::DeviceSetupStatusSuccess;
            }
        }
        ModbusTCPMaster *modbus = new ModbusTCPMaster(ipAddress, port, slaveAddress, this);
        m_modbusSockets.insert(device, modbus);
        return DeviceManager::DeviceSetupStatusSuccess;
    }

    if (device->deviceClassId() == modbusTCPReadDeviceClassId) {

        QHostAddress ipAddress = QHostAddress(device->paramValue(modbusTCPReadDeviceIpv4addressParamTypeId).toString());
        int slaveAddress = device->paramValue(modbusTCPReadDeviceSlaveAddressParamTypeId).toInt();
        int port = device->paramValue(modbusTCPReadDevicePortParamTypeId).toInt();

        foreach(ModbusTCPMaster *modbusTCPMaster, m_modbusSockets.values())
        {
            if ((modbusTCPMaster->ipv4Address() == ipAddress) && (modbusTCPMaster->port() == port)){
                m_modbusSockets.insert(device, modbusTCPMaster);
                return DeviceManager::DeviceSetupStatusSuccess;
            }
        }
        ModbusTCPMaster *modbus = new ModbusTCPMaster(ipAddress, port, slaveAddress, this);
        m_modbusSockets.insert(device, modbus);
        return DeviceManager::DeviceSetupStatusSuccess;
    }

    if (device->deviceClassId() == modbusRTUInterfaceDeviceClassId) {

        QString serialPort = device->paramValue(modbusRTUInterfaceDeviceSerialPortParamTypeId).toString();
        int baudrate = device->paramValue(modbusRTUInterfaceDeviceBaudRateParamTypeId).toInt();
        int stopBits = device->paramValue(modbusRTUInterfaceDeviceStopBitsParamTypeId).toInt();
        int dataBits = device->paramValue(modbusRTUInterfaceDeviceDataBitsParamTypeId).toInt();
        QString parity = device->paramValue(modbusRTUInterfaceDeviceSerialPortParamTypeId).toString();

        ModbusRTUMaster *modbus = new ModbusRTUMaster(serialPort, baudrate, parity, dataBits, stopBits, this);
        m_rtuInterfaces.insert(device, modbus);
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
    return DeviceManager::DeviceErrorDeviceClassNotFound;
}

void DevicePluginModbusCommander::postSetupDevice(Device *device)
{
    if (device->deviceClassId() == modbusTCPReadDeviceClassId) {
        ModbusTCPMaster *modbus = m_modbusSockets.value(device);
        int reg = device->paramValue(modbusTCPReadDeviceRegisterAddressParamTypeId).toInt();

        if (device->paramValue(modbusTCPReadDeviceRegisterTypeParamTypeId) == "coil"){
            bool data = modbus->getCoil(reg);
            device->setStateValue(modbusTCPReadReadDataStateTypeId, data);
        } else if (device->paramValue(modbusTCPReadDeviceRegisterTypeParamTypeId) == "register") {
            int data = modbus->getRegister(reg);
            device->setStateValue(modbusTCPReadReadDataStateTypeId, data);
        }
        device->setStateValue(modbusTCPReadConnectedStateTypeId, true);
    }

    if (device->deviceClassId() == modbusTCPWriteDeviceClassId) {
        device->setStateValue(modbusTCPWriteConnectedStateTypeId, true);
    }

    if (device->deviceClassId() == modbusRTUReadDeviceClassId) {
        ModbusRTUMaster *modbus = m_rtuInterfaces.value(device);
        int slaveAddress = device->paramValue(modbusRTUReadDeviceSlaveAddressParamTypeId).toInt();
        int registerAddress = device->paramValue(modbusRTUReadDeviceRegisterAddressParamTypeId).toInt();

        if (device->paramValue(modbusRTUReadDeviceRegisterTypeParamTypeId) == "coil"){
            bool data = modbus->getCoil(slaveAddress, registerAddress);
            device->setStateValue(modbusRTUReadReadDataStateTypeId, data);
        } else if (device->paramValue(modbusRTUReadDeviceRegisterTypeParamTypeId) == "register") {
            int data = modbus->getRegister(slaveAddress, registerAddress);
            device->setStateValue(modbusRTUReadReadDataStateTypeId, data);
        }
        //device->setStateValue(modbusRTUReadConnectedStateTypeId, true); TODO
    }

    if (device->deviceClassId() == modbusRTUWriteDeviceClassId) {

    }
}


DeviceManager::DeviceError DevicePluginModbusCommander::executeAction(Device *device, const Action &action)
{
    if (device->deviceClassId() == modbusTCPWriteDeviceClassId) {

        if (action.actionTypeId() == modbusTCPWriteWriteDataActionTypeId) {

            ModbusTCPMaster *modbus = m_modbusSockets.value(device);
            int address = device->paramValue(modbusTCPWriteDeviceRegisterAddressParamTypeId).toInt();

            if (device->paramValue(modbusTCPWriteDeviceRegisterTypeParamTypeId).toString() == "coil") {
                bool data = action.param(modbusTCPWriteWriteDataActionDataParamTypeId).value().toBool();
                modbus->setCoil(address, data);

            } else if (device->paramValue(modbusTCPWriteDeviceRegisterTypeParamTypeId).toString() == "register") {
                int data = action.param(modbusTCPWriteWriteDataActionDataParamTypeId).value().toInt();
                modbus->setRegister(address, data);
            }
            return DeviceManager::DeviceErrorNoError;
        }
        return DeviceManager::DeviceErrorActionTypeNotFound;
    }
    if (device->deviceClassId() == modbusRTUWriteDeviceClassId) {

        if (action.actionTypeId() == modbusRTUWriteWriteDataActionTypeId) {

            Device *parent = myDevices().findById(device->parentId());

            ModbusRTUMaster *modbus = m_rtuInterfaces.value(parent);
            int registerAddress = device->paramValue(modbusTCPWriteDeviceRegisterAddressParamTypeId).toInt();
            int slaveAddress = device->paramValue(modbusRTUWriteDeviceSlaveAddressParamTypeId).toInt();

            if (device->paramValue(modbusRTUWriteDeviceRegisterTypeParamTypeId).toString() == "coil") {
                bool data = action.param(modbusTCPWriteWriteDataActionDataParamTypeId).value().toBool();
                modbus->setCoil(slaveAddress, registerAddress, data);

            } else if (device->paramValue(modbusRTUWriteDeviceRegisterTypeParamTypeId).toString() == "register") {
                int data = action.param(modbusRTUWriteWriteDataActionDataParamTypeId).value().toInt();
                modbus->setRegister(slaveAddress, registerAddress, data);
            }

            return DeviceManager::DeviceErrorNoError;
        }
        return DeviceManager::DeviceErrorActionTypeNotFound;
    }

    return DeviceManager::DeviceErrorDeviceClassNotFound;
}


void DevicePluginModbusCommander::deviceRemoved(Device *device)
{
    if ((device->deviceClassId() == modbusTCPWriteDeviceClassId) || (device->deviceClassId() == modbusTCPReadDeviceClassId)) {
        ModbusTCPMaster *modbus = m_modbusSockets.value(device);
        m_modbusSockets.remove(device);
        if (!m_modbusSockets.values().contains(modbus)){
            modbus->deleteLater(); // no device uses this modbus socket anymore
        }
    }

    if (device->deviceClassId() == modbusRTUInterfaceDeviceClassId) {
        m_usedSerialPorts.removeAll(device->paramValue(modbusRTUInterfaceDeviceSerialPortParamTypeId).toString());
        ModbusRTUMaster *modbus = m_rtuInterfaces.take(device);
        modbus->deleteLater();
    }
}

void DevicePluginModbusCommander::onRefreshTimer()
{
    foreach (Device *device, myDevices()) {
        if (device->deviceClassId() == modbusTCPReadDeviceClassId) {
            ModbusTCPMaster *modbus = m_modbusSockets.value(device);
            device->setStateValue(modbusTCPReadConnectedStateTypeId, modbus->connected());
            if (modbus->connected()) {
                int reg = device->paramValue(modbusTCPReadDeviceRegisterAddressParamTypeId).toInt();
                if (device->paramValue(modbusTCPReadDeviceRegisterTypeParamTypeId) == "coil"){
                    bool data = modbus->getCoil(reg);
                    device->setParamValue(modbusTCPWriteWriteDataActionDataParamTypeId, data);
                } else if (device->paramValue(modbusTCPReadDeviceRegisterTypeParamTypeId) == "register") {
                    int data = modbus->getRegister(reg);
                    device->setParamValue(modbusTCPWriteWriteDataActionDataParamTypeId, data);
                }
            } else {
                modbus->reconnect(device->paramValue(modbusTCPReadDeviceRegisterAddressParamTypeId).toInt());
            }
        }

        if (device->deviceClassId() == modbusTCPWriteDeviceClassId) {
            ModbusTCPMaster *modbus = m_modbusSockets.value(device);
            device->setStateValue(modbusTCPWriteConnectedStateTypeId, modbus->connected());
            if (!modbus->connected()) {
                modbus->reconnect(device->paramValue(modbusTCPWriteDeviceRegisterAddressParamTypeId).toInt());
            }
        }

        if (device->deviceClassId() == modbusRTUReadDeviceClassId) {

            ModbusRTUMaster *modbus = m_rtuInterfaces.value(device);
            int registerAddress = device->paramValue(modbusRTUReadDeviceRegisterAddressParamTypeId).toInt();
            int slaveAddress = device->paramValue(modbusRTUReadDeviceSlaveAddressParamTypeId).toInt();
            if (device->paramValue(modbusRTUReadDeviceRegisterTypeParamTypeId) == "coil"){
                bool data = modbus->getCoil(slaveAddress, registerAddress);
                device->setParamValue(modbusTCPWriteWriteDataActionDataParamTypeId, data);
            } else if (device->paramValue(modbusTCPReadDeviceRegisterTypeParamTypeId) == "register") {
                int data = modbus->getRegister(slaveAddress, registerAddress);
                device->setParamValue(modbusTCPWriteWriteDataActionDataParamTypeId, data);
            }
        }

        if (device->deviceClassId() == modbusRTUWriteDeviceClassId) {

        }
    }
}
