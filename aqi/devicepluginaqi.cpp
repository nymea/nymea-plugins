/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *                                                                         *
 *  Copyright (C) 2020 Bernhard Trinnes <bernhard.trinnes@nymea.io>        *
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

#include "devicepluginaqi.h"
#include "plugininfo.h"

#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QUrlQuery>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>

DevicePluginAqi::DevicePluginAqi()
{

}

void DevicePluginAqi::setupDevice(DeviceSetupInfo *info)
{
    if (info->device()->deviceClassId() == airQualityIndexDeviceClassId) {

        if (myDevices().filterByDeviceClassId(info->device()->deviceClassId()).count() > 1) {
            info->finish(Device::DeviceErrorSetupFailed, tr("Service is already in use"));
            return;
        }

        QUrl url;
        url.setUrl(m_baseUrl);
        url.setPath("/feed/here/");
        url.setQuery("token="+configValue(airQualityIndexPluginApiKeyParamTypeId).toString());
        QNetworkRequest request;
        request.setUrl(url);
        request.setRawHeader("User-Agent", "nymea 1.0");

        QNetworkReply *reply = hardwareManager()->networkManager()->get(request);
        connect(reply, &QNetworkReply::finished, this, [reply, info, this] {
            reply->deleteLater();
            int status = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();

            // Check HTTP status code
            if (status != 200 || reply->error() != QNetworkReply::NoError) {
                if (status == 400 || status == 401) {

                }
                qCWarning(dcAirQualityIndex()) << "Request error:" << status << reply->errorString();
                return info->finish(Device::DeviceErrorSetupFailed, reply->errorString());
            }
            return info->finish(Device::DeviceErrorNoError);
        });
    }
}


void DevicePluginAqi::postSetupDevice(Device *device)
{
    Q_UNUSED(device);
    getDataByIp();

    if(!m_pluginTimer) {
        m_pluginTimer = hardwareManager()->pluginTimerManager()->registerTimer(60);
        connect(m_pluginTimer, &PluginTimer::timeout, this, &DevicePluginAqi::onPluginTimer);
    }
}


void DevicePluginAqi::deviceRemoved(Device *device)
{
    Q_UNUSED(device);

    if (myDevices().empty()) {
        hardwareManager()->pluginTimerManager()->unregisterTimer(m_pluginTimer);
        m_pluginTimer = nullptr;
    }
}


void DevicePluginAqi::getDataByIp()
{
    QUrl url;
    url.setUrl(m_baseUrl);
    url.setPath("/feed/here/");
    url.setQuery("token="+configValue(airQualityIndexPluginApiKeyParamTypeId).toString());
    QNetworkRequest request;
    request.setUrl(url);
    request.setRawHeader("User-Agent", "nymea");

    QNetworkReply *reply = hardwareManager()->networkManager()->get(request);
    connect(reply, &QNetworkReply::finished, this, [reply, this] {
        reply->deleteLater();
        int status = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();

        // Check HTTP status code
        if (status != 200 || reply->error() != QNetworkReply::NoError) {
            foreach (Device *device, myDevices().filterByDeviceClassId(airQualityIndexDeviceClassId)) {
                device->setStateValue(airQualityIndexConnectedStateTypeId, true);
            }
            qCWarning(dcAirQualityIndex()) << "Request error:" << status << reply->errorString();
            return;
        }
        QJsonParseError error;
        QJsonDocument data = QJsonDocument::fromJson(reply->readAll(), &error);
        if (error.error != QJsonParseError::NoError) {
            qDebug(dcAirQualityIndex()) << "Received invalide JSON object";
            return;
        }
        qCDebug(dcAirQualityIndex()) << data.toJson();
        QVariantMap city = data.toVariant().toMap().value("data").toMap().value("city").toMap();
        //double lat = city["geo"].toList().takeAt(0).toDouble();
        //double lng = city["geo"].toList().takeAt(1).toDouble();
        QString name = city["name"].toString();
        QString url = city["url"].toString();

        QVariantMap iaqi = data.toVariant().toMap().value("data").toMap().value("iaqi").toMap();
        double humidity = iaqi["h"].toMap().value("v").toDouble();
        double pressure = iaqi["p"].toMap().value("v").toDouble();
        int pm25 = iaqi["pm25"].toMap().value("v").toInt();
        int pm10 = iaqi["pm10"].toMap().value("v").toInt();
        double so2 = iaqi["so2"].toMap().value("v").toDouble();
        double no2 = iaqi["no2"].toMap().value("v").toDouble();
        double o3 = iaqi["o3"].toMap().value("v").toDouble();
        double co = iaqi["co"].toMap().value("v").toDouble();
        double temperature = iaqi["t"].toMap().value("v").toDouble();

        foreach (Device *device, myDevices().filterByDeviceClassId(airQualityIndexDeviceClassId)) {
            device->setStateValue(airQualityIndexStationNameStateTypeId, name);
            device->setStateValue(airQualityIndexConnectedStateTypeId, true);
            device->setStateValue(airQualityIndexCoStateTypeId, co);
            device->setStateValue(airQualityIndexHumidityStateTypeId, humidity);
            device->setStateValue(airQualityIndexTemperatureStateTypeId, temperature);
            device->setStateValue(airQualityIndexPressureStateTypeId, pressure);
            device->setStateValue(airQualityIndexO3StateTypeId, o3);
            device->setStateValue(airQualityIndexNo2StateTypeId, no2);
            device->setStateValue(airQualityIndexSo2StateTypeId, so2);
            device->setStateValue(airQualityIndexPm10StateTypeId, pm10);
            device->setStateValue(airQualityIndexPm25StateTypeId, pm25);

            if (pm25 <= 50.00) {
                device->setStateValue(airQualityIndexAirQualityStateTypeId, "Good");
                device->setStateValue(airQualityIndexCautionaryStatementStateTypeId, "None");
            } else if ((pm25 > 50.00) && (pm25 <= 100.00)) {
                device->setStateValue(airQualityIndexAirQualityStateTypeId, "Moderate");
                device->setStateValue(airQualityIndexCautionaryStatementStateTypeId, "Active children and adults, and people with respiratory disease, such as asthma, should limit prolonged outdoor exertion.");
            } else if ((pm25 > 100.00) && (pm25 <= 150.00)) {
                device->setStateValue(airQualityIndexAirQualityStateTypeId, "Unhealthy for Sensitive Groups");
                device->setStateValue(airQualityIndexCautionaryStatementStateTypeId, "Active children and adults, and people with respiratory disease, such as asthma, should limit prolonged outdoor exertion.");
            }  else if ((pm25 > 150.00) && (pm25 <= 200.00)) {
                device->setStateValue(airQualityIndexAirQualityStateTypeId, "Unhealthy");
                device->setStateValue(airQualityIndexCautionaryStatementStateTypeId, "Active children and adults, and people with respiratory disease, such as asthma, should avoid prolonged outdoor exertion; everyone else, especially children, should limit prolonged outdoor exertion");
            } else if ((pm25 > 200.00) && (pm25 <= 300.00)) {
                device->setStateValue(airQualityIndexAirQualityStateTypeId, "Very Unhealthy");
                device->setStateValue(airQualityIndexCautionaryStatementStateTypeId, "Active children and adults, and people with respiratory disease, such as asthma, should avoid all outdoor exertion; everyone else, especially children, should limit outdoor exertion.");
            } else {
                device->setStateValue(airQualityIndexAirQualityStateTypeId, "Hazardous");
                device->setStateValue(airQualityIndexCautionaryStatementStateTypeId, "Everyone should avoid all outdoor exertion");
            }
        }
    });
}

void DevicePluginAqi::onPluginTimer()
{
    getDataByIp();
}



