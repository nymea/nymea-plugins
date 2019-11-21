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
#include <QNetworkInterface>

DevicePluginHttpCommander::DevicePluginHttpCommander()
{
}

void DevicePluginHttpCommander::setupDevice(DeviceSetupInfo *info)
{
    Device *device = info->device();
    qDebug(dcHttpCommander()) << "Setup device" << device->name() << device->params();


    if (device->deviceClassId() == httpRequestDeviceClassId) {
        QUrl url = device->paramValue(httpRequestDeviceUrlParamTypeId).toUrl();

        if (!url.isValid()) {
            qDebug(dcHttpCommander()) << "Given URL is not valid";
            //: Error setting up device
            return info->finish(Device::DeviceErrorInvalidParameter, QT_TR_NOOP("The given url is not valid."));
        }
        return info->finish(Device::DeviceErrorNoError);
    }

    if (device->deviceClassId() == httpServerDeviceClassId) {
        quint16 port = static_cast<uint16_t>(device->paramValue(httpServerDevicePortParamTypeId).toUInt());
        HttpSimpleServer *httpSimpleServer = new HttpSimpleServer(port, this);
        connect(httpSimpleServer, &HttpSimpleServer::requestReceived, this, &DevicePluginHttpCommander::onHttpSimpleServerRequestReceived);
        m_httpSimpleServer.insert(device, httpSimpleServer);

        return info->finish(Device::DeviceErrorNoError);
    }
    info->finish(Device::DeviceErrorNoError);
}

void DevicePluginHttpCommander::executeAction(DeviceActionInfo *info)
{
    Device *device = info->device();
    Action action = info->action();

    if (device->deviceClassId() == httpRequestDeviceClassId) {

        if (action.actionTypeId() == httpRequestRequestActionTypeId) {
            QUrl url = device->paramValue(httpRequestDeviceUrlParamTypeId).toUrl();
            url.setPort(device->paramValue(httpRequestDevicePortParamTypeId).toInt());
            QString method = action.param(httpRequestRequestActionMethodParamTypeId).value().toString();
            QByteArray payload = action.param(httpRequestRequestActionBodyParamTypeId).value().toByteArray();

            QNetworkReply *reply;
            if (method == "GET") {
                reply = hardwareManager()->networkManager()->get(QNetworkRequest(url));
            } else if (method == "POST") {
                reply = hardwareManager()->networkManager()->post(QNetworkRequest(url), payload);
            } else if (method == "PUT") {
                reply = hardwareManager()->networkManager()->put(QNetworkRequest(url), payload);
            } else if (method == "DELETE") {
                reply = hardwareManager()->networkManager()->deleteResource(QNetworkRequest(url));
            }
            connect(reply, &QNetworkReply::finished, this, [device, reply, this](){

                qDebug(dcHttpCommander()) << "POST reply finished";
                QByteArray data = reply->readAll();
                int status = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();

                device->setStateValue(httpRequestResponseStateTypeId, data);
                device->setStateValue(httpRequestStatusStateTypeId, status);

                // Check HTTP status code
                if (status != 200 || reply->error() != QNetworkReply::NoError) {
                    qCWarning(dcHttpCommander()) << "Request error:" << status << reply->errorString();
                }
                reply->deleteLater();
            });

            return info->finish(Device::DeviceErrorNoError);
        }
        return info->finish(Device::DeviceErrorActionTypeNotFound);
    }
    return info->finish(Device::DeviceErrorDeviceClassNotFound);
}

void DevicePluginHttpCommander::onHttpSimpleServerRequestReceived(const QString &type, const QString &path, const QString &body)
{
    //qCDebug(dcHttpCommander()) << "Request recieved" << type << body;
    HttpSimpleServer *httpServer = static_cast<HttpSimpleServer *>(sender());
    Device *device = m_httpSimpleServer.key(httpServer);
    Event ev = Event(httpServerTriggeredEventTypeId, device->id());
    ParamList params;
    params.append(Param(httpServerTriggeredEventRequestTypeParamTypeId, type));
    params.append(Param(httpServerTriggeredEventPathParamTypeId, path));
    params.append(Param(httpServerTriggeredEventBodyParamTypeId, body));
    ev.setParams(params);
    emit emitEvent(ev);
}


void DevicePluginHttpCommander::deviceRemoved(Device *device)
{
    if (device->deviceClassId() == httpServerDeviceClassId) {
        HttpSimpleServer* httpSimpleServer= m_httpSimpleServer.take(device);
        httpSimpleServer->deleteLater();
    }
}
