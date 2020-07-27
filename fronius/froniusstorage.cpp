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

#include "froniusstorage.h"
#include <QJsonDocument>
#include "extern-plugininfo.h"
#include <QDateTime>

FroniusStorage::FroniusStorage(Thing* thing, QObject *parent) : FroniusThing(thing, parent)
{

}

QString FroniusStorage::charging_state() const
{
    return m_charging_state;
}

void FroniusStorage::setChargingState(const QString &charging_state)
{
    m_charging_state = charging_state;
}

QUrl FroniusStorage::updateUrl()
{
    QUrl requestUrl;
    requestUrl.setScheme("http");
    QUrlQuery query;
    requestUrl.setHost(hostAddress());
    requestUrl.setPath(baseUrl() + "GetStorageRealtimeData.cgi");
    query.addQueryItem("Scope", "Device");
    query.addQueryItem("DeviceId", deviceId());
    requestUrl.setQuery(query);

    return requestUrl;
}

void FroniusStorage::updateThingInfo(const QByteArray &data)
{
    // Convert the rawdata to a JSON document
    QJsonParseError error;
    QJsonDocument jsonDoc = QJsonDocument::fromJson(data, &error);

    if (error.error != QJsonParseError::NoError) {
        qCWarning(dcFronius()) << "FroniusStorage: Failed to parse JSON data" << data << ":" << error.errorString();
        pluginThing()->setStateValue(storageConnectedStateTypeId,false);
        return;
    }

    // Parse the data and update the states of our thing
    QVariantMap dataMap = jsonDoc.toVariant().toMap().value("Body").toMap().value("Data").toMap();
   // QVariantMap headMap = jsonDoc.toVariant().toMap().value("Head").toMap();

    // create StorageInfo list map
    QVariantMap storageInfoMap = dataMap.value("Controller").toMap();

    // copy retrieved information to thing states
    if (storageInfoMap.contains("StateOfCharge_Relative")) {
        pluginThing()->setStateValue(storageBatteryLevelStateTypeId, storageInfoMap.value("StateOfCharge_Relative").toInt());
        pluginThing()->setStateValue(storageBatteryCriticalStateTypeId, storageInfoMap.value("StateOfCharge_Relative").toInt() < 5);
    }

    if (storageInfoMap.contains("Temperature_Cell"))
        pluginThing()->setStateValue(storageCellTemperatureStateTypeId, storageInfoMap.value("Temperature_Cell").toDouble());

    //update successful
    pluginThing()->setStateValue(storageConnectedStateTypeId,true);
}

QUrl FroniusStorage::activityUrl()
{
    QUrl requestUrl;
    requestUrl.setScheme("http");
    requestUrl.setHost(hostAddress());
    requestUrl.setPath(baseUrl()+"GetPowerFlowRealtimeData.fcgi");

    return requestUrl;
}

void FroniusStorage::updateActivityInfo(const QByteArray &data)
{
    // Convert the rawdata to a json document
    QJsonParseError error;
    QJsonDocument jsonDoc = QJsonDocument::fromJson(data, &error);
    if(error.error != QJsonParseError::NoError) {
        qCWarning(dcFronius()) << "FroniusStorage: Failed to parse JSON data" << data << ":" << error.errorString();
        return;
    }

    // create StorageInfo list map
    QVariantMap dataMap = jsonDoc.toVariant().toMap().value("Body").toMap().value("Data").toMap();

    float charge_akku = dataMap.value("Site").toMap().value("P_Akku").toFloat();
    if (charge_akku == 0) {
        pluginThing()->setStateValue(storageChargingStateTypeId, "inactive");
    } else if (charge_akku < 0) {
        pluginThing()->setStateValue(storageChargingStateTypeId, "charging");
    } else {
        pluginThing()->setStateValue(storageChargingStateTypeId, "discharging");
    }
}
