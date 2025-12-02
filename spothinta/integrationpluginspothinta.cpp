// SPDX-License-Identifier: GPL-3.0-or-later

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*
* Copyright (C) 2013 - 2024, nymea GmbH
* Copyright (C) 2024 - 2025, chargebyte austria GmbH
*
* This file is part of nymea-plugins.
*
* nymea-plugins is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* (at your option) any later version.
*
* nymea-plugins is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
* General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with nymea-plugins. If not, see <https://www.gnu.org/licenses/>.
*
* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

#include "integrationpluginspothinta.h"
#include "plugininfo.h"

#include <integrations/thing.h>
#include <hardwaremanager.h>
#include <network/networkaccessmanager.h>
#include <plugintimer.h>

#include <QDateTime>
#include <QJsonDocument>
#include <QSslConfiguration>

IntegrationPluginSpotHinta::IntegrationPluginSpotHinta()
{
}

IntegrationPluginSpotHinta::~IntegrationPluginSpotHinta()
{
}

void IntegrationPluginSpotHinta::setupThing(ThingSetupInfo *info)
{
    qCDebug(dcSpothinta) << "Setup thing" << info->thing()->name() << info->thing()->params();


    if (!m_pluginTimer) {
        m_pluginTimer = hardwareManager()->pluginTimerManager()->registerTimer(60 * 60);
        connect(m_pluginTimer, &PluginTimer::timeout, this, &IntegrationPluginSpotHinta::onPluginTimer);
    }

    requestPriceData(info->thing(), info);
}

void IntegrationPluginSpotHinta::thingRemoved(Thing *thing)
{
    Q_UNUSED(thing)
    if (m_pluginTimer && myThings().isEmpty()) {
        hardwareManager()->pluginTimerManager()->unregisterTimer(m_pluginTimer);
        m_pluginTimer = nullptr;
    }
}

void IntegrationPluginSpotHinta::onPluginTimer()
{
    foreach (Thing *thing, myThings()) {
        requestPriceData(thing);
    }
}

void IntegrationPluginSpotHinta::requestPriceData(Thing* thing, ThingSetupInfo *setup)
{
    QNetworkRequest request(QUrl("https://api.spot-hinta.fi/Today"));
    QNetworkReply *reply = hardwareManager()->networkManager()->get(request);
    connect(reply, &QNetworkReply::finished, reply, &QNetworkReply::deleteLater);
    connect(reply, &QNetworkReply::finished, thing, [this, reply, thing, setup](){

        if (reply->error() != QNetworkReply::NoError) {
            qCWarning(dcSpothinta()) << "Failed to retrieve spot-hinta market prices:" << reply->error() << reply->errorString();
            if (setup) {
                setup->finish(Thing::ThingErrorHardwareNotAvailable, QT_TR_NOOP("Error retrieving spot sprices from spot-hinta.fi."));
            } else {
                thing->setStateValue(spothintaConnectedStateTypeId, false);
            }
            return;
        }

        QByteArray data = reply->readAll();
        QJsonParseError error;
        QJsonDocument jsonDoc = QJsonDocument::fromJson(data, &error);
        if (error.error != QJsonParseError::NoError) {
            qCWarning(dcSpothinta()) << "Error parsing json from server:" << error.errorString() << qUtf8Printable(data);
            if (setup) {
                setup->finish(Thing::ThingErrorHardwareFailure, QT_TR_NOOP("The server returned unexpected data."));
            } else {
                thing->setStateValue(spothintaConnectedStateTypeId, false);
            }
            return;
        }

        if (setup) {
            setup->finish(Thing::ThingErrorNoError);
        }

        thing->setStateValue(spothintaConnectedStateTypeId, true);
        processPriceData(thing, jsonDoc.toVariant());
    });
}


void IntegrationPluginSpotHinta::processPriceData(Thing *thing, const QVariant &data)
{

    QDateTime currentTime = QDateTime::currentDateTime();
    double sum = 0;
    int count = 0;
    double currentPrice = 0;
    double averagePrice = 0;
    int deviation = 0;
    double maxPrice = -1000;
    double minPrice = 1000;
    foreach (QVariant element, data.toList()) {
        QVariantMap elementMap = element.toMap();
        QDateTime startTime = QDateTime::fromString(elementMap.value("DateTime").toString(), Qt::ISODate);
        QDateTime endTime = startTime.addMSecs(60 * 60 * 1000);
        double price = elementMap.value("PriceWithTax").toDouble();
        uint rank = elementMap.value("Rank").toUInt();

        sum += price;
        count++;

        if (price > maxPrice)
            maxPrice = price;

        if (price < minPrice)
            minPrice = price;

        if (currentTime  >= startTime && currentTime <= endTime) {
            currentPrice = price;
            sum += price;
            count++;

            if (price > maxPrice)
                maxPrice = price;

            if (price < minPrice)
                minPrice = price;

            thing->setStateValue(spothintaCurrentMarketPriceStateTypeId, price);
            thing->setStateValue(spothintaValidUntilStateTypeId, endTime.toLocalTime().toSecsSinceEpoch());
            thing->setStateValue(spothintaCurrentRankStateTypeId, rank);
        }
    }

    // calculate averagePrice and mean deviation
    if (count > 0) {
        averagePrice = sum / count;
    } else {
        averagePrice = 0;
    }

    if (currentPrice <= averagePrice) {
        deviation = -1 * qRound(100 + (-100 * (currentPrice - minPrice) / (averagePrice - minPrice)));
    } else {
        deviation = qRound(-100 * (averagePrice - currentPrice) / (maxPrice - averagePrice));
    }

    thing->setStateValue(spothintaAveragePriceStateTypeId, averagePrice);
    thing->setStateValue(spothintaLowestPriceStateTypeId, minPrice);
    thing->setStateValue(spothintaHighestPriceStateTypeId, maxPrice);
    thing->setStateValue(spothintaAverageDeviationStateTypeId, deviation);
}

