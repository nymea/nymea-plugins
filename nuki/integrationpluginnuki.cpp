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

#include "integrationpluginnuki.h"
#include "integrations/thing.h"
#include "plugininfo.h"
#include "hardware/bluetoothlowenergy/bluetoothlowenergymanager.h"

extern "C"{
#include "sodium.h"
}

IntegrationPluginNuki::IntegrationPluginNuki()
{

}

IntegrationPluginNuki::~IntegrationPluginNuki()
{
    hardwareManager()->pluginTimerManager()->unregisterTimer(m_refreshTimer);
}

void IntegrationPluginNuki::init()
{
    // Read every hour the state of the lock
    m_refreshTimer = hardwareManager()->pluginTimerManager()->registerTimer(3600);
    connect(m_refreshTimer, &PluginTimer::timeout, this, &IntegrationPluginNuki::onRefreshTimeout);

    // Bluetooth manager for BTLE bluez handling
    m_bluetoothManager = new BluetoothManager(this);
    if (!m_bluetoothManager->isAvailable()) {
        qCWarning(dcNuki()) << "Bluetooth not available";
        return;
    }

    if (m_bluetoothManager->adapters().isEmpty()) {
        qCWarning(dcNuki()) << "No bluetooth adapter found.";
        return;
    }

    m_bluetoothAdapter = m_bluetoothManager->adapters().first();
    m_bluetoothAdapter->setPower(true);
    m_bluetoothAdapter->setDiscoverable(true);
    m_bluetoothAdapter->setPairable(true);

    qCDebug(dcNuki()) << "Using bluetooth adapter" << m_bluetoothAdapter;

    if (sodium_init() < 0) {
        qCCritical(dcNuki()) << "Could not initialize encryption library sodium";
        m_encrytionLibraryInitialized = false;
        return;
    }

    m_encrytionLibraryInitialized = true;
    qCDebug(dcNuki()) << "Encryption library initialized successfully: libsodium" << sodium_version_string();
}

void IntegrationPluginNuki::setupThing(ThingSetupInfo *info)
{
    Thing *thing = info->thing();
    qCDebug(dcNuki()) << "Setup thing" << thing->name() << thing->params();

    QBluetoothAddress address = QBluetoothAddress(thing->params().paramValue(nukiThingMacParamTypeId).toString());
    if (bluetoothDeviceAlreadyAdded(address)) {
        qCWarning(dcNuki()) << "Device already added.";
        return info->finish(Thing::ThingErrorThingInUse, QT_TR_NOOP("Device is already in use."));
    }

    if (!m_bluetoothAdapter){
        qCWarning(dcNuki()) << "No bluetooth adapter available";
        return info->finish(Thing::ThingErrorHardwareNotAvailable, QT_TR_NOOP("Bluetooth is not available on this system."));
    }

    if (m_bluetoothAdapter->hasDevice(address)) {
        Nuki *nuki = new Nuki(thing, m_bluetoothAdapter->getDevice(address), this);
        m_nukiDevices.insert(nuki, thing);
    } else {
        qCWarning(dcNuki()) << "Could not find bluetooth thing for setup" << address;
        return info->finish(Thing::ThingErrorHardwareNotAvailable, QT_TR_NOOP("Bluetooth thing not found."));
    }

    info->finish(Thing::ThingErrorNoError);
}

void IntegrationPluginNuki::discoverThings(ThingDiscoveryInfo *info)
{

    if (info->thingClassId() != nukiThingClassId)
        return info->finish(Thing::ThingErrorThingClassNotFound);

    if (!hardwareManager()->bluetoothLowEnergyManager()->enabled())
        return info->finish(Thing::ThingErrorHardwareNotAvailable, QT_TR_NOOP("Bluetooth is not available on this system."));

    if (!m_bluetoothAdapter)
        return info->finish(Thing::ThingErrorHardwareNotAvailable, QT_TR_NOOP("Bluetooth is not available on this system."));

    m_bluetoothAdapter->setDiscoverable(true);
    m_bluetoothAdapter->setPairable(true);

    qCDebug(dcNuki()) << "Start bluetooth discovery...";
    if (!m_bluetoothAdapter->discovering())
        m_bluetoothAdapter->startDiscovering();

    QTimer::singleShot(5000, info, [this, info]() { onBluetoothDiscoveryFinished(info); });
}

void IntegrationPluginNuki::startPairing(ThingPairingInfo *info)
{
    info->finish(Thing::ThingErrorNoError, QT_TR_NOOP("Please press the Nuki button for 5 seconds in order to activate the pairing mode before you continue."));
}

void IntegrationPluginNuki::confirmPairing(ThingPairingInfo *info, const QString &username, const QString &secret)
{
    Q_UNUSED(username)
    Q_UNUSED(secret)

    qCDebug(dcNuki()) << "Pairing confirmed, assuming the pairing mode is active. Start authentication process";
    if (info->thingClassId() != nukiThingClassId) {
        qCWarning(dcNuki()) << "Invalid thing class id";
        return info->finish(Thing::ThingErrorThingClassNotFound);
    }

    if (m_asyncSetupNuki) {
        qCWarning(dcNuki()) << "There is already an async setup for a nuki running.";
        return info->finish(Thing::ThingErrorThingInUse);
    }

    QBluetoothAddress address = QBluetoothAddress(info->params().paramValue(nukiThingMacParamTypeId).toString());
    if (!m_bluetoothAdapter->hasDevice(address)) {
        qCWarning(dcNuki()) << "Could not find bluetooth thing for" << address.toString();
        return info->finish(Thing::ThingErrorThingNotFound);
    }

    // Create a temporary nuki for authentication
    BluetoothDevice *bluetoothDevice = m_bluetoothAdapter->getDevice(address);
    m_asyncSetupNuki = new Nuki(nullptr, bluetoothDevice, this);
    connect(m_asyncSetupNuki, &Nuki::authenticationProcessFinished, this, [=](const PairingTransactionId &pairingId, bool success){
        if (m_asyncSetupNuki) {
            qCDebug(dcNuki()) << "Deleting the temporary pairing device";
            m_asyncSetupNuki->deleteLater();
            m_asyncSetupNuki = nullptr;
        }

        if (!m_pairingInfo) {
            qCWarning(dcNuki()) << "Authentication process finished, but have not valid pairing translaction id";
            return;
        }

        if (m_pairingInfo->transactionId() != pairingId) {
            qCWarning(dcNuki()) << "Authentication process finished, but have not valid pairing translaction id";
            return;
        }

        m_pairingInfo->finish(success ? Thing::ThingErrorNoError : Thing::ThingErrorHardwareFailure);
        m_pairingInfo = nullptr;
    });

    connect(m_asyncSetupNuki, &Nuki::availableChanged, this, [=](bool available){
        // Remove possibly running Nuki setup devices on disconnected
        if (m_asyncSetupNuki && !available) {
            qCDebug(dcNuki()) << "Deleting the temporary pairing device";
            m_asyncSetupNuki->deleteLater();
            m_asyncSetupNuki = nullptr;

            // Lost connection during authentication
            if (m_pairingInfo) {
                qCWarning(dcNuki()) << "Device disconnected during pairing.";
                m_pairingInfo->finish(Thing::ThingErrorHardwareFailure);
                m_pairingInfo = nullptr;
            }
        }
    });

    m_pairingInfo = info;
    m_asyncSetupNuki->startAuthenticationProcess(info->transactionId());
    connect(info, &ThingPairingInfo::destroyed, this, [this] {
        m_pairingInfo = nullptr;
        if (m_asyncSetupNuki) {
            qCDebug(dcNuki()) << "Deleting the temporary pairing device";
            m_asyncSetupNuki->deleteLater();
            m_asyncSetupNuki = nullptr;
        }
    });
}

void IntegrationPluginNuki::postSetupThing(Thing *thing)
{
    Nuki *nuki = m_nukiDevices.key(thing);
    nuki->refreshStates();
}

void IntegrationPluginNuki::executeAction(ThingActionInfo *info)
{
    Thing *thing = info->thing();
    Action action = info->action();

    QPointer<Nuki> nuki = m_nukiDevices.key(thing);
    if (nuki.isNull()) {
        qCWarning(dcNuki()) << "Could not execute action. There is no nuki device for this thing";
        return info->finish(Thing::ThingErrorHardwareFailure);
    }

    if (!hardwareManager()->bluetoothLowEnergyManager()->enabled()) {
        qCWarning(dcNuki()) << "Could not execute action. There bluetooth hardware resource is disabled.";
        return info->finish(Thing::ThingErrorHardwareNotAvailable);
    }

    if (action.actionTypeId() == nukiLockActionTypeId) {
        if (!nuki->executeDeviceAction(Nuki::NukiActionLock, info)) {
            return info->finish(Thing::ThingErrorThingInUse);
        }
        return;
    } else if (action.actionTypeId() == nukiUnlockActionTypeId) {
        if (!nuki->executeDeviceAction(Nuki::NukiActionUnlock, info)) {
            return info->finish(Thing::ThingErrorThingInUse);
        }
        return;
    } else if (action.actionTypeId() == nukiUnlatchActionTypeId) {
        if (!nuki->executeDeviceAction(Nuki::NukiActionUnlatch, info)) {
            return info->finish(Thing::ThingErrorThingInUse);
        }
        return;
    } else if (action.actionTypeId() == nukiRefreshActionTypeId) {
        if (!nuki->executeDeviceAction(Nuki::NukiActionRefresh, info)) {
            return info->finish(Thing::ThingErrorThingInUse);
        }
        return;
    }

    info->finish(Thing::ThingErrorActionTypeNotFound);
}

void IntegrationPluginNuki::thingRemoved(Thing *thing)
{
    if (!m_nukiDevices.values().contains(thing))
        return;

    Nuki *nuki = m_nukiDevices.key(thing);
    nuki->clearSettings();

    // TODO: deauthenticate nymea from nuki

    qCDebug(dcNuki()) << "Delete pairing information from bluez" << nuki->bluetoothDevice();
    m_bluetoothAdapter->removeDevice(nuki->bluetoothDevice()->address());
    m_nukiDevices.remove(nuki);
    nuki->deleteLater();
}

bool IntegrationPluginNuki::bluetoothDeviceAlreadyAdded(const QBluetoothAddress &address)
{
    foreach (Thing *thing, m_nukiDevices.values()) {
        if (thing->thingClassId() == nukiThingClassId && thing->paramValue(nukiThingMacParamTypeId).toString() == address.toString()) {
            qCDebug(dcNuki()) << "Nuki with address" << address.toString() << "already added.";
            return true;
        }
    }

    return false;
}

void IntegrationPluginNuki::onRefreshTimeout()
{
    // Only reconnect if the hardware resource is enabled
    if (hardwareManager()->bluetoothLowEnergyManager()->enabled()) {
        foreach (Nuki *nuki, m_nukiDevices.keys()) {
            nuki->refreshStates();
        }
    }
}

void IntegrationPluginNuki::onBluetoothEnabledChanged(const bool &enabled)
{
    qCDebug(dcNuki()) << "Bluetooth hardware resource" << (enabled ? "enabled" : "disabled");

    // Disconnect all devices, autoconnect will not trigger until the resource is enabled again
    foreach (Nuki *nuki, m_nukiDevices.keys()) {
        if (!enabled) {
            nuki->disconnectDevice();
        } else {
            nuki->connectDevice();
        }
    }
}

void IntegrationPluginNuki::onBluetoothDiscoveryFinished(ThingDiscoveryInfo *info)
{
    qCDebug(dcNuki()) << "Bluetooth discovery for nuki devices finished";
    m_bluetoothAdapter->stopDiscovering();

    foreach (BluetoothDevice *thing, m_bluetoothAdapter->devices()) {
        qCDebug(dcNuki()) << "Found bluetooth device" << thing->name() << thing->address().toString();
        if (!bluetoothDeviceAlreadyAdded(thing->address()) && thing->name().toLower().contains("nuki")) {
            qCDebug(dcNuki()) << "Found nuki smart lock which has not been added yet";

            // Get serial number from name
            QString serialNumber;
            QStringList tokens = thing->name().split("_");
            if (tokens.count() == 2) {
                serialNumber = tokens.at(1);
            } else {
                qCWarning(dcNuki()) << "Could not read serial number from bluetooth thing name" << thing->name();
            }

            ThingDescriptor descriptor(nukiThingClassId, "Nuki", thing->address().toString());
            ParamList params;
            params.append(Param(nukiThingNameParamTypeId, thing->name()));
            params.append(Param(nukiThingMacParamTypeId, thing->address().toString()));
            params.append(Param(nukiThingSerialNumberParamTypeId, serialNumber));
            descriptor.setParams(params);
            info->addThingDescriptor(descriptor);
        }
    }

    info->finish(Thing::ThingErrorNoError);
}
