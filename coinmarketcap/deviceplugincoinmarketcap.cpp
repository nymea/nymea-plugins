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

#include "deviceplugincoinmarketcap.h"
#include "network/networkaccessmanager.h"
#include "plugininfo.h"

#include <QJsonDocument>

DevicePluginCoinMarketCap::DevicePluginCoinMarketCap()
{
}

void DevicePluginCoinMarketCap::setupDevice(DeviceSetupInfo *info)
{
    Device *device = info->device();

    if (device->deviceClassId() == currentPricesDeviceClassId) {
        getPriceCall(device);

        if(!m_pluginTimer) {
            m_pluginTimer = hardwareManager()->pluginTimerManager()->registerTimer(10);
            connect(m_pluginTimer, &PluginTimer::timeout, this, &DevicePluginCoinMarketCap::onPluginTimer);
        }

        info->finish(Device::DeviceErrorNoError);
        return;
    }
    info->finish(Device::DeviceErrorSetupFailed);
    return;
}

void DevicePluginCoinMarketCap::deviceRemoved(Device *device)
{
    while (m_httpRequests.values().contains(device)) {
        QNetworkReply *reply = m_httpRequests.key(device);
        m_httpRequests.remove(reply);
        reply->deleteLater();
    }

    if (myDevices().empty()) {
        hardwareManager()->pluginTimerManager()->unregisterTimer(m_pluginTimer);
        m_pluginTimer = nullptr;
    }
}

void DevicePluginCoinMarketCap::onPluginTimer()
{
    foreach (Device *device, myDevices()) {
        if (device->deviceClassId() == currentPricesDeviceClassId) {
            getPriceCall(device);
        }
    }
}

void DevicePluginCoinMarketCap::onPriceCallFinished()
{
    QNetworkReply *reply = static_cast<QNetworkReply *>(sender());
    reply->deleteLater();

    int status = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();

    if (!m_httpRequests.contains(reply)) {
        return;
    }

    Device *device = m_httpRequests.take(reply);

    // Check HTTP status code
    if (status != 200 || reply->error() != QNetworkReply::NoError) {
        qCWarning(dcCoinMarketCap()) << "Request error:" << status << reply->errorString();
        device->setStateValue(currentPricesConnectedStateTypeId, false);
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
        device->setStateValue(currentPricesConnectedStateTypeId, true);
        double price;

        if (elementMap.value("id").toString() == "bitcoin") {
            price = elementMap.value(QString("price_%1").arg(QString(device->paramValue(currentPricesDeviceFiatParamTypeId).toString()).toLower())).toDouble();
            qDebug(dcCoinMarketCap()) << "Bitcoin Price in" << QString(device->paramValue(currentPricesDeviceFiatParamTypeId).toString()).toLower() << price;
            device->setStateValue(currentPricesBtcStateTypeId, price);

        } else if (elementMap.value("id").toString() == "ethereum") {
            price = elementMap.value(QString("price_%1").arg(QString(device->paramValue(currentPricesDeviceFiatParamTypeId).toString()).toLower())).toDouble();
            qDebug(dcCoinMarketCap()) << "Etherium Price in" << QString(device->paramValue(currentPricesDeviceFiatParamTypeId).toString()).toLower() << price;
            device->setStateValue(currentPricesEthStateTypeId, price);

        } else if (elementMap.value("id").toString() == "ripple") {
            price = elementMap.value(QString("price_%1").arg(QString(device->paramValue(currentPricesDeviceFiatParamTypeId).toString()).toLower())).toDouble();
            qDebug(dcCoinMarketCap()) << "Ripple Price in" << QString(device->paramValue(currentPricesDeviceFiatParamTypeId).toString()).toLower() << price;
            device->setStateValue(currentPricesXrpStateTypeId, price);

        } else if (elementMap.value("id").toString() == "bitcoin-cash") {
            price = elementMap.value(QString("price_%1").arg(QString(device->paramValue(currentPricesDeviceFiatParamTypeId).toString()).toLower())).toDouble();
            qDebug(dcCoinMarketCap()) << "Bitcoin-cash Price in" << QString(device->paramValue(currentPricesDeviceFiatParamTypeId).toString()).toLower() << price;
            device->setStateValue(currentPricesBchStateTypeId, price);

        } else if (elementMap.value("id").toString() == "litecoin") {
            price = elementMap.value(QString("price_%1").arg(QString(device->paramValue(currentPricesDeviceFiatParamTypeId).toString()).toLower())).toDouble();
            qDebug(dcCoinMarketCap()) << "Litecoin Price in" << QString(device->paramValue(currentPricesDeviceFiatParamTypeId).toString()).toLower() << price;
            device->setStateValue(currentPricesLtcStateTypeId, price);

        } else if (elementMap.value("id").toString() == "nem") {
            price = elementMap.value(QString("price_%1").arg(QString(device->paramValue(currentPricesDeviceFiatParamTypeId).toString()).toLower())).toDouble();
            qDebug(dcCoinMarketCap()) << "Nem Price in" << QString(device->paramValue(currentPricesDeviceFiatParamTypeId).toString()).toLower() << price;
            device->setStateValue(currentPricesXemStateTypeId, price);

        } else if (elementMap.value("id").toString() == "ethereum-classic") {
            price = elementMap.value(QString("price_%1").arg(QString(device->paramValue(currentPricesDeviceFiatParamTypeId).toString()).toLower())).toDouble();
            qDebug(dcCoinMarketCap()) << "Ethereum Classic Price in" << QString(device->paramValue(currentPricesDeviceFiatParamTypeId).toString()).toLower() << price;
            device->setStateValue(currentPricesEtcStateTypeId, price);

        } else if (elementMap.value("id").toString() == "dash") {
            price = elementMap.value(QString("price_%1").arg(QString(device->paramValue(currentPricesDeviceFiatParamTypeId).toString()).toLower())).toDouble();
            device->setStateValue(currentPricesDashStateTypeId, price);

        } else if (elementMap.value("id").toString() == "iota") {
            price = elementMap.value(QString("price_%1").arg(QString(device->paramValue(currentPricesDeviceFiatParamTypeId).toString()).toLower())).toDouble();
            device->setStateValue(currentPricesMiotaStateTypeId, price);

        } else if (elementMap.value("id").toString() == "neo") {
            price = elementMap.value(QString("price_%1").arg(QString(device->paramValue(currentPricesDeviceFiatParamTypeId).toString()).toLower())).toDouble();
            device->setStateValue(currentPricesAnsStateTypeId, price);
        }
    }
}

void DevicePluginCoinMarketCap::getPriceCall(Device *device)
{
    QUrl url;
    url.setUrl(QString("https://api.coinmarketcap.com/v1/ticker/?convert=%1&limit=30").arg(QString(device->paramValue(currentPricesDeviceFiatParamTypeId).toString()).toLower()));
    QNetworkRequest request;
    request.setUrl(url);
    request.setRawHeader("User-Agent", "nymea 1.0");

    QNetworkReply *reply = hardwareManager()->networkManager()->get(request);
    connect(reply, &QNetworkReply::finished, this, &DevicePluginCoinMarketCap::onPriceCallFinished);

    m_httpRequests.insert(reply, device);
}
