/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *                                                                         *
 *  Copyright (C) 2019 Bernhard Trinnes <bernhard.trinnes@nymea.io>        *
 *                                                                         *
 *  This file is part of nymea.                                            *
 *                                                                         *
 *  This library is free software; you can redistribute it and/or          *
 *  modify it under the terms of the GNU Lesser General Public             *
 *  License as published by the Free Software Foundation; either           *
 *  version 2.1 of the License, or (at your option) any later version.     *
 *                                                                         *
 *  This library is distributed in the hope that it will be useful,        *
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of         *
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU      *
 *  Lesser General Public License for more details.                        *
 *                                                                         *
 *  You should have received a copy of the GNU Lesser General Public       *
 *  License along with this library; If not, see                           *
 *  <http://www.gnu.org/licenses/>.                                        *
 *                                                                         *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

#include "sonos.h"
#include "extern-plugininfo.h"
#include "network/networkaccessmanager.h"

#include <QJsonDocument>

Sonos::Sonos(QByteArray apiKey, QObject *parent) :
    QObject(parent),
    m_apiKey(apiKey)
{
}

void Sonos::authenticate(const QString &username, const QString &password)
{
    Q_UNUSED(username)
    Q_UNUSED(password)

    m_OAuth = new OAuth(m_apiKey, this);
    m_OAuth->setUrl(QUrl(m_baseAuthorizationUrl));
    m_OAuth->setScope("playback-control-all");
    m_OAuth->startAuthentication();
}

void Sonos::getHouseholds()
{
    QNetworkRequest request;
    request.setHeader(QNetworkRequest::KnownHeaders::ContentTypeHeader, "application/json");
    request.setRawHeader("Authorization", "Bearer" + m_OAuth->bearerToken());
    request.setUrl(QUrl(m_baseControlUrl + "/households"));
    QNetworkReply *reply = QNetworkAccessManager.get(request);
    connect(reply, &QNetworkReply::finished, this [this] {
        int status = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();

        // Check HTTP status code
        if (status != 200 || reply->error() != QNetworkReply::NoError) {
            qCWarning(dcSonos()) << "Request error:" << status << reply->errorString();
            return;
        }
        QJsonDocument data = reply->readAll();
        if (!data.isObject())
            return;


        QList<HouseholdObject> households;
        emit householdObjectsReceived(households);
    });
}

