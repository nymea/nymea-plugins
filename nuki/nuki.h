// SPDX-License-Identifier: GPL-3.0-or-later

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*
* Copyright (C) 2013 - 2024, nymea GmbH
* Copyright (C) 2024 - 2025, chargebyte austria GmbH
*
* This file is part of nymea-plugins.
*
* nymea-plugins is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* (at your option) any later version.
*
* nymea-plugins is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
* General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with nymea-plugins. If not, see <https://www.gnu.org/licenses/>.
*
* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

#ifndef NUKI_H
#define NUKI_H

#include <QObject>
#include <QBluetoothUuid>
#include <QByteArray>
#include <QUuid>
#include <QPointer>

#include <typeutils.h>
#include <integrations/thing.h>
#include <integrations/thingactioninfo.h>

#include "nukiutils.h"
#include "nukicontroller.h"
#include "nukiauthenticator.h"

#include "bluez/bluetoothdevice.h"

//#include "nacl-20110221/crypto_auth/"

class Nuki : public QObject
{
    Q_OBJECT

public:
    enum NukiAction {
        NukiActionNone,
        NukiActionAuthenticate,
        NukiActionRefresh,
        NukiActionLock,
        NukiActionUnlock,
        NukiActionUnlatch
    };
    Q_ENUM(NukiAction)

    explicit Nuki(Thing *thing, BluetoothDevice *bluetoothDevice, QObject *parent = nullptr);

    Thing *thing();
    BluetoothDevice *bluetoothDevice();

    bool startAuthenticationProcess(const PairingTransactionId &pairingTransactionId);
    bool refreshStates();
    bool executeDeviceAction(NukiAction action, ThingActionInfo *actionInfo);

    void connectDevice();
    void disconnectDevice();

    void clearSettings();

    static QBluetoothUuid initializationServiceUuid() { return QBluetoothUuid(QUuid("a92ee000-5501-11e4-916c-0800200c9a66")); }
    static QBluetoothUuid pairingServiceUuid() { return QBluetoothUuid(QUuid("a92ee100-5501-11e4-916c-0800200c9a66")); }
    static QBluetoothUuid pairingDataCharacteristicUuid() { return QBluetoothUuid(QUuid("a92ee101-5501-11e4-916c-0800200c9a66")); }
    static QBluetoothUuid keyturnerServiceUuid() { return QBluetoothUuid(QUuid("a92ee200-5501-11e4-916c-0800200c9a66")); }
    static QBluetoothUuid keyturnerDataCharacteristicUuid() { return QBluetoothUuid(QUuid("a92ee201-5501-11e4-916c-0800200c9a66")); }
    static QBluetoothUuid keyturnerUserDataCharacteristicUuid() { return QBluetoothUuid(QUuid("a92ee202-5501-11e4-916c-0800200c9a66")); }

private:
    Thing *m_thing = nullptr;
    BluetoothDevice *m_bluetoothDevice = nullptr;
    NukiController *m_nukiController = nullptr;
    NukiAuthenticator *m_nukiAuthenticator = nullptr;

    BluetoothGattService *m_deviceInformationService = nullptr;
    BluetoothGattService *m_initializationService = nullptr;
    BluetoothGattService *m_pairingService = nullptr;
    BluetoothGattService *m_keyturnerService = nullptr;

    BluetoothGattCharacteristic *m_pairingDataCharacteristic = nullptr;
    BluetoothGattCharacteristic *m_keyturnerDataCharacteristic = nullptr;
    BluetoothGattCharacteristic *m_keyturnerUserDataCharacteristic = nullptr;

    // Device information
    QList<QBluetoothUuid> m_initUuidsToRead;
    QString m_serialNumber;
    QString m_hardwareRevision;
    QString m_firmwareRevision;

    bool m_available = false;

    NukiAction m_nukiAction = NukiActionNone;
    QPointer<ThingActionInfo> m_actionInfo;
    PairingTransactionId m_pairingId;

    bool init();
    void clean();
    bool executeNukiAction(NukiAction action);

    void setAvailable(bool available);
    void printServices();
    void readDeviceInformationCharacteristics();

    void executeCurrentAction();
    bool enableNotificationsIndications(BluetoothGattCharacteristic *characteristic);

signals:
    void availableChanged(bool available);
    void authenticationProcessFinished(const PairingTransactionId &pairingId, bool success);
    void stateChanged(NukiUtils::LockState lockState);

private slots:
    // Bluetooth thing
    void onBluetoothDeviceStateChanged(const BluetoothDevice::State &state);

    // DeviceInfo service
    void onDeviceInfoCharacteristicReadFinished(BluetoothGattCharacteristic *characteristic, const QByteArray &value);

    // Nuki Authenticator
    void onAuthenticationError(NukiUtils::ErrorCode error);
    void onAuthenticationFinished(bool success);

    // Nuki controller
    void onNukiReadStatesFinished(bool success);
    void finishCurrentAction(bool success);
    void onNukiStatesChanged();

};

#endif // NUKI_H
