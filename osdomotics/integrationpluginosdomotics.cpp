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
    Following JSON file contains the definition and the description of all available \l{ThingClass}{DeviceClasses}
    and \l{Vendor}{Vendors} of this \l{DevicePlugin}.

    For more details how to read this JSON file please check out the documentation for \l{The plugin JSON File}.

    \quotefile plugins/deviceplugins/osdomotics/devicepluginosdomotics.json
*/

#include "integrationpluginosdomotics.h"
#include "integrations/thing.h"
#include "plugininfo.h"
#include "network/networkaccessmanager.h"

IntegrationPluginOsdomotics::IntegrationPluginOsdomotics()
{

}

IntegrationPluginOsdomotics::~IntegrationPluginOsdomotics()
{
    hardwareManager()->pluginTimerManager()->unregisterTimer(m_pluginTimer);
}

void IntegrationPluginOsdomotics::init()
{
    m_coap = new Coap(this);
    connect(m_coap, &Coap::replyFinished, this, &IntegrationPluginOsdomotics::coapReplyFinished);

    m_pluginTimer = hardwareManager()->pluginTimerManager()->registerTimer(10);
    connect(m_pluginTimer, &PluginTimer::timeout, this, &IntegrationPluginOsdomotics::onPluginTimer);
}

void IntegrationPluginOsdomotics::setupThing(ThingSetupInfo *info)
{
    Thing *thing = info->thing();

    if (thing->thingClassId() == rplRouterThingClassId) {
        qCDebug(dcOsdomotics) << "Setup RPL router" << thing->paramValue(rplRouterThingRplHostParamTypeId).toString();
        QHostAddress address(thing->paramValue(rplRouterThingRplHostParamTypeId).toString());

        if (address.isNull()) {
            qCWarning(dcOsdomotics) << "Got invalid address" << thing->paramValue(rplRouterThingRplHostParamTypeId).toString();
            //: Error setting up thing
            return info->finish(Thing::ThingErrorInvalidParameter, QT_TR_NOOP("The given RPL address is not valid."));
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
                //: Error setting up thing
                info->finish(Thing::ThingErrorHardwareFailure, QT_TR_NOOP("Error communicating with RPL device."));
                return;
            }

            QByteArray data = reply->readAll();
            parseNodes(info->thing(), data);

            info->finish(Thing::ThingErrorNoError);
        });

        return;
    }

    if (thing->thingClassId() == merkurNodeThingClassId) {
        qCDebug(dcOsdomotics) << "Setup Merkur node" << thing->paramValue(merkurNodeThingHostParamTypeId).toString();
        thing->setParentId(ThingId(thing->paramValue(merkurNodeThingRouterParamTypeId).toString()));
        return info->finish(Thing::ThingErrorNoError);
    }
}

void IntegrationPluginOsdomotics::thingRemoved(Thing *thing)
{
    Q_UNUSED(thing)
}

void IntegrationPluginOsdomotics::postSetupThing(Thing *thing)
{
    updateNode(thing);
}

void IntegrationPluginOsdomotics::executeAction(ThingActionInfo *info)
{
    Thing *thing = info->thing();
    Action action = info->action();

    if (action.actionTypeId() == merkurNodeToggleLedActionTypeId) {
        QUrl url;
        url.setScheme("coap");
        url.setHost(thing->paramValue(merkurNodeThingHostParamTypeId).toString());
        url.setPath("/actuators/toggle");

        qCDebug(dcOsdomotics) << "Toggle light";

        CoapReply *reply = m_coap->post(CoapRequest(url));

        if (reply->isFinished()) {
            if (reply->error() != CoapReply::NoError) {
                qCWarning(dcOsdomotics) << "CoAP reply finished with error" << reply->errorString();
                reply->deleteLater();
                return info->finish(Thing::ThingErrorHardwareNotAvailable);
            }
            return info->finish(Thing::ThingErrorNoError);
        }

        connect(reply, &CoapReply::finished, reply, &CoapReply::deleteLater);

        connect(reply, &CoapReply::finished, info, [info, reply](){
            if (reply->error() != CoapReply::NoError) {
                qCWarning(dcOsdomotics) << "CoAP toggle reply finished with error" << reply->errorString();
                info->finish(Thing::ThingErrorHardwareFailure);
                return;
            }

            info->finish(Thing::ThingErrorNoError);
        });

        return;
    }

    qCWarning(dcOsdomotics()) << "Unhandled executeAction in plugin!";
}

void IntegrationPluginOsdomotics::scanNodes(Thing *thing)
{
    QHostAddress address(thing->paramValue(merkurNodeThingHostParamTypeId).toString());
    qCDebug(dcOsdomotics) << "Scan for new nodes" << address.toString();

    QUrl url;
    url.setScheme("http");
    url.setHost(address.toString());

    QNetworkReply *reply = hardwareManager()->networkManager()->get(QNetworkRequest(url));
    connect(reply, &QNetworkReply::finished, this, &IntegrationPluginOsdomotics::onNetworkReplyFinished);
    m_asyncNodeRescans.insert(reply, thing);
}

void IntegrationPluginOsdomotics::parseNodes(Thing *thing, const QByteArray &data)
{
    //qCDebug(dcOsdomotics) << data;

    // TODO: get all nodes
    //       find better method to get nodes

    int index = data.indexOf("Routes<pre>") + 11;
    int delta = data.indexOf("/128",index);

    QHostAddress nodeAddress(QString(data.mid(index, delta - index)));

    // check if we already have found this node
    foreach (Thing *thing, myThings()) {
        if (thing->paramValue(merkurNodeThingHostParamTypeId).toString() == nodeAddress.toString()) {
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
    m_discoveryRequests.insert(reply, thing);
}

void IntegrationPluginOsdomotics::updateNode(Thing *thing)
{
    qCDebug(dcOsdomotics) << "Update node" << thing->paramValue(merkurNodeThingHostParamTypeId).toString() << "battery value";

    QUrl url;
    url.setScheme("coap");
    url.setHost(thing->paramValue(merkurNodeThingHostParamTypeId).toString());
    url.setPath("/sensors/battery");

    CoapReply *reply = m_coap->get(CoapRequest(url));

    if (reply->isFinished()) {
        if (reply->error() != CoapReply::NoError) {
            qCWarning(dcOsdomotics) << "CoAP reply finished with error" << reply->errorString();
            reply->deleteLater();
        }
    }
    m_updateRequests.insert(reply, thing);
}

Thing *IntegrationPluginOsdomotics::findDevice(const QHostAddress &address)
{
    foreach (Thing *thing, myThings()) {
        if (thing->paramValue(merkurNodeThingHostParamTypeId).toString() == address.toString()) {
            return thing;
        }
    }
    return nullptr;
}

void IntegrationPluginOsdomotics::onPluginTimer()
{
    foreach (Thing *thing, myThings()) {
        if (thing->thingClassId() == merkurNodeThingClassId) {
            updateNode(thing);
        } else if(thing->thingClassId() == rplRouterThingClassId) {
            scanNodes(thing);
        }
    }
}

void IntegrationPluginOsdomotics::onNetworkReplyFinished()
{
    QNetworkReply *reply = static_cast<QNetworkReply *>(sender());
    int status = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();

    if (m_asyncNodeRescans.contains(reply)) {
        Thing *thing = m_asyncSetup.take(reply);

        // check HTTP status code
        if (status != 200) {
            qCWarning(dcOsdomotics) << "Setup reply HTTP error:" << reply->errorString();
            reply->deleteLater();
            return;
        }

        QByteArray data = reply->readAll();
        parseNodes(thing, data);
    }
    reply->deleteLater();
}

void IntegrationPluginOsdomotics::coapReplyFinished(CoapReply *reply)
{
    qCDebug(dcOsdomotics) << "coap reply finished" << reply;

    if (m_discoveryRequests.contains(reply)) {
        Thing *thing = m_discoveryRequests.take(reply);
        if (reply->error() != CoapReply::NoError) {
            qCWarning(dcOsdomotics) << "CoAP discover reply finished with error" << reply->errorString();
            reply->deleteLater();
            return;
        }

        // TODO: parse CoRE links and get the type of the node

        ThingDescriptor descriptor(merkurNodeThingClassId, "Merkur Node", reply->request().url().host());
        ParamList params;
        params.append(Param(merkurNodeThingNameParamTypeId, "Merkur Node"));
        params.append(Param(merkurNodeThingHostParamTypeId,  reply->request().url().host()));
        params.append(Param(merkurNodeThingRouterParamTypeId, thing->id()));
        descriptor.setParams(params);
        emit autoThingsAppeared({descriptor});
    } else if (m_updateRequests.contains(reply)) {
        Thing *thing = m_updateRequests.take(reply);
        if (reply->error() != CoapReply::NoError) {
            qCWarning(dcOsdomotics) << "CoAP update reply finished with error" << reply->errorString();
            reply->deleteLater();
            return;
        }
        int batteryValue = reply->payload().toInt();
        qCDebug(dcOsdomotics) << "Node updated" << batteryValue;
        thing->setStateValue(merkurNodeBatteryStateTypeId, batteryValue);
    }

    reply->deleteLater();
}


