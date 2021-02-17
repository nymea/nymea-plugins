/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*
* Copyright 2013 - 2020, nymea GmbH
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
s
    \quotefile plugins/deviceplugins/ws2812fx/devicepluginws2812fx.json
*/
#include <QColor>
#include <QRgb>

#include "integrationpluginws2812fx.h"
#include "plugininfo.h"

#include "nymealightserialinterface.h"

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

    NymeaLightSerialInterface *lightInterface = new NymeaLightSerialInterface(interface, thing);
    NymeaLight *light = new NymeaLight(lightInterface, this);
    lightInterface->setParent(light);

    if (!lightInterface->open()) {
        qCWarning(dcWs2812fx()) << "Could not open interface" << interface;
        light->deleteLater();
        info->finish(Thing::ThingErrorHardwareFailure, QT_TR_NOOP("Error opening serial port."));
        return;
    }

    connect(light, &NymeaLight::availableChanged, thing, [=](bool available){
        qCDebug(dcWs2812fx()) << thing << "available changed" << available;
        thing->setStateValue(ws2812fxConnectedStateTypeId, available);
    });

    qCDebug(dcWs2812fx()) << "Setup successfully serial port" << interface;
    thing->setStateValue(ws2812fxConnectedStateTypeId, true);
    m_usedInterfaces.append(interface);
    m_lights.insert(thing, light);
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
    NymeaLight *light = m_lights.value(thing);
    if (!light || !light->available()) {
        info->finish(Thing::ThingErrorHardwareNotAvailable);
        return;
    }

    if (action.actionTypeId() == ws2812fxPowerActionTypeId) {
        bool power = action.param(ws2812fxPowerActionPowerParamTypeId).value().toBool();
        quint8 brightness = 0;
        if (power) {
            quint8 brightnessPercentage = thing->stateValue(ws2812fxBrightnessStateTypeId).toUInt();
            brightness = qRound(255.0 * brightnessPercentage / 100);
        }

        qCDebug(dcWs2812fx()) << "Set power" << power;
        NymeaLightInterfaceReply *reply = light->setBrightness(brightness, 500);
        connect(info, &ThingActionInfo::aborted, reply, &NymeaLightInterfaceReply::finished);
        connect(reply, &NymeaLightInterfaceReply::finished, this, [=](){
            if (reply->status() != NymeaLightInterface::StatusSuccess) {
                info->finish(Thing::ThingErrorHardwareFailure);
                return;
            }

            qCDebug(dcWs2812fx()) << "Set power finished successfully" << power;
            thing->setStateValue(ws2812fxPowerStateTypeId, power);
            info->finish(Thing::ThingErrorNoError);
        });
    } else if (action.actionTypeId() == ws2812fxColorActionTypeId) {
        QColor color = action.param(ws2812fxColorActionColorParamTypeId).value().value<QColor>();
        qCDebug(dcWs2812fx()) << "Set color to" << color.name(QColor::HexRgb);
        NymeaLightInterfaceReply *reply = light->setColor(color);
        connect(info, &ThingActionInfo::aborted, reply, &NymeaLightInterfaceReply::finished);
        connect(reply, &NymeaLightInterfaceReply::finished, this, [=](){
            if (reply->status() != NymeaLightInterface::StatusSuccess) {
                info->finish(Thing::ThingErrorHardwareFailure);
                return;
            }
            qCDebug(dcWs2812fx()) << "Set color finished successfully" << color.name(QColor::HexRgb);
            thing->setStateValue(ws2812fxColorStateTypeId, color);
            info->finish(Thing::ThingErrorNoError);
        });
    } else if (action.actionTypeId() == ws2812fxColorTemperatureActionTypeId) {
        // minValue 153, maxValue 500
        uint colorTemperature = action.param(ws2812fxColorTemperatureActionColorTemperatureParamTypeId).value().toDouble();
        QColor color;
        color.setRgb(255, 255, qRound((255.00 - (((colorTemperature - 153.00) / 347.00)) * 255.00)));

        qCDebug(dcWs2812fx()) << "Set color temperature" << colorTemperature << color.name(QColor::HexRgb);
        NymeaLightInterfaceReply *reply = light->setColor(color);
        connect(info, &ThingActionInfo::aborted, reply, &NymeaLightInterfaceReply::finished);
        connect(reply, &NymeaLightInterfaceReply::finished, this, [=](){
            if (reply->status() != NymeaLightInterface::StatusSuccess) {
                info->finish(Thing::ThingErrorHardwareFailure);
                return;
            }

            qCDebug(dcWs2812fx()) << "Set color temperature finished successfully" << colorTemperature;
            thing->setStateValue(ws2812fxColorTemperatureStateTypeId, colorTemperature);
            info->finish(Thing::ThingErrorNoError);
        });
    } else if (action.actionTypeId() == ws2812fxBrightnessActionTypeId) {
        quint8 brightnessPercentage = action.param(ws2812fxBrightnessActionBrightnessParamTypeId).value().toUInt();
        quint8 brightness = qRound(255.0 * brightnessPercentage / 100);
        qCDebug(dcWs2812fx()) << "Set brightness to" << brightnessPercentage << brightness;
        NymeaLightInterfaceReply *reply = light->setBrightness(brightness, 1000);
        connect(info, &ThingActionInfo::aborted, reply, &NymeaLightInterfaceReply::finished);
        connect(reply, &NymeaLightInterfaceReply::finished, this, [=](){
            if (reply->status() != NymeaLightInterface::StatusSuccess) {
                info->finish(Thing::ThingErrorHardwareFailure);
                return;
            }

            qCDebug(dcWs2812fx()) << "Set brightness finished successfully" << brightness;
            thing->setStateValue(ws2812fxBrightnessStateTypeId, brightnessPercentage);
            info->finish(Thing::ThingErrorNoError);
        });
    } else if (action.actionTypeId() == ws2812fxSpeedActionTypeId) {
        quint16 speedPercentage = action.param(ws2812fxSpeedActionSpeedParamTypeId).value().toUInt();
        quint16 speed = 2000 - (speedPercentage * 20);

        qCDebug(dcWs2812fx()) << "Set speed" << speedPercentage << "%" << speed << "ms";
        NymeaLightInterfaceReply *reply = light->setSpeed(speed);
        connect(info, &ThingActionInfo::aborted, reply, &NymeaLightInterfaceReply::finished);
        connect(reply, &NymeaLightInterfaceReply::finished, this, [=](){
            if (reply->status() != NymeaLightInterface::StatusSuccess) {
                info->finish(Thing::ThingErrorHardwareFailure);
                return;
            }

            qCDebug(dcWs2812fx()) << "Set speed finished successfully" << speedPercentage << "%" << speed << "ms";
            thing->setStateValue(ws2812fxSpeedStateTypeId, speedPercentage);
            info->finish(Thing::ThingErrorNoError);
        });
    } else if (action.actionTypeId() == ws2812fxEffectModeActionTypeId) {
        QString effectMode = action.param(ws2812fxEffectModeActionEffectModeParamTypeId).value().toString();
        quint8 mode = FX_MODE_STATIC;
        if (effectMode == "Static") {
            mode = FX_MODE_STATIC;
        } else if (effectMode == "Blink") {
            mode = FX_MODE_BLINK;
        } else if (effectMode == "Color Wipe") {
            mode = FX_MODE_COLOR_WIPE;
        } else if (effectMode == "Color Wipe Inverse") {
            mode = FX_MODE_COLOR_WIPE_INV;
        } else if (effectMode == "Color Wipe Reverse") {
            mode = FX_MODE_COLOR_WIPE_REV;
        } else if (effectMode == "Color Wipe Reverse Inverse") {
            mode = FX_MODE_COLOR_WIPE_REV_INV;
        } else if (effectMode == "Color Wipe Random") {
            mode = FX_MODE_COLOR_WIPE_RANDOM;
        } else if (effectMode == "Random Color") {
            mode = FX_MODE_RANDOM_COLOR;
        } else if (effectMode == "Single Dynamic") {
            mode = FX_MODE_SINGLE_DYNAMIC;
        } else if (effectMode == "Multi Dynamic") {
            mode = FX_MODE_MULTI_DYNAMIC;
        } else if (effectMode == "Rainbow") {
            mode = FX_MODE_RAINBOW;
        } else if (effectMode == "Rainbow Cycle") {
            mode = FX_MODE_RAINBOW_CYCLE;
        } else if (effectMode == "Scan") {
            mode = FX_MODE_SCAN;
        } else if (effectMode == "Dual Scan") {
            mode = FX_MODE_DUAL_SCAN;
        } else if (effectMode == "Fade") {
            mode = FX_MODE_FADE;
        } else if (effectMode == "Theater Chase") {
            mode = FX_MODE_THEATER_CHASE;
        } else if (effectMode == "Theater Chase Rainbow") {
            mode = FX_MODE_THEATER_CHASE_RAINBOW;
        } else if (effectMode == "Running Lights") {
            mode = FX_MODE_RUNNING_LIGHTS;
        } else if (effectMode == "Twinkle") {
            mode = FX_MODE_TWINKLE;
        } else if (effectMode == "Twinkle Random") {
            mode = FX_MODE_TWINKLE_RANDOM;
        } else if (effectMode == "Twinkle Fade") {
            mode = FX_MODE_TWINKLE_FADE;
        } else if (effectMode == "Twinkle Fade Random") {
            mode = FX_MODE_TWINKLE_FADE_RANDOM;
        } else if (effectMode == "Sparkle") {
            mode = FX_MODE_SPARKLE;
        } else if (effectMode == "Flash Sparkle") {
            mode = FX_MODE_FLASH_SPARKLE;
        } else if (effectMode == "Hyper Sparkle") {
            mode = FX_MODE_HYPER_SPARKLE;
        } else if (effectMode == "Strobe") {
            mode = FX_MODE_STROBE;
        } else if (effectMode == "Strobe Rainbow") {
            mode = FX_MODE_STROBE_RAINBOW;
        } else if (effectMode == "Multi Strobe") {
            mode = FX_MODE_MULTI_STROBE;
        } else if (effectMode == "Blink Rainbow") {
            mode = FX_MODE_BLINK_RAINBOW;
        } else if (effectMode == "Chase White") {
            mode = FX_MODE_CHASE_WHITE;
        } else if (effectMode == "Chase Color") {
            mode = FX_MODE_CHASE_COLOR;
        } else if (effectMode == "Chase Random") {
            mode = FX_MODE_CHASE_RANDOM;
        } else if (effectMode == "Chase Flash") {
            mode = FX_MODE_CHASE_FLASH;
        } else if (effectMode == "Chase Flash Random") {
            mode = FX_MODE_CHASE_FLASH_RANDOM;
        } else if (effectMode == "Chase Rainbow White") {
            mode = FX_MODE_CHASE_RAINBOW_WHITE;
        } else if (effectMode == "Chase Blackout") {
            mode = FX_MODE_CHASE_BLACKOUT;
        } else if (effectMode == "Chase Blackout Rainbow") {
            mode = FX_MODE_CHASE_BLACKOUT_RAINBOW;
        } else if (effectMode == "Color Sweep Random") {
            mode = FX_MODE_COLOR_SWEEP_RANDOM;
        } else if (effectMode == "Running Color") {
            mode = FX_MODE_RUNNING_COLOR;
        } else if (effectMode == "Running Red Blue") {
            mode = FX_MODE_RUNNING_RED_BLUE;
        } else if (effectMode == "Running Random") {
            mode = FX_MODE_RUNNING_RANDOM;
        }else if (effectMode == "Larson Scanner") {
            mode = FX_MODE_LARSON_SCANNER;
        }else if (effectMode == "Comet") {
            mode = FX_MODE_COMET;
        }else if (effectMode == "Fireworks") {
            mode = FX_MODE_FIREWORKS;
        }else if (effectMode == "Fireworks Random") {
            mode = FX_MODE_FIREWORKS_RANDOM;
        }else if (effectMode == "Merry Christmas") {
            mode = FX_MODE_MERRY_CHRISTMAS;
        }else if (effectMode == "Fire Flicker") {
            mode = FX_MODE_FIRE_FLICKER;
        }else if (effectMode == "Fire Flicker (soft)") {
            mode = FX_MODE_FIRE_FLICKER_SOFT;
        }else if (effectMode == "Fire Flicker (intense)") {
            mode = FX_MODE_FIRE_FLICKER_INTENSE;
        }else if (effectMode == "Circus Combustus") {
            mode = FX_MODE_CIRCUS_COMBUSTUS;
        }else if (effectMode == "Halloween") {
            mode = FX_MODE_HALLOWEEN;
        }else if (effectMode == "Bicolor Chase") {
            mode = FX_MODE_BICOLOR_CHASE;
        }else if (effectMode == "Tricolor Chase") {
            mode = FX_MODE_TRICOLOR_CHASE;
        }else if (effectMode == "ICU") {
            mode = FX_MODE_ICU;
        }else if (effectMode == "Custom 0") {
            mode = FX_MODE_CUSTOM_0;
        }else if (effectMode == "Custom 1") {
            mode = FX_MODE_CUSTOM_1;
        }else if (effectMode == "Custom 2") {
            mode = FX_MODE_CUSTOM_2;
        }else if (effectMode == "Custom 3") {
            mode = FX_MODE_CUSTOM_3;
        }

        qCDebug(dcWs2812fx()) << "Set mode" << effectMode << mode;
        NymeaLightInterfaceReply *reply = light->setEffect(mode);
        connect(info, &ThingActionInfo::aborted, reply, &NymeaLightInterfaceReply::finished);
        connect(reply, &NymeaLightInterfaceReply::finished, this, [=](){
            if (reply->status() != NymeaLightInterface::StatusSuccess) {
                info->finish(Thing::ThingErrorHardwareFailure);
                return;
            }

            qCDebug(dcWs2812fx()) << "Set mode finished successfully" << effectMode << mode;
            thing->setStateValue(ws2812fxEffectModeStateTypeId, effectMode);
            info->finish(Thing::ThingErrorNoError);
        });
    }
}


void IntegrationPluginWs2812fx::thingRemoved(Thing *thing)
{
    if (thing->thingClassId() == ws2812fxThingClassId) {

        m_usedInterfaces.removeAll(thing->paramValue(ws2812fxThingSerialPortParamTypeId).toString());
        NymeaLight *light = m_lights.take(thing);
        light->deleteLater();
    }
}

