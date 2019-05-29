/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *                                                                         *
 *  Copyright (C) 2015 Simon Stürz <simon.stuerz@guh.io>                   *
 *                                                                         *
 *  This file is part of nymea.                                            *
 *                                                                         *
 *  This library is free software; you can redistribute it and/or          *
 *  modify it under the terms of the GNU Lesser General Public             *
 *  License as published by the Free Software Foundation; either           *
 *  version 2.1 of the License, or (at your option) any later version.     *
 *                                                                         *
 *  This library is distributed in the hope that it will be useful,        *
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of         *
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU      *
 *  Lesser General Public License for more details.                        *
 *                                                                         *
 *  You should have received a copy of the GNU Lesser General Public       *
 *  License along with this library; If not, see                           *
 *  <http://www.gnu.org/licenses/>.                                        *
 *                                                                         *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */


#include "devicepluginawattar.h"
#include "plugin/device.h"
#include "plugininfo.h"
#include "hardwaremanager.h"
#include "network/networkaccessmanager.h"

#include <QDateTime>
#include <QJsonDocument>
#include <QSslConfiguration>

DevicePluginAwattar::DevicePluginAwattar()
{
}

DevicePluginAwattar::~DevicePluginAwattar()
{
}

DeviceManager::DeviceSetupStatus DevicePluginAwattar::setupDevice(Device *device)
{
    qCDebug(dcAwattar) << "Setup device" << device->name() << device->params();

    QString userUuid = device->paramValue(awattarDeviceUserUuidParamTypeId).toString();
    QString token = device->paramValue(awattarDeviceTokenParamTypeId).toString();

    if (token.isEmpty() || userUuid.isEmpty()) {
        qCWarning(dcAwattar) << "Missing token oder user uuid.";
        return DeviceManager::DeviceSetupStatusFailure;
    }

    if (!m_pluginTimer) {
        m_pluginTimer = hardwareManager()->pluginTimerManager()->registerTimer(60 * 60);
        connect(m_pluginTimer, &PluginTimer::timeout, this, &DevicePluginAwattar::onPluginTimer);
    }

    requestPriceData(device, true);

    return DeviceManager::DeviceSetupStatusAsync;
}

void DevicePluginAwattar::deviceRemoved(Device *device)
{
    Q_UNUSED(device)
    if (m_pluginTimer && myDevices().isEmpty()) {
        hardwareManager()->pluginTimerManager()->unregisterTimer(m_pluginTimer);
    }
}

void DevicePluginAwattar::onPluginTimer()
{
    foreach (Device *device, myDevices()) {
        requestPriceData(device);
    }
}

void DevicePluginAwattar::requestPriceData(Device* device, bool setupInProgress)
{
    QString token = device->paramValue(awattarDeviceTokenParamTypeId).toString();
    QByteArray data = QString(token + ":").toUtf8().toBase64();
    QString header = "Basic " + data;
    QNetworkRequest request(QUrl("https://api.awattar.com/v1/marketdata"));
    request.setRawHeader("Authorization", header.toLocal8Bit());
    request.setSslConfiguration(QSslConfiguration::defaultConfiguration());
    QNetworkReply *reply = hardwareManager()->networkManager()->get(request);
    connect(reply, &QNetworkReply::finished, device, [this, reply, device, setupInProgress](){
        reply->deleteLater();

        // check HTTP status code
        int status = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
        if (status != 200) {
            qCWarning(dcAwattar) << "Update reply HTTP error:" << status << reply->errorString();
            if (setupInProgress) {
                emit deviceSetupFinished(device, DeviceManager::DeviceSetupStatusFailure);
            } else {
                device->setStateValue(awattarConnectedStateTypeId, false);
            }
            return;
        }
        // check JSON file
        QJsonParseError error;
        QJsonDocument jsonDoc = QJsonDocument::fromJson(reply->readAll(), &error);
        if (error.error != QJsonParseError::NoError) {
            qCWarning(dcAwattar) << "Update reply JSON error:" << error.errorString();
            if (setupInProgress) {
                emit deviceSetupFinished(device, DeviceManager::DeviceSetupStatusFailure);
            } else {
                device->setStateValue(awattarConnectedStateTypeId, false);
            }
            return;
        }

        emit deviceSetupFinished(device, DeviceManager::DeviceSetupStatusSuccess);
        device->setStateValue(awattarConnectedStateTypeId, true);

        processPriceData(device, jsonDoc.toVariant().toMap());
    });
}


void DevicePluginAwattar::processPriceData(Device *device, const QVariantMap &data)
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

            device->setStateValue(awattarCurrentMarketPriceStateTypeId, currentPrice / 10.0);
            device->setStateValue(awattarValidUntilStateTypeId, endTime.toLocalTime().toTime_t());
        }
    }

    // calculate averagePrice and mean deviation
    averagePrice = sum / count;

    if (currentPrice <= averagePrice) {
        deviation = -1 * qRound(100 + (-100 * (currentPrice - minPrice) / (averagePrice - minPrice)));
    } else {
        deviation = qRound(-100 * (averagePrice - currentPrice) / (maxPrice - averagePrice));
    }

    device->setStateValue(awattarAveragePriceStateTypeId, averagePrice / 10.0);
    device->setStateValue(awattarLowestPriceStateTypeId, minPrice / 10.0);
    device->setStateValue(awattarHighestPriceStateTypeId, maxPrice / 10.0);
    device->setStateValue(awattarAverageDeviationStateTypeId, deviation);
}

