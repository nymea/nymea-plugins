/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*
* Copyright 2013 - 2025, nymea GmbH
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

#include "senecaccount.h"
#include "extern-plugininfo.h"

SenecAccount::SenecAccount(NetworkAccessManager *networkManager, const QString &username, const QString &token, const QString &refreshToken, QObject *parent)
    : QObject{parent},
    m_networkManager{networkManager},
    m_username{username},
    m_token{token},
    m_refreshToken{refreshToken}
{}

QUrl SenecAccount::baseUrl()
{
    return QUrl("https://app-gateway.prod.senec.dev");
}

QUrl SenecAccount::loginUrl()
{
    QUrl url = SenecAccount::baseUrl();
    url.setPath("/v1/senec/login");
    return url;
}

QUrl SenecAccount::systemsUrl()
{
    QUrl url = SenecAccount::baseUrl();
    url.setPath("/v1/senec/systems");
    return url;
}

QNetworkReply *SenecAccount::getSystems()
{
    QNetworkRequest request(SenecAccount::systemsUrl());
    request.setRawHeader("authorization", m_token.toUtf8());
    return m_networkManager->get(request);
}

QNetworkReply *SenecAccount::getDashboard(const QString &id)
{
    QUrl url = SenecAccount::baseUrl();
    url.setPath("/v2/senec/systems/" + id +"/dashboard");

    QNetworkRequest request(url);
    request.setRawHeader("authorization", m_token.toUtf8());
    qCDebug(dcSenec()) << "Get" << url.toString();
    return m_networkManager->get(request);
}

QNetworkReply *SenecAccount::getAbilities(const QString &id)
{
    QUrl url = SenecAccount::baseUrl();
    url.setPath("/v1/senec/systems/" + id +"/abilities");

    QNetworkRequest request(url);
    request.setRawHeader("authorization", m_token.toUtf8());
    qCDebug(dcSenec()) << "Get" << url.toString();
    return m_networkManager->get(request);
}

QNetworkReply *SenecAccount::getTechnicalData(const QString &id)
{
    QUrl url = SenecAccount::baseUrl();
    url.setPath("/v1/senec/systems/" + id +"/technical-data");

    QNetworkRequest request(url);
    request.setRawHeader("authorization", m_token.toUtf8());
    qCDebug(dcSenec()) << "Get" << url.toString();
    return m_networkManager->get(request);
}

