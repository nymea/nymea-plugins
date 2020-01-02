/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*
*  Copyright 2013 - 2020, nymea GmbH
*  Contact: contact@nymea.io
*
*  This file is part of nymea.
*  This project including source code and documentation is protected by copyright law, and
*  remains the property of nymea GmbH. All rights, including reproduction, publication,
*  editing and translation, are reserved. The use of this project is subject to the terms of a
*  license agreement to be concluded with nymea GmbH in accordance with the terms
*  of use of nymea GmbH, available under https://nymea.io/license
*
*  GNU Lesser General Public License Usage
*  This project may also contain libraries licensed under the open source software license GNU GPL v.3.
*  Alternatively, this project may be redistributed and/or modified under the terms of the GNU
*  Lesser General Public License as published by the Free Software Foundation; version 3.
*  this project is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;
*  without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
*  See the GNU Lesser General Public License for more details.
*
*  You should have received a copy of the GNU Lesser General Public License along with this project.
*  If not, see <https://www.gnu.org/licenses/>.
*
*  For any further details and any questions please contact us under contact@nymea.io
*  or see our FAQ/Licensing Information on https://nymea.io/license/faq
*
* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

#include "devicepluginnuki.h"
#include "devices/device.h"
#include "plugininfo.h"
#include "hardware/bluetoothlowenergy/bluetoothlowenergymanager.h"

extern "C"{
#include "sodium.h"
}

DevicePluginNuki::DevicePluginNuki()
{

}

DevicePluginNuki::~DevicePluginNuki()
{
    hardwareManager()->pluginTimerManager()->unregisterTimer(m_refreshTimer);
}

void DevicePluginNuki::init()
{
    // Read every hour the state of the lock
    m_refreshTimer = hardwareManager()->pluginTimerManager()->registerTimer(3600);
    connect(m_refreshTimer, &PluginTimer::timeout, this, &DevicePluginNuki::onRefreshTimeout);

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

void DevicePluginNuki::setupDevice(DeviceSetupInfo *info)
{
    Device *device = info->device();
    qCDebug(dcNuki()) << "Setup device" << device->name() << device->params();

    QBluetoothAddress address = QBluetoothAddress(device->params().paramValue(nukiDeviceMacParamTypeId).toString());
    if (bluetoothDeviceAlreadyAdded(address)) {
        qCWarning(dcNuki()) << "Device already added.";
        return info->finish(Device::DeviceErrorDeviceInUse, QT_TR_NOOP("Device is already in use."));
    }

    if (!m_bluetoothAdapter){
        qCWarning(dcNuki()) << "No bluetooth adapter available";
        return info->finish(Device::DeviceErrorHardwareNotAvailable, QT_TR_NOOP("Bluetooth is not available on this system."));
    }

    if (m_bluetoothAdapter->hasDevice(address)) {
        Nuki *nuki = new Nuki(device, m_bluetoothAdapter->getDevice(address), this);
        m_nukiDevices.insert(nuki, device);
    } else {
        qCWarning(dcNuki()) << "Could not find bluetooth device for setup" << address;
        return info->finish(Device::DeviceErrorHardwareNotAvailable, QT_TR_NOOP("Bluetooth device not found."));
    }

    info->finish(Device::DeviceErrorNoError);
}

void DevicePluginNuki::discoverDevices(DeviceDiscoveryInfo *info)
{

    if (info->deviceClassId() != nukiDeviceClassId)
        return info->finish(Device::DeviceErrorDeviceClassNotFound);

    if (!hardwareManager()->bluetoothLowEnergyManager()->enabled())
        return info->finish(Device::DeviceErrorHardwareNotAvailable, QT_TR_NOOP("Bluetooth is not available on this system."));

    if (!m_bluetoothAdapter)
        return info->finish(Device::DeviceErrorHardwareNotAvailable, QT_TR_NOOP("Bluetooth is not available on this system."));

    m_bluetoothAdapter->setDiscoverable(true);
    m_bluetoothAdapter->setPairable(true);

    qCDebug(dcNuki()) << "Start bluetooth discovery...";
    if (!m_bluetoothAdapter->discovering())
        m_bluetoothAdapter->startDiscovering();

    QTimer::singleShot(5000, info, [this, info]() { onBluetoothDiscoveryFinished(info); });

}

void DevicePluginNuki::startPairing(DevicePairingInfo *info)
{
    info->finish(Device::DeviceErrorNoError, QT_TR_NOOP("Please press the Nuki button for 5 seconds in order to activate the pairing mode before you continue."));
}

void DevicePluginNuki::confirmPairing(DevicePairingInfo *info, const QString &username, const QString &secret)
{
    Q_UNUSED(username)
    Q_UNUSED(secret)

    qCDebug(dcNuki()) << "Pairing confirmed, assuming the pairing mode is active. Start authentication process";
    if (info->deviceClassId() != nukiDeviceClassId) {
        qCWarning(dcNuki()) << "Invalid device class id";
        return info->finish(Device::DeviceErrorDeviceClassNotFound);
    }

    if (m_asyncSetupNuki) {
        qCWarning(dcNuki()) << "There is already an async setup for a nuki running.";
        return info->finish(Device::DeviceErrorDeviceInUse);
    }

    QBluetoothAddress address = QBluetoothAddress(info->params().paramValue(nukiDeviceMacParamTypeId).toString());
    if (!m_bluetoothAdapter->hasDevice(address)) {
        qCWarning(dcNuki()) << "Could not find bluetooth device for" << address.toString();
        return info->finish(Device::DeviceErrorDeviceNotFound);
    }

    BluetoothDevice *bluetoothDevice = m_bluetoothAdapter->getDevice(address);

    m_asyncSetupNuki = new Nuki(nullptr, bluetoothDevice, this);
    connect(m_asyncSetupNuki, &Nuki::authenticationProcessFinished, this, &DevicePluginNuki::onNukiAuthenticationProcessFinished);
    connect(m_asyncSetupNuki, &Nuki::availableChanged, this, &DevicePluginNuki::onAsyncSetupNukiAvailableChanged);

    m_asyncSetupNuki->startAuthenticationProcess(info->transactionId());
    m_pairingInfo = info;
    connect(info, &DevicePairingInfo::destroyed, this, [this] { m_pairingInfo = nullptr; });
}

void DevicePluginNuki::postSetupDevice(Device *device)
{
    Nuki *nuki = m_nukiDevices.key(device);
    nuki->refreshStates();
}

void DevicePluginNuki::executeAction(DeviceActionInfo *info)
{
    Device *device = info->device();
    Action action = info->action();

    QPointer<Nuki> nuki = m_nukiDevices.key(device);
    if (nuki.isNull()) {
        qCWarning(dcNuki()) << "Could not execute action. There is no nuki object for this device";
        return info->finish(Device::DeviceErrorHardwareFailure);
    }

    if (!hardwareManager()->bluetoothLowEnergyManager()->enabled()) {
        qCWarning(dcNuki()) << "Could not execute action. There bluetooth hardware resource is disabled.";
        return info->finish(Device::DeviceErrorHardwareNotAvailable);
    }

    if (action.actionTypeId() == nukiCloseActionTypeId) {
        if (!nuki->executeDeviceAction(Nuki::NukiActionLock, info)) {
            return info->finish(Device::DeviceErrorDeviceInUse);
        }
        return;
    } else if (action.actionTypeId() == nukiOpenActionTypeId) {
        if (!nuki->executeDeviceAction(Nuki::NukiActionUnlock, info)) {
            return info->finish(Device::DeviceErrorDeviceInUse);
        }
        return;
    } else if (action.actionTypeId() == nukiUnlatchActionTypeId) {
        if (!nuki->executeDeviceAction(Nuki::NukiActionUnlatch, info)) {
            return info->finish(Device::DeviceErrorDeviceInUse);
        }
        return;
    } else if (action.actionTypeId() == nukiRefreshActionTypeId) {
        if (!nuki->executeDeviceAction(Nuki::NukiActionRefresh, info)) {
            return info->finish(Device::DeviceErrorDeviceInUse);
        }
        return;
    }

    info->finish(Device::DeviceErrorActionTypeNotFound);
}

void DevicePluginNuki::deviceRemoved(Device *device)
{
    if (!m_nukiDevices.values().contains(device))
        return;

    Nuki *nuki = m_nukiDevices.key(device);
    nuki->clearSettings();

    // FIXME: deauthenticate nymea from nuki device

    qCDebug(dcNuki()) << "Delete pairing information from bluez" << nuki->bluetoothDevice();
    m_bluetoothAdapter->removeDevice(nuki->bluetoothDevice()->address());
    m_nukiDevices.remove(nuki);
    nuki->deleteLater();
}

bool DevicePluginNuki::bluetoothDeviceAlreadyAdded(const QBluetoothAddress &address)
{
    foreach (Device *device, m_nukiDevices.values()) {
        if (device->deviceClassId() == nukiDeviceClassId && device->paramValue(nukiDeviceMacParamTypeId).toString() == address.toString()) {
            qCDebug(dcNuki()) << "Nuki with address" << address.toString() << "already added.";
            return true;
        }
    }

    return false;
}

void DevicePluginNuki::onRefreshTimeout()
{
    // Only reconnect if the hardware resource is enabled
    if (hardwareManager()->bluetoothLowEnergyManager()->enabled()) {
        foreach (Nuki *nuki, m_nukiDevices.keys()) {
            nuki->refreshStates();
        }
    }
}

void DevicePluginNuki::onBluetoothEnabledChanged(const bool &enabled)
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

void DevicePluginNuki::onBluetoothDiscoveryFinished(DeviceDiscoveryInfo *info)
{
    qCDebug(dcNuki()) << "Bluetooth discovery for nuki devices finished";
    m_bluetoothAdapter->stopDiscovering();

    foreach (BluetoothDevice *device, m_bluetoothAdapter->devices()) {
        if (!bluetoothDeviceAlreadyAdded(device->address()) && device->name().contains("Nuki")) {
            DeviceDescriptor descriptor(nukiDeviceClassId, "Nuki", device->address().toString());

            // Get serial number from name
            QString serialNumber;
            QStringList tokens = device->name().split("_");
            if (tokens.count() == 2) {
                serialNumber = tokens.at(1);
            } else {
                qCWarning(dcNuki()) << "Could not read serial number from bluetooth device name" << device->name();
            }

            ParamList params;
            params.append(Param(nukiDeviceNameParamTypeId, device->name()));
            params.append(Param(nukiDeviceMacParamTypeId, device->address().toString()));
            params.append(Param(nukiDeviceSerialNumberParamTypeId, serialNumber));
            descriptor.setParams(params);
            info->addDeviceDescriptor(descriptor);
        }
    }

    info->finish(Device::DeviceErrorNoError);
}

void DevicePluginNuki::onAsyncSetupNukiAvailableChanged(bool available)
{
    // Remove possibly running Nuki setup devices on disconnected
    if (!available && m_asyncSetupNuki) {
        qCDebug(dcNuki()) << "Delete the temporary pairing device";
        m_asyncSetupNuki->deleteLater();
        m_asyncSetupNuki = nullptr;
    }
}

void DevicePluginNuki::onNukiAuthenticationProcessFinished(const PairingTransactionId &pairingTransactionId, bool success)
{
    if (m_asyncSetupNuki) {
        qCDebug(dcNuki()) << "Delete the temporary pairing device";
        m_asyncSetupNuki->deleteLater();
        m_asyncSetupNuki = nullptr;
    }

    if (!m_pairingInfo) {
        qCWarning(dcNuki()) << "Authentication process finished, but have not valid pairing translaction id";
        return;
    }

    if (m_pairingInfo->transactionId() != pairingTransactionId) {
        qCWarning(dcNuki()) << "Authentication process finished, but have not valid pairing translaction id";
        return;
    }

    m_pairingInfo->finish(success ? Device::DeviceErrorNoError : Device::DeviceErrorHardwareFailure);
    m_pairingInfo = nullptr;
}
