/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *                                                                         *
 *  Copyright (C) 2019 Michael Zanetti <michael.zanetti@nymea.io>          *
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

/*!
    \page pushbullet.html
    \title Pushbullet
    \brief Plugin to send notifications via Pushbullet.

    \ingroup plugins
    \ingroup nymea-plugins

    This plugin allows to send notifications via Pushbullet.

    \chapter Usage

    When setting up a device for the pushbullet service, an API token from Pushbullet is required. Please visit
    https://www.pushbullet.com and create an account if you haven't done so already. After logging in successfully,
    open the "Settings" tab and in "Account Settings", create a new access token. Use that token as the device
    parameter when setting up the device.
*/

#include "devicepluginpushbullet.h"
#include "plugininfo.h"

#include "network/networkaccessmanager.h"

#include <QJsonDocument>

DevicePluginPushbullet::DevicePluginPushbullet(QObject* parent): DevicePlugin (parent)
{

}

DevicePluginPushbullet::~DevicePluginPushbullet()
{

}

DeviceManager::DeviceSetupStatus DevicePluginPushbullet::setupDevice(Device *device)
{
    qCDebug(dcPushbullet()) << "Setting up Pushbullet device" << device->name() << device->id().toString();

    QNetworkRequest  request(QUrl("https://api.pushbullet.com/v2/users/me"));
    request.setRawHeader("Access-Token", device->paramValue(pushNotificationDeviceTokenParamTypeId).toByteArray().trimmed());
    QNetworkReply *reply = hardwareManager()->networkManager()->get(request);
    connect(reply, &QNetworkReply::finished, reply, &QNetworkReply::deleteLater);
    connect(reply, &QNetworkReply::finished, device, [this, reply, device]() {
        if (reply->error() != QNetworkReply::NoError) {
            qCWarning(dcPushbullet()) << "Error fetching user profile:" << reply->errorString() << reply->error();
            emit deviceSetupFinished(device, DeviceManager::DeviceSetupStatusFailure);
            return;
        }

        QByteArray data = reply->readAll();

        QJsonParseError error;
        QJsonDocument jsonDoc = QJsonDocument::fromJson(data, &error);
        if (error.error != QJsonParseError::NoError) {
            qCWarning(dcPushbullet()) << "Error parsing reply from Pushbullet:" << error.errorString();
            qCWarning(dcPushbullet()) << qUtf8Printable(data);
            emit deviceSetupFinished(device, DeviceManager::DeviceSetupStatusFailure);
            return;
        }

        QVariantMap replyMap = jsonDoc.toVariant().toMap();
        if (!replyMap.value("active").toBool()) {
            qCWarning(dcPushbullet()) << "Pushbullet account seems to be deactivated.";
            emit deviceSetupFinished(device, DeviceManager::DeviceSetupStatusFailure);
            return;
        }

        qCDebug(dcPushbullet()) << "Pushbullet device" << device->name() << device->id().toString() << "setup complete";
        emit deviceSetupFinished(device, DeviceManager::DeviceSetupStatusSuccess);
    });

    return DeviceManager::DeviceSetupStatusAsync;
}

DeviceManager::DeviceError DevicePluginPushbullet::executeAction(Device *device, const Action &action)
{
    qCDebug(dcPushbullet()) << "Executing action" << action.actionTypeId() << "for device" << device->name() << device->id().toString();

    QNetworkRequest request(QUrl("https://api.pushbullet.com/v2/pushes"));
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    request.setRawHeader("Access-Token", device->paramValue(pushNotificationDeviceTokenParamTypeId).toByteArray().trimmed());

    QVariantMap params;
    params.insert("type", "note");
    params.insert("title", action.param(pushNotificationNotifyActionTitleParamTypeId).value().toString());
    params.insert("body", action.param(pushNotificationNotifyActionBodyParamTypeId).value().toString());
    QNetworkReply *reply = hardwareManager()->networkManager()->post(request, QJsonDocument::fromVariant(params).toJson(QJsonDocument::Compact));
    connect(reply, &QNetworkReply::finished, reply, &QNetworkReply::deleteLater);
    connect(reply, &QNetworkReply::finished, device, [this, reply, device, action]{
        if (reply->error() != QNetworkReply::NoError) {
            qCWarning(dcPushbullet()) << "Push message sending failed for" << device->name() << device->id() << reply->errorString() << reply->error();
            emit actionExecutionFinished(action.id(), DeviceManager::DeviceErrorHardwareNotAvailable);
            return;
        }

        QByteArray data = reply->readAll();

        QJsonParseError error;
        QJsonDocument jsonDoc = QJsonDocument::fromJson(data, &error);
        if (error.error != QJsonParseError::NoError) {
            qCWarning(dcPushbullet()) << "Error reading reply from Pushbullet for" << device->name() << device->id().toString() << error.errorString();
            qCWarning(dcPushbullet()) << qUtf8Printable(data);
            emit actionExecutionFinished(action.id(), DeviceManager::DeviceErrorHardwareFailure);
            return;
        }

        QVariantMap replyMap = jsonDoc.toVariant().toMap();
        if (!replyMap.value("active").toBool()) {
            qCWarning(dcPushbullet()) << "Error sending message. The account seems to be deactivated" << device->name() << device->id().toString();
            emit actionExecutionFinished(action.id(), DeviceManager::DeviceErrorHardwareFailure);
            return;
        }

        if (replyMap.value("dismissed", true).toBool()) {
            qCWarning(dcPushbullet()) << "Error sending message. The message has been dismissed by the server" << device->name() << device->id().toString();
            emit actionExecutionFinished(action.id(), DeviceManager::DeviceErrorHardwareFailure);
            return;
        }

        qCDebug(dcPushbullet()) << "Message sent successfully";
        emit actionExecutionFinished(action.id(), DeviceManager::DeviceErrorNoError);
    });

    return DeviceManager::DeviceErrorAsync;
}

