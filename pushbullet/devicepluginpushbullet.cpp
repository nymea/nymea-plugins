/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *                                                                         *
 *  Copyright (C) 2016 Alexander Lampret <alexander.lampret@gmail.com>     *
 *                                                                         *
 *  This file is part of guh.                                              *
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

#include "devicepluginpushbullet.h"
#include "plugin/device.h"
#include "plugininfo.h"
#include "network/networkaccessmanager.h"

DevicePluginPushbullet::DevicePluginPushbullet()
{

}

DeviceManager::DeviceSetupStatus DevicePluginPushbullet::setupDevice(Device *device)
{
    qCDebug(dcPushbullet()) << "Setup" << device->name();

    return DeviceManager::DeviceSetupStatusSuccess;
}

DeviceManager::DeviceError DevicePluginPushbullet::executeAction(Device *device, const Action &action) {
    if (device->deviceClassId() == pushNotificationDeviceClassId) {
        if (action.actionTypeId() == pushNotificationNotifyActionTypeId) {
            QNetworkReply* reply = sendNotification(device, action.params());
            m_asyncActions.insert(reply, action.id());
            return DeviceManager::DeviceErrorAsync;
        }
        return DeviceManager::DeviceErrorActionTypeNotFound;
    }
    return DeviceManager::DeviceErrorDeviceClassNotFound;
}

QNetworkReply* DevicePluginPushbullet::sendNotification(Device* device, ParamList params) {
    QUrlQuery urlParams;
    urlParams.addQueryItem("body", params.paramValue(pushNotificationBodyParamTypeId).toByteArray());
    urlParams.addQueryItem("title", params.paramValue(pushNotificationTitleParamTypeId).toByteArray());
    urlParams.addQueryItem("type", "note");

    QNetworkRequest request(QUrl("https://api.pushbullet.com/v2/pushes"));
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/x-www-form-urlencoded");
    request.setRawHeader(QByteArray("Access-Token"), device->paramValue(pushNotificationTokenParamTypeId).toByteArray());
    QNetworkReply *reply = hardwareManager()->networkManager()->post(request, urlParams.toString(QUrl::FullyEncoded).toUtf8());
    connect(reply, &QNetworkReply::finished, this, &DevicePluginPushbullet::onNetworkReplyFinished);
    return reply;
}

void DevicePluginPushbullet::onNetworkReplyFinished()
{
    QNetworkReply *reply = static_cast<QNetworkReply *>(sender());

    if (m_asyncActions.keys().contains(reply)) {
        ActionId actionId = m_asyncActions.value(reply);
        if (reply->error()) {
            qCWarning(dcPushbullet) << "Pushbullet reply error: " << reply->errorString();
            emit actionExecutionFinished(actionId, DeviceManager::DeviceErrorInvalidParameter);
            reply->deleteLater();
            return;
        }
        if (reply->readAll().contains("error")) {
            emit actionExecutionFinished(actionId, DeviceManager::DeviceErrorHardwareFailure);
        } else {
            emit actionExecutionFinished(actionId, DeviceManager::DeviceErrorNoError);
        }
    }
    reply->deleteLater();
}
