/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *                                                                         *
 *  Copyright (C) 2019 Bernhard Trinnes <bernhard.trinnes@nymea.io>        *
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

/*!
    \page ws2812fx.html
    \title WS2812FX Control
    \brief Plug-In to control WS2812FX over USB

    \ingroup plugins
    \ingroup nymea-plugins

    \chapter Plugin properties
    Following JSON file contains the definition and the description of all available \l{DeviceClass}{DeviceClasses}
    and \l{Vendor}{Vendors} of this \l{DevicePlugin}.

    For more details how to read this JSON file please check out the documentation for \l{The plugin JSON File}.

    \quotefile plugins/deviceplugins/ws2812fx/devicepluginws2812fx.json
*/
#include <QColor>
#include "devicepluginws2812fx.h"
#include "plugininfo.h"

DevicePluginWs2812fx ::DevicePluginWs2812fx ()
{
}

Device::DeviceSetupStatus DevicePluginWs2812fx::setupDevice(Device *device)
{
    if (device->deviceClassId() == ws2812fxDeviceClassId) {
        QString interface = device->paramValue(ws2812fxDeviceSerialPortParamTypeId).toString();

        if (!m_usedInterfaces.contains(interface)) {

            QSerialPort *serialPort = new QSerialPort(interface, this);
            if(!serialPort)
                return Device::DeviceSetupStatusFailure;

            serialPort->setBaudRate(115200);
            serialPort->setDataBits(QSerialPort::DataBits::Data8);
            serialPort->setParity(QSerialPort::Parity::NoParity);
            serialPort->setStopBits(QSerialPort::StopBits::OneStop);
            serialPort->setFlowControl(QSerialPort::FlowControl::NoFlowControl);

            if (!serialPort->open(QIODevice::ReadWrite)) {
                qCWarning(dcWs2812fx()) << "Could not open serial port" << interface << serialPort->errorString();
                serialPort->deleteLater();
                return Device::DeviceSetupStatusFailure;
            }

            connect(serialPort, SIGNAL(error(QSerialPort::SerialPortError)), this, SLOT(onSerialError(QSerialPort::SerialPortError)));
            connect(serialPort, SIGNAL(readyRead()), this, SLOT(onReadyRead()));

            qCDebug(dcWs2812fx()) << "Setup successfully serial port" << interface;
            device->setStateValue(ws2812fxConnectedStateTypeId, true);
            m_usedInterfaces.append(interface);
            m_serialPorts.insert(device, serialPort);

            if(!m_reconnectTimer) {
                m_reconnectTimer = new QTimer(this);
                m_reconnectTimer->setSingleShot(true);
                m_reconnectTimer->setInterval(5000);

                connect(m_reconnectTimer, &QTimer::timeout, this, &DevicePluginWs2812fx::onReconnectTimer);
            }
        } else {
            return Device::DeviceSetupStatusFailure;
        }
        return Device::DeviceSetupStatusSuccess;
    }
    return Device::DeviceSetupStatusFailure;
}


Device::DeviceError DevicePluginWs2812fx::discoverDevices(const DeviceClassId &deviceClassId, const ParamList &params)
{
    Q_UNUSED(params)
    // Create the list of available serial interfaces
    QList<DeviceDescriptor> deviceDescriptors;

    Q_FOREACH(QSerialPortInfo port, QSerialPortInfo::availablePorts()) {

        qCDebug(dcWs2812fx()) << "Found serial port:" << port.portName();
        QString description = port.manufacturer() + " " + port.description();
        DeviceDescriptor descriptor(deviceClassId, port.portName(), description);
        foreach (Device *existingDevice, myDevices().filterByParam(ws2812fxDeviceSerialPortParamTypeId, port.portName())) {
            descriptor.setDeviceId(existingDevice->id());
        }
        ParamList parameters;
        parameters.append(Param(ws2812fxDeviceSerialPortParamTypeId, port.portName()));
        descriptor.setParams(parameters);
        deviceDescriptors.append(descriptor);
    }
    emit devicesDiscovered(deviceClassId, deviceDescriptors);
    return Device::DeviceErrorAsync;
}


Device::DeviceError DevicePluginWs2812fx::executeAction(Device *device, const Action &action)
{
    if (device->deviceClassId() == ws2812fxDeviceClassId ) {

        QByteArray command;
        if (action.actionTypeId() == ws2812fxPowerActionTypeId) {
            command.append("b ");
            if (action.param(ws2812fxPowerActionPowerParamTypeId).value().toBool()) {
                command.append("30");
            } else {
                command.append("0");
            }
            command.append("\r\n");
            return sendCommand(device, action.id(), command, CommandType::Brightness);
        }

        if (action.actionTypeId() == ws2812fxBrightnessActionTypeId) {

            command.append("b ");
            command.append(action.param(ws2812fxBrightnessActionBrightnessParamTypeId).value().toString());
            command.append("\r\n");
            return sendCommand(device, action.id(), command, CommandType::Brightness);
        }

        if (action.actionTypeId() == ws2812fxSpeedActionTypeId) {

            command.append("s ");
            command.append(action.param(ws2812fxSpeedActionSpeedParamTypeId).value().toString());
            command.append("\r\n");
            return sendCommand(device, action.id(), command, CommandType::Speed);
        }

        if (action.actionTypeId() == ws2812fxColorActionTypeId) {

            QColor color;
            color= action.param(ws2812fxColorActionColorParamTypeId).value().value<QColor>();
            command.append("c ");
            command.append(QString(color.name()).remove("#"));
            command.append("\r\n");
            return sendCommand(device, action.id(), command, CommandType::Color);
        }

        if (action.actionTypeId() == ws2812fxColorTemperatureActionTypeId) {

            // minValue 153, maxValue 500
            QColor color;
            color.setRgb(255, 255, static_cast<int>((255.00-(((action.param(ws2812fxColorTemperatureActionColorTemperatureParamTypeId).value().toDouble()-153.00)/347.00))*255.00)));
            device->setStateValue(ws2812fxColorTemperatureStateTypeId, action.param(ws2812fxColorTemperatureActionColorTemperatureParamTypeId).value());
            command.append("c ");
            command.append(QString(color.name()).remove("#"));
            command.append("\r\n");
            return sendCommand(device, action.id(), command, CommandType::Color);
        }

        if (action.actionTypeId() == ws2812fxEffectModeActionTypeId) {

            QString effectMode = action.param(ws2812fxEffectModeActionEffectModeParamTypeId).value().toString();
            command.append("m ");
            if (effectMode == "Static") {
                command.append(QString::number(FX_MODE_STATIC));
            } else if (effectMode == "Blink") {
                command.append(QString::number(FX_MODE_BLINK));
            } else if (effectMode == "Color Wipe") {
                command.append(QString::number(FX_MODE_COLOR_WIPE));
            } else if (effectMode == "Color Wipe Inverse") {
                command.append(QString::number(FX_MODE_COLOR_WIPE_INV));
            } else if (effectMode == "Color Wipe Reverse") {
                command.append(QString::number(FX_MODE_COLOR_WIPE_REV));
            } else if (effectMode == "Color Wipe Reverse Inverse") {
                command.append(QString::number(FX_MODE_COLOR_WIPE_REV_INV));
            } else if (effectMode == "Color Wipe Random") {
                command.append(QString::number(FX_MODE_COLOR_WIPE_RANDOM));
            } else if (effectMode == "Random Color") {
                command.append(QString::number(FX_MODE_RANDOM_COLOR));
            } else if (effectMode == "Single Dynamic") {
                command.append(QString::number(FX_MODE_SINGLE_DYNAMIC));
            } else if (effectMode == "Multi Dynamic") {
                command.append(QString::number(FX_MODE_MULTI_DYNAMIC));
            } else if (effectMode == "Rainbow") {
                command.append(QString::number(FX_MODE_RAINBOW));
            } else if (effectMode == "Rainbow Cycle") {
                command.append(QString::number(FX_MODE_RAINBOW_CYCLE));
            } else if (effectMode == "Scan") {
                command.append(QString::number(FX_MODE_SCAN));
            } else if (effectMode == "Dual Scan") {
                command.append(QString::number(FX_MODE_DUAL_SCAN));
            } else if (effectMode == "Fade") {
                command.append(QString::number(FX_MODE_FADE));
            } else if (effectMode == "Theater Chase") {
                command.append(QString::number(FX_MODE_THEATER_CHASE));
            } else if (effectMode == "Theater Chase Rainbow") {
                command.append(QString::number(FX_MODE_THEATER_CHASE_RAINBOW));
            } else if (effectMode == "Running Lights") {
                command.append(QString::number(FX_MODE_RUNNING_LIGHTS));
            } else if (effectMode == "Twinkle") {
                command.append(QString::number(FX_MODE_TWINKLE));
            } else if (effectMode == "Twinkle Random") {
                command.append(QString::number(FX_MODE_TWINKLE_RANDOM));
            } else if (effectMode == "Twinkle Fade") {
                command.append(QString::number(FX_MODE_TWINKLE_FADE));
            } else if (effectMode == "Twinkle Fade Random") {
                command.append(QString::number(FX_MODE_TWINKLE_FADE_RANDOM));
            } else if (effectMode == "Sparkle") {
                command.append(QString::number(FX_MODE_SPARKLE));
            } else if (effectMode == "Flash Sparkle") {
                command.append(QString::number(FX_MODE_FLASH_SPARKLE));
            } else if (effectMode == "Hyper Sparkle") {
                command.append(QString::number(FX_MODE_HYPER_SPARKLE));
            } else if (effectMode == "Strobe") {
                command.append(QString::number(FX_MODE_STROBE));
            } else if (effectMode == "Strobe Rainbow") {
                command.append(QString::number(FX_MODE_STROBE_RAINBOW));
            } else if (effectMode == "Multi Strobe") {
                command.append(QString::number(FX_MODE_MULTI_STROBE));
            } else if (effectMode == "Blink Rainbow") {
                command.append(QString::number(FX_MODE_BLINK_RAINBOW));
            } else if (effectMode == "Chase White") {
                command.append(QString::number(FX_MODE_CHASE_WHITE));
            } else if (effectMode == "Chase Color") {
                command.append(QString::number(FX_MODE_CHASE_COLOR));
            } else if (effectMode == "Chase Random") {
                command.append(QString::number(FX_MODE_CHASE_RANDOM));
            } else if (effectMode == "Chase Flash") {
                command.append(QString::number(FX_MODE_CHASE_FLASH));
            } else if (effectMode == "Chase Flash Random") {
                command.append(QString::number(FX_MODE_CHASE_FLASH_RANDOM));
            } else if (effectMode == "Chase Rainbow White") {
                command.append(QString::number(FX_MODE_CHASE_RAINBOW_WHITE));
            } else if (effectMode == "Chase Blackout") {
                command.append(QString::number(FX_MODE_CHASE_BLACKOUT));
            } else if (effectMode == "Chase Blackout Rainbow") {
                command.append(QString::number(FX_MODE_CHASE_BLACKOUT_RAINBOW));
            } else if (effectMode == "Color Sweep Random") {
                command.append(QString::number(FX_MODE_COLOR_SWEEP_RANDOM));
            } else if (effectMode == "Running Color") {
                command.append(QString::number(FX_MODE_RUNNING_COLOR));
            } else if (effectMode == "Running Red Blue") {
                command.append(QString::number(FX_MODE_RUNNING_RED_BLUE));
            } else if (effectMode == "Running Random") {
                command.append(QString::number(FX_MODE_RUNNING_RANDOM));
            }else if (effectMode == "Larson Scanner") {
                command.append(QString::number(FX_MODE_LARSON_SCANNER));
            }else if (effectMode == "Comet") {
                command.append(QString::number(FX_MODE_COMET));
            }else if (effectMode == "Fireworks") {
                command.append(QString::number(FX_MODE_FIREWORKS));
            }else if (effectMode == "Fireworks Random") {
                command.append(QString::number(FX_MODE_FIREWORKS_RANDOM));
            }else if (effectMode == "Merry Christmas") {
                command.append(QString::number(FX_MODE_MERRY_CHRISTMAS));
            }else if (effectMode == "Fire Flicker") {
                command.append(QString::number(FX_MODE_FIRE_FLICKER));
            }else if (effectMode == "Fire Flicker (soft)") {
                command.append(QString::number(FX_MODE_FIRE_FLICKER_SOFT));
            }else if (effectMode == "Fire Flicker (intense)") {
                command.append(QString::number(FX_MODE_FIRE_FLICKER_INTENSE));
            }else if (effectMode == "Circus Combustus") {
                command.append(QString::number(FX_MODE_CIRCUS_COMBUSTUS));
            }else if (effectMode == "Halloween") {
                command.append(QString::number(FX_MODE_HALLOWEEN));
            }else if (effectMode == "Bicolor Chase") {
                command.append(QString::number(FX_MODE_BICOLOR_CHASE));
            }else if (effectMode == "Tricolor Chase") {
                command.append(QString::number(FX_MODE_TRICOLOR_CHASE));
            }else if (effectMode == "ICU") {
                command.append(QString::number(FX_MODE_ICU));
            }else if (effectMode == "Custom 0") {
                command.append(QString::number(FX_MODE_CUSTOM_0));
            }else if (effectMode == "Custom 1") {
                command.append(QString::number(FX_MODE_CUSTOM_1));
            }else if (effectMode == "Custom 2") {
                command.append(QString::number(FX_MODE_CUSTOM_2));
            }else if (effectMode == "Custom 3") {
                command.append(QString::number(FX_MODE_CUSTOM_3));
            }
            command.append("\r\n");
            return sendCommand(device, action.id(), command, CommandType::Mode);
        }
        return Device::DeviceErrorActionTypeNotFound;
    }
    return Device::DeviceErrorDeviceClassNotFound;
}


void DevicePluginWs2812fx::deviceRemoved(Device *device)
{
    if (device->deviceClassId() == ws2812fxDeviceClassId) {

        m_usedInterfaces.removeAll(device->paramValue(ws2812fxDeviceSerialPortParamTypeId).toString());
        QSerialPort *serialPort = m_serialPorts.take(device);
        if(serialPort) {
            serialPort->flush();
            serialPort->close();
            serialPort->deleteLater();
        }
    }

    if (myDevices().empty()) {
        m_reconnectTimer->stop();
        m_reconnectTimer->deleteLater();
    }
}

void DevicePluginWs2812fx::onReadyRead()
{
    QSerialPort *serialPort =  static_cast<QSerialPort*>(sender());

    Device *device = m_serialPorts.key(serialPort);
    if (!device)
        return;

    QByteArray data;
    while (serialPort->canReadLine()) {
        data = serialPort->readLine();
        qDebug(dcWs2812fx()) << "Message received" << data;

        if (data.contains("mode")) {
            if (m_pendingActions.contains(CommandType::Mode)) {
                emit actionExecutionFinished(m_pendingActions.value(CommandType::Mode), Device::DeviceErrorNoError);
            }
            QByteArrayList list = data.split('-');
            if(list.length() >= 2) {
                QString mode = list.at(1);
                mode.remove(0, 1);
                mode.remove("\r\n");
                qDebug(dcWs2812fx()) << "set mode to:" << mode;
                device->setStateValue(ws2812fxEffectModeStateTypeId, mode);
            }
        }
        if (data.contains("brightness")) {
            if (m_pendingActions.contains(CommandType::Brightness)) {
                emit actionExecutionFinished(m_pendingActions.value(CommandType::Brightness), Device::DeviceErrorNoError);
            }
            QByteArrayList list = data.split(':');
            if(list.length() >= 2) {
                QString rawBrightness = list.at(1);;
                rawBrightness.remove(" ");
                rawBrightness.remove("\r\n");
                int brightness = rawBrightness.toInt();

                qDebug(dcWs2812fx()) << "set brightness to:" << brightness;
                device->setStateValue(ws2812fxBrightnessStateTypeId, brightness);
                if (brightness == 0) {
                    device->setStateValue(ws2812fxPowerStateTypeId, false);
                } else {
                    device->setStateValue(ws2812fxPowerStateTypeId, true);
                }
            }
        }
        if (data.contains("speed")) {
            if (m_pendingActions.contains(CommandType::Speed)) {
                emit actionExecutionFinished(m_pendingActions.value(CommandType::Speed), Device::DeviceErrorNoError);
            }
            QByteArrayList list = data.split(':');
            if(list.length() >= 2) {
                QString rawSpeed = list.at(1);
                rawSpeed.remove(" ");
                rawSpeed.remove("\r\n");
                int speed = data.split(':').at(1).toInt();

                qDebug(dcWs2812fx()) << "set speed to:" << speed;
                device->setStateValue(ws2812fxSpeedStateTypeId, speed);
            }
        }
        if (data.contains("color")) {
            if (m_pendingActions.contains(CommandType::Color)) {
                emit actionExecutionFinished(m_pendingActions.value(CommandType::Color), Device::DeviceErrorNoError);
            }
            QByteArrayList list = data.split(':');
            if(list.length() >= 2) {
                QString rawColor = list.at(1);
                rawColor.remove(" ");
                rawColor.remove("0x");
                rawColor.remove("\r\n");
                rawColor.prepend("#");
                qDebug(dcWs2812fx()) << "set color to:" << rawColor;
                device->setStateValue(ws2812fxColorStateTypeId, rawColor);
            }
        }
    }
}

void DevicePluginWs2812fx::onReconnectTimer()
{
    foreach(Device *device, myDevices()) {
        if (!device->stateValue(ws2812fxConnectedStateTypeId).toBool()) {
            QSerialPort *serialPort =  m_serialPorts.value(device);
            if (serialPort) {
                if (serialPort->open(QSerialPort::ReadWrite)) {
                    device->setStateValue(ws2812fxConnectedStateTypeId, true);
                } else {
                    device->setStateValue(ws2812fxConnectedStateTypeId, false);
                    m_reconnectTimer->start();
                }
            }
        }
    }

}

void DevicePluginWs2812fx::onSerialError(QSerialPort::SerialPortError error)
{
    QSerialPort *serialPort =  static_cast<QSerialPort*>(sender());
    Device *device = m_serialPorts.key(serialPort);
    if (!device)
        return;

    if (error != QSerialPort::NoError && serialPort->isOpen()) {
        qCCritical(dcWs2812fx()) << "Serial port error:" << error << serialPort->errorString();
        m_reconnectTimer->start();
        serialPort->close();
        device->setStateValue(ws2812fxConnectedStateTypeId, false);
    }
}

Device::DeviceError DevicePluginWs2812fx::sendCommand(Device* device, ActionId actionId, const QByteArray &command, CommandType commandType)
{
    qDebug(dcWs2812fx()) << "Sending command" << command;
    QSerialPort *serialPort = m_serialPorts.value(device);
    if (!serialPort)
        return Device::DeviceErrorDeviceNotFound;
    if (serialPort->write(command) != command.length()) {
        qCWarning(dcWs2812fx) << "Error writing to serial port";
        return Device::DeviceErrorHardwareFailure;
    }
    m_pendingActions.insert(commandType, actionId);
    return Device::DeviceErrorAsync;
}
