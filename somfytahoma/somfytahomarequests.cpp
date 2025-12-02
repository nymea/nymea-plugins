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

#include "somfytahomarequests.h"

#include <QJsonDocument>
#include <QUrlQuery>

#include <network/networkaccessmanager.h>

#include "extern-plugininfo.h"

static const QString somfyTahomaWebUrl = QStringLiteral("https://ha101-1.overkiz.com/enduser-mobile-web/enduserAPI");
static const QString localSomfyTahomaPath = QStringLiteral("/enduser-mobile-web/1/enduserAPI");

SomfyTahomaRequest::SomfyTahomaRequest(QNetworkReply *reply, QObject *parent) : QObject(parent)
{
    connect(reply, &QNetworkReply::finished, reply, &QNetworkReply::deleteLater);
    connect(reply, &QNetworkReply::finished, this, [this, reply] {
        deleteLater();
        if (reply->error() != QNetworkReply::NoError) {
            int status = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
            QVariantMap requestHeaders;
            foreach (const QByteArray &rawHeader, reply->request().rawHeaderList()) {
              requestHeaders.insert(rawHeader, reply->request().rawHeader(rawHeader));
            }
            QVariantMap replyHeaders;
            foreach (const QByteArray &rawHeader, reply->rawHeaderList()) {
              replyHeaders.insert(rawHeader, reply->rawHeader(rawHeader));
            }
            qCWarning(dcSomfyTahoma()).noquote() << "Request error:" << status << "for URL:" << reply->url().toString()
                                                 << "on operation" << reply->operation() << "\n" << "Content:" << reply->readAll();
            qCDebug(dcSomfyTahoma()).noquote() << "Request headers:" << QJsonDocument::fromVariant(requestHeaders).toJson()
                                               << "Reply headers:" << QJsonDocument::fromVariant(replyHeaders).toJson();
            emit error(reply->error());
            return;
        }

        QByteArray data = reply->readAll();
        QJsonParseError parseError;
        QJsonDocument jsonDoc = QJsonDocument::fromJson(data, &parseError);
        if (parseError.error != QJsonParseError::NoError) {
            qCWarning(dcSomfyTahoma()) << "Json parse error:" << reply->url().toString() << ":" << parseError.error << parseError.errorString();
            emit error(QNetworkReply::UnknownContentError);
            return;
        }

        emit finished(jsonDoc.toVariant());
    });
}

SomfyTahomaRequest *createCloudSomfyTahomaPostRequest(NetworkAccessManager *networkManager, const QString &path, const QString &contentType, const QByteArray &body, QObject *parent)
{
    QUrl url(somfyTahomaWebUrl + path);
    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::KnownHeaders::ContentTypeHeader, contentType);
    QNetworkReply *reply = networkManager->post(request, body);
    return new SomfyTahomaRequest(reply, parent);
}

SomfyTahomaRequest *createCloudSomfyTahomaGetRequest(NetworkAccessManager *networkManager, const QString &path, QObject *parent)
{
    QUrl url(somfyTahomaWebUrl + path);
    QNetworkRequest request(url);
    QNetworkReply *reply = networkManager->get(request);
    return new SomfyTahomaRequest(reply, parent);
}

SomfyTahomaRequest *createCloudSomfyTahomaDeleteRequest(NetworkAccessManager *networkManager, const QString &path, QObject *parent)
{
    QUrl url(somfyTahomaWebUrl + path);
    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::KnownHeaders::ContentLengthHeader, 0);

    QNetworkReply *reply = networkManager->deleteResource(request);
    return new SomfyTahomaRequest(reply, parent);
}

SomfyTahomaRequest *createCloudSomfyTahomaLoginRequest(NetworkAccessManager *networkManager, const QString &username, const QString &password, QObject *parent)
{
    QUrlQuery postData;
    postData.addQueryItem("userId", username);
    postData.addQueryItem("userPassword", password);
    return createCloudSomfyTahomaPostRequest(networkManager, "/login", "application/x-www-form-urlencoded", postData.toString(QUrl::FullyEncoded).toUtf8(), parent);
}

SomfyTahomaRequest *createLocalSomfyTahomaPostRequest(NetworkAccessManager *networkManager, const QString &host, const QString &token, const QString &path, const QString &contentType, const QByteArray &body, QObject *parent)
{
    QUrl url("https://" + host + localSomfyTahomaPath + path);
    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::KnownHeaders::ContentTypeHeader, contentType);
    request.setRawHeader("Authorization", "Bearer " + token.toUtf8());

    QSslConfiguration config = QSslConfiguration::defaultConfiguration();
    config.setPeerVerifyMode(QSslSocket::VerifyNone);
    request.setSslConfiguration(config);

    QNetworkReply *reply = networkManager->post(request, body);

    return new SomfyTahomaRequest(reply, parent);
}

SomfyTahomaRequest *createLocalSomfyTahomaGetRequest(NetworkAccessManager *networkManager, const QString &host, const QString &token, const QString &path, QObject *parent)
{
    QUrl url("https://" + host + localSomfyTahomaPath + path);
    QNetworkRequest request(url);
    request.setRawHeader("Authorization", "Bearer " + token.toUtf8());

    QSslConfiguration config = QSslConfiguration::defaultConfiguration();
    config.setPeerVerifyMode(QSslSocket::VerifyNone);
    request.setSslConfiguration(config);

    QNetworkReply *reply = networkManager->get(request);

    return new SomfyTahomaRequest(reply, parent);
}

SomfyTahomaRequest *createLocalSomfyTahomaEventFetchRequest(NetworkAccessManager *networkManager, const QString &host, const QString &token, const QString &eventListenerId, QObject *parent)
{
    return createLocalSomfyTahomaPostRequest(networkManager, host, token, "/events/" + eventListenerId + "/fetch", "application/json", QByteArray(), parent);
}
