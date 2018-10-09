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
    connectToEvok();

    m_refreshTimer = hardwareManager()->pluginTimerManager()->registerTimer(60);
    connect(m_refreshTimer, &PluginTimer::timeout, this, &DevicePluginUniPi::onRefreshTimer);

}

DeviceManager::DeviceSetupStatus DevicePluginUniPi::setupDevice(Device *device)
{
    if (device->deviceClassId() == relayOutputDeviceClassId) {

        m_usedRelais.insert(device->paramValue(relayOutputRelayNumberParamTypeId).toString(), device);
        return DeviceManager::DeviceSetupStatusSuccess;
    }

    if (device->deviceClassId() == digitalOutputDeviceClassId) {

        m_usedDigitalOutputs.insert(device->paramValue(digitalOutputDigitalOutputNumberParamTypeId).toString(), device);
        return DeviceManager::DeviceSetupStatusSuccess;
    }

    if (device->deviceClassId() == digitalInputDeviceClassId) {

        m_usedDigitalInputs.insert(device->paramValue(digitalInputDigitalInputNumberParamTypeId).toString(), device);
        return DeviceManager::DeviceSetupStatusSuccess;
    }

    if (device->deviceClassId() == analogInputDeviceClassId) {

        m_usedAnalogInputs.insert(device->paramValue(analogInputAnalogInputNumberParamTypeId).toString(), device);
        return DeviceManager::DeviceSetupStatusSuccess;
    }

    if (device->deviceClassId() == analogOutputDeviceClassId) {

        m_usedAnalogOutputs.insert(device->paramValue(analogOutputAnalogOutputNumberParamTypeId).toString(), device);
        return DeviceManager::DeviceSetupStatusSuccess;
    }

    if (device->deviceClassId() == blindDeviceClassId) {

        if (device->paramValue(blindOutputTypeOpenParamTypeId) == GpioType::Relay) {
            m_usedRelais.insert(device->paramValue(blindOutputOpenParamTypeId).toString(), device);
        } else if (device->paramValue(blindOutputTypeOpenParamTypeId) == GpioType::DigitalOutput) {
            m_usedDigitalOutputs.insert(device->paramValue(blindOutputOpenParamTypeId).toString(), device);
        }

        if (device->paramValue(blindOutputTypeCloseParamTypeId) == GpioType::Relay) {
            m_usedRelais.insert(device->paramValue(blindOutputCloseParamTypeId).toString(), device);
        } else if (device->paramValue(blindOutputTypeOpenParamTypeId) == GpioType::DigitalOutput) {
            m_usedDigitalOutputs.insert(device->paramValue(blindOutputCloseParamTypeId).toString(), device);
        }

        return DeviceManager::DeviceSetupStatusSuccess;
    }

    if (device->deviceClassId() == lightDeviceClassId) {

        if (device->paramValue(lightOutputTypeParamTypeId) == GpioType::Relay) {
            m_usedRelais.insert(device->paramValue(lightOutputParamTypeId).toString(), device);
        } else if (device->paramValue(lightOutputParamTypeId) == GpioType::DigitalOutput) {
            m_usedDigitalOutputs.insert(device->paramValue(lightOutputParamTypeId).toString(), device);
        }
        return DeviceManager::DeviceSetupStatusSuccess;
    }

    if (device->deviceClassId() == dimmerSwitchDeviceClassId) {
        m_usedDigitalInputs.insert(device->paramValue(dimmerSwitchInputNumberParamTypeId).toString(), device);
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
                parameters.append(Param(relayOutputRelayNumberParamTypeId, circuit));

                descriptor.setParams(parameters);
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
                parameters.append(Param(digitalOutputDigitalOutputNumberParamTypeId, circuit));
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
                parameters.append(Param(digitalInputDigitalInputNumberParamTypeId, circuit));
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
                parameters.append(Param(analogInputAnalogInputNumberParamTypeId, circuit));
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
                parameters.append(Param(analogOutputAnalogOutputNumberParamTypeId, circuit));
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
                        parameters.append(Param(blindOutputOpenParamTypeId, openingCircuit));
                        parameters.append(Param(blindOutputCloseParamTypeId, closingCircuit));
                        parameters.append(Param(blindOutputTypeOpenParamTypeId, GpioType::Relay));
                        parameters.append(Param(blindOutputTypeCloseParamTypeId, GpioType::Relay));
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
                        parameters.append(Param(blindOutputOpenParamTypeId, openingCircuit));
                        parameters.append(Param(blindOutputCloseParamTypeId, closingCircuit));
                        parameters.append(Param(blindOutputTypeOpenParamTypeId, GpioType::DigitalOutput));
                        parameters.append(Param(blindOutputTypeCloseParamTypeId, GpioType::DigitalOutput));
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
                parameters.append(Param(lightOutputParamTypeId, circuit));
                parameters.append(Param(lightOutputTypeParamTypeId, GpioType::Relay));
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
                parameters.append(Param(lightOutputParamTypeId, circuit));
                parameters.append(Param(lightOutputTypeParamTypeId, GpioType::DigitalOutput));
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
                parameters.append(Param(dimmerSwitchInputNumberParamTypeId, circuit));
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
    if (m_webSocket == NULL) {

        int port = 8080;
        //configValue(uniPiPortParamTypeId).toInt(); //FIXME plugin configuration loading currently not possible in init

        m_webSocket = new QWebSocket();
        connect(m_webSocket, &QWebSocket::connected, this, &DevicePluginUniPi::onWebSocketConnected);
        connect(m_webSocket, &QWebSocket::disconnected, this, &DevicePluginUniPi::onWebSocketDisconnected);

        QUrl url = QUrl("ws://localhost/ws");
        url.setPort(port);
        qCDebug(dcUniPi()) << "Conneting to:" << url.toString();
        m_webSocket->open(url);
    }
}


void DevicePluginUniPi::deviceRemoved(Device *device)
{
    if(device->deviceClassId() == relayOutputDeviceClassId) {
        m_usedRelais.remove(device->paramValue(relayOutputRelayNumberParamTypeId).toString());
    } else if(device->deviceClassId() == digitalOutputDeviceClassId) {
        m_usedDigitalOutputs.remove(device->paramValue(digitalOutputDigitalOutputNumberParamTypeId).toString());
    } else if(device->deviceClassId() == digitalInputDeviceClassId) {
        m_usedDigitalInputs.remove(device->paramValue(digitalInputDigitalInputNumberParamTypeId).toString());
    } else if (device->deviceClassId() == analogOutputDeviceClassId) {
        m_usedAnalogOutputs.remove(device->paramValue(analogOutputAnalogOutputNumberParamTypeId).toString());
    } else if (device->deviceClassId() == analogInputDeviceClassId) {
        m_usedAnalogInputs.remove(device->paramValue(analogInputAnalogInputNumberParamTypeId).toString());

    } else if (device->deviceClassId() == blindDeviceClassId) {
        if (device->paramValue(blindOutputTypeOpenParamTypeId) == GpioType::Relay) {
            m_usedRelais.remove(device->paramValue(blindOutputOpenParamTypeId).toString());
        } else if (device->paramValue(blindOutputOpenParamTypeId) == GpioType::DigitalOutput) {
            m_usedDigitalOutputs.remove(device->paramValue(blindOutputOpenParamTypeId).toString());
        }

        if (device->paramValue(blindOutputTypeCloseParamTypeId) == GpioType::Relay) {
            m_usedRelais.remove(device->paramValue(blindOutputCloseParamTypeId).toString());
        } else if (device->paramValue(blindOutputOpenParamTypeId) == GpioType::DigitalOutput) {
            m_usedDigitalOutputs.remove(device->paramValue(blindOutputCloseParamTypeId).toString());
        }

    } else if (device->deviceClassId() == lightDeviceClassId) {
        if (device->paramValue(lightOutputTypeParamTypeId) == GpioType::Relay) {
            m_usedRelais.remove(device->paramValue(lightOutputParamTypeId).toString());
        } else if (device->paramValue(lightOutputParamTypeId) == GpioType::DigitalOutput) {
            m_usedDigitalOutputs.remove(device->paramValue(lightOutputParamTypeId).toString());
        }
    } else if (device->deviceClassId() == dimmerSwitchDeviceClassId) {
        m_usedDigitalInputs.remove(device->paramValue(dimmerSwitchInputNumberParamTypeId).toString());
        DimmerSwitch *dimmerSwitch = m_dimmerSwitches.key(device);
        m_dimmerSwitches.remove(dimmerSwitch);
        dimmerSwitch->deleteLater();
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
            QString relayNumber = device->paramValue(relayOutputRelayNumberParamTypeId).toString();
            int stateValue = action.param(relayOutputPowerActionParamTypeId).value().toInt();
            setOutput(relayNumber, stateValue);

            return DeviceManager::DeviceErrorNoError;
        }
        return DeviceManager::DeviceErrorActionTypeNotFound;
    }

    if (device->deviceClassId() == digitalOutputDeviceClassId) {
        if (action.actionTypeId() == digitalOutputPowerActionTypeId) {
            QString digitalOutputNumber = device->paramValue(digitalOutputDigitalOutputNumberParamTypeId).toString();
            bool stateValue = action.param(digitalOutputPowerActionParamTypeId).value().toBool();
            setOutput(digitalOutputNumber, stateValue);

            return DeviceManager::DeviceErrorNoError;
        }
        return DeviceManager::DeviceErrorActionTypeNotFound;
    }

    if (device->deviceClassId() == analogOutputDeviceClassId) {

        if (action.actionTypeId() == analogOutputAnalogOutputValueActionTypeId) {
            QString analogOutputNumber = device->paramValue(analogOutputAnalogOutputNumberParamTypeId).toString();
            double analogValue = action.param(analogOutputAnalogOutputValueActionParamTypeId).value().toDouble();

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
        QString circuitOpen = device->paramValue(blindOutputOpenParamTypeId).toString();
        QString circuitClose = device->paramValue(blindOutputCloseParamTypeId).toString();

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

        QString circuit = device->paramValue(lightOutputParamTypeId).toString();
        bool stateValue = action.param(lightPowerActionParamTypeId).value().toBool();

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

    QJsonObject json;
    json["cmd"] = "all";

    QJsonDocument doc(json);
    QByteArray bytes = doc.toJson();
    m_webSocket->sendTextMessage(bytes);
}

void DevicePluginUniPi::onWebSocketDisconnected()
{
    qCDebug(dcUniPi())  << "WebSocket disconnected";

}


void DevicePluginUniPi::onWebSocketTextMessageReceived(QString message)
{
    QJsonArray array;
    QJsonObject obj;
    QJsonDocument doc = QJsonDocument::fromJson(message.toUtf8());

    // check validity of the document
    if(!doc.isNull()) {
        if(doc.isObject()) {
            obj = doc.object();
        } else if (doc.isArray()){
            array = doc.array();
        }else {
            qCDebug(dcUniPi()) << "Document is not an object nor an array";
        }
    } else {
        qCDebug(dcUniPi()) << "Invalid JSON";
    }

    for (int levelIndex = 0; levelIndex < array.size(); ++levelIndex) {
        obj = array[levelIndex].toObject();

        if (obj["cmd"] == "all") {
            //read model number
            qCDebug(dcUniPi()) << message;
        }

        if (obj["dev"] == "relay") {
            qCDebug(dcUniPi()) << "Relay:" << "Circuit:" <<  obj["circuit"].toString() << "Value:" << obj["value"].toInt() << "Relay Type:" << obj["relay_type"].toString() ;

            QString circuit = obj["circuit"].toString();
            bool value = QVariant(obj["value"].toInt()).toBool();

            if (obj["relay_type"].toString() == "physical") {

                if (!m_relais.contains(circuit)) {
                    //New Device detected
                    m_relais.append(circuit);
                } else {
                    if (m_usedRelais.contains(circuit)) {
                        Device *device = m_usedRelais.value(circuit);
                        if (device->deviceClassId() == relayOutputDeviceClassId) {
                            device->setStateValue(relayOutputPowerStateTypeId, value);
                        } else if (device->deviceClassId() == blindDeviceClassId) {
                            if (circuit == device->paramValue(blindOutputOpenParamTypeId).toString()) {
                                if (value && device->stateValue(blindStatusStateTypeId).toString().contains("stopped")) {
                                    device->setStateValue(blindStatusStateTypeId, "opening");
                                } else if (!value && device->stateValue(blindStatusStateTypeId).toString().contains("opening")) {
                                    device->setStateValue(blindStatusStateTypeId, "stopped");
                                } else {
                                    qWarning(dcUniPi()) << "Blind" << device << "Output open:" << value << "Status: " << device->stateValue(blindStatusStateTypeId).toString();
                                }
                            }
                            if (circuit == device->paramValue(blindOutputCloseParamTypeId).toString()) {
                                if (value && device->stateValue(blindStatusStateTypeId).toString().contains("stopped")) {
                                    device->setStateValue(blindStatusStateTypeId, "closing");
                                } else if (!value && device->stateValue(blindStatusStateTypeId).toString().contains("closing")) {
                                    device->setStateValue(blindStatusStateTypeId, "stopped");
                                } else {
                                    qWarning(dcUniPi()) << "Blind" << device << "Output close:" << value << "Status: " << device->stateValue(blindStatusStateTypeId).toString();
                                }
                                break;
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
                            device->setStateValue(digitalOutputPowerStateTypeId, QVariant(obj["value"].toInt()).toBool());
                        } else if (device->deviceClassId() == blindDeviceClassId) {
                            if (circuit == device->paramValue(blindOutputOpenParamTypeId).toString()) {
                                if (value && device->stateValue(blindStatusStateTypeId).toString().contains("stop")) {
                                    device->setStateValue(blindStatusStateTypeId, "open");
                                } else if (!value && device->stateValue(blindStatusStateTypeId).toString().contains("open")) {
                                    device->setStateValue(blindStatusStateTypeId, "stop");
                                } else {
                                    qWarning(dcUniPi()) << "blind" << device << "Output open:" << value << "Status: " << device->stateValue(blindStatusStateTypeId).toString();
                                }
                            }
                            if (circuit == device->paramValue(blindOutputCloseParamTypeId).toString()) {
                                if (value && device->stateValue(blindStatusStateTypeId).toString().contains("stop")) {
                                    device->setStateValue(blindStatusStateTypeId, "close");
                                } else if (!value && device->stateValue(blindStatusStateTypeId).toString().contains("close")) {
                                    device->setStateValue(blindStatusStateTypeId, "stop");
                                } else {
                                    qWarning(dcUniPi()) << "blind" << device << "Output close:" << value << "Status: " << device->stateValue(blindStatusStateTypeId).toString();
                                }
                                break;
                            }
                        } else if (device->deviceClassId() == lightDeviceClassId) {
                            device->setStateValue(lightPowerStateTypeId, QVariant(obj["value"].toInt()).toBool());
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
                    bool value = QVariant(obj["value"].toInt()).toBool();
                    Device *device = m_usedDigitalInputs.value(obj["circuit"].toString());
                    if (device->deviceClassId() == digitalInputDeviceClassId) {
                        device->setStateValue(digitalInputDigitalInputStatusStateTypeId, value);
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
                foreach (Device *device, myDevices()) {
                    if (device->deviceClassId() == analogOutputDeviceClassId) {
                        if (obj["circuit"] == device->paramValue(analogOutputAnalogOutputNumberParamTypeId).toString()) {
                            device->setStateValue(analogOutputAnalogOutputValueStateTypeId, obj["value"].toDouble());
                            break;
                        }
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
                foreach (Device *device, myDevices()) {
                    if (device->deviceClassId() == analogInputDeviceClassId) {

                        if (obj["circuit"] == device->paramValue(analogInputAnalogInputNumberParamTypeId).toString()) {
                            device->setStateValue(analogInputAnalogInputValueStateTypeId, obj["value"].toDouble());
                            break;
                        }
                    }
                }
            }
        }

        if (obj["dev"] == "led") { //TODO can't discover leds without toggling it from another client
            qCDebug(dcUniPi()) << "Led:" << obj["dev"] << "Circuit:" <<  obj["circuit"].toString() << "Value:" << obj["value"].toInt();

            if (!m_leds.contains(obj["circuit"].toString())){
                //New led detected
                m_leds.append(obj["circuit"].toString());
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
