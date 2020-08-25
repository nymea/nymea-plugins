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

#include "froniuslogger.h"
#include "extern-plugininfo.h"

#include <QJsonDocument>
#include <QDateTime>

FroniusLogger::FroniusLogger(Thing *thing, QObject *parent) : FroniusThing(thing, parent)
{

}

QUrl FroniusLogger::updateUrl()
{
    QUrl requestUrl;
    requestUrl.setScheme("http");
    requestUrl.setHost(hostAddress());
    requestUrl.setPath(baseUrl() + "GetLoggerInfo.cgi");

    return requestUrl;
}

void FroniusLogger::updateThingInfo(const QByteArray &data)
{
    // Convert the rawdata to a json document
    QJsonParseError error;
    QJsonDocument jsonDoc = QJsonDocument::fromJson(data, &error);
    if (error.error != QJsonParseError::NoError) {
        // qCWarning(dcFroniusSolar()) << "Failed to parse JSON data" << data << ":" << error.errorString();
        pluginThing()->setStateValue(dataloggerConnectedStateTypeId,false);
        return;
    }

    // Parse the data and update the states of our device
    QVariantMap dataMap = jsonDoc.toVariant().toMap().value("Body").toMap().value("Data").toMap();
    QVariantMap headMap = jsonDoc.toVariant().toMap().value("Head").toMap();
    QVariantMap bodyMap = jsonDoc.toVariant().toMap().value("Body").toMap();

    // print the fetched data in dataMap format to stdout
    //qCDebug(dcFroniusSolar()) << dataMap;

    // create LoggerInfo list Map
    QVariantMap LoggerInfoMap = bodyMap.value("LoggerInfo").toMap();

    // copy retrieved information to device states
    if (LoggerInfoMap.contains("ProductID"))
        pluginThing()->setStateValue(dataloggerProductidStateTypeId, LoggerInfoMap.value("ProductID").toString());

    if (LoggerInfoMap.contains("PlatformID"))
        pluginThing()->setStateValue(dataloggerPlatformidStateTypeId, LoggerInfoMap.value("PlatformID").toString());

    if (LoggerInfoMap.contains("HWVersion"))
        pluginThing()->setStateValue(dataloggerHwversionStateTypeId, LoggerInfoMap.value("HWVersion").toString());

    if (LoggerInfoMap.contains("SWVersion"))
        pluginThing()->setStateValue(dataloggerSwversionStateTypeId, LoggerInfoMap.value("SWVersion").toString());

    if (LoggerInfoMap.contains("TimezoneLocation"))
        pluginThing()->setStateValue(dataloggerTzonelocStateTypeId, LoggerInfoMap.value("TimezoneLocation").toString());

    if (LoggerInfoMap.contains("TimezoneName"))
        pluginThing()->setStateValue(dataloggerTzoneStateTypeId, LoggerInfoMap.value("TimezoneName").toString());

    if (LoggerInfoMap.contains("DefaultLanguage"))
        pluginThing()->setStateValue(dataloggerDefaultlangStateTypeId, LoggerInfoMap.value("DefaultLanguage").toString());

    if (LoggerInfoMap.contains("CashFactor"))
        pluginThing()->setStateValue(dataloggerCashfactorStateTypeId, LoggerInfoMap.value("CashFactor").toDouble());

    if (LoggerInfoMap.contains("CashCurrency"))
        pluginThing()->setStateValue(dataloggerCashcurrencyStateTypeId, LoggerInfoMap.value("CashCurrency").toString());

    if (LoggerInfoMap.contains("CO2Factor"))
        pluginThing()->setStateValue(dataloggerCo2factorStateTypeId, LoggerInfoMap.value("CO2Factor").toDouble());

    if (LoggerInfoMap.contains("CO2Unit"))
        pluginThing()->setStateValue(dataloggerCo2unitStateTypeId, LoggerInfoMap.value("CO2Unit").toString());

    //update successful
    pluginThing()->setStateValue(dataloggerConnectedStateTypeId,true);
}

void FroniusLogger::updatePowerRelayState(const QByteArray &data)
{
    // Convert the rawdata to a json document
    QJsonParseError error;
    QJsonDocument jsonDoc = QJsonDocument::fromJson(data, &error);
    if (error.error != QJsonParseError::NoError) {
        // qCWarning(dcFroniusSolar()) << "Failed to parse JSON data" << data << ":" << error.errorString();
        pluginThing()->setStateValue(dataloggerConnectedStateTypeId,false);
        return;
    }

    // Parse the data and update the states of our device
    QVariantMap bodyMap = jsonDoc.toVariant().toMap().value("Body").toMap();
    QVariantMap dataMap = bodyMap.value("Data").toMap();
    QVariantMap emrsMap = dataMap.value("emrs").toMap();

    // create LoggerInfo list Map
    QVariantMap GpiosMap = emrsMap.value("gpios").toMap();
    //qCDebug(dcFroniusSolar()) << "Body: " << GpiosMap;

    // copy retrieved information to device states
    if (GpiosMap.contains("Reason")) {
        qCDebug(dcFronius()) << "Power Relay State Reason: " << GpiosMap.value("Reason").toString();
        pluginThing()->setStateValue(dataloggerPowerManagmentRelayReasonStateTypeId, GpiosMap.value("Reason").toString());
    }

    if (GpiosMap.contains("State")) {
        qCDebug(dcFronius()) << "Power Relay State: " << GpiosMap.value("State").toString();
        pluginThing()->setStateValue(dataloggerPowerManagmentRelayStateTypeId, GpiosMap.value("State").toBool());
    }

    //update successful
    pluginThing()->setStateValue(dataloggerConnectedStateTypeId,true);
}
