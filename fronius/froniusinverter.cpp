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

#include "froniusinverter.h"
#include <QJsonDocument>
#include "extern-plugininfo.h"
#include <QDateTime>

FroniusInverter::FroniusInverter(Thing *thing, QObject *parent) : FroniusThing(thing, parent)
{

}

QString FroniusInverter::activity() const
{
    return m_activity;
}

void FroniusInverter::setActivity(const QString &activity)
{
    m_activity = activity;
}

QUrl FroniusInverter::updateUrl()
{
    QUrl requestUrl;
    requestUrl.setScheme("http");
    QUrlQuery query;
    requestUrl.setHost(hostAddress());
    requestUrl.setPath(baseUrl() + "GetInverterRealtimeData.cgi");
    query.addQueryItem("Scope", "Device");
    query.addQueryItem("DeviceId", deviceId());
    query.addQueryItem("DataCollection", "CommonInverterData");
    requestUrl.setQuery(query);

    return requestUrl;
}

void FroniusInverter::updateThingInfo(const QByteArray &data)
{
    // Convert the rawdata to a JSON document
    QJsonParseError error;
    QJsonDocument jsonDoc = QJsonDocument::fromJson(data, &error);
    if (error.error != QJsonParseError::NoError) {
        qCWarning(dcFronius()) << "FroniusInverter: Failed to parse JSON data" << data << ":" << error.errorString();
        pluginThing()->setStateValue(inverterConnectedStateTypeId,false);
        return;
    }

    // Parse the data and update the states of our device
    QVariantMap dataMap = jsonDoc.toVariant().toMap().value("Body").toMap().value("Data").toMap();
    QVariantMap headMap = jsonDoc.toVariant().toMap().value("Head").toMap();

    // Set the inverter device state
    if (dataMap.contains("PAC")) {
        if(dataMap.value("PAC").toMap().values().at(0) == "W")
            pluginThing()->setStateValue(inverterCurrentPowerStateTypeId, dataMap.value("PAC").toMap().values().at(1).toInt());
    }

    if (dataMap.contains("DAY_ENERGY")) {
        if (dataMap.value("DAY_ENERGY").toMap().values().at(0) == "Wh")
            pluginThing()->setStateValue(inverterEdayStateTypeId, dataMap.value("DAY_ENERGY").toMap().values().at(1).toDouble()/1000);
    }

    if (dataMap.contains("YEAR_ENERGY")) {
        if(dataMap.value("YEAR_ENERGY").toMap().values().at(0) == "Wh")
            pluginThing()->setStateValue(inverterEyearStateTypeId, dataMap.value("YEAR_ENERGY").toMap().values().at(1).toInt()/1000);
    }

    if (dataMap.contains("TOTAL_ENERGY")) {
        if(dataMap.value("TOTAL_ENERGY").toMap().values().at(0) == "Wh")
            pluginThing()->setStateValue(inverterTotalEnergyProducedStateTypeId, dataMap.value("TOTAL_ENERGY").toMap().values().at(1).toInt()/1000);
    }

    //update successful
    pluginThing()->setStateValue(inverterConnectedStateTypeId,true);

}

QUrl FroniusInverter::activityUrl()
{
    QUrl requestUrl;
    requestUrl.setScheme("http");
    requestUrl.setHost(hostAddress());
    requestUrl.setPath(baseUrl()+"GetPowerFlowRealtimeData.fcgi");

    return requestUrl;
}

void FroniusInverter::updateActivityInfo(const QByteArray &data)
{
    // Convert the rawdata to a json document
    QJsonParseError error;
    QJsonDocument jsonDoc = QJsonDocument::fromJson(data, &error);
    if(error.error != QJsonParseError::NoError) {
        qCWarning(dcFronius()) << "FroniusInverter: Failed to parse JSON data" << data << ":" << error.errorString();
        return;
    }

    // create StorageInfo list map
    QVariantMap dataMap = jsonDoc.toVariant().toMap().value("Body").toMap().value("Data").toMap();

    if (dataMap.value("Site").toMap().value("P_PV").toFloat() > 0) {
        pluginThing()->setStateValue(inverterActiveStateTypeId, "production");
    } else {
        pluginThing()->setStateValue(inverterActiveStateTypeId, "inactive");
    }
}
