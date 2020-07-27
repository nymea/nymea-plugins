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

#include "froniusmeter.h"
#include "extern-plugininfo.h"

#include <QJsonDocument>
#include <QDateTime>

FroniusMeter::FroniusMeter(Thing* thing, QObject *parent) : FroniusThing(thing, parent)
{

}

QString FroniusMeter::activity() const
{
    return m_activity;
}

void FroniusMeter::setActivity(const QString &activity)
{
    m_activity = activity;
}

QUrl FroniusMeter::updateUrl()
{
    QUrl requestUrl;
    requestUrl.setScheme("http");
    QUrlQuery query;
    requestUrl.setHost(hostAddress());
    requestUrl.setPath(baseUrl() + "GetMeterRealtimeData.cgi");
    query.addQueryItem("Scope", "Device");
    query.addQueryItem("DeviceId", deviceId());
    requestUrl.setQuery(query);
    return requestUrl;
}

void FroniusMeter::updateThingInfo(const QByteArray &data)
{
    // Convert the rawdata to a JSON document
    QJsonParseError error;
    QJsonDocument jsonDoc = QJsonDocument::fromJson(data, &error);
    if (error.error != QJsonParseError::NoError) {
        qCWarning(dcFronius()) << "FroniusMeter: Failed to parse JSON data" << data << ":" << error.errorString();
        pluginThing()->setStateValue(meterConnectedStateTypeId, false);
        return;
    }

    // Parse the data and update the states of our thing
    QVariantMap dataMap = jsonDoc.toVariant().toMap().value("Body").toMap().value("Data").toMap();
    //QVariantMap headMap = jsonDoc.toVariant().toMap().value("Head").toMap();

    //Add Smart meter with following states: „PowerReal_P_Sum“, „EnergyReal_WAC_Sum_Produced“, „EnergyReal_WAC_Sum_Consumed“

    // Set the meter thing state
    if (dataMap.contains("PowerReal_P_Sum")) {
        pluginThing()->setStateValue(meterCurrentPowerStateTypeId, dataMap.value("PowerReal_P_Sum").toInt());
    }

    if (dataMap.contains("EnergyReal_WAC_Sum_Produced")) {
            pluginThing()->setStateValue(meterTotalEnergyProducedStateTypeId, dataMap.value("EnergyReal_WAC_Sum_Produced").toInt()/1000.00);
    }

    if (dataMap.contains("EnergyReal_WAC_Sum_Consumed")) {
            pluginThing()->setStateValue(meterTotalEnergyConsumedStateTypeId, dataMap.value("EnergyReal_WAC_Sum_Consumed").toInt()/1000.00);
    }

    //update successful
    pluginThing()->setStateValue(meterConnectedStateTypeId,true);
}

QUrl FroniusMeter::activityUrl()
{
    QUrl requestUrl;
    requestUrl.setScheme("http");
    requestUrl.setHost(hostAddress());
    requestUrl.setPath(baseUrl()+"GetPowerFlowRealtimeData.fcgi");

    return requestUrl;
}
