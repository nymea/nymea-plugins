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

#include "devicepluginonewire.h"
#include "devices/device.h"
#include "plugininfo.h"

#include <QDebug>
#include <QDir>

DevicePluginOneWire::DevicePluginOneWire()
{
}

void DevicePluginOneWire::discoverDevices(DeviceDiscoveryInfo *info)
{
    DeviceClassId deviceClassId = info->deviceClassId();

    if (deviceClassId == temperatureSensorDeviceClassId       ||
            deviceClassId == singleChannelSwitchDeviceClassId ||
            deviceClassId == dualChannelSwitchDeviceClassId   ||
            deviceClassId == eightChannelSwitchDeviceClassId) {

        if (myDevices().filterByDeviceClassId(oneWireInterfaceDeviceClassId).isEmpty()) {
            //No one wire interface intitialized
            //: Error discovering one wire devices
            return info->finish(Device::DeviceErrorHardwareNotAvailable, QT_TR_NOOP("No one wire interface initialized. Please set up a one wire interface first."));
        }

        foreach(Device *parentDevice, myDevices().filterByDeviceClassId(oneWireInterfaceDeviceClassId)) {
            if (parentDevice->stateValue(oneWireInterfaceAutoAddStateTypeId).toBool()) {
                //devices cannot be discovered since auto mode is enabled
                return info->finish(Device::DeviceErrorNoError);
            } else {
                if (m_oneWireInterface)
                    m_oneWireInterface->discoverDevices();
            }
        }

        m_runningDiscoveries.append(info);
        connect(info, &DeviceDiscoveryInfo::destroyed, this, [this, info](){
            m_runningDiscoveries.removeAll(info);
        });
        return;
    }

    qCWarning(dcOneWire()) << "Discovery called for a deviceclass which does not support discovery? Device class ID:" << info->deviceClassId().toString();
    info->finish(Device::DeviceErrorDeviceClassNotFound);
}


void DevicePluginOneWire::setupDevice(DeviceSetupInfo *info)
{
    Device *device = info->device();
    if (device->deviceClassId() == oneWireInterfaceDeviceClassId) {
        qCDebug(dcOneWire) << "Setup one wire interface";

        if (m_oneWireInterface) {
            qCWarning(dcOneWire) << "One wire interface already set up";
            //: Error setting up device
            return info->finish(Device::DeviceErrorDeviceInUse, QT_TR_NOOP("There can only be one one wire interface per system."));
        }
        m_oneWireInterface = new OneWire(this);
        QByteArray initArguments = device->paramValue(oneWireInterfaceDeviceInitArgsParamTypeId).toByteArray();

        if (!m_oneWireInterface->init(initArguments)){
            m_oneWireInterface->deleteLater();
            m_oneWireInterface = nullptr;
            //: Error setting up device
            return info->finish(Device::DeviceErrorHardwareFailure, QT_TR_NOOP("Error initializing one wire interface."));
        }
        connect(m_oneWireInterface, &OneWire::devicesDiscovered, this, &DevicePluginOneWire::onOneWireDevicesDiscovered);
        return info->finish(Device::DeviceErrorNoError);
    }

    if (device->deviceClassId() == temperatureSensorDeviceClassId) {

        qCDebug(dcOneWire) << "Setup one wire temperature sensor" << device->params();
        if (!m_oneWireInterface) { //in case the child was setup before the interface
            double temperature = m_oneWireInterface->getTemperature(device->paramValue(temperatureSensorDeviceAddressParamTypeId).toByteArray());
            device->setStateValue(temperatureSensorTemperatureStateTypeId, temperature);
        }
        return info->finish(Device::DeviceErrorNoError);
    }

    if (device->deviceClassId() == singleChannelSwitchDeviceClassId) {
        qCDebug(dcOneWire) << "Setup one wire switch" << device->params();
        if (!m_oneWireInterface) {
            QByteArray address = device->paramValue(singleChannelSwitchDeviceAddressParamTypeId).toByteArray();
            device->setStateValue(singleChannelSwitchDigitalOutputStateTypeId, m_oneWireInterface->getSwitchOutput(address, OneWire::SwitchChannel::PIO_A));
        }
        return info->finish(Device::DeviceErrorNoError);
    }

    if (device->deviceClassId() == dualChannelSwitchDeviceClassId) {
        qCDebug(dcOneWire) << "Setup one wire dual switch" << device->params();
        if (!m_oneWireInterface) {
            QByteArray address = device->paramValue(dualChannelSwitchDeviceAddressParamTypeId).toByteArray();
            device->setStateValue(dualChannelSwitchDigitalOutput1StateTypeId, m_oneWireInterface->getSwitchOutput(address, OneWire::SwitchChannel::PIO_A));
            device->setStateValue(dualChannelSwitchDigitalOutput2StateTypeId, m_oneWireInterface->getSwitchOutput(address, OneWire::SwitchChannel::PIO_B));
        }
        return info->finish(Device::DeviceErrorNoError);
    }

    if (device->deviceClassId() == eightChannelSwitchDeviceClassId) {
        qCDebug(dcOneWire) << "Setup one wire eight channel switch" << device->params();
        if (!m_oneWireInterface) {
            QByteArray address = device->paramValue(eightChannelSwitchDeviceAddressParamTypeId).toByteArray();
            device->setStateValue(eightChannelSwitchDigitalOutput1StateTypeId, m_oneWireInterface->getSwitchOutput(address, OneWire::SwitchChannel::PIO_A));
            device->setStateValue(eightChannelSwitchDigitalOutput2StateTypeId, m_oneWireInterface->getSwitchOutput(address, OneWire::SwitchChannel::PIO_B));
            device->setStateValue(eightChannelSwitchDigitalOutput3StateTypeId, m_oneWireInterface->getSwitchOutput(address, OneWire::SwitchChannel::PIO_C));
            device->setStateValue(eightChannelSwitchDigitalOutput4StateTypeId, m_oneWireInterface->getSwitchOutput(address, OneWire::SwitchChannel::PIO_D));
            device->setStateValue(eightChannelSwitchDigitalOutput5StateTypeId, m_oneWireInterface->getSwitchOutput(address, OneWire::SwitchChannel::PIO_E));
            device->setStateValue(eightChannelSwitchDigitalOutput6StateTypeId, m_oneWireInterface->getSwitchOutput(address, OneWire::SwitchChannel::PIO_F));
            device->setStateValue(eightChannelSwitchDigitalOutput7StateTypeId, m_oneWireInterface->getSwitchOutput(address, OneWire::SwitchChannel::PIO_G));
            device->setStateValue(eightChannelSwitchDigitalOutput8StateTypeId, m_oneWireInterface->getSwitchOutput(address, OneWire::SwitchChannel::PIO_H));
        }
        return info->finish(Device::DeviceErrorNoError);
    }
    return info->finish(Device::DeviceErrorDeviceNotFound);
}

void DevicePluginOneWire::postSetupDevice(Device *device)
{
    Q_UNUSED(device);

    if(!m_pluginTimer) {
        m_pluginTimer = hardwareManager()->pluginTimerManager()->registerTimer(10);
        connect(m_pluginTimer, &PluginTimer::timeout, this, &DevicePluginOneWire::onPluginTimer);
    }
}

void DevicePluginOneWire::executeAction(DeviceActionInfo *info)
{
    Device *device = info->device();
    Action action = info->action();

    if (device->deviceClassId() == oneWireInterfaceDeviceClassId) {
        if (action.actionTypeId() == oneWireInterfaceAutoAddActionTypeId){
            device->setStateValue(oneWireInterfaceAutoAddStateTypeId, action.param(oneWireInterfaceAutoAddActionAutoAddParamTypeId).value());
            return info->finish(Device::DeviceErrorNoError);
        }
        return info->finish(Device::DeviceErrorActionTypeNotFound);
    }

    if (device->deviceClassId() == singleChannelSwitchDeviceClassId) {
        if (action.actionTypeId() == singleChannelSwitchDigitalOutputActionTypeId){
            m_oneWireInterface->setSwitchOutput(device->paramValue(singleChannelSwitchDeviceAddressParamTypeId).toByteArray(), OneWire::SwitchChannel::PIO_A, action.param(singleChannelSwitchDigitalOutputActionDigitalOutputParamTypeId).value().toBool());

            return info->finish(Device::DeviceErrorNoError);
        }
        return info->finish(Device::DeviceErrorActionTypeNotFound);
    }

    if (device->deviceClassId() == dualChannelSwitchDeviceClassId) {
        if (action.actionTypeId() == dualChannelSwitchDigitalOutput1ActionTypeId){
            m_oneWireInterface->setSwitchOutput(device->paramValue(dualChannelSwitchDeviceAddressParamTypeId).toByteArray(), OneWire::SwitchChannel::PIO_A, action.param(dualChannelSwitchDigitalOutput1ActionDigitalOutput1ParamTypeId).value().toBool());
            return info->finish(Device::DeviceErrorNoError);
        }
        if (action.actionTypeId() == dualChannelSwitchDigitalOutput2ActionTypeId){
            m_oneWireInterface->setSwitchOutput(device->paramValue(dualChannelSwitchDeviceAddressParamTypeId).toByteArray(), OneWire::SwitchChannel::PIO_B, action.param(dualChannelSwitchDigitalOutput2ActionDigitalOutput2ParamTypeId).value().toBool());
            return info->finish(Device::DeviceErrorNoError);
        }
        return info->finish(Device::DeviceErrorActionTypeNotFound);
    }

    if (device->deviceClassId() == eightChannelSwitchDeviceClassId) {
        if (action.actionTypeId() == eightChannelSwitchDigitalOutput1ActionTypeId){
            m_oneWireInterface->setSwitchOutput(device->paramValue(eightChannelSwitchDeviceAddressParamTypeId).toByteArray(), OneWire::SwitchChannel::PIO_A, action.param(eightChannelSwitchDigitalOutput1ActionDigitalOutput1ParamTypeId).value().toBool());
            return info->finish(Device::DeviceErrorNoError);
        }
        if (action.actionTypeId() == eightChannelSwitchDigitalOutput2ActionTypeId){
            m_oneWireInterface->setSwitchOutput(device->paramValue(eightChannelSwitchDeviceAddressParamTypeId).toByteArray(), OneWire::SwitchChannel::PIO_B, action.param(eightChannelSwitchDigitalOutput2ActionDigitalOutput2ParamTypeId).value().toBool());
            return info->finish(Device::DeviceErrorNoError);
        }
        if (action.actionTypeId() == eightChannelSwitchDigitalOutput3ActionTypeId){
            m_oneWireInterface->setSwitchOutput(device->paramValue(eightChannelSwitchDeviceAddressParamTypeId).toByteArray(), OneWire::SwitchChannel::PIO_C, action.param(eightChannelSwitchDigitalOutput3ActionDigitalOutput3ParamTypeId).value().toBool());
            return info->finish(Device::DeviceErrorNoError);
        }
        if (action.actionTypeId() == eightChannelSwitchDigitalOutput4ActionTypeId){
            m_oneWireInterface->setSwitchOutput(device->paramValue(eightChannelSwitchDeviceAddressParamTypeId).toByteArray(), OneWire::SwitchChannel::PIO_D, action.param(eightChannelSwitchDigitalOutput4ActionDigitalOutput4ParamTypeId).value().toBool());
            return info->finish(Device::DeviceErrorNoError);
        }
        if (action.actionTypeId() == eightChannelSwitchDigitalOutput5ActionTypeId){
            m_oneWireInterface->setSwitchOutput(device->paramValue(eightChannelSwitchDeviceAddressParamTypeId).toByteArray(), OneWire::SwitchChannel::PIO_E, action.param(eightChannelSwitchDigitalOutput5ActionDigitalOutput5ParamTypeId).value().toBool());
            return info->finish(Device::DeviceErrorNoError);
        }
        if (action.actionTypeId() == eightChannelSwitchDigitalOutput6ActionTypeId){
            m_oneWireInterface->setSwitchOutput(device->paramValue(eightChannelSwitchDeviceAddressParamTypeId).toByteArray(), OneWire::SwitchChannel::PIO_F, action.param(eightChannelSwitchDigitalOutput6ActionDigitalOutput6ParamTypeId).value().toBool());
            return info->finish(Device::DeviceErrorNoError);
        }
        if (action.actionTypeId() == eightChannelSwitchDigitalOutput7ActionTypeId){
            m_oneWireInterface->setSwitchOutput(device->paramValue(eightChannelSwitchDeviceAddressParamTypeId).toByteArray(), OneWire::SwitchChannel::PIO_G, action.param(eightChannelSwitchDigitalOutput7ActionDigitalOutput7ParamTypeId).value().toBool());
            return info->finish(Device::DeviceErrorNoError);
        }
        if (action.actionTypeId() == eightChannelSwitchDigitalOutput8ActionTypeId){
            m_oneWireInterface->setSwitchOutput(device->paramValue(eightChannelSwitchDeviceAddressParamTypeId).toByteArray(), OneWire::SwitchChannel::PIO_H, action.param(eightChannelSwitchDigitalOutput8ActionDigitalOutput8ParamTypeId).value().toBool());
            return info->finish(Device::DeviceErrorNoError);
        }
        return info->finish(Device::DeviceErrorActionTypeNotFound);
    }
    return info->finish(Device::DeviceErrorNoError);
}


void DevicePluginOneWire::deviceRemoved(Device *device)
{
    if (device->deviceClassId() == oneWireInterfaceDeviceClassId) {
        m_oneWireInterface->deleteLater();
        m_oneWireInterface = nullptr;
        return;
    }

    if (myDevices().empty()) {
        hardwareManager()->pluginTimerManager()->unregisterTimer(m_pluginTimer);
        m_pluginTimer = nullptr;
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

        if (device->deviceClassId() == singleChannelSwitchDeviceClassId) {
            QByteArray address = device->paramValue(singleChannelSwitchDeviceAddressParamTypeId).toByteArray();
            device->setStateValue(singleChannelSwitchDigitalOutputStateTypeId, m_oneWireInterface->getSwitchOutput(address, OneWire::SwitchChannel::PIO_A));
        }

        if (device->deviceClassId() == dualChannelSwitchDeviceClassId) {
            QByteArray address = device->paramValue(dualChannelSwitchDeviceAddressParamTypeId).toByteArray();
            device->setStateValue(dualChannelSwitchDigitalOutput1StateTypeId, m_oneWireInterface->getSwitchOutput(address, OneWire::SwitchChannel::PIO_A));
            device->setStateValue(dualChannelSwitchDigitalOutput2StateTypeId, m_oneWireInterface->getSwitchOutput(address, OneWire::SwitchChannel::PIO_B));
        }

        if (device->deviceClassId() == eightChannelSwitchDeviceClassId) {
            QByteArray address = device->paramValue(eightChannelSwitchDeviceAddressParamTypeId).toByteArray();
            device->setStateValue(eightChannelSwitchDigitalOutput1StateTypeId, m_oneWireInterface->getSwitchOutput(address, OneWire::SwitchChannel::PIO_A));
            device->setStateValue(eightChannelSwitchDigitalOutput2StateTypeId, m_oneWireInterface->getSwitchOutput(address, OneWire::SwitchChannel::PIO_B));
            device->setStateValue(eightChannelSwitchDigitalOutput3StateTypeId, m_oneWireInterface->getSwitchOutput(address, OneWire::SwitchChannel::PIO_C));
            device->setStateValue(eightChannelSwitchDigitalOutput4StateTypeId, m_oneWireInterface->getSwitchOutput(address, OneWire::SwitchChannel::PIO_D));
            device->setStateValue(eightChannelSwitchDigitalOutput5StateTypeId, m_oneWireInterface->getSwitchOutput(address, OneWire::SwitchChannel::PIO_E));
            device->setStateValue(eightChannelSwitchDigitalOutput6StateTypeId, m_oneWireInterface->getSwitchOutput(address, OneWire::SwitchChannel::PIO_F));
            device->setStateValue(eightChannelSwitchDigitalOutput7StateTypeId, m_oneWireInterface->getSwitchOutput(address, OneWire::SwitchChannel::PIO_G));
            device->setStateValue(eightChannelSwitchDigitalOutput8StateTypeId, m_oneWireInterface->getSwitchOutput(address, OneWire::SwitchChannel::PIO_H));
        }
    }
}

void DevicePluginOneWire::onOneWireDevicesDiscovered(QList<OneWire::OneWireDevice> oneWireDevices)
{
    foreach(Device *parentDevice, myDevices().filterByDeviceClassId(oneWireInterfaceDeviceClassId)) {

        bool autoDiscoverEnabled = parentDevice->stateValue(oneWireInterfaceAutoAddStateTypeId).toBool();
        DeviceDescriptors descriptors;
        foreach (OneWire::OneWireDevice oneWireDevice, oneWireDevices){
            switch (oneWireDevice.family) {
            //https://github.com/owfs/owfs-doc/wiki/1Wire-Device-List
            case 0x10:  //DS18S20
            case 0x22:  //DS1822
            case 0x28:  //DS18B20
            case 0x3b:  {//DS1825, MAX31826, MAX31850
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
                descriptors.append(descriptor);
                break;
            }
            case 0x05: { //single channel switch
                DeviceDescriptor descriptor(singleChannelSwitchDeviceClassId, oneWireDevice.type, "One wire single channel switch", parentDevice->id());
                ParamList params;
                params.append(Param(singleChannelSwitchDeviceAddressParamTypeId, oneWireDevice.address));
                params.append(Param(singleChannelSwitchDeviceTypeParamTypeId, oneWireDevice.type));
                foreach (Device *existingDevice, myDevices().filterByDeviceClassId(singleChannelSwitchDeviceClassId)){
                    if (existingDevice->paramValue(singleChannelSwitchDeviceAddressParamTypeId).toString() == oneWireDevice.address) {
                        descriptor.setDeviceId(existingDevice->id());
                        break;
                    }
                }
                descriptor.setParams(params);
                descriptors.append(descriptor);
                break;
            }
            case 0x12:
            case 0x3a: {//dual channel switch
                DeviceDescriptor descriptor(dualChannelSwitchDeviceClassId, oneWireDevice.type, "One wire dual channel switch", parentDevice->id());
                ParamList params;
                params.append(Param(dualChannelSwitchDeviceAddressParamTypeId, oneWireDevice.address));
                params.append(Param(dualChannelSwitchDeviceTypeParamTypeId, oneWireDevice.type));
                foreach (Device *existingDevice, myDevices().filterByDeviceClassId(dualChannelSwitchDeviceClassId)){
                    if (existingDevice->paramValue(dualChannelSwitchDeviceAddressParamTypeId).toString() == oneWireDevice.address) {
                        descriptor.setDeviceId(existingDevice->id());
                        break;
                    }
                }
                descriptor.setParams(params);
                descriptors.append(descriptor);
                break;
            }
            case 0x29: { //eight channel switch
                DeviceDescriptor descriptor(eightChannelSwitchDeviceClassId, oneWireDevice.type, "One wire eight channel switch", parentDevice->id());
                ParamList params;
                params.append(Param(eightChannelSwitchDeviceAddressParamTypeId, oneWireDevice.address));
                params.append(Param(eightChannelSwitchDeviceTypeParamTypeId, oneWireDevice.type));
                foreach (Device *existingDevice, myDevices().filterByDeviceClassId(eightChannelSwitchDeviceClassId)){
                    if (existingDevice->paramValue(eightChannelSwitchDeviceAddressParamTypeId).toString() == oneWireDevice.address) {
                        descriptor.setDeviceId(existingDevice->id());
                        break;
                    }
                }
                descriptor.setParams(params);
                descriptors.append(descriptor);
                break;
            }
            default:
                qDebug(dcOneWire()) << "Unknown Device discovered" << oneWireDevice.type << oneWireDevice.address;
                break;

            }
        }
        if (autoDiscoverEnabled) {
            emit autoDevicesAppeared(descriptors);
        } else {

            while (!m_runningDiscoveries.isEmpty()) {
                DeviceDiscoveryInfo *info = m_runningDiscoveries.takeFirst();
                info->addDeviceDescriptors(descriptors);
                info->finish(Device::DeviceErrorNoError);
            }
        }
        break;
    }
}
