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

#include <QJsonDocument>
#include <QTimer>
#include <QSerialPort>

DevicePluginUniPi::DevicePluginUniPi()
{
}


void DevicePluginUniPi::init()
{
    connect(this, &DevicePluginUniPi::configValueChanged, this, &DevicePluginUniPi::onPluginConfigurationChanged);
}

DeviceManager::DeviceError DevicePluginUniPi::discoverDevices(const DeviceClassId &deviceClassId, const ParamList &params)
{
    Q_UNUSED(params);

    if (deviceClassId == digitalInputDeviceClassId) {
        QList<DeviceDescriptor> deviceDescriptors;
        foreach (NeuronExtension *neuronExtension, m_neuronExtensions) {
            DeviceId parentDeviceId = m_neuronExtensions.key(neuronExtension);
            foreach (QString circuit, neuronExtension->digitalInputs()) {
                /*if (!myDevices().filterByParam(digitalInputDeviceCircuitParamTypeId, circuit).isEmpty()) {
                    continue;
                }*/
                DeviceDescriptor deviceDescriptor(digitalInputDeviceClassId, QString("Digital input %1").arg(circuit), QString("Neuron extension %1, Slave address %2").arg(neuronExtension->type().arg(QString::number(neuronExtension->slaveAddress()))), parentDeviceId);
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
                /*foreach (Device *existingDevice, myDevices().filterByParam(digitalInputDeviceCircuitParamTypeId, circuit)) {
                    if (existingDevice->paramValue(input))
                    break;
                }*/
                DeviceDescriptor deviceDescriptor(digitalInputDeviceClassId, QString("Digital input %1").arg(circuit), QString("Neuron %1").arg(neuron->type()), parentDeviceId);
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
            return DeviceManager::DeviceErrorNoError;
        }
    }

    if (deviceClassId == digitalOutputDeviceClassId) {
        QList<DeviceDescriptor> deviceDescriptors;
        foreach (NeuronExtension *neuronExtension, m_neuronExtensions) {
            DeviceId parentDeviceId = m_neuronExtensions.key(neuronExtension);
            foreach (QString circuit, neuronExtension->digitalOutputs()) {
                /*if (!myDevices().filterByParam(digitalOutputDeviceCircuitParamTypeId, circuit).isEmpty()) {
                    continue;
                }*/

                DeviceDescriptor deviceDescriptor(digitalOutputDeviceClassId, QString("Digital output %1").arg(circuit), QString("Neuron extension %1, Slave address %2").arg(neuronExtension->type().arg(QString::number(neuronExtension->slaveAddress()))), parentDeviceId);
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
                /*if (!myDevices().filterByParam(digitalOutputDeviceCircuitParamTypeId, circuit).isEmpty()) {
                    continue;
                }*/

                DeviceDescriptor deviceDescriptor(digitalOutputDeviceClassId, QString("Digital output %1").arg(circuit), QString("Neuron %1").arg(neuron->type()), parentDeviceId);
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
            return DeviceManager::DeviceErrorNoError;
        }
    }

    if (deviceClassId == analogInputDeviceClassId) {
        QList<DeviceDescriptor> deviceDescriptors;
        foreach (NeuronExtension *neuronExtension, m_neuronExtensions) {
            DeviceId parentDeviceId = m_neuronExtensions.key(neuronExtension);
            foreach (QString circuit, neuronExtension->analogInputs()) {
                /*if (!myDevices().filterByParam(analogInputDeviceCircuitParamTypeId, circuit).isEmpty()) {
                    continue;
                }*/

                DeviceDescriptor deviceDescriptor(analogInputDeviceClassId, QString("Analog input %1").arg(circuit), QString("Neuron extension %1, Slave address %2").arg(neuronExtension->type().arg(QString::number(neuronExtension->slaveAddress()))), parentDeviceId);
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
                /*if (!myDevices().filterByParam(analogInputDeviceCircuitParamTypeId, circuit).isEmpty()) {
                    continue;
                }*/

                DeviceDescriptor deviceDescriptor(analogInputDeviceClassId, QString("Analog input %1").arg(circuit), QString("Neuron %1").arg(neuron->type()), parentDeviceId);
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
            return DeviceManager::DeviceErrorNoError;
        }
    }

    if (deviceClassId == analogOutputDeviceClassId) {
        QList<DeviceDescriptor> deviceDescriptors;
        foreach (NeuronExtension *neuronExtension, m_neuronExtensions) {
            DeviceId parentDeviceId = m_neuronExtensions.key(neuronExtension);
            foreach (QString circuit, neuronExtension->analogOutputs()) {
                /*if (!myDevices().filterByParam(analogOutputDeviceCircuitParamTypeId, circuit).isEmpty()) {
                    continue;
                }*/

                DeviceDescriptor deviceDescriptor(analogOutputDeviceClassId, QString("Analog output %1").arg(circuit), QString("Neuron extension %1, Slave address %2").arg(neuronExtension->type().arg(QString::number(neuronExtension->slaveAddress()))), parentDeviceId);
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
                /*if (!myDevices().filterByParam(analogOutputDeviceCircuitParamTypeId, circuit).isEmpty()) {
                    continue;
                }*/

                DeviceDescriptor deviceDescriptor(analogOutputDeviceClassId, QString("Analog output %1").arg(circuit), QString("Neuron %1").arg(neuron->type()), parentDeviceId);
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
            return DeviceManager::DeviceErrorNoError;
        }
    }

    if (deviceClassId == userLEDDeviceClassId) {
        QList<DeviceDescriptor> deviceDescriptors;
        foreach (NeuronExtension *neuronExtension, m_neuronExtensions) {
            DeviceId parentDeviceId = m_neuronExtensions.key(neuronExtension);
            foreach (QString circuit, neuronExtension->userLEDs()) {
                /*if (!myDevices().filterByParam(userLEDDeviceCircuitParamTypeId, circuit).isEmpty()) {
                    continue;
                }*/

                DeviceDescriptor deviceDescriptor(userLEDDeviceClassId, QString("User programmable LED %1").arg(circuit), QString("Neuron extension %1, Slave address %2").arg(neuronExtension->type().arg(neuronExtension->slaveAddress())), parentDeviceId);
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
                /*if (!myDevices().filterByParam(userLEDDeviceCircuitParamTypeId, circuit).isEmpty()) {
                    continue;
                }*/

                DeviceDescriptor deviceDescriptor(userLEDDeviceClassId, QString("User programmable LED %1").arg(circuit), QString("Neuron %1").arg(neuron->type()), parentDeviceId);
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
            return DeviceManager::DeviceErrorNoError;
        }
    }
    return DeviceManager::DeviceErrorAsync;
}

DeviceManager::DeviceSetupStatus DevicePluginUniPi::setupDevice(Device *device)
{
    if (!m_reconnectTimer) {
        m_reconnectTimer = new QTimer(this);
        m_reconnectTimer->setSingleShot(true);
        connect(m_reconnectTimer, &QTimer::timeout, this, &DevicePluginUniPi::onReconnectTimer);
    }

    if(!m_modbusTCPMaster) {
        int port = configValue(uniPiPluginPortParamTypeId).toInt();;
        QHostAddress ipAddress = QHostAddress(configValue(uniPiPluginAddressParamTypeId).toString());

        m_modbusTCPMaster = new QModbusTcpClient(this);
        m_modbusTCPMaster->setConnectionParameter(QModbusDevice::NetworkPortParameter, port);
        m_modbusTCPMaster->setConnectionParameter(QModbusDevice::NetworkAddressParameter, ipAddress.toString());
        m_modbusTCPMaster->setTimeout(100);
        m_modbusTCPMaster->setNumberOfRetries(1);

        connect(m_modbusTCPMaster, &QModbusTcpClient::stateChanged, this, &DevicePluginUniPi::onModbusTCPStateChanged);
        connect(m_modbusTCPMaster, &QModbusTcpClient::errorOccurred, this, &DevicePluginUniPi::onModbusTCPErrorOccurred);

        if (!m_modbusTCPMaster->connectDevice()) {
            qCWarning(dcUniPi()) << "Connect failed:" << m_modbusTCPMaster->errorString();
            return DeviceManager::DeviceSetupStatusFailure;
        }
    }

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
        m_modbusRTUMaster->setTimeout(100);
        m_modbusRTUMaster->setNumberOfRetries(1);

        connect(m_modbusRTUMaster, &QModbusRtuSerialMaster::stateChanged, this, &DevicePluginUniPi::onModbusRTUStateChanged);
        connect(m_modbusRTUMaster, &QModbusRtuSerialMaster::errorOccurred, this, &DevicePluginUniPi::onModbusRTUErrorOccurred);

        if (!m_modbusRTUMaster->connectDevice()) {
            qCWarning(dcUniPi()) << "Connect failed:" << m_modbusRTUMaster->errorString();
            return DeviceManager::DeviceSetupStatusFailure;
        }
    }

    if(device->deviceClassId() == neuronS103DeviceClassId) {

        Neuron *neuron = new Neuron(Neuron::NeuronTypes::S103, m_modbusTCPMaster, this);
        if (!neuron->init()) {
            qCWarning(dcUniPi()) << "Could not load the modbus map";
            neuron->deleteLater();
            return DeviceManager::DeviceSetupStatusFailure;
        }
        m_neurons.insert(device->id(), neuron);
        connect(neuron, &Neuron::digitalInputStatusChanged, this, &DevicePluginUniPi::onNeuronDigitalInputStatusChanged);
        connect(neuron, &Neuron::digitalOutputStatusChanged, this, &DevicePluginUniPi::onNeuronDigitalOutputStatusChanged);

        device->setStateValue(neuronS103ConnectedStateTypeId, true);

        return DeviceManager::DeviceSetupStatusSuccess;
    }

    if(device->deviceClassId() == neuronM103DeviceClassId) {

        Neuron *neuron = new Neuron(Neuron::NeuronTypes::M103, m_modbusTCPMaster, this);
        if (!neuron->init()) {
            qCWarning(dcUniPi()) << "Could not load the modbus map";
            neuron->deleteLater();
            return DeviceManager::DeviceSetupStatusFailure;
        }
        m_neurons.insert(device->id(), neuron);
        connect(neuron, &Neuron::digitalInputStatusChanged, this, &DevicePluginUniPi::onNeuronDigitalInputStatusChanged);
        connect(neuron, &Neuron::digitalOutputStatusChanged, this, &DevicePluginUniPi::onNeuronDigitalOutputStatusChanged);

        device->setStateValue(neuronM103ConnectedStateTypeId, true);

        return DeviceManager::DeviceSetupStatusSuccess;
    }

    if(device->deviceClassId() == neuronM203DeviceClassId) {

        Neuron *neuron = new Neuron(Neuron::NeuronTypes::M203, m_modbusTCPMaster, this);
        if (!neuron->init()) {
            qCWarning(dcUniPi()) << "Could not load the modbus map";
            neuron->deleteLater();
            return DeviceManager::DeviceSetupStatusFailure;
        }
        m_neurons.insert(device->id(), neuron);
        connect(neuron, &Neuron::digitalInputStatusChanged, this, &DevicePluginUniPi::onNeuronDigitalInputStatusChanged);
        connect(neuron, &Neuron::digitalOutputStatusChanged, this, &DevicePluginUniPi::onNeuronDigitalOutputStatusChanged);

        device->setStateValue(neuronM203ConnectedStateTypeId, true);

        return DeviceManager::DeviceSetupStatusSuccess;
    }

    if(device->deviceClassId() == neuronM303DeviceClassId) {

        Neuron *neuron = new Neuron(Neuron::NeuronTypes::M303, m_modbusTCPMaster, this);
        if (!neuron->init()) {
            qCWarning(dcUniPi()) << "Could not load the modbus map";
            neuron->deleteLater();
            return DeviceManager::DeviceSetupStatusFailure;
        }
        m_neurons.insert(device->id(), neuron);
        connect(neuron, &Neuron::digitalInputStatusChanged, this, &DevicePluginUniPi::onNeuronDigitalInputStatusChanged);
        connect(neuron, &Neuron::digitalOutputStatusChanged, this, &DevicePluginUniPi::onNeuronDigitalOutputStatusChanged);

        device->setStateValue(neuronM303ConnectedStateTypeId, true);

        return DeviceManager::DeviceSetupStatusSuccess;
    }

    if(device->deviceClassId() == neuronM403DeviceClassId) {

        Neuron *neuron = new Neuron(Neuron::NeuronTypes::M403, m_modbusTCPMaster, this);
        if (!neuron->init()) {
            qCWarning(dcUniPi()) << "Could not load the modbus map";
            neuron->deleteLater();
            return DeviceManager::DeviceSetupStatusFailure;
        }
        m_neurons.insert(device->id(), neuron);
        connect(neuron, &Neuron::digitalInputStatusChanged, this, &DevicePluginUniPi::onNeuronDigitalInputStatusChanged);
        connect(neuron, &Neuron::digitalOutputStatusChanged, this, &DevicePluginUniPi::onNeuronDigitalOutputStatusChanged);

        device->setStateValue(neuronM403ConnectedStateTypeId, true);

        return DeviceManager::DeviceSetupStatusSuccess;
    }

    if(device->deviceClassId() == neuronM503DeviceClassId) {

        Neuron *neuron = new Neuron(Neuron::NeuronTypes::M503, m_modbusTCPMaster, this);
        if (!neuron->init()) {
            qCWarning(dcUniPi()) << "Could not load the modbus map";
            neuron->deleteLater();
            return DeviceManager::DeviceSetupStatusFailure;
        }
        m_neurons.insert(device->id(), neuron);
        connect(neuron, &Neuron::digitalInputStatusChanged, this, &DevicePluginUniPi::onNeuronDigitalInputStatusChanged);
        connect(neuron, &Neuron::digitalOutputStatusChanged, this, &DevicePluginUniPi::onNeuronDigitalOutputStatusChanged);

        device->setStateValue(neuronM503ConnectedStateTypeId, true);

        return DeviceManager::DeviceSetupStatusSuccess;
    }

    if(device->deviceClassId() == neuronL203DeviceClassId) {

        Neuron *neuron = new Neuron(Neuron::NeuronTypes::L203, m_modbusTCPMaster, this);
        if (!neuron->init()) {
            qCWarning(dcUniPi()) << "Could not load the modbus map";
            neuron->deleteLater();
            return DeviceManager::DeviceSetupStatusFailure;
        }
        m_neurons.insert(device->id(), neuron);
        connect(neuron, &Neuron::digitalInputStatusChanged, this, &DevicePluginUniPi::onNeuronDigitalInputStatusChanged);
        connect(neuron, &Neuron::digitalOutputStatusChanged, this, &DevicePluginUniPi::onNeuronDigitalOutputStatusChanged);

        device->setStateValue(neuronL203ConnectedStateTypeId, true);

        return DeviceManager::DeviceSetupStatusSuccess;
    }

    if(device->deviceClassId() == neuronL303DeviceClassId) {

        Neuron *neuron = new Neuron(Neuron::NeuronTypes::L303, m_modbusTCPMaster, this);
        if (!neuron->init()) {
            qCWarning(dcUniPi()) << "Could not load the modbus map";
            neuron->deleteLater();
            return DeviceManager::DeviceSetupStatusFailure;
        }
        m_neurons.insert(device->id(), neuron);
        connect(neuron, &Neuron::digitalInputStatusChanged, this, &DevicePluginUniPi::onNeuronDigitalInputStatusChanged);
        connect(neuron, &Neuron::digitalOutputStatusChanged, this, &DevicePluginUniPi::onNeuronDigitalOutputStatusChanged);

        device->setStateValue(neuronL303ConnectedStateTypeId, true);

        return DeviceManager::DeviceSetupStatusSuccess;
    }

    if(device->deviceClassId() == neuronL403DeviceClassId) {

        Neuron *neuron = new Neuron(Neuron::NeuronTypes::L403, m_modbusTCPMaster, this);
        if (!neuron->init()) {
            qCWarning(dcUniPi()) << "Could not load the modbus map";
            neuron->deleteLater();
            return DeviceManager::DeviceSetupStatusFailure;
        }
        m_neurons.insert(device->id(), neuron);
        connect(neuron, &Neuron::digitalInputStatusChanged, this, &DevicePluginUniPi::onNeuronDigitalInputStatusChanged);
        connect(neuron, &Neuron::digitalOutputStatusChanged, this, &DevicePluginUniPi::onNeuronDigitalOutputStatusChanged);

        device->setStateValue(neuronL403ConnectedStateTypeId, true);

        return DeviceManager::DeviceSetupStatusSuccess;
    }

    if(device->deviceClassId() == neuronL503DeviceClassId) {

        Neuron *neuron = new Neuron(Neuron::NeuronTypes::L503, m_modbusTCPMaster, this);
        if (!neuron->init()) {
            qCWarning(dcUniPi()) << "Could not load the modbus map";
            neuron->deleteLater();
            return DeviceManager::DeviceSetupStatusFailure;
        }
        m_neurons.insert(device->id(), neuron);
        connect(neuron, &Neuron::digitalInputStatusChanged, this, &DevicePluginUniPi::onNeuronDigitalInputStatusChanged);
        connect(neuron, &Neuron::digitalOutputStatusChanged, this, &DevicePluginUniPi::onNeuronDigitalOutputStatusChanged);

        device->setStateValue(neuronL503ConnectedStateTypeId, true);

        return DeviceManager::DeviceSetupStatusSuccess;
    }

    if(device->deviceClassId() == neuronL513DeviceClassId) {

        Neuron *neuron = new Neuron(Neuron::NeuronTypes::L513, m_modbusTCPMaster, this);
        if (!neuron->init()) {
            qCWarning(dcUniPi()) << "Could not load the modbus map";
            neuron->deleteLater();
            return DeviceManager::DeviceSetupStatusFailure;
        }
        m_neurons.insert(device->id(), neuron);
        connect(neuron, &Neuron::digitalInputStatusChanged, this, &DevicePluginUniPi::onNeuronDigitalInputStatusChanged);
        connect(neuron, &Neuron::digitalOutputStatusChanged, this, &DevicePluginUniPi::onNeuronDigitalOutputStatusChanged);

        device->setStateValue(neuronL513ConnectedStateTypeId, true);

        return DeviceManager::DeviceSetupStatusSuccess;
    }

    if(device->deviceClassId() == neuronXS10DeviceClassId) {

        int slaveAddress = device->paramValue(neuronXS10DeviceSlaveAddressParamTypeId).toInt();
        NeuronExtension *neuronExtension = new NeuronExtension(NeuronExtension::ExtensionTypes::xS10, m_modbusRTUMaster, slaveAddress, this);
        if (!neuronExtension->init()) {
            qCWarning(dcUniPi()) << "Could not load the modbus map";
            neuronExtension->deleteLater();
            return DeviceManager::DeviceSetupStatusFailure;
        }
        connect(neuronExtension, &NeuronExtension::digitalInputStatusChanged, this, &DevicePluginUniPi::onNeuronExtensionDigitalInputStatusChanged);
        connect(neuronExtension, &NeuronExtension::digitalOutputStatusChanged, this, &DevicePluginUniPi::onNeuronExtensionDigitalOutputStatusChanged);
        m_neuronExtensions.insert(device->id(), neuronExtension);
        device->setStateValue(neuronXS10ConnectedStateTypeId, true);

        return DeviceManager::DeviceSetupStatusSuccess;
    }

    if(device->deviceClassId() == neuronXS20DeviceClassId) {

        int slaveAddress = device->paramValue(neuronXS20DeviceSlaveAddressParamTypeId).toInt();
        NeuronExtension *neuronExtension = new NeuronExtension(NeuronExtension::ExtensionTypes::xS20, m_modbusRTUMaster, slaveAddress, this);
        if (!neuronExtension->init()) {
            qCWarning(dcUniPi()) << "Could not load the modbus map";
            neuronExtension->deleteLater();
            return DeviceManager::DeviceSetupStatusFailure;
        }
        connect(neuronExtension, &NeuronExtension::digitalInputStatusChanged, this, &DevicePluginUniPi::onNeuronExtensionDigitalInputStatusChanged);
        connect(neuronExtension, &NeuronExtension::digitalOutputStatusChanged, this, &DevicePluginUniPi::onNeuronExtensionDigitalOutputStatusChanged);
        m_neuronExtensions.insert(device->id(), neuronExtension);
        device->setStateValue(neuronXS20ConnectedStateTypeId, true);

        return DeviceManager::DeviceSetupStatusSuccess;
    }

    if(device->deviceClassId() == neuronXS30DeviceClassId) {

        int slaveAddress = device->paramValue(neuronXS30DeviceSlaveAddressParamTypeId).toInt();
        NeuronExtension *neuronExtension = new NeuronExtension(NeuronExtension::ExtensionTypes::xS30, m_modbusRTUMaster, slaveAddress, this);
        if (!neuronExtension->init()) {
            qCWarning(dcUniPi()) << "Could not load the modbus map";
            neuronExtension->deleteLater();
            return DeviceManager::DeviceSetupStatusFailure;
        }
        connect(neuronExtension, &NeuronExtension::digitalInputStatusChanged, this, &DevicePluginUniPi::onNeuronExtensionDigitalInputStatusChanged);
        connect(neuronExtension, &NeuronExtension::digitalOutputStatusChanged, this, &DevicePluginUniPi::onNeuronExtensionDigitalOutputStatusChanged);
        m_neuronExtensions.insert(device->id(), neuronExtension);
        device->setStateValue(neuronXS30ConnectedStateTypeId, true);

        return DeviceManager::DeviceSetupStatusSuccess;
    }

    if(device->deviceClassId() == neuronXS40DeviceClassId) {

        int slaveAddress = device->paramValue(neuronXS40DeviceSlaveAddressParamTypeId).toInt();
        NeuronExtension *neuronExtension = new NeuronExtension(NeuronExtension::ExtensionTypes::xS40, m_modbusRTUMaster, slaveAddress, this);
        if (!neuronExtension->init()) {
            qCWarning(dcUniPi()) << "Could not load the modbus map";
            neuronExtension->deleteLater();
            return DeviceManager::DeviceSetupStatusFailure;
        }
        connect(neuronExtension, &NeuronExtension::digitalInputStatusChanged, this, &DevicePluginUniPi::onNeuronExtensionDigitalInputStatusChanged);
        connect(neuronExtension, &NeuronExtension::digitalOutputStatusChanged, this, &DevicePluginUniPi::onNeuronExtensionDigitalOutputStatusChanged);
        m_neuronExtensions.insert(device->id(), neuronExtension);
        device->setStateValue(neuronXS40ConnectedStateTypeId, true);

        return DeviceManager::DeviceSetupStatusSuccess;
    }

    if(device->deviceClassId() == neuronXS50DeviceClassId) {

        int slaveAddress = device->paramValue(neuronXS50DeviceSlaveAddressParamTypeId).toInt();
        NeuronExtension *neuronExtension = new NeuronExtension(NeuronExtension::ExtensionTypes::xS50, m_modbusRTUMaster, slaveAddress, this);
        if (!neuronExtension->init()) {
            qCWarning(dcUniPi()) << "Could not load the modbus map";
            neuronExtension->deleteLater();
            return DeviceManager::DeviceSetupStatusFailure;
        }
        connect(neuronExtension, &NeuronExtension::digitalInputStatusChanged, this, &DevicePluginUniPi::onNeuronExtensionDigitalInputStatusChanged);
        connect(neuronExtension, &NeuronExtension::digitalOutputStatusChanged, this, &DevicePluginUniPi::onNeuronExtensionDigitalOutputStatusChanged);
        m_neuronExtensions.insert(device->id(), neuronExtension);
        device->setStateValue(neuronXS50ConnectedStateTypeId, true);

        return DeviceManager::DeviceSetupStatusSuccess;
    }

    if (device->deviceClassId() == digitalOutputDeviceClassId) {

        return DeviceManager::DeviceSetupStatusSuccess;
    }

    if (device->deviceClassId() == digitalInputDeviceClassId) {

        return DeviceManager::DeviceSetupStatusSuccess;
    }

    if (device->deviceClassId() == userLEDDeviceClassId) {

        return DeviceManager::DeviceSetupStatusSuccess;
    }

    if (device->deviceClassId() == analogInputDeviceClassId) {

        return DeviceManager::DeviceSetupStatusSuccess;
    }

    if (device->deviceClassId() == analogOutputDeviceClassId) {

        return DeviceManager::DeviceSetupStatusSuccess;
    }
    return DeviceManager::DeviceSetupStatusFailure;
}

void DevicePluginUniPi::postSetupDevice(Device *device)
{
    Q_UNUSED(device);
}


DeviceManager::DeviceError DevicePluginUniPi::executeAction(Device *device, const Action &action)
{
    if (device->deviceClassId() == digitalOutputDeviceClassId)  {

        if (action.actionTypeId() == digitalOutputPowerActionTypeId) {
            QString digitalOutputNumber = device->paramValue(digitalOutputDeviceCircuitParamTypeId).toString();
            bool stateValue = action.param(digitalOutputPowerActionPowerParamTypeId).value().toBool();
            if (m_neurons.contains(device->paramValue(digitalOutputDeviceParentIdParamTypeId).toString())) {
                Neuron *neuron = m_neurons.value(device->paramValue(digitalOutputDeviceParentIdParamTypeId).toString());
                neuron->setDigitalOutput(digitalOutputNumber, stateValue);
            }
            if (m_neuronExtensions.contains(device->paramValue(digitalOutputDeviceParentIdParamTypeId).toString())) {
                NeuronExtension *neuronExtension = m_neuronExtensions.value(device->paramValue(digitalOutputDeviceParentIdParamTypeId).toString());
                neuronExtension->setDigitalOutput(digitalOutputNumber, stateValue);
            }
            return DeviceManager::DeviceErrorNoError;
        }
        return DeviceManager::DeviceErrorActionTypeNotFound;
    }

    if (device->deviceClassId() == analogOutputDeviceClassId) {

        if (action.actionTypeId() == analogOutputOutputValueActionTypeId) {
            QString analogOutputNumber = device->paramValue(analogOutputDeviceCircuitParamTypeId).toString();
            double analogValue = action.param(analogOutputOutputValueActionOutputValueParamTypeId).value().toDouble();
            if (m_neurons.contains(device->paramValue(digitalOutputDeviceParentIdParamTypeId).toString())) {
                Neuron *neuron = m_neurons.value(device->paramValue(digitalOutputDeviceParentIdParamTypeId).toString());
                neuron->setAnalogOutput(analogOutputNumber, analogValue);
            }
            if (m_neuronExtensions.contains(device->paramValue(digitalOutputDeviceParentIdParamTypeId).toString())) {
                NeuronExtension *neuronExtension = m_neuronExtensions.value(device->paramValue(digitalOutputDeviceParentIdParamTypeId).toString());
                neuronExtension->setAnalogOutput(analogOutputNumber, analogValue);
            }
            return DeviceManager::DeviceErrorNoError;
        }
        return DeviceManager::DeviceErrorActionTypeNotFound;
    }

    if (device->deviceClassId() == userLEDDeviceClassId) {
        if (action.actionTypeId() == digitalOutputPowerActionTypeId) {
            QString userLED = device->paramValue(userLEDDeviceCircuitParamTypeId).toString();
            bool stateValue = action.param(userLEDPowerActionPowerParamTypeId).value().toBool();
            if (m_neurons.contains(device->paramValue(digitalOutputDeviceParentIdParamTypeId).toString())) {
                Neuron *neuron = m_neurons.value(device->paramValue(digitalOutputDeviceParentIdParamTypeId).toString());
                neuron->setUserLED(userLED, stateValue);
            }
            if (m_neuronExtensions.contains(device->paramValue(digitalOutputDeviceParentIdParamTypeId).toString())) {
                NeuronExtension *neuronExtension = m_neuronExtensions.value(device->paramValue(digitalOutputDeviceParentIdParamTypeId).toString());
                neuronExtension->setUserLED(userLED, stateValue);
            }
            return DeviceManager::DeviceErrorNoError;
        }
        return Device::DeviceErrorActionTypeNotFound;
    }
    return DeviceManager::DeviceErrorDeviceClassNotFound;
}


void DevicePluginUniPi::deviceRemoved(Device *device)
{
    if(m_neurons.contains(device->id())) {
        Neuron *neuron = m_neurons.take(device->id());
        neuron->deleteLater();

        foreach(Device *child, myDevices().filterByParam(digitalInputDeviceParentIdParamTypeId, device->id())) {
            deviceManager()->removeConfiguredDevice(child->id());
        }
        foreach(Device *child, myDevices().filterByParam(digitalOutputDeviceParentIdParamTypeId, device->id())) {
            deviceManager()->removeConfiguredDevice(child->id());
        }
        foreach(Device *child, myDevices().filterByParam(analogInputDeviceParentIdParamTypeId, device->id())) {
            deviceManager()->removeConfiguredDevice(child->id());
        }
        foreach(Device *child, myDevices().filterByParam(analogOutputDeviceParentIdParamTypeId, device->id())) {
            deviceManager()->removeConfiguredDevice(child->id());
        }
        foreach(Device *child, myDevices().filterByParam(userLEDDeviceParentIdParamTypeId,device->id())) {
            deviceManager()->removeConfiguredDevice(child->id());
        }

    }
    if(m_neuronExtensions.contains(device->id())) {
        NeuronExtension *neuronExtension = m_neuronExtensions.take(device->id());
        neuronExtension->deleteLater();

        foreach(Device *child, myDevices().filterByParam(digitalInputDeviceParentIdParamTypeId, device->id())) {
            deviceManager()->removeConfiguredDevice(child->id());
        }
        foreach(Device *child, myDevices().filterByParam(digitalOutputDeviceParentIdParamTypeId, device->id())) {
            deviceManager()->removeConfiguredDevice(child->id());
        }
        foreach(Device *child, myDevices().filterByParam(analogInputDeviceParentIdParamTypeId, device->id())) {
            deviceManager()->removeConfiguredDevice(child->id());
        }
        foreach(Device *child, myDevices().filterByParam(analogOutputDeviceParentIdParamTypeId, device->id())) {
            deviceManager()->removeConfiguredDevice(child->id());
        }
        foreach(Device *child, myDevices().filterByParam(userLEDDeviceParentIdParamTypeId,device->id())) {
            deviceManager()->removeConfiguredDevice(child->id());
        }
    }

    if (myDevices().isEmpty()) {
        m_reconnectTimer->stop();
        m_reconnectTimer->deleteLater();

        m_modbusTCPMaster->disconnectDevice();
        m_modbusTCPMaster->deleteLater();

        m_modbusRTUMaster->disconnectDevice();
        m_modbusRTUMaster->deleteLater();
    }
}

void DevicePluginUniPi::onPluginConfigurationChanged(const ParamTypeId &paramTypeId, const QVariant &value)
{
    qCDebug(dcUniPi()) << "Plugin configuration changed";
    if (paramTypeId == uniPiPluginPortParamTypeId) {
        if (!m_modbusTCPMaster) {
            m_modbusTCPMaster->setConnectionParameter(QModbusDevice::NetworkAddressParameter, value.toString());
        }
    }

    if (paramTypeId == uniPiPluginAddressParamTypeId) {
        if (!m_modbusTCPMaster) {
            m_modbusTCPMaster->setConnectionParameter(QModbusDevice::NetworkPortParameter, value.toInt());
        }
    }

    if (paramTypeId == uniPiPluginSerialPortParamTypeId) {
        if (!m_modbusRTUMaster) {
            m_modbusRTUMaster->setConnectionParameter(QModbusDevice::SerialPortNameParameter, value.toString());
        }
    }

    if (paramTypeId == uniPiPluginBaudrateParamTypeId) {
        if (!m_modbusRTUMaster) {
            m_modbusRTUMaster->setConnectionParameter(QModbusDevice::SerialBaudRateParameter, value.toInt());
        }
    }

    if (paramTypeId == uniPiPluginParityParamTypeId) {
        if (!m_modbusRTUMaster) {
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
