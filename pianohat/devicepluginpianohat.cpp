/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *                                                                         *
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

#include "plugininfo.h"
#include "devicepluginpianohat.h"
#include "types/param.h"

DevicePluginPianohat::DevicePluginPianohat()
{

}

void DevicePluginPianohat::init()
{
    // Initialize/create objects
}

void DevicePluginPianohat::startMonitoringAutoDevices()
{
    // Start seaching for devices which can be discovered and added automatically
}

void DevicePluginPianohat::postSetupDevice(Device *device)
{
    qCDebug(dcPianohat()) << "Post setup device" << device->name() << device->params();

    m_pianohat->enable();
}

void DevicePluginPianohat::deviceRemoved(Device *device)
{
    qCDebug(dcPianohat()) << "Remove device" << device->name() << device->params();

    // Clean up all data related to this device
    if (m_pianohat) {
        m_pianohat->disable();
        m_pianohat->deleteLater();
        m_pianohat = nullptr;
    }

    if (m_device) {
        // Note: the DeviceManager will take care about the device Object
        m_device = nullptr;
    }
}

DeviceManager::DeviceSetupStatus DevicePluginPianohat::setupDevice(Device *device)
{
    qCDebug(dcPianohat()) << "Setup device" << device->name() << device->params();

    // Make sure there is not already a pianohat added, since only one device is supported
    if (m_pianohat) {
        qCWarning(dcPianohat()) << "There has already been setup a PianoHat. Only one instance of this device is allowed.";
        return DeviceManager::DeviceSetupStatusFailure;
    }

    // Create the PianoHat object and store the device pointer (there can always be only one pianohat)
    m_device = device;
    m_pianohat = new PianoHat(this);
    connect(m_pianohat, &PianoHat::keyPressed, this, &DevicePluginPianohat::onPianohatKeyPressed);

    return DeviceManager::DeviceSetupStatusSuccess;
}

DeviceManager::DeviceError DevicePluginPianohat::executeAction(Device *device, const Action &action)
{
    qCDebug(dcPianohat()) << "Executing action for device" << device->name() << action.actionTypeId().toString() << action.params();

    return DeviceManager::DeviceErrorNoError;
}

void DevicePluginPianohat::onPianohatKeyPressed(const PianoHat::Key &key, bool pressed)
{
    qCDebug(dcPianohat()) << key << (pressed ? "pressed" : "released");

    // Emit the key pressed event on pressed and set the bool states
    ParamList eventParams;
    switch (key) {
    case PianoHat::KeyC:
        eventParams.append(Param(pianohatButtonNameParamTypeId, "C"));
        m_device->setStateValue(pianohatCStateTypeId, pressed);
        break;
    case PianoHat::KeyCSharp:
        eventParams.append(Param(pianohatButtonNameParamTypeId, "C#"));
        m_device->setStateValue(pianohatCSharpStateTypeId, pressed);
        break;
    case PianoHat::KeyD:
        eventParams.append(Param(pianohatButtonNameParamTypeId, "D"));
        m_device->setStateValue(pianohatDStateTypeId, pressed);
        break;
    case PianoHat::KeyDSharp:
        eventParams.append(Param(pianohatButtonNameParamTypeId, "D#"));
        m_device->setStateValue(pianohatDSharpStateTypeId, pressed);
        break;
    case PianoHat::KeyE:
        eventParams.append(Param(pianohatButtonNameParamTypeId, "E"));
        m_device->setStateValue(pianohatEStateTypeId, pressed);
        break;
    case PianoHat::KeyF:
        eventParams.append(Param(pianohatButtonNameParamTypeId, "F#"));
        m_device->setStateValue(pianohatFStateTypeId, pressed);
        break;
    case PianoHat::KeyG:
        eventParams.append(Param(pianohatButtonNameParamTypeId, "G"));
        m_device->setStateValue(pianohatGSharpStateTypeId, pressed);
        break;
    case PianoHat::KeyGSharp:
        eventParams.append(Param(pianohatButtonNameParamTypeId, "G#"));
        m_device->setStateValue(pianohatGSharpStateTypeId, pressed);
        break;
    case PianoHat::KeyA:
        eventParams.append(Param(pianohatButtonNameParamTypeId, "A"));
        m_device->setStateValue(pianohatAStateTypeId, pressed);
        break;
    case PianoHat::KeyASharp:
        eventParams.append(Param(pianohatButtonNameParamTypeId, "A#"));
        m_device->setStateValue(pianohatASharpStateTypeId, pressed);
        break;
    case PianoHat::KeyB:
        eventParams.append(Param(pianohatButtonNameParamTypeId, "B"));
        m_device->setStateValue(pianohatBStateTypeId, pressed);
        break;
    case PianoHat::KeyHighC:
        eventParams.append(Param(pianohatButtonNameParamTypeId, "C (high)"));
        m_device->setStateValue(pianohatCHighStateTypeId, pressed);
        break;
    case PianoHat::KeyOctaveDown:
        eventParams.append(Param(pianohatButtonNameParamTypeId, "Octave down"));
        m_device->setStateValue(pianohatOctaveDownStateTypeId, pressed);
        break;
    case PianoHat::KeyOctaveUp:
        eventParams.append(Param(pianohatButtonNameParamTypeId, "Octave up"));
        m_device->setStateValue(pianohatOctaveUpStateTypeId, pressed);
        break;
    case PianoHat::KeyInstrument:
        eventParams.append(Param(pianohatButtonNameParamTypeId, "Instrument"));
        m_device->setStateValue(pianohatInstrumentStateTypeId, pressed);
        break;
    default:
        break;
    }

    // Only emit the pressed event if the button was touched
    if (!eventParams.isEmpty() && pressed)
        emitEvent(Event(pianohatPressedEventTypeId, m_device->id(), eventParams));

}
