/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *                                                                         *
 *  Copyright (C) 2020 Bernhard Trinnes <bernhard.trinnes@nymea.io>        *
 *                                                                         *
 *  This file is part of nymea.                                            *
 *                                                                         *
 *  nymea is free software: you can redistribute it and/or modify          *
 *  it under the terms of the GNU General Public License as published by   *
 *  the Free Software Foundation, version 2 of the License.                *
 *                                                                         *
 *  nymea is distributed in the hope that it will be useful,               *
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of         *
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the           *
 *  GNU General Public License for more details.                           *
 *                                                                         *
 *  You should have received a copy of the GNU General Public License      *
 *  along with nymea. If not, see <http://www.gnu.org/licenses/>.          *
 *                                                                         *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

#include "devicepluginedimax.h"

#include "devices/device.h"
#include "types/param.h"
#include "plugininfo.h"
#include "network/upnp/upnpdiscovery.h"
#include "network/upnp/upnpdiscoveryreply.h"

DevicePluginEdimax::DevicePluginEdimax()
{

}

DevicePluginEdimax::~DevicePluginEdimax()
{

}

void DevicePluginEdimax::init()
{
}

void DevicePluginEdimax::discoverDevices(DeviceDiscoveryInfo *info)
{
    qCDebug(dcEdimax()) << "Starting UPnP discovery...";
    UpnpDiscoveryReply *upnpReply = hardwareManager()->upnpDiscovery()->discoverDevices("libhue:idl");

    // Always clean up the upnp discovery
    connect(upnpReply, &UpnpDiscoveryReply::finished, upnpReply, &UpnpDiscoveryReply::deleteLater);

    // Process results if info is still around
    connect(upnpReply, &UpnpDiscoveryReply::finished, info, [this, upnpReply](){

        if (upnpReply->error() != UpnpDiscoveryReply::UpnpDiscoveryReplyErrorNoError) {
            qCWarning(dcEdimax()) << "Upnp discovery error" << upnpReply->error();
        }

        foreach (const UpnpDeviceDescriptor &upnpDevice, upnpReply->deviceDescriptors()) {
            if (upnpDevice.modelDescription().contains("Philips")) {
                DeviceDescriptor descriptor(wirelessSocketDeviceClassId, "Wireless socket", upnpDevice.hostAddress().toString());
                ParamList params;
                QString bridgeId = upnpDevice.serialNumber().toLower();
                foreach (Device *existingDevice, myDevices()) {
                    if (existingDevice->paramValue(wirelessSocketDeviceIdParamTypeId).toString() == bridgeId) {
                        descriptor.setDeviceId(existingDevice->id());
                        break;
                    }
                }
                params.append(Param(wirelessSocketHostParamTypeId, upnpDevice.hostAddress().toString()));
                params.append(Param(wirelessSocketDeviceIdParamTypeId, bridgeId));
                descriptor.setParams(params);
                qCDebug(dcEdimax()) << "UPnP: Found Hue bridge:" << bridgeId;
            }
        }
    });
}

void DevicePluginEdimax::startPairing(DevicePairingInfo *info)
{

    info->finish(Device::DeviceErrorNoError, QT_TR_NOOP("Please enter the login credentials for your Edimax device."));
}

void DevicePluginEdimax::confirmPairing(DevicePairingInfo *info, const QString &username, const QString &password)
{
    QString ipAddress = info->params().paramValue(wirelessSocketDeviceHostParamTypeId).toString();
    int port = 10000;
    QNetworkRequest request;
    request.setUrl(QUrl(QString("http://%1:%2@%3:%4/smartplug.cgi").arg(username).arg(password).arg(ipAddress).arg(port)));
    qCDebug(dcEdimax()) << "SetupDevice fetching:" << request.url() << request.rawHeader("Authorization") << username << password;
    QNetworkReply *reply = hardwareManager()->networkManager()->get(request);
    connect(reply, &QNetworkReply::finished, reply, &QNetworkReply::deleteLater);
    connect(reply, &QNetworkReply::finished, info, [this, info, reply, username, password](){
        if (reply->error() == QNetworkReply::NoError) {
            pluginStorage()->beginGroup(info->deviceId().toString());
            pluginStorage()->setValue("username", username);
            pluginStorage()->setValue("password", password);
            pluginStorage()->endGroup();
            info->finish(Device::DeviceErrorNoError);
        } else {
            //: Error pairing device
            info->finish(Device::DeviceErrorAuthenticationFailure, QT_TR_NOOP("Wrong username or password."));
        }
    });
}

void DevicePluginEdimax::setupDevice(DeviceSetupInfo *info)
{
    Device *device = info->device();

    if (device->deviceClassId() == wirelessSocketDeviceClassId) {

        info->finish(Device::DeviceErrorNoError);
    }
}

void DevicePluginEdimax::postSetupDevice(Device *device)
{
    if (device->deviceClassId() == wirelessSocketDeviceClassId) {

    }
}

void DevicePluginEdimax::executeAction(DeviceActionInfo *info)
{
    Device *device = info->device();
    Action action = info->action();

    if (device->deviceClassId() == wirelessSocketDeviceClassId) {
        if (action.actionTypeId() == wirelessSocketPowerActionTypeId) {

        } else {
            qCWarning(dcEdimax()) << "ActionTypeId not found" << action.actionTypeId();
        }
    } else {
        qCWarning(dcEdimax()) << "DeviceClassId not found" << device->deviceClassId();
    }
}

void DevicePluginEdimax::deviceRemoved(Device *device)
{
    if (device->deviceClassId() == wirelessSocketDeviceClassId) {

    }

    if (myDevices().isEmpty()) {
        m_pluginTimer->deleteLater();
        m_pluginTimer = nullptr;
    }
}

QUuid DevicePluginEdimax::setPower(QHostAddress address, bool power)
{
    QUuid requestId = QUuid::createUuid();
    QUrl url;
    url.setHost(address.toString());
    url.setScheme("http");
    url.setPort(10000);
    url.setPath("/smartplug.cgi"); //Select source
    QByteArray content;
    QXmlStreamWriter xml(&content);
    xml.writeStartDocument();
    xml.writeStartElement("SMARTPLUG");
    xml.writeAttribute("id", "edimax");
    xml.writeStartElement("CMD");
    xml.writeAttribute("id", "setup");
    if (power) {
        xml.writeTextElement("Device.System.Power.State", "ON");
    } else {
        xml.writeTextElement("Device.System.Power.State", "OFF");
    }
    xml.writeEndElement(); //CMD
    xml.writeEndElement(); //SMARTPLUG
    xml.writeEndDocument();
    qDebug(dcEdimax()) << "Sending request" << url << content;

    QNetworkReply *reply = m_networkManager->post(QNetworkRequest(url), content);
    connect(reply, &QNetworkReply::finished, this, [requestId, this] {

    });
    return requestId;
}

