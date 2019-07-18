/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *                                                                         *
 *  Copyright (C) 2018 Bernhard Trinnes <bernhard.trinnes@guh.io           *
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
    \page onewire.html
    \title One wire
    \brief Plugin for one wire devices.

    \ingroup plugins
    \ingroup nymea-plugins

    This plugin allows to receive data from the onw wire file system.

    \chapter Plugin properties
    Following JSON file contains the definition and the description of all available \l{DeviceClass}{DeviceClasses}
    and \l{Vendor}{Vendors} of this \l{DevicePlugin}.

    For more details how to read this JSON file please check out the documentation for \l{The plugin JSON File}.

    \quotefile plugins/deviceplugins/OneWire/devicepluginOneWire.json
*/

#include "devicepluginonewire.h"
#include "devices/device.h"
#include "plugininfo.h"

#include <QDebug>
#include <QDir>


//https://github.com/owfs
//https://github.com/owfs/owfs-doc/wiki

DevicePluginOneWire::DevicePluginOneWire()
{
}

Device::DeviceError DevicePluginOneWire::discoverDevices(const DeviceClassId &deviceClassId, const ParamList &params)
{
    Q_UNUSED(params);

    if (deviceClassId == temperatureSensorDeviceClassId) {

        foreach(Device *parentDevice, myDevices().filterByDeviceClassId(oneWireInterfaceDeviceClassId)) {
            if (parentDevice->stateValue(oneWireInterfaceAutoAddStateTypeId).toBool()) {
                //devices cannot be discovered since auto mode is enabled
                return Device::DeviceErrorNoError;
            } else {
                if (m_oneWireInterface)
                    m_oneWireInterface->discoverDevices();
            }
        }
        return Device::DeviceErrorAsync;
    }
    return Device::DeviceErrorDeviceClassNotFound;
}


Device::DeviceSetupStatus DevicePluginOneWire::setupDevice(Device *device)
{
    if(!m_pluginTimer) {
        m_pluginTimer = hardwareManager()->pluginTimerManager()->registerTimer(10);
        connect(m_pluginTimer, &PluginTimer::timeout, this, &DevicePluginOneWire::onPluginTimer);
    }

    if (device->deviceClassId() == oneWireInterfaceDeviceClassId) {
        qCDebug(dcOneWire) << "Setup one wire interface";

        /*if(!myDevices().filterByDeviceClassId(oneWireInterfaceDeviceClassId).isEmpty()) {
            qCWarning(dcOneWire) << "Only one one wire interfaces allowed";
            return Device::DeviceSetupStatusFailure;
        }*/
        m_oneWireInterface = new OneWire(device->paramValue(oneWireInterfaceDevicePathParamTypeId).toByteArray(), this);

        if (!m_oneWireInterface->init()){
            m_oneWireInterface->deleteLater();
            return Device::DeviceSetupStatusFailure;
        }
        connect(m_oneWireInterface, &OneWire::devicesDiscovered, this, &DevicePluginOneWire::onOneWireDevicesDiscovered);
        return Device::DeviceSetupStatusSuccess;
    }

    if (device->deviceClassId() == temperatureSensorDeviceClassId) {

        qCDebug(dcOneWire) << "Setup one wire temperature sensor" << device->params();
        if (!m_oneWireInterface) { //in case the child was setup before the interface
            double temperature = m_oneWireInterface->getTemperature(device->paramValue(temperatureSensorDeviceAddressParamTypeId).toByteArray());
            device->setStateValue(temperatureSensorTemperatureStateTypeId, temperature);
        }
        return Device::DeviceSetupStatusSuccess;
    }

    if (device->deviceClassId() == iButtonDeviceClassId) {
        qCDebug(dcOneWire) << "Setup one wire iButton" << device->params();
        if (!m_oneWireInterface) {

        }
        return Device::DeviceSetupStatusSuccess;
    }

    if (device->deviceClassId() == switchDeviceClassId) {
        qCDebug(dcOneWire) << "Setup one wire switch" << device->params();
        if (!m_oneWireInterface) {
            QByteArray address = device->paramValue(switchDeviceAddressParamTypeId).toByteArray();
            device->setStateValue(switchDigitalOutputStateTypeId, m_oneWireInterface->getSwitchState(address));
        }
        return Device::DeviceSetupStatusSuccess;
    }
    return Device::DeviceSetupStatusFailure;
}

Device::DeviceError DevicePluginOneWire::executeAction(Device *device, const Action &action)
{
    Q_UNUSED(action)
    if (device->deviceClassId() == oneWireInterfaceDeviceClassId) {
        if (action.actionTypeId() == oneWireInterfaceAutoAddActionTypeId){
            device->setStateValue(oneWireInterfaceAutoAddStateTypeId, action.param(oneWireInterfaceAutoAddActionAutoAddParamTypeId).value());
            return Device::DeviceErrorNoError;
        }
        return Device::DeviceErrorActionTypeNotFound;
    }

    if (device->deviceClassId() == switchDeviceClassId) {

        if (action.actionTypeId() == switchDigitalOutputActionTypeId){
            m_oneWireInterface->setSwitchState(device->paramValue(switchDeviceAddressParamTypeId).toByteArray(), action.param(switchDigitalOutputActionDigitalOutputParamTypeId).value().toBool());

            return Device::DeviceErrorNoError;
        }
        return Device::DeviceErrorActionTypeNotFound;
    }
    return Device::DeviceErrorNoError;
}


void DevicePluginOneWire::deviceRemoved(Device *device)
{
    if (device->deviceClassId() == oneWireInterfaceDeviceClassId) {
        m_oneWireInterface->deleteLater();
        return;
    }

    if (myDevices().empty()) {
        hardwareManager()->pluginTimerManager()->unregisterTimer(m_pluginTimer);

    }
}


void DevicePluginOneWire::onPluginTimer()
{
    foreach (Device *device, myDevices()) {
        if (device->deviceClassId() == oneWireInterfaceDeviceClassId) {
            device->setStateValue(oneWireInterfaceConnectedStateTypeId, m_oneWireInterface->interfaceIsAvailable());

            if (device->stateValue(oneWireInterfaceAutoAddStateTypeId).toBool()) {
                m_oneWireInterface->discoverDevices();
            }
        }

        if (device->deviceClassId() == temperatureSensorDeviceClassId) {
            QByteArray address = device->paramValue(temperatureSensorDeviceAddressParamTypeId).toByteArray();

            double temperature = m_oneWireInterface->getTemperature(address);
            device->setStateValue(temperatureSensorTemperatureStateTypeId, temperature);
        }
    }
}

void DevicePluginOneWire::onOneWireDevicesDiscovered(QList<OneWire::OneWireDevice> oneWireDevices)
{
    foreach(Device *parentDevice, myDevices().filterByDeviceClassId(oneWireInterfaceDeviceClassId)) {

        bool autoDiscoverEnabled = parentDevice->stateValue(oneWireInterfaceAutoAddStateTypeId).toBool();
        QList<DeviceDescriptor> temperatureDeviceDescriptors;
        foreach (OneWire::OneWireDevice oneWireDevice, oneWireDevices){
            switch (oneWireDevice.family) {
            //https://github.com/owfs/owfs-doc/wiki/1Wire-Device-List
            case 0x10:  //DS18S20
            case 0x28:  //DS18B20
            case 0x3b:  //DS1825, MAX31826, MAX31850
                DeviceDescriptor descriptor(temperatureSensorDeviceClassId, oneWireDevice.type, "One wire temperature sensor", parentDevice->id());
                ParamList params;
                params.append(Param(temperatureSensorDeviceAddressParamTypeId, oneWireDevice.address));
                params.append(Param(temperatureSensorDeviceTypeParamTypeId, oneWireDevice.type));
                foreach (Device *existingDevice, myDevices().filterByDeviceClassId(temperatureSensorDeviceClassId)){
                    if (existingDevice->paramValue(temperatureSensorDeviceAddressParamTypeId).toString() == oneWireDevice.address) {
                        descriptor.setDeviceId(existingDevice->id());
                        break;
                    }
                }
                descriptor.setParams(params);
                temperatureDeviceDescriptors.append(descriptor);
                break;
            }
        }
        if (autoDiscoverEnabled) {
            if (!temperatureDeviceDescriptors.isEmpty())
                emit autoDevicesAppeared(temperatureSensorDeviceClassId,  temperatureDeviceDescriptors);
        } else {
            if (!temperatureDeviceDescriptors.isEmpty())
                emit devicesDiscovered(temperatureSensorDeviceClassId,  temperatureDeviceDescriptors);
        }
        break;
    }
}


/*    foreach(QByteArray member, dirMembers) {
        int family = member.split('.').first().toInt(nullptr, 16);
        qDebug(dcOneWire()) << "Member" << member << member.left(2) << family;
        member.remove(member.indexOf('/'), 1);
        QByteArray type;
        switch (family) {
        //https://github.com/owfs/owfs-doc/wiki/1Wire-Device-List
        case 0x10:  //DS18S20
        case 0x28:  //DS18B20
        case 0x3b:  //DS1825, MAX31826, MAX31850
            OneWireDevice device;
            device.family =family;
            device.Address = member.split('.').last();
            device.Type = getValue(member, "type");
            oneWireDevices.append(device);
            qDebug(dcOneWire()) << "Discovered temperature sensor" <<  type << member;
            break;
        case 0x05:
        case 0x12:
        case 0x1C:
        case 0x3A:
            OneWireDevice device;
            device.family =family;
            device.Address = member.split('.').last();
            device.Type = getValue(member, "type");
            oneWireDevices.append(device);
            qDebug(dcOneWire()) << "Discovered temperature sensor" <<  type << member;
            qDebug(dcOneWire()) << "Discovered switch" <<  type << member;
            break;

        case 0x08: //DS1992 1kbit Memory iButton
        case 0x06: //DS1993 4kbit Memory iButton
        case 0x0A: //DS1995 16kbit Memory iButton
        case 0x0C: //DS1996 64kbit Memory iButton
            type = getValue(member, "type");
            qDebug(dcOneWire()) << "Discovered ID device" <<  type << member;
            break;
        default:
            //type = getValue(member, "type");
            //qDebug(dcOneWire()) << "Discovered unknown " <<  type << member;
            //emit unknownDeviceDiscovered(member, type);
            break ;
        }
    }
    if(!oneWireDevices.isEmpty*/
