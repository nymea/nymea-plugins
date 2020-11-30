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

#include "miele.h"
#include "extern-plugininfo.h"

#include <QNetworkReply>
#include <QNetworkRequest>
#include <QUrl>
#include <QJsonObject>
#include <QUrlQuery>


Miele::Miele(NetworkAccessManager *networkmanager, const QByteArray &clientId, QObject *parent) :
    QObject(parent),
    m_clientId(clientId),
    m_networkManager(networkmanager)
{
    m_tokenRefreshTimer = new QTimer(this);
    m_tokenRefreshTimer->setSingleShot(true);
    connect(m_tokenRefreshTimer, &QTimer::timeout, this, &Miele::onRefreshTimeout);
}

QByteArray Miele::accessToken()
{
    return m_accessToken;
}

QByteArray Miele::refreshToken()
{
    return m_refreshToken;
}

QUrl Miele::getLoginUrl(const QUrl &redirectUrl)
{
    if (m_clientId.isEmpty()) {
        qWarning(dcMiele) << "Client key not defined!";
        return QUrl("");
    }

    if (redirectUrl.isEmpty()){
        qWarning(dcMiele()) << "No redirect uri defined!";
    }
    m_redirectUri = QUrl::toPercentEncoding(redirectUrl.toString());

    QUrl url(m_authorizationUrl);
    QUrlQuery queryParams;
    queryParams.addQueryItem("client_id", m_clientId);
    queryParams.addQueryItem("redirect_uri", m_redirectUri);
    queryParams.addQueryItem("response_type", "code");
    m_state = QUuid::createUuid().toString();
    queryParams.addQueryItem("state", m_state);
    url.setQuery(queryParams);

    return url;
}



void Miele::getDevices()
{

}


QUuid Miele::processAction(const QString &deviceId, Miele::ProcessAction action)
{
    QJsonDocument doc;
    QJsonObject object;
    object.insert("processAction", action);
    doc.setObject(object);
    return putAction(deviceId, doc);
}

QUuid Miele::setPower(const QString &deviceId, bool power)
{
    QJsonDocument doc;
    QJsonObject object;
    if (power) {
        object.insert("powerOn", true);
    } else {
        object.insert("powerOff", true);
    }
    doc.setObject(object);
    return putAction(deviceId, doc);
}

QUuid Miele::setDeviceName(const QString &deviceId, const QString &deviceName)
{
    QJsonDocument doc;
    QJsonObject object;
    object.insert("description", deviceName);
    doc.setObject(object);
    return putAction(deviceId, doc);
}

QUuid Miele::setLight(const QString &deviceId, bool power)
{
    QJsonDocument doc;
    QJsonObject object;
    if (power) {
        object.insert("light", 1);
    } else {
        object.insert("light", 2);
    }
    doc.setObject(object);
    return putAction(deviceId, doc);
}

QUuid Miele::setTargetTemperature(const QString &deviceId, int zone, int targetTemperature)
{
    QJsonDocument doc;
    QJsonObject object;
    QJsonObject temperatureObj;
    temperatureObj.insert("zone", zone);
    temperatureObj.insert("value", targetTemperature);
    object.insert("targetTemperature", temperatureObj);
    doc.setObject(object);
    return putAction(deviceId, doc);
}

QUuid Miele::putAction(const QString &deviceId, const QJsonDocument &action)
{
    QUuid commandId = QUuid::createUuid();
    QUrl url = m_apiUrl;
    url.setPath("/v1/devices/"+deviceId+"/actions");
    url.setQuery("language=en");

    QNetworkRequest request(url);
    request.setRawHeader("Authorization", "Bearer "+m_accessToken);
    // request.setRawHeader("Accept-Language", "en-US");
    request.setRawHeader("accept", "application/json; charset=utf-8");
    request.setHeader(QNetworkRequest::KnownHeaders::ContentTypeHeader, "application/json");

    QNetworkReply *reply = m_networkManager->put(request, action.toJson());
    connect(reply, &QNetworkReply::finished, reply, &QNetworkReply::deleteLater);
    connect(reply, &QNetworkReply::finished, this, [this, commandId, reply]{

        QByteArray rawData = reply->readAll();
        if (!checkStatusCode(reply, rawData)) {
            return;
        }
        QVariantMap map = QJsonDocument::fromJson(rawData).toVariant().toMap();
        qCDebug(dcMiele()) << "Send command" << map;
        if (map.contains("data")) {
            QVariantMap dataMap = map.value("data").toMap();
            qCDebug(dcMiele()) << "key" << dataMap.value("key").toString() << "value" << dataMap.value("value").toString() << dataMap.value("unit").toString();
        } else if (map.contains("error")) {
            qCWarning(dcMiele()) << "Send command" << map.value("error").toMap().value("description").toString();
        }
        emit commandExecuted(commandId, true);
    });
    return commandId;
}
