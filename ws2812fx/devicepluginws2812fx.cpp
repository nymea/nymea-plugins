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

DeviceManager::DeviceSetupStatus DevicePluginWs2812fx::setupDevice(Device *device)
{
    if (device->deviceClassId() == ws2812fxDeviceClassId) {
        QString interface = device->paramValue(ws2812fxDeviceSerialPortParamTypeId).toString();

        if (!m_usedInterfaces.contains(interface)) {

            QSerialPort *serialPort = new QSerialPort(interface, this);
            if(!serialPort)
                return DeviceManager::DeviceSetupStatusFailure;

            serialPort->setBaudRate(11520);
            serialPort->setDataBits(QSerialPort::DataBits::Data8);
            serialPort->setParity(QSerialPort::Parity::EvenParity);
            serialPort->setStopBits(QSerialPort::StopBits::OneStop);
            serialPort->setFlowControl(QSerialPort::FlowControl::NoFlowControl);

            if (!serialPort->open(QIODevice::ReadWrite)) {
                qCWarning(dcWs2812fx()) << "Could not open serial port" << interface << serialPort->errorString();
                return DeviceManager::DeviceSetupStatusFailure;
            }

            connect(serialPort, SIGNAL(error(QSerialPort::SerialPortError)), this, SLOT(onSerialError(QSerialPort::SerialPortError)));
            connect(serialPort, SIGNAL(readyRead()), this, SLOT(onReadyRead()));

            qCDebug(dcWs2812fx()) << "Setup successfully serial port" << interface;
            m_usedInterfaces.append(interface);
            m_serialPorts.insert(device, serialPort);
        } else {
            return DeviceManager::DeviceSetupStatusFailure;
        }
        return DeviceManager::DeviceSetupStatusSuccess;
    }
    return DeviceManager::DeviceSetupStatusFailure;
}


DeviceManager::DeviceError DevicePluginWs2812fx::discoverDevices(const DeviceClassId &deviceClassId, const ParamList &params)
{
    Q_UNUSED(params)
    // Create the list of available serial interfaces
    QList<DeviceDescriptor> deviceDescriptors;

    Q_FOREACH(QSerialPortInfo port, QSerialPortInfo::availablePorts()) {
        if (m_usedInterfaces.contains(port.portName())){
            //device already in use
            qCDebug(dcWs2812fx()) << "Found serial port that is already used:" << port.portName();
        } else {
            //Serial port is not yet used, create now a new one
            qCDebug(dcWs2812fx()) << "Found serial port:" << port.portName();
            QString description = port.manufacturer() + " " + port.description();
            DeviceDescriptor descriptor(deviceClassId, port.portName(), description);
            ParamList parameters;
            parameters.append(Param(ws2812fxDeviceSerialPortParamTypeId, port.portName()));
            descriptor.setParams(parameters);
            deviceDescriptors.append(descriptor);
        }
    }
    emit devicesDiscovered(deviceClassId, deviceDescriptors);
    return DeviceManager::DeviceErrorAsync;
}


DeviceManager::DeviceError DevicePluginWs2812fx::executeAction(Device *device, const Action &action)
{
    qCDebug(dcWs2812fx) << "Execute action" << action.actionTypeId() << action.params();

    if (device->deviceClassId() == ws2812fxDeviceClassId ) {

        QSerialPort *serialPort = m_serialPorts.value(device);
        if (!serialPort)
            return DeviceManager::DeviceErrorDeviceNotFound;
        QByteArray command;

        if (action.actionTypeId() == ws2812fxBrightnessActionTypeId) {

            command.append("b ");
            command.append(action.param(ws2812fxBrightnessActionBrightnessParamTypeId).value().toString());
            serialPort->write(command);
            return DeviceManager::DeviceErrorAsync;
        }

        if (action.actionTypeId() == ws2812fxSpeedActionTypeId) {

            command.append("s ");
            command.append(action.param(ws2812fxSpeedActionSpeedParamTypeId).value().toString());
            serialPort->write(command);
            return DeviceManager::DeviceErrorAsync;
        }

        if (action.actionTypeId() == ws2812fxColorActionTypeId) {

            command.append("c ");
            command.append(action.param(ws2812fxSpeedActionSpeedParamTypeId).value().toString());
            serialPort->write(command);
            return DeviceManager::DeviceErrorAsync;
        }

        if (action.actionTypeId() == ws2812fxColorTemperatureActionTypeId) {

            // minValue 153, maxValue 500
            QColor color;
            color.setRgb(255, 255, ((action.param(ws2812fxColorActionColorParamTypeId).value().toInt()-153)/347.00));
            command.append("c ");
            command.append(color.rgb());
            serialPort->write(command);
            return DeviceManager::DeviceErrorAsync;
        }

        if (action.actionTypeId() == ws2812fxEffectModeActionTypeId) {

            QString effectMode = action.param(ws2812fxEffectModeActionEffectModeParamTypeId).value().toString();
            command.append("m ");
            if (effectMode == "Static") {
                command.append(static_cast<int>(FX_MODE_STATIC));
            } else if (effectMode == "Blink") {
                command.append(FX_MODE_BLINK);
            } else if (effectMode == "Color Wipe") {
                command.append(FX_MODE_COLOR_WIPE);
            } else if (effectMode == "Color Wipe Inverse") {
                command.append(FX_MODE_COLOR_WIPE_INV);
            } else if (effectMode == "Color Wipe Reverse") {
                command.append(FX_MODE_COLOR_WIPE_REV);
            } else if (effectMode == "Color Wipe Reverse Inverse") {
                command.append(FX_MODE_COLOR_WIPE_REV_INV);
            } else if (effectMode == "Color Wipe Random") {
                command.append(FX_MODE_COLOR_WIPE_RANDOM);
            } else if (effectMode == "Random Color") {
                command.append(FX_MODE_RANDOM_COLOR);
            } else if (effectMode == "Single Dynamic") {
                command.append(FX_MODE_SINGLE_DYNAMIC);
            } else if (effectMode == "Multi Dynamic") {
                command.append(FX_MODE_MULTI_DYNAMIC);
            } else if (effectMode == "Rainbow") {
                command.append(FX_MODE_RAINBOW);
            } else if (effectMode == "Rainbow Cycle") {
                command.append(FX_MODE_RAINBOW_CYCLE);
            } else if (effectMode == "Scan") {
                command.append(FX_MODE_SCAN);
            } else if (effectMode == "Dual Scan") {
                command.append(FX_MODE_DUAL_SCAN);
            } else if (effectMode == "Fade") {
                command.append(FX_MODE_FADE);
            } else if (effectMode == "Theater Chase") {
                command.append(FX_MODE_THEATER_CHASE);
            } else if (effectMode == "Theater Chase Rainbow") {
                command.append(FX_MODE_THEATER_CHASE_RAINBOW);
            } else if (effectMode == "Running Lights") {
                command.append(FX_MODE_RUNNING_LIGHTS);
            } else if (effectMode == "Twinkle") {
                command.append(FX_MODE_TWINKLE);
            } else if (effectMode == "Twinkle Random") {
                command.append(FX_MODE_TWINKLE_RANDOM);
            } else if (effectMode == "Twinkle Fade") {
                command.append(FX_MODE_TWINKLE_FADE);
            } else if (effectMode == "Twinkle Fade Random") {
                command.append(FX_MODE_TWINKLE_FADE_RANDOM);
            } else if (effectMode == "Sparkle") {
                command.append(FX_MODE_SPARKLE);
            } else if (effectMode == "Flash Sparkle") {
                command.append(FX_MODE_FLASH_SPARKLE);
            } else if (effectMode == "Hyper Sparkle") {
                command.append(FX_MODE_HYPER_SPARKLE);
            } else if (effectMode == "Strobe") {
                command.append(FX_MODE_STROBE);
            } else if (effectMode == "Strobe Rainbow") {
                command.append(FX_MODE_STROBE_RAINBOW);
            } else if (effectMode == "Multi Strobe") {
                command.append(FX_MODE_MULTI_STROBE);
            } else if (effectMode == "Blink Rainbow") {
                command.append(FX_MODE_BLINK_RAINBOW);
            } else if (effectMode == "Chase White") {
                command.append(FX_MODE_CHASE_WHITE);
            } else if (effectMode == "Chase Color") {
                command.append(FX_MODE_CHASE_COLOR);
            } else if (effectMode == "Chase Random") {
                command.append(FX_MODE_CHASE_RANDOM);
            } else if (effectMode == "Chase Flash") {
                command.append(FX_MODE_CHASE_FLASH);
            } else if (effectMode == "Chase Flash Random") {
                command.append(FX_MODE_CHASE_FLASH_RANDOM);
            } else if (effectMode == "Chase Rainbow White") {
                command.append(FX_MODE_CHASE_RAINBOW_WHITE);
            } else if (effectMode == "Chase Blackout") {
                command.append(FX_MODE_CHASE_BLACKOUT);
            } else if (effectMode == "Chase Blackout Rainbow") {
                command.append(FX_MODE_CHASE_BLACKOUT_RAINBOW);
            } else if (effectMode == "Color Sweep Random") {
                command.append(FX_MODE_COLOR_SWEEP_RANDOM);
            } else if (effectMode == "Running Color") {
                command.append(FX_MODE_RUNNING_COLOR);
            } else if (effectMode == "Running Red Blue") {
                command.append(FX_MODE_RUNNING_RED_BLUE);
            } else if (effectMode == "Running Random") {
                command.append(FX_MODE_RUNNING_RANDOM);
            }else if (effectMode == "Larson Scanner") {
                command.append(FX_MODE_LARSON_SCANNER);
            }else if (effectMode == "Comet") {
                command.append(FX_MODE_COMET);
            }else if (effectMode == "Fireworks") {
                command.append(FX_MODE_FIREWORKS);
            }else if (effectMode == "Fireworks Random") {
                command.append(FX_MODE_FIREWORKS_RANDOM);
            }else if (effectMode == "Merry Christmas") {
                command.append(FX_MODE_MERRY_CHRISTMAS);
            }else if (effectMode == "Fire Flicker") {
                command.append(FX_MODE_FIRE_FLICKER);
            }else if (effectMode == "Fire Flicker (soft)") {
                command.append(FX_MODE_FIRE_FLICKER_SOFT);
            }else if (effectMode == "Fire Flicker (intense)") {
                command.append(FX_MODE_FIRE_FLICKER_INTENSE);
            }else if (effectMode == "Circus Combustus") {
                command.append(FX_MODE_CIRCUS_COMBUSTUS);
            }else if (effectMode == "Halloween") {
                command.append(FX_MODE_HALLOWEEN);
            }else if (effectMode == "Bicolor Chase") {
                command.append(FX_MODE_BICOLOR_CHASE);
            }else if (effectMode == "Tricolor Chase") {
                command.append(FX_MODE_TRICOLOR_CHASE);
            }else if (effectMode == "ICU") {
                command.append(FX_MODE_ICU);
            }else if (effectMode == "Custom 1") {
                command.append(FX_MODE_CUSTOM_0);
            }else if (effectMode == "Custom 2") {
                command.append(FX_MODE_CUSTOM_1);
            }else if (effectMode == "Custom 3") {
                command.append(FX_MODE_CUSTOM_2);
            }else if (effectMode == "Custom 4") {
                command.append(FX_MODE_CUSTOM_3);
            }
            serialPort->write(command);
            return DeviceManager::DeviceErrorAsync;
        }
        return DeviceManager::DeviceErrorActionTypeNotFound;
    }
    return DeviceManager::DeviceErrorDeviceClassNotFound;
}


void DevicePluginWs2812fx::deviceRemoved(Device *device)
{
    if (device->deviceClassId() == ws2812fxDeviceClassId) {

        m_usedInterfaces.removeAll(device->paramValue(ws2812fxDeviceSerialPortParamTypeId).toString());
        QSerialPort *serialPort = m_serialPorts.take(device);
        serialPort->close();
        serialPort->deleteLater();
    }
}

void DevicePluginWs2812fx::onReadyRead()
{
    QSerialPort *serialPort =  static_cast<QSerialPort*>(sender());
    Device *device = m_serialPorts.key(serialPort);
    Q_UNUSED(device);

    QByteArray data;
    while (!serialPort->atEnd()) {
        data = serialPort->read(100);
    }
    qDebug(dcWs2812fx()) << "Message received" << data;
}

void DevicePluginWs2812fx::onSerialError(QSerialPort::SerialPortError error)
{
    qCWarning(dcWs2812fx()) << "Serial Port error happened:" << error;
}
