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

#ifndef NUKICONTROLLER_H
#define NUKICONTROLLER_H

#include <QObject>
#include <QDateTime>
#include <QBluetoothAddress>

#include "nukiutils.h"
#include "nukiauthenticator.h"
#include "bluez/bluetoothgattcharacteristic.h"

class NukiController : public QObject
{
    Q_OBJECT
public:
    enum NukiControllerState {
        NukiControllerStateIdle,

        // Read state
        NukiControllerStateReadingLockStates,

        // Read configuration
        NukiControllerStateReadingConfigurationRequestChallange,
        NukiControllerStateReadingConfigurationExecute,
        NukiControllerStateReadingConfiguration,

        // Lock action
        NukiControllerStateLockActionRequestChallange,
        NukiControllerStateLockActionExecute,
        NukiControllerStateLockActionAccepted,

        // Unlock action
        NukiControllerStateUnlockActionRequestChallange,
        NukiControllerStateUnlockActionExecute,
        NukiControllerStateUnlockActionAccepted,

        // Unlatch action
        NukiControllerStateUnlatchActionRequestChallange,
        NukiControllerStateUnlatchActionExecute,
        NukiControllerStateUnlatchActionAccepted,

        NukiControllerStateError
    };
    Q_ENUM(NukiControllerState)

    explicit NukiController(NukiAuthenticator *nukiAuthenticator, BluetoothGattCharacteristic *userDataCharacteristic, QObject *parent = nullptr);

    // States
    NukiUtils::NukiState nukiState() const;
    NukiUtils::LockState nukiLockState() const;
    NukiUtils::LockTrigger nukiLockTrigger() const;
    QDateTime nukiDateTime() const;
    int nukiUtcOffset() const;
    bool batteryCritical() const;

    // Actions
    bool readLockState();
    bool readConfiguration();
    bool lock();
    bool unlock();
    bool unlatch();

private:
    NukiAuthenticator *m_nukiAuthenticator = nullptr;
    BluetoothGattCharacteristic *m_userDataCharacteristic = nullptr;

    NukiControllerState m_state = NukiControllerStateIdle;

    // Notification parsing helper
    QByteArray m_messageBuffer;
    QByteArray m_messageBufferAData;
    QByteArray m_messageBufferPData;
    QByteArray m_messageBufferNonce;
    quint32 m_messageBufferIdentifier = 0;
    quint16 m_messageBufferLength = 0;
    int m_messageBufferCounter = 0;

    // For development debugging
    bool m_debug = false;

    // Properties
    NukiUtils::NukiState m_nukiState = NukiUtils::NukiStateUninitialized;
    NukiUtils::LockState m_nukiLockState = NukiUtils::LockStateUndefined;
    NukiUtils::LockTrigger m_nukiLockTrigger = NukiUtils::LockTriggerBluetooth;
    QDateTime m_nukiDateTime;
    int m_nukiUtcOffset = 0;
    bool m_batteryCritical = false;

    QByteArray m_nukiNonce;

    // State machine helpers
    void setState(NukiControllerState state);
    void resetExpectedData(int expectedCount = 1);
    void resetMessageBuffer();

    // Data processors
    void processNukiStatesData(const QByteArray &data);
    void processNukiConfigData(const QByteArray &data);
    void processNukiErrorReport(const QByteArray &data);
    void processUserDataNotification(const QByteArray nonce, quint32 authorizationIdentifier, const QByteArray &privateData);

    // State action methods
    void sendReadLockStateRequest();
    void sendReadConfigurationRequest();
    void sendRequestChallengeRequest();
    void sendLockActionRequest(NukiUtils::LockAction lockAction, quint8 flag = 0);

signals:
    void stateChanged(NukiControllerState state);
    void readNukiStatesFinished(bool success);
    void lockFinished(bool success);
    void unlockFinished(bool success);
    void unlatchFinished(bool success);
    void nukiStatesChanged();

private slots:
    void onUserDataCharacteristicChanged(const QByteArray &value);

};

#endif // NUKICONTROLLER_H
