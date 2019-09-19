/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *                                                                         *
 *  Copyright (C) 2015 Simon Stürz <simon.stuerz@guh.io>                   *
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

#include "devicepluginwemo.h"

#include "devices/device.h"
#include "plugininfo.h"
#include "network/networkaccessmanager.h"
#include "network/upnp/upnpdiscovery.h"

#include <QDebug>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QXmlStreamReader>
#include <QXmlStreamWriter>
#include <QXmlStreamAttributes>

DevicePluginWemo::DevicePluginWemo()
{
}

DevicePluginWemo::~DevicePluginWemo()
{
    hardwareManager()->pluginTimerManager()->unregisterTimer(m_pluginTimer);
}

void DevicePluginWemo::init()
{
    m_pluginTimer = hardwareManager()->pluginTimerManager()->registerTimer(10);
    connect(m_pluginTimer, &PluginTimer::timeout, this, &DevicePluginWemo::onPluginTimer);

    connect(hardwareManager()->upnpDiscovery(), &UpnpDiscovery::upnpNotify, this, &DevicePluginWemo::onUpnpNotifyReceived);
}

void DevicePluginWemo::discoverDevices(DeviceDiscoveryInfo *info)
{
    UpnpDiscoveryReply *reply = hardwareManager()->upnpDiscovery()->discoverDevices("upnp:rootdevice");

    connect(reply, &UpnpDiscoveryReply::finished, reply, &UpnpDiscoveryReply::deleteLater);

    connect(reply, &UpnpDiscoveryReply::finished, info, [this, info, reply](){
        qCDebug(dcWemo()) << "Upnp discovery finished";

        if (reply->error() != UpnpDiscoveryReply::UpnpDiscoveryReplyErrorNoError) {
            qCWarning(dcWemo()) << "Upnp discovery error" << reply->error();
            info->finish(Device::DeviceErrorHardwareFailure, QT_TR_NOOP("An error happened during discovery."));
            return ;
        }

        foreach (const UpnpDeviceDescriptor &upnpDeviceDescriptor, reply->deviceDescriptors()) {
            if (upnpDeviceDescriptor.deviceType() == "urn:Belkin:device:controllee:1") {
                DeviceDescriptor descriptor(wemoSwitchDeviceClassId, upnpDeviceDescriptor.friendlyName(), upnpDeviceDescriptor.serialNumber());
                ParamList params;
                params.append(Param(wemoSwitchDeviceHostParamTypeId, upnpDeviceDescriptor.hostAddress().toString()));
                params.append(Param(wemoSwitchDevicePortParamTypeId, upnpDeviceDescriptor.port()));
                params.append(Param(wemoSwitchDeviceSerialParamTypeId, upnpDeviceDescriptor.serialNumber()));
                descriptor.setParams(params);

                foreach (Device *existingDevice, myDevices()) {
                    if (existingDevice->paramValue(wemoSwitchDeviceSerialParamTypeId).toString() == upnpDeviceDescriptor.serialNumber()) {
                        descriptor.setDeviceId(existingDevice->id());
                        break;
                    }
                }
                info->addDeviceDescriptor(descriptor);
            }
        }
        info->finish(Device::DeviceErrorNoError);
    });
}

void DevicePluginWemo::setupDevice(DeviceSetupInfo *info)
{
    refresh(info->device());
    info->finish(Device::DeviceErrorNoError);
}

void DevicePluginWemo::executeAction(DeviceActionInfo *info)
{
    Device *device = info->device();
    Action action = info->action();

    // Set power
    Q_ASSERT_X(action.actionTypeId() == wemoSwitchPowerActionTypeId, "WemoPlugin", "Unexpected action type in executeAction");

    // Check if wemo device is reachable
    if (!device->stateValue(wemoSwitchConnectedStateTypeId).toBool()) {
        info->finish(Device::DeviceErrorHardwareNotAvailable);
        return;
    }

    bool power = action.param(wemoSwitchPowerActionPowerParamTypeId).value().toBool();

    if (device->stateValue(wemoSwitchPowerStateTypeId).toBool() == power) {
        info->finish(Device::DeviceErrorNoError);
        return;
    }

    QByteArray setPowerMessage("<?xml version=\"1.0\" encoding=\"utf-8\"?><s:Envelope xmlns:s=\"http://schemas.xmlsoap.org/soap/envelope/\" s:encodingStyle=\"http://schemas.xmlsoap.org/soap/encoding/\"><s:Body><u:SetBinaryState xmlns:u=\"urn:Belkin:service:basicevent:1\"><BinaryState>" + QByteArray::number((int)power) + "</BinaryState></u:SetBinaryState></s:Body></s:Envelope>");

    QNetworkRequest request;
    request.setUrl(QUrl("http://" + device->paramValue(wemoSwitchDeviceHostParamTypeId).toString() + ":" + device->paramValue(wemoSwitchDevicePortParamTypeId).toString() + "/upnp/control/basicevent1"));
    request.setHeader(QNetworkRequest::ContentTypeHeader,QVariant("text/xml; charset=\"utf-8\""));
    request.setHeader(QNetworkRequest::UserAgentHeader,QVariant("nymea"));
    request.setRawHeader("SOAPACTION", "\"urn:Belkin:service:basicevent:1#SetBinaryState\"");

    QNetworkReply *reply = hardwareManager()->networkManager()->post(request, setPowerMessage);

    connect(reply, &QNetworkReply::finished, reply, &QNetworkReply::deleteLater);

    connect(reply, &QNetworkReply::finished, info, [this, info, reply](){
        if (reply->error() != QNetworkReply::NoError){
            info->finish(Device::DeviceErrorHardwareFailure, QT_TR_NOOP("Could not connect to wemo switch."));
            return;
        }
        QByteArray data = reply->readAll();

        if (data.contains("<BinaryState>1</BinaryState>") || data.contains("<BinaryState>0</BinaryState>")) {
            info->finish(Device::DeviceErrorNoError);
            info->device()->setStateValue(wemoSwitchConnectedStateTypeId, true);
            refresh(info->device());
        } else {
            info->finish(Device::DeviceErrorHardwareNotAvailable);
            info->device()->setStateValue(wemoSwitchConnectedStateTypeId, false);
        }

    });
}

void DevicePluginWemo::deviceRemoved(Device *device)
{
    // Check if there is a missing reply for this device
    foreach (Device *d, m_refreshReplies.values()) {
        if (d->id() == device->id()) {
            QNetworkReply * reply = m_refreshReplies.key(device);
            m_refreshReplies.remove(reply);
            // Note: delete will be done in networkManagerReplyReady()
        }
    }
}

void DevicePluginWemo::refresh(Device *device)
{
    QByteArray getBinarayStateMessage("<?xml version=\"1.0\" encoding=\"utf-8\"?><s:Envelope xmlns:s=\"http://schemas.xmlsoap.org/soap/envelope/\" s:encodingStyle=\"http://schemas.xmlsoap.org/soap/encoding/\"><s:Body><u:GetBinaryState xmlns:u=\"urn:Belkin:service:basicevent:1\"><BinaryState>1</BinaryState></u:GetBinaryState></s:Body></s:Envelope>");

    QNetworkRequest request;
    request.setUrl(QUrl("http://" + device->paramValue(wemoSwitchDeviceHostParamTypeId).toString() + ":" + device->paramValue(wemoSwitchDevicePortParamTypeId).toString() + "/upnp/control/basicevent1"));
    request.setHeader(QNetworkRequest::ContentTypeHeader,QVariant("text/xml; charset=\"utf-8\""));
    request.setHeader(QNetworkRequest::UserAgentHeader,QVariant("nymea"));
    request.setRawHeader("SOAPACTION", "\"urn:Belkin:service:basicevent:1#GetBinaryState\"");

    QNetworkReply *reply = hardwareManager()->networkManager()->post(request, getBinarayStateMessage);
    connect(reply, &QNetworkReply::finished, this, &DevicePluginWemo::onNetworkReplyFinished);
    m_refreshReplies.insert(reply, device);
}

void DevicePluginWemo::processRefreshData(const QByteArray &data, Device *device)
{
    if (data.contains("<BinaryState>0</BinaryState>")) {
        device->setStateValue(wemoSwitchPowerStateTypeId, false);
        device->setStateValue(wemoSwitchConnectedStateTypeId, true);
    } else if (data.contains("<BinaryState>1</BinaryState>")) {
        device->setStateValue(wemoSwitchPowerStateTypeId, true);
        device->setStateValue(wemoSwitchConnectedStateTypeId, true);
    } else {
        device->setStateValue(wemoSwitchConnectedStateTypeId, false);
    }
}

void DevicePluginWemo::onNetworkReplyFinished()
{
    QNetworkReply *reply = static_cast<QNetworkReply *>(sender());
    int status = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();

    // check HTTP status code
    if (status != 200 || reply->error() != QNetworkReply::NoError) {
        qCWarning(dcWemo()) << "Request error:" << status << reply->errorString();
        reply->deleteLater();
        return;
    }

    if (m_refreshReplies.contains(reply)) {
        QByteArray data = reply->readAll();
        Device *device = m_refreshReplies.take(reply);
        processRefreshData(data, device);
    }

    reply->deleteLater();
}

void DevicePluginWemo::onPluginTimer()
{
    foreach (Device* device, myDevices()) {
        refresh(device);
    }
}

void DevicePluginWemo::onUpnpDiscoveryFinished()
{
}

void DevicePluginWemo::onUpnpNotifyReceived(const QByteArray &notification)
{
    Q_UNUSED(notification)
}
