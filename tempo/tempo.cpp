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

QUrl Tempo::getLoginUrl(const QUrl &redirectUrl, const QString &jiraCloudInstanceName, const QByteArray &clientId)
{
    QUrl url;
    url.setScheme("https");
    url.setHost(jiraCloudInstanceName+"atlassian.net");
    url.setPath("/plugins/servlet/ac/io.tempo.jira/oauth-authorize/");
    QUrlQuery query;
    query.addQueryItem("client_id", clientId);
    query.addQueryItem("redirect_uri", redirectUrl.toString());
    query.addQueryItem("access_type", "tenant_user");
    url.setQuery(query);
    return url;
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

}

void Tempo::onRefreshTimer()
{

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
