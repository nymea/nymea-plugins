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

Nuki::Nuki(Thing *thing, BluetoothDevice *bluetoothDevice, QObject *parent) :
    QObject(parent),
    m_thing(thing),
    m_bluetoothDevice(bluetoothDevice)
{
    connect(m_bluetoothDevice, &BluetoothDevice::stateChanged, this, &Nuki::onBluetoothDeviceStateChanged);
    onBluetoothDeviceStateChanged(m_bluetoothDevice->state());
}

Thing *Nuki::thing()
{
    return m_thing;
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

bool Nuki::executeDeviceAction(Nuki::NukiAction action, ThingActionInfo *actionInfo)
{
    if (m_nukiAction != NukiActionNone || !m_actionInfo.isNull()) {
        qCWarning(dcNuki()) << "Nuki is busy and already processing an action. Please retry again."  << m_nukiAction;
        return false;
    }

    m_actionInfo = QPointer<ThingActionInfo>(actionInfo);
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
            qCDebug(dcNuki()) << "    " << characteristic;
            foreach (BluetoothGattDescriptor *descriptor, characteristic->descriptors()) {
                qCDebug(dcNuki()) << "        " << descriptor;
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

bool Nuki::enableNotificationsIndications(BluetoothGattCharacteristic *characteristic)
{
    qCDebug(dcNuki()) << "Start enable notifications/indications for" << characteristic;

    // Get the client configuration descriptor
    // https://www.bluetooth.com/wp-content/uploads/Sitecore-Media-Library/Gatt/Xml/Descriptors/org.bluetooth.descriptor.gatt.client_characteristic_configuration.xml
    BluetoothGattDescriptor *clientConfiguration = characteristic->getDescriptor(QBluetoothUuid::ClientCharacteristicConfiguration);
    if (!clientConfiguration) {
        qCWarning(dcNuki()) << "Could not start notification/indications for" << characteristic << "because the client configuration descriptor is missing";
        return false;
    }

    qCDebug(dcNuki()) << "Enable notifications on" << characteristic;
    if (!characteristic->startNotifications()) {
        qCDebug(dcNuki()) << "Failed to start notifications on" << characteristic;
        return false;
    }

    if (characteristic->properties().testFlag(BluetoothGattCharacteristic::Indicate)) {
        qCDebug(dcNuki()) << "Enable indications on" << characteristic;
        QByteArray configuration;
        QDataStream stream(&configuration, QIODevice::WriteOnly);
        stream.setByteOrder(QDataStream::LittleEndian);
        stream << static_cast<quint16>(0x02);
        if (!clientConfiguration->writeValue(configuration)) {
            qCWarning(dcNuki()) << "Failed to write client configuration descriptor on" << characteristic;
            return false;
        }

    } else {
        qCWarning(dcNuki()) << "Could not enable notifications. Access properties do not allow indicate or notify" << characteristic;
        return false;
    }

    clientConfiguration->readValue();

    return true;
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
    qCDebug(dcNuki()) << "Read thing information characteristic finished" << characteristic->chararcteristicName() << qUtf8Printable(value);
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
        // Initial read done. Make thing available
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
    if (!m_thing)
        return;

    m_thing->setStateValue(nukiHardwareRevisionStateTypeId, m_hardwareRevision);
    m_thing->setStateValue(nukiFirmwareRevisionStateTypeId, m_firmwareRevision);
    m_thing->setStateValue(nukiBatteryCriticalStateTypeId, m_nukiController->batteryCritical());

    switch (m_nukiController->nukiLockTrigger()) {
    case NukiUtils::LockTriggerBluetooth:
        m_thing->setStateValue(nukiTriggerStateTypeId, "Bluetooth");
        break;
    case NukiUtils::LockTriggerButton:
        m_thing->setStateValue(nukiTriggerStateTypeId, "Button");
        break;
    case NukiUtils::LockTriggerManual:
        m_thing->setStateValue(nukiTriggerStateTypeId, "Manual");
        break;
    default:
        break;
    }

    switch (m_nukiController->nukiState()) {
    case NukiUtils::NukiStateDoorMode:
        m_thing->setStateValue(nukiModeStateTypeId, "Door");
        break;
    case NukiUtils::NukiStatePairingMode:
        m_thing->setStateValue(nukiModeStateTypeId, "Pairing");
        break;
    case NukiUtils::NukiStateUninitialized:
        m_thing->setStateValue(nukiModeStateTypeId, "Uninitialized");
        break;
    default:
        break;
    }

    switch (m_nukiController->nukiLockState()) {
    case NukiUtils::LockStateLocked:
        m_thing->setStateValue(nukiStateStateTypeId, "locked");
        m_thing->setStateValue(nukiStatusStateTypeId, "Ok");
        break;
    case NukiUtils::LockStateLocking:
        m_thing->setStateValue(nukiStateStateTypeId, "locking");
        m_thing->setStateValue(nukiStatusStateTypeId, "Ok");
        break;
    case NukiUtils::LockStateMotorBlocked:
        m_thing->setStateValue(nukiStatusStateTypeId, "Motor blocked");
        break;
    case NukiUtils::LockStateUncalibrated:
        m_thing->setStateValue(nukiStatusStateTypeId, "Uncalibrated");
        break;
    case NukiUtils::LockStateUndefined:
        m_thing->setStateValue(nukiStatusStateTypeId, "Undefined");
        break;
    case NukiUtils::LockStateUnlatched:
        m_thing->setStateValue(nukiStateStateTypeId, "unlatched");
        m_thing->setStateValue(nukiStatusStateTypeId, "Ok");
        break;
    case NukiUtils::LockStateUnlatching:
        m_thing->setStateValue(nukiStateStateTypeId, "unlatching");
        m_thing->setStateValue(nukiStatusStateTypeId, "Ok");
        break;
    case NukiUtils::LockStateUnlockedLocknGoActive:
        m_thing->setStateValue(nukiStatusStateTypeId, "unlocked");
        break;
    case NukiUtils::LockStateUnlocked:
        m_thing->setStateValue(nukiStateStateTypeId, "unlocked");
        m_thing->setStateValue(nukiStatusStateTypeId, "Ok");
        break;
    case NukiUtils::LockStateUnlocking:
        m_thing->setStateValue(nukiStateStateTypeId, "unlocking");
        m_thing->setStateValue(nukiStatusStateTypeId, "Ok");
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
    // Set key turner characteristics for data and user data
    if (!m_keyturnerService->hasCharacteristic(keyturnerDataCharacteristicUuid)) {
        qCWarning(dcNuki()) << "Could not find data characteristc on device" << m_bluetoothDevice;
        return false;
    }

    // Enable notifications/indications
    m_keyturnerUserDataCharacteristic = m_keyturnerService->getCharacteristic(keyturnerUserDataCharacteristicUuid);
    if (!enableNotificationsIndications(m_keyturnerUserDataCharacteristic)) {
        qCWarning(dcNuki()) << "Could not enable notifications/indications for user data characteristic.";
        return false;
    }

    m_keyturnerDataCharacteristic = m_keyturnerService->getCharacteristic(keyturnerDataCharacteristicUuid);
    if (!enableNotificationsIndications(m_keyturnerDataCharacteristic)) {
        qCWarning(dcNuki()) << "Could not enable notifications/indications for key turner data characteristic.";
        return false;
    }



    // Pairing service
    m_pairingService = m_bluetoothDevice->getService(pairingServiceUuid);
    if (!m_pairingService->hasCharacteristic(pairingDataCharacteristicUuid)) {
        qCWarning(dcNuki()) << "Could not find pairing data characteristc on device" << m_bluetoothDevice;
        return false;
    }

    m_pairingDataCharacteristic = m_pairingService->getCharacteristic(pairingDataCharacteristicUuid);
    if (!enableNotificationsIndications(m_pairingDataCharacteristic)) {
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

    m_actionInfo->finish(success ? Thing::ThingErrorNoError : Thing::ThingErrorHardwareFailure);
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

    if (!m_thing)
        return;

    m_thing->setStateValue(nukiConnectedStateTypeId, m_available);
}
