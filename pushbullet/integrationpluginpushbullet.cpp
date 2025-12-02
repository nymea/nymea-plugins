// SPDX-License-Identifier: GPL-3.0-or-later

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*
* Copyright (C) 2013 - 2024, nymea GmbH
* Copyright (C) 2024 - 2025, chargebyte austria GmbH
*
* This file is part of nymea-plugins.
*
* nymea-plugins is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* (at your option) any later version.
*
* nymea-plugins is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
* General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with nymea-plugins. If not, see <https://www.gnu.org/licenses/>.
*
* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

#include "integrationpluginpushbullet.h"
#include "plugininfo.h"

#include <network/networkaccessmanager.h>

#include <QJsonDocument>

IntegrationPluginPushbullet::IntegrationPluginPushbullet(QObject* parent): IntegrationPlugin (parent)
{

}

IntegrationPluginPushbullet::~IntegrationPluginPushbullet()
{

}

void IntegrationPluginPushbullet::setupThing(ThingSetupInfo *info)
{
    Thing *thing = info->thing();

    qCDebug(dcPushbullet()) << "Setting up Pushbullet" << thing->name() << thing->id().toString();

    QString token;

    // TODO: Activate this and drop the token param from json when maveo is ready for it.
//    pluginStorage()->beginGroup(info->thing()->id().toString());
//    token = pluginStorage()->value("token").toString();
//    pluginStorage()->endGroup();

    if (token.isEmpty()) {
        // Check for legacy token in thing params...
        ParamTypeId pushNotificationDeviceTokenParamTypeId = ParamTypeId("{1f37ce22-50b8-4183-8b74-557fdb2d3076}");
        token = thing->paramValue(pushNotificationDeviceTokenParamTypeId).toString();
    }

    if (token.isEmpty()) {
        //: Error setting up thing
        return info->finish(Thing::ThingErrorAuthenticationFailure, QT_TR_NOOP("The provided token must not be empty."));
    }

    QNetworkRequest  request(QUrl("https://api.pushbullet.com/v2/users/me"));
    request.setRawHeader("Access-Token", token.toUtf8().trimmed());

    QNetworkReply *reply = hardwareManager()->networkManager()->get(request);
    connect(reply, &QNetworkReply::finished, reply, &QNetworkReply::deleteLater);

    connect(reply, &QNetworkReply::finished, info, [reply, info]() {
        if (reply->error() == QNetworkReply::AuthenticationRequiredError) {
            //: Error setting up thing
            info->finish(Thing::ThingErrorAuthenticationFailure, QT_TR_NOOP("The provided token is not valid."));
            return;
        }
        if (reply->error() != QNetworkReply::NoError) {
            qCWarning(dcPushbullet()) << "Error fetching user profile:" << reply->errorString() << reply->error();
            //: Error setting up thing
            info->finish(Thing::ThingErrorHardwareFailure, QT_TR_NOOP("Error connecting to Pushbullet."));
            return;
        }

        QByteArray data = reply->readAll();

        QJsonParseError error;
        QJsonDocument jsonDoc = QJsonDocument::fromJson(data, &error);
        if (error.error != QJsonParseError::NoError) {
            qCWarning(dcPushbullet()) << "Error parsing reply from Pushbullet:" << error.errorString();
            qCWarning(dcPushbullet()) << qUtf8Printable(data);
            //: Error setting up thing
            info->finish(Thing::ThingErrorHardwareFailure, QT_TR_NOOP("Received unexpected data from Pushbullet."));
            return;
        }

        QVariantMap replyMap = jsonDoc.toVariant().toMap();
        if (!replyMap.value("active").toBool()) {
            qCWarning(dcPushbullet()) << "Pushbullet account seems to be deactivated.";
            //: Error setting up thing
            info->finish(Thing::ThingErrorAuthenticationFailure, QT_TR_NOOP("The Pushbullet account seems to be disabled."));
            return;
        }

        qCDebug(dcPushbullet()) << "Pushbullet" << info->thing()->name() << info->thing()->id().toString() << "setup complete";
        info->finish(Thing::ThingErrorNoError);
    });
}

void IntegrationPluginPushbullet::executeAction(ThingActionInfo *info)
{
    Thing *thing = info->thing();
    Action action = info->action();

    qCDebug(dcPushbullet()) << "Executing action" << action.actionTypeId() << "for" << thing->name() << thing->id().toString();

    QString token;

    // TODO: Activate this and drop the token param from json when maveo is ready for it.
//    pluginStorage()->beginGroup(info->thing()->id().toString());
//    token = pluginStorage()->value("token").toString();
//    pluginStorage()->endGroup();

    if (token.isEmpty()) {
        // Check for legacy token in thing params...
        ParamTypeId pushNotificationDeviceTokenParamTypeId = ParamTypeId("{1f37ce22-50b8-4183-8b74-557fdb2d3076}");
        token = thing->paramValue(pushNotificationDeviceTokenParamTypeId).toString();
    }

    if (token.isEmpty()) {
        return info->finish(Thing::ThingErrorAuthenticationFailure, QT_TR_NOOP("Not authenticated to Pushbullet."));
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
            qCWarning(dcPushbullet()) << "Push message sending failed for" << info->thing()->name() << info->thing()->id() << reply->errorString() << reply->error();
            info->finish(Thing::ThingErrorHardwareNotAvailable);
            return;
        }

        QByteArray data = reply->readAll();

        QJsonParseError error;
        QJsonDocument jsonDoc = QJsonDocument::fromJson(data, &error);
        if (error.error != QJsonParseError::NoError) {
            qCWarning(dcPushbullet()) << "Error reading reply from Pushbullet for" << info->thing()->name() << info->thing()->id().toString() << error.errorString();
            qCWarning(dcPushbullet()) << qUtf8Printable(data);
            info->finish(Thing::ThingErrorHardwareFailure);
            return;
        }

        QVariantMap replyMap = jsonDoc.toVariant().toMap();
        if (!replyMap.value("active").toBool()) {
            qCWarning(dcPushbullet()) << "Error sending message. The account seems to be deactivated" << info->thing()->name() << info->thing()->id().toString();
            info->finish(Thing::ThingErrorAuthenticationFailure, QT_TR_NOOP("The Pushbullet account seems to be disabled."));
            return;
        }

        if (replyMap.value("dismissed", true).toBool()) {
            qCWarning(dcPushbullet()) << "Error sending message. The message has been dismissed by the server" << info->thing()->name() << info->thing()->id().toString();
            info->finish(Thing::ThingErrorHardwareFailure);
            return;
        }

        qCDebug(dcPushbullet()) << "Message sent successfully";
        info->finish(Thing::ThingErrorNoError);
    });
}

