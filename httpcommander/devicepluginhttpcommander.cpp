/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *                                                                         *
 *  Copyright (C) 2017 Bernhard Trinnes <bernhard.trinnes@guh.io>          *
 *  Copyright (C) 2018 Simon Stürz <simon.stuerz@guh.io>                   *
 *                                                                         *
 *  This file is part of guh.                                              *
 *                                                                         *
 *  Guh is free software: you can redistribute it and/or modify            *
 *  it under the terms of the GNU General Public License as published by   *
 *  the Free Software Foundation, version 2 of the License.                *
 *                                                                         *
 *  Guh is distributed in the hope that it will be useful,                 *
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of         *
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the           *
 *  GNU General Public License for more details.                           *
 *                                                                         *
 *  You should have received a copy of the GNU General Public License      *
 *  along with guh. If not, see <http://www.gnu.org/licenses/>.            *
 *                                                                         *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

#include "devicepluginhttpcommander.h"
#include "network/networkaccessmanager.h"
#include "plugininfo.h"


DevicePluginHttpCommander::DevicePluginHttpCommander()
{
}

DevicePluginHttpCommander::~DevicePluginHttpCommander()
{
    hardwareManager()->pluginTimerManager()->unregisterTimer(m_pluginTimer);
}

void DevicePluginHttpCommander::init()
{
    m_pluginTimer = hardwareManager()->pluginTimerManager()->registerTimer(10);
    connect(m_pluginTimer, &PluginTimer::timeout, this, &DevicePluginHttpCommander::onPluginTimer);
}

DeviceManager::DeviceSetupStatus DevicePluginHttpCommander::setupDevice(Device *device)
{
    qDebug(dcHttpCommander()) << "Setup device" << device->name() << device->params();

    // Get
    if (device->deviceClassId() == httpGetCommanderDeviceClassId) {
        QUrl url = device->paramValue(httpGetCommanderUrlParamTypeId).toUrl();
        if (!url.isValid()) {
            qDebug(dcHttpCommander()) << "Given URL is not valid";
            return DeviceManager::DeviceSetupStatusFailure;
        }

        return DeviceManager::DeviceSetupStatusSuccess;
    }

    // Put
    if (device->deviceClassId() == httpPutCommanderDeviceClassId) {
        QUrl url = device->paramValue(httpPutCommanderUrlParamTypeId).toUrl();
        if (!url.isValid()) {
            qDebug(dcHttpCommander()) << "Given URL is not valid";
            return DeviceManager::DeviceSetupStatusFailure;
        }

        return DeviceManager::DeviceSetupStatusSuccess;
    }

    // Post
    if (device->deviceClassId() == httpPostCommanderDeviceClassId) {
        QUrl url = device->paramValue(httpPostCommanderUrlParamTypeId).toUrl();
        if (!url.isValid()) {
            qDebug(dcHttpCommander()) << "Given URL is not valid";
            return DeviceManager::DeviceSetupStatusFailure;
        }

        return DeviceManager::DeviceSetupStatusSuccess;
    }

    return DeviceManager::DeviceSetupStatusFailure;
}


void  DevicePluginHttpCommander::postSetupDevice(Device *device)
{
    if (device->deviceClassId() == httpGetCommanderDeviceClassId) {
        makeGetCall(device);
    }

    if (device->deviceClassId() == httpPostCommanderDeviceClassId) {
        //TODO find a way to check it the URL is reachable
        device->setStateValue(httpPostCommanderConnectedStateTypeId, true);
    }

    if (device->deviceClassId() == httpPutCommanderDeviceClassId) {
        //TODO find a way to check it the URL is reachable
        device->setStateValue(httpPutCommanderConnectedStateTypeId, true);
    }
}


DeviceManager::DeviceError DevicePluginHttpCommander::executeAction(Device *device, const Action &action)
{
    if (device->deviceClassId() == httpPostCommanderDeviceClassId) {

        if (action.actionTypeId() == httpPostCommanderPostActionTypeId) {
            QUrl url = device->paramValue(httpPostCommanderUrlParamTypeId).toUrl();
            url.setPort(device->paramValue(httpPostCommanderPortParamTypeId).toInt());
            QByteArray payload = action.param(httpPostCommanderDataParamTypeId).value().toByteArray();

            QNetworkReply *reply = hardwareManager()->networkManager()->post(QNetworkRequest(url), payload);
            connect(reply, &QNetworkReply::finished, this, &DevicePluginHttpCommander::onPostRequestFinished);

            m_httpRequests.insert(reply, device);

            return DeviceManager::DeviceErrorNoError;
        }
        return DeviceManager::DeviceErrorActionTypeNotFound;

    }

    if (device->deviceClassId() == httpPutCommanderDeviceClassId) {

        // check if this is the "press" action
        if (action.actionTypeId() == httpPutCommanderPutActionTypeId) {

            QUrl url = device->paramValue(httpPutCommanderUrlParamTypeId).toUrl();
            url.setPort(device->paramValue(httpPutCommanderPortParamTypeId).toInt());
            QByteArray payload = action.param(httpPutCommanderDataParamTypeId).value().toByteArray();
            QNetworkReply *reply = hardwareManager()->networkManager()->put(QNetworkRequest(url), payload);
            connect(reply, &QNetworkReply::finished, this, &DevicePluginHttpCommander::onPutRequestFinished);

            m_httpRequests.insert(reply, device);

            return DeviceManager::DeviceErrorNoError;
        }
    }
    return DeviceManager::DeviceErrorDeviceClassNotFound;
}

void DevicePluginHttpCommander::makeGetCall(Device *device)
{
    QUrl url = device->paramValue(httpGetCommanderUrlParamTypeId).toUrl();
    url.setPort(device->paramValue(httpGetCommanderPortParamTypeId).toInt());
    QNetworkRequest request;
    request.setUrl(url);
    request.setRawHeader("User-Agent", "guhIO 1.0");

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

    // Check HTTP status code
    if (status != 200 || reply->error() != QNetworkReply::NoError) {
        qCWarning(dcHttpCommander()) << "Request error:" << status << reply->errorString();
        device->setStateValue(httpGetCommanderConnectedStateTypeId, false);
        reply->deleteLater();
        return;
    }

    device->setStateValue(httpGetCommanderConnectedStateTypeId, true);
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

    // Check HTTP status code
    if (status != 200 || reply->error() != QNetworkReply::NoError) {
        qCWarning(dcHttpCommander()) << "Request error:" << status << reply->errorString();
        device->setStateValue(httpPostCommanderConnectedStateTypeId, false);
        reply->deleteLater();
        return;
    }

    device->setStateValue(httpPostCommanderConnectedStateTypeId, true);
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

    // Check HTTP status code
    if (status != 200 || reply->error() != QNetworkReply::NoError) {
        qCWarning(dcHttpCommander()) << "Request error:" << status << reply->errorString();
        device->setStateValue(httpPutCommanderConnectedStateTypeId, false);
        reply->deleteLater();
        return;
    }

    device->setStateValue(httpPutCommanderConnectedStateTypeId, true);
    reply->deleteLater();
}


void DevicePluginHttpCommander::deviceRemoved(Device *device)
{
    if (m_httpRequests.values().contains(device)) {
        QNetworkReply *reply = m_httpRequests.key(device);
        m_httpRequests.remove(reply);
        // Note: will be deleted once finished
    }
}

