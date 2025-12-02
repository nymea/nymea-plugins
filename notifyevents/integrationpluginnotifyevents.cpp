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

#include "integrationpluginnotifyevents.h"
#include "plugininfo.h"

#include <network/networkaccessmanager.h>

#include <QJsonDocument>
#include <QUrlQuery>

IntegrationPluginNotfyEvents::IntegrationPluginNotfyEvents(QObject* parent): IntegrationPlugin (parent)
{
}

IntegrationPluginNotfyEvents::~IntegrationPluginNotfyEvents()
{

}

void IntegrationPluginNotfyEvents::setupThing(ThingSetupInfo *info)
{
    info->finish(Thing::ThingErrorNoError);
}

void IntegrationPluginNotfyEvents::executeAction(ThingActionInfo *info)
{
    Thing *thing = info->thing();
    Action action = info->action();

    qCDebug(dcNotifyEvents()) << "Executing action" << action.actionTypeId() << "for" << thing->name() << thing->id().toString();

    QString token = thing->paramValue(notifyEventsThingTokenParamTypeId).toString();
    QString title = action.param(notifyEventsNotifyActionTitleParamTypeId).value().toString();
    QString body = action.param(notifyEventsNotifyActionBodyParamTypeId).value().toString();

    QUrlQuery query;
    query.addQueryItem("title", title);
    query.addQueryItem("content", body);
    query.addQueryItem("level", "info");
    query.addQueryItem("priority", "normal");

    QString url = QString("https://notify.events/api/v1/channel/source/%1/execute").arg(token);
    QNetworkRequest request((QUrl(url)));
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/x-www-form-urlencoded");

    qCDebug(dcNotifyEvents()) << "Sending notification" << request.url().toString() << query.toString();
    foreach (const QByteArray &headerName, request.rawHeaderList()) {
        qCDebug(dcNotifyEvents()) << "Header:" << headerName << request.rawHeader(headerName);
    }
//    QNetworkReply *reply = hardwareManager()->networkManager()->post(request, QJsonDocument::fromVariant(data).toJson(QJsonDocument::Compact));
    QNetworkReply *reply = hardwareManager()->networkManager()->post(request, query.toString().toUtf8());
    connect(reply, &QNetworkReply::finished, reply, &QNetworkReply::deleteLater);
    connect(reply, &QNetworkReply::finished, info, [reply, info]{
        if (reply->error() != QNetworkReply::NoError) {
            qCWarning(dcNotifyEvents()) << "Notify.Events message sending failed for" << info->thing()->name() << info->thing()->id() << reply->errorString() << reply->error();
            info->finish(Thing::ThingErrorHardwareNotAvailable);
            return;
        }

        QByteArray data = reply->readAll();

        QJsonParseError error;
        QJsonDocument jsonDoc = QJsonDocument::fromJson(data, &error);
        if (error.error != QJsonParseError::NoError) {
            qCWarning(dcNotifyEvents()) << "Error reading reply from server for" << info->thing()->name() << info->thing()->id().toString() << error.errorString();
            qCWarning(dcNotifyEvents()) << qUtf8Printable(data);
            info->finish(Thing::ThingErrorHardwareFailure);
            return;
        }

        qCDebug(dcNotifyEvents()) << qUtf8Printable(jsonDoc.toJson());
        qCDebug(dcNotifyEvents()) << "Message sent successfully";
        info->finish(Thing::ThingErrorNoError);
    });
}

