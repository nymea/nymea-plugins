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

#include "nukiauthenticator.h"

#include "nukiutils.h"
#include "nymeasettings.h"
#include "extern-plugininfo.h"

extern "C" {
#include "sodium.h"
#include "sodium/crypto_box.h"
#include "sodium/crypto_secretbox.h"
}

#include <QtEndian>
#include <QSettings>
#include <QDataStream>

NukiAuthenticator::NukiAuthenticator(const QBluetoothHostInfo &hostInfo, BluetoothGattCharacteristic *pairingCharacteristic, QObject *parent) :
    QObject(parent),
    m_hostInfo(hostInfo),
    m_pairingCharacteristic(pairingCharacteristic)
{

#ifdef QT_DEBUG
    // Enable full debug messages containing sensible data for debug builds
    m_debug = true;
#endif

    // Check if we have authentication data for this thing and set initial state
    loadData();
    if (isValid()) {
        qCDebug(dcNuki()) << "Found valid authroization data for" << hostInfo.address().toString();
        setState(AuthenticationStateAuthenticated);
    } else {
        setState(AuthenticationStateUnauthenticated);
    }

    connect(m_pairingCharacteristic, &BluetoothGattCharacteristic::valueChanged, this, &NukiAuthenticator::onPairingDataCharacteristicChanged);
}

NukiUtils::ErrorCode NukiAuthenticator::error() const
{
    return m_error;
}

NukiAuthenticator::AuthenticationState NukiAuthenticator::state() const
{
    return m_state;
}

bool NukiAuthenticator::isValid() const
{
    return !m_privateKey.isEmpty() &&
            !m_publicKey.isEmpty() &&
            !m_publicKeyNuki.isEmpty() &&
            m_authorizationId != 0 &&
            !m_authorizationIdRawData.isEmpty() &&
            !m_uuid.isEmpty();
}

void NukiAuthenticator::clearSettings()
{
    QSettings setting(NymeaSettings::settingsPath() + "/plugin-nuki.conf", QSettings::IniFormat);
    setting.beginGroup(m_hostInfo.address().toString());
    setting.remove("");
    setting.endGroup();

    qCDebug(dcNuki()) << "Settings cleared for" << m_hostInfo.address().toString() << "in" << setting.fileName();
}

void NukiAuthenticator::startAuthenticationProcess()
{
    setState(AuthenticationStateRequestPublicKey);
}

quint32 NukiAuthenticator::authorizationId() const
{
    return m_authorizationId;
}

QByteArray NukiAuthenticator::authorizationIdRawData() const
{
    return m_authorizationIdRawData;
}

QByteArray NukiAuthenticator::encryptData(const QByteArray &data, const QByteArray &nonce)
{
    // Calculate shared key
    qCDebug(dcNuki()) << "Authenticator: Encrypt data";
    Q_ASSERT_X(nonce.length() == crypto_box_NONCEBYTES, "data length", "The nonce does not have the correct length.");

    /* Note: https://download.libsodium.org/doc/public-key_cryptography/authenticated_encryption.html
     *      unsigned char *c         The encrypted message (length of the data + crypto_box_MACBYTES)
     *      const unsigned char *m   The message to encrypt
     *      unsigned long long mlen  The length of the message to encrypt
     *      const unsigned char *n   The nonce (must also sent unencrypted)
     *      const unsigned char *pk  The public key of the Nuki
     *      const unsigned char *sk  The private key of nymea for this Nuki
     */

    unsigned char encrypted[crypto_box_MACBYTES + data.length()];
    int result = crypto_box_easy(encrypted,
                                 reinterpret_cast<const unsigned char *>(data.data()),
                                 static_cast<unsigned long long>(data.length()),
                                 reinterpret_cast<const unsigned char *>(nonce.data()),
                                 reinterpret_cast<const unsigned char *>(m_publicKeyNuki.data()),
                                 reinterpret_cast<const unsigned char *>(m_privateKey.data()));

    if (result < 0) {
        qCWarning(dcNuki()) << "Could not encrypt data. Something went wrong";
        return QByteArray();
    }

    QByteArray encryptedData = QByteArray(reinterpret_cast<const char*>(encrypted), crypto_box_MACBYTES + data.length());

    if (m_debug) qCDebug(dcNuki()) << "    Private key     :" << NukiUtils::convertByteArrayToHexStringCompact(m_privateKey);
    if (m_debug) qCDebug(dcNuki()) << "    Public key      :" << NukiUtils::convertByteArrayToHexStringCompact(m_publicKey);
    if (m_debug) qCDebug(dcNuki()) << "    Nuki public key :" << NukiUtils::convertByteArrayToHexStringCompact(m_publicKeyNuki);
    if (m_debug) qCDebug(dcNuki()) << "    Unencrypted data:" << NukiUtils::convertByteArrayToHexStringCompact(data);
    if (m_debug) qCDebug(dcNuki()) << "    Encrypted data  :" << NukiUtils::convertByteArrayToHexStringCompact(encryptedData);

    return encryptedData;
}

QByteArray NukiAuthenticator::decryptData(const QByteArray &data, const QByteArray &nonce)
{
    qCDebug(dcNuki()) << "Authenticator: Decrypt data";
    Q_ASSERT_X(nonce.length() == crypto_box_NONCEBYTES, "data length", "The nonce does not have the correct length.");
    Q_ASSERT_X(static_cast<uint>(data.length()) >= crypto_box_MACBYTES, "data length", "The encrypted data is to short.");

    /* Note: https://download.libsodium.org/doc/public-key_cryptography/authenticated_encryption.html
     *      unsigned char *m         The decrypted message result
     *      const unsigned char *c   The message to decrypt / cyphertext (length of the encrypted data + crypto_box_MACBYTES)
     *      unsigned long long clen  The length of the message to decrypt
     *      const unsigned char *n   The nonce used while encryption (received in the unencrypted ADATA)
     *      const unsigned char *pk  The public key of the Nuki
     *      const unsigned char *sk  The private key of nymea for this Nuki
     */

    unsigned char decrypted[data.length() - crypto_box_MACBYTES];
    int result = crypto_box_open_easy(decrypted,
                                      reinterpret_cast<const unsigned char *>(data.data()),
                                      static_cast<unsigned long long>(data.length()),
                                      reinterpret_cast<const unsigned char *>(nonce.data()),
                                      reinterpret_cast<const unsigned char *>(m_publicKeyNuki.data()),
                                      reinterpret_cast<const unsigned char *>(m_privateKey.data()));

    if (result < 0) {
        qCWarning(dcNuki()) << "Could not decrypt data. Something went wrong";
        return QByteArray();
    }

    QByteArray decryptedData = QByteArray(reinterpret_cast<const char*>(decrypted), data.length() - crypto_box_MACBYTES);

    if (m_debug) qCDebug(dcNuki()) << "    Private key     :" << NukiUtils::convertByteArrayToHexStringCompact(m_privateKey);
    if (m_debug) qCDebug(dcNuki()) << "    Public key      :" << NukiUtils::convertByteArrayToHexStringCompact(m_publicKey);
    if (m_debug) qCDebug(dcNuki()) << "    Nuki public key :" << NukiUtils::convertByteArrayToHexStringCompact(m_publicKeyNuki);
    if (m_debug) qCDebug(dcNuki()) << "    Encrypted data  :" << NukiUtils::convertByteArrayToHexStringCompact(data);
    if (m_debug) qCDebug(dcNuki()) << "    Decrypted data  :" << NukiUtils::convertByteArrayToHexStringCompact(decryptedData);

    return decryptedData;
}

QByteArray NukiAuthenticator::generateNonce(const int &length) const
{
    unsigned char nounce[length];
    randombytes_buf(nounce, length);
    return QByteArray(reinterpret_cast<const char *>(nounce), length);
}

void NukiAuthenticator::setState(NukiAuthenticator::AuthenticationState state)
{
    if (m_state == state)
        return;

    m_state = state;
    emit stateChanged(m_state);

    qCDebug(dcNuki()) << m_state;

    switch (m_state) {
    case AuthenticationStateUnauthenticated:
        break;
    case AuthenticationStateAuthenticated:
        qCDebug(dcNuki()) << "Device" << m_hostInfo.address().toString() << "authenticated.";
        if (m_debug) qCDebug(dcNuki()) << "    Private key     :" << NukiUtils::convertByteArrayToHexStringCompact(m_privateKey);
        if (m_debug) qCDebug(dcNuki()) << "    Public key      :" << NukiUtils::convertByteArrayToHexStringCompact(m_publicKey);
        if (m_debug) qCDebug(dcNuki()) << "    Nuki public key :" << NukiUtils::convertByteArrayToHexStringCompact(m_publicKeyNuki);
        if (m_debug) qCDebug(dcNuki()) << "    Authorization ID:" << NukiUtils::convertByteArrayToHexStringCompact(m_authorizationIdRawData) << m_authorizationId;
        break;
    case AuthenticationStateRequestPublicKey:
        requestPublicKey();
        break;
    case AuthenticationStateGenerateKeyPair:
        generateKeyPair();
        setState(AuthenticationStateSendPublicKey);
        break;
    case AuthenticationStateSendPublicKey:
        sendPublicKey();
        setState(AuthenticationStateReadChallenge);
        break;
    case AuthenticationStateReadChallenge:
        break;
    case AuthenticationStateAutorization:
        sendAuthorizationAuthenticator();
        setState(AuthenticationStateReadSecondChallenge);
        break;
    case AuthenticationStateReadSecondChallenge:
        break;
    case AuthenticationStateAuthenticateData:
        sendAuthenticateData();
        setState(AuthenticationStateAuthorizationId);
        break;
    case AuthenticationStateAuthorizationId:
        break;
    case AuthenticationStateAuthorizationIdConfirm:
        sendAuthoizationIdConfirm();
        setState(AuthenticationStateStatus);
        break;
    case AuthenticationStateStatus:
        break;
    case AuthenticationStateError:
        emit errorOccured(m_error);
        emit authenticationProcessFinished(false);
        break;
    default:
        qCWarning(dcNuki()) << "Authenticator: Unknown state.";
        break;
    }
}

bool NukiAuthenticator::createAuthenticator(const QByteArray content)
{
    // Create shared key
    qCDebug(dcNuki()) << "Authenticator: Calculate shared key";
    unsigned char sharedKey[crypto_box_BEFORENMBYTES];
    int result = crypto_box_beforenm(sharedKey, reinterpret_cast<const unsigned char *>(m_publicKeyNuki.data()), reinterpret_cast<const unsigned char *>(m_privateKey.data()));
    if (result < 0) {
        qCWarning(dcNuki()) << "Could not create shared key for autorization authenticator.";
        return false;
    }

    m_sharedKey = QByteArray(reinterpret_cast<const char*>(sharedKey), crypto_box_BEFORENMBYTES);
    Q_ASSERT_X(m_sharedKey.length() == 32, "data length", "The shared key does not have the correct length.");

    if (m_debug) qCDebug(dcNuki()) << "Authenticator: Calculate authenticator hash HMAC-SHA-256";
    if (m_debug) qCDebug(dcNuki()) << "    Shared key      :" << NukiUtils::convertByteArrayToHexStringCompact(m_sharedKey);
    if (m_debug) qCDebug(dcNuki()) << "    Nuki nonce      :" << NukiUtils::convertByteArrayToHexStringCompact(m_nonceNuki);

    // Calculate authenticator hash input for HMAC-SHA-256
    qCDebug(dcNuki()) << "Authenticator: Calculate authenticator data";
    unsigned char authenticator[crypto_auth_hmacsha256_BYTES];
    result = crypto_auth_hmacsha256(authenticator, reinterpret_cast<const unsigned char *>(content.data()), content.length(), reinterpret_cast<const unsigned char *>(m_sharedKey.data()));
    if (result < 0) {
        qCWarning(dcNuki()) << "Could not create authenticator hash for autorization authenticator.";
        return false;
    }

    m_authenticator = QByteArray(reinterpret_cast<const char*>(authenticator), crypto_auth_hmacsha256_BYTES);
    if (m_debug) qCDebug(dcNuki()) << "    Authenticator   :" << NukiUtils::convertByteArrayToHexStringCompact(m_authenticator);
    return true;
}

void NukiAuthenticator::requestPublicKey()
{
    qCDebug(dcNuki()) << "Authenticator: Request public key fom Nuki";

    QByteArray payload;
    QDataStream stream(&payload, QIODevice::WriteOnly);
    stream.setByteOrder(QDataStream::LittleEndian);
    stream << static_cast<quint16>(NukiUtils::CommandPublicKey);
    QByteArray data = NukiUtils::createRequestMessageForUnencrypted(NukiUtils::CommandRequestData, payload);
    if (m_debug) qCDebug(dcNuki()) << "-->" << NukiUtils::convertByteArrayToHexStringCompact(data);
    m_pairingCharacteristic->writeCharacteristic(data);
}

void NukiAuthenticator::sendPublicKey()
{
    qCDebug(dcNuki()) << "Authenticator: Send public key to Nuki";
    QByteArray data = NukiUtils::createRequestMessageForUnencrypted(NukiUtils::CommandPublicKey, m_publicKey);
    if (m_debug) qCDebug(dcNuki()) << "-->" << NukiUtils::convertByteArrayToHexStringCompact(data);
    m_pairingCharacteristic->writeCharacteristic(data);
}

void NukiAuthenticator::generateKeyPair()
{
    qCDebug(dcNuki()) << "Generate key pair";
    unsigned char publicKey[crypto_box_PUBLICKEYBYTES];
    unsigned char secretKey[crypto_box_SECRETKEYBYTES];
    crypto_box_keypair(publicKey, secretKey);
    m_publicKey = QByteArray(reinterpret_cast<const char *>(publicKey), crypto_box_PUBLICKEYBYTES);
    m_privateKey = QByteArray(reinterpret_cast<const char *>(secretKey), crypto_box_SECRETKEYBYTES);

    if (m_debug) qCDebug(dcNuki()) << "    Private key     :" << NukiUtils::convertByteArrayToHexStringCompact(m_privateKey);
    if (m_debug) qCDebug(dcNuki()) << "    Public key      :" << NukiUtils::convertByteArrayToHexStringCompact(m_publicKey);
    if (m_debug) qCDebug(dcNuki()) << "    Nuki public key :" << NukiUtils::convertByteArrayToHexStringCompact(m_publicKeyNuki);
}

void NukiAuthenticator::sendAuthorizationAuthenticator()
{
    QByteArray valueR;
    valueR.append(m_publicKey);
    valueR.append(m_publicKeyNuki);
    valueR.append(m_nonceNuki);

    // Create authenticator and store it into m_authenticator
    if (!createAuthenticator(valueR)) {
        qCWarning(dcNuki()) << "Could not create authenticator hash HMAC-SHA-256";
        setState(AuthenticationStateError);
    }

    // Send the authenticator
    qCDebug(dcNuki()) << "Authenticator: Send authorization authenticator to Nuki";
    QByteArray message = NukiUtils::createRequestMessageForUnencrypted(NukiUtils::CommandAuthorizationAuthenticator, m_authenticator);
    if (m_debug) qCDebug(dcNuki()) << "-->" << NukiUtils::convertByteArrayToHexStringCompact(message);
    m_pairingCharacteristic->writeCharacteristic(message);
}

void NukiAuthenticator::sendAuthenticateData()
{
    // Calculate new nounce
    m_nonce = generateNonce();

    QByteArray content;
    QDataStream stream(&content, QIODevice::WriteOnly);
    // Note: 0x00 = App, 0x01 = Bridge, 0x02 = Fob
    stream << static_cast<quint8>(0x01);
    // Note: app id (42)
    stream << static_cast<quint32>(0x002A);

    // Note: the name of the bridge in 32 bytes [ 0 0 0 ... n y m e a ]
    QByteArray name = QByteArray(27, '\0').append(QByteArray("nymea"));
    Q_ASSERT_X(name.count() == 32, "data length", "Name has not the correct length.");

    QByteArray valueR = content;
    valueR.append(name);
    valueR.append(m_nonce);
    valueR.append(m_nonceNuki);

    if (m_debug) qCDebug(dcNuki()) << "    Name            :" << qUtf8Printable(name) << NukiUtils::convertByteArrayToHexStringCompact(name);
    if (m_debug) qCDebug(dcNuki()) << "    Nonce           :" << NukiUtils::convertByteArrayToHexStringCompact(m_nonce);

    // Create authenticator and store it into m_authenticator
    if (!createAuthenticator(valueR)) {
        qCWarning(dcNuki()) << "Could not create authenticator hash HMAC-SHA-256";
        setState(AuthenticationStateError);
    }

    // Prepare message to send to Nuki
    QByteArray data;
    data.append(m_authenticator);
    data.append(content);
    data.append(name);
    data.append(m_nonce);

    qCDebug(dcNuki()) << "Authenticator: Send authentication data to Nuki";
    QByteArray message = NukiUtils::createRequestMessageForUnencrypted(NukiUtils::CommandAuthorizationData, data);
    if (m_debug) qCDebug(dcNuki()) << "-->" << NukiUtils::convertByteArrayToHexStringCompact(message);
    m_pairingCharacteristic->writeCharacteristic(message);
}

void NukiAuthenticator::sendAuthoizationIdConfirm()
{
    qCDebug(dcNuki()) << "Authenticator: Create data for authentication ID confirm";
    QByteArray valueR;
    valueR.append(m_authorizationIdRawData);
    valueR.append(m_nonceNuki);

    // Create authenticator and store it into m_authenticator
    if (!createAuthenticator(valueR)) {
        qCWarning(dcNuki()) << "Could not create authenticator hash HMAC-SHA-256";
        setState(AuthenticationStateError);
    }

    // Calculate new nounce
    m_nonce = generateNonce();

    if (m_debug) qCDebug(dcNuki()) << "    Nonce           :" << NukiUtils::convertByteArrayToHexStringCompact(m_nonce);
    if (m_debug) qCDebug(dcNuki()) << "    Nuki Nonce      :" << NukiUtils::convertByteArrayToHexStringCompact(m_nonceNuki);
    if (m_debug) qCDebug(dcNuki()) << "    Authorization ID:" << NukiUtils::convertByteArrayToHexStringCompact(m_authorizationIdRawData) << m_authorizationId;

    // Prepare message to send to Nuki
    QByteArray data;
    data.append(m_authenticator);
    data.append(m_authorizationIdRawData);

    qCDebug(dcNuki()) << "Authenticator: Send authentication ID confirm to Nuki";
    QByteArray message = NukiUtils::createRequestMessageForUnencrypted(NukiUtils::CommandAuthorizationIdConfirmation, data);
    if (m_debug) qCDebug(dcNuki()) << "-->" << NukiUtils::convertByteArrayToHexStringCompact(message);
    m_pairingCharacteristic->writeCharacteristic(message);
}

void NukiAuthenticator::saveData()
{
    QSettings setting(NymeaSettings::settingsPath() + "/plugin-nuki.conf", QSettings::IniFormat);
    setting.beginGroup(m_hostInfo.address().toString());
    setting.setValue("privateKey", m_privateKey);
    setting.setValue("publicKey", m_publicKey);
    setting.setValue("publicKeyNuki", m_publicKeyNuki);
    setting.setValue("authenticationIdRawData", m_authorizationIdRawData);
    setting.setValue("authenticationId", m_authorizationId);
    setting.setValue("uuid", m_uuid);
    setting.endGroup();

    qCDebug(dcNuki()) << "Authenticator: Settings saved to" << setting.fileName();
}

void NukiAuthenticator::loadData()
{
    QSettings setting(NymeaSettings::settingsPath() + "/plugin-nuki.conf", QSettings::IniFormat);
    setting.beginGroup(m_hostInfo.address().toString());
    m_privateKey = setting.value("privateKey", QByteArray()).toByteArray();
    m_publicKey = setting.value("publicKey", QByteArray()).toByteArray();
    m_publicKeyNuki = setting.value("publicKeyNuki", QByteArray()).toByteArray();
    m_authorizationIdRawData = setting.value("authenticationIdRawData", QByteArray()).toByteArray();
    m_authorizationId = static_cast<quint32>(setting.value("authenticationId", 0).toInt());
    m_uuid = setting.value("uuid", QByteArray()).toByteArray();
    setting.endGroup();

    qCDebug(dcNuki()) << "Authenticator: Settings loaded from" << setting.fileName();
}

void NukiAuthenticator::onPairingDataCharacteristicChanged(const QByteArray &value)
{

    // Process pairing characteristic data
    QByteArray data = QByteArray(value);
    QDataStream stream(&data, QIODevice::ReadOnly);
    stream.setByteOrder(QDataStream::LittleEndian);
    quint16 command;
    stream >> command;

    m_currentReceivingData = value;

    if (m_debug) qCDebug(dcNuki()) << "Authenticator data received: <--" << static_cast<NukiUtils::Command>(command) << "|" << NukiUtils::convertByteArrayToHexStringCompact(value);

    switch (command) {
    case NukiUtils::CommandErrorReport:
        quint8 error;
        quint16 commandIdentifier;
        quint16 crc;
        stream >> error >> commandIdentifier >> crc;
        if (!NukiUtils::validateMessageCrc(data)) {
            qCWarning(dcNuki()) << "Invalid message";
            // FIXME: check what to do if crc is invalid
        }
        m_error = static_cast<NukiUtils::ErrorCode>(error);
        qCWarning(dcNuki()) << "Authenticator: Error for command" << static_cast<NukiUtils::Command>(commandIdentifier) << m_error;
        setState(AuthenticationStateError);
        break;
    case NukiUtils::CommandPublicKey:
        if (!NukiUtils::validateMessageCrc(m_currentReceivingData)) {
            qCWarning(dcNuki()) << "Invalid CRC CCITT value for public key message.";
            // FIXME: check what to do if crc is invalid
        }

        qCDebug(dcNuki()) << "Authenticator: Nuki public key message received" << (m_debug ? NukiUtils::convertByteArrayToHexStringCompact(m_currentReceivingData) : "");
        m_publicKeyNuki = m_currentReceivingData.mid(2, 32);
        if (m_debug) qCDebug(dcNuki()) << "Authenticator: --> Nuki public key:" <<   NukiUtils::convertByteArrayToHexStringCompact(m_publicKeyNuki);

        setState(AuthenticationStateGenerateKeyPair);
        break;
    case NukiUtils::CommandChallenge:
        qCDebug(dcNuki()) << "Authenticator: Nuki challenge message received" << (m_debug ? NukiUtils::convertByteArrayToHexStringCompact(m_currentReceivingData) : "");
        if (!NukiUtils::validateMessageCrc(m_currentReceivingData)) {
            qCWarning(dcNuki()) << "Invalid CRC CCITT value for challenge message.";
            // FIXME: check what to do if crc is invalid
        }

        m_nonceNuki = m_currentReceivingData.mid(2, 32);
        if (m_debug) qCDebug(dcNuki()) << "Authenticator: --> Nuki nonce:" << NukiUtils::convertByteArrayToHexStringCompact(m_nonceNuki);
        // Check if this was from the first challenge read or the second
        if (m_state == AuthenticationStateReadChallenge) {
            setState(AuthenticationStateAutorization);
        } else if (m_state == AuthenticationStateReadSecondChallenge) {
            setState(AuthenticationStateAuthenticateData);
        } else {
            qCWarning(dcNuki()) << "Received a challenge without expecting one.";
            setState(AuthenticationStateError);
        }
        break;
    case NukiUtils::CommandAuthorizationId: {
        qCDebug(dcNuki()) << "Authenticator: Nuki authorization ID message received" << (m_debug ? NukiUtils::convertByteArrayToHexStringCompact(m_currentReceivingData) : "");

        if (!NukiUtils::validateMessageCrc(m_currentReceivingData)) {
            qCWarning(dcNuki()) << "Invalid CRC CCITT value for challenge message.";
            // FIXME: check what to do if crc is invalid
        }

        // Parse data
        QByteArray message = m_currentReceivingData.mid(2, m_currentReceivingData.count() - 4);
        QByteArray authenticator = message.left(32);
        Q_ASSERT_X(authenticator.count() == 32, "data length", "Nuki nonce has not the correct length.");

        // Read authorization ID
        m_authorizationIdRawData = message.mid(32, 4);
        QDataStream stream(&m_authorizationIdRawData, QIODevice::ReadOnly);
        stream.setByteOrder(QDataStream::LittleEndian);
        stream >> m_authorizationId;

        m_uuid = message.mid(36, 16);
        Q_ASSERT_X(m_uuid.count() == 16, "data length", "UUID has not the correct length.");

        m_nonceNuki = message.mid(52, 32);
        Q_ASSERT_X(m_nonceNuki.count() == 32, "data length", "Nuki nonce has not the correct length.");

        if (m_debug) qCDebug(dcNuki()) << "    Full message    :" <<   NukiUtils::convertByteArrayToHexStringCompact(message);
        if (m_debug) qCDebug(dcNuki()) << "    Authenticator   :" <<   NukiUtils::convertByteArrayToHexStringCompact(authenticator);
        if (m_debug) qCDebug(dcNuki()) << "    Authorization ID:" <<   NukiUtils::convertByteArrayToHexStringCompact(m_authorizationIdRawData) << m_authorizationId;
        if (m_debug) qCDebug(dcNuki()) << "    UUID data       :" <<   NukiUtils::convertByteArrayToHexStringCompact(m_uuid);
        if (m_debug) qCDebug(dcNuki()) << "    Nuki nonce      :" <<   NukiUtils::convertByteArrayToHexStringCompact(m_nonceNuki);

        setState(AuthenticationStateAuthorizationIdConfirm);
        break;
    }
    case NukiUtils::CommandStatus: {
        quint8 status;
        stream >> status;

        if (!NukiUtils::validateMessageCrc(data)) {
            qCWarning(dcNuki()) << "Invalid message";
            // FIXME: check what to do if crc is invalid
        }

        NukiUtils::StatusCode statusCode = static_cast<NukiUtils::StatusCode>(status);
        if (m_debug) qCDebug(dcNuki()) << statusCode;
        switch (statusCode) {
        case NukiUtils::StatusCodeAccepted:
            qCWarning(dcNuki()) << "The command was accepted, but not completed.";
            setState(AuthenticationStateError);
            break;
        case NukiUtils::StatusCodeCompeted:
            qCDebug(dcNuki()) << "Nuki authentication process finished successfully!";
            saveData();
            setState(AuthenticationStateAuthenticated);
            emit authenticationProcessFinished(true);
            break;
        default:
            break;
        }
        break;
    }
    default:
        qCWarning(dcNuki()) << "Authenticator: Unhandled command identifier for parining charateristic" << NukiUtils::convertUint16ToHexString(command);
        break;
    }
}
