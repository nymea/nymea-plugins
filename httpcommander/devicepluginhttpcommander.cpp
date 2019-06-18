/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *                                                                         *
 *  Copyright (C) 2019 Bernhard Trinnes <bernhard.trinnes@nymea.io>        *
 *  Copyright (C) 2018 Simon Stürz <simon.stuerz@nymea.io>                 *
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

#include "devicepluginhttpcommander.h"
#include "network/networkaccessmanager.h"
#include "plugininfo.h"


/*!
    \page httpcommander.html
    \title HTTP commander
    \brief Plugin for generic HTTP commands
    \ingroup plugins
    \ingroup nymea-plugins
    This plug-in supports generic HTTP calls like get, put, post
    \chapter Plugin properties
    Following JSON file contains the definition and the description of all available \l{DeviceClass}{DeviceClasses}
    and \l{Vendor}{Vendors} of this \l{DevicePlugin}.
    For more details how to read this JSON file please check out the documentation for \l{The plugin JSON File}.
    \quotefile plugins/deviceplugins/denon/devicepluginhttpcommander.json
*/

DevicePluginHttpCommander::DevicePluginHttpCommander()
{
}

Device::DeviceSetupStatus DevicePluginHttpCommander::setupDevice(Device *device)
{
    qDebug(dcHttpCommander()) << "Setup device" << device->name() << device->params();

    if(!m_pluginTimer) {
        m_pluginTimer = hardwareManager()->pluginTimerManager()->registerTimer(60);
        connect(m_pluginTimer, &PluginTimer::timeout, this, &DevicePluginHttpCommander::onPluginTimer);
    }

    if (device->deviceClassId() == httpGetCommanderDeviceClassId) {
        QUrl url = device->paramValue(httpGetCommanderDeviceUrlParamTypeId).toUrl();
        if (!url.isValid()) {
            qDebug(dcHttpCommander()) << "Given URL is not valid";
            return Device::DeviceSetupStatusFailure;
        }

        return Device::DeviceSetupStatusSuccess;
    }

    if (device->deviceClassId() == httpPutCommanderDeviceClassId) {
        QUrl url = device->paramValue(httpPutCommanderDeviceUrlParamTypeId).toUrl();
        if (!url.isValid()) {
            qDebug(dcHttpCommander()) << "Given URL is not valid";
            return Device::DeviceSetupStatusFailure;
        }
        return Device::DeviceSetupStatusSuccess;
    }

    if (device->deviceClassId() == httpPostCommanderDeviceClassId) {
        QUrl url = device->paramValue(httpPostCommanderDeviceUrlParamTypeId).toUrl();
        if (!url.isValid()) {
            qDebug(dcHttpCommander()) << "Given URL is not valid";
            return Device::DeviceSetupStatusFailure;
        }
        return Device::DeviceSetupStatusSuccess;
    }
    return Device::DeviceSetupStatusFailure;
}


void  DevicePluginHttpCommander::postSetupDevice(Device *device)
{
    if (device->deviceClassId() == httpGetCommanderDeviceClassId) {
        makeGetCall(device);
    }
}


Device::DeviceError DevicePluginHttpCommander::executeAction(Device *device, const Action &action)
{
    if (device->deviceClassId() == httpPostCommanderDeviceClassId) {

        if (action.actionTypeId() == httpPostCommanderPostActionTypeId) {
            QUrl url = device->paramValue(httpPostCommanderDeviceUrlParamTypeId).toUrl();
            url.setPort(device->paramValue(httpPostCommanderDevicePortParamTypeId).toInt());
            QByteArray payload = action.param(httpPostCommanderPostActionDataParamTypeId).value().toByteArray();

            QNetworkReply *reply = hardwareManager()->networkManager()->post(QNetworkRequest(url), payload);
            connect(reply, &QNetworkReply::finished, this, &DevicePluginHttpCommander::onPostRequestFinished);

            m_httpRequests.insert(reply, device);

            return Device::DeviceErrorNoError;
        }
        return Device::DeviceErrorActionTypeNotFound;

    }

    if (device->deviceClassId() == httpPutCommanderDeviceClassId) {

        // check if this is the "press" action
        if (action.actionTypeId() == httpPutCommanderPutActionTypeId) {
            QUrl url = device->paramValue(httpPutCommanderDeviceUrlParamTypeId).toUrl();
            url.setPort(device->paramValue(httpPutCommanderDevicePortParamTypeId).toInt());
            QByteArray payload = action.param(httpPutCommanderPutActionDataParamTypeId).value().toByteArray();

            QNetworkReply *reply = hardwareManager()->networkManager()->put(QNetworkRequest(url), payload);
            connect(reply, &QNetworkReply::finished, this, &DevicePluginHttpCommander::onPutRequestFinished);

            m_httpRequests.insert(reply, device);

            return Device::DeviceErrorNoError;
        }
    }
    return Device::DeviceErrorDeviceClassNotFound;
}

void DevicePluginHttpCommander::makeGetCall(Device *device)
{
    QUrl url = device->paramValue(httpGetCommanderDeviceUrlParamTypeId).toUrl();
    url.setPort(device->paramValue(httpGetCommanderDevicePortParamTypeId).toInt());
    QNetworkRequest request;
    request.setUrl(url);
    request.setRawHeader("User-Agent", "nymea 1.0");

    QNetworkReply *reply = hardwareManager()->networkManager()->get(request);
    connect(reply, &QNetworkReply::finished, this, &DevicePluginHttpCommander::onGetRequestFinished);

    m_httpRequests.insert(reply, device);
}

void DevicePluginHttpCommander::onPluginTimer()
{
    foreach (Device *device, myDevices()) {
        if (device->deviceClassId() == httpGetCommanderDeviceClassId) {
            makeGetCall(device);
        }
    }
}

void DevicePluginHttpCommander::onGetRequestFinished()
{
    QNetworkReply *reply = static_cast<QNetworkReply *>(sender());
    qDebug(dcHttpCommander()) << "GET reply finished";
    QByteArray data = reply->readAll();
    int status = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();

    if (!m_httpRequests.contains(reply)) {
        reply->deleteLater();
        return;
    }

    Device *device = m_httpRequests.take(reply);
    device->setStateValue(httpGetCommanderResponseStateTypeId, data);
    device->setStateValue(httpGetCommanderStatusStateTypeId, true);

    // Check HTTP status code
    if (status != 200 || reply->error() != QNetworkReply::NoError) {
        qCWarning(dcHttpCommander()) << "Request error:" << status << reply->errorString();
    }
    reply->deleteLater();
}

void DevicePluginHttpCommander::onPostRequestFinished()
{
    QNetworkReply *reply = static_cast<QNetworkReply *>(sender());
    qDebug(dcHttpCommander()) << "POST reply finished";
    QByteArray data = reply->readAll();
    int status = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();

    if (!m_httpRequests.contains(reply)) {
        reply->deleteLater();
        return;
    }

    Device *device = m_httpRequests.take(reply);
    device->setStateValue(httpPostCommanderResponseStateTypeId, data);
    device->setStateValue(httpPostCommanderStatusStateTypeId, status);

    // Check HTTP status code
    if (status != 200 || reply->error() != QNetworkReply::NoError) {
        qCWarning(dcHttpCommander()) << "Request error:" << status << reply->errorString();
    }
    reply->deleteLater();
}

void DevicePluginHttpCommander::onPutRequestFinished()
{
    QNetworkReply *reply = static_cast<QNetworkReply *>(sender());
    qDebug(dcHttpCommander()) << "PUT reply finished";
    QByteArray data = reply->readAll();
    int status = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();

    if (!m_httpRequests.contains(reply)) {
        reply->deleteLater();
        return;
    }

    Device *device = m_httpRequests.take(reply);
    device->setStateValue(httpPutCommanderResponseStateTypeId, data);
    device->setStateValue(httpPutCommanderStatusStateTypeId, status);

    // Check HTTP status code
    if (status != 200 || reply->error() != QNetworkReply::NoError) {
        qCWarning(dcHttpCommander()) << "Request error:" << status << reply->errorString();
    }
    reply->deleteLater();
}


void DevicePluginHttpCommander::deviceRemoved(Device *device)
{
    if ((device->deviceClassId() == httpPostCommanderDeviceClassId) ||
            (device->deviceClassId() == httpPutCommanderDeviceClassId) ||
            (device->deviceClassId() == httpGetCommanderDeviceClassId)) {

        while (m_httpRequests.values().contains(device)) {
            QNetworkReply *reply = m_httpRequests.key(device);
            m_httpRequests.remove(reply);
            reply->deleteLater();
        }
    }

    if (myDevices().empty()) {
        hardwareManager()->pluginTimerManager()->unregisterTimer(m_pluginTimer);
    }
}
