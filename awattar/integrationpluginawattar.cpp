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

#include "integrationpluginawattar.h"
#include "integrations/thing.h"
#include "plugininfo.h"
#include "hardwaremanager.h"
#include "network/networkaccessmanager.h"

#include <QDateTime>
#include <QJsonDocument>
#include <QSslConfiguration>

IntegrationPluginAwattar::IntegrationPluginAwattar()
{
    m_serverUrls[awattarATThingClassId] = "https://api.awattar.com/v1/marketdata";
    m_serverUrls[awattarDEThingClassId] = "https://api.awattar.de/v1/marketdata";

    m_connectedStateTypeIds[awattarATThingClassId] = awattarATConnectedStateTypeId;
    m_connectedStateTypeIds[awattarDEThingClassId] = awattarDEConnectedStateTypeId;

    m_currentMarketPriceStateTypeIds[awattarATThingClassId] = awattarATCurrentMarketPriceStateTypeId;
    m_currentMarketPriceStateTypeIds[awattarDEThingClassId] = awattarDECurrentMarketPriceStateTypeId;

    m_validUntilStateTypeIds[awattarATThingClassId] = awattarATValidUntilStateTypeId;
    m_validUntilStateTypeIds[awattarDEThingClassId] = awattarDEValidUntilStateTypeId;

    m_averagePriceStateTypeIds[awattarATThingClassId] = awattarATAveragePriceStateTypeId;
    m_averagePriceStateTypeIds[awattarDEThingClassId] = awattarDEAveragePriceStateTypeId;

    m_lowestPriceStateTypeIds[awattarATThingClassId] = awattarATLowestPriceStateTypeId;
    m_lowestPriceStateTypeIds[awattarDEThingClassId] = awattarDELowestPriceStateTypeId;

    m_highestPriceStateTypeIds[awattarATThingClassId] = awattarATHighestPriceStateTypeId;
    m_highestPriceStateTypeIds[awattarDEThingClassId] = awattarDEHighestPriceStateTypeId;

    m_averageDeviationStateTypeIds[awattarATThingClassId] = awattarATAverageDeviationStateTypeId;
    m_averageDeviationStateTypeIds[awattarDEThingClassId] = awattarDEAverageDeviationStateTypeId;
}

IntegrationPluginAwattar::~IntegrationPluginAwattar()
{
}

void IntegrationPluginAwattar::setupThing(ThingSetupInfo *info)
{
    qCDebug(dcAwattar) << "Setup thing" << info->thing()->name() << info->thing()->params();


    if (!m_pluginTimer) {
        m_pluginTimer = hardwareManager()->pluginTimerManager()->registerTimer(60 * 60);
        connect(m_pluginTimer, &PluginTimer::timeout, this, &IntegrationPluginAwattar::onPluginTimer);
    }

    requestPriceData(info->thing(), info);
}

void IntegrationPluginAwattar::thingRemoved(Thing *thing)
{
    Q_UNUSED(thing)
    if (m_pluginTimer && myThings().isEmpty()) {
        hardwareManager()->pluginTimerManager()->unregisterTimer(m_pluginTimer);
        m_pluginTimer = nullptr;
    }
}

void IntegrationPluginAwattar::onPluginTimer()
{
    foreach (Thing *thing, myThings()) {
        requestPriceData(thing);
    }
}

void IntegrationPluginAwattar::requestPriceData(Thing* thing, ThingSetupInfo *setup)
{
    QNetworkRequest request(QUrl(m_serverUrls.value(thing->thingClassId())));
    request.setSslConfiguration(QSslConfiguration::defaultConfiguration());
    QNetworkReply *reply = hardwareManager()->networkManager()->get(request);
    connect(reply, &QNetworkReply::finished, thing, [this, reply, thing, setup](){
        reply->deleteLater();

        // check HTTP status code
        int status = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
        if (status != 200) {
            qCWarning(dcAwattar) << "Update reply HTTP error:" << status << reply->errorString();
            if (setup) {
                setup->finish(Thing::ThingErrorHardwareFailure, QT_TR_NOOP("Error getting data from server."));
            } else {
                thing->setStateValue(m_connectedStateTypeIds.value(thing->thingClassId()), false);
            }
            return;
        }
        // check JSON file
        QJsonParseError error;
        QJsonDocument jsonDoc = QJsonDocument::fromJson(reply->readAll(), &error);
        if (error.error != QJsonParseError::NoError) {
            qCWarning(dcAwattar) << "Update reply JSON error:" << error.errorString();
            if (setup) {
                setup->finish(Thing::ThingErrorHardwareFailure, QT_TR_NOOP("The server returned unexpected data."));
            } else {
                thing->setStateValue(m_connectedStateTypeIds.value(thing->thingClassId()), false);
            }
            return;
        }

        if (setup) {
            setup->finish(Thing::ThingErrorNoError);
        }

        thing->setStateValue(m_connectedStateTypeIds.value(thing->thingClassId()), true);

        processPriceData(thing, jsonDoc.toVariant().toMap());
    });
}


void IntegrationPluginAwattar::processPriceData(Thing *thing, const QVariantMap &data)
{
    QVariantList dataElements = data.value("data").toList();

    QDateTime currentTime = QDateTime::currentDateTime();
    double sum = 0;
    double count = 0;
    double averagePrice = 0;
    double currentPrice = 0;
    int deviation = 0;
    double maxPrice = -1000;
    double minPrice = 1000;
    foreach (QVariant element, dataElements) {
        QVariantMap elementMap = element.toMap();
        QDateTime startTime = QDateTime::fromMSecsSinceEpoch(elementMap.value("start_timestamp").toLongLong());
        QDateTime endTime = QDateTime::fromMSecsSinceEpoch(elementMap.value("end_timestamp").toLongLong());
        double price = elementMap.value("marketprice").toDouble();

        // check interval [-12h < x < + 12h]
        if ((startTime >= currentTime.addSecs(-3600 * 12) && endTime <= currentTime ) ||
                (endTime <= currentTime.addSecs(3600 * 12) && startTime >= currentTime )) {
            sum += price;
            count++;

            if (price > maxPrice)
                maxPrice = price;

            if (price < minPrice)
                minPrice = price;
        }

        if (currentTime  >= startTime && currentTime <= endTime) {
            currentPrice = price;
            sum += price;
            count++;

            if (price > maxPrice)
                maxPrice = price;

            if (price < minPrice)
                minPrice = price;

            thing->setStateValue(m_currentMarketPriceStateTypeIds.value(thing->thingClassId()), currentPrice / 10.0);
            thing->setStateValue(m_validUntilStateTypeIds.value(thing->thingClassId()), endTime.toLocalTime().toTime_t());
        }
    }

    // calculate averagePrice and mean deviation
    averagePrice = sum / count;

    if (currentPrice <= averagePrice) {
        deviation = -1 * qRound(100 + (-100 * (currentPrice - minPrice) / (averagePrice - minPrice)));
    } else {
        deviation = qRound(-100 * (averagePrice - currentPrice) / (maxPrice - averagePrice));
    }

    thing->setStateValue(m_averagePriceStateTypeIds.value(thing->thingClassId()), averagePrice / 10.0);
    thing->setStateValue(m_lowestPriceStateTypeIds.value(thing->thingClassId()), minPrice / 10.0);
    thing->setStateValue(m_highestPriceStateTypeIds.value(thing->thingClassId()), maxPrice / 10.0);
    thing->setStateValue(m_averageDeviationStateTypeIds.value(thing->thingClassId()), deviation);
}

