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

#include "nukicontroller.h"
#include "extern-plugininfo.h"

#include <QByteArray>
#include <QDataStream>

extern "C" {
#include "sodium.h"
}

NukiController::NukiController(NukiAuthenticator *nukiAuthenticator, BluetoothGattCharacteristic *userDataCharacteristic, QObject *parent) :
    QObject(parent),
    m_nukiAuthenticator(nukiAuthenticator),
    m_userDataCharacteristic(userDataCharacteristic)
{
#ifdef QT_DEBUG
    // Enable full debug messages containing sensible data for debug builds
    m_debug = true;
#endif

    connect(m_userDataCharacteristic, &BluetoothGattCharacteristic::valueChanged, this, &NukiController::onUserDataCharacteristicChanged);
}

NukiUtils::NukiState NukiController::nukiState() const
{
    return m_nukiState;
}

NukiUtils::LockState NukiController::nukiLockState() const
{
    return m_nukiLockState;
}

NukiUtils::LockTrigger NukiController::nukiLockTrigger() const
{
    return m_nukiLockTrigger;
}

bool NukiController::batteryCritical() const
{
    return m_batteryCritical;
}

bool NukiController::readLockState()
{
    if (m_state != NukiControllerStateIdle) {
        // TODO: maybe queue commands
        qCWarning(dcNuki()) << "Controller: Could not read lock state, Nuki is currenty busy";
        return false;
    }

    if (!m_nukiAuthenticator->isValid()) {
        qCWarning(dcNuki()) << "Invalid authenticator. Please authenticate the thing first.";
        return false;
    }

    setState(NukiControllerStateReadingLockStates);
    return true;
}

bool NukiController::readConfiguration()
{
    if (m_state != NukiControllerStateIdle) {
        // TODO: maybe queue commands
        qCWarning(dcNuki()) << "Controller: Could not read lock state, Nuki is currenty busy";
        return false;
    }

    if (!m_nukiAuthenticator->isValid()) {
        qCWarning(dcNuki()) << "Invalid authenticator. Please authenticate the thing first.";
        return false;
    }

    setState(NukiControllerStateReadingConfiguration);
    return true;
}

bool NukiController::lock()
{
    if (m_state != NukiControllerStateIdle) {
        // TODO: maybe queue commands
        qCWarning(dcNuki()) << "Controller: Could not lock, Nuki is currenty busy";
        return false;
    }

    if (!m_nukiAuthenticator->isValid()) {
        qCWarning(dcNuki()) << "Invalid authenticator. Please authenticate the thing first.";
        return false;
    }

    setState(NukiControllerStateLockActionRequestChallange);
    return true;
}

bool NukiController::unlock()
{
    if (m_state != NukiControllerStateIdle) {
        // TODO: maybe queue commands
        qCWarning(dcNuki()) << "Controller: Could not lock, Nuki is currenty busy";
        return false;
    }

    if (!m_nukiAuthenticator->isValid()) {
        qCWarning(dcNuki()) << "Invalid authenticator. Please authenticate the thing first.";
        return false;
    }

    setState(NukiControllerStateUnlockActionRequestChallange);
    return true;
}

bool NukiController::unlatch()
{
    if (m_state != NukiControllerStateIdle) {
        // TODO: maybe queue commands
        qCWarning(dcNuki()) << "Controller: Could not unlatch, Nuki is currenty busy";
        return false;
    }

    if (!m_nukiAuthenticator->isValid()) {
        qCWarning(dcNuki()) << "Invalid authenticator. Please authenticate the thing first.";
        return false;
    }

    setState(NukiControllerStateUnlatchActionRequestChallange);
    return true;
}

void NukiController::setState(NukiController::NukiControllerState state)
{
    if (m_state == state)
        return;

    m_state = state;

    qCDebug(dcNuki()) << m_state;

    switch (m_state) {
    case NukiControllerStateIdle:
        break;
    case NukiControllerStateReadingLockStates:
        sendReadLockStateRequest();
        break;
    case NukiControllerStateReadingConfigurationRequestChallange:
        sendRequestChallengeRequest();
        break;
    case NukiControllerStateReadingConfigurationExecute:
        sendReadConfigurationRequest();
        setState(NukiControllerStateReadingConfiguration);
        break;
    case NukiControllerStateReadingConfiguration:
        break;
    case NukiControllerStateLockActionRequestChallange:
        sendRequestChallengeRequest();
        break;
    case NukiControllerStateLockActionExecute:
        sendLockActionRequest(NukiUtils::LockActionLock);
        setState(NukiControllerStateLockActionAccepted);
        break;
    case NukiControllerStateLockActionAccepted:
        break;
    case NukiControllerStateUnlockActionRequestChallange:
        sendRequestChallengeRequest();
        break;
    case NukiControllerStateUnlockActionExecute:
        sendLockActionRequest(NukiUtils::LockActionUnlock);
        setState(NukiControllerStateUnlockActionAccepted);
        break;
    case NukiControllerStateUnlockActionAccepted:
        break;
    case NukiControllerStateUnlatchActionRequestChallange:
        sendRequestChallengeRequest();
        break;
    case NukiControllerStateUnlatchActionExecute:
        sendLockActionRequest(NukiUtils::LockActionUnlatch);
        setState(NukiControllerStateUnlatchActionAccepted);
        break;
    case NukiControllerStateUnlatchActionAccepted:
        break;
    default:
        break;
    }

    emit stateChanged(m_state);
}

void NukiController::resetMessageBuffer()
{
    m_messageBuffer.clear();
    m_messageBufferNonce.clear();
    m_messageBufferIdentifier = 0;
    m_messageBufferLength = 0;
    m_messageBufferCounter = 0;
}

void NukiController::processNukiStatesData(const QByteArray &data)
{
    quint8 nukiState = 0;
    quint8 nukiLockState = 0;
    quint8 nukiLockTrigger = 0;
    quint16 year = 1970;
    quint8 month = 1;
    quint8 day = 1;
    quint8 hour = 0;
    quint8 minute = 0;
    quint8 second = 0;
    qint16 utcOffset = 0;
    quint8 batteryCritical = 0;

    QByteArray payload = data;
    QDataStream stream(&payload, QIODevice::ReadOnly);
    stream.setByteOrder(QDataStream::LittleEndian);
    stream >> nukiState >> nukiLockState >> nukiLockTrigger >> year >> month >> day >> hour >> minute >> second >> utcOffset >> batteryCritical;

    m_nukiState = static_cast<NukiUtils::NukiState>(nukiState);
    m_nukiLockState = static_cast<NukiUtils::LockState>(nukiLockState);
    m_nukiLockTrigger = static_cast<NukiUtils::LockTrigger>(nukiLockTrigger);
    m_nukiDateTime = QDateTime(QDate(year, month, day), QTime(hour, minute, second));
    m_nukiUtcOffset = utcOffset;
    m_batteryCritical = (batteryCritical == 0 ? false : true);

    if (m_debug) qCDebug(dcNuki()) << "--------------------:" << m_state;
    if (m_debug) qCDebug(dcNuki()) << "    Nuki state      :" << m_nukiState;
    if (m_debug) qCDebug(dcNuki()) << "    Nuki lock state :" << m_nukiLockState;
    if (m_debug) qCDebug(dcNuki()) << "    Lock trigger    :" << m_nukiLockTrigger;
    if (m_debug) qCDebug(dcNuki()) << "    Date time       :" << m_nukiDateTime.toString("dd.MM.yyyy hh:mm:ss") << "UTC offset:" << m_nukiUtcOffset;
    if (m_debug) qCDebug(dcNuki()) << "    Battery critical:" << m_batteryCritical;

    qCDebug(dcNuki()) << "Nuki states refreshed.";

    emit nukiStatesChanged();
}

void NukiController::processNukiConfigData(const QByteArray &data)
{
    qCDebug(dcNuki()) << "Processing config data from nuki" << data;

}

void NukiController::processNukiErrorReport(const QByteArray &data)
{
    qint8 errorCode;
    quint16 nukiCommand;

    QByteArray payload = data;
    QDataStream stream(&payload, QIODevice::ReadOnly);
    stream.setByteOrder(QDataStream::LittleEndian);
    stream >> errorCode >> nukiCommand;

    qCDebug(dcNuki()) << "Received error report" << static_cast<NukiUtils::ErrorCode>(errorCode) << static_cast<NukiUtils::Command>(nukiCommand);
}

void NukiController::processUserDataNotification(const QByteArray nonce, quint32 authorizationIdentifier, const QByteArray &privateData)
{
    QByteArray decryptedMessage = m_nukiAuthenticator->decryptData(privateData, nonce);

    // Process decrypted data
    if (!NukiUtils::validateMessageCrc(decryptedMessage)) {
        qCWarning(dcNuki()) << "Controller: User notification data has invalid CRC CCITT value. Rejecting data.";
        return;
    }

    // We have the unencrypted and valid PDATA, let's see what this is
    quint32 decryptedAuthenticationId = NukiUtils::convertByteArrayToUint32BigEndian(decryptedMessage.left(4));
    NukiUtils::Command command = static_cast<NukiUtils::Command>(NukiUtils::convertByteArrayToUint16BigEndian(decryptedMessage.mid(4, 2)));
    QByteArray payload = decryptedMessage.mid(6, decryptedMessage.length() - 8);

    qCDebug(dcNuki()) << "Controller: Processing notification" << command;

    if (m_debug) qCDebug(dcNuki()) << "    Nonce           :" << NukiUtils::convertByteArrayToHexStringCompact(nonce);
    if (m_debug) qCDebug(dcNuki()) << "    Authorization ID:" << authorizationIdentifier;
    if (m_debug) qCDebug(dcNuki()) << "    Encrypted data  :" << NukiUtils::convertByteArrayToHexStringCompact(privateData) << privateData.length();
    if (m_debug) qCDebug(dcNuki()) << "    Decrypted data  :" << NukiUtils::convertByteArrayToHexStringCompact(decryptedMessage) << decryptedMessage.length();
    if (m_debug) qCDebug(dcNuki()) << "    Command         :" << command;
    if (m_debug) qCDebug(dcNuki()) << "    Authorization ID:" << NukiUtils::convertByteArrayToHexStringCompact(decryptedMessage.left(4)) << decryptedAuthenticationId;
    if (m_debug) qCDebug(dcNuki()) << "    Payload         :" << NukiUtils::convertByteArrayToHexStringCompact(payload);

    // Lets see if this was an expected
    switch (m_state) {
    case NukiControllerStateReadingLockStates:
        // We are expecting the states notification
        if (command == NukiUtils::CommandNukiStates) {
            processNukiStatesData(payload);
            emit readNukiStatesFinished(true);
            // TODO: read configuration
            setState(NukiControllerStateIdle);
            return;
        }
        break;
    case NukiControllerStateReadingConfigurationRequestChallange:
        if (command == NukiUtils::CommandChallenge) {
            m_nukiNonce = payload;
            setState(NukiControllerStateReadingConfigurationExecute);
            return;
        }
        break;
    case NukiControllerStateReadingConfiguration:
        if (command == NukiUtils::CommandConfig) {
            processNukiConfigData(payload);
            setState(NukiControllerStateIdle);
            return;
        }
        break;
    case NukiControllerStateLockActionRequestChallange:
        // We are expecting callange message
        if (command == NukiUtils::CommandChallenge) {
            m_nukiNonce = payload;
            setState(NukiControllerStateLockActionExecute);
            return;
        }
        break;
    case NukiControllerStateLockActionAccepted:
        // We are expecting the status
        if (command == NukiUtils::CommandStatus) {
            NukiUtils::StatusCode statusCode = static_cast<NukiUtils::StatusCode>((quint8)payload.at(0));
            qCDebug(dcNuki()) << "Controller:" << statusCode;

            switch (statusCode) {
            case NukiUtils::StatusCodeAccepted:
                // Lets wait for completed
                break;
            case NukiUtils::StatusCodeCompeted:
                emit lockFinished(true);
                setState(NukiControllerStateIdle);
                break;
            default:
                break;
            }
        }
        break;
    case NukiControllerStateUnlockActionRequestChallange:
        // We are expecting callenge message
        if (command == NukiUtils::CommandChallenge) {
            m_nukiNonce = payload;
            setState(NukiControllerStateUnlockActionExecute);
            return;
        }
        break;
    case NukiControllerStateUnlockActionAccepted:
        // We are expecting the status
        if (command == NukiUtils::CommandStatus) {
            NukiUtils::StatusCode statusCode = static_cast<NukiUtils::StatusCode>((quint8)payload.at(0));
            qCDebug(dcNuki()) << "Controller:" << statusCode;

            switch (statusCode) {
            case NukiUtils::StatusCodeAccepted:
                // Lets wait for completed
                break;
            case NukiUtils::StatusCodeCompeted:
                emit unlockFinished(true);
                setState(NukiControllerStateIdle);
                break;
            default:
                break;
            }
        }
        break;
    case NukiControllerStateUnlatchActionRequestChallange:
        // We are expecting callenge message
        if (command == NukiUtils::CommandChallenge) {
            m_nukiNonce = payload;
            setState(NukiControllerStateUnlatchActionExecute);
            return;
        }
        break;
    case NukiControllerStateUnlatchActionAccepted:
        // We are expecting the status
        if (command == NukiUtils::CommandStatus) {
            NukiUtils::StatusCode statusCode = static_cast<NukiUtils::StatusCode>((quint8)payload.at(0));
            qCDebug(dcNuki()) << "Controller:" << statusCode;

            switch (statusCode) {
            case NukiUtils::StatusCodeAccepted:
                // Lets wait for completed
                break;
            case NukiUtils::StatusCodeCompeted:
                emit unlatchFinished(true);
                setState(NukiControllerStateIdle);
                break;
            default:
                break;
            }
        }
        break;
    default:
        break;
    }

    // Other notification
    switch (command) {
    case NukiUtils::CommandNukiStates:
        processNukiStatesData(payload);
        break;
    case NukiUtils::CommandErrorReport:
        processNukiErrorReport(payload);
        break;
    case NukiUtils::CommandStatus: {
        NukiUtils::StatusCode statusCode = static_cast<NukiUtils::StatusCode>((quint8)payload.at(0));
        qCDebug(dcNuki()) << "Controller:" << statusCode;
        break;
    }
    default:
        qCWarning(dcNuki()) << "Controller: Received unhandled notification:" << command;
        break;
    }
}

void NukiController::sendReadLockStateRequest()
{
    qCDebug(dcNuki()) << "Controller: Reading lock state";

    // Create data for encryption
    QByteArray payload;
    QDataStream stream(&payload, QIODevice::WriteOnly);
    stream.setByteOrder(QDataStream::LittleEndian);
    stream << static_cast<quint16>(NukiUtils::CommandNukiStates);

    // Create unencrypted PDATA
    QByteArray unencryptedMessage = NukiUtils::createRequestMessageForUnencryptedForEncryption(m_nukiAuthenticator->authorizationId(), NukiUtils::CommandRequestData, payload);

    // Encrypt PDATA
    QByteArray nonce = m_nukiAuthenticator->generateNonce(crypto_box_NONCEBYTES);
    QByteArray encryptedMessage = m_nukiAuthenticator->encryptData(unencryptedMessage, nonce);

    // Create ADATA
    QByteArray header;
    header.append(nonce);
    header.append(m_nukiAuthenticator->authorizationIdRawData());
    header.append(NukiUtils::converUint16ToByteArrayLittleEndian(static_cast<quint16>(encryptedMessage.length())));

    // Message ADATA + PDATA
    QByteArray message;
    message.append(header);
    message.append(encryptedMessage);

    // Send data
    qCDebug(dcNuki()) << "Controller: Sending read lock states request";
    if (m_debug) qCDebug(dcNuki()) << "    Nonce          :" << NukiUtils::convertByteArrayToHexStringCompact(nonce);
    if (m_debug) qCDebug(dcNuki()) << "    Header         :" << NukiUtils::convertByteArrayToHexStringCompact(header);
    if (m_debug) qCDebug(dcNuki()) << "Controller: -->" << NukiUtils::convertByteArrayToHexStringCompact(message);
    m_userDataCharacteristic->writeCharacteristic(message);
}

void NukiController::sendReadConfigurationRequest()
{
    qCDebug(dcNuki()) << "Controller: Reading configurations";

    // Create data for encryption
    QByteArray payload;
    QDataStream stream(&payload, QIODevice::WriteOnly);
    stream.setByteOrder(QDataStream::LittleEndian);
    stream << static_cast<quint16>(NukiUtils::CommandRequestConfig);
    for (int i = 0; i < m_nukiNonce.length(); i++) {
        stream << static_cast<quint8>(m_nukiNonce.at(i));
    }

    // Create unencrypted PDATA
    QByteArray unencryptedMessage = NukiUtils::createRequestMessageForUnencryptedForEncryption(m_nukiAuthenticator->authorizationId(), NukiUtils::CommandRequestData, payload);

    // Encrypt PDATA
    QByteArray nonce = m_nukiAuthenticator->generateNonce(crypto_box_NONCEBYTES);
    QByteArray encryptedMessage = m_nukiAuthenticator->encryptData(unencryptedMessage, nonce);

    // Create ADATA
    QByteArray header;
    header.append(nonce);
    header.append(m_nukiAuthenticator->authorizationIdRawData());
    header.append(NukiUtils::converUint16ToByteArrayLittleEndian(static_cast<quint16>(encryptedMessage.length())));

    // Message ADATA + PDATA
    QByteArray message;
    message.append(header);
    message.append(encryptedMessage);

    // Send data
    qCDebug(dcNuki()) << "Controller: Sending get config request";
    if (m_debug) qCDebug(dcNuki()) << "    Nonce          :" << NukiUtils::convertByteArrayToHexStringCompact(nonce);
    if (m_debug) qCDebug(dcNuki()) << "    Header         :" << NukiUtils::convertByteArrayToHexStringCompact(header);
    if (m_debug) qCDebug(dcNuki()) << "Controller: -->" << NukiUtils::convertByteArrayToHexStringCompact(message);
    m_userDataCharacteristic->writeCharacteristic(message);
}

void NukiController::sendRequestChallengeRequest()
{
    qCDebug(dcNuki()) << "Controller: Request challenge";

    // Create data for encryption
    QByteArray payload;
    QDataStream stream(&payload, QIODevice::WriteOnly);
    stream.setByteOrder(QDataStream::LittleEndian);
    stream << static_cast<quint16>(NukiUtils::CommandChallenge);

    // Create unencrypted PDATA
    QByteArray unencryptedMessage = NukiUtils::createRequestMessageForUnencryptedForEncryption(m_nukiAuthenticator->authorizationId(), NukiUtils::CommandRequestData, payload);

    // Encrypt PDATA
    QByteArray nonce = m_nukiAuthenticator->generateNonce(crypto_box_NONCEBYTES);
    QByteArray encryptedMessage = m_nukiAuthenticator->encryptData(unencryptedMessage, nonce);

    // Create ADATA
    QByteArray header;
    header.append(nonce);
    header.append(m_nukiAuthenticator->authorizationIdRawData());
    header.append(NukiUtils::converUint16ToByteArrayLittleEndian(static_cast<quint16>(encryptedMessage.length())));

    // Message ADATA + PDATA
    QByteArray message;
    message.append(header);
    message.append(encryptedMessage);

    // Send data
    qCDebug(dcNuki()) << "Controller: Sending challange request";
    if (m_debug) qCDebug(dcNuki()) << "    Nonce          :" << NukiUtils::convertByteArrayToHexStringCompact(nonce);
    if (m_debug) qCDebug(dcNuki()) << "    Header         :" << NukiUtils::convertByteArrayToHexStringCompact(header);
    if (m_debug) qCDebug(dcNuki()) << "Controller: -->" << NukiUtils::convertByteArrayToHexStringCompact(message);
    m_userDataCharacteristic->writeCharacteristic(message);
}

void NukiController::sendLockActionRequest(NukiUtils::LockAction lockAction, quint8 flag)
{
    qCDebug(dcNuki()) << "Controller: Send lock request" << lockAction;

    QByteArray nonce = m_nukiAuthenticator->generateNonce(crypto_box_NONCEBYTES);

    // Create data for encryption
    QByteArray payload;
    QDataStream stream(&payload, QIODevice::WriteOnly);
    stream.setByteOrder(QDataStream::LittleEndian);
    stream << static_cast<quint8>(lockAction);
    stream << static_cast<quint32>(m_nukiAuthenticator->authorizationId());
    stream << flag;

    for (int i = 0; i < m_nukiNonce.length(); i++) {
        stream << static_cast<quint8>(m_nukiNonce.at(i));
    }

    // Create unencrypted PDATA
    QByteArray unencryptedMessage = NukiUtils::createRequestMessageForUnencryptedForEncryption(m_nukiAuthenticator->authorizationId(), NukiUtils::CommandLockAction, payload);

    // Encrypt PDATA
    QByteArray encryptedMessage = m_nukiAuthenticator->encryptData(unencryptedMessage, nonce);

    // Create ADATA
    QByteArray header;
    header.append(nonce);
    header.append(m_nukiAuthenticator->authorizationIdRawData());
    header.append(NukiUtils::converUint16ToByteArrayLittleEndian(static_cast<quint16>(encryptedMessage.length())));

    // Message ADATA + PDATA
    QByteArray message;
    message.append(header);
    message.append(encryptedMessage);

    // Send data
    qCDebug(dcNuki()) << "Controller: Sending lock request";
    if (m_debug) qCDebug(dcNuki()) << "    Nonce          :" << NukiUtils::convertByteArrayToHexStringCompact(nonce);
    if (m_debug) qCDebug(dcNuki()) << "    Header         :" << NukiUtils::convertByteArrayToHexStringCompact(header);
    if (m_debug) qCDebug(dcNuki()) << "Controller: -->" << NukiUtils::convertByteArrayToHexStringCompact(message);

    m_userDataCharacteristic->writeCharacteristic(message);
}

void NukiController::onUserDataCharacteristicChanged(const QByteArray &value)
{
    if (m_debug) qCDebug(dcNuki()) << "Controller: Data received: <--" << NukiUtils::convertByteArrayToHexStringCompact(value);

    m_messageBuffer = value;

    if (m_messageBuffer.count() < 30) {
        qCWarning(dcNuki()) << "Controller: Cannot understand message. Rejecting.";
        resetMessageBuffer();
        return;
    }

    // Parse message length
    // ADATA: 24 byte nonce, 4 byte autorization, 2 byte encrypted message length
    m_messageBufferAData = m_messageBuffer.left(30);
    m_messageBufferPData = m_messageBuffer.right(m_messageBuffer.count() - 30);
    m_messageBufferNonce = m_messageBufferAData.left(24);
    QByteArray messageInformation = m_messageBufferAData.right(6);

    QDataStream stream(&messageInformation, QIODevice::ReadOnly);
    stream.setByteOrder(QDataStream::LittleEndian);
    stream >> m_messageBufferIdentifier >> m_messageBufferLength;
    if (m_messageBufferPData.count() == m_messageBufferLength) {
        processUserDataNotification(m_messageBufferNonce, m_messageBufferIdentifier, m_messageBufferPData);
        resetMessageBuffer();
    }

}
