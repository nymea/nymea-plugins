/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *                                                                         *
 *  Copyright (C) 2019 Bernhard Trinnes <bernhard.trinnes@nymea.io>        *
 *  Copyright (C) 2018 Simon Stürz <simon.stuerz@nymea.io>                 *
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

#include "devicepluginunipi.h"
#include "plugininfo.h"
#include "devices/devicemanager.h"
#include "devices/deviceplugin.h"
#include "devices/device.h"

#include <QJsonDocument>
#include <QTimer>
#include <QSerialPort>

DevicePluginUniPi::DevicePluginUniPi()
{
}


void DevicePluginUniPi::init()
{
    connect(this, &DevicePluginUniPi::configValueChanged, this, &DevicePluginUniPi::onPluginConfigurationChanged);
    QLoggingCategory::setFilterRules(QStringLiteral("qt.modbus* = false"));
}

Device::DeviceError DevicePluginUniPi::discoverDevices(const DeviceClassId &deviceClassId, const ParamList &params)
{
    Q_UNUSED(params);

    if (deviceClassId == digitalInputDeviceClassId) {
        QList<DeviceDescriptor> deviceDescriptors;
        foreach(Device *parentDevice, myDevices()) {
            if ((parentDevice->deviceClassId() == uniPi1DeviceClassId) || (parentDevice->deviceClassId() == uniPi1LiteDeviceClassId)) {
                foreach (QString circuit, m_unipi->digitalInputs()) {
                    DeviceDescriptor deviceDescriptor(digitalInputDeviceClassId, QString("Digital input %1").arg(circuit), "UniPi 1", parentDevice->id());
                    foreach(Device *device, myDevices().filterByParam(digitalInputDeviceParentIdParamTypeId, parentDevice->id())) {
                        if (device->paramValue(digitalInputDeviceCircuitParamTypeId) == circuit) {
                            qCDebug(dcUniPi()) << "Found already added Circuit:" << circuit << parentDevice->id();
                            deviceDescriptor.setDeviceId(parentDevice->id());
                            break;
                        }
                    }
                    ParamList params;
                    params.append(Param(digitalInputDeviceCircuitParamTypeId, circuit));
                    params.append(Param(digitalInputDeviceParentIdParamTypeId, parentDevice->id()));
                    deviceDescriptor.setParams(params);
                    deviceDescriptors.append(deviceDescriptor);
                }
                break;
            }
        }

        foreach (NeuronExtension *neuronExtension, m_neuronExtensions) {
            DeviceId parentDeviceId = m_neuronExtensions.key(neuronExtension);
            foreach (QString circuit, neuronExtension->digitalInputs()) {
                DeviceDescriptor deviceDescriptor(digitalInputDeviceClassId, QString("Digital input %1").arg(circuit), QString("Neuron extension %1, slave address %2").arg(neuronExtension->type().arg(QString::number(neuronExtension->slaveAddress()))), parentDeviceId);
                foreach(Device *device, myDevices().filterByParam(digitalInputDeviceParentIdParamTypeId, m_neuronExtensions.key(neuronExtension))) {
                    if (device->paramValue(digitalInputDeviceCircuitParamTypeId) == circuit) {
                        qCDebug(dcUniPi()) << "Found already added Circuit:" << circuit << parentDeviceId;
                        deviceDescriptor.setDeviceId(parentDeviceId);
                        break;
                    }
                }
                ParamList params;
                params.append(Param(digitalInputDeviceCircuitParamTypeId, circuit));
                params.append(Param(digitalInputDeviceParentIdParamTypeId, parentDeviceId));
                deviceDescriptor.setParams(params);
                deviceDescriptors.append(deviceDescriptor);
            }
        }

        foreach (Neuron *neuron, m_neurons) {
            DeviceId parentDeviceId = m_neurons.key(neuron);
            foreach (QString circuit, neuron->digitalInputs()) {
                DeviceDescriptor deviceDescriptor(digitalInputDeviceClassId, QString("Digital input %1").arg(circuit), QString("Neuron %1").arg(neuron->type()), parentDeviceId);
                foreach(Device *device, myDevices().filterByParam(digitalInputDeviceParentIdParamTypeId, m_neurons.key(neuron))) {
                    if (device->paramValue(digitalInputDeviceCircuitParamTypeId) == circuit) {
                        qCDebug(dcUniPi()) << "Found already added Circuit:" << circuit << parentDeviceId;
                        deviceDescriptor.setDeviceId(parentDeviceId);
                        break;
                    }
                }
                ParamList params;
                params.append(Param(digitalInputDeviceCircuitParamTypeId, circuit));
                params.append(Param(digitalInputDeviceParentIdParamTypeId, parentDeviceId));
                deviceDescriptor.setParams(params);
                deviceDescriptors.append(deviceDescriptor);
            }
        }
        if (!deviceDescriptors.isEmpty()) {
            emit devicesDiscovered(digitalInputDeviceClassId, deviceDescriptors);
        } else {
            return Device::DeviceErrorNoError;
        }
    }

    if (deviceClassId == digitalOutputDeviceClassId) {
        QList<DeviceDescriptor> deviceDescriptors;
        foreach(Device *parentDevice, myDevices()) {
            if ((parentDevice->deviceClassId() == uniPi1DeviceClassId) || (parentDevice->deviceClassId() == uniPi1LiteDeviceClassId)) {
                foreach (QString circuit, m_unipi->digitalOutputs()) {
                    DeviceDescriptor deviceDescriptor(digitalOutputDeviceClassId, QString("Digital output %1").arg(circuit), "UniPi 1", parentDevice->id());
                    foreach(Device *device, myDevices().filterByParam(digitalOutputDeviceParentIdParamTypeId, parentDevice->id())) {
                        if (device->paramValue(digitalOutputDeviceCircuitParamTypeId) == circuit) {
                            qCDebug(dcUniPi()) << "Found already added Circuit:" << circuit << parentDevice->id();
                            deviceDescriptor.setDeviceId(parentDevice->id());
                            break;
                        }
                    }
                    ParamList params;
                    params.append(Param(digitalOutputDeviceCircuitParamTypeId, circuit));
                    params.append(Param(digitalOutputDeviceParentIdParamTypeId, parentDevice->id()));
                    deviceDescriptor.setParams(params);
                    deviceDescriptors.append(deviceDescriptor);
                }
                break;
            }
        }

        foreach (NeuronExtension *neuronExtension, m_neuronExtensions) {
            DeviceId parentDeviceId = m_neuronExtensions.key(neuronExtension);
            foreach (QString circuit, neuronExtension->digitalOutputs()) {
                DeviceDescriptor deviceDescriptor(digitalOutputDeviceClassId, QString("Digital output %1").arg(circuit), QString("Neuron extension %1, Slave address %2").arg(neuronExtension->type().arg(QString::number(neuronExtension->slaveAddress()))), parentDeviceId);
                foreach(Device *device, myDevices().filterByParam(digitalOutputDeviceParentIdParamTypeId, m_neuronExtensions.key(neuronExtension))) {
                    if (device->paramValue(digitalOutputDeviceCircuitParamTypeId) == circuit) {
                        qCDebug(dcUniPi()) << "Found already added Circuit:" << circuit << parentDeviceId;
                        deviceDescriptor.setDeviceId(parentDeviceId);
                        break;
                    }
                }
                ParamList params;
                params.append(Param(digitalOutputDeviceCircuitParamTypeId, circuit));
                params.append(Param(digitalOutputDeviceParentIdParamTypeId, parentDeviceId));
                deviceDescriptor.setParams(params);
                deviceDescriptors.append(deviceDescriptor);
            }
        }

        foreach (Neuron *neuron, m_neurons) {
            DeviceId parentDeviceId = m_neurons.key(neuron);
            foreach (QString circuit, neuron->digitalOutputs()) {
                DeviceDescriptor deviceDescriptor(digitalOutputDeviceClassId, QString("Digital output %1").arg(circuit), QString("Neuron %1").arg(neuron->type()), parentDeviceId);
                foreach(Device *device, myDevices().filterByParam(digitalOutputDeviceParentIdParamTypeId, m_neurons.key(neuron))) {
                    if (device->paramValue(digitalOutputDeviceCircuitParamTypeId) == circuit) {
                        qCDebug(dcUniPi()) << "Found already added Circuit:" << circuit << parentDeviceId;
                        deviceDescriptor.setDeviceId(parentDeviceId);
                        break;
                    }
                }
                ParamList params;
                params.append(Param(digitalOutputDeviceCircuitParamTypeId, circuit));
                params.append(Param(digitalOutputDeviceParentIdParamTypeId, parentDeviceId));
                deviceDescriptor.setParams(params);
                deviceDescriptors.append(deviceDescriptor);
            }
        }
        if (!deviceDescriptors.isEmpty()) {
            emit devicesDiscovered(digitalOutputDeviceClassId, deviceDescriptors);
        } else {
            return Device::DeviceErrorNoError;
        }
    }

    if (deviceClassId == analogInputDeviceClassId) {
        QList<DeviceDescriptor> deviceDescriptors;
        foreach(Device *parentDevice, myDevices()) {
            if ((parentDevice->deviceClassId() == uniPi1DeviceClassId) || (parentDevice->deviceClassId() == uniPi1LiteDeviceClassId)) {
                foreach (QString circuit, m_unipi->analogInputs()) {
                    DeviceDescriptor deviceDescriptor(analogInputDeviceClassId, QString("Analog input %1").arg(circuit), "UniPi", parentDevice->id());
                    foreach(Device *device, myDevices().filterByParam(analogInputDeviceParentIdParamTypeId, parentDevice->id())) {
                        if (device->paramValue(analogInputDeviceCircuitParamTypeId) == circuit) {
                            qCDebug(dcUniPi()) << "Found already added Circuit:" << circuit << parentDevice->id();
                            deviceDescriptor.setDeviceId(parentDevice->id());
                            break;
                        }
                    }
                    ParamList params;
                    params.append(Param(analogInputDeviceCircuitParamTypeId, circuit));
                    params.append(Param(analogInputDeviceParentIdParamTypeId, parentDevice->id()));
                    deviceDescriptor.setParams(params);
                    deviceDescriptors.append(deviceDescriptor);
                }
                break;
            }
        }

        foreach (NeuronExtension *neuronExtension, m_neuronExtensions) {
            DeviceId parentDeviceId = m_neuronExtensions.key(neuronExtension);
            foreach (QString circuit, neuronExtension->analogInputs()) {
                DeviceDescriptor deviceDescriptor(analogInputDeviceClassId, QString("Analog input %1").arg(circuit), QString("Neuron extension %1, Slave address %2").arg(neuronExtension->type().arg(QString::number(neuronExtension->slaveAddress()))), parentDeviceId);
                foreach(Device *device, myDevices().filterByParam(analogInputDeviceParentIdParamTypeId, m_neuronExtensions.key(neuronExtension))) {
                    if (device->paramValue(analogInputDeviceCircuitParamTypeId) == circuit) {
                        qCDebug(dcUniPi()) << "Found already added Circuit:" << circuit << parentDeviceId;
                        deviceDescriptor.setDeviceId(parentDeviceId);
                        break;
                    }
                }
                ParamList params;
                params.append(Param(analogInputDeviceCircuitParamTypeId, circuit));
                params.append(Param(analogInputDeviceParentIdParamTypeId, parentDeviceId));
                deviceDescriptor.setParams(params);
                deviceDescriptors.append(deviceDescriptor);
            }
        }

        foreach (Neuron *neuron, m_neurons) {
            DeviceId parentDeviceId = m_neurons.key(neuron);
            foreach (QString circuit, neuron->analogInputs()) {
                DeviceDescriptor deviceDescriptor(analogInputDeviceClassId, QString("Analog input %1").arg(circuit), QString("Neuron %1").arg(neuron->type()), parentDeviceId);
                foreach(Device *device, myDevices().filterByParam(analogInputDeviceParentIdParamTypeId, m_neurons.key(neuron))) {
                    if (device->paramValue(analogInputDeviceCircuitParamTypeId) == circuit) {
                        qCDebug(dcUniPi()) << "Found already added Circuit:" << circuit << parentDeviceId;
                        deviceDescriptor.setDeviceId(parentDeviceId);
                        break;
                    }
                }
                ParamList params;
                params.append(Param(analogInputDeviceCircuitParamTypeId, circuit));
                params.append(Param(analogInputDeviceParentIdParamTypeId, parentDeviceId));
                deviceDescriptor.setParams(params);
                deviceDescriptors.append(deviceDescriptor);
            }
        }
        if (!deviceDescriptors.isEmpty()) {
            emit devicesDiscovered(analogInputDeviceClassId, deviceDescriptors);
        } else {
            return Device::DeviceErrorNoError;
        }
    }

    if (deviceClassId == analogOutputDeviceClassId) {
        //TODO add unipi 1 and unipi 1 lite

        QList<DeviceDescriptor> deviceDescriptors;
        foreach (NeuronExtension *neuronExtension, m_neuronExtensions) {
            DeviceId parentDeviceId = m_neuronExtensions.key(neuronExtension);
            foreach (QString circuit, neuronExtension->analogOutputs()) {
                DeviceDescriptor deviceDescriptor(analogOutputDeviceClassId, QString("Analog output %1").arg(circuit), QString("Neuron extension %1, Slave address %2").arg(neuronExtension->type().arg(QString::number(neuronExtension->slaveAddress()))), parentDeviceId);
                foreach(Device *device, myDevices().filterByParam(analogOutputDeviceParentIdParamTypeId, m_neuronExtensions.key(neuronExtension))) {
                    if (device->paramValue(analogOutputDeviceCircuitParamTypeId) == circuit) {
                        qCDebug(dcUniPi()) << "Found already added Circuit:" << circuit << parentDeviceId;
                        deviceDescriptor.setDeviceId(parentDeviceId);
                        break;
                    }
                }
                ParamList params;
                params.append(Param(analogOutputDeviceCircuitParamTypeId, circuit));
                params.append(Param(analogOutputDeviceParentIdParamTypeId, parentDeviceId));
                deviceDescriptor.setParams(params);
                deviceDescriptors.append(deviceDescriptor);
            }
        }

        foreach (Neuron *neuron, m_neurons) {
            DeviceId parentDeviceId = m_neurons.key(neuron);
            foreach (QString circuit, neuron->analogOutputs()) {
                DeviceDescriptor deviceDescriptor(analogOutputDeviceClassId, QString("Analog output %1").arg(circuit), QString("Neuron %1").arg(neuron->type()), parentDeviceId);
                foreach(Device *device, myDevices().filterByParam(analogOutputDeviceParentIdParamTypeId, m_neurons.key(neuron))) {
                    if (device->paramValue(analogOutputDeviceCircuitParamTypeId) == circuit) {
                        qCDebug(dcUniPi()) << "Found already added Circuit:" << circuit << parentDeviceId;
                        deviceDescriptor.setDeviceId(parentDeviceId);
                        break;
                    }
                }
                ParamList params;
                params.append(Param(analogOutputDeviceCircuitParamTypeId, circuit));
                params.append(Param(analogOutputDeviceParentIdParamTypeId, parentDeviceId));
                deviceDescriptor.setParams(params);
                deviceDescriptors.append(deviceDescriptor);
            }
        }
        if (!deviceDescriptors.isEmpty()) {
            emit devicesDiscovered(analogOutputDeviceClassId, deviceDescriptors);
        } else {
            return Device::DeviceErrorNoError;
        }
    }

    if (deviceClassId == userLEDDeviceClassId) {
        QList<DeviceDescriptor> deviceDescriptors;
        foreach (NeuronExtension *neuronExtension, m_neuronExtensions) {
            DeviceId parentDeviceId = m_neuronExtensions.key(neuronExtension);
            foreach (QString circuit, neuronExtension->userLEDs()) {
                DeviceDescriptor deviceDescriptor(userLEDDeviceClassId, QString("User programmable LED %1").arg(circuit), QString("Neuron extension %1, Slave address %2").arg(neuronExtension->type().arg(neuronExtension->slaveAddress())), parentDeviceId);
                foreach(Device *device, myDevices().filterByParam(userLEDDeviceParentIdParamTypeId, m_neuronExtensions.key(neuronExtension))) {
                    if (device->paramValue(userLEDDeviceCircuitParamTypeId) == circuit) {
                        qCDebug(dcUniPi()) << "Found already added Circuit:" << circuit << parentDeviceId;
                        deviceDescriptor.setDeviceId(parentDeviceId);
                        break;
                    }
                }
                ParamList params;
                params.append(Param(userLEDDeviceCircuitParamTypeId, circuit));
                params.append(Param(userLEDDeviceParentIdParamTypeId, parentDeviceId));
                deviceDescriptor.setParams(params);
                deviceDescriptors.append(deviceDescriptor);
            }
        }

        foreach (Neuron *neuron, m_neurons) {
            DeviceId parentDeviceId = m_neurons.key(neuron);
            foreach (QString circuit, neuron->userLEDs()) {
                DeviceDescriptor deviceDescriptor(userLEDDeviceClassId, QString("User programmable LED %1").arg(circuit), QString("Neuron %1").arg(neuron->type()), parentDeviceId);
                foreach(Device *device, myDevices().filterByParam(userLEDDeviceParentIdParamTypeId, m_neurons.key(neuron))) {
                    if (device->paramValue(userLEDDeviceCircuitParamTypeId) == circuit) {
                        qCDebug(dcUniPi()) << "Found already added Circuit:" << circuit << parentDeviceId;
                        deviceDescriptor.setDeviceId(parentDeviceId);
                        break;
                    }
                }
                ParamList params;
                params.append(Param(userLEDDeviceCircuitParamTypeId, circuit));
                params.append(Param(userLEDDeviceParentIdParamTypeId, parentDeviceId));
                deviceDescriptor.setParams(params);
                deviceDescriptors.append(deviceDescriptor);
            }
        }
        if (!deviceDescriptors.isEmpty()) {
            emit devicesDiscovered(userLEDDeviceClassId, deviceDescriptors);
        } else {
            return Device::DeviceErrorNoError;
        }
    }
    return Device::DeviceErrorAsync;
}

Device::DeviceSetupStatus DevicePluginUniPi::setupDevice(Device *device)
{
    if (!m_reconnectTimer) {
        m_reconnectTimer = new QTimer(this);
        m_reconnectTimer->setSingleShot(true);
        connect(m_reconnectTimer, &QTimer::timeout, this, &DevicePluginUniPi::onReconnectTimer);
    }


    if(device->deviceClassId() == uniPi1DeviceClassId) {
        if (m_unipi)
            return Device::DeviceSetupStatusFailure; //only one parent device allowed

        m_unipi = new UniPi(UniPi::UniPiType::UniPi1, this);
        if (!m_unipi->init()) {
            qCWarning(dcUniPi()) << "Could not setup UniPi";
            m_unipi->deleteLater();
            return Device::DeviceSetupStatusFailure;
        }
        connect(m_unipi, &UniPi::digitalInputStatusChanged, this, &DevicePluginUniPi::onUniPiDigitalInputStatusChanged);
        connect(m_unipi, &UniPi::digitalOutputStatusChanged, this, &DevicePluginUniPi::onUniPiDigitalOutputStatusChanged);
        connect(m_unipi, &UniPi::analogInputStatusChanged, this, &DevicePluginUniPi::onUniPiAnalogInputStatusChanged);
        connect(m_unipi, &UniPi::analogOutputStatusChanged, this, &DevicePluginUniPi::onUniPiAnalogOutputStatusChanged);
        device->setStateValue(uniPi1ConnectedStateTypeId, true);

        return Device::DeviceSetupStatusSuccess;
    }
    if(device->deviceClassId() == uniPi1LiteDeviceClassId) {
        if (m_unipi)
            return Device::DeviceSetupStatusFailure; //only one parent device allowed

        m_unipi = new UniPi(UniPi::UniPiType::UniPi1Lite, this);
        if (!m_unipi->init()) {
            qCWarning(dcUniPi()) << "Could not setup UniPi";
            m_unipi->deleteLater();
            return Device::DeviceSetupStatusFailure;
        }
        connect(m_unipi, &UniPi::digitalInputStatusChanged, this, &DevicePluginUniPi::onUniPiDigitalInputStatusChanged);
        connect(m_unipi, &UniPi::digitalOutputStatusChanged, this, &DevicePluginUniPi::onUniPiDigitalOutputStatusChanged);
        connect(m_unipi, &UniPi::analogInputStatusChanged, this, &DevicePluginUniPi::onUniPiAnalogInputStatusChanged);
        connect(m_unipi, &UniPi::analogOutputStatusChanged, this, &DevicePluginUniPi::onUniPiAnalogOutputStatusChanged);
        device->setStateValue(uniPi1LiteConnectedStateTypeId, true);

        return Device::DeviceSetupStatusSuccess;
    }
    if(device->deviceClassId() == neuronS103DeviceClassId) {
        if (!neuronDeviceInit())
            return Device::DeviceSetupStatusFailure;

        Neuron *neuron = new Neuron(Neuron::NeuronTypes::S103, m_modbusTCPMaster, this);
        if (!neuron->init()) {
            qCWarning(dcUniPi()) << "Could not load the modbus map";
            neuron->deleteLater();
            return Device::DeviceSetupStatusFailure;
        }
        m_neurons.insert(device->id(), neuron);
        connect(neuron, &Neuron::digitalInputStatusChanged, this, &DevicePluginUniPi::onNeuronDigitalInputStatusChanged);
        connect(neuron, &Neuron::digitalOutputStatusChanged, this, &DevicePluginUniPi::onNeuronDigitalOutputStatusChanged);
        connect(neuron, &Neuron::analogInputStatusChanged, this, &DevicePluginUniPi::onNeuronAnalogInputStatusChanged);
        connect(neuron, &Neuron::analogOutputStatusChanged, this, &DevicePluginUniPi::onNeuronAnalogOutputStatusChanged);
        connect(neuron, &Neuron::userLEDStatusChanged, this, &DevicePluginUniPi::onNeuronUserLEDStatusChanged);

        device->setStateValue(neuronS103ConnectedStateTypeId, true);

        return Device::DeviceSetupStatusSuccess;
    }

    if(device->deviceClassId() == neuronM103DeviceClassId) {
        if (!neuronDeviceInit())
            return Device::DeviceSetupStatusFailure;

        Neuron *neuron = new Neuron(Neuron::NeuronTypes::M103, m_modbusTCPMaster, this);
        if (!neuron->init()) {
            qCWarning(dcUniPi()) << "Could not load the modbus map";
            neuron->deleteLater();
            return Device::DeviceSetupStatusFailure;
        }
        m_neurons.insert(device->id(), neuron);
        connect(neuron, &Neuron::digitalInputStatusChanged, this, &DevicePluginUniPi::onNeuronDigitalInputStatusChanged);
        connect(neuron, &Neuron::digitalOutputStatusChanged, this, &DevicePluginUniPi::onNeuronDigitalOutputStatusChanged);
        connect(neuron, &Neuron::analogInputStatusChanged, this, &DevicePluginUniPi::onNeuronAnalogInputStatusChanged);
        connect(neuron, &Neuron::analogOutputStatusChanged, this, &DevicePluginUniPi::onNeuronAnalogOutputStatusChanged);
        connect(neuron, &Neuron::userLEDStatusChanged, this, &DevicePluginUniPi::onNeuronUserLEDStatusChanged);

        device->setStateValue(neuronM103ConnectedStateTypeId, true);

        return Device::DeviceSetupStatusSuccess;
    }

    if(device->deviceClassId() == neuronM203DeviceClassId) {
        if (!neuronDeviceInit())
            return Device::DeviceSetupStatusFailure;

        Neuron *neuron = new Neuron(Neuron::NeuronTypes::M203, m_modbusTCPMaster, this);
        if (!neuron->init()) {
            qCWarning(dcUniPi()) << "Could not load the modbus map";
            neuron->deleteLater();
            return Device::DeviceSetupStatusFailure;
        }
        m_neurons.insert(device->id(), neuron);
        connect(neuron, &Neuron::digitalInputStatusChanged, this, &DevicePluginUniPi::onNeuronDigitalInputStatusChanged);
        connect(neuron, &Neuron::digitalOutputStatusChanged, this, &DevicePluginUniPi::onNeuronDigitalOutputStatusChanged);
        connect(neuron, &Neuron::analogInputStatusChanged, this, &DevicePluginUniPi::onNeuronAnalogInputStatusChanged);
        connect(neuron, &Neuron::analogOutputStatusChanged, this, &DevicePluginUniPi::onNeuronAnalogOutputStatusChanged);
        connect(neuron, &Neuron::userLEDStatusChanged, this, &DevicePluginUniPi::onNeuronUserLEDStatusChanged);

        device->setStateValue(neuronM203ConnectedStateTypeId, true);

        return Device::DeviceSetupStatusSuccess;
    }

    if(device->deviceClassId() == neuronM303DeviceClassId) {
        if (!neuronDeviceInit())
            return Device::DeviceSetupStatusFailure;

        Neuron *neuron = new Neuron(Neuron::NeuronTypes::M303, m_modbusTCPMaster, this);
        if (!neuron->init()) {
            qCWarning(dcUniPi()) << "Could not load the modbus map";
            neuron->deleteLater();
            return Device::DeviceSetupStatusFailure;
        }
        m_neurons.insert(device->id(), neuron);
        connect(neuron, &Neuron::digitalInputStatusChanged, this, &DevicePluginUniPi::onNeuronDigitalInputStatusChanged);
        connect(neuron, &Neuron::digitalOutputStatusChanged, this, &DevicePluginUniPi::onNeuronDigitalOutputStatusChanged);
        connect(neuron, &Neuron::analogInputStatusChanged, this, &DevicePluginUniPi::onNeuronAnalogInputStatusChanged);
        connect(neuron, &Neuron::analogOutputStatusChanged, this, &DevicePluginUniPi::onNeuronAnalogOutputStatusChanged);
        connect(neuron, &Neuron::userLEDStatusChanged, this, &DevicePluginUniPi::onNeuronUserLEDStatusChanged);

        device->setStateValue(neuronM303ConnectedStateTypeId, true);

        return Device::DeviceSetupStatusSuccess;
    }

    if(device->deviceClassId() == neuronM403DeviceClassId) {
        if (!neuronDeviceInit())
            return Device::DeviceSetupStatusFailure;

        Neuron *neuron = new Neuron(Neuron::NeuronTypes::M403, m_modbusTCPMaster, this);
        if (!neuron->init()) {
            qCWarning(dcUniPi()) << "Could not load the modbus map";
            neuron->deleteLater();
            return Device::DeviceSetupStatusFailure;
        }
        m_neurons.insert(device->id(), neuron);
        connect(neuron, &Neuron::digitalInputStatusChanged, this, &DevicePluginUniPi::onNeuronDigitalInputStatusChanged);
        connect(neuron, &Neuron::digitalOutputStatusChanged, this, &DevicePluginUniPi::onNeuronDigitalOutputStatusChanged);
        connect(neuron, &Neuron::analogInputStatusChanged, this, &DevicePluginUniPi::onNeuronAnalogInputStatusChanged);
        connect(neuron, &Neuron::analogOutputStatusChanged, this, &DevicePluginUniPi::onNeuronAnalogOutputStatusChanged);
        connect(neuron, &Neuron::userLEDStatusChanged, this, &DevicePluginUniPi::onNeuronUserLEDStatusChanged);

        device->setStateValue(neuronM403ConnectedStateTypeId, true);

        return Device::DeviceSetupStatusSuccess;
    }

    if(device->deviceClassId() == neuronM503DeviceClassId) {
        if (!neuronDeviceInit())
            return Device::DeviceSetupStatusFailure;

        Neuron *neuron = new Neuron(Neuron::NeuronTypes::M503, m_modbusTCPMaster, this);
        if (!neuron->init()) {
            qCWarning(dcUniPi()) << "Could not load the modbus map";
            neuron->deleteLater();
            return Device::DeviceSetupStatusFailure;
        }
        m_neurons.insert(device->id(), neuron);
        connect(neuron, &Neuron::digitalInputStatusChanged, this, &DevicePluginUniPi::onNeuronDigitalInputStatusChanged);
        connect(neuron, &Neuron::digitalOutputStatusChanged, this, &DevicePluginUniPi::onNeuronDigitalOutputStatusChanged);
        connect(neuron, &Neuron::analogInputStatusChanged, this, &DevicePluginUniPi::onNeuronAnalogInputStatusChanged);
        connect(neuron, &Neuron::analogOutputStatusChanged, this, &DevicePluginUniPi::onNeuronAnalogOutputStatusChanged);
        connect(neuron, &Neuron::userLEDStatusChanged, this, &DevicePluginUniPi::onNeuronUserLEDStatusChanged);

        device->setStateValue(neuronM503ConnectedStateTypeId, true);

        return Device::DeviceSetupStatusSuccess;
    }

    if(device->deviceClassId() == neuronL203DeviceClassId) {
        if (!neuronDeviceInit())
            return Device::DeviceSetupStatusFailure;

        Neuron *neuron = new Neuron(Neuron::NeuronTypes::L203, m_modbusTCPMaster, this);
        if (!neuron->init()) {
            qCWarning(dcUniPi()) << "Could not load the modbus map";
            neuron->deleteLater();
            return Device::DeviceSetupStatusFailure;
        }
        m_neurons.insert(device->id(), neuron);
        connect(neuron, &Neuron::digitalInputStatusChanged, this, &DevicePluginUniPi::onNeuronDigitalInputStatusChanged);
        connect(neuron, &Neuron::digitalOutputStatusChanged, this, &DevicePluginUniPi::onNeuronDigitalOutputStatusChanged);
        connect(neuron, &Neuron::analogInputStatusChanged, this, &DevicePluginUniPi::onNeuronAnalogInputStatusChanged);
        connect(neuron, &Neuron::analogOutputStatusChanged, this, &DevicePluginUniPi::onNeuronAnalogOutputStatusChanged);
        connect(neuron, &Neuron::userLEDStatusChanged, this, &DevicePluginUniPi::onNeuronUserLEDStatusChanged);

        device->setStateValue(neuronL203ConnectedStateTypeId, true);

        return Device::DeviceSetupStatusSuccess;
    }

    if(device->deviceClassId() == neuronL303DeviceClassId) {
        if (!neuronDeviceInit())
            return Device::DeviceSetupStatusFailure;

        Neuron *neuron = new Neuron(Neuron::NeuronTypes::L303, m_modbusTCPMaster, this);
        if (!neuron->init()) {
            qCWarning(dcUniPi()) << "Could not load the modbus map";
            neuron->deleteLater();
            return Device::DeviceSetupStatusFailure;
        }
        m_neurons.insert(device->id(), neuron);
        connect(neuron, &Neuron::digitalInputStatusChanged, this, &DevicePluginUniPi::onNeuronDigitalInputStatusChanged);
        connect(neuron, &Neuron::digitalOutputStatusChanged, this, &DevicePluginUniPi::onNeuronDigitalOutputStatusChanged);
        connect(neuron, &Neuron::analogInputStatusChanged, this, &DevicePluginUniPi::onNeuronAnalogInputStatusChanged);
        connect(neuron, &Neuron::analogOutputStatusChanged, this, &DevicePluginUniPi::onNeuronAnalogOutputStatusChanged);
        connect(neuron, &Neuron::userLEDStatusChanged, this, &DevicePluginUniPi::onNeuronUserLEDStatusChanged);

        device->setStateValue(neuronL303ConnectedStateTypeId, true);

        return Device::DeviceSetupStatusSuccess;
    }

    if(device->deviceClassId() == neuronL403DeviceClassId) {
        if (!neuronDeviceInit())
            return Device::DeviceSetupStatusFailure;

        Neuron *neuron = new Neuron(Neuron::NeuronTypes::L403, m_modbusTCPMaster, this);
        if (!neuron->init()) {
            qCWarning(dcUniPi()) << "Could not load the modbus map";
            neuron->deleteLater();
            return Device::DeviceSetupStatusFailure;
        }
        m_neurons.insert(device->id(), neuron);
        connect(neuron, &Neuron::digitalInputStatusChanged, this, &DevicePluginUniPi::onNeuronDigitalInputStatusChanged);
        connect(neuron, &Neuron::digitalOutputStatusChanged, this, &DevicePluginUniPi::onNeuronDigitalOutputStatusChanged);
        connect(neuron, &Neuron::analogInputStatusChanged, this, &DevicePluginUniPi::onNeuronAnalogInputStatusChanged);
        connect(neuron, &Neuron::analogOutputStatusChanged, this, &DevicePluginUniPi::onNeuronAnalogOutputStatusChanged);
        connect(neuron, &Neuron::userLEDStatusChanged, this, &DevicePluginUniPi::onNeuronUserLEDStatusChanged);

        device->setStateValue(neuronL403ConnectedStateTypeId, true);

        return Device::DeviceSetupStatusSuccess;
    }

    if(device->deviceClassId() == neuronL503DeviceClassId) {
        if (!neuronDeviceInit())
            return Device::DeviceSetupStatusFailure;

        Neuron *neuron = new Neuron(Neuron::NeuronTypes::L503, m_modbusTCPMaster, this);
        if (!neuron->init()) {
            qCWarning(dcUniPi()) << "Could not load the modbus map";
            neuron->deleteLater();
            return Device::DeviceSetupStatusFailure;
        }
        m_neurons.insert(device->id(), neuron);
        connect(neuron, &Neuron::digitalInputStatusChanged, this, &DevicePluginUniPi::onNeuronDigitalInputStatusChanged);
        connect(neuron, &Neuron::digitalOutputStatusChanged, this, &DevicePluginUniPi::onNeuronDigitalOutputStatusChanged);
        connect(neuron, &Neuron::analogInputStatusChanged, this, &DevicePluginUniPi::onNeuronAnalogInputStatusChanged);
        connect(neuron, &Neuron::analogOutputStatusChanged, this, &DevicePluginUniPi::onNeuronAnalogOutputStatusChanged);
        connect(neuron, &Neuron::userLEDStatusChanged, this, &DevicePluginUniPi::onNeuronUserLEDStatusChanged);

        device->setStateValue(neuronL503ConnectedStateTypeId, true);

        return Device::DeviceSetupStatusSuccess;
    }

    if(device->deviceClassId() == neuronL513DeviceClassId) {
        if (!neuronDeviceInit())
            return Device::DeviceSetupStatusFailure;

        Neuron *neuron = new Neuron(Neuron::NeuronTypes::L513, m_modbusTCPMaster, this);
        if (!neuron->init()) {
            qCWarning(dcUniPi()) << "Could not load the modbus map";
            neuron->deleteLater();
            return Device::DeviceSetupStatusFailure;
        }
        m_neurons.insert(device->id(), neuron);
        connect(neuron, &Neuron::digitalInputStatusChanged, this, &DevicePluginUniPi::onNeuronDigitalInputStatusChanged);
        connect(neuron, &Neuron::digitalOutputStatusChanged, this, &DevicePluginUniPi::onNeuronDigitalOutputStatusChanged);
        connect(neuron, &Neuron::analogInputStatusChanged, this, &DevicePluginUniPi::onNeuronAnalogInputStatusChanged);
        connect(neuron, &Neuron::analogOutputStatusChanged, this, &DevicePluginUniPi::onNeuronAnalogOutputStatusChanged);
        connect(neuron, &Neuron::userLEDStatusChanged, this, &DevicePluginUniPi::onNeuronUserLEDStatusChanged);

        device->setStateValue(neuronL513ConnectedStateTypeId, true);

        return Device::DeviceSetupStatusSuccess;
    }

    if(device->deviceClassId() == neuronXS10DeviceClassId) {
        if (!neuronExtensionInterfaceInit())
            return Device::DeviceSetupStatusFailure;

        int slaveAddress = device->paramValue(neuronXS10DeviceSlaveAddressParamTypeId).toInt();
        NeuronExtension *neuronExtension = new NeuronExtension(NeuronExtension::ExtensionTypes::xS10, m_modbusRTUMaster, slaveAddress, this);
        if (!neuronExtension->init()) {
            qCWarning(dcUniPi()) << "Could not load the modbus map";
            neuronExtension->deleteLater();
            return Device::DeviceSetupStatusFailure;
        }
        connect(neuronExtension, &NeuronExtension::digitalInputStatusChanged, this, &DevicePluginUniPi::onNeuronExtensionDigitalInputStatusChanged);
        connect(neuronExtension, &NeuronExtension::digitalOutputStatusChanged, this, &DevicePluginUniPi::onNeuronExtensionDigitalOutputStatusChanged);
        connect(neuronExtension, &NeuronExtension::analogInputStatusChanged, this, &DevicePluginUniPi::onNeuronExtensionAnalogInputStatusChanged);
        connect(neuronExtension, &NeuronExtension::analogOutputStatusChanged, this, &DevicePluginUniPi::onNeuronExtensionAnalogOutputStatusChanged);
        connect(neuronExtension, &NeuronExtension::userLEDStatusChanged, this, &DevicePluginUniPi::onNeuronExtensionUserLEDStatusChanged);

        m_neuronExtensions.insert(device->id(), neuronExtension);
        device->setStateValue(neuronXS10ConnectedStateTypeId, true);

        return Device::DeviceSetupStatusSuccess;
    }

    if(device->deviceClassId() == neuronXS20DeviceClassId) {
        if (!neuronExtensionInterfaceInit())
            return Device::DeviceSetupStatusFailure;

        int slaveAddress = device->paramValue(neuronXS20DeviceSlaveAddressParamTypeId).toInt();
        NeuronExtension *neuronExtension = new NeuronExtension(NeuronExtension::ExtensionTypes::xS20, m_modbusRTUMaster, slaveAddress, this);
        if (!neuronExtension->init()) {
            qCWarning(dcUniPi()) << "Could not load the modbus map";
            neuronExtension->deleteLater();
            return Device::DeviceSetupStatusFailure;
        }
        connect(neuronExtension, &NeuronExtension::digitalInputStatusChanged, this, &DevicePluginUniPi::onNeuronExtensionDigitalInputStatusChanged);
        connect(neuronExtension, &NeuronExtension::digitalOutputStatusChanged, this, &DevicePluginUniPi::onNeuronExtensionDigitalOutputStatusChanged);
        connect(neuronExtension, &NeuronExtension::analogInputStatusChanged, this, &DevicePluginUniPi::onNeuronExtensionAnalogInputStatusChanged);
        connect(neuronExtension, &NeuronExtension::analogOutputStatusChanged, this, &DevicePluginUniPi::onNeuronExtensionAnalogOutputStatusChanged);
        connect(neuronExtension, &NeuronExtension::userLEDStatusChanged, this, &DevicePluginUniPi::onNeuronExtensionUserLEDStatusChanged);

        m_neuronExtensions.insert(device->id(), neuronExtension);
        device->setStateValue(neuronXS20ConnectedStateTypeId, true);

        return Device::DeviceSetupStatusSuccess;
    }

    if(device->deviceClassId() == neuronXS30DeviceClassId) {
        if (!neuronExtensionInterfaceInit())
            return Device::DeviceSetupStatusFailure;

        int slaveAddress = device->paramValue(neuronXS30DeviceSlaveAddressParamTypeId).toInt();
        NeuronExtension *neuronExtension = new NeuronExtension(NeuronExtension::ExtensionTypes::xS30, m_modbusRTUMaster, slaveAddress, this);
        if (!neuronExtension->init()) {
            qCWarning(dcUniPi()) << "Could not load the modbus map";
            neuronExtension->deleteLater();
            return Device::DeviceSetupStatusFailure;
        }
        connect(neuronExtension, &NeuronExtension::digitalInputStatusChanged, this, &DevicePluginUniPi::onNeuronExtensionDigitalInputStatusChanged);
        connect(neuronExtension, &NeuronExtension::digitalOutputStatusChanged, this, &DevicePluginUniPi::onNeuronExtensionDigitalOutputStatusChanged);
        connect(neuronExtension, &NeuronExtension::analogInputStatusChanged, this, &DevicePluginUniPi::onNeuronExtensionAnalogInputStatusChanged);
        connect(neuronExtension, &NeuronExtension::analogOutputStatusChanged, this, &DevicePluginUniPi::onNeuronExtensionAnalogOutputStatusChanged);
        connect(neuronExtension, &NeuronExtension::userLEDStatusChanged, this, &DevicePluginUniPi::onNeuronExtensionUserLEDStatusChanged);

        m_neuronExtensions.insert(device->id(), neuronExtension);
        device->setStateValue(neuronXS30ConnectedStateTypeId, true);

        return Device::DeviceSetupStatusSuccess;
    }

    if(device->deviceClassId() == neuronXS40DeviceClassId) {
        if (!neuronExtensionInterfaceInit())
            return Device::DeviceSetupStatusFailure;

        int slaveAddress = device->paramValue(neuronXS40DeviceSlaveAddressParamTypeId).toInt();
        NeuronExtension *neuronExtension = new NeuronExtension(NeuronExtension::ExtensionTypes::xS40, m_modbusRTUMaster, slaveAddress, this);
        if (!neuronExtension->init()) {
            qCWarning(dcUniPi()) << "Could not load the modbus map";
            neuronExtension->deleteLater();
            return Device::DeviceSetupStatusFailure;
        }
        connect(neuronExtension, &NeuronExtension::digitalInputStatusChanged, this, &DevicePluginUniPi::onNeuronExtensionDigitalInputStatusChanged);
        connect(neuronExtension, &NeuronExtension::digitalOutputStatusChanged, this, &DevicePluginUniPi::onNeuronExtensionDigitalOutputStatusChanged);
        connect(neuronExtension, &NeuronExtension::analogInputStatusChanged, this, &DevicePluginUniPi::onNeuronExtensionAnalogInputStatusChanged);
        connect(neuronExtension, &NeuronExtension::analogOutputStatusChanged, this, &DevicePluginUniPi::onNeuronExtensionAnalogOutputStatusChanged);
        connect(neuronExtension, &NeuronExtension::userLEDStatusChanged, this, &DevicePluginUniPi::onNeuronExtensionUserLEDStatusChanged);

        m_neuronExtensions.insert(device->id(), neuronExtension);
        device->setStateValue(neuronXS40ConnectedStateTypeId, true);

        return Device::DeviceSetupStatusSuccess;
    }

    if(device->deviceClassId() == neuronXS50DeviceClassId) {
        if (!neuronExtensionInterfaceInit())
            return Device::DeviceSetupStatusFailure;

        int slaveAddress = device->paramValue(neuronXS50DeviceSlaveAddressParamTypeId).toInt();
        NeuronExtension *neuronExtension = new NeuronExtension(NeuronExtension::ExtensionTypes::xS50, m_modbusRTUMaster, slaveAddress, this);
        if (!neuronExtension->init()) {
            qCWarning(dcUniPi()) << "Could not load the modbus map";
            neuronExtension->deleteLater();
            return Device::DeviceSetupStatusFailure;
        }
        connect(neuronExtension, &NeuronExtension::digitalInputStatusChanged, this, &DevicePluginUniPi::onNeuronExtensionDigitalInputStatusChanged);
        connect(neuronExtension, &NeuronExtension::digitalOutputStatusChanged, this, &DevicePluginUniPi::onNeuronExtensionDigitalOutputStatusChanged);
        connect(neuronExtension, &NeuronExtension::analogInputStatusChanged, this, &DevicePluginUniPi::onNeuronExtensionAnalogInputStatusChanged);
        connect(neuronExtension, &NeuronExtension::analogOutputStatusChanged, this, &DevicePluginUniPi::onNeuronExtensionAnalogOutputStatusChanged);
        connect(neuronExtension, &NeuronExtension::userLEDStatusChanged, this, &DevicePluginUniPi::onNeuronExtensionUserLEDStatusChanged);

        m_neuronExtensions.insert(device->id(), neuronExtension);
        device->setStateValue(neuronXS50ConnectedStateTypeId, true);

        return Device::DeviceSetupStatusSuccess;
    }

    if (device->deviceClassId() == digitalOutputDeviceClassId) {
        return Device::DeviceSetupStatusSuccess;
    }

    if (device->deviceClassId() == digitalInputDeviceClassId) {
        return Device::DeviceSetupStatusSuccess;
    }

    if (device->deviceClassId() == userLEDDeviceClassId) {
        return Device::DeviceSetupStatusSuccess;
    }

    if (device->deviceClassId() == analogInputDeviceClassId) {
        return Device::DeviceSetupStatusSuccess;
    }

    if (device->deviceClassId() == analogOutputDeviceClassId) {
        return Device::DeviceSetupStatusSuccess;
    }
    return Device::DeviceSetupStatusFailure;
}


Device::DeviceError DevicePluginUniPi::executeAction(Device *device, const Action &action)
{
    if (device->deviceClassId() == digitalOutputDeviceClassId)  {

        if (action.actionTypeId() == digitalOutputPowerActionTypeId) {
            QString digitalOutputNumber = device->paramValue(digitalOutputDeviceCircuitParamTypeId).toString();
            bool stateValue = action.param(digitalOutputPowerActionPowerParamTypeId).value().toBool();

            if (m_unipi) {
                m_unipi->setDigitalOutput(digitalOutputNumber, stateValue);
            }
            if (m_neurons.contains(device->paramValue(digitalOutputDeviceParentIdParamTypeId).toString())) {
                Neuron *neuron = m_neurons.value(device->paramValue(digitalOutputDeviceParentIdParamTypeId).toString());
                neuron->setDigitalOutput(digitalOutputNumber, stateValue);
            }
            if (m_neuronExtensions.contains(device->paramValue(digitalOutputDeviceParentIdParamTypeId).toString())) {
                NeuronExtension *neuronExtension = m_neuronExtensions.value(device->paramValue(digitalOutputDeviceParentIdParamTypeId).toString());
                neuronExtension->setDigitalOutput(digitalOutputNumber, stateValue);
            }
            return Device::DeviceErrorNoError;
        }
        return Device::DeviceErrorActionTypeNotFound;
    }

    if (device->deviceClassId() == analogOutputDeviceClassId) {

        if (action.actionTypeId() == analogOutputOutputValueActionTypeId) {
            QString analogOutputNumber = device->paramValue(analogOutputDeviceCircuitParamTypeId).toString();
            double analogValue = action.param(analogOutputOutputValueActionOutputValueParamTypeId).value().toDouble();

            if (m_unipi) {
                m_unipi->setAnalogOutput(analogOutputNumber, analogValue);
            }
            if (m_neurons.contains(device->paramValue(analogOutputDeviceParentIdParamTypeId).toString())) {
                Neuron *neuron = m_neurons.value(device->paramValue(analogOutputDeviceParentIdParamTypeId).toString());
                neuron->setAnalogOutput(analogOutputNumber, analogValue);
            }
            if (m_neuronExtensions.contains(device->paramValue(analogOutputDeviceParentIdParamTypeId).toString())) {
                NeuronExtension *neuronExtension = m_neuronExtensions.value(device->paramValue(analogOutputDeviceParentIdParamTypeId).toString());
                neuronExtension->setAnalogOutput(analogOutputNumber, analogValue);
            }
            return Device::DeviceErrorNoError;
        }
        return Device::DeviceErrorActionTypeNotFound;
    }

    if (device->deviceClassId() == userLEDDeviceClassId) {
        if (action.actionTypeId() == userLEDPowerActionTypeId) {
            QString userLED = device->paramValue(userLEDDeviceCircuitParamTypeId).toString();
            bool stateValue = action.param(userLEDPowerActionPowerParamTypeId).value().toBool();
            if (m_neurons.contains(device->paramValue(userLEDDeviceParentIdParamTypeId).toString())) {
                Neuron *neuron = m_neurons.value(device->paramValue(userLEDDeviceParentIdParamTypeId).toString());
                neuron->setUserLED(userLED, stateValue);
            }
            if (m_neuronExtensions.contains(device->paramValue(userLEDDeviceParentIdParamTypeId).toString())) {
                NeuronExtension *neuronExtension = m_neuronExtensions.value(device->paramValue(userLEDDeviceParentIdParamTypeId).toString());
                neuronExtension->setUserLED(userLED, stateValue);
            }
            return Device::DeviceErrorNoError;
        }
        return Device::DeviceErrorActionTypeNotFound;
    }
    return Device::DeviceErrorDeviceClassNotFound;
}


void DevicePluginUniPi::deviceRemoved(Device *device)
{
    Q_UNUSED(device);
    if(m_neurons.contains(device->id())) {
        Neuron *neuron = m_neurons.take(device->id());
        neuron->deleteLater();
    }
    if(m_neuronExtensions.contains(device->id())) {
        NeuronExtension *neuronExtension = m_neuronExtensions.take(device->id());
        neuronExtension->deleteLater();
    }

    if ((device->deviceClassId() == uniPi1DeviceClassId) || (device->deviceClassId() == uniPi1LiteDeviceClassId)) {
        if(m_unipi) {
            m_unipi->deleteLater();
        }
    }

    if (myDevices().isEmpty()) {
        m_reconnectTimer->stop();
        m_reconnectTimer->deleteLater();

        if (m_modbusTCPMaster) {
            m_modbusTCPMaster->disconnectDevice();
            m_modbusTCPMaster->deleteLater();
        }
        if (m_modbusRTUMaster) {
            m_modbusRTUMaster->disconnectDevice();
            m_modbusRTUMaster->deleteLater();
        }
    }
}

void DevicePluginUniPi::onPluginConfigurationChanged(const ParamTypeId &paramTypeId, const QVariant &value)
{
    qCDebug(dcUniPi()) << "Plugin configuration changed";
    if (paramTypeId == uniPiPluginPortParamTypeId) {
        if (m_modbusTCPMaster) {
            m_modbusTCPMaster->setConnectionParameter(QModbusDevice::NetworkAddressParameter, value.toString());
        }
    }

    if (paramTypeId == uniPiPluginAddressParamTypeId) {
        if (m_modbusTCPMaster) {
            m_modbusTCPMaster->setConnectionParameter(QModbusDevice::NetworkPortParameter, value.toInt());
        }
    }

    if (paramTypeId == uniPiPluginSerialPortParamTypeId) {
        if (m_modbusRTUMaster) {
            m_modbusRTUMaster->setConnectionParameter(QModbusDevice::SerialPortNameParameter, value.toString());
        }
    }

    if (paramTypeId == uniPiPluginBaudrateParamTypeId) {
        if (m_modbusRTUMaster) {
            m_modbusRTUMaster->setConnectionParameter(QModbusDevice::SerialBaudRateParameter, value.toInt());
        }
    }

    if (paramTypeId == uniPiPluginParityParamTypeId) {
        if (m_modbusRTUMaster) {
            if (value == "Even") {
                m_modbusRTUMaster->setConnectionParameter(QModbusDevice::SerialParityParameter, QSerialPort::Parity::EvenParity);
            } else {
                m_modbusRTUMaster->setConnectionParameter(QModbusDevice::SerialParityParameter, QSerialPort::Parity::NoParity);
            }
        }
    }
}

void DevicePluginUniPi::onNeuronDigitalInputStatusChanged(QString &circuit, bool value)
{
    Neuron *neuron = static_cast<Neuron *>(sender());

    foreach(Device *device, myDevices().filterByParam(digitalInputDeviceParentIdParamTypeId, m_neurons.key(neuron))) {
        if (device->deviceClassId() == digitalInputDeviceClassId) {
            if (device->paramValue(digitalInputDeviceCircuitParamTypeId).toString() == circuit) {

                device->setStateValue(digitalInputInputStatusStateTypeId, value);
                return;
            }
        }
    }
}

void DevicePluginUniPi::onNeuronExtensionDigitalInputStatusChanged(QString &circuit, bool value)
{
    NeuronExtension *neuronExtension = static_cast<NeuronExtension *>(sender());

    foreach(Device *device, myDevices().filterByParam(digitalInputDeviceParentIdParamTypeId, m_neuronExtensions.key(neuronExtension))) {
        if (device->deviceClassId() == digitalInputDeviceClassId) {
            if (device->paramValue(digitalInputDeviceCircuitParamTypeId).toString() == circuit) {

                device->setStateValue(digitalInputInputStatusStateTypeId, value);
                return;
            }
        }
    }
}

void DevicePluginUniPi::onNeuronDigitalOutputStatusChanged(QString &circuit, bool value)
{
    Neuron *neuron = static_cast<Neuron *>(sender());

    foreach(Device *device, myDevices().filterByParam(digitalOutputDeviceParentIdParamTypeId, m_neurons.key(neuron))) {
        if (device->deviceClassId() == digitalOutputDeviceClassId) {
            if (device->paramValue(digitalOutputDeviceCircuitParamTypeId).toString() == circuit) {

                device->setStateValue(digitalOutputPowerStateTypeId, value);
                return;
            }
        }
    }
}

void DevicePluginUniPi::onNeuronExtensionDigitalOutputStatusChanged(QString &circuit, bool value)
{
    NeuronExtension *neuronExtension = static_cast<NeuronExtension *>(sender());

    foreach(Device *device, myDevices().filterByParam(digitalOutputDeviceParentIdParamTypeId, m_neuronExtensions.key(neuronExtension))) {
        if (device->deviceClassId() == digitalOutputDeviceClassId) {
            if (device->paramValue(digitalOutputDeviceCircuitParamTypeId).toString() == circuit) {

                device->setStateValue(digitalOutputPowerStateTypeId, value);
                return;
            }
        }
    }
}

void DevicePluginUniPi::onNeuronAnalogInputStatusChanged(QString &circuit, double value)
{
    Neuron *neuron = static_cast<Neuron *>(sender());

    foreach(Device *device, myDevices().filterByParam(analogInputDeviceParentIdParamTypeId, m_neurons.key(neuron))) {
        if (device->deviceClassId() == analogInputDeviceClassId) {
            if (device->paramValue(analogInputDeviceCircuitParamTypeId).toString() == circuit) {

                device->setStateValue(analogInputInputValueStateTypeId, value);
                return;
            }
        }
    }
}

void DevicePluginUniPi::onNeuronExtensionAnalogInputStatusChanged(QString &circuit, double value)
{
    NeuronExtension *neuronExtension = static_cast<NeuronExtension *>(sender());

    foreach(Device *device, myDevices().filterByParam(analogInputDeviceParentIdParamTypeId, m_neuronExtensions.key(neuronExtension))) {
        if (device->deviceClassId() == analogInputDeviceClassId) {
            if (device->paramValue(analogInputDeviceCircuitParamTypeId).toString() == circuit) {
                device->setStateValue(analogInputInputValueStateTypeId, value);
                return;
            }
        }
    }
}

void DevicePluginUniPi::onNeuronAnalogOutputStatusChanged(QString &circuit, double value)
{
    Neuron *neuron = static_cast<Neuron *>(sender());

    foreach(Device *device, myDevices().filterByParam(analogOutputDeviceParentIdParamTypeId, m_neurons.key(neuron))) {
        if (device->deviceClassId() == analogOutputDeviceClassId) {
            if (device->paramValue(analogOutputDeviceCircuitParamTypeId).toString() == circuit) {

                device->setStateValue(analogOutputOutputValueStateTypeId, value);
                return;
            }
        }
    }
}

void DevicePluginUniPi::onNeuronExtensionAnalogOutputStatusChanged(QString &circuit, double value)
{
    NeuronExtension *neuronExtension = static_cast<NeuronExtension *>(sender());

    foreach(Device *device, myDevices().filterByParam(analogOutputDeviceParentIdParamTypeId, m_neuronExtensions.key(neuronExtension))) {
        if (device->deviceClassId() == analogOutputDeviceClassId) {
            if (device->paramValue(analogOutputDeviceCircuitParamTypeId).toString() == circuit) {

                device->setStateValue(analogOutputOutputValueStateTypeId, value);
                return;
            }
        }
    }
}

void DevicePluginUniPi::onNeuronUserLEDStatusChanged(QString &circuit, bool value)
{
    Neuron *neuron = static_cast<Neuron *>(sender());

    foreach(Device *device, myDevices().filterByParam(userLEDDeviceParentIdParamTypeId, m_neurons.key(neuron))) {
        if (device->deviceClassId() == userLEDDeviceClassId) {
            if (device->paramValue(userLEDDeviceCircuitParamTypeId).toString() == circuit) {

                device->setStateValue(userLEDPowerStateTypeId, value);
                return;
            }
        }
    }
}

void DevicePluginUniPi::onNeuronExtensionUserLEDStatusChanged(QString &circuit, bool value)
{
    NeuronExtension *neuronExtension = static_cast<NeuronExtension *>(sender());

    foreach(Device *device, myDevices().filterByParam(userLEDDeviceParentIdParamTypeId, m_neuronExtensions.key(neuronExtension))) {
        if (device->deviceClassId() == userLEDDeviceClassId) {
            if (device->paramValue(userLEDDeviceCircuitParamTypeId).toString() == circuit) {

                device->setStateValue(userLEDPowerStateTypeId, value);
                return;
            }
        }
    }
}

void DevicePluginUniPi::onReconnectTimer()
{
    if(m_modbusRTUMaster) {
        if (!m_modbusRTUMaster->connectDevice()) {
            m_reconnectTimer->start(10000);
        }
    }
    if(m_modbusTCPMaster) {
        if (!m_modbusTCPMaster->connectDevice()) {
            m_reconnectTimer->start(10000);
        }
    }
}


void DevicePluginUniPi::onModbusTCPErrorOccurred(QModbusDevice::Error error)
{
    qCWarning(dcUniPi()) << "An error occured" << error;
}

void DevicePluginUniPi::onModbusRTUErrorOccurred(QModbusDevice::Error error)
{
    qCWarning(dcUniPi()) << "An error occured" << error;
}

void DevicePluginUniPi::onModbusTCPStateChanged(QModbusDevice::State state)
{
    bool connected = (state != QModbusDevice::UnconnectedState);

    foreach (DeviceId deviceId, m_neurons.keys()) {
        Device *device = myDevices().findById(deviceId);
        if (device->deviceClassId() == neuronS103DeviceClassId)
            device->setStateValue(neuronM103ConnectedStateTypeId, connected);
        if (device->deviceClassId() == neuronM103DeviceClassId)
            device->setStateValue(neuronM103ConnectedStateTypeId, connected);
        if (device->deviceClassId() == neuronM203DeviceClassId)
            device->setStateValue(neuronM203ConnectedStateTypeId, connected);
        if (device->deviceClassId() == neuronM303DeviceClassId)
            device->setStateValue(neuronM303ConnectedStateTypeId, connected);
        if (device->deviceClassId() == neuronM403DeviceClassId)
            device->setStateValue(neuronM403ConnectedStateTypeId, connected);
        if (device->deviceClassId() == neuronM503DeviceClassId)
            device->setStateValue(neuronM503ConnectedStateTypeId, connected);
        if (device->deviceClassId() == neuronL203DeviceClassId)
            device->setStateValue(neuronL203ConnectedStateTypeId, connected);
        if (device->deviceClassId() == neuronL303DeviceClassId)
            device->setStateValue(neuronL303ConnectedStateTypeId, connected);
        if (device->deviceClassId() == neuronL403DeviceClassId)
            device->setStateValue(neuronL403ConnectedStateTypeId, connected);
        if (device->deviceClassId() == neuronL503DeviceClassId)
            device->setStateValue(neuronL503ConnectedStateTypeId, connected);
        if (device->deviceClassId() == neuronL513DeviceClassId)
            device->setStateValue(neuronL513ConnectedStateTypeId, connected);
    }

    if (!connected) {
        //try to reconnect in 10 seconds
        m_reconnectTimer->start(10000);
    }
    qCDebug(dcUniPi()) << "Connection status changed:" << connected;
}

void DevicePluginUniPi::onModbusRTUStateChanged(QModbusDevice::State state)
{
    bool connected = (state != QModbusDevice::UnconnectedState);

    foreach (DeviceId deviceId, m_neurons.keys()) {
        Device *device = myDevices().findById(deviceId);
        if (device->deviceClassId() == neuronXS10DeviceClassId)
            device->setStateValue(neuronXS10ConnectedStateTypeId, connected);
        if (device->deviceClassId() == neuronXS20DeviceClassId)
            device->setStateValue(neuronXS20ConnectedStateTypeId, connected);
        if (device->deviceClassId() == neuronXS30DeviceClassId)
            device->setStateValue(neuronXS30ConnectedStateTypeId, connected);
        if (device->deviceClassId() == neuronXS40DeviceClassId)
            device->setStateValue(neuronXS40ConnectedStateTypeId, connected);
    }

    if (!connected) {
        //try to reconnect in 10 seconds
        m_reconnectTimer->start(10000);
    }
    qCDebug(dcUniPi()) << "Connection status changed:" << connected;
}

void DevicePluginUniPi::onUniPiDigitalInputStatusChanged(const QString &circuit, bool value)
{
    foreach(Device *parentDevice, myDevices()) {
        if ((parentDevice->deviceClassId() == uniPi1DeviceClassId) || (parentDevice->deviceClassId() == uniPi1LiteDeviceClassId)) {
            foreach(Device *device, myDevices().filterByParam(digitalInputDeviceParentIdParamTypeId, parentDevice->id())) {
                if (device->deviceClassId() == digitalInputDeviceClassId) {
                    if (device->paramValue(digitalInputDeviceCircuitParamTypeId).toString() == circuit) {

                        device->setStateValue(digitalInputInputStatusStateTypeId, value);
                        return;
                    }
                }
            }
            break;
        }
    }
}

void DevicePluginUniPi::onUniPiDigitalOutputStatusChanged(const QString &circuit, bool value)
{
    foreach(Device *parentDevice, myDevices()) {
        if ((parentDevice->deviceClassId() == uniPi1DeviceClassId) || (parentDevice->deviceClassId() == uniPi1LiteDeviceClassId)) {
            foreach(Device *device, myDevices().filterByParam(digitalOutputDeviceParentIdParamTypeId, parentDevice->id())) {
                if (device->deviceClassId() == digitalOutputDeviceClassId) {
                    if (device->paramValue(digitalOutputDeviceCircuitParamTypeId).toString() == circuit) {

                        device->setStateValue(digitalOutputPowerStateTypeId, value);
                        return;
                    }
                }
            }
            break;
        }
    }
}

void DevicePluginUniPi::onUniPiAnalogInputStatusChanged(const QString &circuit, double value)
{
    foreach(Device *parentDevice, myDevices()) {
        if ((parentDevice->deviceClassId() == uniPi1DeviceClassId) || (parentDevice->deviceClassId() == uniPi1LiteDeviceClassId)) {
            foreach(Device *device, myDevices().filterByParam(analogInputDeviceParentIdParamTypeId, parentDevice->id())) {
                if (device->deviceClassId() == analogInputDeviceClassId) {
                    if (device->paramValue(analogInputDeviceCircuitParamTypeId).toString() == circuit) {
                        device->setStateValue(analogInputInputValueStateTypeId, value);
                        return;
                    }
                }
            }
            break;
        }
    }
}


void DevicePluginUniPi::onUniPiAnalogOutputStatusChanged(const QString &circuit, double value)
{
    foreach(Device *parentDevice, myDevices()) {
        if ((parentDevice->deviceClassId() == uniPi1DeviceClassId) || (parentDevice->deviceClassId() == uniPi1LiteDeviceClassId)) {
            foreach(Device *device, myDevices().filterByParam(analogOutputDeviceParentIdParamTypeId, parentDevice->id())) {
                if (device->deviceClassId() == analogOutputDeviceClassId) {
                    if (device->paramValue(analogOutputDeviceCircuitParamTypeId).toString() == circuit) {

                        device->setStateValue(analogOutputOutputValueStateTypeId, value);
                        return;
                    }
                }
            }
            break;
        }
    }
}

bool DevicePluginUniPi::neuronDeviceInit()
{
    if(!m_modbusTCPMaster) {
        int port = configValue(uniPiPluginPortParamTypeId).toInt();;
        QHostAddress ipAddress = QHostAddress(configValue(uniPiPluginAddressParamTypeId).toString());

        m_modbusTCPMaster = new QModbusTcpClient(this);
        m_modbusTCPMaster->setConnectionParameter(QModbusDevice::NetworkPortParameter, port);
        m_modbusTCPMaster->setConnectionParameter(QModbusDevice::NetworkAddressParameter, ipAddress.toString());
        //m_modbusTCPMaster->setTimeout(100);
        //m_modbusTCPMaster->setNumberOfRetries(1);

        connect(m_modbusTCPMaster, &QModbusTcpClient::stateChanged, this, &DevicePluginUniPi::onModbusTCPStateChanged);
        connect(m_modbusTCPMaster, &QModbusTcpClient::errorOccurred, this, &DevicePluginUniPi::onModbusTCPErrorOccurred);

        if (!m_modbusTCPMaster->connectDevice()) {
            qCWarning(dcUniPi()) << "Connect failed:" << m_modbusTCPMaster->errorString();
            return false;
        }
    }
    return true;
}

bool DevicePluginUniPi::neuronExtensionInterfaceInit()
{
    if(!m_modbusRTUMaster) {
        QString serialPort = configValue(uniPiPluginSerialPortParamTypeId).toString();
        int baudrate = configValue(uniPiPluginBaudrateParamTypeId).toInt();
        QString parity = configValue(uniPiPluginParityParamTypeId).toString();

        m_modbusRTUMaster = new QModbusRtuSerialMaster(this);
        m_modbusRTUMaster->setConnectionParameter(QModbusDevice::SerialPortNameParameter, serialPort);
        if (parity == "Even") {
            m_modbusRTUMaster->setConnectionParameter(QModbusDevice::SerialParityParameter, QSerialPort::Parity::EvenParity);
        } else {
            m_modbusRTUMaster->setConnectionParameter(QModbusDevice::SerialParityParameter, QSerialPort::Parity::NoParity);
        }
        m_modbusRTUMaster->setConnectionParameter(QModbusDevice::SerialBaudRateParameter, baudrate);
        m_modbusRTUMaster->setConnectionParameter(QModbusDevice::SerialDataBitsParameter, 8);
        m_modbusRTUMaster->setConnectionParameter(QModbusDevice::SerialStopBitsParameter, 1);
        //m_modbusRTUMaster->setTimeout(100);
        //m_modbusRTUMaster->setNumberOfRetries(1);

        connect(m_modbusRTUMaster, &QModbusRtuSerialMaster::stateChanged, this, &DevicePluginUniPi::onModbusRTUStateChanged);
        connect(m_modbusRTUMaster, &QModbusRtuSerialMaster::errorOccurred, this, &DevicePluginUniPi::onModbusRTUErrorOccurred);

        if (!m_modbusRTUMaster->connectDevice()) {
            qCWarning(dcUniPi()) << "Connect failed:" << m_modbusRTUMaster->errorString();
            return false;
        }
    }
    return true;
}
