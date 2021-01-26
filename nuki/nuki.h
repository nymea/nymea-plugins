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

#ifndef NUKI_H
#define NUKI_H

#include <QObject>
#include <QBluetoothUuid>
#include <QByteArray>
#include <QUuid>
#include <QPointer>

#include "typeutils.h"
#include "integrations/thing.h"
#include "bluez/bluetoothdevice.h"
#include "integrations/thingactioninfo.h"

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

    explicit Nuki(Thing *thing, BluetoothDevice *bluetoothDevice, QObject *parent = nullptr);

    Thing *thing();
    BluetoothDevice *bluetoothDevice();

    bool startAuthenticationProcess(const PairingTransactionId &pairingTransactionId);
    bool refreshStates();
    bool executeDeviceAction(NukiAction action, ThingActionInfo *actionInfo);

    void connectDevice();
    void disconnectDevice();

    void clearSettings();


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
