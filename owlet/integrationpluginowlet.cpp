/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*
* Copyright 2013 - 2021, nymea GmbH
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

#include "integrationpluginowlet.h"
#include "plugininfo.h"
#include "owletclient.h"

#include "hardwaremanager.h"
#include "platform/platformzeroconfcontroller.h"
#include "network/zeroconf/zeroconfservicebrowser.h"
#include "network/zeroconf/zeroconfserviceentry.h"

#include "owlettcptransport.h"
#include "owletserialtransport.h"

#include <QColor>
#include <QTimer>

IntegrationPluginOwlet::IntegrationPluginOwlet()
{
}

void IntegrationPluginOwlet::init()
{
    m_owletIdParamTypeMap.insert(digitalOutputThingClassId, digitalOutputThingOwletIdParamTypeId);
    m_owletIdParamTypeMap.insert(digitalInputThingClassId, digitalInputThingOwletIdParamTypeId);
    m_owletIdParamTypeMap.insert(ws2812ThingClassId, ws2812ThingOwletIdParamTypeId);

    m_owletSerialPortParamTypeMap.insert(arduinoMiniProThingClassId, arduinoMiniProThingSerialPortParamTypeId);

    m_zeroConfBrowser = hardwareManager()->zeroConfController()->createServiceBrowser("_nymea-owlet._tcp");
}

void IntegrationPluginOwlet::discoverThings(ThingDiscoveryInfo *info)
{
    if (info->thingClassId() == arduinoMiniProThingClassId) {
        // Discover serial ports for arduino bords
        foreach(QSerialPortInfo port, QSerialPortInfo::availablePorts()) {
            qCDebug(dcOwlet()) << "Found serial port" << port.systemLocation();
            QString description = port.systemLocation() + " " + port.manufacturer() + " " + port.description();
            ThingDescriptor thingDescriptor(info->thingClassId(), "Owlet Arduino Pro Mini", description);
            ParamList parameters;

            foreach (Thing *existingThing, myThings()) {
                if (existingThing->paramValue(m_owletSerialPortParamTypeMap.value(info->thingClassId())).toString() == port.systemLocation()) {
                    thingDescriptor.setThingId(existingThing->id());
                    break;
                }
            }

            parameters.append(Param(m_owletSerialPortParamTypeMap.value(info->thingClassId()), port.systemLocation()));
            thingDescriptor.setParams(parameters);
            info->addThingDescriptor(thingDescriptor);
        }
        info->finish(Thing::ThingErrorNoError);
    } else {
        foreach (const ZeroConfServiceEntry &entry, m_zeroConfBrowser->serviceEntries()) {
            qCDebug(dcOwlet()) << "Found owlet:" << entry;
            ThingDescriptor descriptor(info->thingClassId(), entry.name(), entry.txt("platform"));
            descriptor.setParams(ParamList() << Param(m_owletIdParamTypeMap.value(info->thingClassId()), entry.txt("id")));
            foreach (Thing *existingThing, myThings().filterByParam(m_owletIdParamTypeMap.value(info->thingClassId()), entry.txt("id"))) {
                descriptor.setThingId(existingThing->id());
                break;
            }
            info->addThingDescriptor(descriptor);
        }
        info->finish(Thing::ThingErrorNoError);
    }
}


void IntegrationPluginOwlet::setupThing(ThingSetupInfo *info)
{
    Thing *thing = info->thing();
    OwletTransport *transport = nullptr;
    QHostAddress ip;
    int port = 5555;

    if (thing->thingClassId() != arduinoMiniProThingClassId) {
        foreach (const ZeroConfServiceEntry &entry, m_zeroConfBrowser->serviceEntries()) {
            if (entry.txt("id") == info->thing()->paramValue(m_owletIdParamTypeMap.value(info->thing()->thingClassId()))) {
                ip = entry.hostAddress();
                port = entry.port();
                break;
            }
        }
        // Try cached ip
        if (ip.isNull()) {
            pluginStorage()->beginGroup(thing->id().toString());
            ip = QHostAddress(pluginStorage()->value("cachedIP").toString());
            pluginStorage()->endGroup();
        }

        if (ip.isNull()) {
            qCWarning(dcOwlet()) << "Can't find owlet in the local network.";
            info->finish(Thing::ThingErrorHardwareNotAvailable);
            return;
        }

        transport = new OwletTcpTransport(ip, port, this);

    } else if (thing->thingClassId() == arduinoMiniProThingClassId) {
        QString serialPort = thing->paramValue(arduinoMiniProThingSerialPortParamTypeId).toString();
        qCDebug(dcOwlet()) << "Setup arduino mini pro owlet on" << serialPort;
        transport = new OwletSerialTransport(serialPort, 115200, this);
    }

    OwletClient *client = new OwletClient(transport, this);
    connect(client, &OwletClient::connected, info, [=](){
        qCDebug(dcOwlet()) << "Connected to owlet";
        m_clients.insert(thing, client);

        if (thing->thingClassId() == digitalOutputThingClassId) {
            QVariantMap params;
            params.insert("id", thing->paramValue(digitalOutputThingPinParamTypeId).toInt());
            params.insert("mode", "GPIOOutput");
            client->sendCommand("GPIO.ConfigurePin", params);
        }
        if (thing->thingClassId() == digitalInputThingClassId) {
            QVariantMap params;
            params.insert("id", thing->paramValue(digitalInputThingPinParamTypeId).toInt());
            params.insert("mode", "GPIOInput");
            client->sendCommand("GPIO.ConfigurePin", params);
        }
        if (thing->thingClassId() == ws2812ThingClassId) {
            QVariantMap params;
            params.insert("id", thing->paramValue(ws2812ThingPinParamTypeId).toInt());
            params.insert("mode", "WS2812");
            params.insert("ledCount", thing->paramValue(ws2812ThingLedCountParamTypeId).toUInt());
            params.insert("ledMode", "WS2812Mode" + thing->paramValue(ws2812ThingLedModeParamTypeId).toString());
            params.insert("ledClock", "WS2812Clock" + thing->paramValue(ws2812ThingLedClockParamTypeId).toString());
            client->sendCommand("GPIO.ConfigurePin", params);
        }

        info->finish(Thing::ThingErrorNoError);
    });

    connect(client, &OwletClient::error, info, [=](){
        info->finish(Thing::ThingErrorHardwareFailure);
    });

    connect(client, &OwletClient::connected, thing, [=](){
        thing->setStateValue("connected", true);

        // FIXME: find a better way
        if (thing->thingClassId() != arduinoMiniProThingClassId) {
            pluginStorage()->beginGroup(thing->id().toString());
            pluginStorage()->setValue("cachedIP", ip.toString());
            pluginStorage()->endGroup();
        }

        qCDebug(dcOwlet()) << "Sending get platform information request...";
        int id = client->sendCommand("Platform.GetInformation");
        connect(client, &OwletClient::replyReceived, thing, [=](int commandId, const QVariantMap &params){
            if (id != commandId)
                return;

            qCDebug(dcOwlet()) << "Reply from owlet platform information:" << params;
        });
    });

    connect(client, &OwletClient::disconnected, thing, [=](){
        thing->setStateValue("connected", false);
    });

    connect(client, &OwletClient::notificationReceived, this, [=](const QString &name, const QVariantMap &params){
        qCDebug(dcOwlet()) << "***Notif" << name << params;
        if (thing->thingClassId() == digitalInputThingClassId) {
            if (params.value("id").toInt() == thing->paramValue(digitalInputThingPinParamTypeId)) {
                thing->setStateValue(digitalInputPowerStateTypeId, params.value("power").toBool());
            }
        }
        if (thing->thingClassId() == digitalOutputThingClassId) {
            if (params.value("id").toInt() == thing->paramValue(digitalOutputThingPinParamTypeId)) {
                thing->setStateValue(digitalOutputPowerStateTypeId, params.value("power").toBool());
            }
        }
        if (thing->thingClassId() == ws2812ThingClassId) {
            if (name == "GPIO.PinChanged") {
                if (params.contains("power")) {
                    thing->setStateValue(ws2812PowerStateTypeId, params.value("power").toBool());
                }
                if (params.contains("brightness")) {
                    thing->setStateValue(ws2812BrightnessStateTypeId, params.value("brightness").toInt());
                }
                if (params.contains("color")) {
                    thing->setStateValue(ws2812ColorStateTypeId, params.value("color").value<QColor>());
                }
                if (params.contains("effect")) {
                    thing->setStateValue(ws2812EffectStateTypeId, params.value("effect").toInt());
                }
            }
        }
    });

    client->transport()->connectTransport();
}


void IntegrationPluginOwlet::executeAction(ThingActionInfo *info)
{
    if (info->thing()->thingClassId() == digitalOutputThingClassId) {
        OwletClient *client = m_clients.value(info->thing());
        QVariantMap params;
        params.insert("id", info->thing()->paramValue(digitalOutputThingPinParamTypeId).toInt());
        params.insert("power", info->action().paramValue(digitalOutputPowerActionPowerParamTypeId).toBool());
        qCDebug(dcOwlet()) << "Sending ControlPin" << params;
        int id = client->sendCommand("GPIO.ControlPin", params);
        connect(client, &OwletClient::replyReceived, info, [=](int commandId, const QVariantMap &params){
            if (id != commandId) {
                return;
            }
            qCDebug(dcOwlet()) << "reply from owlet:" << params;
            QString error = params.value("error").toString();
            if (error == "GPIOErrorNoError") {
                info->thing()->setStateValue(digitalOutputPowerStateTypeId, info->action().paramValue(digitalOutputPowerActionPowerParamTypeId).toBool());
                info->finish(Thing::ThingErrorNoError);
            } else {
                info->finish(Thing::ThingErrorHardwareFailure);
            }
        });

        return;
    }

    if (info->thing()->thingClassId() == ws2812ThingClassId) {
        OwletClient *client = m_clients.value(info->thing());
        QVariantMap params;
        params.insert("id", info->thing()->paramValue(ws2812ThingPinParamTypeId).toUInt());

        if (info->action().actionTypeId() == ws2812PowerActionTypeId) {
            params.insert("power", info->action().paramValue(ws2812PowerActionPowerParamTypeId).toBool());
        }
        if (info->action().actionTypeId() == ws2812BrightnessActionTypeId) {
            params.insert("brightness", info->action().paramValue(ws2812BrightnessActionBrightnessParamTypeId).toInt());
        }
        if (info->action().actionTypeId() == ws2812ColorActionTypeId) {
            QColor color = info->action().paramValue(ws2812ColorActionColorParamTypeId).value<QColor>();
            params.insert("color", (color.rgb() & 0xFFFFFF));
        }
        if (info->action().actionTypeId() == ws2812EffectActionTypeId) {
            int effect = info->action().paramValue(ws2812EffectActionEffectParamTypeId).toInt();
            params.insert("effect", effect);
        }

        int id = client->sendCommand("GPIO.ControlPin", params);
        connect(client, &OwletClient::replyReceived, info, [=](int commandId, const QVariantMap &params){
            if (id != commandId) {
                return;
            }
            qCDebug(dcOwlet()) << "reply from owlet:" << params;
            QString error = params.value("error").toString();
            if (error == "GPIOErrorNoError") {
                info->finish(Thing::ThingErrorNoError);
            } else {
                info->finish(Thing::ThingErrorHardwareFailure);
            }
        });
        return;
    }



    Q_ASSERT_X(false, "IntegrationPluginOwlet", "Not implemented");
    info->finish(Thing::ThingErrorUnsupportedFeature);
}

void IntegrationPluginOwlet::thingRemoved(Thing *thing)
{
    Q_UNUSED(thing)
}
