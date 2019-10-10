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

/*!
    \page osdomotics.html
    \title OSDomotics
    \brief Plugin for the OSDomotics Merkur board based on the OSDomotics tutorials.

    \ingroup plugins
    \ingroup nymea-plugins-merkur

    This plugin allows you to connect nymea to a 6LoWPAN network by adding a Mercury Board from OSDomotics
    as a RPL router to your devices \l{http://osdwiki.open-entry.com/doku.php/de:tutorials:contiki:merkur_board_rpl_usb_router}{OSDomotics Tutorial- RPL Router}.
    All nodes in the 6LoWPAN network of the added RPL router will appear automatically in the system.

    \note Currently the plugin recognizes only one node. That node has to be flashed like the Node in this \l{http://osdwiki.open-entry.com/doku.php/de:tutorials:contiki:use_example_firmware}{OSDomotics Tutorial - Firmware}.

    \chapter Plugin properties
    Following JSON file contains the definition and the description of all available \l{DeviceClass}{DeviceClasses}
    and \l{Vendor}{Vendors} of this \l{DevicePlugin}.

    For more details how to read this JSON file please check out the documentation for \l{The plugin JSON File}.

    \quotefile plugins/deviceplugins/osdomotics/devicepluginosdomotics.json
*/

#include "devicepluginosdomotics.h"
#include "devices/device.h"
#include "plugininfo.h"
#include "network/networkaccessmanager.h"

DevicePluginOsdomotics::DevicePluginOsdomotics()
{

}

DevicePluginOsdomotics::~DevicePluginOsdomotics()
{
    hardwareManager()->pluginTimerManager()->unregisterTimer(m_pluginTimer);
}

void DevicePluginOsdomotics::init()
{
    m_coap = new Coap(this);
    connect(m_coap, &Coap::replyFinished, this, &DevicePluginOsdomotics::coapReplyFinished);

    m_pluginTimer = hardwareManager()->pluginTimerManager()->registerTimer(10);
    connect(m_pluginTimer, &PluginTimer::timeout, this, &DevicePluginOsdomotics::onPluginTimer);
}

void DevicePluginOsdomotics::setupDevice(DeviceSetupInfo *info)
{
    Device *device = info->device();

    if (device->deviceClassId() == rplRouterDeviceClassId) {
        qCDebug(dcOsdomotics) << "Setup RPL router" << device->paramValue(rplRouterDeviceRplHostParamTypeId).toString();
        QHostAddress address(device->paramValue(rplRouterDeviceRplHostParamTypeId).toString());

        if (address.isNull()) {
            qCWarning(dcOsdomotics) << "Got invalid address" << device->paramValue(rplRouterDeviceRplHostParamTypeId).toString();
            return info->finish(Device::DeviceErrorInvalidParameter, QT_TR_NOOP("The given RPL address is not valid."));
        }

        QUrl url;
        url.setScheme("http");
        url.setHost(address.toString());

        QNetworkReply *reply = hardwareManager()->networkManager()->get(QNetworkRequest(url));
        connect(reply, &QNetworkReply::finished, reply, &QNetworkReply::deleteLater);

        connect(reply, &QNetworkReply::finished, info, [this, info, reply](){
            // check HTTP status code
            int status = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
            if (status != 200) {
                qCWarning(dcOsdomotics) << "Setup reply HTTP error:" << reply->errorString();
                info->finish(Device::DeviceErrorHardwareFailure, QT_TR_NOOP("Error communicating with RPL device."));
                return;
            }

            QByteArray data = reply->readAll();
            parseNodes(info->device(), data);

            info->finish(Device::DeviceErrorNoError);
        });

        return;
    }

    if (device->deviceClassId() == merkurNodeDeviceClassId) {
        qCDebug(dcOsdomotics) << "Setup Merkur node" << device->paramValue(merkurNodeDeviceHostParamTypeId).toString();
        device->setParentId(DeviceId(device->paramValue(merkurNodeDeviceRouterParamTypeId).toString()));
        return info->finish(Device::DeviceErrorNoError);
    }
}

void DevicePluginOsdomotics::deviceRemoved(Device *device)
{
    Q_UNUSED(device)
}

void DevicePluginOsdomotics::postSetupDevice(Device *device)
{
    updateNode(device);
}

void DevicePluginOsdomotics::executeAction(DeviceActionInfo *info)
{
    Device *device = info->device();
    Action action = info->action();

    if (action.actionTypeId() == merkurNodeToggleLedActionTypeId) {
        QUrl url;
        url.setScheme("coap");
        url.setHost(device->paramValue(merkurNodeDeviceHostParamTypeId).toString());
        url.setPath("/actuators/toggle");

        qCDebug(dcOsdomotics) << "Toggle light";

        CoapReply *reply = m_coap->post(CoapRequest(url));

        if (reply->isFinished()) {
            if (reply->error() != CoapReply::NoError) {
                qCWarning(dcOsdomotics) << "CoAP reply finished with error" << reply->errorString();
                reply->deleteLater();
                return info->finish(Device::DeviceErrorHardwareNotAvailable);
            }
            return info->finish(Device::DeviceErrorNoError);
        }

        connect(reply, &CoapReply::finished, reply, &CoapReply::deleteLater);

        connect(reply, &CoapReply::finished, info, [info, reply](){
            if (reply->error() != CoapReply::NoError) {
                qCWarning(dcOsdomotics) << "CoAP toggle reply finished with error" << reply->errorString();
                info->finish(Device::DeviceErrorHardwareFailure);
                return;
            }

            info->finish(Device::DeviceErrorNoError);
        });

        return;
    }

    qCWarning(dcOsdomotics()) << "Unhandled executeAction in plugin!";
}

void DevicePluginOsdomotics::scanNodes(Device *device)
{
    QHostAddress address(device->paramValue(merkurNodeDeviceHostParamTypeId).toString());
    qCDebug(dcOsdomotics) << "Scan for new nodes" << address.toString();

    QUrl url;
    url.setScheme("http");
    url.setHost(address.toString());

    QNetworkReply *reply = hardwareManager()->networkManager()->get(QNetworkRequest(url));
    connect(reply, &QNetworkReply::finished, this, &DevicePluginOsdomotics::onNetworkReplyFinished);
    m_asyncNodeRescans.insert(reply, device);
}

void DevicePluginOsdomotics::parseNodes(Device *device, const QByteArray &data)
{
    //qCDebug(dcOsdomotics) << data;

    // TODO: get all nodes
    //       find better method to get nodes

    int index = data.indexOf("Routes<pre>") + 11;
    int delta = data.indexOf("/128",index);

    QHostAddress nodeAddress(QString(data.mid(index, delta - index)));

    // check if we already have found this node
    foreach (Device *device, myDevices()) {
        if (device->paramValue(merkurNodeDeviceHostParamTypeId).toString() == nodeAddress.toString()) {
            return;
        }
    }

    QUrl url;
    url.setScheme("coap");
    url.setHost(nodeAddress.toString());
    url.setPath("/.well-known/core");

    qCDebug(dcOsdomotics) << "Discover node on" << url.toString();

    CoapReply *reply = m_coap->get(CoapRequest(url));
    if (reply->isFinished()) {
        if (reply->error() != CoapReply::NoError) {
            qCWarning(dcOsdomotics) << "Reply finished with error" << reply->errorString();
        } else {
            qCDebug(dcOsdomotics) << "Reply finished" << reply;
        }

        // Note: please don't forget to delete the reply
        reply->deleteLater();
        return;
    }
    m_discoveryRequests.insert(reply, device);
}

void DevicePluginOsdomotics::updateNode(Device *device)
{
    qCDebug(dcOsdomotics) << "Update node" << device->paramValue(merkurNodeDeviceHostParamTypeId).toString() << "battery value";

    QUrl url;
    url.setScheme("coap");
    url.setHost(device->paramValue(merkurNodeDeviceHostParamTypeId).toString());
    url.setPath("/sensors/battery");

    CoapReply *reply = m_coap->get(CoapRequest(url));

    if (reply->isFinished()) {
        if (reply->error() != CoapReply::NoError) {
            qCWarning(dcOsdomotics) << "CoAP reply finished with error" << reply->errorString();
            reply->deleteLater();
        }
    }
    m_updateRequests.insert(reply, device);
}

Device *DevicePluginOsdomotics::findDevice(const QHostAddress &address)
{
    foreach (Device *device, myDevices()) {
        if (device->paramValue(merkurNodeDeviceHostParamTypeId).toString() == address.toString()) {
            return device;
        }
    }
    return nullptr;
}

void DevicePluginOsdomotics::onPluginTimer()
{
    foreach (Device *device, myDevices()) {
        if (device->deviceClassId() == merkurNodeDeviceClassId) {
            updateNode(device);
        } else if(device->deviceClassId() == rplRouterDeviceClassId) {
            scanNodes(device);
        }
    }
}

void DevicePluginOsdomotics::onNetworkReplyFinished()
{
    QNetworkReply *reply = static_cast<QNetworkReply *>(sender());
    int status = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();

    if (m_asyncNodeRescans.contains(reply)) {
        Device *device = m_asyncSetup.take(reply);

        // check HTTP status code
        if (status != 200) {
            qCWarning(dcOsdomotics) << "Setup reply HTTP error:" << reply->errorString();
            reply->deleteLater();
            return;
        }

        QByteArray data = reply->readAll();
        parseNodes(device, data);
    }
    reply->deleteLater();
}

void DevicePluginOsdomotics::coapReplyFinished(CoapReply *reply)
{
    qCDebug(dcOsdomotics) << "coap reply finished" << reply;

    if (m_discoveryRequests.contains(reply)) {
        Device *device = m_discoveryRequests.take(reply);
        if (reply->error() != CoapReply::NoError) {
            qCWarning(dcOsdomotics) << "CoAP discover reply finished with error" << reply->errorString();
            reply->deleteLater();
            return;
        }

        // TODO: parse CoRE links and get the type of the node

        DeviceDescriptor descriptor(merkurNodeDeviceClassId, "Merkur Node", reply->request().url().host());
        ParamList params;
        params.append(Param(merkurNodeDeviceNameParamTypeId, "Merkur Node"));
        params.append(Param(merkurNodeDeviceHostParamTypeId,  reply->request().url().host()));
        params.append(Param(merkurNodeDeviceRouterParamTypeId, device->id()));
        descriptor.setParams(params);
        emit autoDevicesAppeared({descriptor});
    } else if (m_updateRequests.contains(reply)) {
        Device *device = m_updateRequests.take(reply);
        if (reply->error() != CoapReply::NoError) {
            qCWarning(dcOsdomotics) << "CoAP update reply finished with error" << reply->errorString();
            reply->deleteLater();
            return;
        }
        int batteryValue = reply->payload().toInt();
        qCDebug(dcOsdomotics) << "Node updated" << batteryValue;
        device->setStateValue(merkurNodeBatteryStateTypeId, batteryValue);
    }

    reply->deleteLater();
}


