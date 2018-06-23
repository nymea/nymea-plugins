/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *                                                                         *
 *  Copyright (C) 2017 Bernhard Trinnes <bernhard.trinnes@guh.io           *
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
        QHostAddress ipAddress = QHostAddress(device->paramValue(modbusTCPWriteIpv4addressParamTypeId).toString());
        int slaveAddress = device->paramValue(modbusTCPWriteSlaveAddressParamTypeId).toInt();
        int port = device->paramValue(modbusTCPWritePortParamTypeId).toInt();

        foreach(ModbusTCPClient *modbusTCPClient, m_modbusSockets.values())
        {
            if ((modbusTCPClient->ipv4Address() == ipAddress) && (modbusTCPClient->port() == port)){
                m_modbusSockets.insert(device, modbusTCPClient);
                return DeviceManager::DeviceSetupStatusSuccess;
            }
        }
        ModbusTCPClient *modbus = new ModbusTCPClient(ipAddress, port, slaveAddress, this);
        m_modbusSockets.insert(device, modbus);
        return DeviceManager::DeviceSetupStatusSuccess;
    }

    if (device->deviceClassId() == modbusTCPReadDeviceClassId) {

        QHostAddress ipAddress = QHostAddress(device->paramValue(modbusTCPReadIpv4addressParamTypeId).toString());
        int slaveAddress = device->paramValue(modbusTCPReadSlaveAddressParamTypeId).toInt();
        int port = device->paramValue(modbusTCPReadPortParamTypeId).toInt();

        foreach(ModbusTCPClient *modbusTCPClient, m_modbusSockets.values())
        {
            if ((modbusTCPClient->ipv4Address() == ipAddress) && (modbusTCPClient->port() == port)){
                m_modbusSockets.insert(device, modbusTCPClient);
                return DeviceManager::DeviceSetupStatusSuccess;
            }
        }
        ModbusTCPClient *modbus = new ModbusTCPClient(ipAddress, port, slaveAddress, this);
        m_modbusSockets.insert(device, modbus);
        return DeviceManager::DeviceSetupStatusSuccess;
    }
    return DeviceManager::DeviceSetupStatusFailure;
}


void DevicePluginModbusCommander::postSetupDevice(Device *device)
{
    if (device->deviceClassId() == modbusTCPReadDeviceClassId) {
        ModbusTCPClient *modbus = m_modbusSockets.value(device);
        int reg = device->paramValue(modbusTCPReadRegisterAddressParamTypeId).toInt();

        if (device->paramValue(modbusTCPReadRegisterTypeParamTypeId) == "coil"){
            bool data = modbus->getCoil(reg);
            device->setStateValue(modbusTCPReadReadDataStateTypeId, data);
        } else if (device->paramValue(modbusTCPReadRegisterTypeParamTypeId) == "register") {
            int data = modbus->getRegister(reg);
            device->setStateValue(modbusTCPReadReadDataStateTypeId, data);
        }
        device->setStateValue(modbusTCPReadConnectedStateTypeId, true);
    } else if (device->deviceClassId() == modbusTCPWriteDeviceClassId) {
        device->setStateValue(modbusTCPWriteConnectedStateTypeId, true);
    }
}


DeviceManager::DeviceError DevicePluginModbusCommander::executeAction(Device *device, const Action &action)
{
    if (device->deviceClassId() == modbusTCPWriteDeviceClassId) {

        if (action.actionTypeId() == modbusTCPWriteWriteDataActionTypeId) {

            ModbusTCPClient *modbus = m_modbusSockets.value(device);
            int address = device->paramValue(modbusTCPWriteRegisterAddressParamTypeId).toInt();

            if (device->paramValue(modbusTCPWriteRegisterTypeParamTypeId).toString() == "coil") {
                bool data = action.param(modbusTCPWriteDataParamTypeId).value().toBool();
                modbus->setCoil(address, data);

            } else if (device->paramValue(modbusTCPWriteRegisterTypeParamTypeId).toString() == "register") {
                int data = action.param(modbusTCPWriteDataParamTypeId).value().toInt();
                modbus->setRegister(address, data);
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
        ModbusTCPClient *modbus = m_modbusSockets.value(device);
        m_modbusSockets.remove(device);
        if (!m_modbusSockets.values().contains(modbus)){
            modbus->deleteLater(); // no device uses this modbus socket anymore
        }
    }
}

void DevicePluginModbusCommander::onRefreshTimer()
{
    foreach (Device *device, myDevices()) {
        if (device->deviceClassId() == modbusTCPReadDeviceClassId) {
            ModbusTCPClient *modbus = m_modbusSockets.value(device);
            device->setStateValue(modbusTCPReadConnectedStateTypeId, modbus->connected());
            if (modbus->connected()) {
                int reg = device->paramValue(modbusTCPReadRegisterAddressParamTypeId).toInt();
                if (device->paramValue(modbusTCPReadRegisterTypeParamTypeId) == "coil"){
                    bool data = modbus->getCoil(reg);
                    device->setStateValue(modbusTCPReadReadDataStateTypeId, data);
                } else if (device->paramValue(modbusTCPReadRegisterTypeParamTypeId) == "register") {
                    int data = modbus->getRegister(reg);
                    device->setStateValue(modbusTCPReadReadDataStateTypeId, data);
                }
            } else {
                modbus->reconnect(device->paramValue(modbusTCPReadRegisterAddressParamTypeId).toInt());
            }
        } else if (device->deviceClassId() == modbusTCPWriteDeviceClassId) {
            ModbusTCPClient *modbus = m_modbusSockets.value(device);
            device->setStateValue(modbusTCPWriteConnectedStateTypeId, modbus->connected());
             if (!modbus->connected()) {
                 modbus->reconnect(device->paramValue(modbusTCPWriteRegisterAddressParamTypeId).toInt());
             }
        }
    }
}
