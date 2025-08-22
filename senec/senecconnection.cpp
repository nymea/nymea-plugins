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

#include "senecconnection.h"
#include "extern-plugininfo.h"

#include <QUrlQuery>
#include <QJsonDocument>
#include <QJsonParseError>
#include <QRegularExpression>

SenecConnection::SenecConnection(NetworkAccessManager *networkManager, QObject *parent)
    : QObject{parent},
    m_networkManager{networkManager}
{
    m_configUrl = QUrl("https://sso.senec.com/realms/senec/.well-known/openid-configuration");
    m_systemsUrl = QUrl("https://senec-app-systems-proxy.prod.senec.dev");
    m_measurementsUrl = QUrl("https://senec-app-measurements-proxy.prod.senec.dev");

    m_clientId = QString("endcustomer-app-frontend");
    m_redirectUrl =/* QString("http://127.0.0.1:8888");*/ QUrl("senec-app-auth://keycloak.prod");
    m_scopes << "roles" << "meinsenec" << "openid";
    m_userAgent = QString("nymea (+https://github.com/nymea/nymea)");
}

void SenecConnection::initialize()
{
    qCDebug(dcSenec()) << "OpenID: Initialize and get configuration";

    QNetworkRequest request(m_configUrl);
    request.setHeader(QNetworkRequest::UserAgentHeader, m_userAgent.toUtf8());

    QNetworkReply *reply = m_networkManager->get(request);
    connect(reply, &QNetworkReply::finished, reply, &QNetworkReply::deleteLater);
    connect(reply, &QNetworkReply::finished, this, [this, reply, request](){

        QByteArray data = reply->readAll();
        if (reply->error() != QNetworkReply::NoError) {
            qCWarning(dcSenec()) << "OpenID: Failed to get configuration" << request.url().toString() << reply->errorString();
            if (!data.isEmpty())
                qCWarning(dcSenec()) << "OpenID: Response data:" << qUtf8Printable(data);

            emit initFinished(false);
            return;
        }

        //qCDebug(dcPushNotifications()) << "OAuth2: Request access token response" << qUtf8Printable(data);

        QJsonParseError error;
        QJsonDocument jsonDoc = QJsonDocument::fromJson(data, &error);
        if (error.error != QJsonParseError::NoError) {
            qCWarning(dcSenec()) << "OpenID: Failed to get configuration. Response data JSON error:" << error.errorString();
            emit initFinished(false);
            return;
        }

        QVariantMap configMap = jsonDoc.toVariant().toMap();
        m_authEndpoint = QUrl(configMap.value("authorization_endpoint").toString());

        QUrlQuery queryParams;
        queryParams.addQueryItem("client_id", m_clientId);
        queryParams.addQueryItem("redirect_uri", m_redirectUrl.toString());
        queryParams.addQueryItem("response_type", "code");
        queryParams.addQueryItem("scope", m_scopes.join("+"));

        // queryParams.addQueryItem("nonce", QUuid::createUuid().toString());
        // m_codeChallenge = QUuid::createUuid().toString().remove(QRegularExpression("[{}-]"));

        // queryParams.addQueryItem("code_challenge", m_codeChallenge);
        // queryParams.addQueryItem("code_challenge_method", "S256");
        m_authEndpoint.setQuery(queryParams);
        emit initFinished(true);
    });
}

QUrl SenecConnection::authEndpoint() const
{
    return m_authEndpoint;
}

// void SenecConnection::login(const QString &username, const QString &password)
// {
//     Q_UNUSED(username, password)
//     QNetworkRequest request(m_configUrl);
//     request.setHeader(QNetworkRequest::UserAgentHeader, m_userAgent.toUtf8());

//     QNetworkReply *reply = m_networkManager->get(request);
//     connect(reply, &QNetworkReply::finished, reply, &QNetworkReply::deleteLater);
//     connect(reply, &QNetworkReply::finished, this, [this, reply, request](){

//         QByteArray data = reply->readAll();
//         if (reply->error() != QNetworkReply::NoError) {
//             qCWarning(dcSenec()) << "OpenID: Failed to get configuration" << request.url().toString() << reply->errorString();
//             if (!data.isEmpty())
//                 qCWarning(dcSenec()) << "OpenID: Response data:" << qUtf8Printable(data);

//             // setAuthenticated(false);

//             // // Retry in 30 seconds, maybe we are not online yet
//             // m_refreshTimer.start(60 * 1000);
//             return;
//         }

//         //qCDebug(dcPushNotifications()) << "OAuth2: Request access token response" << qUtf8Printable(data);

//         QJsonParseError error;
//         QJsonDocument jsonDoc = QJsonDocument::fromJson(data, &error);
//         if (error.error != QJsonParseError::NoError) {
//             qCWarning(dcSenec()) << "OpenID: Failed to get configuration. Response data JSON error:" << error.errorString();
//             //setAuthenticated(false);
//             return;
//         }

//         QVariantMap configMap = jsonDoc.toVariant().toMap();
//         m_authEndpoint = QUrl(configMap.value("authorization_endpoint").toString());



//         // QUrl url(m_baseTokenUrl);
//         // QUrlQuery query;
//         // query.clear();
//         // query.addQueryItem("grant_type", "refresh_token");
//         // query.addQueryItem("refresh_token", refreshToken);
//         // query.addQueryItem("client_secret", m_clientSecret);

//         // QNetworkRequest request(m_authEndpoint);
//         // request.setHeader(QNetworkRequest::UserAgentHeader, m_userAgent.toUtf8());
//         // request.setHeader(QNetworkRequest::ContentTypeHeader, "application/x-www-form-urlencoded");


//         // // QUrlQuery queryParams;
//         // // queryParams.addQueryItem("client_id", m_clientId);
//         // // queryParams.addQueryItem("redirect_uri", m_redirectUrl);
//         // // queryParams.addQueryItem("response_type", "code");
//         // // queryParams.addQueryItem("scope", m_scopes.join(" "));
//         // // queryParams.addQueryItem("nonce", QUuid::createUuid().toString());
//         // // m_codeChallenge = QUuid::createUuid().toString().remove(QRegularExpression("[{}-]"));

//         // // queryParams.addQueryItem("code_challenge", m_codeChallenge);
//         // // queryParams.addQueryItem("code_challenge_method", "S256");
//         // // url.setQuery(queryParams);


//         // // QNetworkReply *reply = m_networkManager->post(request, query.toString().toUtf8());
//         // // connect(reply, &QNetworkReply::finished, reply, &QNetworkReply::deleteLater);
//         // // connect(reply, &QNetworkReply::finished, this, [this, reply](){

//         // redirect_uri: REDIRECT_URI,
//         //         scope: SCOPE,
//         //                 code_challenge:,
//         //                                 code_challenge_method: 'S256',


//         });

//     });
// }
