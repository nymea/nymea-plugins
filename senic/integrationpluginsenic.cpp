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

#include "integrationpluginsenic.h"
#include "integrations/thing.h"
#include "plugininfo.h"
#include "hardware/bluetoothlowenergy/bluetoothlowenergymanager.h"

IntegrationPluginSenic::IntegrationPluginSenic()
{

}

void IntegrationPluginSenic::init()
{
    // Initialize plugin configurations
    m_autoSymbolMode = configValue(senicPluginAutoSymbolsParamTypeId).toBool();
    connect(this, &IntegrationPluginSenic::configValueChanged, this, &IntegrationPluginSenic::onPluginConfigurationChanged);
}


void IntegrationPluginSenic::discoverThings(ThingDiscoveryInfo *info)
{
    if (!hardwareManager()->bluetoothLowEnergyManager()->available())
        return info->finish(Thing::ThingErrorHardwareNotAvailable, QT_TR_NOOP("Bluetooth is not available on this system."));

    if (!hardwareManager()->bluetoothLowEnergyManager()->enabled())
        return info->finish(Thing::ThingErrorHardwareNotAvailable, QT_TR_NOOP("Bluetooth is disabled. Please enable Bluetooth and try again."));

    BluetoothDiscoveryReply *reply = hardwareManager()->bluetoothLowEnergyManager()->discoverDevices();
    connect(reply, &BluetoothDiscoveryReply::finished, reply, &BluetoothDiscoveryReply::deleteLater);

    connect(reply, &BluetoothDiscoveryReply::finished, info, [this, info, reply](){

        if (reply->error() != BluetoothDiscoveryReply::BluetoothDiscoveryReplyErrorNoError) {
            qCWarning(dcSenic()) << "Bluetooth discovery error:" << reply->error();
            info->finish(Thing::ThingErrorHardwareFailure, QT_TR_NOOP("An error happened during Bluetooth discovery."));
            return;
        }

        foreach (const QBluetoothDeviceInfo &deviceInfo, reply->discoveredDevices()) {
            if (deviceInfo.name().contains("Nuimo")) {
                ThingDescriptor descriptor(nuimoThingClassId, "Nuimo", deviceInfo.name() + " (" + deviceInfo.address().toString() + ")");
                ParamList params;

                foreach (Thing *existingThing, myThings()) {
                    if (existingThing->paramValue(nuimoThingMacParamTypeId).toString() == deviceInfo.address().toString()) {
                        descriptor.setThingId(existingThing->id());
                        break;
                    }
                }
                params.append(Param(nuimoThingMacParamTypeId, deviceInfo.address().toString()));
                descriptor.setParams(params);
                info->addThingDescriptor(descriptor);
            }
        }
        info->finish(Thing::ThingErrorNoError);
    });
}


void IntegrationPluginSenic::setupThing(ThingSetupInfo *info)
{
    Thing *thing = info->thing();

    qCDebug(dcSenic()) << "Setup thing" << thing->name() << thing->params();

    QBluetoothAddress address = QBluetoothAddress(thing->paramValue(nuimoThingMacParamTypeId).toString());
    QBluetoothDeviceInfo deviceInfo = QBluetoothDeviceInfo(address, thing->name(), 0);

    BluetoothLowEnergyDevice *bluetoothDevice = hardwareManager()->bluetoothLowEnergyManager()->registerDevice(deviceInfo, QLowEnergyController::RandomAddress);

    Nuimo *nuimo = new Nuimo(bluetoothDevice, this);
    nuimo->setLongPressTime(configValue(senicPluginLongPressTimeParamTypeId).toInt());
    connect(nuimo, &Nuimo::buttonPressed, this, &IntegrationPluginSenic::onButtonPressed);
    connect(nuimo, &Nuimo::buttonLongPressed, this, &IntegrationPluginSenic::onButtonLongPressed);
    connect(nuimo, &Nuimo::swipeDetected, this, &IntegrationPluginSenic::onSwipeDetected);
    connect(nuimo, &Nuimo::rotationValueChanged, this, &IntegrationPluginSenic::onRotationValueChanged);
    connect(nuimo, &Nuimo::connectedChanged, this, &IntegrationPluginSenic::onConnectedChanged);
    connect(nuimo, &Nuimo::deviceInformationChanged, this, &IntegrationPluginSenic::onDeviceInformationChanged);
    connect(nuimo, &Nuimo::batteryValueChanged, this, &IntegrationPluginSenic::onBatteryValueChanged);

    m_nuimos.insert(nuimo, thing);

    connect(nuimo, &Nuimo::deviceInitializationFinished, info, [this, info, nuimo](bool success){
        Thing *thing = info->thing();

        if (!thing->setupComplete()) {
            if (success) {
                info->finish(Thing::ThingErrorNoError);
            } else {
                m_nuimos.take(nuimo);

                hardwareManager()->bluetoothLowEnergyManager()->unregisterDevice(nuimo->bluetoothDevice());
                nuimo->deleteLater();

                info->finish(Thing::ThingErrorHardwareFailure, QT_TR_NOOP("Error connecting to nuimo."));
            }
        }

    });


    nuimo->bluetoothDevice()->connectDevice();
}

void IntegrationPluginSenic::postSetupThing(Thing *thing)
{
    Q_UNUSED(thing)

    if (!m_reconnectTimer) {
        m_reconnectTimer = hardwareManager()->pluginTimerManager()->registerTimer(10);
        connect(m_reconnectTimer, &PluginTimer::timeout, this, &IntegrationPluginSenic::onReconnectTimeout);
    }
}


void IntegrationPluginSenic::executeAction(ThingActionInfo *info)
{
    Thing *thing = info->thing();
    Action action = info->action();

    QPointer<Nuimo> nuimo = m_nuimos.key(thing);
    if (nuimo.isNull())
        return info->finish(Thing::ThingErrorHardwareFailure);

    if (!nuimo->bluetoothDevice()->connected()) {
        return info->finish(Thing::ThingErrorHardwareNotAvailable);
    }

    if (action.actionTypeId() == nuimoShowLogoActionTypeId) {

        if (action.param(nuimoShowLogoActionLogoParamTypeId).value().toString() == "Up")
            nuimo->showImage(Nuimo::MatrixTypeUp);
        if (action.param(nuimoShowLogoActionLogoParamTypeId).value().toString() == "Down")
            nuimo->showImage(Nuimo::MatrixTypeDown);
        if (action.param(nuimoShowLogoActionLogoParamTypeId).value().toString() == "Left")
            nuimo->showImage(Nuimo::MatrixTypeLeft);
        if (action.param(nuimoShowLogoActionLogoParamTypeId).value().toString() == "Right")
            nuimo->showImage(Nuimo::MatrixTypeRight);
        if (action.param(nuimoShowLogoActionLogoParamTypeId).value().toString() == "Play")
            nuimo->showImage(Nuimo::MatrixTypePlay);
        if (action.param(nuimoShowLogoActionLogoParamTypeId).value().toString() == "Pause")
            nuimo->showImage(Nuimo::MatrixTypePause);
        if (action.param(nuimoShowLogoActionLogoParamTypeId).value().toString() == "Stop")
            nuimo->showImage(Nuimo::MatrixTypeStop);
        if (action.param(nuimoShowLogoActionLogoParamTypeId).value().toString() == "Music")
            nuimo->showImage(Nuimo::MatrixTypeMusic);
        if (action.param(nuimoShowLogoActionLogoParamTypeId).value().toString() == "Heart")
            nuimo->showImage(Nuimo::MatrixTypeHeart);
        if (action.param(nuimoShowLogoActionLogoParamTypeId).value().toString() == "Next")
            nuimo->showImage(Nuimo::MatrixTypeNext);
        if (action.param(nuimoShowLogoActionLogoParamTypeId).value().toString() == "Previous")
            nuimo->showImage(Nuimo::MatrixTypePrevious);
        if (action.param(nuimoShowLogoActionLogoParamTypeId).value().toString() == "Circle")
            nuimo->showImage(Nuimo::MatrixTypeCircle);
        if (action.param(nuimoShowLogoActionLogoParamTypeId).value().toString() == "Light")
            nuimo->showImage(Nuimo::MatrixTypeLight);

        return info->finish(Thing::ThingErrorNoError);
    }
}


void IntegrationPluginSenic::thingRemoved(Thing *thing)
{
    if (!m_nuimos.values().contains(thing))
        return;

    Nuimo *nuimo = m_nuimos.key(thing);
    m_nuimos.take(nuimo);

    hardwareManager()->bluetoothLowEnergyManager()->unregisterDevice(nuimo->bluetoothDevice());
    nuimo->deleteLater();

    if (myThings().isEmpty()) {
        hardwareManager()->pluginTimerManager()->unregisterTimer(m_reconnectTimer);
        m_reconnectTimer = nullptr;
    }
}


void IntegrationPluginSenic::onReconnectTimeout()
{
    foreach (Nuimo *nuimo, m_nuimos.keys()) {
        if (!nuimo->bluetoothDevice()->connected()) {
            nuimo->bluetoothDevice()->connectDevice();
        }
    }
}

void IntegrationPluginSenic::onConnectedChanged(bool connected)
{
    Nuimo *nuimo = static_cast<Nuimo *>(sender());
    Thing *thing = m_nuimos.value(nuimo);
    thing->setStateValue(nuimoConnectedStateTypeId, connected);
}


void IntegrationPluginSenic::onButtonPressed()
{
    Nuimo *nuimo = static_cast<Nuimo *>(sender());
    Thing *thing = m_nuimos.value(nuimo);
    emitEvent(Event(nuimoPressedEventTypeId, thing->id(), ParamList() << Param(nuimoPressedEventButtonNameParamTypeId, "•")));

    if (m_autoSymbolMode) {
        nuimo->showImage(Nuimo::MatrixTypeCircle);
    }
}


void IntegrationPluginSenic::onButtonLongPressed()
{
    Nuimo *nuimo = static_cast<Nuimo *>(sender());
    Thing *thing = m_nuimos.value(nuimo);
    emitEvent(Event(nuimoLongPressedEventTypeId, thing->id()));

    if (m_autoSymbolMode) {
        nuimo->showImage(Nuimo::MatrixTypeFilledCircle);
    }
}


void IntegrationPluginSenic::onSwipeDetected(const Nuimo::SwipeDirection &direction)
{
    Nuimo *nuimo = static_cast<Nuimo *>(sender());
    Thing *thing = m_nuimos.value(nuimo);

    switch (direction) {
    case Nuimo::SwipeDirectionLeft:
        emitEvent(Event(nuimoPressedEventTypeId, thing->id(), ParamList() << Param(nuimoPressedEventButtonNameParamTypeId, "←")));
        break;
    case Nuimo::SwipeDirectionRight:
        emitEvent(Event(nuimoPressedEventTypeId, thing->id(), ParamList() << Param(nuimoPressedEventButtonNameParamTypeId, "→")));
        break;
    case Nuimo::SwipeDirectionUp:
        emitEvent(Event(nuimoPressedEventTypeId, thing->id(), ParamList() << Param(nuimoPressedEventButtonNameParamTypeId, "↑")));
        break;
    case Nuimo::SwipeDirectionDown:
        emitEvent(Event(nuimoPressedEventTypeId, thing->id(), ParamList() << Param(nuimoPressedEventButtonNameParamTypeId, "↓")));
        break;
    }

    if (m_autoSymbolMode) {
        switch (direction) {
        case Nuimo::SwipeDirectionLeft:
            nuimo->showImage(Nuimo::MatrixType::MatrixTypeLeft);
            break;
        case Nuimo::SwipeDirectionRight:
            nuimo->showImage(Nuimo::MatrixType::MatrixTypeRight);
            break;
        case Nuimo::SwipeDirectionUp:
            nuimo->showImage(Nuimo::MatrixType::MatrixTypeUp);
            break;
        case Nuimo::SwipeDirectionDown:
            nuimo->showImage(Nuimo::MatrixType::MatrixTypeDown);
            break;
        }
    }
}


void IntegrationPluginSenic::onRotationValueChanged(const uint &value)
{
    Nuimo *nuimo = static_cast<Nuimo *>(sender());
    Thing *thing = m_nuimos.value(nuimo);
    thing->setStateValue(nuimoRotationStateTypeId, value);
}


void IntegrationPluginSenic::onDeviceInformationChanged(const QString &firmwareRevision, const QString &hardwareRevision, const QString &softwareRevision)
{
    Nuimo *nuimo = static_cast<Nuimo *>(sender());
    Thing *thing = m_nuimos.value(nuimo);

    thing->setStateValue(nuimoFirmwareRevisionStateTypeId, firmwareRevision);
    thing->setStateValue(nuimoHardwareRevisionStateTypeId, hardwareRevision);
    thing->setStateValue(nuimoSoftwareRevisionStateTypeId, softwareRevision);
}


void IntegrationPluginSenic::onPluginConfigurationChanged(const ParamTypeId &paramTypeId, const QVariant &value)
{
    qCDebug(dcSenic()) << "Plugin configuration changed";

    // Check auto symbol mode
    if (paramTypeId == senicPluginAutoSymbolsParamTypeId) {
        qCDebug(dcSenic()) << "Auto symbol mode" << (value.toBool() ? "enabled." : "disabled.");
        m_autoSymbolMode = value.toBool();
    }

    if (paramTypeId == senicPluginLongPressTimeParamTypeId) {
        qCDebug(dcSenic()) << "Long press time" << value.toInt();
        foreach(Nuimo *nuimo, m_nuimos.keys()) {
            nuimo->setLongPressTime(value.toInt());
        }
    }
}

void IntegrationPluginSenic::onBatteryValueChanged(const uint &percentage)
{
    Nuimo *nuimo = static_cast<Nuimo *>(sender());
    Thing *thing = m_nuimos.value(nuimo);

    thing->setStateValue(nuimoBatteryLevelStateTypeId, percentage);
    if (percentage < 20) {
        thing->setStateValue(nuimoBatteryCriticalStateTypeId, true);
    } else {
        thing->setStateValue(nuimoBatteryCriticalStateTypeId, false);
    }
}
