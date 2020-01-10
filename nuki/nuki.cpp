/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *                                                                         *
 *  Copyright (C) 2018 Bernhard Trinnes <bernhard.trinnes@guh.io>          *
 *  Copyright (C) 2018 Simon Stürz <simon.stuerz@guh.io>                   *
 *                                                                         *
 *  This file is part of guh.                                              *
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

#include "nuki.h"
#include "extern-plugininfo.h"

#include <QBitArray>
#include <QtEndian>
#include <QByteArray>
#include <QDataStream>
#include <QTimer>

static QBluetoothUuid initializationServiceUuid             = QBluetoothUuid(QUuid("a92ee000-5501-11e4-916c-0800200c9a66"));
static QBluetoothUuid pairingServiceUuid                    = QBluetoothUuid(QUuid("a92ee100-5501-11e4-916c-0800200c9a66"));
static QBluetoothUuid pairingDataCharacteristicUuid         = QBluetoothUuid(QUuid("a92ee101-5501-11e4-916c-0800200c9a66"));
static QBluetoothUuid keyturnerServiceUuid                  = QBluetoothUuid(QUuid("a92ee200-5501-11e4-916c-0800200c9a66"));
static QBluetoothUuid keyturnerDataCharacteristicUuid       = QBluetoothUuid(QUuid("a92ee201-5501-11e4-916c-0800200c9a66"));
static QBluetoothUuid keyturnerUserDataCharacteristicUuid   = QBluetoothUuid(QUuid("a92ee202-5501-11e4-916c-0800200c9a66"));

Nuki::Nuki(Device *device, BluetoothDevice *bluetoothDevice, QObject *parent) :
    QObject(parent),
    m_device(device),
    m_bluetoothDevice(bluetoothDevice)
{
    connect(m_bluetoothDevice, &BluetoothDevice::stateChanged, this, &Nuki::onBluetoothDeviceStateChanged);
    onBluetoothDeviceStateChanged(m_bluetoothDevice->state());
}

Device *Nuki::device()
{
    return m_device;
}

BluetoothDevice *Nuki::bluetoothDevice()
{
    return m_bluetoothDevice;
}

bool Nuki::startAuthenticationProcess(const PairingTransactionId &pairingTransactionId)
{
    if (m_nukiAction != NukiActionNone) {
        qCWarning(dcNuki()) << "Cannot start authentication process. Nuki is busy and already processing an action. Please retry again." << m_nukiAction;
        return false;
    }

    m_nukiAction = NukiActionAuthenticate;
    m_pairingId = pairingTransactionId;

    if (m_available) {
        executeCurrentAction();
    } else {
        m_bluetoothDevice->connectDevice();
    }

    return true;
}

bool Nuki::refreshStates()
{
    return executeNukiAction(NukiActionRefresh);
}

bool Nuki::executeNukiAction(Nuki::NukiAction action)
{
    if (m_nukiAction != NukiActionNone) {
        qCWarning(dcNuki()) << "Cannot execute Nuki action. Nuki is busy and already processing an action." << m_nukiAction;
        return false;
    }

    m_nukiAction = action;

    if (m_available) {
        executeCurrentAction();
    } else {
        m_bluetoothDevice->connectDevice();
    }
    return true;
}

bool Nuki::executeDeviceAction(Nuki::NukiAction action, DeviceActionInfo *actionInfo)
{
    if (m_nukiAction != NukiActionNone || !m_actionInfo.isNull()) {
        qCWarning(dcNuki()) << "Nuki is busy and already processing an action. Please retry again."  << m_nukiAction;
        return false;
    }

    m_actionInfo = QPointer<DeviceActionInfo>(actionInfo);
    m_nukiAction = action;

    if (m_available) {
        executeCurrentAction();
    } else {
        m_bluetoothDevice->connectDevice();
    }
    return true;
}

void Nuki::connectDevice()
{
    if (!m_bluetoothDevice)
        return;

    m_bluetoothDevice->connectDevice();
}

void Nuki::disconnectDevice()
{
    if (!m_bluetoothDevice)
        return;

    m_bluetoothDevice->disconnectDevice();
}

void Nuki::clearSettings()
{
    if (m_nukiAuthenticator) {
        m_nukiAuthenticator->clearSettings();
    }
}

void Nuki::printServices()
{
    foreach (BluetoothGattService *service, m_bluetoothDevice->services()) {
        qCDebug(dcNuki()) << service;
        foreach (BluetoothGattCharacteristic *characteristic, service->characteristics()) {
            qCDebug(dcNuki()) << "    " << characteristic->chararcteristicName() << characteristic->uuid().toString();
            foreach (BluetoothGattDescriptor *descriptor, characteristic->descriptors()) {
                qCDebug(dcNuki()) << "        " << descriptor->name() << descriptor->uuid().toString();
            }
        }
    }
}

void Nuki::readDeviceInformationCharacteristics()
{
    m_initUuidsToRead.append(QBluetoothUuid::SerialNumberString);
    m_initUuidsToRead.append(QBluetoothUuid::HardwareRevisionString);
    m_initUuidsToRead.append(QBluetoothUuid::FirmwareRevisionString);

    m_deviceInformationService->readCharacteristic(QBluetoothUuid::SerialNumberString);
    m_deviceInformationService->readCharacteristic(QBluetoothUuid::HardwareRevisionString);
    m_deviceInformationService->readCharacteristic(QBluetoothUuid::FirmwareRevisionString);
}

void Nuki::executeCurrentAction()
{
    qCDebug(dcNuki()) << "Executing" << m_nukiAction;

    switch (m_nukiAction) {
    case NukiActionAuthenticate:
        m_nukiAuthenticator->startAuthenticationProcess();
        break;
    case NukiActionRefresh:
        if (!m_nukiController->readLockState()) {
            finishCurrentAction(false);
        }
        break;
    case NukiActionLock:
        if (!m_nukiController->lock()) {
            finishCurrentAction(false);
        }
        break;
    case NukiActionUnlock:
        if (!m_nukiController->unlock()) {
            finishCurrentAction(false);
        }
        break;
    case NukiActionUnlatch:
        if (!m_nukiController->unlatch()) {
            finishCurrentAction(false);
        }
        break;
    default:
        break;
    }
}

void Nuki::onBluetoothDeviceStateChanged(const BluetoothDevice::State &state)
{
    qCDebug(dcNuki()) << m_bluetoothDevice  << "state changed --> " << state;
    switch (state) {
    case BluetoothDevice::Connecting:
        break;
    case BluetoothDevice::Connected:
        if (m_bluetoothDevice->servicesResolved()) {
            // Services already discovered
            if (!init()) {
                qCWarning(dcNuki()) << "Could not initialze device" << m_bluetoothDevice;
                m_bluetoothDevice->disconnectDevice();
            } else {
                readDeviceInformationCharacteristics();
            }
        }
        break;
    case BluetoothDevice::Pairing:
        break;
    case BluetoothDevice::Discovering:
        break;
    case BluetoothDevice::Discovered:
        printServices();
        if (!init()) {
            qCWarning(dcNuki()) << "Could not initialze device" << m_bluetoothDevice;
            m_bluetoothDevice->disconnectDevice();
        } else {
            readDeviceInformationCharacteristics();
        }
        break;
    case BluetoothDevice::Disconnecting:
        setAvailable(false);
        clean();
        break;
    case BluetoothDevice::Disconnected:
        setAvailable(false);
        clean();
        break;
    default:
        break;
    }
}

void Nuki::onDeviceInfoCharacteristicReadFinished(BluetoothGattCharacteristic *characteristic, const QByteArray &value)
{
    qCDebug(dcNuki()) << "Read device information characteristic finished" << characteristic->chararcteristicName() << qUtf8Printable(value);
    if (characteristic->uuid() == QBluetoothUuid::SerialNumberString) {
        m_serialNumber = QString::fromUtf8(value);
        m_initUuidsToRead.removeOne(QBluetoothUuid::SerialNumberString);
    } else if (characteristic->uuid() == QBluetoothUuid::HardwareRevisionString) {
        m_hardwareRevision = QString::fromUtf8(value);
        m_initUuidsToRead.removeOne(QBluetoothUuid::HardwareRevisionString);
    } else if (characteristic->uuid() == QBluetoothUuid::FirmwareRevisionString) {
        m_firmwareRevision = QString::fromUtf8(value);
        m_initUuidsToRead.removeOne(QBluetoothUuid::FirmwareRevisionString);
    }

    if (m_initUuidsToRead.isEmpty()) {
        // Initial read done. Make device available
        setAvailable(true);
    }
}

void Nuki::onAuthenticationError(NukiUtils::ErrorCode error)
{
    qCWarning(dcNuki()) << "Authentication error occured" << error;

    if (m_pairingId.isNull())
        return;

    // If we have a pairing id
    emit authenticationProcessFinished(m_pairingId, false);
    m_pairingId = PairingTransactionId();
}

void Nuki::onAuthenticationFinished(bool success)
{
    qCDebug(dcNuki()) << "Authentication process finished" << (success ? "successfully." : "with error.");

    if (m_pairingId.isNull())
        return;

    // If we have a pairing id
    emit authenticationProcessFinished(m_pairingId, success);
    m_pairingId = PairingTransactionId();
}

void Nuki::onNukiReadStatesFinished(bool success)
{
    m_nukiAction = NukiActionNone;

    if (success) {
        // Update states
        onNukiStatesChanged();
    }

    // Check if this was an action call
    if (m_actionInfo.isNull()) {
        // Looks like this was a refresh call, lets disconnect to minimize the not reachable time for other apps
        QTimer::singleShot(0, m_bluetoothDevice, &BluetoothDevice::disconnectDevice);
        return;
    }

    finishCurrentAction(true);
}

void Nuki::onNukiStatesChanged()
{
    if (!m_device)
        return;

    m_device->setStateValue(nukiHardwareRevisionStateTypeId, m_hardwareRevision);
    m_device->setStateValue(nukiFirmwareRevisionStateTypeId, m_firmwareRevision);
    m_device->setStateValue(nukiBatteryCriticalStateTypeId, m_nukiController->batteryCritical());

    switch (m_nukiController->nukiLockTrigger()) {
    case NukiUtils::LockTriggerBluetooth:
        m_device->setStateValue(nukiTriggerStateTypeId, "Bluetooth");
        break;
    case NukiUtils::LockTriggerButton:
        m_device->setStateValue(nukiTriggerStateTypeId, "Button");
        break;
    case NukiUtils::LockTriggerManual:
        m_device->setStateValue(nukiTriggerStateTypeId, "Manual");
        break;
    default:
        break;
    }

    switch (m_nukiController->nukiState()) {
    case NukiUtils::NukiStateDoorMode:
        m_device->setStateValue(nukiModeStateTypeId, "Door");
        break;
    case NukiUtils::NukiStatePairingMode:
        m_device->setStateValue(nukiModeStateTypeId, "Pairing");
        break;
    case NukiUtils::NukiStateUninitialized:
        m_device->setStateValue(nukiModeStateTypeId, "Uninitialized");
        break;
    default:
        break;
    }

    switch (m_nukiController->nukiLockState()) {
    case NukiUtils::LockStateLocked:
        m_device->setStateValue(nukiStateStateTypeId, "locked");
        m_device->setStateValue(nukiStatusStateTypeId, "Ok");
        break;
    case NukiUtils::LockStateLocking:
        m_device->setStateValue(nukiStateStateTypeId, "locking");
        m_device->setStateValue(nukiStatusStateTypeId, "Ok");
        break;
    case NukiUtils::LockStateMotorBlocked:
        m_device->setStateValue(nukiStatusStateTypeId, "Motor blocked");
        break;
    case NukiUtils::LockStateUncalibrated:
        m_device->setStateValue(nukiStatusStateTypeId, "Uncalibrated");
        break;
    case NukiUtils::LockStateUndefined:
        m_device->setStateValue(nukiStatusStateTypeId, "Undefined");
        break;
    case NukiUtils::LockStateUnlatched:
        m_device->setStateValue(nukiStateStateTypeId, "unlatched");
        m_device->setStateValue(nukiStatusStateTypeId, "Ok");
        break;
    case NukiUtils::LockStateUnlatching:
        m_device->setStateValue(nukiStateStateTypeId, "unlatching");
        m_device->setStateValue(nukiStatusStateTypeId, "Ok");
        break;
    case NukiUtils::LockStateUnlockedLocknGoActive:
        m_device->setStateValue(nukiStatusStateTypeId, "unlocked");
        break;
    case NukiUtils::LockStateUnlocked:
        m_device->setStateValue(nukiStateStateTypeId, "unlocked");
        m_device->setStateValue(nukiStatusStateTypeId, "Ok");
        break;
    case NukiUtils::LockStateUnlocking:
        m_device->setStateValue(nukiStateStateTypeId, "unlocking");
        m_device->setStateValue(nukiStatusStateTypeId, "Ok");
        break;
    default:
        break;
    }
}

bool Nuki::init()
{
    if (!m_bluetoothDevice)
        return false;

    qCDebug(dcNuki()) << "Init" << m_bluetoothDevice;

    // If not connected, connect
    if (!m_bluetoothDevice->connected()) {
        qCWarning(dcNuki()) << "Device is not connected" << m_bluetoothDevice;
        return false;
    }

    // If services not resolved yet, wait
    if (!m_bluetoothDevice->servicesResolved()) {
        qCWarning(dcNuki()) << "Device services not resolved yet" << m_bluetoothDevice;
        return false;
    }

    // Verify services
    if (!m_bluetoothDevice->hasService(QBluetoothUuid::DeviceInformation)) {
        qCWarning(dcNuki()) << "Could not find device information service on device" << m_bluetoothDevice;
        return false;
    }

    if (!m_bluetoothDevice->hasService(pairingServiceUuid)) {
        qCWarning(dcNuki()) << "Could not find pairing service on device" << m_bluetoothDevice;
        return false;
    }

    if (!m_bluetoothDevice->hasService(keyturnerServiceUuid)) {
        qCWarning(dcNuki()) << "Could not find key turner service on device" << m_bluetoothDevice;
        return false;
    }

    // Create service and characteristic objects
    // Device information
    m_deviceInformationService = m_bluetoothDevice->getService(QBluetoothUuid::DeviceInformation);
    connect(m_deviceInformationService, &BluetoothGattService::characteristicReadFinished, this, &Nuki::onDeviceInfoCharacteristicReadFinished);

    // Keyturner service
    m_keyturnerService = m_bluetoothDevice->getService(keyturnerServiceUuid);
    if (!m_keyturnerService->hasCharacteristic(keyturnerUserDataCharacteristicUuid)) {
        qCWarning(dcNuki()) << "Could not find user data characteristc on device" << m_bluetoothDevice;
        return false;
    }
    if (!m_keyturnerService->hasCharacteristic(keyturnerDataCharacteristicUuid)) {
        qCWarning(dcNuki()) << "Could not find data characteristc on device" << m_bluetoothDevice;
        return false;
    }
    m_keyturnerUserDataCharacteristic = m_keyturnerService->getCharacteristic(keyturnerUserDataCharacteristicUuid);
    if (!m_keyturnerUserDataCharacteristic->startNotifications()) {
        qCWarning(dcNuki()) << "Could not enable notifications for user data characteristic.";
        return false;
    }
    m_keyturnerDataCharacteristic = m_keyturnerService->getCharacteristic(keyturnerDataCharacteristicUuid);
    if (!m_keyturnerDataCharacteristic->startNotifications()) {
        qCWarning(dcNuki()) << "Could not enable notifications for data characteristic.";
        return false;
    }

    // Pairing service
    m_pairingService = m_bluetoothDevice->getService(pairingServiceUuid);
    if (!m_pairingService->hasCharacteristic(pairingDataCharacteristicUuid)) {
        qCWarning(dcNuki()) << "Could not find pairing data characteristc on device" << m_bluetoothDevice;
        return false;
    }
    m_pairingDataCharacteristic = m_pairingService->getCharacteristic(pairingDataCharacteristicUuid);
    if (!m_pairingDataCharacteristic->startNotifications()) {
        qCWarning(dcNuki()) << "Could not enable notifications for pairing characteristic.";
        return false;
    }

    // Create authenticator
    if (m_nukiAuthenticator) {
        delete m_nukiAuthenticator;
        m_nukiAuthenticator = nullptr;
    }

    m_nukiAuthenticator = new NukiAuthenticator(m_bluetoothDevice->hostInfo(), m_pairingDataCharacteristic, this);
    connect(m_nukiAuthenticator, &NukiAuthenticator::errorOccured, this, &Nuki::onAuthenticationError);
    connect(m_nukiAuthenticator, &NukiAuthenticator::authenticationProcessFinished, this, &Nuki::onAuthenticationFinished);

    // Create nuki handler for encrypted communication
    if (m_nukiController) {
        delete m_nukiController;
        m_nukiController = nullptr;
    }

    m_nukiController = new NukiController(m_nukiAuthenticator, m_keyturnerUserDataCharacteristic, this);
    connect(m_nukiController, &NukiController::readNukiStatesFinished, this, &Nuki::onNukiReadStatesFinished);
    connect(m_nukiController, &NukiController::lockFinished, this, &Nuki::finishCurrentAction);
    connect(m_nukiController, &NukiController::unlockFinished, this, &Nuki::finishCurrentAction);
    connect(m_nukiController, &NukiController::unlatchFinished, this, &Nuki::finishCurrentAction);
    connect(m_nukiController, &NukiController::nukiStatesChanged, this, &Nuki::onNukiStatesChanged);

    return true;
}

void Nuki::clean()
{
    // Reset properties
    m_hardwareRevision = QString();
    m_serialNumber = QString();
    m_firmwareRevision = QString();
    m_initUuidsToRead.clear();

    finishCurrentAction(false);

    // Forget all services and characteristics
    if (m_deviceInformationService) {
        disconnect(m_deviceInformationService, &BluetoothGattService::characteristicReadFinished, this, &Nuki::onDeviceInfoCharacteristicReadFinished);
        m_deviceInformationService = nullptr;
    }

    m_keyturnerService = nullptr;
    m_keyturnerDataCharacteristic = nullptr;
    m_keyturnerUserDataCharacteristic = nullptr;

    m_pairingService = nullptr;
    m_pairingDataCharacteristic = nullptr;

    // Delete handler
    if (m_nukiController) {
        delete m_nukiController;
        m_nukiController = nullptr;
    }

    // Note: delete the authenticator after the handler
    if (m_nukiAuthenticator) {
        delete m_nukiAuthenticator;
        m_nukiAuthenticator = nullptr;
    }
}

void Nuki::finishCurrentAction(bool success)
{
    m_nukiAction = NukiActionNone;
    if (m_actionInfo.isNull())
        return;

    m_actionInfo->finish(success ? Device::DeviceErrorNoError : Device::DeviceErrorHardwareFailure);
    m_actionInfo.clear();
}

void Nuki::setAvailable(bool available)
{
    if (m_available == available)
        return;

    m_available = available;
    emit availableChanged(m_available);

    qCDebug(dcNuki()) << "Bluetooth device" << m_bluetoothDevice->name() << "is now" << (m_available ? "available" : "unavailable");

    if (m_available) {
        executeCurrentAction();
    } else {
        // Finish any running actions
        finishCurrentAction(false);

        // Finish possible running pairing transations
        if (!m_pairingId.isNull()) {
            qCWarning(dcNuki()) << "Cancel authentication process because of disconnection.";
            emit authenticationProcessFinished(m_pairingId, false);
            m_pairingId = PairingTransactionId();
        }
    }

    if (!m_device)
        return;

    m_device->setStateValue(nukiConnectedStateTypeId, m_available);
}
