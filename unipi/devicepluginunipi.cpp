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

DevicePluginUniPi::DevicePluginUniPi()
{
}


Device::DeviceSetupStatus DevicePluginUniPi::setupDevice(Device *device)
{
    if(!m_refreshTimer) {
        m_refreshTimer = hardwareManager()->pluginTimerManager()->registerTimer(1); //needs to be faster for the input polling
        connect(m_refreshTimer, &PluginTimer::timeout, this, &DevicePluginUniPi::onRefreshTimer);
    }

    if(device->deviceClassId() == neuronL403DeviceClassId) {

        int port = configValue(uniPiPluginPortParamTypeId).toInt();;
        QHostAddress ipAddress = QHostAddress(configValue(uniPiPluginAddressParamTypeId).toString());
        m_modbusTCPMaster = new ModbusTCPMaster(ipAddress, port, this);
        if(!m_modbusTCPMaster->createInterface()) {
            qCWarning(dcUniPi()) << "Could not create interface";
            m_modbusTCPMaster->deleteLater();
            return DeviceManager::DeviceSetupStatusFailure;
        }

        Neuron *neuron = new Neuron(Neuron::NeuronTypes::L403, m_modbusTCPMaster, this);
        if (!neuron->loadModbusMap()) {
            qCWarning(dcUniPi()) << "Could not load the modbus map";
            neuron->deleteLater();
            return DeviceManager::DeviceSetupStatusFailure;
        }
        m_neurons.insert(device->id(), neuron);

        device->setStateValue(neuronL403ConnectedStateTypeId, true);

        return DeviceManager::DeviceSetupStatusSuccess;
    }

    if(device->deviceClassId() == neuronXS30DeviceClassId) {

        QString serialPort = configValue(uniPiPluginSerialPortParamTypeId).toString();
        int slaveAddress = device->paramValue(neuronXS30DeviceSlaveAddressParamTypeId).toInt();

        if(!m_modbusRTUMaster) {
            // Seems to be the first Modbus extension
            m_modbusRTUMaster = new ModbusRTUMaster(serialPort, 19600, "E", 8, 1, this);
            if(!m_modbusRTUMaster->createInterface()) {
                qCWarning(dcUniPi()) << "Could not create interface";
                m_modbusRTUMaster->deleteLater();
                return DeviceManager::DeviceSetupStatusFailure;
            }
        }
        NeuronExtension *neuronExtension = new NeuronExtension(NeuronExtension::ExtensionTypes::xS30, m_modbusRTUMaster, slaveAddress, this);
        if (!neuronExtension->loadModbusMap()) {
            qCWarning(dcUniPi()) << "Could not load the modbus map";
            neuronExtension->deleteLater();
            return DeviceManager::DeviceSetupStatusFailure;
        }
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
        /*
        DimmerSwitch* dimmerSwitch = new DimmerSwitch(this);

        connect(dimmerSwitch, &DimmerSwitch::pressed, this, &DevicePluginUniPi::onDimmerSwitchPressed);
        connect(dimmerSwitch, &DimmerSwitch::longPressed, this, &DevicePluginUniPi::onDimmerSwitchLongPressed);
        connect(dimmerSwitch, &DimmerSwitch::doublePressed, this, &DevicePluginUniPi::onDimmerSwitchDoublePressed);
        connect(dimmerSwitch, &DimmerSwitch::dimValueChanged, this, &DevicePluginUniPi::onDimmerSwitchDimValueChanged);
        m_dimmerSwitches.insert(dimmerSwitch, device);*/
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
            if (param.value().toString() == "Generic") {
                DeviceClass deviceClass = deviceManager()->findDeviceClass(neuronL403DeviceClassId);
                QString displayName = deviceClass.paramTypes().findById(param.paramTypeId()).displayName();
                QString outputNumber = displayName.split(" ").at(1);

                if(!myDevices().filterByParam(relayOutputDeviceNumberParamTypeId, outputNumber).isEmpty()) {
                    qDebug(dcUniPi()) << "Skipping device because already added" << outputNumber;
                    continue;
                }

                DeviceDescriptor deviceDescriptor(relayOutputDeviceClassId, QString("Relais %1").arg(outputNumber), "", device->id());
                ParamList params;
                params.append(Param(relayOutputDeviceNumberParamTypeId, outputNumber));
                deviceDescriptor.setParams(params);
                deviceDescriptor.setParentDeviceId(device->id());
                relayOutputDescriptors.append(deviceDescriptor);
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
                deviceDescriptor.setParams(params);
                deviceDescriptor.setParentDeviceId(device->id());
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

            if (param.value().toString() == "Generic") {
                DeviceClass deviceClass = deviceManager()->findDeviceClass(neuronL403DeviceClassId);
                QString displayName = deviceClass.paramTypes().findById(param.paramTypeId()).displayName();
                QString circuit = displayName.split(" ").at(1);

                if(!myDevices().filterByParam(digitalInputDeviceNumberParamTypeId, circuit).isEmpty()) {
                    qDebug(dcUniPi()) << "Skipping device because already added" << circuit;
                    continue;
                }

                DeviceDescriptor deviceDescriptor(digitalInputDeviceClassId, QString("Digital input %1").arg(circuit), "", device->id());
                ParamList params;
                params.append(Param(digitalInputDeviceNumberParamTypeId, circuit));
                deviceDescriptor.setParams(params);
                digitalInputDescriptors.append(deviceDescriptor);
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
            if (param.value().toString() == "Generic") {
                DeviceClass deviceClass = deviceManager()->findDeviceClass(neuronXS30DeviceClassId);
                QString displayName = deviceClass.paramTypes().findById(param.paramTypeId()).displayName();
                QString circuit = displayName.split(" ").at(1);

                if(!myDevices().filterByParam(digitalInputDeviceNumberParamTypeId, circuit).isEmpty()) {
                    qDebug(dcUniPi()) << "Skipping device because already added" << circuit;
                    continue;
                }

                DeviceDescriptor deviceDescriptor(digitalInputDeviceClassId, QString("Digital input %1").arg(circuit), "", device->id());
                ParamList params;
                params.append(Param(digitalInputDeviceNumberParamTypeId, circuit));
                deviceDescriptor.setParams(params);
                digitalInputDescriptors.append(deviceDescriptor);
            }
        }

        if (!digitalInputDescriptors.isEmpty())
            emit autoDevicesAppeared(digitalInputDeviceClassId, digitalInputDescriptors);
    }
    if (device->deviceClassId() == lockDeviceClassId) {
        QTimer *unlatchTimer = new QTimer(this);
        unlatchTimer->setSingleShot(true);
        unlatchTimer->setInterval(device->paramValue(lockDeviceUnlatchTimeParamTypeId).toInt() * 1000);
        m_unlatchTimer.insert(device, unlatchTimer);
    }
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

    if (myDevices().isEmpty()) {
        m_refreshTimer->stop();
        hardwareManager()->pluginTimerManager()->unregisterTimer(m_refreshTimer);
    }
}


DeviceManager::DeviceError DevicePluginUniPi::executeAction(Device *device, const Action &action)
{
    if (!m_modbusTCPMaster)
        return DeviceManager::DeviceErrorHardwareNotAvailable;

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
            unlatchTimer->start();
            connect(unlatchTimer, &QTimer::timeout, this, [this]() {
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
            });
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

            return Device::DeviceErrorNoError;
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
            Neuron *neuron = m_neurons.value(device->parentId()); //TODO what if parent is an extension
            neuron->setDigitalOutput(circuit, stateValue);
        }else if (m_neuronExtensions.contains(device->parentId())) {
            NeuronExtension *neuronExtension = m_neuronExtensions.value(device->parentId()); //TODO what if parent is an extension
            neuronExtension->setDigitalOutput(circuit, stateValue);
        } else {
            qCWarning(dcUniPi()) << "No valid parent ID";
        }
        return DeviceManager::DeviceErrorNoError;
    }

    return Device::DeviceErrorDeviceClassNotFound;
}

void DevicePluginUniPi::onRefreshTimer()
{
    foreach(Device *device, myDevices()) {

        if (device->deviceClassId() == relayOutputDeviceClassId) {
            QString circuit = device->paramValue(relayOutputDeviceNumberParamTypeId).toString();
            bool value = false;

            if (m_neurons.contains(device->parentId())) {
                Neuron *neuron = m_neurons.value(device->parentId());
                value = neuron->getDigitalOutput(circuit);
            } else if (m_neuronExtensions.contains(device->parentId())) {
                NeuronExtension *neuronExtension = m_neuronExtensions.value(device->parentId());
                value =  neuronExtension->getDigitalOutput(circuit);
            } else {
                qCWarning(dcUniPi()) << "No valid parent ID";
            }
            device->setStateValue(relayOutputPowerStateTypeId, value);
        }

        if (device->deviceClassId() == lightDeviceClassId) {
            QString circuit = device->paramValue(lightDeviceOutputParamTypeId).toString();
            bool value = false;

            if (m_neurons.contains(device->parentId())) {
                Neuron *neuron = m_neurons.value(device->parentId());
                value = neuron->getDigitalOutput(circuit);
            } else if (m_neuronExtensions.contains(device->parentId())) {
                NeuronExtension *neuronExtension = m_neuronExtensions.value(device->parentId());
                value =  neuronExtension->getDigitalOutput(circuit);
            } else {
                qCWarning(dcUniPi()) << "No valid parent ID";
            }
            device->setStateValue(lightPowerStateTypeId, value);
        }

        if (device->deviceClassId() == blindDeviceClassId) {
            QString openOutputNumber = device->paramValue(blindDeviceOutputOpenParamTypeId).toString();
            QString closeOutputNumber = device->paramValue(blindDeviceOutputOpenParamTypeId).toString();

            bool openValue = false;
            bool closeValue = false;

            if (m_neurons.contains(device->parentId())) {
                Neuron *neuron = m_neurons.value(device->parentId()); //TODO what if parent is an extension
                openValue = neuron->getDigitalOutput(openOutputNumber);
                closeValue = neuron->getDigitalOutput(closeOutputNumber);
            } else if (m_neuronExtensions.contains(device->parentId())) {
                NeuronExtension *neuronExtension = m_neuronExtensions.value(device->parentId()); //TODO what if parent is an extension
                openValue = neuronExtension->getDigitalOutput(openOutputNumber);
                closeValue = neuronExtension->getDigitalOutput(closeOutputNumber);
            } else {
                qCWarning(dcUniPi()) << "No valid parent ID";
            }

            if (!openValue && !closeValue) {
                device->setStateValue(blindStatusStateTypeId, "stopped");
            } else if (openValue && !closeValue) {
                device->setStateValue(blindStatusStateTypeId, "opening");
            } else if (!openValue && closeValue) {
                device->setStateValue(blindStatusStateTypeId, "closing");
            } else {
                qCDebug(dcUniPi()) << "Warning illegal blind status";
                device->setStateValue(blindStatusStateTypeId, "stopped");
            }
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
