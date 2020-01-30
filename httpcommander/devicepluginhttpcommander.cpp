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
                info->finish(Device::DeviceErrorInvalidParameter);
                return;
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
