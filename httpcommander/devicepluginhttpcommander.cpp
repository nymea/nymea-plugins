/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *                                                                         *
 *  Copyright (C) 2017 Bernhard Trinnes <bernhard.trinnes@guh.io>          *
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
#include "plugininfo.h"


DevicePluginHttpCommander::DevicePluginHttpCommander()
{
}

DeviceManager::HardwareResources DevicePluginHttpCommander::requiredHardware() const
{
    return DeviceManager::HardwareResourceNetworkManager | DeviceManager::HardwareResourceTimer;
}


DeviceManager::DeviceSetupStatus DevicePluginHttpCommander::setupDevice(Device *device)
{
    if (device->deviceClassId() == httpGetDeviceClassId) {
        return DeviceManager::DeviceSetupStatusSuccess;
    }

    if (device->deviceClassId() == httpPostDeviceClassId) {
        return DeviceManager::DeviceSetupStatusSuccess;
    }
    return DeviceManager::DeviceSetupStatusFailure;
}


void  DevicePluginHttpCommander::postSetupDevice(Device *device)
{
    if (device->deviceClassId() == httpGetDeviceClassId) {
        QUrl url = device->paramValue(urlParamTypeId).toUrl();
        url.setPort(device->paramValue(portParamTypeId).toInt());
        QNetworkRequest request;
        request.setUrl(url);
        request.setRawHeader("User-Agent", "guhIO 1.0");
        QNetworkReply *reply = networkManagerGet(request);;
        m_httpRequests.insert(reply, device);
    }
}


DeviceManager::DeviceError DevicePluginHttpCommander::executeAction(Device *device, const Action &action)
{
    if (device->deviceClassId() == httpPostDeviceClassId) {

        // check if this is the "press" action
        if (action.actionTypeId() == postActionTypeId) {

            QUrl url = device->paramValue(urlParamTypeId).toUrl();
            url.setPort(device->paramValue(portParamTypeId).toInt());
            QByteArray payload = action.param(postDataParamTypeId).value().toByteArray();
            QNetworkReply *reply = networkManagerPost(QNetworkRequest(url), payload);
            m_httpRequests.insert(reply, device);

            return DeviceManager::DeviceErrorNoError;
        }
        return DeviceManager::DeviceErrorActionTypeNotFound;

    }
    return DeviceManager::DeviceErrorDeviceClassNotFound;
}


void DevicePluginHttpCommander::deviceRemoved(Device *device)
{
    Q_UNUSED(device);
}


void DevicePluginHttpCommander::guhTimer()
{

    foreach (Device *device, myDevices()) {

        if (device->deviceClassId() == httpGetDeviceClassId) {
            QUrl url = device->paramValue(urlParamTypeId).toUrl();
            url.setPort(device->paramValue(portParamTypeId).toInt());
            QNetworkRequest request;
            request.setUrl(url);
            request.setRawHeader("User-Agent", "guhIO 1.0");
            QNetworkReply *reply = networkManagerGet(request);;
            m_httpRequests.insert(reply, device);
        }
    }

}


void DevicePluginHttpCommander::networkManagerReplyReady(QNetworkReply *reply)
{
    QByteArray data = reply->readAll();
    qDebug(dcHttpCommander()) << "Reply received";
    int status = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();

    if (m_httpRequests.contains(reply)) {
        Device *device = m_httpRequests.take(reply);

        if (device->deviceClassId() == httpGetDeviceClassId) {
            device->setStateValue(httpResponseStateTypeId, data);
            //device->setStateValue(statusStateTypeId, reply->attribute(QNetworkRequest::HttpReasonPhraseAttribute).toString());
            // check HTTP status code
            if (status != 200 || reply->error() != QNetworkReply::NoError) {
                qCWarning(dcHttpCommander()) << "Request error:" << status << reply->errorString();
                device->setStateValue(reachableStateTypeId, false);
                reply->deleteLater();
                return;
            }
            device->setStateValue(reachableStateTypeId, true);
        } else if (device->deviceClassId() == httpPostDeviceClassId) {

            // check HTTP status code
            if (status != 200 || reply->error() != QNetworkReply::NoError) {
                qCWarning(dcHttpCommander()) << "Request error:" << status << reply->errorString();
                device->setStateValue(reachableStateTypeId, false);
                reply->deleteLater();
                return;
            }
            device->setStateValue(reachableStateTypeId, true);
        }
    }
    reply->deleteLater();
}

