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

#include "integrationpluginsolarlog.h"
#include "plugininfo.h"
#include "network/networkaccessmanager.h"

#include <QJsonDocument>
#include <QJsonObject>
#include <QVariantMap>

IntegrationPluginSolarLog::IntegrationPluginSolarLog()
{

}

void IntegrationPluginSolarLog::setupThing(ThingSetupInfo *info)
{
    Thing *thing = info->thing();
    if (thing->thingClassId() == solarlogThingClassId) {
        getData(thing);
        m_asyncSetup.insert(thing, info);
        connect(info, &ThingSetupInfo::aborted, this, [thing, this] {m_asyncSetup.remove(thing);});
    } else {
        Q_ASSERT_X(false, "setupThing", QString("Unhandled thingClassId: %1").arg(thing->thingClassId().toString()).toUtf8());
    }
}

void IntegrationPluginSolarLog::postSetupThing(Thing *thing)
{
    Q_UNUSED(thing)

    if (!m_refreshTimer) {
        m_refreshTimer = hardwareManager()->pluginTimerManager()->registerTimer(2);
        connect(m_refreshTimer, &PluginTimer::timeout, this, &IntegrationPluginSolarLog::onRefreshTimer);
    }
}

void IntegrationPluginSolarLog::thingRemoved(Thing *thing)
{
    Q_UNUSED(thing)

    if (myThings().isEmpty()) {
        hardwareManager()->pluginTimerManager()->unregisterTimer(m_refreshTimer);
        m_refreshTimer = nullptr;
    }
}

void IntegrationPluginSolarLog::onRefreshTimer()
{
    foreach (Thing *thing, myThings().filterByThingClassId(solarlogThingClassId)) {
        getData(thing);
    }
}

void IntegrationPluginSolarLog::getData(Thing *thing)
{
    QUrl url;
    url.setHost(thing->paramValue(solarlogThingHostParamTypeId).toString());
    url.setPath("/getjp");
    url.setScheme("http");
    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::KnownHeaders::ContentTypeHeader, "application/json");
    QNetworkReply *reply = hardwareManager()->networkManager()->post(request, QByteArray("{\"801\":{\"170\":null}}"));
    connect(reply, &QNetworkReply::finished, reply, &QNetworkReply::deleteLater);
    connect(reply, &QNetworkReply::finished, thing, [this, reply, thing]{

        int status = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();

        // Check HTTP status code
        if (status != 200 || reply->error() != QNetworkReply::NoError) {
            qCWarning(dcSolarlog()) << "Request error:" << status << reply->errorString();
            if (m_asyncSetup.contains(thing)) {
                ThingSetupInfo *info = m_asyncSetup.take(thing);
                info->finish(Thing::ThingErrorHardwareNotAvailable, tr("No Solar-Log device at given IP-Address"));
            }
            markThingDisconnected(thing);
            return;
        }

        thing->setStateValue(solarlogConnectedStateTypeId, true);
        QByteArray rawData = reply->readAll();
        QJsonParseError error;
        QJsonDocument data = QJsonDocument::fromJson(rawData, &error);
        if (error.error != QJsonParseError::NoError) {
            qCWarning(dcSolarlog()) << "Received invalid JSON object, try to upgrade the Solarlog firmware. Min Version is 3.5.";
            if (m_asyncSetup.contains(thing)) {
                ThingSetupInfo *info = m_asyncSetup.take(thing);
                info->finish(Thing::ThingErrorHardwareFailure, tr("Outdated Solar-Log firmware"));
            }
            return;
        }

        if (m_asyncSetup.contains(thing)) {
            ThingSetupInfo *info = m_asyncSetup.take(thing);
            info->finish(Thing::ThingErrorNoError);
        }

        QVariantMap map = data.toVariant().toMap().value("801").toMap().value("170").toMap();
        thing->setStateValue(solarlogLastupdateStateTypeId, map.value(QString::number(JsonObjectNumbers::LastUpdateTime)));
        thing->setStateValue(solarlogCurrentPowerStateTypeId, (map.value(QString::number(JsonObjectNumbers::Pac)).toDouble()/1000.00));
        thing->setStateValue(solarlogPdcStateTypeId, (map.value(QString::number(JsonObjectNumbers::Pdc)).toDouble()/1000.00));
        thing->setStateValue(solarlogUacStateTypeId, map.value(QString::number(JsonObjectNumbers::Uac)));
        thing->setStateValue(solarlogDcVoltageStateTypeId, map.value(QString::number(JsonObjectNumbers::DCVoltage)));
        thing->setStateValue(solarlogYieldDayStateTypeId, (map.value(QString::number(JsonObjectNumbers::YieldDay)).toDouble()/1000.00));
        thing->setStateValue(solarlogYieldYesterdayStateTypeId, (map.value(QString::number(JsonObjectNumbers::YieldYesterday)).toDouble()/1000.00));
        thing->setStateValue(solarlogYieldMonthStateTypeId, (map.value(QString::number(JsonObjectNumbers::YieldMonth)).toDouble()/1000.00));
        thing->setStateValue(solarlogYieldYearStateTypeId, (map.value(QString::number(JsonObjectNumbers::YieldYear)).toDouble()/1000.00));
        thing->setStateValue(solarlogTotalEnergyProducedStateTypeId, (map.value(QString::number(JsonObjectNumbers::YieldTotal)).toDouble()/1000.00));
        thing->setStateValue(solarlogPowerUsageStateTypeId, map.value(QString::number(JsonObjectNumbers::ConsPac)));
        thing->setStateValue(solarlogConsYieldDayStateTypeId, map.value(QString::number(JsonObjectNumbers::ConsYieldDay)));
        thing->setStateValue(solarlogConsYieldYesterdayStateTypeId, map.value(QString::number(JsonObjectNumbers::ConsYieldYesterday)));
        thing->setStateValue(solarlogConsYieldMonthStateTypeId, map.value(QString::number(JsonObjectNumbers::ConsYieldMonth)));
        thing->setStateValue(solarlogConsYieldYearStateTypeId, map.value(QString::number(JsonObjectNumbers::ConsYieldYear)));
        thing->setStateValue(solarlogConsYieldTotalStateTypeId, map.value(QString::number(JsonObjectNumbers::ConsYieldTotal)));
        thing->setStateValue(solarlogTotalPowerStateTypeId, (map.value(QString::number(JsonObjectNumbers::TotalPower)).toDouble()/1000.00));
    });
}

void IntegrationPluginSolarLog::markThingDisconnected(Thing *thing)
{
    qCDebug(dcSolarlog()) << "Resetting states of" << thing;
    thing->setStateValue(solarlogConnectedStateTypeId, false);
    thing->setStateValue(solarlogCurrentPowerStateTypeId, 0);
    thing->setStateValue(solarlogPdcStateTypeId, 0);
    thing->setStateValue(solarlogUacStateTypeId, 0);
    thing->setStateValue(solarlogDcVoltageStateTypeId, 0);
    thing->setStateValue(solarlogPowerUsageStateTypeId, 0);
    thing->setStateValue(solarlogTotalPowerStateTypeId, 0);
}
