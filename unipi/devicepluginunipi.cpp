/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *                                                                         *
 *  Copyright (C) 2017 Bernhard Trinnes <bernhard.trinnes@guh.io>          *
 *                                                                         *
 *  This file is part of guh.                                              *
 *                                                                         *
 *  Guh is free software: you can redistribute it and/or modify            *
 *  it under the terms of the GNU General Public License as published by   *
 *  the Free Software Foundation, version 2 of the License.                *
 *                                                                         *
 *  Guh is distributed in the hope that it will be useful,                 *
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of         *
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the           *
 *  GNU General Public License for more details.                           *
 *                                                                         *
 *  You should have received a copy of the GNU General Public License      *
 *  along with guh. If not, see <http://www.gnu.org/licenses/>.            *
 *                                                                         *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

#include "devicepluginunipi.h"
#include "plugininfo.h"
#include <QJsonDocument>



DevicePluginUniPi::DevicePluginUniPi()
{
}


void DevicePluginUniPi::init()
{

}


void DevicePluginUniPi::createAutoDevice(GPIOType gpioType, DeviceId parentDeviceId, QString deviceName)
{
    ParamList params;

    switch(gpioType){
    case relay : {
        QList<DeviceDescriptor> relayOutputDescriptors;
        DeviceDescriptor descriptor(relayOutputDeviceClassId, "Relay Output", deviceName);
        params.append(Param(relayOutputRelayNumberParamTypeId, deviceName));
        params.append(Param(relayOutputParentIdParamTypeId, parentDeviceId));
        descriptor.setParams(params);
        relayOutputDescriptors.append(descriptor);
        emit autoDevicesAppeared(relayOutputDeviceClassId, relayOutputDescriptors);
        break;
    }

    case digitalOutput : {
        QList<DeviceDescriptor> digitalOutputDescriptors;
        DeviceDescriptor descriptor(digitalOutputDeviceClassId, "Digital Output", deviceName);
        params.append(Param(digitalOutputDigitalOutputNumberParamTypeId, deviceName));
        params.append(Param(digitalOutputParentIdParamTypeId, parentDeviceId));
        descriptor.setParams(params);
        digitalOutputDescriptors.append(descriptor);
        emit autoDevicesAppeared(digitalOutputDeviceClassId, digitalOutputDescriptors);
        break;
    }

    case digitalInput: {
        QList<DeviceDescriptor> digitalInputDescriptors;
        DeviceDescriptor descriptor(digitalInputDeviceClassId, "Digital Input", deviceName);
        params.append(Param(digitalInputDigitalInputNumberParamTypeId, deviceName));
        params.append(Param(digitalInputParentIdParamTypeId, parentDeviceId));
        descriptor.setParams(params);
        digitalInputDescriptors.append(descriptor);
        emit autoDevicesAppeared(digitalInputDeviceClassId, digitalInputDescriptors);
        break;
    }

    case analogOutput: {
        QList<DeviceDescriptor> analogOutputDescriptors;
        DeviceDescriptor descriptor(analogOutputDeviceClassId, "Analog Output", deviceName);
        params.append(Param(analogOutputAnalogOutputNumberParamTypeId,  deviceName));
        params.append(Param(analogOutputParentIdParamTypeId, parentDeviceId));
        descriptor.setParams(params);
        analogOutputDescriptors.append(descriptor);
        emit autoDevicesAppeared(analogOutputDeviceClassId, analogOutputDescriptors);
        break;
    }

    case analogInput: {
        QList<DeviceDescriptor> analogInputDescriptors;
        DeviceDescriptor descriptor(analogInputDeviceClassId, "Analog Input", deviceName);
        params.append(Param(analogInputAnalogInputNumberParamTypeId, deviceName));
        params.append(Param(analogInputParentIdParamTypeId, parentDeviceId));
        descriptor.setParams(params);
        analogInputDescriptors.append(descriptor);
        emit autoDevicesAppeared(analogInputDeviceClassId, analogInputDescriptors);
        break;
    }

    default:
        break;

    }
}

DeviceManager::DeviceSetupStatus DevicePluginUniPi::setupDevice(Device *device)
{
    if (device->deviceClassId() == neuronDeviceClassId) {

        int port = device->paramValue(neuronPortParamTypeId).toInt();

        m_webSocket = new QWebSocket();
        connect(m_webSocket, &QWebSocket::connected, this, &DevicePluginUniPi::onWebSocketConnected);
        connect(m_webSocket, &QWebSocket::disconnected, this, &DevicePluginUniPi::onWebSocketDisconnected);

        QUrl url = QUrl("ws://localhost/ws");
        url.setPort(port);
        qDebug(dcUniPi()) << "Conneting to:" << url.toString();
        m_webSocket->open(url);
        m_parentId = device->id();

        return DeviceManager::DeviceSetupStatusSuccess;
    }

    if (device->deviceClassId() == relayOutputDeviceClassId) {

        device->setName(QString("Relay Ouput %1").arg(device->paramValue(relayOutputRelayNumberParamTypeId).toString()));
        device->setParentId(device->paramValue(relayOutputParentIdParamTypeId).toString());
        return DeviceManager::DeviceSetupStatusSuccess;
    }

    if (device->deviceClassId() == digitalOutputDeviceClassId) {

        device->setName(QString("Digital Output %1").arg(device->paramValue(digitalOutputDigitalOutputNumberParamTypeId).toString()));
        device->setParentId(device->paramValue(digitalOutputParentIdParamTypeId).toString());
        return DeviceManager::DeviceSetupStatusSuccess;
    }

    if (device->deviceClassId() == digitalInputDeviceClassId) {

        device->setName(QString("Digital Input %1").arg(device->paramValue(digitalInputDigitalInputNumberParamTypeId).toString()));
        device->setParentId(device->paramValue(digitalInputParentIdParamTypeId).toString());
        return DeviceManager::DeviceSetupStatusSuccess;
    }

    if (device->deviceClassId() == analogInputDeviceClassId) {

        device->setName(QString("Analog Input %1").arg(device->paramValue(analogInputAnalogInputNumberParamTypeId).toString()));
        device->setParentId(device->paramValue(analogInputParentIdParamTypeId).toString());
        return DeviceManager::DeviceSetupStatusSuccess;
    }

    if (device->deviceClassId() == analogOutputDeviceClassId) {

        device->setName(QString("Analog Output %1").arg(device->paramValue(analogOutputAnalogOutputNumberParamTypeId).toString()));
        device->setParentId(device->paramValue(analogOutputParentIdParamTypeId).toString());
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

    if (device->deviceClassId() == relayOutputDeviceClassId) {

        if (action.actionTypeId() == relayOutputRelayStatusActionTypeId) {
            QString relayNumber = device->paramValue(relayOutputRelayNumberParamTypeId).toString();
            int stateValue = action.param(relayOutputRelayStatusStateParamTypeId).value().toInt();

            QJsonObject json;
            json["cmd"] = "set";
            json["dev"] = "relay";
            json["circuit"] = relayNumber;
            json["value"] = stateValue;

            QJsonDocument doc(json);
            QByteArray bytes = doc.toJson(QJsonDocument::Compact);
            qDebug(dcUniPi()) << "Send command" << bytes;
            m_webSocket->sendBinaryMessage(bytes);
            return DeviceManager::DeviceErrorNoError;
        }
        return DeviceManager::DeviceErrorActionTypeNotFound;
    }


    if (device->deviceClassId() == digitalOutputDeviceClassId) {
        if (action.actionTypeId() == digitalOutputDigitalOutputStatusActionTypeId) {
            int digitalOutputNumber = device->paramValue(digitalOutputDigitalOutputNumberParamTypeId).toInt();
            int stateValue = action.param(digitalOutputDigitalOutputStatusStateParamTypeId).value().toInt();

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
            double analogValue = device->paramValue(analogOutputAnalogOutputValueStateParamTypeId).toDouble();

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
    return DeviceManager::DeviceErrorDeviceClassNotFound;
}


void DevicePluginUniPi::deviceRemoved(Device *device)
{
    Q_UNUSED(device);

    m_webSocket->close();
    m_webSocket->deleteLater();
}


void DevicePluginUniPi::onWebSocketConnected()
{
    qDebug(dcUniPi()) << "WebSocket connected";

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
    qDebug(dcUniPi())  << "WebSocket disconnected";

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
            qDebug(dcUniPi()) << "Document is not an object nor an array";
        }
    } else {
        qDebug(dcUniPi()) << "Invalid JSON";
    }

    for (int levelIndex = 0; levelIndex < array.size(); ++levelIndex) {
        obj = array[levelIndex].toObject();

        if (obj["cmd"] == "all") {
            //read model number
            qDebug(dcUniPi()) << message;

        }

        if (obj["dev"] == "relay") {
            qDebug(dcUniPi()) << "Relay:" << obj["dev"].toString() << "Circuit:" <<  obj["circuit"].toString() << "Value:" << obj["value"].toInt();

            bool newDevice = true;
            foreach (Device *device, myDevices()) {
                if (device->deviceClassId() == relayOutputDeviceClassId) {
                    if (obj["circuit"] == device->paramValue(relayOutputRelayNumberParamTypeId).toString()) {
                        device->setStateValue(relayOutputRelayStatusStateTypeId, QVariant(obj["value"].toInt()).toBool());
                        newDevice = false;
                        break;
                    }
                }
            }
            if (newDevice) {
                qDebug(dcUniPi()) << "New Device detected";
                createAutoDevice(GPIOType::relay, m_parentId, obj["circuit"].toString());
            }
        }

        if (obj["dev"] == "input") {
            qDebug(dcUniPi()) << "Input:" << obj["dev"].toString() << "Circuit:" <<  obj["circuit"].toString() << "Value:" << obj["value"].toInt();

            bool newDevice = true;
            foreach (Device *device, myDevices()) {
                if (device->deviceClassId() == digitalInputDeviceClassId) {
                    if (obj["circuit"] == device->paramValue(digitalInputDigitalInputNumberParamTypeId).toString()) {
                        device->setStateValue(digitalInputDigitalInputStatusStateTypeId, QVariant(obj["value"].toInt()).toBool());
                        newDevice = false;
                        break;
                    }
                }
            }
            if (newDevice){
                //New Device detected
                createAutoDevice(GPIOType::digitalInput, m_parentId, obj["circuit"].toString());
            }
        }

        if (obj["dev"] == "output") {
            qDebug(dcUniPi()) << "Output:" << obj["dev"].toString() << "Circuit:" <<  obj["circuit"].toString() << "Value:" << obj["value"].toInt();

            bool newDevice = true;
            foreach (Device *device, myDevices()) {
                if (device->deviceClassId() == digitalOutputDeviceClassId) {

                    if (obj["circuit"] == device->paramValue(digitalOutputDigitalOutputNumberParamTypeId).toString()) {
                        device->setStateValue(digitalOutputDigitalOutputStatusStateTypeId, QVariant(obj["value"].toInt()).toBool());
                        newDevice = false;
                        break;
                    }
                }
            }
            if (newDevice){
                //New Device detected
                createAutoDevice(GPIOType::digitalOutput, m_parentId, obj["circuit"].toString());
            }
        }

        if (obj["dev"] == "ao") {
            qDebug(dcUniPi()) << "Analog Output:" << obj["dev"] << "Circuit:" <<  obj["circuit"].toString() << "Value:" << obj["value"].toDouble();

            bool newDevice = true;
            foreach (Device *device, myDevices()) {
                if (device->deviceClassId() == analogOutputDeviceClassId) {

                    if (obj["circuit"] == device->paramValue(analogOutputAnalogOutputNumberParamTypeId).toString()) {
                        device->setStateValue(analogOutputAnalogOutputValueStateTypeId, obj["value"].toDouble());
                        newDevice = false;
                        break;
                    }
                }
            }
            if (newDevice){
                //New Device detected
                createAutoDevice(GPIOType::analogOutput, m_parentId, obj["circuit"].toString());
            }
        }

        if (obj["dev"] == "ai") {
            qDebug(dcUniPi()) << "Analog Input:" << obj["dev"] << "Circuit:" <<  obj["circuit"].toString() << "Value:" << obj["value"].toDouble();

            bool newDevice = true;
            foreach (Device *device, myDevices()) {
                if (device->deviceClassId() == analogInputDeviceClassId) {

                    if (obj["circuit"] == device->paramValue(analogInputAnalogInputNumberParamTypeId).toString()) {
                        device->setStateValue(analogInputAnalogInputValueStateTypeId, obj["value"].toDouble());
                        newDevice = false;
                        break;
                    }
                }
            }
            if (newDevice){
                //New Device detected
                createAutoDevice(GPIOType::analogInput, m_parentId, obj["circuit"].toString());
            }
        }
    }
}

