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

#include <QJsonDocument>
#include <QUrlQuery>

#include "tempo.h"
#include "extern-plugininfo.h"

Tempo::Tempo(NetworkAccessManager *networkmanager, const QByteArray &clientId, const QByteArray &clientSecret, QObject *parent) :
    QObject(parent),
    m_clientId(clientId),
    m_clientSecret(clientSecret),
    m_networkManager(networkmanager)

{
    m_tokenRefreshTimer = new QTimer(this);
    m_tokenRefreshTimer->setSingleShot(true);
    connect(m_tokenRefreshTimer, &QTimer::timeout, this, &Tempo::onRefreshTimer);
}

QByteArray Tempo::accessToken()
{
    return m_accessToken;
}

QByteArray Tempo::refreshToken()
{
    return m_refreshToken;
}

QUrl Tempo::getLoginUrl(const QUrl &redirectUrl, const QString &jiraCloudInstanceName)
{
    if (m_clientId.isEmpty()) {
        qWarning(dcTempo) << "Client Id not defined!";
        return QUrl("");
    }

    if (redirectUrl.isEmpty()){
        qWarning(dcTempo) << "No redirect uri defined!";
    }
    m_redirectUri = QUrl::toPercentEncoding(redirectUrl.toString());

    QUrl url;
    url.setScheme("https");
    url.setHost(jiraCloudInstanceName+".atlassian.net");
    url.setPath("/plugins/servlet/ac/io.tempo.jira/oauth-authorize/");
    QUrlQuery query;
    query.addQueryItem("client_id", m_clientId);
    query.addQueryItem("redirect_uri", m_redirectUri);
    query.addQueryItem("access_type", "tenant_user");
    url.setQuery(query);
    return url;
}

void Tempo::getAccessTokenFromRefreshToken(const QByteArray &refreshToken)
{
    if (refreshToken.isEmpty()) {
        qWarning(dcTempo) << "No refresh token given!";
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
            qCDebug(dcTempo) << "Access token expires int" << expireTime << "s, at" << QDateTime::currentDateTime().addSecs(expireTime).toString();
            if (!m_tokenRefreshTimer) {
                qWarning(dcTempo()) << "Access token refresh timer not initialized";
                return;
            }
            if (expireTime < 20) {
                qCWarning(dcTempo()) << "Expire time too short";
                return;
            }
            m_tokenRefreshTimer->start((expireTime - 20) * 1000);
        }
    });
}

void Tempo::getAccessTokenFromAuthorizationCode(const QByteArray &authorizationCode)
{
    // Obtaining access token
    if(authorizationCode.isEmpty())
        qWarning(dcTempo()) << "No authorization code given!";
    if(m_clientId.isEmpty())
        qWarning(dcTempo()) << "Client key not set!";
    if(m_clientSecret.isEmpty())
        qWarning(dcTempo()) << "Client secret not set!";

    QUrl url = QUrl(m_baseTokenUrl);
    QUrlQuery query;    url.setQuery(query);

    query.clear();
    query.addQueryItem("client_id", m_clientId);
    query.addQueryItem("client_secret", m_clientSecret);
    query.addQueryItem("redirect_uri", m_redirectUri);
    query.addQueryItem("grant_type", "authorization_code");
    query.addQueryItem("code", authorizationCode);
    // query.addQueryItem("code_verifier", m_codeChallenge);

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
            qCDebug(dcTempo()) << "Token expires in" << expireTime << "s, at" << QDateTime::currentDateTime().addSecs(expireTime).toString();
            if (!m_tokenRefreshTimer) {
                qWarning(dcTempo()) << "Token refresh timer not initialized";
                setAuthenticated(false);
                return;
            }
            if (expireTime < 20) {
                qCWarning(dcTempo()) << "Expire time too short";
                return;
            }
            m_tokenRefreshTimer->start((expireTime - 20) * 1000);
        }
    });
}

void Tempo::getAccounts()
{
    QUrl url = QUrl(m_baseControlUrl+"/accounts");

    QNetworkRequest request(url);
    request.setRawHeader("Authorization", "Bearer "+m_accessToken);

    QNetworkReply *reply = m_networkManager->get(request);
    connect(reply, &QNetworkReply::finished, reply, &QNetworkReply::deleteLater);
    connect(reply, &QNetworkReply::finished, this, [this, reply]{

        QByteArray rawData = reply->readAll();
        if (!checkStatusCode(reply, rawData)) {
            return;
        }
        QVariantMap dataMap = QJsonDocument::fromJson(rawData).toVariant().toMap();
        QVariantList accountList = dataMap.value("results").toList();
        QList<Account> accounts;
        Q_FOREACH(QVariant var, accountList) {
            QVariantMap map = var.toMap();
            Account account;
            account.self = map["self"].toString();
            account.key = map["key"].toString();
            account.id = map["id"].toInt();
            account.name = map["name"].toString();

            if (map["status"] == "OPEN") {
                account.status = Status::Open;
            } else if (map["status"] == "CLOSED") {
                account.status = Status::Closed;
            } else if (map["status"] == "ARCHIVED") {
                account.status = Status::Archived;
            }
            account.global = map["global"].toBool();
            account.monthlyBudget = map["monthlyBudget"].toInt();

            QVariantMap lead = map["lead"].toMap();
            account.lead.self = lead["self"].toString();
            account.lead.accountId = lead["accountId"].toString();
            account.lead.displayName = lead["displayName"].toString();
            //TODO Customer
            QVariantMap customer = map["customer"].toMap();
            account.customer.self = lead["self"].toString();
            //TODO Category
            QVariantMap category = map["category"].toMap();
            account.category.self = lead["self"].toString();
            //TODO Contact
            QVariantMap contact = map["contact"].toMap();
            account.contact.self = lead["self"].toString();

            accounts.append(account);
        }
        if (!accounts.isEmpty()) {

        }
    });
}

void Tempo::getWorkloadByAccount(const QString &accountKey, QDate from, QDate to)
{
    QUrl url = QUrl(m_baseControlUrl+"/worklogs/account/"+accountKey);
    QUrlQuery query;
    query.addQueryItem("from", from.toString(Qt::DateFormat::ISODate));
    query.addQueryItem("to", to.toString(Qt::DateFormat::ISODate));
    url.setQuery(query);

    QNetworkRequest request(url);
    request.setRawHeader("Authorization", "Bearer "+m_accessToken);

    QNetworkReply *reply = m_networkManager->get(request);
    connect(reply, &QNetworkReply::finished, reply, &QNetworkReply::deleteLater);
    connect(reply, &QNetworkReply::finished, this, [this, accountKey, reply]{

        QByteArray rawData = reply->readAll();
        if (!checkStatusCode(reply, rawData)) {
            return;
        }
        QVariantMap dataMap = QJsonDocument::fromJson(rawData).toVariant().toMap();
        QVariantList worklogList = dataMap.value("results").toList();
        QList<Worklog> worklogs;
        Q_FOREACH(QVariant var, worklogList) {
            QVariantMap map = var.toMap();
            Worklog worklog;
            worklog.self = map["self"].toString();
            worklog.tempoWorklogId = map["tempoWorklogId"].toInt();
            worklog.jiraWorklogId = map["jiraWorklogId"].toInt();
            worklog.issue = map["issue"].toMap().value("key").toString();
            worklog.timeSpentSeconds = map["timeSpentSeconds"].toInt();
            //TODO startDate: required (date-only)
            //TODO startTime: required (time-only)
            worklog.description = map["description"].toString();
            //TODO createdAt: required (datetime)
            //TODO updatedAt: required (datetime)
            worklog.authorAccountId = map["author"].toMap().value("accountId").toString();
            worklog.authorDisplayName = map["author"].toMap().value("displayName").toString();
            worklogs.append(worklog);
        }
        if (!worklogs.isEmpty())
            emit accountWorklogsReceived(accountKey, worklogs);
    });
}

void Tempo::onRefreshTimer()
{
    qCDebug(dcTempo()) << "Refresh authentication token";
    getAccessTokenFromRefreshToken(m_refreshToken);
}

void Tempo::setAuthenticated(bool state)
{
    if (state != m_authenticated) {
        m_authenticated = state;
        emit authenticationStatusChanged(state);
    }
}

void Tempo::setConnected(bool state)
{
    if (state != m_connected) {
        m_connected = state;
        emit connectionChanged(state);
    }
}

bool Tempo::checkStatusCode(QNetworkReply *reply, const QByteArray &rawData)
{
    // Check for the internet connection
    if (reply->error() == QNetworkReply::NetworkError::HostNotFoundError ||
            reply->error() == QNetworkReply::NetworkError::UnknownNetworkError ||
            reply->error() == QNetworkReply::NetworkError::TemporaryNetworkFailureError) {
        qCWarning(dcTempo()) << "Connection error" << reply->errorString();
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
                qWarning(dcTempo()) << "Client token provided doesn’t correspond to client that generated auth code.";
            }
            if(jsonDoc.toVariant().toMap().value("error").toString() == "invalid_redirect_uri") {
                qWarning(dcTempo()) << "Missing redirect_uri parameter.";
            }
            if(jsonDoc.toVariant().toMap().value("error").toString() == "invalid_code") {
                qWarning(dcTempo()) << "Expired authorization code.";
            }
        }
        setAuthenticated(false);
        return false;
    case 401:
        qWarning(dcTempo()) << "Client does not have permission to use this API.";
        setAuthenticated(false);
        return false;
    case 403:
        qCWarning(dcTempo()) << "Forbidden, Scope has not been granted or home appliance is not assigned to HC account";
        setAuthenticated(false);
        return false;
    case 404:
        qCWarning(dcTempo()) << "Not Found. This resource is not available (e.g. no images on washing machine)";
        return false;
    case 405:
        qWarning(dcTempo()) << "Wrong HTTP method used.";
        setAuthenticated(false);
        return false;
    case 408:
        qCWarning(dcTempo())<< "Request Timeout, API Server failed to produce an answer or has no connection to backend service";
        return false;
    case 409:
        qCWarning(dcTempo()) << "Conflict - Command/Query cannot be executed for the home appliance, the error response contains the error details";
        qCWarning(dcTempo()) << "Error" << jsonDoc;
        return false;
    case 415:
        qCWarning(dcTempo())<< "Unsupported Media Type. The request's Content-Type is not supported";
        return false;
    case 429:
        qCWarning(dcTempo())<< "Too Many Requests, the number of requests for a specific endpoint exceeded the quota of the client";
        return false;
    case 500:
        qCWarning(dcTempo())<< "Internal Server Error, in case of a server configuration error or any errors in resource files";
        return false;
    case 503:
        qCWarning(dcTempo())<< "Service Unavailable,if a required backend service is not available";
        return false;
    default:
        break;
    }

    if (error.error != QJsonParseError::NoError) {
        qCWarning(dcTempo()) << "Received invalide JSON object" << rawData;
        qCWarning(dcTempo()) << "Status" << status;
        setAuthenticated(false);
        return false;
    }

    setAuthenticated(true);
    return true;
}
