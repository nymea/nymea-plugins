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

Tempo::Tempo(NetworkAccessManager *networkmanager, const QString &token, QObject *parent) :
    QObject(parent),
    m_token(token),
    m_networkManager(networkmanager)
{
    qCDebug(dcTempo()) << "Creating tempo connection";
}

Tempo::~Tempo()
{
    qCDebug(dcTempo()) << "Deleting tempo connection";
}

QString Tempo::token() const
{
    return m_token;
}

void Tempo::getTeams()
{
    QUrl url = QUrl(m_baseControlUrl+"/teams");
    qCDebug(dcTempo()) << "Get teams, url" << url.toString();

    QNetworkRequest request(url);
    request.setRawHeader("Authorization", "Bearer "+m_token.toUtf8());

    QNetworkReply *reply = m_networkManager->get(request);
    connect(reply, &QNetworkReply::finished, reply, &QNetworkReply::deleteLater);
    connect(reply, &QNetworkReply::finished, this, [this, reply]{

        QByteArray rawData = reply->readAll();
        if (!checkStatusCode(reply, rawData)) {
            return;
        }
        QVariantMap dataMap = QJsonDocument::fromJson(rawData).toVariant().toMap();
        QVariantList teamList = dataMap.value("results").toList();
        QList<Team> teams;
        Q_FOREACH(QVariant var, teamList) {
            QVariantMap map = var.toMap();
            Team team;
            team.self = map["self"].toString();
            team.id = map["id"].toInt();
            team.name = map["name"].toString();
            team.summary = map["summery"].toString();

            QVariantMap lead = map["lead"].toMap();
            team.lead.self = lead["self"].toString();
            team.lead.accountId = lead["accountId"].toString();
            team.lead.displayName = lead["displayName"].toString();

            teams.append(team);
        }
        if (!teams.isEmpty()) {
            emit teamsReceived(teams);
        }
    });
}

void Tempo::getAccounts()
{
    QUrl url = QUrl(m_baseControlUrl+"/accounts");
    qCDebug(dcTempo()) << "Get accounts. Url" << url.toString();

    QNetworkRequest request(url);
    request.setRawHeader("Authorization", "Bearer "+m_token.toUtf8());

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

            QVariantMap customer = map["customer"].toMap();
            account.customer.self = customer["self"].toString();
            account.customer.key = customer["key"].toString();
            account.customer.id = customer["id"].toInt();
            account.customer.name = customer["name"].toString();

            QVariantMap category = map["category"].toMap();
            account.category.self = category["self"].toString();
            account.category.key = category["key"].toString();
            account.category.name = category["name"].toString();
            account.category.id = category["id"].toInt();

            QVariantMap contact = map["contact"].toMap();
            account.contact.self = contact["self"].toString();
            account.contact.type = contact["type"].toString();
            account.contact.accountId = contact["accountId"].toString();
            account.contact.displayName = contact["displayName"].toString();

            accounts.append(account);
        }
        if (!accounts.isEmpty()) {
            emit accountsReceived(accounts);
        }
    });
}

void Tempo::getWorkloadByAccount(const QString &accountKey, QDate from, QDate to, int offset, int limit)
{
    QUrl url = QUrl(m_baseControlUrl+"/worklogs/account/"+accountKey);
    QUrlQuery query;
    query.addQueryItem("from", from.toString(Qt::DateFormat::ISODate));
    query.addQueryItem("to", to.toString(Qt::DateFormat::ISODate));
    query.addQueryItem("offset", QString::number(offset));
    query.addQueryItem("limit", QString::number(limit));
    url.setQuery(query);

    qCDebug(dcTempo()) << "Get workload by account. Url" << url.toString();

    QNetworkRequest request(url);
    request.setRawHeader("Authorization", "Bearer "+m_token.toUtf8());

    QNetworkReply *reply = m_networkManager->get(request);
    connect(reply, &QNetworkReply::finished, reply, &QNetworkReply::deleteLater);
    connect(reply, &QNetworkReply::finished, this, [this, accountKey, reply]{

        QByteArray rawData = reply->readAll();
        if (!checkStatusCode(reply, rawData)) {
            return;
        }
        QVariantMap dataMap = QJsonDocument::fromJson(rawData).toVariant().toMap();
        int offset = dataMap.value("metadata").toMap().value("offset").toInt();
        int limit = dataMap.value("metadata").toMap().value("limit").toInt();
        QList<Worklog> worklogs = parseJsonForWorklog(dataMap);
        if (!worklogs.isEmpty())
            emit accountWorklogsReceived(accountKey, worklogs, limit, offset);
    });
}

void Tempo::getWorkloadByTeam(int teamId, QDate from, QDate to, int offset, int limit)
{
    QUrl url = QUrl(m_baseControlUrl+"/worklogs/team/"+QString::number(teamId));
    QUrlQuery query;
    query.addQueryItem("from", from.toString(Qt::DateFormat::ISODate));
    query.addQueryItem("to", to.toString(Qt::DateFormat::ISODate));
    query.addQueryItem("offset", QString::number(offset));
    query.addQueryItem("limit", QString::number(limit));
    url.setQuery(query);

    qCDebug(dcTempo()) << "Get workload by account. Url" << url.toString();

    QNetworkRequest request(url);
    request.setRawHeader("Authorization", "Bearer "+m_token.toUtf8());

    QNetworkReply *reply = m_networkManager->get(request);
    connect(reply, &QNetworkReply::finished, reply, &QNetworkReply::deleteLater);
    connect(reply, &QNetworkReply::finished, this, [this, teamId, reply]{
        QByteArray rawData = reply->readAll();
        if (!checkStatusCode(reply, rawData)) {
            return;
        }
        QVariantMap dataMap = QJsonDocument::fromJson(rawData).toVariant().toMap();
        int offset = dataMap.value("metadata").toMap().value("offset").toInt();
        int limit = dataMap.value("metadata").toMap().value("limit").toInt();
        QList<Worklog> worklogs = parseJsonForWorklog(dataMap);
        if (!worklogs.isEmpty())
            emit teamWorklogsReceived(teamId, worklogs, limit, offset);
    });
}

void Tempo::setAuthenticated(bool state)
{
    // We'll want to emit this in any case, even if it going from false to false, so we can properly handle the expired token case
    m_authenticated = state;
    emit authenticationStatusChanged(state);
}

void Tempo::setConnected(bool state)
{
    if (state != m_connected) {
        m_connected = state;
        emit connectionChanged(state);
    }
}

QList<Tempo::Worklog> Tempo::parseJsonForWorklog(const QVariantMap &data)
{
    QVariantList worklogList = data.value("results").toList();
    qCDebug(dcTempo()) << "Worklog received";
    qCDebug(dcTempo()) << "     - Count:" << data.value("metadata").toMap().value("count");
    qCDebug(dcTempo()) << "     - Offset:" << data.value("metadata").toMap().value("offset");
    qCDebug(dcTempo()) << "     - Limit:" << data.value("metadata").toMap().value("limit");

    QList<Worklog> worklogs;
    Q_FOREACH(QVariant var, worklogList) {
        QVariantMap map = var.toMap();
        Worklog worklog;
        worklog.self = map["self"].toString();
        worklog.tempoWorklogId = map["tempoWorklogId"].toInt();
        worklog.jiraWorklogId = map["jiraWorklogId"].toInt();
        worklog.issue = map["issue"].toMap().value("key").toString();
        worklog.timeSpentSeconds = map["timeSpentSeconds"].toInt();
        worklog.startDate = QDate::fromString(map["startDate"].toString(), Qt::ISODate);
        worklog.startTime = QTime::fromString(map["startTime"].toString(), Qt::ISODate);
        worklog.description = map["description"].toString();
        worklog.createdAt =  QDateTime::fromString(map["createdAt"].toString(), Qt::ISODate);
        worklog.updatedAt =  QDateTime::fromString(map["updatedAt"].toString(), Qt::ISODate);
        worklog.authorAccountId = map["author"].toMap().value("accountId").toString();
        worklog.authorDisplayName = map["author"].toMap().value("displayName").toString();
        worklogs.append(worklog);
    }
    return worklogs;
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
