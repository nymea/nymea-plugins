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

#include "integrationplugincoinmarketcap.h"
#include "network/networkaccessmanager.h"
#include "plugininfo.h"

#include <QJsonDocument>

IntegrationPluginCoinMarketCap::IntegrationPluginCoinMarketCap()
{
}

void IntegrationPluginCoinMarketCap::setupThing(ThingSetupInfo *info)
{
    Thing *thing = info->thing();

    if (thing->thingClassId() == currentPricesThingClassId) {
        getPriceCall(thing);

        if(!m_pluginTimer) {
            m_pluginTimer = hardwareManager()->pluginTimerManager()->registerTimer(10);
            connect(m_pluginTimer, &PluginTimer::timeout, this, &IntegrationPluginCoinMarketCap::onPluginTimer);
        }

        info->finish(Thing::ThingErrorNoError);
        return;
    }
    info->finish(Thing::ThingErrorSetupFailed);
    return;
}

void IntegrationPluginCoinMarketCap::thingRemoved(Thing *thing)
{
    while (m_httpRequests.values().contains(thing)) {
        QNetworkReply *reply = m_httpRequests.key(thing);
        m_httpRequests.remove(reply);
        reply->deleteLater();
    }

    if (myThings().empty()) {
        hardwareManager()->pluginTimerManager()->unregisterTimer(m_pluginTimer);
        m_pluginTimer = nullptr;
    }
}

void IntegrationPluginCoinMarketCap::onPluginTimer()
{
    foreach (Thing *thing, myThings()) {
        if (thing->thingClassId() == currentPricesThingClassId) {
            getPriceCall(thing);
        }
    }
}

void IntegrationPluginCoinMarketCap::onPriceCallFinished()
{
    QNetworkReply *reply = static_cast<QNetworkReply *>(sender());
    reply->deleteLater();

    int status = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();

    if (!m_httpRequests.contains(reply)) {
        return;
    }

    Thing *thing = m_httpRequests.take(reply);

    // Check HTTP status code
    if (status != 200 || reply->error() != QNetworkReply::NoError) {
        qCWarning(dcCoinMarketCap()) << "Request error:" << status << reply->errorString();
        thing->setStateValue(currentPricesConnectedStateTypeId, false);
    }

    // check JSON file
    QJsonParseError error;
    QJsonDocument jsonResponse = QJsonDocument::fromJson(reply->readAll(), &error);

    if (error.error != QJsonParseError::NoError) {
        qCWarning(dcCoinMarketCap()) << "Update reply JSON error:" << error.errorString();
        return;
    }

    QVariantList list = jsonResponse.toVariant().toList();

    foreach (QVariant element, list) {
        QVariantMap elementMap = element.toMap();
        thing->setStateValue(currentPricesConnectedStateTypeId, true);
        double price;

        if (elementMap.value("id").toString() == "bitcoin") {
            price = elementMap.value(QString("price_%1").arg(QString(thing->paramValue(currentPricesThingFiatParamTypeId).toString()).toLower())).toDouble();
            qDebug(dcCoinMarketCap()) << "Bitcoin Price in" << QString(thing->paramValue(currentPricesThingFiatParamTypeId).toString()).toLower() << price;
            thing->setStateValue(currentPricesBtcStateTypeId, price);

        } else if (elementMap.value("id").toString() == "ethereum") {
            price = elementMap.value(QString("price_%1").arg(QString(thing->paramValue(currentPricesThingFiatParamTypeId).toString()).toLower())).toDouble();
            qDebug(dcCoinMarketCap()) << "Etherium Price in" << QString(thing->paramValue(currentPricesThingFiatParamTypeId).toString()).toLower() << price;
            thing->setStateValue(currentPricesEthStateTypeId, price);

        } else if (elementMap.value("id").toString() == "ripple") {
            price = elementMap.value(QString("price_%1").arg(QString(thing->paramValue(currentPricesThingFiatParamTypeId).toString()).toLower())).toDouble();
            qDebug(dcCoinMarketCap()) << "Ripple Price in" << QString(thing->paramValue(currentPricesThingFiatParamTypeId).toString()).toLower() << price;
            thing->setStateValue(currentPricesXrpStateTypeId, price);

        } else if (elementMap.value("id").toString() == "bitcoin-cash") {
            price = elementMap.value(QString("price_%1").arg(QString(thing->paramValue(currentPricesThingFiatParamTypeId).toString()).toLower())).toDouble();
            qDebug(dcCoinMarketCap()) << "Bitcoin-cash Price in" << QString(thing->paramValue(currentPricesThingFiatParamTypeId).toString()).toLower() << price;
            thing->setStateValue(currentPricesBchStateTypeId, price);

        } else if (elementMap.value("id").toString() == "litecoin") {
            price = elementMap.value(QString("price_%1").arg(QString(thing->paramValue(currentPricesThingFiatParamTypeId).toString()).toLower())).toDouble();
            qDebug(dcCoinMarketCap()) << "Litecoin Price in" << QString(thing->paramValue(currentPricesThingFiatParamTypeId).toString()).toLower() << price;
            thing->setStateValue(currentPricesLtcStateTypeId, price);

        } else if (elementMap.value("id").toString() == "nem") {
            price = elementMap.value(QString("price_%1").arg(QString(thing->paramValue(currentPricesThingFiatParamTypeId).toString()).toLower())).toDouble();
            qDebug(dcCoinMarketCap()) << "Nem Price in" << QString(thing->paramValue(currentPricesThingFiatParamTypeId).toString()).toLower() << price;
            thing->setStateValue(currentPricesXemStateTypeId, price);

        } else if (elementMap.value("id").toString() == "ethereum-classic") {
            price = elementMap.value(QString("price_%1").arg(QString(thing->paramValue(currentPricesThingFiatParamTypeId).toString()).toLower())).toDouble();
            qDebug(dcCoinMarketCap()) << "Ethereum Classic Price in" << QString(thing->paramValue(currentPricesThingFiatParamTypeId).toString()).toLower() << price;
            thing->setStateValue(currentPricesEtcStateTypeId, price);

        } else if (elementMap.value("id").toString() == "dash") {
            price = elementMap.value(QString("price_%1").arg(QString(thing->paramValue(currentPricesThingFiatParamTypeId).toString()).toLower())).toDouble();
            thing->setStateValue(currentPricesDashStateTypeId, price);

        } else if (elementMap.value("id").toString() == "iota") {
            price = elementMap.value(QString("price_%1").arg(QString(thing->paramValue(currentPricesThingFiatParamTypeId).toString()).toLower())).toDouble();
            thing->setStateValue(currentPricesMiotaStateTypeId, price);

        } else if (elementMap.value("id").toString() == "neo") {
            price = elementMap.value(QString("price_%1").arg(QString(thing->paramValue(currentPricesThingFiatParamTypeId).toString()).toLower())).toDouble();
            thing->setStateValue(currentPricesAnsStateTypeId, price);
        }
    }
}

void IntegrationPluginCoinMarketCap::getPriceCall(Thing *thing)
{
    QUrl url;
    url.setUrl(QString("https://api.coinmarketcap.com/v1/ticker/?convert=%1&limit=30").arg(QString(thing->paramValue(currentPricesThingFiatParamTypeId).toString()).toLower()));
    QNetworkRequest request;
    request.setUrl(url);
    request.setRawHeader("User-Agent", "nymea 1.0");

    QNetworkReply *reply = hardwareManager()->networkManager()->get(request);
    connect(reply, &QNetworkReply::finished, this, &IntegrationPluginCoinMarketCap::onPriceCallFinished);

    m_httpRequests.insert(reply, thing);
}
