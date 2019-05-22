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

DevicePluginUniPi::DevicePluginUniPi()
{
}


Device::DeviceSetupStatus DevicePluginUniPi::setupDevice(Device *device)
{
    if(myDevices().empty()) {
        m_refreshTimer = hardwareManager()->pluginTimerManager()->registerTimer(1);
        connect(m_refreshTimer, &PluginTimer::timeout, this, &DevicePluginUniPi::onRefreshTimer);
    }

    if(device->deviceClassId() == neuronL403DeviceClassId) {

        int port = configValue(uniPiPluginPortParamTypeId).toInt();;
        QHostAddress ipAddress = QHostAddress(configValue(uniPiPluginAddressParamTypeId).toString());
        m_modbusTCPMaster = new ModbusTCPMaster(ipAddress, port, this);
        m_neuronModel = "L403";

        QList<DeviceDescriptor> lightDescriptors;
        foreach (Param param, device->params()) {
            if (param.value().toString() == "Generic") {

            }

            if (param.value().toString() == "Light") {
                DeviceClass deviceClass = deviceManager()->findDeviceClass(neuronL403DeviceClassId);
                QString displayName = deviceClass.paramTypes().findById(param.paramTypeId()).displayName();
                QString outputNumber = displayName.mid(5, 3);

                DeviceDescriptor deviceDescriptor(lightDeviceClassId, "Light", outputNumber);
                ParamList params;
                params.append(Param(lightDeviceOutputParamTypeId, outputNumber));
                deviceDescriptor.setParams(params);
                deviceDescriptor.setParentDeviceId(device->id());
                lightDescriptors.append(deviceDescriptor);
            }

            if (param.value().toString() == "Blind open") {
                DeviceClass deviceClass = deviceManager()->findDeviceClass(neuronL403DeviceClassId);
                QString displayName = deviceClass.paramTypes().findById(param.paramTypeId()).displayName();
                QString outputNumber = displayName.mid(5, 3);

                DeviceDescriptor deviceDescriptor(lightDeviceClassId, "Blind", outputNumber);
                ParamList params;
                params.append(Param(blindDeviceOutputOpenParamTypeId, outputNumber));
                params.append(Param(blindDeviceOutputCloseParamTypeId, outputNumber));
                deviceDescriptor.setParams(params);
                deviceDescriptor.setParentDeviceId(device->id());
                lightDescriptors.append(deviceDescriptor);
            }
        }
        if (!lightDescriptors.isEmpty())
            emit autoDevicesAppeared(lightDeviceClassId, lightDescriptors);

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


void DevicePluginUniPi::deviceRemoved(Device *device)
{
    if(device->deviceClassId() == neuronL403DeviceClassId) {
        m_modbusTCPMaster->deleteLater();
        m_neuronModel = nullptr;

    } else if(device->deviceClassId() == relayOutputDeviceClassId) {

    } else if(device->deviceClassId() == digitalOutputDeviceClassId) {

    } else if(device->deviceClassId() == digitalInputDeviceClassId) {

    } else if (device->deviceClassId() == analogOutputDeviceClassId) {

    } else if (device->deviceClassId() == analogInputDeviceClassId) {

    } else if (device->deviceClassId() == blindDeviceClassId) {

    } else if (device->deviceClassId() == lightDeviceClassId) {

    } else if (device->deviceClassId() == dimmerSwitchDeviceClassId) {

    }

    if (myDevices().isEmpty()) {
        m_refreshTimer->stop();
        hardwareManager()->pluginTimerManager()->unregisterTimer(m_refreshTimer);
    }
}

void DevicePluginUniPi::setDigitalOutput(const QString &circuit, bool value)
{
    //read CSV file
    QString csvFilePath = QString("/opt/neuron/Neuron_%1/Neuron_%2-Coils-group-%3.csv").arg(m_neuronModel).arg(m_neuronModel).arg(circuit.at(0));
    qDebug(dcUniPi()) << "Open CSV File:" << csvFilePath;
    QFile *csvFile = new QFile(csvFilePath);
    if (!csvFile->open(QIODevice::ReadOnly | QIODevice::Text)) {
        qCDebug(dcUniPi()) << csvFile->errorString();
        csvFile->deleteLater();
        return;
    }

    QTextStream *textStream = new QTextStream(csvFile);
    while (!textStream->atEnd()) {
        QString line = textStream->readLine();
        QStringList list = line.split(',');
        if (list[3].contains(circuit)) {
            int modbusAddress = list[0].toInt();
            if(!m_modbusTCPMaster->setCoil(0, modbusAddress, value)) {
                qCWarning(dcUniPi()) << "Error reading coil:" << modbusAddress;
            }
            break;
        }
    }
    csvFile->deleteLater();
}

bool DevicePluginUniPi::getDigitalOutput(const QString &circuit)
{
    //read CSV file
    QString csvFilePath = QString("/opt/neuron/Neuron_%1/Neuron_%2-Coils-group-%3.csv").arg(m_neuronModel).arg(m_neuronModel).arg(circuit.at(0));
    qDebug(dcUniPi()) << "Open CSV File:" << csvFilePath;
    QFile *csvFile = new QFile(csvFilePath);
    if (!csvFile->open(QIODevice::ReadOnly | QIODevice::Text)) {
        qCDebug(dcUniPi()) << csvFile->errorString();
        csvFile->deleteLater();
        return false;
    }

    QTextStream *textStream = new QTextStream(csvFile);
    while (!textStream->atEnd()) {
        QString line = textStream->readLine();
        QStringList list = line.split(',');
        if (list[3].contains(circuit)) {
            int modbusAddress = list[0].toInt();
            bool value;
            if (!m_modbusTCPMaster->getCoil(0, modbusAddress, &value)) {
                qCWarning(dcUniPi()) << "Error reading coil:" << modbusAddress;
            } else {
                return value;
            }
            break;
        }
    }
    csvFile->deleteLater();
    return false;
}

Device::DeviceError DevicePluginUniPi::executeAction(Device *device, const Action &action)
{
    if (!m_modbusTCPMaster)
        return DeviceManager::DeviceErrorHardwareNotAvailable;

    if (device->deviceClassId() == relayOutputDeviceClassId) {

        if (action.actionTypeId() == relayOutputPowerActionTypeId) {
            QString relayNumber = device->paramValue(relayOutputDeviceNumberParamTypeId).toString();
            int stateValue = action.param(relayOutputPowerActionPowerParamTypeId).value().toInt();
            setDigitalOutput(relayNumber, stateValue);

            return Device::DeviceErrorNoError;
        }
        return Device::DeviceErrorActionTypeNotFound;
    }

    if (device->deviceClassId() == digitalOutputDeviceClassId) {
        if (action.actionTypeId() == digitalOutputPowerActionTypeId) {
            QString digitalOutputNumber = device->paramValue(digitalOutputDeviceNumberParamTypeId).toString();
            bool stateValue = action.param(digitalOutputPowerActionPowerParamTypeId).value().toBool();
            setDigitalOutput(digitalOutputNumber, stateValue);

            return Device::DeviceErrorNoError;
        }
        return Device::DeviceErrorActionTypeNotFound;
    }

    if (device->deviceClassId() == analogOutputDeviceClassId) {

        if (action.actionTypeId() == analogOutputOutputValueActionTypeId) {
            //QString analogOutputNumber = device->paramValue(analogOutputDeviceOutputNumberParamTypeId).toString();
            //double analogValue = action.param(analogOutputOutputValueActionOutputValueParamTypeId).value().toDouble();

            return DeviceManager::DeviceErrorNoError;
        }
        return Device::DeviceErrorActionTypeNotFound;
    }

    if (device->deviceClassId() == blindDeviceClassId) {
        QString circuitOpen = device->paramValue(blindDeviceOutputOpenParamTypeId).toString();
        QString circuitClose = device->paramValue(blindDeviceOutputCloseParamTypeId).toString();

        if (action.actionTypeId() == blindCloseActionTypeId) {

            setDigitalOutput(circuitOpen, false);
            setDigitalOutput(circuitClose, true);
            return DeviceManager::DeviceErrorNoError;
        }
        if (action.actionTypeId() == blindOpenActionTypeId) {

            setDigitalOutput(circuitClose, false);
            setDigitalOutput(circuitOpen, true);
            return DeviceManager::DeviceErrorNoError;
        }
        if (action.actionTypeId() == blindStopActionTypeId) {
            setDigitalOutput(circuitOpen, false);
            setDigitalOutput(circuitClose, false);

            return Device::DeviceErrorNoError;
        }
        return Device::DeviceErrorActionTypeNotFound;
    }

    if (device->deviceClassId() == lightDeviceClassId) {

        QString circuit = device->paramValue(lightDeviceOutputParamTypeId).toString();
        bool stateValue = action.param(lightPowerActionPowerParamTypeId).value().toBool();

        setDigitalOutput(circuit, stateValue);
        return DeviceManager::DeviceErrorNoError;
    }

    return Device::DeviceErrorDeviceClassNotFound;
}

void DevicePluginUniPi::onRefreshTimer()
{
    foreach(Device *device, myDevices()) {
        if (device->deviceClassId() == neuronL403DeviceClassId) {

        }

        if (device->deviceClassId() == lightDeviceClassId) {
            QString outputNumber = device->paramValue(lightDeviceOutputParamTypeId).toString();
            bool value = getDigitalOutput(outputNumber);
            device->setStateValue(lightPowerStateTypeId, value);
        }

        if (device->deviceClassId() == blindDeviceClassId) {
            QString openOutputNumber = device->paramValue(blindDeviceOutputOpenParamTypeId).toString();
            QString closeOutputNumber = device->paramValue(blindDeviceOutputOpenParamTypeId).toString();
            bool openValue = getDigitalOutput(openOutputNumber);
            bool closeValue = getDigitalOutput(closeOutputNumber);
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
