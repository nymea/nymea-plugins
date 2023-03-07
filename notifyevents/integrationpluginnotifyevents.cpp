/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*
* Copyright 2013 - 2023, nymea GmbH
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
            emit info->finish(Thing::ThingErrorHardwareNotAvailable);
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

        QVariantMap replyMap = jsonDoc.toVariant().toMap();
        qDebug(dcNotifyEvents()) << qUtf8Printable(jsonDoc.toJson());


        qCDebug(dcNotifyEvents()) << "Message sent successfully";
        info->finish(Thing::ThingErrorNoError);
    });
}

