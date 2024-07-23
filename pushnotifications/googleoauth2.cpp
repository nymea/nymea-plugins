/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*
* Copyright 2013 - 2024, nymea GmbH
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

#include "googleoauth2.h"

#include <QUrlQuery>
#include <QJsonDocument>
#include <QSharedPointer>
#include <QNetworkRequest>

#include <openssl/rsa.h>
#include <openssl/pem.h>
#include <openssl/sha.h>

#include "extern-plugininfo.h"

GoogleOAuth2::GoogleOAuth2(NetworkAccessManager *networkManager, const ApiKey &apiKey, QObject *parent)
    : QObject{parent},
    m_networkManager{networkManager},
    m_apiKey{apiKey}
{
    connect(&m_refreshTimer, &QTimer::timeout, this, &GoogleOAuth2::authorize);
}

QString GoogleOAuth2::accessToken() const
{
    return m_accessToken;
}

void GoogleOAuth2::authorize()
{
    qCDebug(dcPushNotifications()) << "OAuth2: Authorize client and request access token";

    QVariantMap jwtHeader;
    jwtHeader.insert("alg", "RS256");
    jwtHeader.insert("typ", "JWT");
    jwtHeader.insert("kid", m_apiKey.data("private_key_id"));

    QVariantMap jwtClaim;
    jwtClaim.insert("iss", m_apiKey.data("client_email"));
    jwtClaim.insert("scope", "https://www.googleapis.com/auth/firebase.messaging");
    jwtClaim.insert("aud", "https://oauth2.googleapis.com/token");
    jwtClaim.insert("iat", QDateTime::currentDateTime().toSecsSinceEpoch());
    jwtClaim.insert("exp", QDateTime::currentDateTime().addSecs(3600).toSecsSinceEpoch());

    QByteArray headerData = QJsonDocument::fromVariant(jwtHeader).toJson(QJsonDocument::Compact);
    QByteArray claimData = QJsonDocument::fromVariant(jwtClaim).toJson(QJsonDocument::Compact);

    // qCDebug(dcPushNotifications()) << "OAuth2: JWT Header:" << qUtf8Printable(headerData);
    // qCDebug(dcPushNotifications()) << "OAuth2: JWT Claim:" << qUtf8Printable(claimData);

    QString dataToSign = QString("%1.%2").arg(QString(headerData.toBase64(QByteArray::Base64UrlEncoding | QByteArray::OmitTrailingEquals)))
                             .arg(QString(claimData.toBase64(QByteArray::Base64UrlEncoding | QByteArray::OmitTrailingEquals)));

    QByteArray signature = signData(dataToSign.toUtf8(), m_apiKey.data("private_key"));
    QString jwt = QString("%1.%2").arg(dataToSign).arg(QString(signature.toBase64(QByteArray::Base64UrlEncoding | QByteArray::OmitTrailingEquals)));

    // qCDebug(dcPushNotifications()) << "OAuth2: JWT for bearer access token request:" << jwt;

    QNetworkRequest request(QUrl("https://oauth2.googleapis.com/token"));
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/x-www-form-urlencoded");

    QUrlQuery payloadQuery;
    payloadQuery.addQueryItem("grant_type", "urn:ietf:params:oauth:grant-type:jwt-bearer");
    payloadQuery.addQueryItem("assertion", jwt);

    qCDebug(dcPushNotifications()) << "OAuth2: Request access token from" << request.url().toString();

    QNetworkReply *reply = m_networkManager->post(request, payloadQuery.toString().toUtf8());
    connect(reply, &QNetworkReply::finished, reply, &QNetworkReply::deleteLater);
    connect(reply, &QNetworkReply::finished, this, [this, reply, request](){

        QByteArray data = reply->readAll();
        if (reply->error() != QNetworkReply::NoError) {
            qCWarning(dcPushNotifications()) << "OAuth2: Failed to request access token from" << request.url().toString() << reply->errorString();
            if (!data.isEmpty())
                qCWarning(dcPushNotifications()) << "OAuth2: Response data:" << qUtf8Printable(data);

            setAuthenticated(false);

            // Retry in 30 seconds, maybe we are not online yet
            m_refreshTimer.start(60 * 1000);
            return;
        }

        //qCDebug(dcPushNotifications()) << "OAuth2: Request access token response" << qUtf8Printable(data);

        QJsonParseError error;
        QJsonDocument jsonDoc = QJsonDocument::fromJson(data, &error);
        if (error.error != QJsonParseError::NoError) {
            qCWarning(dcPushNotifications()) << "OAuth2: Failed to get access token. Response data JSON error:" << error.errorString();
            setAuthenticated(false);
            return;
        }

        QVariantMap responseMap = jsonDoc.toVariant().toMap();
        if (!responseMap.contains("access_token")) {
            qCWarning(dcPushNotifications()) << "OAuth2: Failed to get access token. Response has no property \"access_token\"" << qUtf8Printable(data);
            setAuthenticated(false);
            return;
        }

        if (!responseMap.contains("expires_in")) {
            qCWarning(dcPushNotifications()) << "OAuth2: Failed to get access token. Response has no property \"expires_in\"" << qUtf8Printable(data);
            setAuthenticated(false);
            return;
        }

        uint expiresIn = responseMap.value("expires_in").toUInt();
        uint refreshTimeout = expiresIn - 120; // Refresh two minutes before expiration
        qCDebug(dcPushNotifications()) << "The token will expire in" << expiresIn << "seconds. Refreshing access token in" << refreshTimeout << "seconds.";
        m_refreshTimer.start(refreshTimeout * 1000);

        QString accessToken = responseMap.value("access_token").toString();
        if (accessToken.isEmpty()) {
            qCWarning(dcPushNotifications()) << "OAuth2: Received empty access token.";
            setAuthenticated(false);
            return;
        }

        setAuthenticated(true);

        if (m_accessToken != accessToken) {
            qCDebug(dcPushNotifications()) << "OAuth2: New access token available.";
            m_accessToken = accessToken;
            emit accessTokenChanged(m_accessToken);
        }
    });

}

bool GoogleOAuth2::authenticated() const
{
    return m_authenticated;
}

void GoogleOAuth2::setAuthenticated(bool authenticated)
{
    if (m_authenticated == authenticated)
        return;

    m_authenticated = authenticated;
    emit authenticatedChanged(m_authenticated);

    if (m_authenticated) {
        qCDebug(dcPushNotifications()) << "OAuth2: Authenticated";
    } else {
        qCWarning(dcPushNotifications()) << "OAuth2: Not authenticated any more.";
        if (!m_accessToken.isEmpty()) {
            qCWarning(dcPushNotifications()) << "OAuth2: Forgetting current access token.";
            m_accessToken.clear();
        }
    }
}

QByteArray GoogleOAuth2::signData(const QByteArray &data, const QByteArray &key)
{
    // Inspired by: https://jorisvergeer.nl/2023/03/22/c-qt-openssl-jwt-minimalistic-implementation-to-create-a-signed-jtw-token/

    // qCDebug(dcPushNotifications()) << "Signing data" << qUtf8Printable(data);

    QSharedPointer<BIO> bioSet = QSharedPointer<BIO>(BIO_new_mem_buf(key.constData(), -1), &BIO_free_all);
    if (!bioSet) {
        qCWarning(dcPushNotifications()) << "Failed to create data buffer for signing";
        return QByteArray();
    }

    QSharedPointer<RSA> rsaKey = QSharedPointer<RSA>(PEM_read_bio_RSAPrivateKey(bioSet.data(), nullptr, nullptr, nullptr), &RSA_free);
    if (!rsaKey) {
        qCWarning(dcPushNotifications()) << "Failed to load private key for singing JWT into buffer";
        return QByteArray();
    }

    QByteArray sha256_hash(SHA256_DIGEST_LENGTH, 0);
    SHA256(reinterpret_cast<const unsigned char *>(data.constData()), data.length(),
           reinterpret_cast<unsigned char *>(sha256_hash.data()));

    uint signatureLength = 0;
    QByteArray signatureBuffer(RSA_size(rsaKey.data()), 0);

    if (RSA_sign(NID_sha256,
                 reinterpret_cast<unsigned char *>(sha256_hash.data()), SHA256_DIGEST_LENGTH,
                 reinterpret_cast<unsigned char *>(signatureBuffer.data()), &signatureLength, rsaKey.data()) != 1) {
        qCWarning(dcPushNotifications()) << "Failed to signing data from JWT";
        return QByteArray();
    }

    return signatureBuffer.left(signatureLength);
}

