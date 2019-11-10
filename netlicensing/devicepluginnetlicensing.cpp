/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *                                                                         *
 *  Copyright (C) 2019 Bernhard Trinnes <bernhard.trinnes@nymea.io>        *
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

#include "plugininfo.h"
#include "devicepluginnetlicensing.h"
#include "devices/device.h"
#include "network/networkaccessmanager.h"

#include <QDebug>
#include <QUrlQuery>
#include <QJsonDocument>

DevicePluginNetlicensing::DevicePluginNetlicensing()
{

}

void DevicePluginNetlicensing::init()
{

}

void DevicePluginNetlicensing::setupDevice(DeviceSetupInfo *info)
{
    Device *device = info->device();
    Q_UNUSED(device)
    QString licenseeNumber = QSysInfo::machineUniqueId();
    validateLicensee(licenseeNumber);
    qCWarning(dcNetlicensing()) << "Unhandled device class in setupDevice";
}

void DevicePluginNetlicensing::deviceRemoved(Device *device)
{
    Q_UNUSED(device)
    if (myDevices().isEmpty() && m_pluginTimer) {
        m_pluginTimer->deleteLater();
        m_pluginTimer = nullptr;
    }
}

void DevicePluginNetlicensing::postSetupDevice(Device *device)
{
    Q_UNUSED(device)
    if(!m_pluginTimer) {
        m_pluginTimer = hardwareManager()->pluginTimerManager()->registerTimer(60);
        connect(m_pluginTimer, &PluginTimer::timeout, this, &DevicePluginNetlicensing::onPluginTimer);
    }
}

void DevicePluginNetlicensing::validateLicensee(const QString &licenseeNumber)
{
    QUrl url = m_baseUrl + "/core/v2/rest/licensee/" + licenseeNumber;
    url.setPort(443);
    url.setUserName("apiKey");
    url.setPassword("4ec3e47c-7f1c-4de6-924d-ba502088162d");

    QNetworkRequest request;
    request.setUrl(url);
    request.setHeader(QNetworkRequest::KnownHeaders::ContentTypeHeader, "application/x-www-form-urlencoded");
    request.setRawHeader("Accept", "application/json");

    QNetworkReply *reply = hardwareManager()->networkManager()->get(request);
    connect(reply, &QNetworkReply::finished, this, [reply, this]() {
        reply->deleteLater();

        int status = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
        // Check HTTP status code
        if (status != 200 || reply->error() != QNetworkReply::NoError) {
            qCWarning(dcNetlicensing()) << "Request error:" << status << reply->errorString();
        }
    });
}

void DevicePluginNetlicensing::onPluginTimer()
{

}

void DevicePluginNetlicensing::onNetworkReplyFinished()
{
    QNetworkReply *reply = static_cast<QNetworkReply *>(sender());
    reply->deleteLater();

    int status = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
    Q_UNUSED(status)
}
