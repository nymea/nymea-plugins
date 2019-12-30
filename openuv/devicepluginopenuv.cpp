/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *                                                                         *
 *  Copyright (C) 2020 Berhard Trinnes <bernhard.trinnes@nymea.io>         *
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

#include "devicepluginopenuv.h"
#include "devices/device.h"
#include "plugininfo.h"

#include <QDebug>
#include <QJsonDocument>
#include <QVariantMap>
#include <QUrl>
#include <QUrlQuery>
#include <QDateTime>
#include <QTimeZone>

DevicePluginOpenUv::DevicePluginOpenUv()
{
}

void DevicePluginOpenUv::init()
{
    m_apiKey = configValue(openUvPluginApiKeyParamTypeId).toByteArray();

    connect(this, &DevicePluginOpenUv::configValueChanged, this, [this]{
        m_apiKey = configValue(openUvPluginApiKeyParamTypeId).toByteArray();
    });
}


void DevicePluginOpenUv::discoverDevices(DeviceDiscoveryInfo *info)
{
    QNetworkRequest request(QUrl("http://ip-api.com/json"));
    QNetworkReply* reply = hardwareManager()->networkManager()->get(request);
    connect(reply, &QNetworkReply::finished, reply, &QNetworkReply::deleteLater);
    connect(reply, &QNetworkReply::finished, info, [this, reply, info]() {
        if (reply->error() != QNetworkReply::NoError) {
            qCWarning(dcOpenUv()) << "Error fetching geolocation from ip-api:" << reply->error() << reply->errorString();
            info->finish(Device::DeviceErrorHardwareFailure, QT_TR_NOOP("Failed to fetch data from the internet."));
            return;
        }
        QJsonParseError error;
        QJsonDocument jsonDoc = QJsonDocument::fromJson(reply->readAll(), &error);
        if (error.error != QJsonParseError::NoError) {
            qCWarning(dcOpenUv()) << "Failed to parse json from ip-api:" << error.error << error.errorString();
            info->finish(Device::DeviceErrorHardwareFailure, QT_TR_NOOP("The server returned unexpected data."));
            return;
        }
        if (!jsonDoc.toVariant().toMap().contains("lat") || !jsonDoc.toVariant().toMap().contains("lon")) {
            qCWarning(dcOpenUv()) << "Reply missing geolocation info" << qUtf8Printable(jsonDoc.toJson(QJsonDocument::Indented));
            info->finish(Device::DeviceErrorHardwareFailure, QT_TR_NOOP("The server returned unexpected data."));
            return;
        }
        qreal lat = jsonDoc.toVariant().toMap().value("lat").toDouble();
        qreal lon = jsonDoc.toVariant().toMap().value("lon").toDouble();

        DeviceDescriptor descriptor(info->deviceClassId(), tr("OpenUV location"), jsonDoc.toVariant().toMap().value("city").toString());
        ParamList params;
        params.append(Param(openUvDeviceLatitudeParamTypeId, lat));
        params.append(Param(openUvDeviceLongitudeParamTypeId, lon));
        descriptor.setParams(params);
        info->addDeviceDescriptor(descriptor);
        info->finish(Device::DeviceErrorNoError);
    });
}

void DevicePluginOpenUv::setupDevice(DeviceSetupInfo *info)
{
    if (m_apiKey.isEmpty() || m_apiKey == "-") {
        info->finish(Device::DeviceErrorSetupFailed, tr("API key is missing, add your OpenUV API key in the plug-in configs"));
        return;
    }

    if (!m_pluginTimer) {
        m_pluginTimer = hardwareManager()->pluginTimerManager()->registerTimer(60 * 60);
        connect(m_pluginTimer, &PluginTimer::timeout, this, &DevicePluginOpenUv::onPluginTimer);
    }
    info->finish(Device::DeviceErrorNoError);
}

void DevicePluginOpenUv::postSetupDevice(Device *device)
{
    getUvIndex(device);
}


void DevicePluginOpenUv::deviceRemoved(Device *device)
{
    Q_UNUSED(device);

    if (myDevices().isEmpty()) {
        hardwareManager()->pluginTimerManager()->unregisterTimer(m_pluginTimer);
        m_pluginTimer = nullptr;
    }
}

void DevicePluginOpenUv::getUvIndex(Device *device)
{
    qCDebug(dcOpenUv()) << "Refresh data for" << device->name();
    QUrl url("https://api.openuv.io/api/v1/uv");
    QUrlQuery query;
    query.addQueryItem("lat", device->paramValue(openUvDeviceLatitudeParamTypeId).toString());
    query.addQueryItem("lng", device->paramValue(openUvDeviceLongitudeParamTypeId).toString());
    url.setQuery(query);
    QNetworkRequest request;
    request.setUrl(url);
    request.setRawHeader("x-access-token", m_apiKey);

    QNetworkReply *reply = hardwareManager()->networkManager()->get(request);
    connect(reply, &QNetworkReply::finished, this, [reply, device, this] {
        reply->deleteLater();
        int status = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();

        // Check HTTP status code
        if (status != 200 || reply->error() != QNetworkReply::NoError) {
            if (reply->error() == QNetworkReply::HostNotFoundError) {
                device->setStateValue(openUvConnectedStateTypeId, false);
            }
            qCWarning(dcOpenUv()) << "Request error:" << status << reply->errorString();
            return;
        }
        device->setStateValue(openUvConnectedStateTypeId, true);

        QJsonParseError error;
        QJsonDocument data = QJsonDocument::fromJson(reply->readAll(), &error);
        if (error.error != QJsonParseError::NoError) {
            qDebug(dcOpenUv()) << "Recieved invalide JSON object";
            return;
        }
        QVariantMap result = data.toVariant().toMap().value("result").toMap();
        device->setStateValue(openUvUvStateTypeId, result["uv"].toDouble());
        device->setStateValue(openUvUvMaxStateTypeId, result["uv_max"].toDouble());
        device->setStateValue(openUvOzoneStateTypeId, result["ozone"].toDouble());

        device->setStateValue(openUvUvTimeStateTypeId, QDateTime::fromString(result["uv_time"].toString(),Qt::DateFormat::ISODateWithMs).toSecsSinceEpoch());
        device->setStateValue(openUvOzoneTimeStateTypeId, QDateTime::fromString(result["ozone_time"].toString(),Qt::DateFormat::ISODateWithMs).toSecsSinceEpoch());
        device->setStateValue(openUvUvMaxTimeStateTypeId, QDateTime::fromString(result["uv_max_time"].toString(),Qt::DateFormat::ISODateWithMs).toSecsSinceEpoch());

        QVariantMap safeExposureTimes = result["safe_exposure_time"].toMap();
        device->setStateValue(openUvSafeExposureTimeSt1StateTypeId, safeExposureTimes["st1"].toInt());
        device->setStateValue(openUvSafeExposureTimeSt2StateTypeId, safeExposureTimes["st2"].toInt());
        device->setStateValue(openUvSafeExposureTimeSt3StateTypeId, safeExposureTimes["st3"].toInt());
        device->setStateValue(openUvSafeExposureTimeSt4StateTypeId, safeExposureTimes["st4"].toInt());
        device->setStateValue(openUvSafeExposureTimeSt5StateTypeId, safeExposureTimes["st5"].toInt());
        device->setStateValue(openUvSafeExposureTimeSt6StateTypeId, safeExposureTimes["st6"].toInt());
    });
}

void DevicePluginOpenUv::onPluginTimer()
{
    foreach (Device *device, myDevices()) {
        getUvIndex(device);
    }
}

