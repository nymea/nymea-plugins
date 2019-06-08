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
                if (!myDevices().filterByParam(digitalInputDeviceCircuitParamTypeId, circuit).isEmpty()) {
                    continue;
                }

                DeviceDescriptor deviceDescriptor(digitalInputDeviceClassId, circuit, QString("Neuron extension: %1, Slave address: %2").arg(neuronExtension->type().arg(neuronExtension->slaveAddress())), parentDeviceId);
                ParamList params;
                params.append(Param(digitalInputDeviceCircuitParamTypeId, circuit));
                deviceDescriptor.setParams(params);
                deviceDescriptors.append(deviceDescriptor);
            }
        }

        foreach (Neuron *neuron, m_neurons) {
            DeviceId parentDeviceId = m_neurons.key(neuron);
            foreach (QString circuit, neuron->digitalInputs()) {
                if (!myDevices().filterByParam(digitalInputDeviceCircuitParamTypeId, circuit).isEmpty()) {
                    continue;
                }

                DeviceDescriptor deviceDescriptor(digitalInputDeviceClassId, circuit, QString("Neuron: %1").arg(neuron->type()), parentDeviceId);
                ParamList params;
                params.append(Param(digitalInputDeviceCircuitParamTypeId, circuit));
                deviceDescriptor.setParams(params);
                deviceDescriptors.append(deviceDescriptor);
            }
        }
        if (!deviceDescriptors.isEmpty())
            emit devicesDiscovered(digitalInputDeviceClassId, deviceDescriptors);
    }

    if (deviceClassId == digitalOutputDeviceClassId) {
        QList<DeviceDescriptor> deviceDescriptors;

        if (!deviceDescriptors.isEmpty())
            emit devicesDiscovered(digitalOutputDeviceClassId, deviceDescriptors);

    }

    if (deviceClassId == analogInputDeviceClassId) {

    }

    if (deviceClassId == analogOutputDeviceClassId) {

    }

    if (deviceClassId == userLEDDeviceClassId) {

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

        connect(m_modbusTCPMaster, &QModbusTcpClient::stateChanged, this, &DevicePluginUniPi::onStateChanged);
        connect(m_modbusTCPMaster, &QModbusTcpClient::errorOccurred, this, &DevicePluginUniPi::onErrorOccurred);

        if (!m_modbusTCPMaster->connectDevice()) {
            qCWarning(dcUniPi()) << "Connect failed:" << m_modbusTCPMaster->errorString();
            return DeviceManager::DeviceSetupStatusFailure;
        }
    }

    if(!m_modbusRTUMaster) {
        QString serialPort = configValue(uniPiPluginSerialPortParamTypeId).toString();
        int baudrate = configValue(uniPiPluginBaudrateParamTypeId).toInt();

        m_modbusRTUMaster = new QModbusRtuSerialMaster(this);
        m_modbusRTUMaster->setConnectionParameter(QModbusDevice::SerialPortNameParameter, serialPort);
        m_modbusRTUMaster->setConnectionParameter(QModbusDevice::SerialParityParameter, QSerialPort::Parity::NoParity);
        m_modbusRTUMaster->setConnectionParameter(QModbusDevice::SerialBaudRateParameter, baudrate);
        m_modbusRTUMaster->setConnectionParameter(QModbusDevice::SerialDataBitsParameter, 8);
        m_modbusRTUMaster->setConnectionParameter(QModbusDevice::SerialStopBitsParameter, 1);
        m_modbusRTUMaster->setTimeout(100);
        m_modbusRTUMaster->setNumberOfRetries(1);

        connect(m_modbusRTUMaster, &QModbusRtuSerialMaster::stateChanged, this, &DevicePluginUniPi::onStateChanged);
        connect(m_modbusRTUMaster, &QModbusRtuSerialMaster::errorOccurred, this, &DevicePluginUniPi::onErrorOccurred);

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
        connect(neuron, &Neuron::digitalInputStatusChanged, this, &DevicePluginUniPi::onDigitalInputStatusChanged);
        connect(neuron, &Neuron::digitalOutputStatusChanged, this, &DevicePluginUniPi::onDigitalOutputStatusChanged);

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
        connect(neuron, &Neuron::digitalInputStatusChanged, this, &DevicePluginUniPi::onDigitalInputStatusChanged);
        connect(neuron, &Neuron::digitalOutputStatusChanged, this, &DevicePluginUniPi::onDigitalOutputStatusChanged);

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
        connect(neuron, &Neuron::digitalInputStatusChanged, this, &DevicePluginUniPi::onDigitalInputStatusChanged);
        connect(neuron, &Neuron::digitalOutputStatusChanged, this, &DevicePluginUniPi::onDigitalOutputStatusChanged);

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
        connect(neuron, &Neuron::digitalInputStatusChanged, this, &DevicePluginUniPi::onDigitalInputStatusChanged);
        connect(neuron, &Neuron::digitalOutputStatusChanged, this, &DevicePluginUniPi::onDigitalOutputStatusChanged);

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
        connect(neuron, &Neuron::digitalInputStatusChanged, this, &DevicePluginUniPi::onDigitalInputStatusChanged);
        connect(neuron, &Neuron::digitalOutputStatusChanged, this, &DevicePluginUniPi::onDigitalOutputStatusChanged);

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
        connect(neuron, &Neuron::digitalInputStatusChanged, this, &DevicePluginUniPi::onDigitalInputStatusChanged);
        connect(neuron, &Neuron::digitalOutputStatusChanged, this, &DevicePluginUniPi::onDigitalOutputStatusChanged);

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
        connect(neuron, &Neuron::digitalInputStatusChanged, this, &DevicePluginUniPi::onDigitalInputStatusChanged);
        connect(neuron, &Neuron::digitalOutputStatusChanged, this, &DevicePluginUniPi::onDigitalOutputStatusChanged);

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
        connect(neuron, &Neuron::digitalInputStatusChanged, this, &DevicePluginUniPi::onDigitalInputStatusChanged);
        connect(neuron, &Neuron::digitalOutputStatusChanged, this, &DevicePluginUniPi::onDigitalOutputStatusChanged);

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
        connect(neuron, &Neuron::digitalInputStatusChanged, this, &DevicePluginUniPi::onDigitalInputStatusChanged);
        connect(neuron, &Neuron::digitalOutputStatusChanged, this, &DevicePluginUniPi::onDigitalOutputStatusChanged);

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
        connect(neuron, &Neuron::digitalInputStatusChanged, this, &DevicePluginUniPi::onDigitalInputStatusChanged);
        connect(neuron, &Neuron::digitalOutputStatusChanged, this, &DevicePluginUniPi::onDigitalOutputStatusChanged);

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
        connect(neuron, &Neuron::digitalInputStatusChanged, this, &DevicePluginUniPi::onDigitalInputStatusChanged);
        connect(neuron, &Neuron::digitalOutputStatusChanged, this, &DevicePluginUniPi::onDigitalOutputStatusChanged);

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
        connect(neuronExtension, &NeuronExtension::digitalInputStatusChanged, this, &DevicePluginUniPi::onDigitalInputStatusChanged);
        connect(neuronExtension, &NeuronExtension::digitalOutputStatusChanged, this, &DevicePluginUniPi::onDigitalOutputStatusChanged);
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
        connect(neuronExtension, &NeuronExtension::digitalInputStatusChanged, this, &DevicePluginUniPi::onDigitalInputStatusChanged);
        connect(neuronExtension, &NeuronExtension::digitalOutputStatusChanged, this, &DevicePluginUniPi::onDigitalOutputStatusChanged);
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
        connect(neuronExtension, &NeuronExtension::digitalInputStatusChanged, this, &DevicePluginUniPi::onDigitalInputStatusChanged);
        connect(neuronExtension, &NeuronExtension::digitalOutputStatusChanged, this, &DevicePluginUniPi::onDigitalOutputStatusChanged);
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
        connect(neuronExtension, &NeuronExtension::digitalInputStatusChanged, this, &DevicePluginUniPi::onDigitalInputStatusChanged);
        connect(neuronExtension, &NeuronExtension::digitalOutputStatusChanged, this, &DevicePluginUniPi::onDigitalOutputStatusChanged);
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
        connect(neuronExtension, &NeuronExtension::digitalInputStatusChanged, this, &DevicePluginUniPi::onDigitalInputStatusChanged);
        connect(neuronExtension, &NeuronExtension::digitalOutputStatusChanged, this, &DevicePluginUniPi::onDigitalOutputStatusChanged);
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
            if (m_neurons.contains(device->parentId())) {
                Neuron *neuron = m_neurons.value(device->parentId());
                neuron->setDigitalOutput(digitalOutputNumber, stateValue);
            }
            if (m_neuronExtensions.contains(device->parentId())) {
                NeuronExtension *neuronExtension = m_neuronExtensions.value(device->parentId());
                neuronExtension->setDigitalOutput(digitalOutputNumber, stateValue);
            }
            return DeviceManager::DeviceErrorNoError;
        }
        return DeviceManager::DeviceErrorActionTypeNotFound;
    }

    if (device->deviceClassId() == analogOutputDeviceClassId) {

        if (action.actionTypeId() == analogOutputOutputValueActionTypeId) {
            QString analogOutputNumber = device->paramValue(analogOutputDeviceCicuitParamTypeId).toString();
            double analogValue = action.param(analogOutputOutputValueActionOutputValueParamTypeId).value().toDouble();
            if (m_neurons.contains(device->parentId())) {
                Neuron *neuron = m_neurons.value(device->parentId());
                neuron->setAnalogOutput(analogOutputNumber, analogValue);
            }
            if (m_neuronExtensions.contains(device->parentId())) {
                NeuronExtension *neuronExtension = m_neuronExtensions.value(device->parentId());
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
            if (m_neurons.contains(device->parentId())) {
                Neuron *neuron = m_neurons.value(device->parentId());
                neuron->setDigitalOutput(userLED, stateValue);
            }
            if (m_neuronExtensions.contains(device->parentId())) {
                NeuronExtension *neuronExtension = m_neuronExtensions.value(device->parentId());
                neuronExtension->setDigitalOutput(userLED, stateValue);
            }
            return DeviceManager::DeviceErrorNoError;
        }
        return Device::DeviceErrorActionTypeNotFound;
    }
    return DeviceManager::DeviceErrorDeviceClassNotFound;
}


void DevicePluginUniPi::deviceRemoved(Device *device)
{
    if(device->deviceClassId() == neuronL403DeviceClassId) {
        m_modbusTCPMaster->deleteLater();
        m_neurons.remove(device->id());

    } else if(device->deviceClassId() == neuronXS30DeviceClassId) {
        m_neuronExtensions.remove(device->id());
        if (m_neuronExtensions.isEmpty()) {
            m_modbusRTUMaster->deleteLater();
        }
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

void DevicePluginUniPi::onDigitalInputStatusChanged(QString &circuit, bool value)
{
    foreach(Device *device, myDevices()) {
        if (device->deviceClassId() == digitalInputDeviceClassId) {
            if (device->paramValue(digitalInputDeviceCircuitParamTypeId).toString() == circuit) {

                device->setStateValue(digitalInputInputStatusStateTypeId, value);
                return;
            }
        }
    }
}

void DevicePluginUniPi::onDigitalOutputStatusChanged(QString &circuit, bool value)
{
    foreach(Device *device, myDevices()) {
        if (device->deviceClassId() == digitalOutputDeviceClassId) {
            if (device->paramValue(digitalOutputDeviceCircuitParamTypeId).toString() == circuit) {

                device->setStateValue(digitalOutputPowerStateTypeId, value);
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

void DevicePluginUniPi::onErrorOccurred(QModbusDevice::Error error)
{
    qCWarning(dcUniPi()) << "An error occured" << error;
}

void DevicePluginUniPi::onStateChanged(QModbusDevice::State state)
{
    bool connected = (state != QModbusDevice::UnconnectedState);
    if (!connected) {
        //try to reconnect in 10 seconds
        m_reconnectTimer->start(10000);
    }
    qCDebug(dcUniPi()) << "Connection status changed:" << connected;
}
