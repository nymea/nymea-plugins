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


DeviceManager::DeviceSetupStatus DevicePluginModbusCommander::setupDevice(Device *device)
{
    if (!m_refreshTimer) {
        // Refresh timer for TCP read
        int refreshTime = configValue(modbusCommanderPluginUpdateIntervalParamTypeId).toInt();
        m_refreshTimer = hardwareManager()->pluginTimerManager()->registerTimer(refreshTime);
        connect(m_refreshTimer, &PluginTimer::timeout, this, &DevicePluginModbusCommander::onRefreshTimer);
    }

    if (device->deviceClassId() == modbusTCPInterfaceDeviceClassId) {
        QString ipAddress = device->paramValue(modbusTCPInterfaceDeviceIpv4addressParamTypeId).toString();
        int port = device->paramValue(modbusTCPInterfaceDevicePortParamTypeId).toInt();

        foreach (ModbusTCPMaster *modbusTCPMaster, m_modbusTCPMasters.values()) {
            if ((modbusTCPMaster->ipv4Address() == ipAddress) && (modbusTCPMaster->port() == port)){
                m_modbusTCPMasters.insert(device, modbusTCPMaster);
                return DeviceManager::DeviceSetupStatusSuccess;
            }
        }

        ModbusTCPMaster *modbusTCPMaster = new ModbusTCPMaster(ipAddress, port, this);
        connect(modbusTCPMaster, )
                m_modbusTCPMasters.insert(device, modbus);
        return DeviceManager::DeviceSetupStatusSuccess;
    }

    if (device->deviceClassId() == modbusTCPWriteDeviceClassId) {

        device->setParentId(device->paramValue(modbusTCPWriteDeviceParentIdParamTypeId).toString());
        return DeviceManager::DeviceSetupStatusSuccess;
    }

    if (device->deviceClassId() == modbusTCPReadDeviceClassId) {

        device->setParentId(device->paramValue(modbusTCPReadDeviceParentIdParamTypeId).toString());
        return DeviceManager::DeviceSetupStatusSuccess;
    }

    if (device->deviceClassId() == modbusRTUInterfaceDeviceClassId) {

        QString serialPort = device->paramValue(modbusRTUInterfaceDeviceSerialPortParamTypeId).toString();
        int baudrate = device->paramValue(modbusRTUInterfaceDeviceBaudRateParamTypeId).toInt();
        int stopBits = device->paramValue(modbusRTUInterfaceDeviceStopBitsParamTypeId).toInt();
        int dataBits = device->paramValue(modbusRTUInterfaceDeviceDataBitsParamTypeId).toInt();
        QString parity;
        if (device->paramValue(modbusRTUInterfaceDeviceParityParamTypeId).toString().contains("No")) {
            modbusRTUMaster->setConnectionParameter(QModbusDevice::SerialParityParameter, QSerialPort::Parity::EvenParity);
        } else if (device->paramValue(modbusRTUInterfaceDeviceParityParamTypeId).toString().contains("Even")) {
            modbusRTUMaster->setConnectionParameter(QModbusDevice::SerialParityParameter, QSerialPort::Parity::NoParity);
        } else if (device->paramValue(modbusRTUInterfaceDeviceParityParamTypeId).toString().contains("Odd")) {
            modbusRTUMaster->setConnectionParameter(QModbusDevice::SerialParityParameter, QSerialPort::Parity::OddParity);
        }

        m_modbusRTUMasters.insert(device, modbusRTUMaster);
        return DeviceManager::DeviceSetupStatusSuccess;
    }

    if (device->deviceClassId() == modbusWriteDeviceClassId) {
        device->setParentId(device->paramValue(modbusWriteDeviceParentIdParamTypeId).toString());
        return DeviceManager::DeviceSetupStatusSuccess;
    }

    if (device->deviceClassId() == modbusReadDeviceClassId) {
        device->setParentId(device->paramValue(modbusReadDeviceParentIdParamTypeId).toString());
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
            //Serial port is not yet used, create now a new one
            qCDebug(dcModbusCommander()) << "Found serial port:" << port.systemLocation();
            QString description = port.manufacturer() + " " + port.description();
            DeviceDescriptor deviceDescriptor(deviceClassId, port.portName(), description);
            ParamList parameters;
            QString serialPort = port.systemLocation();
            foreach (Device *existingDevice, myDevices()) {
                if (existingDevice->paramValue(modbusRTUInterfaceDeviceSerialPortParamTypeId).toString() == serialPort) {
                    deviceDescriptor.setDeviceId(existingDevice->id());
                    break;
                }
            }
            parameters.append(Param(modbusRTUInterfaceDeviceSerialPortParamTypeId, serialPort));
            deviceDescriptor.setParams(parameters);
            deviceDescriptors.append(deviceDescriptor);
        }
        emit devicesDiscovered(deviceClassId, deviceDescriptors);
        return DeviceManager::DeviceErrorAsync;
    }

    if (deviceClassId == modbusReadDeviceClassId) {
        Q_FOREACH(Device *interfaceDevice, myDevices()){
            if (interfaceDevice->deviceClassId() == modbusTCPInterfaceDeviceClassId) {
                DeviceDescriptor descriptor(deviceClassId, interfaceDevice->name(), interfaceDevice->paramValue(modbusTCPInterfaceDeviceIpv4addressParamTypeId).toString() + "Port: " + interfaceDevice->paramValue(modbusTCPInterfaceDevicePortParamTypeId).toString());
                ParamList parameters;
                parameters.append(Param(modbusReadDeviceParentIdParamTypeId, interfaceDevice->id()));
                descriptor.setParams(parameters);
                deviceDescriptors.append(descriptor);
            }
            if (interfaceDevice->deviceClassId() == modbusRTUInterfaceDeviceClassId) {
                DeviceDescriptor descriptor(deviceClassId, interfaceDevice->name(), interfaceDevice->paramValue(modbusRTUInterfaceDeviceSerialPortParamTypeId).toString());
                ParamList parameters;
                parameters.append(Param(modbusReadDeviceParentIdParamTypeId, interfaceDevice->id()));
                descriptor.setParams(parameters);
                deviceDescriptors.append(descriptor);
            }
        }
        emit devicesDiscovered(deviceClassId, deviceDescriptors);
        return DeviceManager::DeviceErrorAsync;
    }

    if (deviceClassId == modbusWriteDeviceClassId) {
        Q_FOREACH(Device *interfaceDevice, myDevices()){
            if (interfaceDevice->deviceClassId() == modbusTCPInterfaceDeviceClassId) {
                DeviceDescriptor descriptor(deviceClassId, interfaceDevice->name(), interfaceDevice->paramValue(modbusRTUInterfaceDeviceSerialPortParamTypeId).toString());
                ParamList parameters;
                parameters.append(Param(modbusWriteDeviceParentIdParamTypeId, interfaceDevice->id()));
                descriptor.setParams(parameters);
                deviceDescriptors.append(descriptor);
            }
            if (interfaceDevice->deviceClassId() == modbusRTUInterfaceDeviceClassId) {
                DeviceDescriptor descriptor(deviceClassId, interfaceDevice->name(), interfaceDevice->paramValue(modbusRTUInterfaceDeviceSerialPortParamTypeId).toString());
                ParamList parameters;
                parameters.append(Param(modbusWriteDeviceParentIdParamTypeId, interfaceDevice->id()));
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
    if (device->deviceClassId() == modbusReadDeviceClassId) {
        readData(device);
    }

    if (device->deviceClassId() == modbusWriteDeviceClassId) {
        readData(device);
    }
}


DeviceManager::DeviceError DevicePluginModbusCommander::executeAction(Device *device, const Action &action)
{
    if (device->deviceClassId() == modbusWriteDeviceClassId) {

        if (action.actionTypeId() == modbusWriteDataActionTypeId) {
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
    foreach (Device *clientDevice, m_modbusTCPMasters.keys()) {
        ModbusTCPMaster *modbusTcpMaster = m_modbusTCPMasters.value(clientDevice);
        myDevices().filterByParam()
    if (device->deviceClassId() == coilDeviceClassId){

    }
    if (device->deviceClassId() == discreteInputDeviceClassId){

    }
    if (device->deviceClassId() == inputRegisterDeviceClassId){

    }
    if (device->deviceClassId() == holdingRegisterDeviceClassId){

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

