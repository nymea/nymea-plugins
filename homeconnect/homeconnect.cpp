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

#include "homeconnect.h"
#include "extern-plugininfo.h"

#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QUrlQuery>

HomeConnect::HomeConnect(NetworkAccessManager *networkmanager,  const QByteArray &clientKey,  const QByteArray &clientSecret, bool simulationMode, QObject *parent) :
    QObject(parent),
    m_clientKey(clientKey),
    m_clientSecret(clientSecret),
    m_networkManager(networkmanager)

{
    m_tokenRefreshTimer = new QTimer(this);
    m_tokenRefreshTimer->setSingleShot(true);
    connect(m_tokenRefreshTimer, &QTimer::timeout, this, &HomeConnect::onRefreshTimeout);
    setSimulationMode(simulationMode);
}

QByteArray HomeConnect::accessToken()
{
    return m_accessToken;
}

QByteArray HomeConnect::refreshToken()
{
    return m_refreshToken;
}

void HomeConnect::setSimulationMode(bool simulation)
{
    m_simulationMode = simulation;
    if (simulation) {
        m_baseAuthorizationUrl = "https://simulator.home-connect.com/security/oauth/authorize";
        m_baseTokenUrl = "https://simulator.home-connect.com/security/oauth/token";
        m_baseControlUrl = "https://simulator.home-connect.com";
    } else {
        m_baseAuthorizationUrl = "https://api.home-connect.com/security/oauth/authorize";
        m_baseTokenUrl = "https://api.home-connect.com/security/oauth/token";
        m_baseControlUrl = "https://api.home-connect.com";
    }
}

QUrl HomeConnect::getLoginUrl(const QUrl &redirectUrl, const QString &scope)
{
    if (m_clientKey.isEmpty()) {
        qWarning(dcHomeConnect) << "Client key not defined!";
        return QUrl("");
    }

    if (redirectUrl.isEmpty()){
        qWarning(dcHomeConnect) << "No redirect uri defined!";
    }
    m_redirectUri = QUrl::toPercentEncoding(redirectUrl.toString());

    QUrl url(m_baseAuthorizationUrl);
    QUrlQuery queryParams;
    queryParams.addQueryItem("client_id", m_clientKey);
    queryParams.addQueryItem("redirect_uri", m_redirectUri);
    queryParams.addQueryItem("response_type", "code");
    queryParams.addQueryItem("scope", scope);
    queryParams.addQueryItem("state", QUuid::createUuid().toString());
    queryParams.addQueryItem("nonce", QUuid::createUuid().toString());
    m_codeChallenge = QUuid::createUuid().toString().remove(QRegExp("[{}-]"));
    queryParams.addQueryItem("code_challenge", m_codeChallenge);
    queryParams.addQueryItem("code_challenge_method", "plain");
    url.setQuery(queryParams);

    return url;
}

void HomeConnect::onRefreshTimeout()
{
    qCDebug(dcHomeConnect) << "Refresh authentication token";
    getAccessTokenFromRefreshToken(m_refreshToken);
}

bool HomeConnect::checkStatusCode(QNetworkReply *reply, const QByteArray &rawData)
{
    // Check for the internet connection
    if (reply->error() == QNetworkReply::NetworkError::HostNotFoundError ||
            reply->error() == QNetworkReply::NetworkError::UnknownNetworkError ||
            reply->error() == QNetworkReply::NetworkError::TemporaryNetworkFailureError) {
        qCWarning(dcHomeConnect()) << "Connection error" << reply->errorString();
        setConnected(false);
        setAuthenticated(false);
        return false;
    } else {
        setConnected(true);
    }
    int status = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
    QJsonParseError error;
    QJsonDocument jsonDoc = QJsonDocument::fromJson(rawData, &error);

    switch (status){
    case 200: //The request was successful. Typically returned for successful GET requests.
    case 204: //The request was successful. Typically returned for successful PUT/DELETE requests with no payload.
        break;
    case 400: //Error occurred (e.g. validation error - value is out of range)
        if(!jsonDoc.toVariant().toMap().contains("error")) {
            if(jsonDoc.toVariant().toMap().value("error").toString() == "invalid_client") {
                qWarning(dcHomeConnect()) << "Client token provided doesn’t correspond to client that generated auth code.";
            }
            if(jsonDoc.toVariant().toMap().value("error").toString() == "invalid_redirect_uri") {
                qWarning(dcHomeConnect()) << "Missing redirect_uri parameter.";
            }
            if(jsonDoc.toVariant().toMap().value("error").toString() == "invalid_code") {
                qWarning(dcHomeConnect()) << "Expired authorization code.";
            }
        }
        setAuthenticated(false);
        return false;
    case 401:
        qWarning(dcHomeConnect()) << "Client does not have permission to use this API.";
        setAuthenticated(false);
        return false;
    case 403:
        qCWarning(dcHomeConnect()) << "Forbidden, Scope has not been granted or home appliance is not assigned to HC account";
        setAuthenticated(false);
        return false;
    case 404:
        qCWarning(dcHomeConnect()) << "Not Found. This resource is not available (e.g. no images on washing machine)";
        return false;
    case 405:
        qWarning(dcHomeConnect()) << "Wrong HTTP method used.";
        setAuthenticated(false);
        return false;
    case 408:
        qCWarning(dcHomeConnect())<< "Request Timeout, API Server failed to produce an answer or has no connection to backend service";
        return false;
    case 409:
        qCWarning(dcHomeConnect()) << "Conflict - Command/Query cannot be executed for the home appliance, the error response contains the error details";
        qCWarning(dcHomeConnect()) << "Error" << jsonDoc.toVariant().toMap().value("error").toString();
        return false;
    case 415:
        qCWarning(dcHomeConnect())<< "Unsupported Media Type. The request's Content-Type is not supported";
        return false;
    case 429:
        qCWarning(dcHomeConnect())<< "Too Many Requests, the number of requests for a specific endpoint exceeded the quota of the client";
        return false;
    case 500:
        qCWarning(dcHomeConnect())<< "Internal Server Error, in case of a server configuration error or any errors in resource files";
        return false;
    case 503:
        qCWarning(dcHomeConnect())<< "Service Unavailable,if a required backend service is not available";
        return false;
    default:
        break;
    }

    if (error.error != QJsonParseError::NoError) {
        qCWarning(dcHomeConnect()) << "Received invalide JSON object" << rawData;
        qCWarning(dcHomeConnect()) << "Status" << status;
        setAuthenticated(false);
        return false;
    }

    setAuthenticated(true);
    return true;
}

void HomeConnect::getAccessTokenFromRefreshToken(const QByteArray &refreshToken)
{
    if (refreshToken.isEmpty()) {
        qWarning(dcHomeConnect) << "No refresh token given!";
        setAuthenticated(false);
        return;
    }

    QUrl url(m_baseTokenUrl);
    QUrlQuery query;
    query.clear();
    query.addQueryItem("grant_type", "refresh_token");
    query.addQueryItem("refresh_token", refreshToken);
    query.addQueryItem("client_secret", m_clientSecret);

    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/x-www-form-urlencoded");

    QNetworkReply *reply = m_networkManager->post(request, query.toString().toUtf8());
    connect(reply, &QNetworkReply::finished, reply, &QNetworkReply::deleteLater);
    connect(reply, &QNetworkReply::finished, this, [this, reply](){

        QByteArray rawData = reply->readAll();
        if (!checkStatusCode(reply, rawData)) {
            return;
        }
        QJsonDocument data = QJsonDocument::fromJson(rawData);

        if(!data.toVariant().toMap().contains("access_token")) {
            setAuthenticated(false);
            return;
        }
        m_accessToken = data.toVariant().toMap().value("access_token").toByteArray();
        emit receivedAccessToken(m_accessToken);

        if (data.toVariant().toMap().contains("expires_in")) {
            int expireTime = data.toVariant().toMap().value("expires_in").toInt();
            qCDebug(dcHomeConnect) << "Access token expires int" << expireTime << "s, at" << QDateTime::currentDateTime().addSecs(expireTime).toString();
            if (!m_tokenRefreshTimer) {
                qWarning(dcHomeConnect()) << "Access token refresh timer not initialized";
                return;
            }
            if (expireTime < 20) {
                qCWarning(dcHomeConnect()) << "Expire time too short";
                return;
            }
            m_tokenRefreshTimer->start((expireTime - 20) * 1000);
        }
    });
}

void HomeConnect::getAccessTokenFromAuthorizationCode(const QByteArray &authorizationCode)
{
    // Obtaining access token
    if(authorizationCode.isEmpty())
        qWarning(dcHomeConnect) << "No authorization code given!";
    if(m_clientKey.isEmpty())
        qWarning(dcHomeConnect) << "Client key not set!";
    if(m_clientSecret.isEmpty())
        qWarning(dcHomeConnect) << "Client secret not set!";

    QUrl url = QUrl(m_baseTokenUrl);
    QUrlQuery query;    url.setQuery(query);

    query.clear();
    query.addQueryItem("client_id", m_clientKey);
    query.addQueryItem("client_secret", m_clientSecret);
    query.addQueryItem("redirect_uri", m_redirectUri);
    query.addQueryItem("grant_type", "authorization_code");
    query.addQueryItem("code", authorizationCode);
    query.addQueryItem("code_verifier", m_codeChallenge);

    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/x-www-form-urlencoded");

    QNetworkReply *reply = m_networkManager->post(request, query.toString().toUtf8());
    connect(reply, &QNetworkReply::finished, reply, &QNetworkReply::deleteLater);
    connect(reply, &QNetworkReply::finished, this, [this, reply](){

        QByteArray rawData = reply->readAll();
        if (!checkStatusCode(reply, rawData)) {
            return;
        }
        QJsonDocument jsonDoc = QJsonDocument::fromJson(rawData);
        if(!jsonDoc.toVariant().toMap().contains("access_token") || !jsonDoc.toVariant().toMap().contains("refresh_token") ) {
            setAuthenticated(false);
            return;
        }
        m_accessToken = jsonDoc.toVariant().toMap().value("access_token").toByteArray();
        receivedAccessToken(m_accessToken);
        m_refreshToken = jsonDoc.toVariant().toMap().value("refresh_token").toByteArray();
        receivedRefreshToken(m_refreshToken);

        if (jsonDoc.toVariant().toMap().contains("expires_in")) {
            int expireTime = jsonDoc.toVariant().toMap().value("expires_in").toInt();
            qCDebug(dcHomeConnect()) << "Token expires in" << expireTime << "s, at" << QDateTime::currentDateTime().addSecs(expireTime).toString();
            if (!m_tokenRefreshTimer) {
                qWarning(dcHomeConnect()) << "Token refresh timer not initialized";
                setAuthenticated(false);
                return;
            }
            if (expireTime < 20) {
                qCWarning(dcHomeConnect()) << "Expire time too short";
                return;
            }
            m_tokenRefreshTimer->start((expireTime - 20) * 1000);
        }
    });
}

void HomeConnect::getHomeAppliances()
{
    QUrl url = QUrl(m_baseControlUrl+"/api/homeappliances");

    QNetworkRequest request(url);
    request.setRawHeader("Authorization", "Bearer "+m_accessToken);
    request.setRawHeader("accept", "application/vnd.bsh.sdk.v1+json");

    QNetworkReply *reply = m_networkManager->get(request);
    connect(reply, &QNetworkReply::finished, reply, &QNetworkReply::deleteLater);
    connect(reply, &QNetworkReply::finished, this, [this, reply](){

        QByteArray rawData = reply->readAll();
        if (!checkStatusCode(reply, rawData)) {
            return;
        }
        QVariantMap dataMap = QJsonDocument::fromJson(rawData).toVariant().toMap().value("data").toMap();

        QList<HomeAppliance> appliances;
        foreach (const QVariant &variant, dataMap.value("homeappliances").toList()) {
            QVariantMap obj = variant.toMap();
            HomeAppliance appliance;
            appliance.name = obj["name"].toString();
            appliance.brand = obj["brand"].toString();
            appliance.vib = obj["vib"].toString();
            appliance.type = obj["type"].toString();
            appliance.homeApplianceId = obj["haId"].toString();
            appliance.enumber = obj["enumber"].toString();
            appliance.connected = obj["connected"].toBool();
            appliances.append(appliance);
        }
        if (!appliances.isEmpty())
            emit receivedHomeAppliances(appliances);

    });
}

void HomeConnect::getPrograms(const QString &haId)
{
    QUrl url = QUrl(m_baseControlUrl+"/api/homeappliances/"+haId+"/programs");

    QNetworkRequest request(url);
    request.setRawHeader("Authorization", "Bearer "+m_accessToken);
    request.setRawHeader("Accept-Language", "en-US");
    request.setRawHeader("accept", "application/vnd.bsh.sdk.v1+json");

    QNetworkReply *reply = m_networkManager->get(request);
    connect(reply, &QNetworkReply::finished, reply, &QNetworkReply::deleteLater);
    connect(reply, &QNetworkReply::finished, this, [this, haId, reply]{

        QByteArray rawData = reply->readAll();
        if (!checkStatusCode(reply, rawData)) {
            return;
        }

        QVariantMap dataMap = QJsonDocument::fromJson(rawData).toVariant().toMap().value("data").toMap();
        QVariantList programList = dataMap.value("programs").toList();
        QStringList programs;
        Q_FOREACH(QVariant var, programList) {
            if (var.toMap().contains("key")) {
                programs.append(var.toMap().value("key").toString());
            }
        }
        if (!programs.isEmpty())
            emit receivedPrograms(haId, programs);
    });
}

void HomeConnect::getProgramsAvailable(const QString &haId)
{
    QUrl url = QUrl(m_baseControlUrl+"/api/homeappliances/"+haId+"/programs/available");

    QNetworkRequest request(url);
    request.setRawHeader("Authorization", "Bearer "+m_accessToken);
    request.setRawHeader("Accept-Language", "en-US");
    request.setRawHeader("accept", "application/vnd.bsh.sdk.v1+json");

    QNetworkReply *reply = m_networkManager->get(request);
    connect(reply, &QNetworkReply::finished, reply, &QNetworkReply::deleteLater);
    connect(reply, &QNetworkReply::finished, this, [this, haId, reply]{

        QByteArray rawData = reply->readAll();
        if (!checkStatusCode(reply, rawData)) {
            return;
        }

        QVariantMap dataMap = QJsonDocument::fromJson(rawData).toVariant().toMap().value("data").toMap();
        QVariantList programList = dataMap.value("programs").toList();
        QStringList programs;
        Q_FOREACH(QVariant var, programList) {
            if (var.toMap().contains("key")) {
                programs.append(var.toMap().value("key").toString());
            }
        }
        if (!programs.isEmpty())
            emit receivedAvailablePrograms(haId, programs);
    });
}

void HomeConnect::getProgramsActive(const QString &haId)
{
    QUrl url = QUrl(m_baseControlUrl+"/api/homeappliances/"+haId+"/programs/active");

    QNetworkRequest request(url);
    request.setRawHeader("Authorization", "Bearer "+m_accessToken);
    request.setRawHeader("Accept-Language", "en-US");
    request.setRawHeader("accept", "application/vnd.bsh.sdk.v1+json");

    QNetworkReply *reply = m_networkManager->get(request);
    connect(reply, &QNetworkReply::finished, reply, &QNetworkReply::deleteLater);
    connect(reply, &QNetworkReply::finished, this, [this, haId, reply]{

        QByteArray rawData = reply->readAll();
        if (!checkStatusCode(reply, rawData)) {
            return;
        }

        QVariantMap map = QJsonDocument::fromJson(rawData).toVariant().toMap();
        QHash<QString, QVariant> options;
        if (map.contains("data")) {
            QString key = map.value("data").toMap().value("key").toString();
            Q_FOREACH(QVariant var, map.value("data").toMap().value("options").toList()) {
                options.insert(var.toMap().value("key").toString(), var.toMap().value("value"));
            }
            emit receivedActiveProgram(haId, key, options);
        } else if (map.contains("error")) {
            QString key = map.value("error").toMap().value("key").toString();
            emit receivedActiveProgram(haId, key, options);
        }
    });
}

void HomeConnect::getProgramsSelected(const QString &haId)
{
    QUrl url = QUrl(m_baseControlUrl+"/api/homeappliances/"+haId+"/programs/selected");

    QNetworkRequest request(url);
    request.setRawHeader("Authorization", "Bearer "+m_accessToken);
    request.setRawHeader("Accept-Language", "en-US");
    request.setRawHeader("accept", "application/vnd.bsh.sdk.v1+json");

    QNetworkReply *reply = m_networkManager->get(request);
    connect(reply, &QNetworkReply::finished, reply, &QNetworkReply::deleteLater);
    connect(reply, &QNetworkReply::finished, this, [this, haId, reply]{

        QByteArray rawData = reply->readAll();
        if (!checkStatusCode(reply, rawData)) {
            return;
        }

        QVariantMap map = QJsonDocument::fromJson(rawData).toVariant().toMap();
        QHash<QString, QVariant> options;
        if (map.contains("data")) {
            QString key = map.value("data").toMap().value("key").toString();
            Q_FOREACH(QVariant var, map.value("data").toMap().value("options").toList()) {
                options.insert(var.toMap().value("key").toString(), var.toMap().value("value"));
            }
            emit receivedSelectedProgram(haId, key, options);
        } else if (map.contains("error")) {
            QString key = map.value("error").toMap().value("key").toString();
            emit receivedSelectedProgram(haId, key, options);
        }
    });
}

void HomeConnect::getProgramsActiveOption(const QString &haId, const QString &optionKey)
{
    QUrl url = QUrl(m_baseControlUrl+"/api/homeappliances/"+haId+"/programs/active/options/"+optionKey);

    QNetworkRequest request(url);
    request.setRawHeader("Authorization", "Bearer "+m_accessToken);
    request.setRawHeader("Accept-Language", "en-US");
    request.setRawHeader("accept", "application/vnd.bsh.sdk.v1+json");

    QNetworkReply *reply = m_networkManager->get(request);
    connect(reply, &QNetworkReply::finished, reply, &QNetworkReply::deleteLater);
    connect(reply, &QNetworkReply::finished, this, [this, reply]{

        QByteArray rawData = reply->readAll();
        if (!checkStatusCode(reply, rawData)) {
            return;
        }
        QVariantMap dataMap = QJsonDocument::fromJson(rawData).toVariant().toMap().value("data").toMap();
        qCDebug(dcHomeConnect()) << "key" << dataMap.value("key").toString() << "value" << dataMap.value("value").toString() << dataMap.value("unit").toString();
    });
}

QUuid HomeConnect::selectProgram(const QString &haId, const QString &programKey, QList<HomeConnect::Option> options)
{
    QUuid commandId = QUuid::createUuid();
    QUrl url = QUrl(m_baseControlUrl+"/api/homeappliances/"+haId+"/programs/selected");

    QNetworkRequest request(url);
    request.setRawHeader("Authorization", "Bearer "+m_accessToken);
    request.setRawHeader("Accept-Language", "en-US");
    request.setRawHeader("accept", "application/vnd.bsh.sdk.v1+json");
    request.setHeader(QNetworkRequest::KnownHeaders::ContentTypeHeader, "application/vnd.bsh.sdk.v1+json");

    QJsonDocument doc;
    QVariantMap data;
    data.insert("key", programKey);
    if (!options.isEmpty()) {
        QVariantList optionsArray;
        Q_FOREACH(Option option, options) {
            QVariantMap optionObject;
            optionObject["key"] = option.key;
            optionObject["value"] = option.value;
            if (!option.unit.isEmpty())
                optionObject["unit"] = option.unit;
            optionsArray.append(optionObject);
        }
        data.insert("options", optionsArray);
    }
    QVariantMap obj;
    obj.insert("data", data);
    doc.setObject(QJsonObject::fromVariantMap(obj));
    QNetworkReply *reply = m_networkManager->put(request, doc.toJson());
    connect(reply, &QNetworkReply::finished, reply, &QNetworkReply::deleteLater);
    connect(reply, &QNetworkReply::finished, this, [this, commandId, reply]{

        int status = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
        if (status != 204) {
            emit commandExecuted(commandId, false);
            QByteArray rawData = reply->readAll();
            qCDebug(dcHomeConnect()) << "Selected program" << rawData;
            QJsonParseError error;
            QJsonDocument data = QJsonDocument::fromJson(rawData, &error);
            if (error.error != QJsonParseError::NoError) {
                qCDebug(dcHomeConnect()) << "Selected program: Received invalide JSON object";
                return;
            }
            if (data.toVariant().toMap().contains("error")) {
                qCWarning(dcHomeConnect()) << "Start program" << data.toVariant().toMap().value("error").toMap().value("description").toString();
                return;
            }
        } else {
            emit commandExecuted(commandId, true);
        }
    });
    return commandId;
}

QUuid HomeConnect::setSelectedProgramOptions(const QString &haId, QList<HomeConnect::Option> options)
{
    if (options.isEmpty())
        return "";

    QUuid commandId = QUuid::createUuid();
    QUrl url = QUrl(m_baseControlUrl+"/api/homeappliances/"+haId+"/programs/selected/options");

    QNetworkRequest request(url);
    request.setRawHeader("Authorization", "Bearer "+m_accessToken);
    request.setRawHeader("Accept-Language", "en-US");
    request.setRawHeader("accept", "application/vnd.bsh.sdk.v1+json");
    request.setHeader(QNetworkRequest::KnownHeaders::ContentTypeHeader, "application/vnd.bsh.sdk.v1+json");

    QJsonDocument doc;
    QVariantMap data;
    QVariantList optionsArray;
    Q_FOREACH(Option option, options) {
        QVariantMap optionObject;
        optionObject["key"] = option.key;
        optionObject["value"] = option.value;
        if (!option.unit.isEmpty())
            optionObject["unit"] = option.unit;
        optionsArray.append(optionObject);
    }
    data.insert("options", optionsArray);
    QVariantMap obj;
    obj.insert("data", data);
    doc.setObject(QJsonObject::fromVariantMap(obj));
    qCDebug(dcHomeConnect()) << "Selected Program Options" << doc.toJson();

    QNetworkReply *reply = m_networkManager->put(request, doc.toJson());
    connect(reply, &QNetworkReply::finished, reply, &QNetworkReply::deleteLater);
    connect(reply, &QNetworkReply::finished, this, [this, commandId, reply]{

        int status = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
        if (status != 204) {
            emit commandExecuted(commandId, false);
            QByteArray rawData = reply->readAll();
            qCDebug(dcHomeConnect()) << "Selected program" << rawData;
            QJsonParseError error;
            QJsonDocument data = QJsonDocument::fromJson(rawData, &error);
            if (error.error != QJsonParseError::NoError) {
                qCDebug(dcHomeConnect()) << "Selected program options: Received invalide JSON object";
                return;
            }
            if (data.toVariant().toMap().contains("error")) {
                qCWarning(dcHomeConnect()) << "Selected program options:" << data.toVariant().toMap().value("error").toMap().value("description").toString();
                return;
            }
        } else {
            emit commandExecuted(commandId, true);
        }
    });
    return commandId;
}

QUuid HomeConnect::startProgram(const QString &haId, const QString &programKey, QList<HomeConnect::Option> options)
{
    QUuid commandId = QUuid::createUuid();
    QUrl url = QUrl(m_baseControlUrl+"/api/homeappliances/"+haId+"/programs/active");

    QNetworkRequest request(url);
    request.setRawHeader("Authorization", "Bearer "+m_accessToken);
    request.setRawHeader("Accept-Language", "en-US");
    request.setRawHeader("accept", "application/vnd.bsh.sdk.v1+json");
    request.setHeader(QNetworkRequest::KnownHeaders::ContentTypeHeader, "application/vnd.bsh.sdk.v1+json");

    QJsonDocument doc;
    QVariantMap data;
    data.insert("key", programKey);
    QVariantList optionsArray;
    Q_FOREACH(Option option, options) {
        QVariantMap optionObject;
        optionObject["key"] = option.key;
        optionObject["value"] = option.value;
        if (!option.unit.isEmpty())
            optionObject["unit"] = option.unit;
        optionsArray.append(optionObject);
    }
    data.insert("options", optionsArray);
    QVariantMap obj;
    obj.insert("data", data);
    doc.setObject(QJsonObject::fromVariantMap(obj));
    QNetworkReply *reply = m_networkManager->put(request, doc.toJson());
    connect(reply, &QNetworkReply::finished, reply, &QNetworkReply::deleteLater);
    connect(reply, &QNetworkReply::finished, this, [this, commandId, reply]{

        int status = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
        if (status != 204) {
            emit commandExecuted(commandId, false);
            QJsonParseError error;
            QJsonDocument data = QJsonDocument::fromJson(reply->readAll(), &error);
            if (error.error != QJsonParseError::NoError) {
                qCDebug(dcHomeConnect()) << "Start program: Received invalide JSON object";
                return;
            }
            qCDebug(dcHomeConnect()) << "Start program response" << data.toJson();
            if (data.toVariant().toMap().contains("error")) {
                qCWarning(dcHomeConnect()) << "Start program" << data.toVariant().toMap().value("error").toMap().value("description").toString();
            }
        } else {
            emit commandExecuted(commandId, true);
        }
    });
    return commandId;
}

QUuid HomeConnect::stopProgram(const QString &haId)
{
    QUuid commandId = QUuid::createUuid();
    QUrl url = QUrl(m_baseControlUrl+"/api/homeappliances/"+haId+"/programs/active");

    QNetworkRequest request(url);
    request.setRawHeader("Authorization", "Bearer "+m_accessToken);
    request.setRawHeader("Accept-Language", "en-US");
    request.setRawHeader("accept", "application/vnd.bsh.sdk.v1+json");

    QNetworkReply *reply = m_networkManager->deleteResource(request);
    connect(reply, &QNetworkReply::finished, reply, &QNetworkReply::deleteLater);
    connect(reply, &QNetworkReply::finished, this, [this, commandId, reply]{


        int status = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
        if (status != 204) {
            emit commandExecuted(commandId, false);
        } else {
            emit commandExecuted(commandId, true);
        }
    });
    return commandId;
}

void HomeConnect::getStatus(const QString &haid)
{
    QUrl url = QUrl(m_baseControlUrl+"/api/homeappliances/"+haid+"/status");

    QNetworkRequest request(url);
    request.setRawHeader("Authorization", "Bearer "+m_accessToken);
    request.setRawHeader("Accept-Language", "en-US");
    request.setRawHeader("accept", "application/vnd.bsh.sdk.v1+json");

    QNetworkReply *reply = m_networkManager->get(request);
    connect(reply, &QNetworkReply::finished, reply, &QNetworkReply::deleteLater);
    connect(reply, &QNetworkReply::finished, this, [this, haid, reply]{

        QByteArray rawData = reply->readAll();
        if (!checkStatusCode(reply, rawData)) {
            return;
        }

        QVariantMap dataMap = QJsonDocument::fromJson(rawData).toVariant().toMap().value("data").toMap();
        QHash<QString, QVariant> statusList;
        QVariantList statusVariantList= dataMap.value("status").toList();
        Q_FOREACH(QVariant status, statusVariantList) {
            QVariantMap map = status.toMap();
            statusList.insert(map.value("key").toString(), map.value("value"));
        }
        if (!statusList.isEmpty())
            emit receivedStatusList(haid, statusList);
    });
}

/* Get a list of available setting of the home appliance.
 * Possible Settings:
 *      Power state
 *      Fridge temperature
 *      Fridge super mode
 *      Freezer temperature
 *      Freezer super mode
 */

void HomeConnect::getSettings(const QString &haid)
{
    QUrl url = QUrl(m_baseControlUrl+"/api/homeappliances/"+haid+"/settings");

    QNetworkRequest request(url);
    request.setRawHeader("Authorization", "Bearer "+m_accessToken);
    request.setRawHeader("Accept-Language", "en-US");
    request.setRawHeader("accept", "application/vnd.bsh.sdk.v1+json");

    QNetworkReply *reply = m_networkManager->get(request);
    connect(reply, &QNetworkReply::finished, reply, &QNetworkReply::deleteLater);
    connect(reply, &QNetworkReply::finished, this, [this, haid, reply]{

        QByteArray rawData = reply->readAll();
        if (!checkStatusCode(reply, rawData)) {
            return;
        }
        QVariantMap dataMap = QJsonDocument::fromJson(rawData).toVariant().toMap().value("data").toMap();
        QVariantList settingsList = dataMap.value("settings").toList();
        QHash<QString, QVariant> settings;
        Q_FOREACH(QVariant var, settingsList) {
            settings.insert(var.toMap().value("key").toString(), var.toMap().value("value"));
        }
        if (!settings.isEmpty())
            emit receivedSettings(haid, settings);
    });
}

void HomeConnect::connectEventStream()
{
    QUrl url = QUrl(m_baseControlUrl+"/api/homeappliances/events");

    QNetworkRequest request(url);
    request.setRawHeader("Authorization", "Bearer "+m_accessToken);
    request.setRawHeader("Accept-Language", "en-US");
    request.setRawHeader("accept", "text/event-stream");

    QNetworkReply *reply = m_networkManager->get(request);
    connect(reply, &QNetworkReply::finished, [reply, this] {
        reply->deleteLater();
        QTimer::singleShot(5000, this, [this] {connectEventStream();}); //try to reconnect every 5 seconds
    });
    connect(reply, &QNetworkReply::readyRead, this, [this, reply]{

        while (reply->canReadLine()) {
            QJsonDocument data;
            QString haId;
            EventType eventType;

            QByteArray eventTypeLine = reply->readLine();
            if (eventTypeLine == "\n")
                continue;
            if (eventTypeLine.startsWith("event")) {
                QString eventString = eventTypeLine.split(':').last().trimmed();
                if (eventString == "KEEP-ALIVE") {
                    eventType = EventTypeKeepAlive;
                } else if (eventString == "STATUS") {
                    eventType = EventTypeStatus;
                } else if (eventString == "EVENT") {
                    eventType = EventTypeEvent;
                } else if (eventString == "NOTIFY") {
                    eventType = EventTypeNotify;
                } else if (eventString == "DISCONNECTED") {
                    eventType = EventTypeDisconnected;
                } else if (eventString == "CONNECTED") {
                    eventType = EventTypeConnected;
                } else if (eventString == "PAIRED") {
                    eventType = EventTypePaired;
                } else if (eventString == "DEPAIRED") {
                    eventType = EventTypeDepaired;
                } else {
                    qCWarning(dcHomeConnect()) << "Unhandled event type" << eventString;
                    return;
                }
                QByteArray dataLine = reply->readLine();
                if (dataLine.startsWith("data")) {
                    data = QJsonDocument::fromJson(dataLine.remove(0,6));

                    QByteArray idLine = reply->readLine();
                    if (idLine.startsWith("id")) {
                        haId = idLine.split(':').last().trimmed();
                    } else {
                        qCWarning(dcHomeConnect()) << "Id line: Unexpected line" << eventTypeLine;
                        continue;
                    }
                } else {
                    qCWarning(dcHomeConnect()) << "Data Line: Unexpected line" << eventTypeLine;
                    continue;
                }
            } else {
                qCWarning(dcHomeConnect()) << "Event type: Unexpected line" << eventTypeLine;
                continue;
            }

            if (data.toVariant().toMap().contains("items")) {
                QList<Event> events;
                QVariantList itemsList = data.toVariant().toMap().value("items").toList();
                Q_FOREACH(QVariant item, itemsList) {
                    QVariantMap map = item.toMap();
                    Event event;
                    event.key = map["key"].toString();
                    event.uri = map["uri"].toString();
                    event.name = map["uri"].toString();
                    event.value  = map["value"];
                    event.unit = map["unit"].toString();
                    event.timestamp  = map["timestamp"].toInt();
                    events.append(event);
                }
                if (!events.isEmpty())
                    emit receivedEvents(eventType, haId, events);
            } else if (data.toVariant().toMap().contains("error")) {
                qCWarning(dcHomeConnect()) << "Event stream error" << data.toVariant().toMap().value("error");
            }
        }
    });
}

QUuid HomeConnect::sendCommand(const QString &haid, const QString &command)
{
    QUuid commandId = QUuid::createUuid();
    QUrl url = QUrl(m_baseControlUrl+"/api/homeappliances/"+haid+"/commands/"+command);

    QNetworkRequest request(url);
    request.setRawHeader("Authorization", "Bearer "+m_accessToken);
    request.setRawHeader("Accept-Language", "en-US");
    request.setRawHeader("accept", "application/vnd.bsh.sdk.v1+json");

    QJsonDocument doc;
    QJsonObject data;
    data.insert("key", command);
    data.insert("value", true);
    QJsonObject obj;
    obj.insert("data", data);
    doc.setObject(obj);
    QNetworkReply *reply = m_networkManager->put(request, doc.toJson());
    connect(reply, &QNetworkReply::finished, reply, &QNetworkReply::deleteLater);
    connect(reply, &QNetworkReply::finished, this, [this, commandId, reply]{

        QByteArray rawData = reply->readAll();
        if (!checkStatusCode(reply, rawData)) {
            return;
        }
        QVariantMap map = QJsonDocument::fromJson(rawData).toVariant().toMap();
        qCDebug(dcHomeConnect()) << "Send command" << map;
        if (map.contains("data")) {
            QVariantMap dataMap = map.value("data").toMap();
            qCDebug(dcHomeConnect()) << "key" << dataMap.value("key").toString() << "value" << dataMap.value("value").toString() << dataMap.value("unit").toString();
        } else if (map.contains("error")) {
            qCWarning(dcHomeConnect()) << "Send command" << map.value("error").toMap().value("description").toString();
        }
        emit commandExecuted(commandId, true);
    });
    return commandId;
}

void HomeConnect::setAuthenticated(bool state)
{
    if (state != m_authenticated) {
        m_authenticated = state;
        emit authenticationStatusChanged(state);
    }
}

void HomeConnect::setConnected(bool state)
{
    if (state != m_connected) {
        m_connected = state;
        emit connectionChanged(state);
    }
}
