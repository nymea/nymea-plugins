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

#ifndef NUKI_H
#define NUKI_H

#include <QObject>
#include <QBluetoothUuid>
#include <QByteArray>
#include <QUuid>
#include <QPointer>

#include "typeutils.h"
#include "devices/device.h"
#include "bluez/bluetoothdevice.h"
#include "devices/deviceactioninfo.h"

#include "nukiutils.h"
#include "nukicontroller.h"
#include "nukiauthenticator.h"

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

    explicit Nuki(Device *device, BluetoothDevice *bluetoothDevice, QObject *parent = nullptr);

    Device *device();
    BluetoothDevice *bluetoothDevice();

    bool startAuthenticationProcess(const PairingTransactionId &pairingTransactionId);
    bool refreshStates();
    bool executeDeviceAction(NukiAction action, DeviceActionInfo *actionInfo);

    void connectDevice();
    void disconnectDevice();

    void clearSettings();


private:
    Device *m_device = nullptr;
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
    QPointer<DeviceActionInfo> m_actionInfo;
    PairingTransactionId m_pairingId;

    bool init();
    void clean();
    bool executeNukiAction(NukiAction action);

    void setAvailable(bool available);
    void printServices();
    void readDeviceInformationCharacteristics();

    void executeCurrentAction();

signals:
    void availableChanged(bool available);
    void authenticationProcessFinished(const PairingTransactionId &pairingId, bool success);
    void stateChanged(NukiUtils::LockState lockState);
    void actionFinished(const ActionId &action, bool success);

private slots:
    // Bluetooth device
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
