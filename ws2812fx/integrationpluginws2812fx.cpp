/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*
* Copyright 2013 - 2025, nymea GmbH
* Contact: contact@nymea.io
*
* This file is part of nymea.
* This project including source code and documentation is protected by
* copyright law, and remains the property of nymea GmbH. All rights, including
* reproduction, publication, editing and translation, are reserved. The use of
* this project is subject to the terms of a license agreement to be concluded
* with nymea GmbH in accordance with the terms of use of nymea GmbH, available
* under https://nymea.io/license
*
* GNU Lesser General Public License Usage
* Alternatively, this project may be redistributed and/or modified under the
* terms of the GNU Lesser General Public License as published by the Free
* Software Foundation; version 3. This project is distributed in the hope that
* it will be useful, but WITHOUT ANY WARRANTY; without even the implied
* warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
* Lesser General Public License for more details.
*
* You should have received a copy of the GNU Lesser General Public License
* along with this project. If not, see <https://www.gnu.org/licenses/>.
*
* For any further details and any questions please contact us under
* contact@nymea.io or see our FAQ/Licensing Information on
* https://nymea.io/license/faq
*
* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

/*!
    \page ws2812fx.html
    \title WS2812FX Control
    \brief Plug-In to control WS2812FX over USB

    \ingroup plugins
    \ingroup nymea-plugins

    \chapter Plugin properties
    Following JSON file contains the definition and the description of all available \l{ThingClass}{DeviceClasses}
    and \l{Vendor}{Vendors} of this \l{DevicePlugin}.

    For more details how to read this JSON file please check out the documentation for \l{The plugin JSON File}.

    \quotefile plugins/deviceplugins/ws2812fx/devicepluginws2812fx.json
*/

#include <QColor>
#include "integrationpluginws2812fx.h"
#include "plugininfo.h"

IntegrationPluginWs2812fx ::IntegrationPluginWs2812fx ()
{
}

void IntegrationPluginWs2812fx::setupThing(ThingSetupInfo *info)
{
    Thing *thing = info->thing();

    QString interface = thing->paramValue(ws2812fxThingSerialPortParamTypeId).toString();

    if (m_usedInterfaces.contains(interface)) {
        info->finish(Thing::ThingErrorHardwareNotAvailable, QT_TR_NOOP("This serial port is already used."));
        return;
    }

    QSerialPort *serialPort = new QSerialPort(interface, this);

    serialPort->setBaudRate(115200);
    serialPort->setDataBits(QSerialPort::DataBits::Data8);
    serialPort->setParity(QSerialPort::Parity::NoParity);
    serialPort->setStopBits(QSerialPort::StopBits::OneStop);
    serialPort->setFlowControl(QSerialPort::FlowControl::NoFlowControl);

    if (!serialPort->open(QIODevice::ReadWrite)) {
        qCWarning(dcWs2812fx()) << "Could not open serial port" << interface << serialPort->errorString();
        serialPort->deleteLater();
        return info->finish(Thing::ThingErrorHardwareFailure, QT_TR_NOOP("Error opening serial port."));
    }

#if QT_VERSION >= QT_VERSION_CHECK(5, 8, 0)
    connect(serialPort, &QSerialPort::errorOccurred, this, &IntegrationPluginWs2812fx::onSerialError);
#else
    connect(serialPort, SIGNAL(error(QSerialPort::SerialPortError)), this, SLOT(onSerialError(QSerialPort::SerialPortError)));
#endif
    connect(serialPort, &QSerialPort::readyRead, this, &IntegrationPluginWs2812fx::onReadyRead);

    qCDebug(dcWs2812fx()) << "Setup successfully serial port" << interface;
    thing->setStateValue(ws2812fxConnectedStateTypeId, true);
    m_usedInterfaces.append(interface);
    m_serialPorts.insert(thing, serialPort);

    if(!m_reconnectTimer) {
        m_reconnectTimer = new QTimer(this);
        m_reconnectTimer->setSingleShot(true);
        m_reconnectTimer->setInterval(5000);

        connect(m_reconnectTimer, &QTimer::timeout, this, &IntegrationPluginWs2812fx::onReconnectTimer);
    }

    info->finish(Thing::ThingErrorNoError);
}


void IntegrationPluginWs2812fx::discoverThings(ThingDiscoveryInfo *info)
{
    // Create the list of available serial interfaces

    Q_FOREACH(QSerialPortInfo port, QSerialPortInfo::availablePorts()) {

        qCDebug(dcWs2812fx()) << "Found serial port:" << port.portName();
        QString description = port.manufacturer() + " " + port.description();
        ThingDescriptor descriptor(info->thingClassId(), port.portName(), description);
        foreach (Thing *existingThing, myThings().filterByParam(ws2812fxThingSerialPortParamTypeId, port.portName())) {
            descriptor.setThingId(existingThing->id());
        }
        ParamList parameters;
        parameters.append(Param(ws2812fxThingSerialPortParamTypeId, port.portName()));
        descriptor.setParams(parameters);
        info->addThingDescriptor(descriptor);
    }
    info->finish(Thing::ThingErrorNoError);
}


void IntegrationPluginWs2812fx::executeAction(ThingActionInfo *info)
{
    Thing *thing = info->thing();
    Action action = info->action();


    QByteArray command;
    if (action.actionTypeId() == ws2812fxPowerActionTypeId) {
        command.append("b ");
        if (action.param(ws2812fxPowerActionPowerParamTypeId).value().toBool()) {
            command.append("30");
        } else {
            command.append("0");
        }
        command.append("\r\n");
        return sendCommand(info, command, CommandType::Brightness);
    }

    if (action.actionTypeId() == ws2812fxBrightnessActionTypeId) {

        command.append("b ");
        command.append(action.param(ws2812fxBrightnessActionBrightnessParamTypeId).value().toString().toUtf8());
        command.append("\r\n");
        return sendCommand(info, command, CommandType::Brightness);
    }

    if (action.actionTypeId() == ws2812fxSpeedActionTypeId) {

        command.append("s ");
        command.append(action.param(ws2812fxSpeedActionSpeedParamTypeId).value().toString().toUtf8());
        command.append("\r\n");
        return sendCommand(info, command, CommandType::Speed);
    }

    if (action.actionTypeId() == ws2812fxColorActionTypeId) {

        QColor color;
        color= action.param(ws2812fxColorActionColorParamTypeId).value().value<QColor>();
        command.append("c ");
        command.append(QString(color.name()).remove("#").toUtf8());
        command.append("\r\n");
        return sendCommand(info, command, CommandType::Color);
    }

    if (action.actionTypeId() == ws2812fxColorTemperatureActionTypeId) {

        // minValue 153, maxValue 500
        QColor color;
        color.setRgb(255, 255, static_cast<int>((255.00-(((action.param(ws2812fxColorTemperatureActionColorTemperatureParamTypeId).value().toDouble()-153.00)/347.00))*255.00)));
        thing->setStateValue(ws2812fxColorTemperatureStateTypeId, action.param(ws2812fxColorTemperatureActionColorTemperatureParamTypeId).value());
        command.append("c ");
        command.append(QString(color.name()).remove("#").toUtf8());
        command.append("\r\n");
        return sendCommand(info, command, CommandType::Color);
    }

    if (action.actionTypeId() == ws2812fxEffectModeActionTypeId) {

        QString effectMode = action.param(ws2812fxEffectModeActionEffectModeParamTypeId).value().toString();
        command.append("m ");
        if (effectMode == "Static") {
            command.append(QString::number(FX_MODE_STATIC).toUtf8());
        } else if (effectMode == "Blink") {
            command.append(QString::number(FX_MODE_BLINK).toUtf8());
        } else if (effectMode == "Color Wipe") {
            command.append(QString::number(FX_MODE_COLOR_WIPE).toUtf8());
        } else if (effectMode == "Color Wipe Inverse") {
            command.append(QString::number(FX_MODE_COLOR_WIPE_INV).toUtf8());
        } else if (effectMode == "Color Wipe Reverse") {
            command.append(QString::number(FX_MODE_COLOR_WIPE_REV).toUtf8());
        } else if (effectMode == "Color Wipe Reverse Inverse") {
            command.append(QString::number(FX_MODE_COLOR_WIPE_REV_INV).toUtf8());
        } else if (effectMode == "Color Wipe Random") {
            command.append(QString::number(FX_MODE_COLOR_WIPE_RANDOM).toUtf8());
        } else if (effectMode == "Random Color") {
            command.append(QString::number(FX_MODE_RANDOM_COLOR).toUtf8());
        } else if (effectMode == "Single Dynamic") {
            command.append(QString::number(FX_MODE_SINGLE_DYNAMIC).toUtf8());
        } else if (effectMode == "Multi Dynamic") {
            command.append(QString::number(FX_MODE_MULTI_DYNAMIC).toUtf8());
        } else if (effectMode == "Rainbow") {
            command.append(QString::number(FX_MODE_RAINBOW).toUtf8());
        } else if (effectMode == "Rainbow Cycle") {
            command.append(QString::number(FX_MODE_RAINBOW_CYCLE).toUtf8());
        } else if (effectMode == "Scan") {
            command.append(QString::number(FX_MODE_SCAN).toUtf8());
        } else if (effectMode == "Dual Scan") {
            command.append(QString::number(FX_MODE_DUAL_SCAN).toUtf8());
        } else if (effectMode == "Fade") {
            command.append(QString::number(FX_MODE_FADE).toUtf8());
        } else if (effectMode == "Theater Chase") {
            command.append(QString::number(FX_MODE_THEATER_CHASE).toUtf8());
        } else if (effectMode == "Theater Chase Rainbow") {
            command.append(QString::number(FX_MODE_THEATER_CHASE_RAINBOW).toUtf8());
        } else if (effectMode == "Running Lights") {
            command.append(QString::number(FX_MODE_RUNNING_LIGHTS).toUtf8());
        } else if (effectMode == "Twinkle") {
            command.append(QString::number(FX_MODE_TWINKLE).toUtf8());
        } else if (effectMode == "Twinkle Random") {
            command.append(QString::number(FX_MODE_TWINKLE_RANDOM).toUtf8());
        } else if (effectMode == "Twinkle Fade") {
            command.append(QString::number(FX_MODE_TWINKLE_FADE).toUtf8());
        } else if (effectMode == "Twinkle Fade Random") {
            command.append(QString::number(FX_MODE_TWINKLE_FADE_RANDOM).toUtf8());
        } else if (effectMode == "Sparkle") {
            command.append(QString::number(FX_MODE_SPARKLE).toUtf8());
        } else if (effectMode == "Flash Sparkle") {
            command.append(QString::number(FX_MODE_FLASH_SPARKLE).toUtf8());
        } else if (effectMode == "Hyper Sparkle") {
            command.append(QString::number(FX_MODE_HYPER_SPARKLE).toUtf8());
        } else if (effectMode == "Strobe") {
            command.append(QString::number(FX_MODE_STROBE).toUtf8());
        } else if (effectMode == "Strobe Rainbow") {
            command.append(QString::number(FX_MODE_STROBE_RAINBOW).toUtf8());
        } else if (effectMode == "Multi Strobe") {
            command.append(QString::number(FX_MODE_MULTI_STROBE).toUtf8());
        } else if (effectMode == "Blink Rainbow") {
            command.append(QString::number(FX_MODE_BLINK_RAINBOW).toUtf8());
        } else if (effectMode == "Chase White") {
            command.append(QString::number(FX_MODE_CHASE_WHITE).toUtf8());
        } else if (effectMode == "Chase Color") {
            command.append(QString::number(FX_MODE_CHASE_COLOR).toUtf8());
        } else if (effectMode == "Chase Random") {
            command.append(QString::number(FX_MODE_CHASE_RANDOM).toUtf8());
        } else if (effectMode == "Chase Flash") {
            command.append(QString::number(FX_MODE_CHASE_FLASH).toUtf8());
        } else if (effectMode == "Chase Flash Random") {
            command.append(QString::number(FX_MODE_CHASE_FLASH_RANDOM).toUtf8());
        } else if (effectMode == "Chase Rainbow White") {
            command.append(QString::number(FX_MODE_CHASE_RAINBOW_WHITE).toUtf8());
        } else if (effectMode == "Chase Blackout") {
            command.append(QString::number(FX_MODE_CHASE_BLACKOUT).toUtf8());
        } else if (effectMode == "Chase Blackout Rainbow") {
            command.append(QString::number(FX_MODE_CHASE_BLACKOUT_RAINBOW).toUtf8());
        } else if (effectMode == "Color Sweep Random") {
            command.append(QString::number(FX_MODE_COLOR_SWEEP_RANDOM).toUtf8());
        } else if (effectMode == "Running Color") {
            command.append(QString::number(FX_MODE_RUNNING_COLOR).toUtf8());
        } else if (effectMode == "Running Red Blue") {
            command.append(QString::number(FX_MODE_RUNNING_RED_BLUE).toUtf8());
        } else if (effectMode == "Running Random") {
            command.append(QString::number(FX_MODE_RUNNING_RANDOM).toUtf8());
        }else if (effectMode == "Larson Scanner") {
            command.append(QString::number(FX_MODE_LARSON_SCANNER).toUtf8());
        }else if (effectMode == "Comet") {
            command.append(QString::number(FX_MODE_COMET).toUtf8());
        }else if (effectMode == "Fireworks") {
            command.append(QString::number(FX_MODE_FIREWORKS).toUtf8());
        }else if (effectMode == "Fireworks Random") {
            command.append(QString::number(FX_MODE_FIREWORKS_RANDOM).toUtf8());
        }else if (effectMode == "Merry Christmas") {
            command.append(QString::number(FX_MODE_MERRY_CHRISTMAS).toUtf8());
        }else if (effectMode == "Fire Flicker") {
            command.append(QString::number(FX_MODE_FIRE_FLICKER).toUtf8());
        }else if (effectMode == "Fire Flicker (soft)") {
            command.append(QString::number(FX_MODE_FIRE_FLICKER_SOFT).toUtf8());
        }else if (effectMode == "Fire Flicker (intense)") {
            command.append(QString::number(FX_MODE_FIRE_FLICKER_INTENSE).toUtf8());
        }else if (effectMode == "Circus Combustus") {
            command.append(QString::number(FX_MODE_CIRCUS_COMBUSTUS).toUtf8());
        }else if (effectMode == "Halloween") {
            command.append(QString::number(FX_MODE_HALLOWEEN).toUtf8());
        }else if (effectMode == "Bicolor Chase") {
            command.append(QString::number(FX_MODE_BICOLOR_CHASE).toUtf8());
        }else if (effectMode == "Tricolor Chase") {
            command.append(QString::number(FX_MODE_TRICOLOR_CHASE).toUtf8());
        }else if (effectMode == "ICU") {
            command.append(QString::number(FX_MODE_ICU).toUtf8());
        }else if (effectMode == "Custom 0") {
            command.append(QString::number(FX_MODE_CUSTOM_0).toUtf8());
        }else if (effectMode == "Custom 1") {
            command.append(QString::number(FX_MODE_CUSTOM_1).toUtf8());
        }else if (effectMode == "Custom 2") {
            command.append(QString::number(FX_MODE_CUSTOM_2).toUtf8());
        }else if (effectMode == "Custom 3") {
            command.append(QString::number(FX_MODE_CUSTOM_3).toUtf8());
        }
        command.append("\r\n");
        return sendCommand(info, command, CommandType::Mode);
    }
}


void IntegrationPluginWs2812fx::thingRemoved(Thing *thing)
{
    if (thing->thingClassId() == ws2812fxThingClassId) {

        m_usedInterfaces.removeAll(thing->paramValue(ws2812fxThingSerialPortParamTypeId).toString());
        QSerialPort *serialPort = m_serialPorts.take(thing);
        serialPort->flush();
        serialPort->close();
        serialPort->deleteLater();
    }

    if (myThings().empty()) {
        m_reconnectTimer->stop();
        m_reconnectTimer->deleteLater();
    }
}

void IntegrationPluginWs2812fx::onReadyRead()
{
    QSerialPort *serialPort =  static_cast<QSerialPort*>(sender());
    Thing *thing = m_serialPorts.key(serialPort);

    QByteArray data;
    while (serialPort->canReadLine()) {
        data = serialPort->readLine();
        qCDebug(dcWs2812fx()) << "Message received" << data;

        if (data.contains("mode")) {
            if (m_pendingActions.contains(CommandType::Mode)) {
                m_pendingActions.take(CommandType::Mode)->finish(Thing::ThingErrorNoError);
            }
            QString mode = data.split('-').at(1);
            mode.remove(0, 1);
            mode.remove("\r\n");
            qCDebug(dcWs2812fx()) << "set mode to:" << mode;
            thing->setStateValue(ws2812fxEffectModeStateTypeId, mode);
        }
        if (data.contains("brightness")) {
            if (m_pendingActions.contains(CommandType::Brightness)) {
                m_pendingActions.take(CommandType::Brightness)->finish(Thing::ThingErrorNoError);
            }
            QString rawBrightness = data.split(':').at(1);
            rawBrightness.remove(" ");
            rawBrightness.remove("\r\n");
            int brightness = rawBrightness.toInt();

            qCDebug(dcWs2812fx()) << "set brightness to:" << brightness;
            thing->setStateValue(ws2812fxBrightnessStateTypeId, brightness);
            if (brightness == 0) {
                thing->setStateValue(ws2812fxPowerStateTypeId, false);
            } else {
                thing->setStateValue(ws2812fxPowerStateTypeId, true);
            }
        }
        if (data.contains("speed")) {
            if (m_pendingActions.contains(CommandType::Speed)) {
                m_pendingActions.take(CommandType::Speed)->finish(Thing::ThingErrorNoError);
            }
            QString rawSpeed = data.split(':').at(1);
            rawSpeed.remove(" ");
            rawSpeed.remove("\r\n");
            int speed = data.split(':').at(1).toInt();

            qCDebug(dcWs2812fx()) << "set speed to:" << speed;
            thing->setStateValue(ws2812fxSpeedStateTypeId, speed);
        }
        if (data.contains("color")) {
            if (m_pendingActions.contains(CommandType::Color)) {
                m_pendingActions.take(CommandType::Color)->finish(Thing::ThingErrorNoError);
            }
            QString rawColor = data.split(':').at(1);
            rawColor.remove(" ");
            rawColor.remove("0x");
            rawColor.remove("\r\n");
            rawColor.prepend("#");
            qCDebug(dcWs2812fx()) << "set color to:" << rawColor;
            thing->setStateValue(ws2812fxColorStateTypeId, rawColor);
        }
    }
}

void IntegrationPluginWs2812fx::onReconnectTimer()
{
    foreach(Thing *thing, myThings()) {
        if (!thing->stateValue(ws2812fxConnectedStateTypeId).toBool()) {
            QSerialPort *serialPort =  m_serialPorts.value(thing);
            if (serialPort) {
                if (serialPort->open(QSerialPort::ReadWrite)) {
                    thing->setStateValue(ws2812fxConnectedStateTypeId, true);
                } else {
                    thing->setStateValue(ws2812fxConnectedStateTypeId, false);
                    m_reconnectTimer->start();
                }
            }
        }
    }
}

void IntegrationPluginWs2812fx::onSerialError(QSerialPort::SerialPortError error)
{
    QSerialPort *serialPort =  static_cast<QSerialPort*>(sender());
    Thing *thing = m_serialPorts.key(serialPort);

    if (error != QSerialPort::NoError && serialPort->isOpen()) {
        qCCritical(dcWs2812fx()) << "Serial port error:" << error << serialPort->errorString();
        m_reconnectTimer->start();
        serialPort->close();
        thing->setStateValue(ws2812fxConnectedStateTypeId, false);
    }
}

void IntegrationPluginWs2812fx::sendCommand(ThingActionInfo *info, const QByteArray &command, CommandType commandType)
{
    qCDebug(dcWs2812fx()) << "Sending command" << command;
    QSerialPort *serialPort = m_serialPorts.value(info->thing());
    if (!serialPort)
        return info->finish(Thing::ThingErrorThingNotFound);
    if (serialPort->write(command) != command.length()) {
        qCWarning(dcWs2812fx) << "Error writing to serial port";
        return info->finish(Thing::ThingErrorHardwareFailure);
    }
    m_pendingActions.insert(commandType, info);
}
