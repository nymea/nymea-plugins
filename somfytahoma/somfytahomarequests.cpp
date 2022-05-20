/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*
* Copyright 2013 - 2022, nymea GmbH
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

#include "somfytahomarequests.h"

#include <QJsonDocument>
#include <QUrlQuery>

#include "network/networkaccessmanager.h"

#include "extern-plugininfo.h"


static const QString somfyTahomaWebUrl = QStringLiteral("https://ha101-1.overkiz.com/enduser-mobile-web/enduserAPI");
static const QString localSomfyTahomaPath = QStringLiteral("/enduser-mobile-web/1/enduserAPI");

SomfyTahomaRequest::SomfyTahomaRequest(QNetworkReply *reply, QObject *parent) : QObject(parent)
{
    connect(reply, &QNetworkReply::finished, reply, &QNetworkReply::deleteLater);
    connect(reply, &QNetworkReply::finished, this, [this, reply] {
        deleteLater();
        if (reply->error() != QNetworkReply::NoError) {
            qCWarning(dcSomfyTahoma()) << "Request for" << reply->url().path() << "failed:" << reply->errorString();
            emit error(reply->error());
            return;
        }

        QByteArray data = reply->readAll();
        QJsonParseError parseError;
        QJsonDocument jsonDoc = QJsonDocument::fromJson(data, &parseError);
        if (parseError.error != QJsonParseError::NoError) {
            qCWarning(dcSomfyTahoma()) << "Json parse error in reply for" << reply->url().path() << ":" << parseError.errorString();
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
