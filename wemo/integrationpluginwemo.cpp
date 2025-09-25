/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*
* Copyright 2013 - 2025, nymea GmbH
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

#include "integrationpluginwemo.h"
#include "plugininfo.h"

#include <plugintimer.h>
#include <integrations/thing.h>
#include <network/networkaccessmanager.h>
#include <network/upnp/upnpdiscovery.h>

#include <QDebug>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QXmlStreamReader>
#include <QXmlStreamWriter>
#include <QXmlStreamAttributes>

IntegrationPluginWemo::IntegrationPluginWemo()
{
}

IntegrationPluginWemo::~IntegrationPluginWemo()
{
    hardwareManager()->pluginTimerManager()->unregisterTimer(m_pluginTimer);
}

void IntegrationPluginWemo::init()
{
    m_pluginTimer = hardwareManager()->pluginTimerManager()->registerTimer(10);
    connect(m_pluginTimer, &PluginTimer::timeout, this, &IntegrationPluginWemo::onPluginTimer);

    connect(hardwareManager()->upnpDiscovery(), &UpnpDiscovery::upnpNotify, this, &IntegrationPluginWemo::onUpnpNotifyReceived);
}

void IntegrationPluginWemo::discoverThings(ThingDiscoveryInfo *info)
{
    UpnpDiscoveryReply *reply = hardwareManager()->upnpDiscovery()->discoverDevices("upnp:rootdevice");

    connect(reply, &UpnpDiscoveryReply::finished, reply, &UpnpDiscoveryReply::deleteLater);

    connect(reply, &UpnpDiscoveryReply::finished, info, [this, info, reply](){
        qCDebug(dcWemo()) << "Upnp discovery finished";

        if (reply->error() != UpnpDiscoveryReply::UpnpDiscoveryReplyErrorNoError) {
            qCWarning(dcWemo()) << "Upnp discovery error" << reply->error();
            info->finish(Thing::ThingErrorHardwareFailure, QT_TR_NOOP("An error happened during discovery."));
            return ;
        }

        foreach (const UpnpDeviceDescriptor &upnpDeviceDescriptor, reply->deviceDescriptors()) {
            if (upnpDeviceDescriptor.deviceType() == "urn:Belkin:thing:controllee:1") {
                ThingDescriptor descriptor(wemoSwitchThingClassId, upnpDeviceDescriptor.friendlyName(), upnpDeviceDescriptor.serialNumber());
                ParamList params;
                params.append(Param(wemoSwitchThingHostParamTypeId, upnpDeviceDescriptor.hostAddress().toString()));
                params.append(Param(wemoSwitchThingPortParamTypeId, upnpDeviceDescriptor.port()));
                params.append(Param(wemoSwitchThingSerialParamTypeId, upnpDeviceDescriptor.serialNumber()));
                descriptor.setParams(params);

                foreach (Thing *existingThing, myThings()) {
                    if (existingThing->paramValue(wemoSwitchThingSerialParamTypeId).toString() == upnpDeviceDescriptor.serialNumber()) {
                        descriptor.setThingId(existingThing->id());
                        break;
                    }
                }
                info->addThingDescriptor(descriptor);
            }
        }
        info->finish(Thing::ThingErrorNoError);
    });
}

void IntegrationPluginWemo::setupThing(ThingSetupInfo *info)
{
    refresh(info->thing());
    info->finish(Thing::ThingErrorNoError);
}

void IntegrationPluginWemo::executeAction(ThingActionInfo *info)
{
    Thing *thing = info->thing();
    Action action = info->action();

    // Set power
    Q_ASSERT_X(action.actionTypeId() == wemoSwitchPowerActionTypeId, "WemoPlugin", "Unexpected action type in executeAction");

    // Check if wemo thing is reachable
    if (!thing->stateValue(wemoSwitchConnectedStateTypeId).toBool()) {
        info->finish(Thing::ThingErrorHardwareNotAvailable);
        return;
    }

    bool power = action.param(wemoSwitchPowerActionPowerParamTypeId).value().toBool();

    if (thing->stateValue(wemoSwitchPowerStateTypeId).toBool() == power) {
        info->finish(Thing::ThingErrorNoError);
        return;
    }

    QByteArray setPowerMessage("<?xml version=\"1.0\" encoding=\"utf-8\"?><s:Envelope xmlns:s=\"http://schemas.xmlsoap.org/soap/envelope/\" s:encodingStyle=\"http://schemas.xmlsoap.org/soap/encoding/\"><s:Body><u:SetBinaryState xmlns:u=\"urn:Belkin:service:basicevent:1\"><BinaryState>" + QByteArray::number((int)power) + "</BinaryState></u:SetBinaryState></s:Body></s:Envelope>");

    QNetworkRequest request;
    request.setUrl(QUrl("http://" + thing->paramValue(wemoSwitchThingHostParamTypeId).toString() + ":" + thing->paramValue(wemoSwitchThingPortParamTypeId).toString() + "/upnp/control/basicevent1"));
    request.setHeader(QNetworkRequest::ContentTypeHeader,QVariant("text/xml; charset=\"utf-8\""));
    request.setHeader(QNetworkRequest::UserAgentHeader,QVariant("nymea"));
    request.setRawHeader("SOAPACTION", "\"urn:Belkin:service:basicevent:1#SetBinaryState\"");

    QNetworkReply *reply = hardwareManager()->networkManager()->post(request, setPowerMessage);

    connect(reply, &QNetworkReply::finished, reply, &QNetworkReply::deleteLater);

    connect(reply, &QNetworkReply::finished, info, [this, info, reply](){
        if (reply->error() != QNetworkReply::NoError){
            info->finish(Thing::ThingErrorHardwareFailure, QT_TR_NOOP("Could not connect to wemo switch."));
            return;
        }
        QByteArray data = reply->readAll();

        if (data.contains("<BinaryState>1</BinaryState>") || data.contains("<BinaryState>0</BinaryState>")) {
            info->finish(Thing::ThingErrorNoError);
            info->thing()->setStateValue(wemoSwitchConnectedStateTypeId, true);
            refresh(info->thing());
        } else {
            info->finish(Thing::ThingErrorHardwareNotAvailable);
            info->thing()->setStateValue(wemoSwitchConnectedStateTypeId, false);
        }

    });
}

void IntegrationPluginWemo::thingRemoved(Thing *thing)
{
    // Check if there is a missing reply for this thing
    foreach (Thing *d, m_refreshReplies.values()) {
        if (d->id() == thing->id()) {
            QNetworkReply * reply = m_refreshReplies.key(thing);
            m_refreshReplies.remove(reply);
            // Note: delete will be done in networkManagerReplyReady()
        }
    }
}

void IntegrationPluginWemo::refresh(Thing *thing)
{
    QByteArray getBinarayStateMessage("<?xml version=\"1.0\" encoding=\"utf-8\"?><s:Envelope xmlns:s=\"http://schemas.xmlsoap.org/soap/envelope/\" s:encodingStyle=\"http://schemas.xmlsoap.org/soap/encoding/\"><s:Body><u:GetBinaryState xmlns:u=\"urn:Belkin:service:basicevent:1\"><BinaryState>1</BinaryState></u:GetBinaryState></s:Body></s:Envelope>");

    QNetworkRequest request;
    request.setUrl(QUrl("http://" + thing->paramValue(wemoSwitchThingHostParamTypeId).toString() + ":" + thing->paramValue(wemoSwitchThingPortParamTypeId).toString() + "/upnp/control/basicevent1"));
    request.setHeader(QNetworkRequest::ContentTypeHeader,QVariant("text/xml; charset=\"utf-8\""));
    request.setHeader(QNetworkRequest::UserAgentHeader,QVariant("nymea"));
    request.setRawHeader("SOAPACTION", "\"urn:Belkin:service:basicevent:1#GetBinaryState\"");

    QNetworkReply *reply = hardwareManager()->networkManager()->post(request, getBinarayStateMessage);
    connect(reply, &QNetworkReply::finished, this, &IntegrationPluginWemo::onNetworkReplyFinished);
    m_refreshReplies.insert(reply, thing);
}

void IntegrationPluginWemo::processRefreshData(const QByteArray &data, Thing *thing)
{
    if (data.contains("<BinaryState>0</BinaryState>")) {
        thing->setStateValue(wemoSwitchPowerStateTypeId, false);
        thing->setStateValue(wemoSwitchConnectedStateTypeId, true);
    } else if (data.contains("<BinaryState>1</BinaryState>")) {
        thing->setStateValue(wemoSwitchPowerStateTypeId, true);
        thing->setStateValue(wemoSwitchConnectedStateTypeId, true);
    } else {
        thing->setStateValue(wemoSwitchConnectedStateTypeId, false);
    }
}

void IntegrationPluginWemo::onNetworkReplyFinished()
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
        Thing *thing = m_refreshReplies.take(reply);
        processRefreshData(data, thing);
    }

    reply->deleteLater();
}

void IntegrationPluginWemo::onPluginTimer()
{
    foreach (Thing* thing, myThings()) {
        refresh(thing);
    }
}

void IntegrationPluginWemo::onUpnpDiscoveryFinished()
{
}

void IntegrationPluginWemo::onUpnpNotifyReceived(const QByteArray &notification)
{
    Q_UNUSED(notification)
}
