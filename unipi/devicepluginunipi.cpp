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

}

void DevicePluginUniPi::init()
{

}

DeviceManager::DeviceSetupStatus DevicePluginUniPi::setupDevice(Device *device)
{
    if (m_webSocket == NULL) {

        int port = device->paramValue(uniPiPortParamTypeId).toInt();

        m_webSocket = new QWebSocket();
        connect(m_webSocket, &QWebSocket::connected, this, &DevicePluginUniPi::onWebSocketConnected);
        connect(m_webSocket, &QWebSocket::disconnected, this, &DevicePluginUniPi::onWebSocketDisconnected);

        QUrl url = QUrl("ws://localhost/ws");
        url.setPort(port);
        qCDebug(dcUniPi()) << "Conneting to:" << url.toString();
        m_webSocket->open(url);

        return DeviceManager::DeviceSetupStatusSuccess;
    }

    if (device->deviceClassId() == relayOutputDeviceClassId) {

        m_usedGpios.insert(device->paramValue(relayOutputRelayNumberParamTypeId).toString(), device);
        return DeviceManager::DeviceSetupStatusSuccess;
    }

    if (device->deviceClassId() == digitalOutputDeviceClassId) {

        m_usedGpios.insert(device->paramValue(digitalOutputDigitalOutputNumberParamTypeId).toString(), device);
        return DeviceManager::DeviceSetupStatusSuccess;
    }

    if (device->deviceClassId() == digitalInputDeviceClassId) {

        m_usedGpios.insert(device->paramValue(digitalInputDigitalInputNumberParamTypeId).toString(), device);
        return DeviceManager::DeviceSetupStatusSuccess;
    }

    if (device->deviceClassId() == analogInputDeviceClassId) {

        m_usedGpios.insert(device->paramValue(analogInputAnalogInputNumberParamTypeId).toString(), device);
        return DeviceManager::DeviceSetupStatusSuccess;
    }

    if (device->deviceClassId() == analogOutputDeviceClassId) {

        m_usedGpios.insert(device->paramValue(analogOutputAnalogOutputNumberParamTypeId).toString(), device);
        return DeviceManager::DeviceSetupStatusSuccess;
    }

    if (device->deviceClassId() == shadingDeviceClassId) {

        m_usedGpios.insert(device->paramValue(shadingOutputUpParamTypeId).toString(), device);
        m_usedGpios.insert(device->paramValue(shadingOutputDownParamTypeId).toString(), device);
        return DeviceManager::DeviceSetupStatusSuccess;
    }

    if (device->deviceClassId() == lightDeviceClassId) {

        m_usedGpios.insert(device->paramValue(relayOutputRelayNumberParamTypeId).toString(), device);
        return DeviceManager::DeviceSetupStatusSuccess;
    }

    return DeviceManager::DeviceSetupStatusFailure;
}

DeviceManager::DeviceError DevicePluginUniPi::discoverDevices(const DeviceClassId &deviceClassId, const ParamList &params)
{
    Q_UNUSED(params);

    const DeviceClass deviceClass = deviceManager()->findDeviceClass(deviceClassId);
    if (deviceClass.vendorId() == unipiVendorId) {

        if (deviceClassId == relayOutputDeviceClassId) {

            // Create the list of available gpios
            QList<DeviceDescriptor> deviceDescriptors;

            for (int i = 0; i < m_relais.count(); i++) {
                const QString circuit = m_relais.at(i);

                // Offer only gpios which arn't in use already
                if (m_usedGpios.contains(circuit)){
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
            return DeviceManager::DeviceErrorAsync;
        }

        if (deviceClassId == digitalInputDeviceClassId) {
            return DeviceManager::DeviceErrorAsync;
        }

        if (deviceClassId == analogInputDeviceClassId) {
            return DeviceManager::DeviceErrorAsync;
        }

        if (deviceClassId == analogOutputDeviceClassId) {
            return DeviceManager::DeviceErrorAsync;
        }

        if (deviceClassId == shadingDeviceClassId) {
            // Create the list of available gpios
            QList<DeviceDescriptor> deviceDescriptors;

            for (int i = 0; i < m_relais.count(); i++) {
                const QString circuit = m_relais.at(i);

                // Offer only gpios which aren't in use already
                if (m_usedGpios.contains(circuit)){
                    continue;
                }

                DeviceDescriptor descriptor(deviceClassId, QString("Up Relay %1 + Down Relay %2").arg(circuit, m_relais.at(i+1)), circuit);
                ParamList parameters;
                parameters.append(Param(shadingOutputUpParamTypeId, circuit));
                parameters.append(Param(shadingOutputDownParamTypeId, m_relais.at(i+1)));
                parameters.append(Param(shadingOutputTypeUpParamTypeId, GPIOType::relay));
                parameters.append(Param(shadingOutputTypeDownParamTypeId, GPIOType::relay));
                descriptor.setParams(parameters);
                deviceDescriptors.append(descriptor);
            }
            emit devicesDiscovered(deviceClassId, deviceDescriptors);
            return DeviceManager::DeviceErrorAsync;
        }

        if (deviceClassId == lightDeviceClassId) {
            return DeviceManager::DeviceErrorAsync;
        }
    }
    return DeviceManager::DeviceErrorDeviceClassNotFound;
}

void DevicePluginUniPi::setOutput(const QString &circuit, const QString &type, bool value)
{
    QJsonObject json;
    json["cmd"] = "set";
    json["dev"] = type;
    json["circuit"] = circuit;
    json["value"] = value;

    QJsonDocument doc(json);
    QByteArray bytes = doc.toJson(QJsonDocument::Compact);
    qCDebug(dcUniPi()) << "Send command" << bytes;
    m_webSocket->sendBinaryMessage(bytes);
}

void DevicePluginUniPi::postSetupDevice(Device *device)
{
    Q_UNUSED(device);
}

void DevicePluginUniPi::deviceRemoved(Device *device)
{
    m_usedGpios.remove(m_usedGpios.key(device)); //TODO remove multible IOs associated to a single device

    if (myDevices().isEmpty()) {
        m_webSocket->close();
        m_webSocket->deleteLater();
    }
}

DeviceManager::DeviceError DevicePluginUniPi::executeAction(Device *device, const Action &action)
{

    if (device->deviceClassId() == relayOutputDeviceClassId) {

        if (action.actionTypeId() == relayOutputRelayStatusActionTypeId) {
            QString relayNumber = device->paramValue(relayOutputRelayNumberParamTypeId).toString();
            int stateValue = action.param(relayOutputRelayStatusActionParamTypeId).value().toInt();

            QJsonObject json;
            json["cmd"] = "set";
            json["dev"] = "relay";
            json["circuit"] = relayNumber;
            json["value"] = stateValue;

            QJsonDocument doc(json);
            QByteArray bytes = doc.toJson(QJsonDocument::Compact);
            qCDebug(dcUniPi()) << "Send command" << bytes;
            m_webSocket->sendBinaryMessage(bytes);
            return DeviceManager::DeviceErrorNoError;
        }
        return DeviceManager::DeviceErrorActionTypeNotFound;
    }


    if (device->deviceClassId() == digitalOutputDeviceClassId) {
        if (action.actionTypeId() == digitalOutputDigitalOutputStatusActionTypeId) {
            int digitalOutputNumber = device->paramValue(digitalOutputDigitalOutputNumberParamTypeId).toInt();
            int stateValue = action.param(digitalOutputDigitalOutputStatusActionParamTypeId).value().toInt();

            QJsonObject json;
            json["cmd"] = "set";
            json["dev"] = "do";
            json["circuit"] = digitalOutputNumber;
            json["value"] = stateValue;

            QJsonDocument doc(json);
            QByteArray bytes = doc.toJson();
            m_webSocket->sendTextMessage(bytes);
            return DeviceManager::DeviceErrorNoError;
        }
        return DeviceManager::DeviceErrorActionTypeNotFound;
    }

    if (device->deviceClassId() == analogOutputDeviceClassId) {

        if (action.actionTypeId() == analogOutputAnalogOutputValueActionTypeId) {
            int analogOutputNumber = device->paramValue(analogOutputAnalogOutputNumberParamTypeId).toInt();
            double analogValue = action.param(analogOutputAnalogOutputValueActionParamTypeId).value().toDouble();

            QJsonObject json;
            json["cmd"] = "set";
            json["dev"] = "ao";
            json["circuit"] = analogOutputNumber;
            json["value"] = analogValue;

            QJsonDocument doc(json);
            QByteArray bytes = doc.toJson();
            m_webSocket->sendTextMessage(bytes);
            return DeviceManager::DeviceErrorNoError;
        }
        return DeviceManager::DeviceErrorActionTypeNotFound;
    }

    if (device->deviceClassId() == shadingDeviceClassId) {

        if (action.actionTypeId() == shadingDownActionTypeId) {
            QString circuit = device->paramValue(shadingOutputDownParamTypeId).toString();
            QString type = device->paramValue(shadingOutputTypeDownParamTypeId).toString();
            setOutput(circuit, type, false);
            return DeviceManager::DeviceErrorNoError;
        }
        if (action.actionTypeId() == shadingUpActionTypeId) {
            QString circuit = device->paramValue(shadingOutputDownParamTypeId).toString();
            QString type = device->paramValue(shadingOutputTypeDownParamTypeId).toString();
            setOutput(circuit, type, false);
            return DeviceManager::DeviceErrorNoError;
        }

        if (action.actionTypeId() == shadingStopActionTypeId) {
            QString circuitUp = device->paramValue(shadingOutputDownParamTypeId).toString();
            QString typeUp = device->paramValue(shadingOutputTypeDownParamTypeId).toString();
            setOutput(circuitUp, typeUp, false);

            QString circuitDown = device->paramValue(shadingOutputDownParamTypeId).toString();
            QString typeDown = device->paramValue(shadingOutputTypeDownParamTypeId).toString();
            setOutput(circuitDown, typeDown, false);

            return DeviceManager::DeviceErrorNoError;
        }
        return DeviceManager::DeviceErrorActionTypeNotFound;
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

    //send signal device Setup was successfull
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
            qCDebug(dcUniPi()) << "Relay:" << obj["dev"].toString() << "Circuit:" <<  obj["circuit"].toString() << "Value:" << obj["value"].toInt();

            QString circuit = obj["circuit"].toString();
            bool value = QVariant(obj["value"].toInt()).toBool();

            if (!m_relais.contains(circuit)) {
                //New Device detected
                m_relais.append(circuit);
            } else {

                foreach (Device *device, myDevices()) {
                    if (device->deviceClassId() == relayOutputDeviceClassId) {
                        if (circuit == device->paramValue(relayOutputRelayNumberParamTypeId).toString()) {
                            device->setStateValue(relayOutputRelayStatusStateTypeId, value);
                            break;
                        }
                    } else if (device->deviceClassId() == shadingDeviceClassId) {
                        if (circuit == device->paramValue(shadingOutputUpParamTypeId).toString()) {
                            if (value && device->stateValue(shadingStatusStateTypeId).toString().contains("stop")) {
                                device->setStateValue(shadingStatusStateTypeId, "up");
                            } else if (!value && device->stateValue(shadingStatusStateTypeId).toString().contains("up")) {
                                device->setStateValue(shadingStatusStateTypeId, "stop");
                            } else {
                                qWarning(dcUniPi()) << "Shading" << device << "Output Up:" << value << "Status: " << device->stateValue(shadingStatusStateTypeId).toString();
                            }

                            break;
                        }
                        if (circuit == device->paramValue(shadingOutputDownParamTypeId).toString()) {
                            if (value && device->stateValue(shadingStatusStateTypeId).toString().contains("stop")) {
                                device->setStateValue(shadingStatusStateTypeId, "down");
                            } else if (!value && device->stateValue(shadingStatusStateTypeId).toString().contains("down")) {
                                device->setStateValue(shadingStatusStateTypeId, "stop");
                            } else {
                                qWarning(dcUniPi()) << "Shading" << device << "Output Down:" << value << "Status: " << device->stateValue(shadingStatusStateTypeId).toString();
                            }
                            break;
                        }

                    } else if (device->deviceClassId() == lightDeviceClassId) {
                        if (circuit == device->paramValue(lightOutputParamTypeId).toString()) {
                            device->setStateValue(lightStatusStateTypeId, value);
                            break;
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

                foreach (Device *device, myDevices()) {
                    if (device->deviceClassId() == digitalInputDeviceClassId) {
                        if (obj["circuit"] == device->paramValue(digitalInputDigitalInputNumberParamTypeId).toString()) {
                            device->setStateValue(digitalInputDigitalInputStatusStateTypeId, QVariant(obj["value"].toInt()).toBool());
                            break;
                        }
                    }
                }
            }
        }

        if (obj["dev"] == "output") {
            qCDebug(dcUniPi()) << "Output:" << obj["dev"].toString() << "Circuit:" <<  obj["circuit"].toString() << "Value:" << obj["value"].toInt();

            if (!m_digitalOutputs.contains(obj["circuit"].toString())){
                //New Device detected
                m_digitalOutputs.append(obj["circuit"].toString());
            } else {

                foreach (Device *device, myDevices()) {
                    if (device->deviceClassId() == digitalOutputDeviceClassId) {

                        if (obj["circuit"] == device->paramValue(digitalOutputDigitalOutputNumberParamTypeId).toString()) {
                            device->setStateValue(digitalOutputDigitalOutputStatusStateTypeId, QVariant(obj["value"].toInt()).toBool());
                            break;
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
                    //New Device detected
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
        }
    } // end for loop
}
