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

#include "integrationpluginhttpcommander.h"
#include "plugininfo.h"

#include <network/networkaccessmanager.h>

#include <QHostInfo>
#include <QNetworkReply>
#include <QNetworkInterface>

IntegrationPluginHttpCommander::IntegrationPluginHttpCommander()
{
}

void IntegrationPluginHttpCommander::setupThing(ThingSetupInfo *info)
{
    Thing *thing = info->thing();
    qCDebug(dcHttpCommander()) << "Setup thing" << thing->name() << thing->params();


    if (thing->thingClassId() == httpRequestThingClassId) {
        QUrl url = thing->paramValue(httpRequestThingUrlParamTypeId).toUrl();

        if (!url.isValid()) {
            qCDebug(dcHttpCommander()) << "Given URL is not valid";
            //: Error setting up thing
            return info->finish(Thing::ThingErrorInvalidParameter, QT_TR_NOOP("The given url is not valid."));
        }
        return info->finish(Thing::ThingErrorNoError);
    }

    if (thing->thingClassId() == httpServerThingClassId) {
        quint16 port = static_cast<uint16_t>(thing->paramValue(httpServerThingPortParamTypeId).toUInt());
        HttpSimpleServer *httpSimpleServer = new HttpSimpleServer(port, this);
        connect(httpSimpleServer, &HttpSimpleServer::requestReceived, this, &IntegrationPluginHttpCommander::onHttpSimpleServerRequestReceived);
        m_httpSimpleServer.insert(thing, httpSimpleServer);

        return info->finish(Thing::ThingErrorNoError);
    }
    info->finish(Thing::ThingErrorNoError);
}

void IntegrationPluginHttpCommander::executeAction(ThingActionInfo *info)
{
    Thing *thing = info->thing();
    Action action = info->action();

    if (thing->thingClassId() == httpRequestThingClassId) {

        if (action.actionTypeId() == httpRequestRequestActionTypeId) {
            QUrl url = thing->paramValue(httpRequestThingUrlParamTypeId).toUrl();
            url.setPort(thing->paramValue(httpRequestThingPortParamTypeId).toInt());
            QString method = action.param(httpRequestRequestActionMethodParamTypeId).value().toString();
            QByteArray payload = action.param(httpRequestRequestActionBodyParamTypeId).value().toByteArray();

            QNetworkReply *reply = nullptr;
            if (method == "GET") {
                reply = hardwareManager()->networkManager()->get(QNetworkRequest(url));
            } else if (method == "POST") {
                reply = hardwareManager()->networkManager()->post(QNetworkRequest(url), payload);
            } else if (method == "PUT") {
                reply = hardwareManager()->networkManager()->put(QNetworkRequest(url), payload);
            } else if (method == "DELETE") {
                reply = hardwareManager()->networkManager()->deleteResource(QNetworkRequest(url));
            } else {
                qCWarning(dcHttpCommander()) << "Unsupported HTTP method" << method;
                info->finish(Thing::ThingErrorInvalidParameter);
                return;
            }

            connect(reply, &QNetworkReply::finished, this, [thing, reply](){
                qCDebug(dcHttpCommander()) << "POST reply finished";
                QByteArray data = reply->readAll();
                int status = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();

                thing->setStateValue(httpRequestResponseStateTypeId, data);
                thing->setStateValue(httpRequestStatusStateTypeId, status);

                // Check HTTP status code
                if (status != 200 || reply->error() != QNetworkReply::NoError) {
                    qCWarning(dcHttpCommander()) << "Request error:" << status << reply->errorString();
                }
                reply->deleteLater();
            });

            return info->finish(Thing::ThingErrorNoError);
        }
        return info->finish(Thing::ThingErrorActionTypeNotFound);
    }
    return info->finish(Thing::ThingErrorThingClassNotFound);
}

void IntegrationPluginHttpCommander::onHttpSimpleServerRequestReceived(const QString &type, const QString &path, const QString &body)
{
    //qCDebug(dcHttpCommander()) << "Request recieved" << type << body;
    HttpSimpleServer *httpServer = static_cast<HttpSimpleServer *>(sender());
    Thing *thing = m_httpSimpleServer.key(httpServer);
    Event ev = Event(httpServerTriggeredEventTypeId, thing->id());
    ParamList params;
    params.append(Param(httpServerTriggeredEventRequestTypeParamTypeId, type));
    params.append(Param(httpServerTriggeredEventPathParamTypeId, path));
    params.append(Param(httpServerTriggeredEventBodyParamTypeId, body));
    ev.setParams(params);
    emit emitEvent(ev);
}


void IntegrationPluginHttpCommander::thingRemoved(Thing *thing)
{
    if (thing->thingClassId() == httpServerThingClassId) {
        HttpSimpleServer* httpSimpleServer= m_httpSimpleServer.take(thing);
        httpSimpleServer->deleteLater();
    }
}
