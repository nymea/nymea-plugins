/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *                                                                         *
 *  Copyright (C) 2019 Bernhard Trinnes <bernhard.trinnes@nymea.io>        *
 *  Copyright (C) 2019 Michael Zanetti <michael.zanetti@nymea.io>          *
 *                                                                         *
 *  This file is part of nymea.                                            *
 *                                                                         *
 *  nymea is free software: you can redistribute it and/or modify          *
 *  it under the terms of the GNU General Public License as published by   *
 *  the Free Software Foundation, version 2 of the License.                *
 *                                                                         *
 *  nymea is distributed in the hope that it will be useful,               *
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of         *
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the           *
 *  GNU General Public License for more details.                           *
 *                                                                         *
 *  You should have received a copy of the GNU General Public License      *
 *  along with nymea. If not, see <http://www.gnu.org/licenses/>.          *
 *                                                                         *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

#include "doorbird.h"
#include "extern-plugininfo.h"

#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QAuthenticator>
#include <QTimer>
#include <QImage>
#include <QUrl>
#include <QUrlQuery>

Doorbird::Doorbird(const QHostAddress &address, const QString &username, const QString &password, QObject *parent) :
    QObject(parent),
    m_address(address),
    m_username(username),
    m_password(password)
{
    m_networkAccessManager = new QNetworkAccessManager(this);
    connect(m_networkAccessManager, &QNetworkAccessManager::authenticationRequired, this, [this](QNetworkReply *reply, QAuthenticator *authenticator) {
        Q_UNUSED(reply);
        qCDebug(dcDoorBird) << "Credentials requested:";
        authenticator->setUser(m_username);
        authenticator->setPassword(m_password);
    });
}

QUuid Doorbird::getSession()
{
    QNetworkRequest request(QString("http://%1/bha-api/getsession.cgi").arg(m_address.toString()));
    qCDebug(dcDoorBird) << "Sending request:" << request.url();
    QNetworkReply *reply = m_networkAccessManager->get(request);
    QUuid requestId = QUuid::createUuid();

    connect(reply, &QNetworkReply::finished, this, [this, reply, requestId](){
        reply->deleteLater();

        if (reply->error() != QNetworkReply::NoError) {
            qCWarning(dcDoorBird) << "Error unlatching DoorBird device";
            emit requestSent(requestId, false);
            return;
        }
        emit requestSent(requestId, true);

        QByteArray data = reply->readAll();
        QJsonParseError error;
        QJsonDocument jsonDoc = QJsonDocument::fromJson(data, &error);
        if (error.error != QJsonParseError::NoError) {
            qCWarning(dcDoorBird()) << "Error parsing json:" << data;
            return;
        }
    });
    return requestId;
}

QUuid Doorbird::openDoor(int value)
{
    QNetworkRequest request(QString("http://%1/bha-api/open-door.cgi?r=%2").arg(m_address.toString()).arg(QString::number(value)));
    qCDebug(dcDoorBird) << "Sending request:" << request.url();
    QNetworkReply *reply = m_networkAccessManager->get(request);
    QUuid requestId = QUuid::createUuid();
    connect(reply, &QNetworkReply::finished, this, [this, reply, requestId](){
        reply->deleteLater();

        if (reply->error() != QNetworkReply::NoError) {
            qCWarning(dcDoorBird) << "Error opening DoorBird "  << reply->error() << reply->errorString();
            emit requestSent(requestId, false);
            return;
        }
        qCDebug(dcDoorBird) << "DoorBird unlatched:";
        emit requestSent(requestId, true);
    });
    return requestId;
}

QUuid Doorbird::lightOn()
{
    QNetworkRequest request(QString("http://%1/bha-api/light-on.cgi").arg(m_address.toString()));
    qCDebug(dcDoorBird) << "Sending request:" << request.url();
    QNetworkReply *reply = m_networkAccessManager->get(request);
    QUuid requestId = QUuid::createUuid();
    connect(reply, &QNetworkReply::finished, this, [this, reply, requestId](){
        reply->deleteLater();

        if (reply->error() != QNetworkReply::NoError) {
            qCWarning(dcDoorBird) << "Error unlatching DoorBird device"  << reply->error() << reply->errorString();
            emit requestSent(requestId, false);
            return;
        }
        qCDebug(dcDoorBird) << "DoorBird light on:";
        emit requestSent(requestId, true);
    });
    return requestId;
}

QUuid Doorbird::liveVideoRequest()
{
    QNetworkRequest request(QString("http://%1/bha-api/info.cgi").arg(m_address.toString()));
    qCDebug(dcDoorBird) << "Sending request:" << request.url();
    QNetworkReply *reply = m_networkAccessManager->get(request);
    QUuid requestId = QUuid::createUuid();
    connect(reply, &QNetworkReply::finished, this, [this, reply, requestId](){
        reply->deleteLater();

        if (reply->error() != QNetworkReply::NoError) {
            qCWarning(dcDoorBird) << "Error unlatching DoorBird device";
            emit requestSent(requestId, false);
            return;
        }
        qCDebug(dcDoorBird) << "DoorBird unlatched:" << reply->error() << reply->errorString();
        emit requestSent(requestId, true);
    });
    return requestId;
}

QUuid Doorbird::liveImageRequest()
{
    QNetworkRequest request(QString("http://%1/bha-api/light-on.cgi").arg(m_address.toString()));
    qCDebug(dcDoorBird) << "Sending request:" << request.url();
    QNetworkReply *reply = m_networkAccessManager->get(request);
    QUuid requestId = QUuid::createUuid();
    connect(reply, &QNetworkReply::finished, this, [this, reply, requestId](){
        reply->deleteLater();

        if (reply->error() != QNetworkReply::NoError) {
            qCWarning(dcDoorBird) << "Error unlatching DoorBird device"  << reply->error() << reply->errorString();
            emit requestSent(requestId, false);
            return;
        }
        qCDebug(dcDoorBird) << "DoorBird light on:";
        emit requestSent(requestId, true);

        QImage* image = new QImage();
        image->loadFromData(reply->readAll());
    });
    return requestId;
}

QUuid Doorbird::historyImageRequest(int index)
{
    QUrl url(QString("http://%1/bha-api/history.cgi").arg(m_address.toString()));
    QUrlQuery query;
    query.addQueryItem("index", QString::number(index));
    url.setQuery(query);

    qCDebug(dcDoorBird) << "Sending request:" << url;
    QNetworkReply *reply = m_networkAccessManager->get(QNetworkRequest(url));
    QUuid requestId = QUuid::createUuid();
    connect(reply, &QNetworkReply::finished, this, [this, reply, requestId](){
        reply->deleteLater();

        if (reply->error() != QNetworkReply::NoError) {
            qCWarning(dcDoorBird) << "Error unlatching DoorBird device";
            emit requestSent(requestId, false);
            return;
        }
        qCDebug(dcDoorBird) << "DoorBird unlatched:" << reply->error() << reply->errorString();
        emit requestSent(requestId, true);
    });
    return requestId;
}

QUuid Doorbird::liveAudioReceive()
{
    QNetworkRequest request(QString("http://%1/bha-api/audio-receive.cgi").arg(m_address.toString()));
    qCDebug(dcDoorBird) << "Sending request:" << request.url();
    QNetworkReply *reply = m_networkAccessManager->get(request);
    QUuid requestId = QUuid::createUuid();
    connect(reply, &QNetworkReply::finished, this, [this, reply, requestId](){
        reply->deleteLater();

        if (reply->error() != QNetworkReply::NoError) {
            qCWarning(dcDoorBird) << "Error DoorBird device";
            emit requestSent(requestId, false);
            return;
        }
        qCDebug(dcDoorBird) << "DoorBird unlatched:" << reply->error() << reply->errorString();
        emit requestSent(requestId, true);
    });
    return requestId;
}

QUuid Doorbird::liveAudioTransmit()
{

    return QUuid::createUuid();
}

QUuid Doorbird::infoRequest()
{
    QNetworkRequest request(QString("http://%1/bha-api/info.cgi").arg(m_address.toString()));
    qCDebug(dcDoorBird) << "Sending request:" << request.url();
    QNetworkReply *reply = m_networkAccessManager->get(request);
    QUuid requestId = QUuid::createUuid();
    connect(reply, &QNetworkReply::finished, this, [this, reply, requestId](){
        reply->deleteLater();

        if (reply->error() != QNetworkReply::NoError) {
            qCWarning(dcDoorBird) << "Error DoorBird" << reply->error() << reply->errorString();
            emit requestSent(requestId, false);
            return;
        }
        qCDebug(dcDoorBird) << "DoorBird info:" << reply->readAll() ;
        emit requestSent(requestId, true);
    });
    return requestId;
}

QUuid Doorbird::listFavorites()
{
    QNetworkRequest request(QString("http://%1/bha-api/favorites.cgi").arg(m_address.toString()));
    qCDebug(dcDoorBird) << "Sending request:" << request.url();
    QNetworkReply *reply = m_networkAccessManager->get(request);
    QUuid requestId = QUuid::createUuid();
    connect(reply, &QNetworkReply::finished, this, [this, reply, requestId](){
        reply->deleteLater();

        if (reply->error() != QNetworkReply::NoError) {
            qCWarning(dcDoorBird) << "Error DoorBird device" << reply->error() << reply->errorString();
            emit requestSent(requestId, false);
            return;
        }
        emit requestSent(requestId, true);
    });
    return requestId;
}

QUuid Doorbird::addFavorite(FavoriteType type, const QString &name, const QUrl &commandUrl, int id)
{
    QUrl url(QString("http://%1/bha-api/favorites.cgi").arg(m_address.toString()));
    QUrlQuery query;
    query.addQueryItem("action", "save");
    if (type == FavoriteType::Http) {
        query.addQueryItem("type", "http");
    } else {
        query.addQueryItem("type", "sip");
    }
    query.addQueryItem("title", name);
    query.addQueryItem("value", commandUrl.toString());
    query.addQueryItem("id", QString::number(id));
    url.setQuery(query);

    QNetworkReply *reply = m_networkAccessManager->get(QNetworkRequest(url));
    QUuid requestId = QUuid::createUuid();
    connect(reply, &QNetworkReply::finished, this, [this, reply, requestId](){
        reply->deleteLater();

        if (reply->error() != QNetworkReply::NoError) {
            qCWarning(dcDoorBird) << "Error DoorBird device" << reply->error() << reply->errorString();
            emit requestSent(requestId, false);
            return;
        }
        emit requestSent(requestId, true);
    });
    return requestId;
}

QUuid Doorbird::listSchedules()
{
    QNetworkRequest request(QString("http://%1/bha-api/schedule.cgi").arg(m_address.toString()));
    qCDebug(dcDoorBird) << "Sending request:" << request.url();
    QNetworkReply *reply = m_networkAccessManager->get(request);
    QUuid requestId = QUuid::createUuid();
    connect(reply, &QNetworkReply::finished, this, [this, reply, requestId](){
        reply->deleteLater();

        if (reply->error() != QNetworkReply::NoError) {
            qCWarning(dcDoorBird) << "Error DoorBird device" << reply->error() << reply->errorString();
            emit requestSent(requestId, false);
            return;
        }
        emit requestSent(requestId, true);
    });
    return requestId;
}

QUuid Doorbird::restart()
{
    QNetworkRequest request(QString("http://%1/bha-api/restart.cgi").arg(m_address.toString()));
    qCDebug(dcDoorBird) << "Sending request:" << request.url();
    QNetworkReply *reply = m_networkAccessManager->get(request);
    QUuid requestId = QUuid::createUuid();
    connect(reply, &QNetworkReply::finished, this, [this, reply, requestId](){
        reply->deleteLater();

        if (reply->error() != QNetworkReply::NoError) {
            qCWarning(dcDoorBird) << "Error restarting DoorBird device" << reply;
            emit requestSent(requestId, false);
            return;
        }
        qCDebug(dcDoorBird) << "DoorBird restarting";
        emit requestSent(requestId, true);
    });
    return requestId;
}

void Doorbird::connectToEventMonitor()
{
    qCDebug(dcDoorBird) << "Starting monitoring";

    QNetworkRequest request(QString("http://%1/bha-api/monitor.cgi?ring=doorbell,motionsensor").arg(m_address.toString()));
    QNetworkReply *reply = m_networkAccessManager->get(request);

    connect(reply, &QNetworkReply::downloadProgress, this, [this, reply](qint64 bytesReceived, qint64 bytesTotal){
        Q_UNUSED(bytesReceived)
        Q_UNUSED(bytesTotal);

        emit deviceConnected(true);
        m_readBuffer.append(reply->readAll());
        qCDebug(dcDoorBird) << "Event received" << m_readBuffer;

        // Input data looks like:
        // "--ioboundary\r\nContent-Type: text/plain\r\n\r\ndoorbell:H\r\n\r\n"

        while (!m_readBuffer.isEmpty()) {
            // find next --ioboundary
            QString boundary = QStringLiteral("--ioboundary");
            int startIndex = m_readBuffer.indexOf(boundary);
            if (startIndex == -1) {
                qCWarning(dcDoorBird) << "No meaningful data in buffer:" << m_readBuffer;
                if (m_readBuffer.size() > 1024) {
                    qCWarning(dcDoorBird) << "Buffer size > 1KB and still no meaningful data. Discarding buffer...";
                    m_readBuffer.clear();
                }
                // Assuming we don't have enough data yet...
                return;
            }

            QByteArray contentType = QByteArrayLiteral("Content-Type: text/plain");
            int contentTypeIndex = m_readBuffer.indexOf(contentType);
            if (contentTypeIndex == -1) {
                qCWarning(dcDoorBird) << "Cannot find Content-Type in buffer:" << m_readBuffer;
                if (m_readBuffer.size() > startIndex + 50) {
                    qCWarning(dcDoorBird) << boundary << "found but unexpected data follows. Skipping this element...";
                    m_readBuffer.remove(0, startIndex + boundary.length());
                    continue;
                }
                // Assuming we don't have enough data yet...
                return;
            }

            // At this point we have the boundary and Content-Type. Remove all of that and take the entire string to either end or next boundary
            m_readBuffer.remove(0, contentTypeIndex + contentType.length());
            int nextStartIndex = m_readBuffer.indexOf(boundary);
            QByteArray data;
            if (nextStartIndex == -1) {
                data = m_readBuffer;
                m_readBuffer.clear();
            } else {
                data = m_readBuffer.left(nextStartIndex);
                m_readBuffer.remove(0, nextStartIndex);
            }

            QString message = data.trimmed();
            QStringList parts = message.split(":");
            if (parts.count() != 2) {
                qCWarning(dcDoorBird) << "Message has invalid format:" << message << "Expected device:state";
                continue;
            }
            if (parts.first() == "doorbell") {
                if (parts.at(1) == "H") {
                    qCDebug(dcDoorBird) << "Doorbell ringing!";
                    emit eventReveiced(EventType::Doorbell, true);
                } else {
                    emit eventReveiced(EventType::Doorbell, false);
                }
            } else if (parts.first() == "motionsensor") {
                if (parts.at(1) == "H") {
                    qCDebug(dcDoorBird) << "Motion sensor detected a person";
                    emit eventReveiced(EventType::Motion, true);
                } else {
                    emit eventReveiced(EventType::Motion, false);
                }
            } else {
                qCWarning(dcDoorBird) << "Unhandled DoorBird data:" << message;
            }
        }
    });

    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        reply->deleteLater();

        emit deviceConnected(false);
        qCDebug(dcDoorBird) << "Monitor request finished:" << reply->error();

        QTimer::singleShot(2000, this, [this] {
            connectToEventMonitor();
        });
    });
}

void Doorbird::onUdpBroadcast(const QByteArray &data)
{
    Q_UNUSED(data);
    //TODO decryption
}


/*NotifyBroadcastCiphertext decryptBroadcastNotification(const NotifyBroadcast* notification, const
                                                       StretchedPassword* password) {
    NotifyBroadcastCiphertext decrypted = {{0},{0},0};
    if(crypto_aead_chacha20poly1305_decrypt((unsigned char*)&decrypted, NULL, NULL, notification->ciphertext,
                                            sizeof(notification->ciphertext), NULL, 0, notification->nonce, password->key)!=0){
        LOGGING("crypto_aead_chacha20poly1305_decrypt() failed");
    }
    return decrypted;
}

unsigned char* stretchPasswordArgon(const char *password, unsigned char *salt, unsigned* oplimit, unsigned* memlimit) {
    if (sodium_is_zero(salt, CRYPTO_SALT_BYTES) && random_bytes(salt, CRYPTO_SALT_BYTES)) {
        return NULL;
    }
    unsigned char* key = malloc(CRYPTO_ARGON_OUT_SIZE);
    if (!*oplimit) {
        *oplimit = crypto_pwhash_argon2i_OPSLIMIT_INTERACTIVE;
    }
    if (!*memlimit) {
        *memlimit = crypto_pwhash_MEMLIMIT_MIN;
    }
    if (crypto_pwhash(key, CRYPTO_ARGON_OUT_SIZE, password, str_len(password), salt, *oplimit, *memlimit, crypto_pwhash_ALG_ARGON2I13)) {
        LOGGING("Argon2 Failed");
        *oplimit = 0;
        *memlimit = 0;
        CLEAN(key);
        return NULL;
    }
    return key;
}*/
