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
    void processNukiErrorReport(const QByteArray &data);
    void processUserDataNotification(const QByteArray nonce, quint32 authorizationIdentifier, const QByteArray &privateData);

    // State action methods
    void sendReadLockStateRequest();
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
