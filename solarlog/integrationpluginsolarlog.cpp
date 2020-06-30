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
    if (!m_refreshTimer) {
        m_refreshTimer = hardwareManager()->pluginTimerManager()->registerTimer(2);
        connect(m_refreshTimer, &PluginTimer::timeout, this, &IntegrationPluginSolarLog::onRefreshTimer);
    }
    info->finish(Thing::ThingErrorNoError);
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
    request.setHeader(QNetworkRequest::KnownHeaders::ContentTypeHeader, "text/plain");
    QNetworkReply *reply = hardwareManager()->networkManager()->post(request, QByteArray("{\"801\":{\"170\":null}}"));
    connect(reply, &QNetworkReply::finished, reply, &QNetworkReply::deleteLater);
    connect(reply, &QNetworkReply::finished, this, [this, reply, thing]{

        int status = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();

        // Check HTTP status code
        if (status != 200 || reply->error() != QNetworkReply::NoError) {
            if (reply->error() == QNetworkReply::HostNotFoundError) {
            }
            qCWarning(dcSolarlog()) << "Request error:" << status << reply->errorString();
            return;
        }

        QByteArray rawData = reply->readAll();
        qCDebug(dcSolarlog()) << "Data:" << rawData;
        QJsonParseError error;
        QJsonDocument data = QJsonDocument::fromJson(rawData, &error);
        if (error.error != QJsonParseError::NoError) {
            qCWarning(dcSolarlog()) << "Received invalide JSON object" << data.toJson();
            return;
        }

        QVariantMap map = data.toVariant().toMap().value("801").toMap().value("170").toMap();
        qCDebug(dcSolarlog()) << "Map 170:" << map;
        thing->setStateValue(solarlogLastupdateStateTypeId, map.value(QString::number(JsonObjectNumbers::LastUpdateTime)));
        thing->setStateValue(solarlogPacStateTypeId, map.value(QString::number(JsonObjectNumbers::Pac)));
        thing->setStateValue(solarlogPdcStateTypeId, map.value(QString::number(JsonObjectNumbers::Pdc)));
        thing->setStateValue(solarlogUacStateTypeId, map.value(QString::number(JsonObjectNumbers::Uac)));
        thing->setStateValue(solarlogDcVoltageStateTypeId, map.value(QString::number(JsonObjectNumbers::DCVoltage)));
        thing->setStateValue(solarlogYieldDayStateTypeId, map.value(QString::number(JsonObjectNumbers::YieldDay)));
        thing->setStateValue(solarlogYieldYesterdayStateTypeId, map.value(QString::number(JsonObjectNumbers::YieldYesterday)));
        thing->setStateValue(solarlogYieldMonthStateTypeId, map.value(QString::number(JsonObjectNumbers::YieldMonth)));
        thing->setStateValue(solarlogYieldYearStateTypeId, map.value(QString::number(JsonObjectNumbers::YieldYear)));
        thing->setStateValue(solarlogYieldTotalStateTypeId, map.value(QString::number(JsonObjectNumbers::YieldTotal)));
        thing->setStateValue(solarlogCurrentTotalConsumptionStateTypeId, map.value(QString::number(JsonObjectNumbers::ConsPac)));
        thing->setStateValue(solarlogConsYieldDayStateTypeId, map.value(QString::number(JsonObjectNumbers::ConsYieldDay)));
        thing->setStateValue(solarlogConsYieldYesterdayStateTypeId, map.value(QString::number(JsonObjectNumbers::ConsYieldYesterday)));
        thing->setStateValue(solarlogConsYieldMonthStateTypeId, map.value(QString::number(JsonObjectNumbers::ConsYieldMonth)));
        thing->setStateValue(solarlogConsYieldYearStateTypeId, map.value(QString::number(JsonObjectNumbers::ConsYieldYear)));
        thing->setStateValue(solarlogConsYieldTotalStateTypeId, map.value(QString::number(JsonObjectNumbers::ConsYieldTotal)));
        thing->setStateValue(solarlogTotalPowerStateTypeId, map.value(QString::number(JsonObjectNumbers::TotalPower)));
    });
}
