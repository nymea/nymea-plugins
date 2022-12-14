/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*
* Copyright 2013 - 2022, nymea GmbH
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

#include "integrationpluginspothinta.h"
#include "integrations/thing.h"
#include "plugininfo.h"
#include "hardwaremanager.h"
#include "network/networkaccessmanager.h"

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
            thing->setStateValue(spothintaValidUntilStateTypeId, endTime.toLocalTime().toTime_t());
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

