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

void DevicePluginPushbullet::setupDevice(DeviceSetupInfo *info)
{
    Device *device = info->device();

    qCDebug(dcPushbullet()) << "Setting up Pushbullet device" << device->name() << device->id().toString();

    QString token;

    // TODO: Activate this and drop the token param from json when maveo is ready for it.
//    pluginStorage()->beginGroup(info->device()->id().toString());
//    token = pluginStorage()->value("token").toString();
//    pluginStorage()->endGroup();

    if (token.isEmpty()) {
        // Check for legacy token in device params...
        ParamTypeId pushNotificationDeviceTokenParamTypeId = ParamTypeId("{1f37ce22-50b8-4183-8b74-557fdb2d3076}");
        token = device->paramValue(pushNotificationDeviceTokenParamTypeId).toString();
    }

    if (token.isEmpty()) {
        //: Error setting up device
        return info->finish(Device::DeviceErrorAuthenticationFailure, QT_TR_NOOP("The provided token must not be empty."));
    }

    QNetworkRequest  request(QUrl("https://api.pushbullet.com/v2/users/me"));
    request.setRawHeader("Access-Token", token.toUtf8().trimmed());

    QNetworkReply *reply = hardwareManager()->networkManager()->get(request);
    connect(reply, &QNetworkReply::finished, reply, &QNetworkReply::deleteLater);

    connect(reply, &QNetworkReply::finished, info, [reply, info]() {
        if (reply->error() == QNetworkReply::AuthenticationRequiredError) {
            //: Error setting up device
            info->finish(Device::DeviceErrorAuthenticationFailure, QT_TR_NOOP("The provided token is not valid."));
            return;
        }
        if (reply->error() != QNetworkReply::NoError) {
            qCWarning(dcPushbullet()) << "Error fetching user profile:" << reply->errorString() << reply->error();
            //: Error setting up device
            info->finish(Device::DeviceErrorHardwareFailure, QT_TR_NOOP("Error connecting to Pushbullet."));
            return;
        }

        QByteArray data = reply->readAll();

        QJsonParseError error;
        QJsonDocument jsonDoc = QJsonDocument::fromJson(data, &error);
        if (error.error != QJsonParseError::NoError) {
            qCWarning(dcPushbullet()) << "Error parsing reply from Pushbullet:" << error.errorString();
            qCWarning(dcPushbullet()) << qUtf8Printable(data);
            //: Error setting up device
            info->finish(Device::DeviceErrorHardwareFailure, QT_TR_NOOP("Received unexpected data from Pushbullet."));
            return;
        }

        QVariantMap replyMap = jsonDoc.toVariant().toMap();
        if (!replyMap.value("active").toBool()) {
            qCWarning(dcPushbullet()) << "Pushbullet account seems to be deactivated.";
            //: Error setting up device
            info->finish(Device::DeviceErrorAuthenticationFailure, QT_TR_NOOP("The Pushbullet account seems to be disabled."));
            return;
        }

        qCDebug(dcPushbullet()) << "Pushbullet device" << info->device()->name() << info->device()->id().toString() << "setup complete";
        info->finish(Device::DeviceErrorNoError);
    });
}

void DevicePluginPushbullet::executeAction(DeviceActionInfo *info)
{
    Device *device = info->device();
    Action action = info->action();

    qCDebug(dcPushbullet()) << "Executing action" << action.actionTypeId() << "for device" << device->name() << device->id().toString();

    QString token;

    // TODO: Activate this and drop the token param from json when maveo is ready for it.
//    pluginStorage()->beginGroup(info->device()->id().toString());
//    token = pluginStorage()->value("token").toString();
//    pluginStorage()->endGroup();

    if (token.isEmpty()) {
        // Check for legacy token in device params...
        ParamTypeId pushNotificationDeviceTokenParamTypeId = ParamTypeId("{1f37ce22-50b8-4183-8b74-557fdb2d3076}");
        token = device->paramValue(pushNotificationDeviceTokenParamTypeId).toString();
    }

    if (token.isEmpty()) {
        return info->finish(Device::DeviceErrorAuthenticationFailure, QT_TR_NOOP("Not authenticated to Pushbullet."));
    }

    QNetworkRequest request(QUrl("https://api.pushbullet.com/v2/pushes"));
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    request.setRawHeader("Access-Token", token.toUtf8().trimmed());

    QVariantMap params;
    params.insert("type", "note");
    params.insert("title", action.param(pushNotificationNotifyActionTitleParamTypeId).value().toString());
    params.insert("body", action.param(pushNotificationNotifyActionBodyParamTypeId).value().toString());
    QNetworkReply *reply = hardwareManager()->networkManager()->post(request, QJsonDocument::fromVariant(params).toJson(QJsonDocument::Compact));
    connect(reply, &QNetworkReply::finished, reply, &QNetworkReply::deleteLater);
    connect(reply, &QNetworkReply::finished, info, [reply, info]{
        if (reply->error() != QNetworkReply::NoError) {
            qCWarning(dcPushbullet()) << "Push message sending failed for" << info->device()->name() << info->device()->id() << reply->errorString() << reply->error();
            emit info->finish(Device::DeviceErrorHardwareNotAvailable);
            return;
        }

        QByteArray data = reply->readAll();

        QJsonParseError error;
        QJsonDocument jsonDoc = QJsonDocument::fromJson(data, &error);
        if (error.error != QJsonParseError::NoError) {
            qCWarning(dcPushbullet()) << "Error reading reply from Pushbullet for" << info->device()->name() << info->device()->id().toString() << error.errorString();
            qCWarning(dcPushbullet()) << qUtf8Printable(data);
            info->finish(Device::DeviceErrorHardwareFailure);
            return;
        }

        QVariantMap replyMap = jsonDoc.toVariant().toMap();
        if (!replyMap.value("active").toBool()) {
            qCWarning(dcPushbullet()) << "Error sending message. The account seems to be deactivated" << info->device()->name() << info->device()->id().toString();
            info->finish(Device::DeviceErrorAuthenticationFailure, QT_TR_NOOP("The Pushbullet account seems to be disabled."));
            return;
        }

        if (replyMap.value("dismissed", true).toBool()) {
            qCWarning(dcPushbullet()) << "Error sending message. The message has been dismissed by the server" << info->device()->name() << info->device()->id().toString();
            info->finish(Device::DeviceErrorHardwareFailure);
            return;
        }

        qCDebug(dcPushbullet()) << "Message sent successfully";
        info->finish(Device::DeviceErrorNoError);
    });
}

