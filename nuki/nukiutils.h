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

#ifndef NUKIUTILS_H
#define NUKIUTILS_H

#include <QObject>
#include <QString>
#include <QByteArray>
#include <QBitArray>

class NukiUtils
{
    Q_GADGET
public:
    enum StatusCode {
        StatusCodeCompeted      = 0x00,
        StatusCodeAccepted      = 0x01
    };
    Q_ENUM(StatusCode)


    enum ErrorCode {
        ErrorCodeNoError                = 0x00,

        // Pairing service error codes
        ErrorCodeNotInPairingMode       = 0x10,
        ErrorCodeBadAuthenticator       = 0x11,
        ErrorCodeBadPairingParameter    = 0x12,
        ErrorCodeMaxUsersReached        = 0x13,

        // Key turner service error codes
        ErrorCodeNotAuthorized          = 0x20,
        ErrorCodeBadPin                 = 0x21,
        ErrorCodeBadNonce               = 0x22,
        ErrorCodeBadParameter           = 0x23,
        ErrorCodeInvalidAuthId          = 0x24,
        ErrorCodeDisabled               = 0x25,
        ErrorCodeRemoteNotAllowed       = 0x26,
        ErrorCodeTimeNotAllowed         = 0x27,
        ErrorCodeTooManyPinAttempts     = 0x28,
        ErrorCodeAutoUnlockTooRecent    = 0x40,
        ErrorCodePositionUnknown        = 0x41,
        ErrorCodeMotorBlocked           = 0x42,
        ErrorCodeClutchFailure          = 0x43,
        ErrorCodeMotorTimeout           = 0x44,
        ErrorCodeBusy                   = 0x45,

        // General error codes
        ErrorCodeBadCrc                 = 0xFD,
        ErrorCodeBadLength              = 0xFE,
        ErrorCodeUnknown                = 0xFF
    };
    Q_ENUM(ErrorCode)


    enum LockState {
        LockStateUncalibrated           = 0x00,
        LockStateLocked                 = 0x01,
        LockStateUnlocking              = 0x02,
        LockStateUnlocked               = 0x03,
        LockStateLocking                = 0x04,
        LockStateUnlatched              = 0x05,
        LockStateUnlockedLocknGoActive  = 0x06,
        LockStateUnlatching             = 0x07,
        LockStateMotorBlocked           = 0xfe,
        LockStateUndefined              = 0xff
    };
    Q_ENUM(LockState)


    enum LockAction {
        LockActionUnlock                = 0x01,
        LockActionLock                  = 0x02,
        LockActionUnlatch               = 0x03,
        LockActionLockNGo               = 0x04,
        LockActionLockNGoWithUnlatch    = 0x05,
        LockActionFobAction1            = 0x81,
        LockActionFobAction2            = 0x82,
        LockActionFobAction3            = 0x83
    };
    Q_ENUM(LockAction)


    enum LockTrigger {
        LockTriggerBluetooth    = 0x00,
        LockTriggerManual       = 0x01,
        LockTriggerButton       = 0x02
    };
    Q_ENUM(LockTrigger)


    enum NukiState {
        NukiStateUninitialized  = 0x00,
        NukiStatePairingMode    = 0x01,
        NukiStateDoorMode       = 0x02
    };
    Q_ENUM(NukiState)


    enum Command {
        CommandRequestData                  = 0x0001,
        CommandPublicKey                    = 0x0003,
        CommandChallenge                    = 0x0004,
        CommandAuthorizationAuthenticator   = 0x0005,
        CommandAuthorizationData            = 0x0006,
        CommandAuthorizationId              = 0x0007,
        CommandRemoveUserAuthorization      = 0x0008,
        CommandRequestAuthorizationEntries  = 0x0009,
        CommandAuthorizationEntry           = 0x000A,
        CommandAuthorizationDataInvite      = 0x000B,
        CommandNukiStates                   = 0x000C,
        CommandLockAction                   = 0x000D,
        CommandStatus                       = 0x000E,
        CommandMostRecentCommand            = 0x000F,
        CommandOpeningsClosingsSummary      = 0x0010,
        CommandBatteryReport                = 0x0011,
        CommandErrorReport                  = 0x0012,
        CommandSetConfig                    = 0x0013,
        CommandRequestConfig                = 0x0014,
        CommandConfig                       = 0x0015,
        CommandSetSecurityPIN               = 0x0019,
        CommandRequestCalibration           = 0x001A,
        CommandRequestReboot                = 0x001D,
        CommandAuthorizationIdConfirmation  = 0x001E,
        CommandAuthorizationIdInvite        = 0x001F,
        CommandVerifySecurityPIN            = 0x0020,
        CommandUpdateTime                   = 0x0021,
        CommandUpdateUserAuthorization      = 0x0025,
        CommandAuthorizationEntryCount      = 0x0027,
        CommandRequestDisconnect            = 0x0030,
        CommandRequestLogEntries            = 0x0031,
        CommandLogEntry                     = 0x0032,
        CommandLogEntryCount                = 0x0033,
        CommandEnableLogging                = 0x0034,
        CommandSetAdvancedConG              = 0x0035,
        CommandRequestAdvancedConG          = 0x0036,
        CommandAdvancedConG                 = 0x0037,
        CommandAddTimeControlEntry          = 0x0039,
        CommandTimeControlEntryId           = 0x003A,
        CommandRemoveTimeControlEntry       = 0x003B,
        CommandRequestTimeControlEntries    = 0x003C,
        CommandTimeControlEntryCount        = 0x003D,
        CommandTimeControlEntry             = 0x003E,
        CommandUpdateTimeControlEntry       = 0x003F
    };
    Q_ENUM(Command)

    // Data helpers
    static QString convertByteToHexString(const quint8 &byte);
    static QString convertByteArrayToHexString(const QByteArray &byteArray);
    static QString convertByteArrayToHexStringCompact(const QByteArray &byteArray);
    static QString convertUint16ToHexString(const quint16 &value);

    static QByteArray converUint32ToByteArrayLittleEndian(const quint32 &value);
    static QByteArray converUint16ToByteArrayLittleEndian(const quint16 &value);
    static quint16 convertByteArrayToUint16BigEndian(const QByteArray &littleEndianByteArray);
    static quint32 convertByteArrayToUint32BigEndian(const QByteArray &littleEndianByteArray);

    // Crc calculation
    static quint16 calculateCrc(const QByteArray &data);
    static bool validateMessageCrc(const QByteArray &message);

    // Message helper
    static QByteArray createRequestMessageForUnencrypted(NukiUtils::Command command, const QByteArray &payload);
    static QByteArray createRequestMessageForUnencryptedForEncryption(quint32 authenticationId, NukiUtils::Command command, const QByteArray &payload = QByteArray());

};

#endif // NUKIUTILS_H
