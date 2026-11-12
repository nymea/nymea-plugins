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

#include "senecconnect.h"
#include "extern-plugininfo.h"

#include <QUrlQuery>
#include <QJsonDocument>
#include <QJsonParseError>

SenecConnect::SenecConnect(NetworkAccessManager *networkManager, const QString &subscriptionKey, QObject *parent)
    : QObject{parent},
    m_networkManager{networkManager},
    m_subscriptionKey{subscriptionKey}
{

}

QUrl SenecConnect::deviceDataUrl()
{
    QUrl url("https://apim-eds-gwc-prod.azure-api.net/senec-connect/v1/systems/device-data/general");
    QUrlQuery query;
    query.addQueryItem("include", "battery,meter,bessNameplate");
    url.setQuery(query);
    return url;
}

QList<SenecConnect::SystemData> SenecConnect::parseDeviceData(const QByteArray &data, bool *ok)
{
    QList<SystemData> systems;

    QJsonParseError jsonError;
    QJsonDocument jsonDoc = QJsonDocument::fromJson(data, &jsonError);
    if (jsonError.error != QJsonParseError::NoError || !jsonDoc.isArray()) {
        if (ok)
            *ok = false;

        return systems;
    }

    foreach (const QVariant &systemVariant, jsonDoc.toVariant().toList()) {
        QVariantMap systemMap = systemVariant.toMap();
        QVariantMap batteryMap = systemMap.value("battery").toMap();
        QVariantMap meterMap = systemMap.value("meter").toMap();
        QVariantMap nameplateMap = systemMap.value("bessNameplate").toMap();

        SystemData system;
        system.systemId = nameplateMap.value("system_id").toString();
        if (system.systemId.isEmpty())
            continue;

        system.serialNumber = nameplateMap.value("serial_number").toString();
        system.model = nameplateMap.value("model").toString();
        system.capacity = nameplateMap.value("design_capacity").toFloat() / 1000.0;
        system.batteryPower = batteryMap.value("power").toFloat();
        system.batteryLevel = batteryMap.value("state_of_charge").toFloat();
        system.gridPower = meterMap.value("grid_power").toFloat();
        system.production = meterMap.value("production").toFloat();
        // Stale systems are still listed, but without live battery measurements
        system.active = batteryMap.contains("voltage") || batteryMap.contains("current");
        systems.append(system);
    }

    if (ok)
        *ok = true;

    return systems;
}

QNetworkReply *SenecConnect::getDeviceData()
{
    QNetworkRequest request(deviceDataUrl());
    request.setRawHeader("Ocp-Apim-Subscription-Key", m_subscriptionKey.toUtf8());
    request.setRawHeader("Accept", "application/json");
    qCDebug(dcSenec()) << "Get" << request.url().toString();
    return m_networkManager->get(request);
}
