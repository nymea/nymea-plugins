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

#include "huebridge.h"
#include <QJsonDocument>

HueBridge::HueBridge(QObject *parent) :
    QObject(parent),
    m_zigbeeChannel(-1)
{

}

QString HueBridge::name() const
{
    return m_name;
}

void HueBridge::setName(const QString &name)
{
    m_name = name;
}

QString HueBridge::id() const
{
    return m_id;
}

void HueBridge::setId(const QString &id)
{
    m_id = id;
}

QString HueBridge::apiKey() const
{
    return m_apiKey;
}

void HueBridge::setApiKey(const QString &apiKey)
{
    m_apiKey = apiKey;
}

QHostAddress HueBridge::hostAddress() const
{
    return m_hostAddress;
}

void HueBridge::setHostAddress(const QHostAddress &hostAddress)
{
    m_hostAddress = hostAddress;
}

QString HueBridge::macAddress() const
{
    return m_macAddress;
}

void HueBridge::setMacAddress(const QString &macAddress)
{
    m_macAddress = macAddress;
}

QString HueBridge::apiVersion() const
{
    return m_apiVersion;
}

void HueBridge::setApiVersion(const QString &apiVersion)
{
    m_apiVersion = apiVersion;
}

QString HueBridge::softwareVersion() const
{
    return m_softwareVersion;
}

void HueBridge::setSoftwareVersion(const QString &softwareVersion)
{
    m_softwareVersion = softwareVersion;
}

int HueBridge::zigbeeChannel() const
{
    return m_zigbeeChannel;
}

void HueBridge::setZigbeeChannel(const int &zigbeeChannel)
{
    m_zigbeeChannel = zigbeeChannel;
}

QPair<QNetworkRequest, QByteArray> HueBridge::createDiscoverLightsRequest()
{
    QNetworkRequest request(QUrl("http://" + hostAddress().toString() + "/api/" + apiKey() + "/lights/"));
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    return QPair<QNetworkRequest, QByteArray>(request, QByteArray());
}

QPair<QNetworkRequest, QByteArray> HueBridge::createSearchLightsRequest(const QString &deviceId)
{
    QNetworkRequest request(QUrl("http://" + hostAddress().toString() + "/api/" + apiKey() + "/lights/"));
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    QByteArray payload;
    if (!deviceId.isEmpty()) {
        QVariantMap params;
        QVariantList deviceIds;
        deviceIds.append(deviceId);
        params.insert("deviceId", deviceIds);
        payload = QJsonDocument::fromVariant(params).toJson(QJsonDocument::Compact);
    }
    return QPair<QNetworkRequest, QByteArray>(request, payload);
}

QPair<QNetworkRequest, QByteArray> HueBridge::createSearchSensorsRequest()
{
    QNetworkRequest request(QUrl("http://" + hostAddress().toString() + "/api/" + apiKey() + "/sensors/"));
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    return QPair<QNetworkRequest, QByteArray>(request, QByteArray());
}

QPair<QNetworkRequest, QByteArray> HueBridge::createCheckUpdatesRequest()
{
    QVariantMap updateMap;
    updateMap.insert("checkforupdate", true);

    QVariantMap requestMap;

    // TODO: check if portalservice is true, cannot be done in one step
    //requestMap.insert("portalservices", true);

    requestMap.insert("swupdate", updateMap);

    QJsonDocument jsonDoc = QJsonDocument::fromVariant(requestMap);

    QNetworkRequest request(QUrl("http://" + hostAddress().toString() + "/api/" + apiKey() + "/config"));
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

    return QPair<QNetworkRequest, QByteArray>(request, jsonDoc.toJson());
}

QPair<QNetworkRequest, QByteArray> HueBridge::createUpgradeRequest()
{
    QVariantMap updateMap;
    updateMap.insert("updatestate", 3);

    QVariantMap requestMap;
    requestMap.insert("swupdate", updateMap);

    QJsonDocument jsonDoc = QJsonDocument::fromVariant(requestMap);

    QNetworkRequest request(QUrl("http://" + hostAddress().toString() + "/api/" + apiKey() + "/config"));
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

    return QPair<QNetworkRequest, QByteArray>(request, jsonDoc.toJson());
}
