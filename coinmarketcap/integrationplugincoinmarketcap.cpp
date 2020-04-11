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


void IntegrationPluginCoinMarketCap::startPairing(ThingPairingInfo *info)
{
    NetworkAccessManager *network = hardwareManager()->networkManager();
    QNetworkReply *reply = network->get(QNetworkRequest(QUrl("https://pro-api.coinmarketcap.com")));
    connect(reply, &QNetworkReply::finished, this, [reply, info] {
        reply->deleteLater();

        if (reply->error() == QNetworkReply::NetworkError::HostNotFoundError) {
            info->finish(Thing::ThingErrorHardwareNotAvailable, QT_TR_NOOP("CoinMarketCap server is not reachable."));
        } else {
            info->finish(Thing::ThingErrorNoError, QT_TR_NOOP("Please enter your API token."));
        }
    });
}

void IntegrationPluginCoinMarketCap::confirmPairing(ThingPairingInfo *info, const QString &username, const QString &secret)
{
    Q_UNUSED(username)

    QNetworkRequest request(QUrl("https://pro-api.coinmarketcap.com/v1/key/info"));
    request.setRawHeader("X-CMC_PRO_API_KEY", secret.toUtf8());
    request.setRawHeader("Accept", "application/json");

    QNetworkReply *reply = hardwareManager()->networkManager()->get(request);
    connect(reply, &QNetworkReply::finished, info, [this, reply, info, secret](){
        reply->deleteLater();

        int status = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();

        // check HTTP status code
        if (status != 200) {
            //: Error setting up device with invalid token
            info->finish(Thing::ThingErrorAuthenticationFailure, QT_TR_NOOP("This token is not valid."));
            return;
        }

        pluginStorage()->beginGroup(info->thingId().toString());
        pluginStorage()->setValue("apiKey", secret);
        pluginStorage()->endGroup();
        info->finish(Thing::ThingErrorNoError);
    });
}

void IntegrationPluginCoinMarketCap::setupThing(ThingSetupInfo *info)
{
    Thing *thing = info->thing();

    if (thing->thingClassId() == currentPricesThingClassId) {

        pluginStorage()->beginGroup(info->thing()->id().toString());
        QByteArray apiKey = pluginStorage()->value("apiKey").toByteArray();
        pluginStorage()->endGroup();

        m_apiKeys.insert(thing->id(), apiKey);
        getPriceCall(thing);
        info->finish(Thing::ThingErrorNoError);
        return;
    }
    info->finish(Thing::ThingErrorSetupFailed);
    return;
}

void IntegrationPluginCoinMarketCap::thingRemoved(Thing *thing)
{
    if (thing->thingClassId() == currentPricesThingClassId) {
        m_apiKeys.remove(thing->id());

        while (m_httpRequests.values().contains(thing)) {
            QNetworkReply *reply = m_httpRequests.key(thing);
            m_httpRequests.remove(reply);
            reply->deleteLater();
        }
    }

    if (myThings().empty() && m_pluginTimer) {
        hardwareManager()->pluginTimerManager()->unregisterTimer(m_pluginTimer);
        m_pluginTimer = nullptr;
    }
}

void IntegrationPluginCoinMarketCap::postSetupThing(Thing *thing)
{
    Q_UNUSED(thing)

    if(!m_pluginTimer) {
        m_pluginTimer = hardwareManager()->pluginTimerManager()->registerTimer(120);
        connect(m_pluginTimer, &PluginTimer::timeout, this, &IntegrationPluginCoinMarketCap::onPluginTimer);
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

    QVariantList list = jsonResponse.toVariant().toMap().value("data").toList();

    foreach (QVariant element, list) {
        QVariantMap elementMap = element.toMap();
        thing->setStateValue(currentPricesConnectedStateTypeId, true);
        double price;
        QString fiatCurrency = thing->paramValue(currentPricesThingFiatParamTypeId).toString().toUpper();
        qCDebug(dcCoinMarketCap()) << "Name" << elementMap.value("name").toString();
        price = elementMap.value("quote").toMap().value(fiatCurrency).toMap().value("price").toDouble();
        if (elementMap.value("name").toString() == "Bitcoin") {
            qDebug(dcCoinMarketCap()) << "Bitcoin Price in" << fiatCurrency << price;
            thing->setStateValue(currentPricesBtcStateTypeId, price);

        } else if (elementMap.value("name").toString() == "Ethereum") {
            qDebug(dcCoinMarketCap()) << "Etherium Price in" <<  fiatCurrency << price;
            thing->setStateValue(currentPricesEthStateTypeId, price);

        } else if (elementMap.value("name").toString() == "XRP") {
            qDebug(dcCoinMarketCap()) << "XRP Price in" << fiatCurrency << price;
            thing->setStateValue(currentPricesXrpStateTypeId, price);

        } else if (elementMap.value("name").toString() == "Bitcoin Cash") {
            qDebug(dcCoinMarketCap()) << "Bitcoin-cash Price in" << fiatCurrency << price;
            thing->setStateValue(currentPricesBchStateTypeId, price);

        } else if (elementMap.value("name").toString() == "Litecoin") {
            qDebug(dcCoinMarketCap()) << "Litecoin Price in" << fiatCurrency << price;
            thing->setStateValue(currentPricesLtcStateTypeId, price);

        } else if (elementMap.value("name").toString() == "NEM") {
            qDebug(dcCoinMarketCap()) << "Nem Price in" << fiatCurrency << price;
            thing->setStateValue(currentPricesXemStateTypeId, price);

        } else if (elementMap.value("name").toString() == "Ethereum Classic") {
            qDebug(dcCoinMarketCap()) << "Ethereum Classic Price in" << fiatCurrency << price;
            thing->setStateValue(currentPricesEtcStateTypeId, price);

        } else if (elementMap.value("name").toString() == "Dash") {
            thing->setStateValue(currentPricesDashStateTypeId, price);

        } else if (elementMap.value("name").toString() == "IOTA") {
            thing->setStateValue(currentPricesMiotaStateTypeId, price);

        } else if (elementMap.value("name").toString() == "Neo") {
            thing->setStateValue(currentPricesAnsStateTypeId, price);
        }
    }
}

void IntegrationPluginCoinMarketCap::getPriceCall(Thing *thing)
{
    QUrl url;
    QString fiatCurrency = thing->paramValue(currentPricesThingFiatParamTypeId).toString().toUpper();
    url.setUrl(QString("https://pro-api.coinmarketcap.com/v1/cryptocurrency/listings/latest?convert=%1&start=1&limit=30").arg(fiatCurrency));
    QNetworkRequest request;
    request.setUrl(url);
    request.setRawHeader("X-CMC_PRO_API_KEY", m_apiKeys.value(thing->id()));
    request.setRawHeader("Accept", "application/json");
    request.setRawHeader("User-Agent", "nymea 1.0");
    qCDebug(dcCoinMarketCap()) << "Sending request" << url << "API key" << m_apiKeys.value(thing->id());
    QNetworkReply *reply = hardwareManager()->networkManager()->get(request);
    connect(reply, &QNetworkReply::finished, this, &IntegrationPluginCoinMarketCap::onPriceCallFinished);

    m_httpRequests.insert(reply, thing);
}
