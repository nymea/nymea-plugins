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

    if (deviceClassId == neuronL403DeviceClassId) {
        QList<DeviceDescriptor> deviceDescriptors;
        foreach (Device *device, myDevices()) {
            if (device->deviceClassId() == neuronL403DeviceClassId) {
                DeviceDescriptor deviceDescriptor(device->deviceClassId(), device->name(), "existing device");
                deviceDescriptor.setParams(device->params());
                deviceDescriptor.setDeviceId(device->id());
                deviceDescriptors.append(deviceDescriptor);
            }
        }

        if (deviceDescriptors.isEmpty()) {
            DeviceDescriptor deviceDescriptor(neuronL403DeviceClassId, "Neuron L402", "new device");
            deviceDescriptors.append(deviceDescriptor);
        }

        emit devicesDiscovered(neuronL403DeviceClassId, deviceDescriptors);
    }

    if (deviceClassId == neuronXS30DeviceClassId) {
        QList<DeviceDescriptor> deviceDescriptors;
        foreach (Device *device, myDevices()) {
            if (device->deviceClassId() == neuronXS30DeviceClassId) {
                DeviceDescriptor deviceDescriptor(device->deviceClassId(), device->name(), "existing device");
                ParamList params;
                deviceDescriptor.setParams(device->params());
                deviceDescriptor.setDeviceId(device->id());
                deviceDescriptors.append(deviceDescriptor);
            }
        }

        DeviceDescriptor deviceDescriptor(neuronXS30DeviceClassId, "Neuron Extension xS30", "new device");
        deviceDescriptors.append(deviceDescriptor);

        emit devicesDiscovered(neuronXS30DeviceClassId, deviceDescriptors);
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
    if(device->deviceClassId() == neuronL403DeviceClassId) {

        int port = configValue(uniPiPluginPortParamTypeId).toInt();;
        QHostAddress ipAddress = QHostAddress(configValue(uniPiPluginAddressParamTypeId).toString());

        if(!m_modbusTCPMaster) {
            m_modbusTCPMaster = new QModbusTcpClient(this);
            m_modbusTCPMaster->setConnectionParameter(QModbusDevice::NetworkPortParameter, port);
            m_modbusTCPMaster->setConnectionParameter(QModbusDevice::NetworkAddressParameter, ipAddress.toString());
            //m_modbusTCPMaster->setTimeout(50);
            m_modbusTCPMaster->setNumberOfRetries(1);

            connect(m_modbusTCPMaster, &QModbusTcpClient::stateChanged, this, &DevicePluginUniPi::onStateChanged);
            connect(m_modbusTCPMaster, &QModbusTcpClient::errorOccurred, this, &DevicePluginUniPi::onErrorOccurred);

            if (!m_modbusTCPMaster->connectDevice()) {
                qCWarning(dcUniPi()) << "Connect failed:" << m_modbusTCPMaster->errorString();
                return DeviceManager::DeviceSetupStatusFailure;
            }
        }

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

    if(device->deviceClassId() == neuronXS30DeviceClassId) {

        QString serialPort = configValue(uniPiPluginSerialPortParamTypeId).toString();
        int baudrate = configValue(uniPiPluginBaudrateParamTypeId).toInt();
        int slaveAddress = device->paramValue(neuronXS30DeviceSlaveAddressParamTypeId).toInt();

        if(!m_modbusRTUMaster) {
            // Seems to be the first Modbus extension
            m_modbusRTUMaster = new QModbusRtuSerialMaster(this);
            m_modbusRTUMaster->setConnectionParameter(QModbusDevice::SerialPortNameParameter, serialPort);
            m_modbusRTUMaster->setConnectionParameter(QModbusDevice::SerialParityParameter, QSerialPort::Parity::NoParity);
            m_modbusRTUMaster->setConnectionParameter(QModbusDevice::SerialBaudRateParameter, baudrate);
            m_modbusRTUMaster->setConnectionParameter(QModbusDevice::SerialDataBitsParameter, 8);
            m_modbusRTUMaster->setConnectionParameter(QModbusDevice::SerialStopBitsParameter, 1);
            //m_modbusRTUMaster->setTimeout(50);
            m_modbusRTUMaster->setNumberOfRetries(1);

            connect(m_modbusRTUMaster, &QModbusRtuSerialMaster::stateChanged, this, &DevicePluginUniPi::onStateChanged);
            connect(m_modbusRTUMaster, &QModbusRtuSerialMaster::errorOccurred, this, &DevicePluginUniPi::onErrorOccurred);

            if (!m_modbusRTUMaster->connectDevice()) {
                qCWarning(dcUniPi()) << "Connect failed:" << m_modbusRTUMaster->errorString();
                return DeviceManager::DeviceSetupStatusFailure;
            }
        }
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

    if (device->deviceClassId() == relayOutputDeviceClassId) {

        return DeviceManager::DeviceSetupStatusSuccess;
    }

    if (device->deviceClassId() == digitalOutputDeviceClassId) {

        return DeviceManager::DeviceSetupStatusSuccess;
    }

    if (device->deviceClassId() == digitalInputDeviceClassId) {

        return DeviceManager::DeviceSetupStatusSuccess;
    }

    if (device->deviceClassId() == lockDeviceClassId) {

        return DeviceManager::DeviceSetupStatusSuccess;
    }

    if (device->deviceClassId() == analogInputDeviceClassId) {

        return DeviceManager::DeviceSetupStatusSuccess;
    }

    if (device->deviceClassId() == analogOutputDeviceClassId) {

        return DeviceManager::DeviceSetupStatusSuccess;
    }

    if (device->deviceClassId() == blindDeviceClassId) {

        return DeviceManager::DeviceSetupStatusSuccess;
    }

    if (device->deviceClassId() == lightDeviceClassId) {

        return DeviceManager::DeviceSetupStatusSuccess;
    }

    if (device->deviceClassId() == dimmerSwitchDeviceClassId) {

        DimmerSwitch* dimmerSwitch = new DimmerSwitch(this);

        connect(dimmerSwitch, &DimmerSwitch::pressed, this, &DevicePluginUniPi::onDimmerSwitchPressed);
        connect(dimmerSwitch, &DimmerSwitch::longPressed, this, &DevicePluginUniPi::onDimmerSwitchLongPressed);
        connect(dimmerSwitch, &DimmerSwitch::doublePressed, this, &DevicePluginUniPi::onDimmerSwitchDoublePressed);
        connect(dimmerSwitch, &DimmerSwitch::dimValueChanged, this, &DevicePluginUniPi::onDimmerSwitchDimValueChanged);
        m_dimmerSwitches.insert(dimmerSwitch, device);
        return DeviceManager::DeviceSetupStatusSuccess;
    }
    return DeviceManager::DeviceSetupStatusFailure;
}

void DevicePluginUniPi::postSetupDevice(Device *device)
{
    if(device->deviceClassId() == neuronL403DeviceClassId) {
        QList<DeviceDescriptor> relayOutputDescriptors;
        QList<DeviceDescriptor> lightDescriptors;
        QList<DeviceDescriptor> blindDescriptors;
        QList<DeviceDescriptor> lockDescriptors;
        QList<DeviceDescriptor> digitalInputDescriptors;

        foreach (Param param, device->params()) {
            if (param.value().toString().contains("Unconfigured", Qt::CaseSensitivity::CaseInsensitive)) {
                DeviceClass deviceClass = deviceManager()->findDeviceClass(neuronL403DeviceClassId);
                QString displayName = deviceClass.paramTypes().findById(param.paramTypeId()).displayName();

                if(deviceClass.paramTypes().findById(param.paramTypeId()).name().contains("relay", Qt::CaseSensitivity::CaseInsensitive)) {

                    QString outputNumber = displayName.split(" ").at(1);

                    foreach(Device *existingDevice, deviceManager()->findChildDevices(device->id())) {
                        qCDebug(dcUniPi()) << "Removing device" << outputNumber;
                        if(existingDevice->deviceClassId() == relayOutputDeviceClassId) {
                            if (device->paramValue(relayOutputDeviceNumberParamTypeId).toString() == outputNumber) {
                                deviceManager()->removeConfiguredDevice(existingDevice->id());
                            }
                        }
                        if(existingDevice->deviceClassId() == lightDeviceClassId) {
                            if (device->paramValue(lightDeviceOutputParamTypeId).toString() == outputNumber) {
                                deviceManager()->removeConfiguredDevice(existingDevice->id());
                            }
                        }
                        if(existingDevice->deviceClassId() == lockDeviceClassId) {
                            if (device->paramValue(lockDeviceNumberParamTypeId).toString() == outputNumber) {
                                deviceManager()->removeConfiguredDevice(existingDevice->id());
                            }
                        }
                        if(existingDevice->deviceClassId() ==  blindDeviceClassId) {
                            if ((device->paramValue(blindDeviceOutputOpenParamTypeId).toString() == outputNumber) ||
                                    (device->paramValue(blindDeviceOutputCloseParamTypeId).toString() == outputNumber)) {
                                deviceManager()->removeConfiguredDevice(existingDevice->id());
                            }
                        }
                        break;
                    }
                }
                if(deviceClass.paramTypes().findById(param.paramTypeId()).name().contains("Input", Qt::CaseSensitivity::CaseInsensitive)) {
                    QString outputNumber = displayName.split(" ").at(1);

                    foreach(Device *existingDevice, deviceManager()->findChildDevices(device->id())) {
                        qCDebug(dcUniPi()) << "Removing device" << outputNumber;
                        if(existingDevice->deviceClassId() == digitalInputDeviceClassId) {
                            if (device->paramValue(digitalInputDeviceNumberParamTypeId).toString() == outputNumber) {
                                deviceManager()->removeConfiguredDevice(existingDevice->id());
                            }
                        }
                        if(existingDevice->deviceClassId() == dimmerSwitchDeviceClassId) {
                            if (device->paramValue(dimmerSwitchDeviceInputNumberParamTypeId).toString() == outputNumber) {
                                deviceManager()->removeConfiguredDevice(existingDevice->id());
                            }
                        }
                    }
                }
            }

            if (param.value().toString().contains("Generic ouput", Qt::CaseSensitivity::CaseInsensitive)) {
                DeviceClass deviceClass = deviceManager()->findDeviceClass(neuronL403DeviceClassId);
                QString displayName = deviceClass.paramTypes().findById(param.paramTypeId()).displayName();
                QString outputNumber = displayName.split(" ").at(1);
                //TODO improve

                if(!myDevices().filterByParam(relayOutputDeviceNumberParamTypeId, outputNumber).isEmpty()) {
                    qDebug(dcUniPi()) << "Skipping device because already added" << outputNumber;
                    continue;
                }

                DeviceDescriptor deviceDescriptor(relayOutputDeviceClassId, QString("Relais %1").arg(outputNumber), "", device->id());
                ParamList params;
                params.append(Param(relayOutputDeviceNumberParamTypeId, outputNumber));
                deviceDescriptor.setParams(params);
                relayOutputDescriptors.append(deviceDescriptor);
                qCDebug(dcUniPi()) << "Adding generic output:" << outputNumber;
            }

            if (param.value().toString() == "Lock") {
                DeviceClass deviceClass = deviceManager()->findDeviceClass(neuronL403DeviceClassId);
                QString displayName = deviceClass.paramTypes().findById(param.paramTypeId()).displayName();
                QString outputNumber = displayName.split(" ").at(1);

                if(!myDevices().filterByParam(lockDeviceNumberParamTypeId, outputNumber).isEmpty()) {
                    qDebug(dcUniPi()) << "Skipping device because already added" << outputNumber;
                    continue;
                }

                DeviceDescriptor deviceDescriptor(lockDeviceClassId, QString("Lock %1").arg(outputNumber), "", device->id());
                ParamList params;
                params.append(Param(lockDeviceNumberParamTypeId, outputNumber));
                params.append(Param(lockDeviceUnlatchTimeParamTypeId, 60));
                deviceDescriptor.setParams(params);
                lockDescriptors.append(deviceDescriptor);
            }

            if (param.value().toString() == "Light") {
                DeviceClass deviceClass = deviceManager()->findDeviceClass(neuronL403DeviceClassId);
                QString displayName = deviceClass.paramTypes().findById(param.paramTypeId()).displayName();
                QString outputNumber = displayName.split(" ").at(1);

                if(!myDevices().filterByParam(lightDeviceOutputParamTypeId, outputNumber).isEmpty()) {
                    qDebug(dcUniPi()) << "Skipping device because already added" << outputNumber;
                    continue;
                }

                DeviceDescriptor deviceDescriptor(lightDeviceClassId, QString("Light %1").arg(outputNumber), "", device->id());
                ParamList params;
                params.append(Param(lightDeviceOutputParamTypeId, outputNumber));
                deviceDescriptor.setParams(params);
                lightDescriptors.append(deviceDescriptor);
            }

            if (param.value().toString() == "Blind open") {
                DeviceClass deviceClass = deviceManager()->findDeviceClass(neuronL403DeviceClassId);
                QString openFunctionParamDisplayName = deviceClass.paramTypes().findById(param.paramTypeId()).displayName();
                QString outputOpenNumber = openFunctionParamDisplayName.split(" ").at(1);

                if(!myDevices().filterByParam(blindDeviceOutputOpenParamTypeId, outputOpenNumber).isEmpty()) {
                    qDebug(dcUniPi()) << "Skipping device because open output already used" << outputOpenNumber;
                    continue;
                }

                QString openFunctionParamName = deviceClass.paramTypes().findById(param.paramTypeId()).name();
                ParamType openGroupParamType = deviceClass.paramTypes().findByName(openFunctionParamName.replace("Function", "Group", Qt::CaseSensitivity::CaseInsensitive));
                int groupNumber = device->paramValue(openGroupParamType.id()).toInt();

                QString outputCloseNumber;
                foreach (Param closeParam, device->params()) {
                    if (closeParam.paramTypeId() != openGroupParamType.id()) {
                        if (closeParam.value().toInt() == groupNumber) {
                            QString closeGroupParamName = deviceClass.paramTypes().findById(closeParam.paramTypeId()).name();
                            ParamType closeFunctionParamType = deviceClass.paramTypes().findByName(closeGroupParamName.replace("Group", "Function", Qt::CaseSensitivity::CaseInsensitive));
                            if (device->paramValue(closeFunctionParamType.id()).toString() == "Blind close") {
                                QString closeFunctionParamDisplayName = deviceClass.paramTypes().findById(closeParam.paramTypeId()).displayName();
                                outputCloseNumber = closeFunctionParamDisplayName.split(" ").at(1);
                            }
                        }
                    }
                }

                if(!myDevices().filterByParam(blindDeviceOutputCloseParamTypeId, outputCloseNumber).isEmpty()) {
                    qDebug(dcUniPi()) << "Skipping device because close output already used" << outputCloseNumber;
                    continue;
                }

                DeviceDescriptor deviceDescriptor(blindDeviceClassId, QString("Blind %1").arg(outputOpenNumber), "", device->id());
                ParamList params;
                params.append(Param(blindDeviceOutputOpenParamTypeId, outputOpenNumber));
                params.append(Param(blindDeviceOutputCloseParamTypeId, outputCloseNumber));
                deviceDescriptor.setParams(params);
                blindDescriptors.append(deviceDescriptor);
            }

            if (param.value().toString().contains("Generic input", Qt::CaseSensitivity::CaseInsensitive)) {
                DeviceClass deviceClass = deviceManager()->findDeviceClass(neuronL403DeviceClassId);
                QString displayName = deviceClass.paramTypes().findById(param.paramTypeId()).displayName();
                QString circuit = displayName.split(" ").at(2);

                if(!myDevices().filterByParam(digitalInputDeviceNumberParamTypeId, circuit).isEmpty()) {
                    qDebug(dcUniPi()) << "Skipping device because already added" << circuit;
                    continue;
                }

                DeviceDescriptor deviceDescriptor(digitalInputDeviceClassId, QString("Digital input %1").arg(circuit), "", device->id());
                ParamList params;
                params.append(Param(digitalInputDeviceNumberParamTypeId, circuit));
                deviceDescriptor.setParams(params);
                digitalInputDescriptors.append(deviceDescriptor);
                qCDebug(dcUniPi()) << "Adding digital input:" << circuit;
            }
        }
        if (!relayOutputDescriptors.isEmpty())
            emit autoDevicesAppeared(relayOutputDeviceClassId, relayOutputDescriptors);

        if (!lightDescriptors.isEmpty())
            emit autoDevicesAppeared(lightDeviceClassId, lightDescriptors);

        if (!blindDescriptors.isEmpty())
            emit autoDevicesAppeared(blindDeviceClassId, blindDescriptors);

        if (!digitalInputDescriptors.isEmpty())
            emit autoDevicesAppeared(digitalInputDeviceClassId, digitalInputDescriptors);

        if (!lockDescriptors.isEmpty())
            emit autoDevicesAppeared(lockDeviceClassId, lockDescriptors);
    }

    if(device->deviceClassId() == neuronXS30DeviceClassId) {

        QList<DeviceDescriptor> digitalInputDescriptors;
        foreach (Param param, device->params()) {
            if (param.value().toString().contains("Generic input", Qt::CaseSensitivity::CaseInsensitive)) {
                DeviceClass deviceClass = deviceManager()->findDeviceClass(neuronXS30DeviceClassId);
                QString displayName = deviceClass.paramTypes().findById(param.paramTypeId()).displayName();
                QString circuit = displayName.split(" ").at(2);

                if(!myDevices().filterByParam(digitalInputDeviceNumberParamTypeId, circuit).isEmpty()) {
                    qDebug(dcUniPi()) << "Skipping device because already added" << circuit;
                    continue;
                }

                DeviceDescriptor deviceDescriptor(digitalInputDeviceClassId, QString("Digital input %1").arg(circuit), "", device->id());
                ParamList params;
                params.append(Param(digitalInputDeviceNumberParamTypeId, circuit));
                deviceDescriptor.setParams(params);
                digitalInputDescriptors.append(deviceDescriptor);
                qCDebug(dcUniPi()) << "Adding digital input:" << circuit;
            }
        }

        if (!digitalInputDescriptors.isEmpty())
            emit autoDevicesAppeared(digitalInputDeviceClassId, digitalInputDescriptors);
    }
    if (device->deviceClassId() == lockDeviceClassId) {
        QTimer *unlatchTimer = new QTimer(this);
        connect(unlatchTimer, &QTimer::timeout, this, &DevicePluginUniPi::onUnlatchTimer);
        unlatchTimer->setSingleShot(true);
        m_unlatchTimer.insert(device, unlatchTimer);
    }
}


DeviceManager::DeviceError DevicePluginUniPi::executeAction(Device *device, const Action &action)
{
    if (device->deviceClassId() == lockDeviceClassId) {
        if (action.actionTypeId() == lockUnlatchActionTypeId) {
            QString digitalOutputNumber = device->paramValue(lockDeviceNumberParamTypeId).toString();
            if (m_neurons.contains(device->parentId())) {
                Neuron *neuron = m_neurons.value(device->parentId());
                neuron->setDigitalOutput(digitalOutputNumber, true);
            }
            if (m_neuronExtensions.contains(device->parentId())) {
                NeuronExtension *neuronExtension = m_neuronExtensions.value(device->parentId());
                neuronExtension->setDigitalOutput(digitalOutputNumber, true);
            }

            QTimer *unlatchTimer = m_unlatchTimer.value(device);
            int time = device->paramValue(lockDeviceUnlatchTimeParamTypeId).toInt()*1000;
            unlatchTimer->start(time);
            qCDebug(dcUniPi()) << "Starting unlatch timer, time in sec:" << time;
        }

        if (action.actionTypeId() == lockPermanentlyUnlatchedActionTypeId) {
            QString digitalOutputNumber = device->paramValue(lockDeviceNumberParamTypeId).toString();
            bool stateValue = action.param(lockPermanentlyUnlatchedActionPermanentlyUnlatchedParamTypeId).value().toBool();
            if (m_neurons.contains(device->parentId())) {
                Neuron *neuron = m_neurons.value(device->parentId());
                neuron->setDigitalOutput(digitalOutputNumber, stateValue);
            }
            if (m_neuronExtensions.contains(device->parentId())) {
                NeuronExtension *neuronExtension = m_neuronExtensions.value(device->parentId());
                neuronExtension->setDigitalOutput(digitalOutputNumber, stateValue);
            }
        }
    }

    if ((device->deviceClassId() == digitalOutputDeviceClassId) ||
            (device->deviceClassId() == relayOutputDeviceClassId))  {
        //TODO unpredictable behaviour
        if (action.actionTypeId() == digitalOutputPowerActionTypeId) {
            QString digitalOutputNumber = device->paramValue(digitalOutputDeviceNumberParamTypeId).toString();
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
        return Device::DeviceErrorActionTypeNotFound;
    }

    if (device->deviceClassId() == analogOutputDeviceClassId) {

        if (action.actionTypeId() == analogOutputOutputValueActionTypeId) {
            /*QString analogOutputNumber = device->paramValue(analogOutputDeviceOutputNumberParamTypeId).toString();
            double analogValue = action.param(analogOutputOutputValueActionOutputValueParamTypeId).value().toDouble();
            Neuron *neuron = m_neurons.value(device->parentId()); //TODO what if parent is an extension
            neuron->setAnalogOutput(digitalOutputNumber, stateValue);
*/
            return DeviceManager::DeviceErrorNoError;
        }
        return Device::DeviceErrorActionTypeNotFound;
    }

    if (device->deviceClassId() == blindDeviceClassId) {
        QString circuitOpen = device->paramValue(blindDeviceOutputOpenParamTypeId).toString();
        QString circuitClose = device->paramValue(blindDeviceOutputCloseParamTypeId).toString();
        bool openValue = false;
        bool closeValue = false;

        if (action.actionTypeId() == blindCloseActionTypeId) {
            openValue =false;
            closeValue = true;
        }
        if (action.actionTypeId() == blindOpenActionTypeId) {
            openValue =true;
            closeValue = false;
        }
        if (action.actionTypeId() == blindStopActionTypeId) {
            openValue =false;
            closeValue = false;
        }

        if (m_neurons.contains(device->parentId())) {
            Neuron *neuron = m_neurons.value(device->parentId());
            neuron->setDigitalOutput(circuitOpen, openValue);
            neuron->setDigitalOutput(circuitClose, closeValue);
        }
        if (m_neuronExtensions.contains(device->parentId())) {
            NeuronExtension *neuronExtension = m_neuronExtensions.value(device->parentId());
            neuronExtension->setDigitalOutput(circuitOpen, openValue);
            neuronExtension->setDigitalOutput(circuitClose, openValue);
        }
        return DeviceManager::DeviceErrorNoError;
    }

    if (device->deviceClassId() == lightDeviceClassId) {

        QString circuit = device->paramValue(lightDeviceOutputParamTypeId).toString();
        bool stateValue = action.param(lightPowerActionPowerParamTypeId).value().toBool();

        if (m_neurons.contains(device->parentId())) {
            Neuron *neuron = m_neurons.value(device->parentId());
            neuron->setDigitalOutput(circuit, stateValue);
        }else if (m_neuronExtensions.contains(device->parentId())) {
            NeuronExtension *neuronExtension = m_neuronExtensions.value(device->parentId());
            neuronExtension->setDigitalOutput(circuit, stateValue);
        } else {
            qCWarning(dcUniPi()) << "No valid parent ID";
        }
        return DeviceManager::DeviceErrorNoError;
    }

    return Device::DeviceErrorDeviceClassNotFound;
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

void DevicePluginUniPi::onDimmerSwitchPressed()
{
    DimmerSwitch *dimmerSwitch = static_cast<DimmerSwitch *>(sender());
    Device *device = m_dimmerSwitches.value(dimmerSwitch);
    emit emitEvent(Event(dimmerSwitchPressedEventTypeId, device->id()));
}

void DevicePluginUniPi::onDimmerSwitchLongPressed()
{
    DimmerSwitch *dimmerSwitch = static_cast<DimmerSwitch *>(sender());
    Device *device = m_dimmerSwitches.value(dimmerSwitch);
    emit emitEvent(Event(dimmerSwitchLongPressedEventTypeId, device->id()));
}

void DevicePluginUniPi::onDimmerSwitchDoublePressed()
{
    DimmerSwitch *dimmerSwitch = static_cast<DimmerSwitch *>(sender());
    Device *device = m_dimmerSwitches.value(dimmerSwitch);
    emit emitEvent(Event(dimmerSwitchDoublePressedEventTypeId, device->id()));
}

void DevicePluginUniPi::onDimmerSwitchDimValueChanged(int dimValue)
{
    DimmerSwitch *dimmerSwitch = static_cast<DimmerSwitch *>(sender());
    Device *device = m_dimmerSwitches.value(dimmerSwitch);
    device->setStateValue(dimmerSwitchDimValueStateTypeId, dimValue);
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
}

void DevicePluginUniPi::onDigitalInputStatusChanged(QString &circuit, bool value)
{
    foreach(Device *device, myDevices()) {
        if (device->deviceClassId() == digitalInputDeviceClassId) {
            if (device->paramValue(digitalInputDeviceNumberParamTypeId).toString() == circuit) {

                device->setStateValue(digitalInputInputStatusStateTypeId, value);
                return;
            }
        }
        if (device->deviceClassId() == dimmerSwitchDeviceClassId) {
            if (device->paramValue(dimmerSwitchDeviceInputNumberParamTypeId).toString() == circuit) {

                device->setStateValue(dimmerSwitchStatusStateTypeId, value);
                DimmerSwitch *dimmerSwitch = m_dimmerSwitches.key(device);
                dimmerSwitch->setPower(value);
                return;
            }
        }
    }
}

void DevicePluginUniPi::onDigitalOutputStatusChanged(QString &circuit, bool value)
{
    foreach(Device *device, myDevices()) {
        if (device->deviceClassId() == relayOutputDeviceClassId) {
            if (device->paramValue(relayOutputDeviceNumberParamTypeId).toString() == circuit) {

                device->setStateValue(relayOutputPowerStateTypeId, value);
                return;
            }
        }

        if (device->deviceClassId() == lightDeviceClassId) {
            if(device->paramValue(lightDeviceOutputParamTypeId).toString() == circuit) {

                device->setStateValue(lightPowerStateTypeId, value);
                return;
            }
        }

        if (device->deviceClassId() == lockDeviceClassId) {
            if(device->paramValue(lockDeviceNumberParamTypeId).toString() == circuit) {

                device->setStateValue(lockStateStateTypeId, value);
                return;
            }
        }

        if (device->deviceClassId() == blindDeviceClassId) {
            if (device->paramValue(blindDeviceOutputCloseParamTypeId).toString() == circuit){
                QString state = device->stateValue(blindStatusStateTypeId).toString();
                if (!value && (state == "closing")) {
                    device->setStateValue(blindStatusStateTypeId, "stopped");
                } else if (value && (state == "stopped")) {
                    device->setStateValue(blindStatusStateTypeId, "closing");
                } else {
                    qCDebug(dcUniPi()) << "Warning illegal blind status";
                    device->setStateValue(blindStatusStateTypeId, "stopped");
                }
                return;
            }
            if (device->paramValue(blindDeviceOutputOpenParamTypeId).toString() == circuit) {
                QString state = device->stateValue(blindStatusStateTypeId).toString();
                if (!value && (state == "opening")) {
                    device->setStateValue(blindStatusStateTypeId, "stopped");
                } else if (value && (state == "stopped")) {
                    device->setStateValue(blindStatusStateTypeId, "opening");
                } else {
                    qCDebug(dcUniPi()) << "Warning illegal blind status";
                    device->setStateValue(blindStatusStateTypeId, "stopped");
                }
                return;
            }
        }
    }
}

void DevicePluginUniPi::onUnlatchTimer()
{
    QTimer *timer= static_cast<QTimer*>(sender());
    Device *device = m_unlatchTimer.key(timer);
    QString digitalOutputNumber = device->paramValue(lockDeviceNumberParamTypeId).toString();
    if (m_neurons.contains(device->parentId())) {
        Neuron *neuron = m_neurons.value(device->parentId());
        neuron->setDigitalOutput(digitalOutputNumber, false);
    }
    if (m_neuronExtensions.contains(device->parentId())) {
        NeuronExtension *neuronExtension = m_neuronExtensions.value(device->parentId());
        neuronExtension->setDigitalOutput(digitalOutputNumber, false);
    }
}

void DevicePluginUniPi::onReconnectTimer()
{
    if(!m_modbusRTUMaster) {
        if (!m_modbusRTUMaster->connectDevice()) {
            m_reconnectTimer->start(10000);
        }
    }
    if(!m_modbusTCPMaster) {
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
