/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *                                                                         *
 *  Copyright (C) 2017 Bernhard Trinnes <bernhard.trinnes@guh.io>          *
 *  Copyright (C) 2018 Simon Stürz <simon.stuerz@guh.io>                   *
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

DevicePluginUniPi::~DevicePluginUniPi()
{
    hardwareManager()->pluginTimerManager()->unregisterTimer(m_refreshTimer);
}

void DevicePluginUniPi::init()
{
}

DeviceManager::DeviceSetupStatus DevicePluginUniPi::setupDevice(Device *device)
{
    connectToEvok();

    if(myDevices().empty()) {
        m_refreshTimer = hardwareManager()->pluginTimerManager()->registerTimer(60);
        connect(m_refreshTimer, &PluginTimer::timeout, this, &DevicePluginUniPi::onRefreshTimer);
    }

    if (device->deviceClassId() == relayOutputDeviceClassId) {

        m_usedRelais.insert(device->paramValue(relayOutputDeviceNumberParamTypeId).toString(), device);
        return DeviceManager::DeviceSetupStatusSuccess;
    }

    if (device->deviceClassId() == digitalOutputDeviceClassId) {

        m_usedDigitalOutputs.insert(device->paramValue(digitalOutputDeviceNumberParamTypeId).toString(), device);
        return DeviceManager::DeviceSetupStatusSuccess;
    }

    if (device->deviceClassId() == digitalInputDeviceClassId) {

        m_usedDigitalInputs.insert(device->paramValue(digitalInputDeviceNumberParamTypeId).toString(), device);
        requestAllData();
        return DeviceManager::DeviceSetupStatusSuccess;
    }

    if (device->deviceClassId() == analogInputDeviceClassId) {

        m_usedAnalogInputs.insert(device->paramValue(analogInputDeviceInputNumberParamTypeId).toString(), device);
        requestAllData();
        return DeviceManager::DeviceSetupStatusSuccess;
    }

    if (device->deviceClassId() == analogOutputDeviceClassId) {

        m_usedAnalogOutputs.insert(device->paramValue(analogOutputDeviceOutputNumberParamTypeId).toString(), device);
        return DeviceManager::DeviceSetupStatusSuccess;
    }

    if (device->deviceClassId() == blindDeviceClassId) {

        if (device->paramValue(blindDeviceOutputTypeOpenParamTypeId) == GpioType::Relay) {
            m_usedRelais.insert(device->paramValue(blindDeviceOutputOpenParamTypeId).toString(), device);
        } else if (device->paramValue(blindDeviceOutputTypeOpenParamTypeId) == GpioType::DigitalOutput) {
            m_usedDigitalOutputs.insert(device->paramValue(blindDeviceOutputOpenParamTypeId).toString(), device);
        }

        if (device->paramValue(blindDeviceOutputTypeCloseParamTypeId) == GpioType::Relay) {
            m_usedRelais.insert(device->paramValue(blindDeviceOutputCloseParamTypeId).toString(), device);
        } else if (device->paramValue(blindDeviceOutputTypeOpenParamTypeId) == GpioType::DigitalOutput) {
            m_usedDigitalOutputs.insert(device->paramValue(blindDeviceOutputCloseParamTypeId).toString(), device);
        }

        return DeviceManager::DeviceSetupStatusSuccess;
    }

    if (device->deviceClassId() == lightDeviceClassId) {

        if (device->paramValue(lightDeviceOutputTypeParamTypeId) == GpioType::Relay) {
            m_usedRelais.insert(device->paramValue(lightDeviceOutputParamTypeId).toString(), device);
        } else if (device->paramValue(lightDeviceOutputParamTypeId) == GpioType::DigitalOutput) {
            m_usedDigitalOutputs.insert(device->paramValue(lightDeviceOutputParamTypeId).toString(), device);
        }
        return DeviceManager::DeviceSetupStatusSuccess;
    }

    if (device->deviceClassId() == dimmerSwitchDeviceClassId) {
        m_usedDigitalInputs.insert(device->paramValue(dimmerSwitchDeviceInputNumberParamTypeId).toString(), device);
        DimmerSwitch* dimmerSwitch = new DimmerSwitch(this);

        connect(dimmerSwitch, &DimmerSwitch::pressed, this, &DevicePluginUniPi::onDimmerSwitchPressed);
        connect(dimmerSwitch, &DimmerSwitch::longPressed, this, &DevicePluginUniPi::onDimmerSwitchLongPressed);
        connect(dimmerSwitch, &DimmerSwitch::doublePressed, this, &DevicePluginUniPi::onDimmerSwitchDoublePressed);
        connect(dimmerSwitch, &DimmerSwitch::dimValueChanged, this, &DevicePluginUniPi::onDimmerSwitchDimValueChanged);
        m_dimmerSwitches.insert(dimmerSwitch, device);
        return DeviceManager::DeviceSetupStatusSuccess;
    }

    if (device->deviceClassId() == temperatureSensorDeviceClassId) {

        m_usedTemperatureSensors.insert(device->paramValue(temperatureSensorDeviceAddressParamTypeId).toString(), device);
        requestAllData();
        return DeviceManager::DeviceSetupStatusSuccess;
    }

    return DeviceManager::DeviceSetupStatusFailure;
}

DeviceManager::DeviceError DevicePluginUniPi::discoverDevices(const DeviceClassId &deviceClassId, const ParamList &params)
{
    Q_UNUSED(params);
    qSort(m_relais);
    qSort(m_digitalOutputs);
    qSort(m_digitalInputs);
    qSort(m_analogInputs);
    qSort(m_analogOutputs);

    const DeviceClass deviceClass = deviceManager()->findDeviceClass(deviceClassId);
    if (deviceClass.vendorId() == unipiVendorId) {

        if (deviceClassId == relayOutputDeviceClassId) {
            // Create the list of available relais
            QList<DeviceDescriptor> deviceDescriptors;
            for (int i = 0; i < m_relais.count(); i++) {
                const QString circuit = m_relais.at(i);

                // Offer only gpios which arn't in use already
                if (m_usedRelais.contains(circuit)){
                    continue;
                }
                DeviceDescriptor descriptor(deviceClassId, QString("Relay %1").arg(circuit), circuit);
                ParamList parameters;
                parameters.append(Param(relayOutputDeviceNumberParamTypeId, circuit));
                descriptor.setParams(parameters);

                foreach (Device *existingDevice, myDevices()) {
                    if (existingDevice->paramValue(relayOutputDeviceNumberParamTypeId).toString() == circuit) {
                        descriptor.setDeviceId(existingDevice->id());
                        break;
                    }
                }


                deviceDescriptors.append(descriptor);
            }
            emit devicesDiscovered(deviceClassId, deviceDescriptors);
            return DeviceManager::DeviceErrorAsync;
        }

        if (deviceClassId == digitalOutputDeviceClassId) {
            // Create the list of available digital outputs
            QList<DeviceDescriptor> deviceDescriptors;
            for (int i = 0; i < m_digitalOutputs.count(); i++) {
                const QString circuit = m_digitalOutputs.at(i);

                // Offer only gpios which arn't in use already
                if (m_usedDigitalOutputs.contains(circuit)){
                    continue;
                }
                DeviceDescriptor descriptor(deviceClassId, QString("Digital output %1").arg(circuit), circuit);
                ParamList parameters;
                parameters.append(Param(digitalOutputDeviceNumberParamTypeId, circuit));
                descriptor.setParams(parameters);
                deviceDescriptors.append(descriptor);
            }
            emit devicesDiscovered(deviceClassId, deviceDescriptors);
            return DeviceManager::DeviceErrorAsync;
        }

        if (deviceClassId == digitalInputDeviceClassId) {
            // Create the list of available digital inputs
            QList<DeviceDescriptor> deviceDescriptors;
            for (int i = 0; i < m_digitalInputs.count(); i++) {
                const QString circuit = m_digitalInputs.at(i);

                // Offer only digital inputs which arn't in use already
                if (m_usedDigitalInputs.contains(circuit)){
                    continue;
                }
                DeviceDescriptor descriptor(deviceClassId, QString("Digital input %1").arg(circuit), circuit);
                ParamList parameters;
                parameters.append(Param(digitalInputDeviceNumberParamTypeId, circuit));
                descriptor.setParams(parameters);
                deviceDescriptors.append(descriptor);
            }
            emit devicesDiscovered(deviceClassId, deviceDescriptors);
            return DeviceManager::DeviceErrorAsync;
        }

        if (deviceClassId == analogInputDeviceClassId) {
            // Create the list of available digital inputs
            QList<DeviceDescriptor> deviceDescriptors;
            for (int i = 0; i < m_analogInputs.count(); i++) {
                const QString circuit = m_analogInputs.at(i);

                // Offer only analog inputs which aren't in use already
                if (m_usedAnalogInputs.contains(circuit)){
                    continue;
                }
                DeviceDescriptor descriptor(deviceClassId, QString("Analog input %1").arg(circuit), circuit);
                ParamList parameters;
                parameters.append(Param(analogInputDeviceInputNumberParamTypeId, circuit));
                descriptor.setParams(parameters);
                deviceDescriptors.append(descriptor);
            }
            emit devicesDiscovered(deviceClassId, deviceDescriptors);
            return DeviceManager::DeviceErrorAsync;
        }

        if (deviceClassId == analogOutputDeviceClassId) {
            // Create the list of available digital inputs
            QList<DeviceDescriptor> deviceDescriptors;
            for (int i = 0; i < m_analogOutputs.count(); i++) {
                const QString circuit = m_analogOutputs.at(i);

                // Offer only digital inputs which arn't in use already
                if (m_usedAnalogOutputs.contains(circuit)){
                    continue;
                }
                DeviceDescriptor descriptor(deviceClassId, QString("Analog Output %1").arg(circuit), circuit);
                ParamList parameters;
                parameters.append(Param(analogOutputDeviceOutputNumberParamTypeId, circuit));
                descriptor.setParams(parameters);
                deviceDescriptors.append(descriptor);
            }
            emit devicesDiscovered(deviceClassId, deviceDescriptors);
            return DeviceManager::DeviceErrorAsync;
        }

        if (deviceClassId == blindDeviceClassId) {
            // Create the list of available gpios
            QList<DeviceDescriptor> deviceDescriptors;
            for (int i = 0; i < (m_relais.count()-1); i++) {

                const QString openingCircuit = m_relais.at(i);

                // Offer only relais which aren't in use already
                if (m_usedRelais.contains(openingCircuit)){
                    continue;
                }
                for (int a = (i+1); a < (m_relais.count()); a++) {

                    const QString closingCircuit = m_relais.at(a);
                    // Offer only relais which aren't in use already
                    if (!m_usedRelais.contains(closingCircuit)){

                        DeviceDescriptor descriptor(deviceClassId, "Blind", QString("Opening relay %1 | Closing relay %2").arg(openingCircuit, closingCircuit));
                        ParamList parameters;
                        parameters.append(Param(blindDeviceOutputOpenParamTypeId, openingCircuit));
                        parameters.append(Param(blindDeviceOutputCloseParamTypeId, closingCircuit));
                        parameters.append(Param(blindDeviceOutputTypeOpenParamTypeId, GpioType::Relay));
                        parameters.append(Param(blindDeviceOutputTypeCloseParamTypeId, GpioType::Relay));
                        descriptor.setParams(parameters);
                        deviceDescriptors.append(descriptor);
                        break;
                    }
                }
            }

            for (int i = 0; i < (m_digitalOutputs.count()-1); i++) {

                const QString openingCircuit = m_digitalOutputs.at(i);

                // Offer only relais which aren't in use already
                if (m_usedDigitalOutputs.contains(openingCircuit)){
                    continue;
                }
                for (int a = (i+1); a < (m_digitalOutputs.count()); a++) {

                    const QString closingCircuit = m_digitalOutputs.at(a);
                    // Offer only relais which aren't in use already
                    if (!m_usedDigitalOutputs.contains(closingCircuit)){

                        DeviceDescriptor descriptor(deviceClassId, "Blind", QString("Opening output %1 | Closing output %2").arg(openingCircuit, closingCircuit));
                        ParamList parameters;
                        parameters.append(Param(blindDeviceOutputOpenParamTypeId, openingCircuit));
                        parameters.append(Param(blindDeviceOutputCloseParamTypeId, closingCircuit));
                        parameters.append(Param(blindDeviceOutputTypeOpenParamTypeId, GpioType::DigitalOutput));
                        parameters.append(Param(blindDeviceOutputTypeCloseParamTypeId, GpioType::DigitalOutput));
                        descriptor.setParams(parameters);
                        deviceDescriptors.append(descriptor);
                        break;
                    }
                }
            }

            emit devicesDiscovered(deviceClassId, deviceDescriptors);
            return DeviceManager::DeviceErrorAsync;
        }

        if (deviceClassId == lightDeviceClassId) {
            // Create the list of available gpios
            QList<DeviceDescriptor> deviceDescriptors;
            for (int i = 0; i < m_relais.count(); i++) {
                const QString circuit = m_relais.at(i);

                // Offer only gpios which arn't in use already
                if (m_usedRelais.contains(circuit)){
                    continue;
                }
                DeviceDescriptor descriptor(deviceClassId, "Light", QString("Relay %1").arg(circuit));
                ParamList parameters;
                parameters.append(Param(lightDeviceOutputParamTypeId, circuit));
                parameters.append(Param(lightDeviceOutputTypeParamTypeId, GpioType::Relay));
                descriptor.setParams(parameters);
                deviceDescriptors.append(descriptor);
            }

            for (int i = 0; i < m_digitalOutputs.count(); i++) {
                const QString circuit =  m_digitalOutputs.at(i);

                // Offer only gpios which arn't in use already
                if (m_usedDigitalOutputs.contains(circuit)){
                    continue;
                }
                DeviceDescriptor descriptor(deviceClassId, "Light", QString("Digital output %1").arg(circuit));
                ParamList parameters;
                parameters.append(Param(lightDeviceOutputParamTypeId, circuit));
                parameters.append(Param(lightDeviceOutputTypeParamTypeId, GpioType::DigitalOutput));
                descriptor.setParams(parameters);
                deviceDescriptors.append(descriptor);
            }
            emit devicesDiscovered(deviceClassId, deviceDescriptors);
            return DeviceManager::DeviceErrorAsync;
        }

        if (deviceClassId == dimmerSwitchDeviceClassId) {
            // Create the list of available digital inputs
            QList<DeviceDescriptor> deviceDescriptors;
            for (int i = 0; i < m_digitalInputs.count(); i++) {
                const QString circuit = m_digitalInputs.at(i);

                // Offer only digital inputs which arn't in use already
                if (m_usedDigitalInputs.contains(circuit)){
                    continue;
                }
                DeviceDescriptor descriptor(deviceClassId, "Dimmer switch", QString("Digital Input %1").arg(circuit));
                ParamList parameters;
                parameters.append(Param(dimmerSwitchDeviceInputNumberParamTypeId, circuit));
                descriptor.setParams(parameters);
                deviceDescriptors.append(descriptor);
            }
            emit devicesDiscovered(deviceClassId, deviceDescriptors);
            return DeviceManager::DeviceErrorAsync;
        }

        if (deviceClassId == temperatureSensorDeviceClassId) {
            // Create the list of available temperature sensor
            QList<DeviceDescriptor> deviceDescriptors;
            for (int i = 0; i < m_temperatureSensors.count(); i++) {
                const QString circuit = m_temperatureSensors.at(i);

                // Offer only temperature sensors which aren't in use already
                if (m_usedTemperatureSensors.contains(circuit)){
                    continue;
                }
                DeviceDescriptor descriptor(deviceClassId, "Temperature Sensor", circuit);
                ParamList parameters;
                parameters.append(Param(temperatureSensorDeviceAddressParamTypeId, circuit));
                descriptor.setParams(parameters);
                deviceDescriptors.append(descriptor);
            }
            emit devicesDiscovered(deviceClassId, deviceDescriptors);
            return DeviceManager::DeviceErrorAsync;
        }
    }
    return DeviceManager::DeviceErrorDeviceClassNotFound;
}

void DevicePluginUniPi::setOutput(const QString &circuit, bool value)
{
    QJsonObject json;
    json["cmd"] = "set";
    json["dev"] = "relay";
    json["circuit"] = circuit;
    json["value"] = value;

    QJsonDocument doc(json);
    QByteArray bytes = doc.toJson(QJsonDocument::Compact);
    qCDebug(dcUniPi()) << "Send command" << bytes;
    m_webSocket->sendBinaryMessage(bytes);
}

void DevicePluginUniPi::connectToEvok()
{
    if ((m_webSocket == NULL) || !m_webSocket) {

        int port = configValue(uniPiPluginPortParamTypeId).toInt();

        m_webSocket = new QWebSocket();
        connect(m_webSocket, &QWebSocket::connected, this, &DevicePluginUniPi::onWebSocketConnected);
        connect(m_webSocket, &QWebSocket::disconnected, this, &DevicePluginUniPi::onWebSocketDisconnected);

        QUrl url = QUrl("ws://localhost/ws");
        url.setPort(port);
        qCDebug(dcUniPi()) << "Conneting to:" << url.toString();
        m_webSocket->open(url);
    } else {
        requestAllData();
    }
}


void DevicePluginUniPi::deviceRemoved(Device *device)
{
    if(device->deviceClassId() == relayOutputDeviceClassId) {
        m_usedRelais.remove(device->paramValue(relayOutputDeviceNumberParamTypeId).toString());
    } else if(device->deviceClassId() == digitalOutputDeviceClassId) {
        m_usedDigitalOutputs.remove(device->paramValue(digitalOutputDeviceNumberParamTypeId).toString());
    } else if(device->deviceClassId() == digitalInputDeviceClassId) {
        m_usedDigitalInputs.remove(device->paramValue(digitalInputDeviceNumberParamTypeId).toString());
    } else if (device->deviceClassId() == analogOutputDeviceClassId) {
        m_usedAnalogOutputs.remove(device->paramValue(analogOutputDeviceOutputNumberParamTypeId).toString());
    } else if (device->deviceClassId() == analogInputDeviceClassId) {
        m_usedAnalogInputs.remove(device->paramValue(analogInputDeviceInputNumberParamTypeId).toString());

    } else if (device->deviceClassId() == blindDeviceClassId) {
        if (device->paramValue(blindDeviceOutputTypeOpenParamTypeId) == GpioType::Relay) {
            m_usedRelais.remove(device->paramValue(blindDeviceOutputOpenParamTypeId).toString());
        } else if (device->paramValue(blindDeviceOutputOpenParamTypeId) == GpioType::DigitalOutput) {
            m_usedDigitalOutputs.remove(device->paramValue(blindDeviceOutputOpenParamTypeId).toString());
        }

        if (device->paramValue(blindDeviceOutputTypeCloseParamTypeId) == GpioType::Relay) {
            m_usedRelais.remove(device->paramValue(blindDeviceOutputCloseParamTypeId).toString());
        } else if (device->paramValue(blindDeviceOutputOpenParamTypeId) == GpioType::DigitalOutput) {
            m_usedDigitalOutputs.remove(device->paramValue(blindDeviceOutputCloseParamTypeId).toString());
        }

    } else if (device->deviceClassId() == lightDeviceClassId) {
        if (device->paramValue(lightDeviceOutputTypeParamTypeId) == GpioType::Relay) {
            m_usedRelais.remove(device->paramValue(lightDeviceOutputParamTypeId).toString());
        } else if (device->paramValue(lightDeviceOutputParamTypeId) == GpioType::DigitalOutput) {
            m_usedDigitalOutputs.remove(device->paramValue(lightDeviceOutputParamTypeId).toString());
        }
    } else if (device->deviceClassId() == dimmerSwitchDeviceClassId) {
        m_usedDigitalInputs.remove(device->paramValue(dimmerSwitchDeviceInputNumberParamTypeId).toString());
        DimmerSwitch *dimmerSwitch = m_dimmerSwitches.key(device);
        m_dimmerSwitches.remove(dimmerSwitch);
        dimmerSwitch->deleteLater();
    } else if (device->deviceClassId() == temperatureSensorDeviceClassId) {
        m_usedTemperatureSensors.remove(device->paramValue(temperatureSensorDeviceAddressParamTypeId).toString());
    }


    if (myDevices().isEmpty()) {
        m_webSocket->close();
        m_webSocket->deleteLater();
    }
}

DeviceManager::DeviceError DevicePluginUniPi::executeAction(Device *device, const Action &action)
{
    if (m_webSocket->state() != QAbstractSocket::ConnectedState)
        return DeviceManager::DeviceErrorHardwareNotAvailable;

    if (device->deviceClassId() == relayOutputDeviceClassId) {

        if (action.actionTypeId() == relayOutputPowerActionTypeId) {
            QString relayNumber = device->paramValue(relayOutputDeviceNumberParamTypeId).toString();
            int stateValue = action.param(relayOutputPowerActionPowerParamTypeId).value().toInt();
            setOutput(relayNumber, stateValue);

            return DeviceManager::DeviceErrorNoError;
        }
        return DeviceManager::DeviceErrorActionTypeNotFound;
    }

    if (device->deviceClassId() == digitalOutputDeviceClassId) {
        if (action.actionTypeId() == digitalOutputPowerActionTypeId) {
            QString digitalOutputNumber = device->paramValue(digitalOutputDeviceNumberParamTypeId).toString();
            bool stateValue = action.param(digitalOutputPowerActionPowerParamTypeId).value().toBool();
            setOutput(digitalOutputNumber, stateValue);

            return DeviceManager::DeviceErrorNoError;
        }
        return DeviceManager::DeviceErrorActionTypeNotFound;
    }

    if (device->deviceClassId() == analogOutputDeviceClassId) {

        if (action.actionTypeId() == analogOutputOutputValueActionTypeId) {
            QString analogOutputNumber = device->paramValue(analogOutputDeviceOutputNumberParamTypeId).toString();
            double analogValue = action.param(analogOutputOutputValueActionOutputValueParamTypeId).value().toDouble();

            QJsonObject json;
            json["cmd"] = "set";
            json["dev"] = "ao";
            json["circuit"] = analogOutputNumber;
            json["value"] = analogValue;

            QJsonDocument doc(json);
            QByteArray bytes = doc.toJson(QJsonDocument::Compact);
            qCDebug(dcUniPi()) << "Send command" << bytes;
            m_webSocket->sendTextMessage(bytes);
            return DeviceManager::DeviceErrorNoError;
        }
        return DeviceManager::DeviceErrorActionTypeNotFound;
    }

    if (device->deviceClassId() == blindDeviceClassId) {
        QString circuitOpen = device->paramValue(blindDeviceOutputOpenParamTypeId).toString();
        QString circuitClose = device->paramValue(blindDeviceOutputCloseParamTypeId).toString();

        if (action.actionTypeId() == blindCloseActionTypeId) {

            setOutput(circuitOpen, false);
            setOutput(circuitClose, true);
            return DeviceManager::DeviceErrorNoError;
        }
        if (action.actionTypeId() == blindOpenActionTypeId) {

            setOutput(circuitClose, false);
            setOutput(circuitOpen, true);
            return DeviceManager::DeviceErrorNoError;
        }
        if (action.actionTypeId() == blindStopActionTypeId) {
            setOutput(circuitOpen, false);
            setOutput(circuitClose, false);

            return DeviceManager::DeviceErrorNoError;
        }
        return DeviceManager::DeviceErrorActionTypeNotFound;
    }

    if (device->deviceClassId() == lightDeviceClassId) {

        QString circuit = device->paramValue(lightDeviceOutputParamTypeId).toString();
        bool stateValue = action.param(lightPowerActionPowerParamTypeId).value().toBool();

        setOutput(circuit, stateValue);
        return DeviceManager::DeviceErrorNoError;
    }

    return DeviceManager::DeviceErrorDeviceClassNotFound;
}


void DevicePluginUniPi::onWebSocketConnected()
{
    qCDebug(dcUniPi()) << "WebSocket connected";

    connect(m_webSocket, &QWebSocket::textMessageReceived,
            this, &DevicePluginUniPi::onWebSocketTextMessageReceived);

    requestAllData();
}

void DevicePluginUniPi::onWebSocketDisconnected()
{
    qCDebug(dcUniPi())  << "WebSocket disconnected";

}

void DevicePluginUniPi::requestAllData()
{
    QJsonObject json;
    json["cmd"] = "all";

    QJsonDocument doc(json);
    QByteArray bytes = doc.toJson();
    m_webSocket->sendTextMessage(bytes);
}

void DevicePluginUniPi::onWebSocketTextMessageReceived(const QString &message)
{
    QJsonArray array;
    QJsonParseError error;
    QJsonDocument doc = QJsonDocument::fromJson(message.toUtf8(), &error);


    if(error.error != QJsonParseError::NoError) {
        qCWarning(dcUniPi) << "failed to parse data" << message << ":" << error.errorString();
        return;
    }

    // check validity of the document
    if(!doc.isNull()) {
        if(doc.isObject()) {
            array.append(doc.object());
        } else if (doc.isArray()){
            array = doc.array();;
        }else {
            qCDebug(dcUniPi()) << "Document is not an object nor an array";
        }
    } else {
        qCDebug(dcUniPi()) << "Invalid JSON";
        return;
    }

    for (int levelIndex = 0; levelIndex < array.size(); ++levelIndex) {
        QJsonObject obj;
        obj = array[levelIndex].toObject();

        if (obj["dev"] == "relay") {
            qCDebug(dcUniPi()) << "Relay:" << "Circuit:" <<  obj["circuit"].toString() << "Value:" << obj["value"].toInt() << "Relay Type:" << obj["relay_type"].toString() ;

            QString circuit = obj["circuit"].toString();
            bool value = obj["value"].toBool();

            if ((obj["relay_type"].toString() == "physical") || (obj["relay_type"].toString() == "")) {

                if (!m_relais.contains(circuit)) {
                    //New Device detected
                    m_relais.append(circuit);
                } else {
                    if (m_usedRelais.contains(circuit)) {
                        Device *device = m_usedRelais.value(circuit);
                        if (device->deviceClassId() == relayOutputDeviceClassId) {
                            device->setStateValue(relayOutputPowerStateTypeId, value);
                        } else if (device->deviceClassId() == blindDeviceClassId) {
                            if (circuit == device->paramValue(blindDeviceOutputOpenParamTypeId).toString()) {
                                if (value) {
                                    if (device->stateValue(blindStatusStateTypeId).toString().contains("stopped")) {
                                        device->setStateValue(blindStatusStateTypeId, "opening");
                                    } else if (device->stateValue(blindStatusStateTypeId).toString().contains("closing")) {
                                        device->setStateValue(blindStatusStateTypeId, "opening");
                                    } else if (device->stateValue(blindStatusStateTypeId).toString().contains("opening")) {
                                        //state unchanged
                                    }
                                } else {
                                    if (device->stateValue(blindStatusStateTypeId).toString().contains("stopped")) {
                                        // state unchanged
                                    } else if (device->stateValue(blindStatusStateTypeId).toString().contains("closing")) {
                                        // state unchanged
                                    } else if (device->stateValue(blindStatusStateTypeId).toString().contains("opening")) {
                                        device->setStateValue(blindStatusStateTypeId, "stopped");
                                    }
                                }
                            }
                            if (circuit == device->paramValue(blindDeviceOutputCloseParamTypeId).toString()) {
                                if (value) {
                                    if (device->stateValue(blindStatusStateTypeId).toString().contains("stopped")) {
                                        device->setStateValue(blindStatusStateTypeId, "closing");
                                    } else if (device->stateValue(blindStatusStateTypeId).toString().contains("closing")) {
                                        //state unchanged
                                    } else if (device->stateValue(blindStatusStateTypeId).toString().contains("opening")) {
                                        device->setStateValue(blindStatusStateTypeId, "closing");
                                    }
                                } else {
                                    if (device->stateValue(blindStatusStateTypeId).toString().contains("stopped")) {
                                        // state unchanged
                                    } else if (device->stateValue(blindStatusStateTypeId).toString().contains("closing")) {
                                        device->setStateValue(blindStatusStateTypeId, "stopped");
                                    } else if (device->stateValue(blindStatusStateTypeId).toString().contains("opening")) {
                                        // state unchanged
                                    }
                                }
                            }

                        } else if (device->deviceClassId() == lightDeviceClassId) {
                            device->setStateValue(lightPowerStateTypeId, value);
                        }
                    }
                }
            } else if (obj["relay_type"].toString() == "digital") {
                if (!m_digitalOutputs.contains(obj["circuit"].toString())){
                    //New Device detected
                    m_digitalOutputs.append(obj["circuit"].toString());
                } else {
                    if (m_usedDigitalOutputs.contains(obj["circuit"].toString())) {
                        Device *device = m_usedDigitalOutputs.value(obj["circuit"].toString());
                        if (device->deviceClassId() == digitalOutputDeviceClassId) {
                            device->setStateValue(digitalOutputPowerStateTypeId, obj["value"].toBool());
                        } else if (device->deviceClassId() == blindDeviceClassId) {
                            if (circuit == device->paramValue(blindDeviceOutputOpenParamTypeId).toString()) {
                                if (value && device->stateValue(blindStatusStateTypeId).toString().contains("stopped")) {
                                    device->setStateValue(blindStatusStateTypeId, "opening");
                                } else if (!value && device->stateValue(blindStatusStateTypeId).toString().contains("opening")) {
                                    device->setStateValue(blindStatusStateTypeId, "stopped");
                                } else {
                                    qCWarning(dcUniPi()) << "blind" << device << "Output open:" << value << "Status: " << device->stateValue(blindStatusStateTypeId).toString();
                                    device->setStateValue(blindStatusStateTypeId, "stopped");
                                }
                            }
                            if (circuit == device->paramValue(blindDeviceOutputCloseParamTypeId).toString()) {
                                if (value && device->stateValue(blindStatusStateTypeId).toString().contains("stopped")) {
                                    device->setStateValue(blindStatusStateTypeId, "closing");
                                } else if (!value && device->stateValue(blindStatusStateTypeId).toString().contains("closing")) {
                                    device->setStateValue(blindStatusStateTypeId, "stopped");
                                } else {
                                    qCWarning(dcUniPi()) << "blind" << device << "Output close:" << value << "Status: " << device->stateValue(blindStatusStateTypeId).toString();
                                    device->setStateValue(blindStatusStateTypeId, "stopped");
                                }
                            }
                        } else if (device->deviceClassId() == lightDeviceClassId) {
                            device->setStateValue(lightPowerStateTypeId, obj["value"].toBool());
                        }
                    }
                }
            }
        }

        if (obj["dev"] == "input") {
            qCDebug(dcUniPi()) << "Input:" << obj["dev"].toString() << "Circuit:" <<  obj["circuit"].toString() << "Value:" << obj["value"].toInt();

            if (!m_digitalInputs.contains(obj["circuit"].toString())){
                //New Device detected
                m_digitalInputs.append(obj["circuit"].toString());
            } else {
                if (m_usedDigitalInputs.contains(obj["circuit"].toString())) {
                    bool value = obj["value"].toBool();
                    Device *device = m_usedDigitalInputs.value(obj["circuit"].toString());
                    if (device->deviceClassId() == digitalInputDeviceClassId) {
                        device->setStateValue(digitalInputInputStatusStateTypeId, value);
                    } else if (device->deviceClassId() == dimmerSwitchDeviceClassId) {
                        device->setStateValue(dimmerSwitchStatusStateTypeId, value);
                        DimmerSwitch *dimmerSwitch = m_dimmerSwitches.key(device);
                        dimmerSwitch->setPower(value);
                    }
                }
            }
        }

        if (obj["dev"] == "ao") {
            qCDebug(dcUniPi()) << "Analog Output:" << obj["dev"] << "Circuit:" <<  obj["circuit"].toString() << "Value:" << obj["value"].toDouble();

            if (!m_analogOutputs.contains(obj["circuit"].toString())){
                //New Device detected
                m_analogOutputs.append(obj["circuit"].toString());
            } else {
                if (m_usedAnalogOutputs.contains(obj["circuit"].toString())) {
                    double value = obj["value"].toDouble();
                    Device *device = m_usedAnalogOutputs.value(obj["circuit"].toString());

                    if (device->deviceClassId() == analogOutputDeviceClassId) {
                        device->setStateValue(analogOutputOutputValueStateTypeId, value);
                    }
                }
            }
        }

        if (obj["dev"] == "ai") {
            qCDebug(dcUniPi()) << "Analog Input:" << obj["dev"] << "Circuit:" <<  obj["circuit"].toString() << "Value:" << obj["value"].toDouble();

            if (!m_analogInputs.contains(obj["circuit"].toString())){
                //New analog output detected
                m_analogInputs.append(obj["circuit"].toString());
            } else {
                if (m_usedAnalogInputs.contains(obj["circuit"].toString())) {
                    double value = obj["value"].toDouble();
                    Device *device = m_usedAnalogInputs.value(obj["circuit"].toString());

                    if (device->deviceClassId() == analogInputDeviceClassId) {
                        device->setStateValue(analogInputInputValueStateTypeId, value);
                    }
                }
            }
        }

        if (obj["dev"] == "temp") {
            qCDebug(dcUniPi()) << "Temperature Sensor:" << obj["typ"].toString() << "Address:" <<  obj["circuit"].toString() << "Value:" << obj["value"].toDouble() << "Connected:" << !(QVariant(obj["lost"]).toBool());
            if (!m_temperatureSensors.contains(obj["circuit"].toString())){
                //New temperature sensor detected
                m_temperatureSensors.append(obj["circuit"].toString());
            } else {
                //Updating states of already added temperature sensor
                if (m_usedTemperatureSensors.contains(obj["circuit"].toString())) {
                    double value = obj["value"].toDouble();
                    bool connected = !(obj["lost"]).toBool();
                    Device *device = m_usedTemperatureSensors.value(obj["circuit"].toString());

                    if (device->deviceClassId() == temperatureSensorDeviceClassId) {
                        device->setStateValue(temperatureSensorTemperatureStateTypeId, value);
                        device->setStateValue(temperatureSensorConnectedStateTypeId, connected);
                    }
                }
            }
        }
    }
}

void DevicePluginUniPi::onRefreshTimer()
{
    connectToEvok();
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
