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

#include "integrationpluginboblight.h"

#include "integrations/thing.h"

#include "bobclient.h"
#include "plugininfo.h"
#include "plugintimer.h"

#include <QDebug>
#include <QStringList>
#include <QtMath>

IntegrationPluginBoblight::IntegrationPluginBoblight()
{
}

void IntegrationPluginBoblight::init()
{
    m_pluginTimer = hardwareManager()->pluginTimerManager()->registerTimer(15);
    connect(m_pluginTimer, &PluginTimer::timeout, this, &IntegrationPluginBoblight::guhTimer);
}

void IntegrationPluginBoblight::thingRemoved(Thing *thing)
{
    BobClient *client = m_bobClients.take(thing->id());
    if (thing->thingClassId() == boblightServerThingClassId) {
        client->deleteLater();
    }
}

void IntegrationPluginBoblight::startMonitoringAutoThings()
{
    m_canCreateAutoDevices = true;
    qCDebug(dcBoblight()) << "Populating auto devices" << myThings().count();
    QHash<ThingId, int> parentDevices;
    QHash<ThingId, Thing*> deviceIds;
    foreach (Thing *thing, myThings()) {
        deviceIds.insert(thing->id(), thing);
        if (thing->thingClassId() == boblightServerThingClassId) {
//            qWarning() << "Device" << thing->id() << "is bridge";
            if (!parentDevices.contains(thing->id())) {
                parentDevices[thing->id()] = 0;
            }
        } else if (thing->thingClassId() == boblightThingClassId) {
//            qWarning() << "Device" << thing->id() << "is child to bridge" << thing->parentId();
            parentDevices[thing->parentId()] += 1;
        }
    }
    QList<ThingDescriptor> descriptors;
    foreach (const ThingId &id, parentDevices.keys()) {
        if (parentDevices.value(id) < deviceIds.value(id)->paramValue(boblightServerThingChannelsParamTypeId).toInt()) {
            for (int i = parentDevices.value(id); i < deviceIds.value(id)->paramValue(boblightServerThingChannelsParamTypeId).toInt(); i++) {
                ThingDescriptor descriptor(boblightThingClassId, deviceIds.value(id)->name() + " " + QString::number(i + 1), QString(), id);
                descriptor.setParams(ParamList() << Param(boblightThingChannelParamTypeId, i));
                qCDebug(dcBoblight()) << "Adding new boblight channel" << i + 1;
                descriptors.append(descriptor);
            }
        }
    }
    if (!descriptors.isEmpty()) {
        emit autoThingsAppeared(descriptors);
    }
}

void IntegrationPluginBoblight::guhTimer()
{
    foreach (BobClient *client, m_bobClients) {
        if (!client->connected()) {
            client->connectToBoblight();
        }
    }
}

void IntegrationPluginBoblight::onPowerChanged(int channel, bool power)
{
    qCDebug(dcBoblight()) << "power changed" << channel << power;
    BobClient *sndr = dynamic_cast<BobClient*>(sender());
    foreach (Thing* thing, myThings()) {
        if (m_bobClients.value(thing->parentId()) == sndr && thing->paramValue(boblightThingChannelParamTypeId).toInt() == channel) {
            qCDebug(dcBoblight()) << "setting state power" << power;
            thing->setStateValue(boblightPowerStateTypeId, power);
        }
    }
}

void IntegrationPluginBoblight::onBrightnessChanged(int channel, int brightness)
{
    BobClient *sndr = dynamic_cast<BobClient*>(sender());
    foreach (Thing* thing, myThings()) {
        if (m_bobClients.value(thing->parentId()) == sndr && thing->paramValue(boblightThingChannelParamTypeId).toInt() == channel) {
            thing->setStateValue(boblightBrightnessStateTypeId, brightness);
        }
    }
}

void IntegrationPluginBoblight::onColorChanged(int channel, const QColor &color)
{
    BobClient *sndr = dynamic_cast<BobClient*>(sender());
    foreach (Thing* thing, myThings()) {
        if (m_bobClients.value(thing->parentId()) == sndr && thing->paramValue(boblightThingChannelParamTypeId).toInt() == channel) {
            thing->setStateValue(boblightColorStateTypeId, color);
        }
    }
}

void IntegrationPluginBoblight::onPriorityChanged(int priority)
{
    BobClient *sndr = dynamic_cast<BobClient*>(sender());
    foreach (Thing* thing, myThings()) {
        if (thing->thingClassId() == boblightServerThingClassId && m_bobClients.value(thing->id()) == sndr) {
            thing->setStateValue(boblightServerPriorityStateTypeId, priority);
        }
    }
}

QColor IntegrationPluginBoblight::tempToRgb(int temp)
{
    //  153   cold: 0.839216, 1, 0.827451
    //  500   warm: 0.870588, 1, 0.266667

    //   =>  0 : 214,255,212
    //       100 : 222,255,67

    //  r => 0 : 214 = 100 : 222
    //    => temp : (x-214) = 100 : (255 - 214)
    //    => x = temp * (255-214) / 100
    //       r = x + 214

    int red = temp * 41 / 100 + 214;

    int green = 255;

    //  b => 0 : 212 = 100 : 67
    //    => temp : (212 - x) = 100 : (212 - 145)
    //    => x = temp * 145 / 100
    //       g = 212 - x

    int blue = 212 - temp * 145 / 100;

    qWarning() << "temp:" << temp << "rgb" << red << green << blue;
    return QColor(red, green, blue);
}

void IntegrationPluginBoblight::setupThing(ThingSetupInfo *info)
{
    Thing *thing = info->thing();

    if (thing->thingClassId() == boblightServerThingClassId) {

        BobClient *bobClient = new BobClient(thing->paramValue(boblightServerThingHostAddressParamTypeId).toString(), thing->paramValue(boblightServerThingPortParamTypeId).toInt(), this);
        bool connected = bobClient->connectToBoblight();
        if (!connected) {
            qCWarning(dcBoblight()) << "Error connecting to boblight on" << thing->paramValue(boblightServerThingHostAddressParamTypeId).toString();
        } else {
            qCDebug(dcBoblight()) << "Connected to boblight";
        }
        bobClient->setPriority(thing->stateValue(boblightServerPriorityStateTypeId).toInt());
        thing->setStateValue(boblightServerConnectedStateTypeId, connected);
        m_bobClients.insert(thing->id(), bobClient);
        connect(bobClient, &BobClient::connectionChanged, this, &IntegrationPluginBoblight::onConnectionChanged);
        connect(bobClient, &BobClient::powerChanged, this, &IntegrationPluginBoblight::onPowerChanged);
        connect(bobClient, &BobClient::brightnessChanged, this, &IntegrationPluginBoblight::onBrightnessChanged);
        connect(bobClient, &BobClient::colorChanged, this, &IntegrationPluginBoblight::onColorChanged);
        connect(bobClient, &BobClient::priorityChanged, this, &IntegrationPluginBoblight::onPriorityChanged);
    } else if (thing->thingClassId() == boblightThingClassId) {
        BobClient *bobClient = m_bobClients.value(thing->parentId());
        thing->setStateValue(boblightConnectedStateTypeId, bobClient->connected());
        m_bobClients.insert(thing->id(), bobClient);
    }

    info->finish(Thing::ThingErrorNoError);
}

void IntegrationPluginBoblight::postSetupThing(Thing *thing)
{
    if (thing->thingClassId() == boblightServerThingClassId && m_canCreateAutoDevices) {
        startMonitoringAutoThings();
    }
    if (thing->thingClassId() == boblightThingClassId) {
        BobClient *bobClient = m_bobClients.value(thing->parentId());
        if (bobClient && bobClient->connected()) {
            thing->setStateValue(boblightConnectedStateTypeId, bobClient->connected());

            QColor color = thing->stateValue(boblightColorStateTypeId).value<QColor>();
            int brightness = thing->stateValue(boblightBrightnessStateTypeId).toInt();
            bool power = thing->stateValue(boblightPowerStateTypeId).toBool();

            bobClient->setColor(thing->paramValue(boblightThingChannelParamTypeId).toInt(), color);
            bobClient->setBrightness(thing->paramValue(boblightThingChannelParamTypeId).toInt(), brightness);
            bobClient->setPower(thing->paramValue(boblightThingChannelParamTypeId).toInt(), power);

        }
    }
}

void IntegrationPluginBoblight::executeAction(ThingActionInfo *info)
{
    Thing *thing = info->thing();
    Action action = info->action();

    qCDebug(dcBoblight()) << "Execute action for boblight" << action.params();
    if (thing->thingClassId() == boblightServerThingClassId) {
        BobClient *bobClient = m_bobClients.value(thing->id());
        if (!bobClient || !bobClient->connected()) {
            qCWarning(dcBoblight()) << "Boblight on" << thing->paramValue(boblightServerThingHostAddressParamTypeId).toString() << "not connected";
            info->finish(Thing::ThingErrorHardwareNotAvailable);
            return;
        }

        if (action.actionTypeId() == boblightServerPriorityActionTypeId) {
            bobClient->setPriority(action.param(boblightServerPriorityActionPriorityParamTypeId).value().toInt());
            info->finish(Thing::ThingErrorNoError);
            return;
        }
        qCWarning(dcBoblight()) << "Unhandled action" << action.actionTypeId() << "for BoblightServer" << info;
        info->finish(Thing::ThingErrorActionTypeNotFound);
        return;
    }

    if (thing->thingClassId() == boblightThingClassId) {
        BobClient *bobClient = m_bobClients.value(thing->parentId());
        if (!bobClient || !bobClient->connected()) {
            qCWarning(dcBoblight()) << "Boblight not connected";
            info->finish(Thing::ThingErrorHardwareNotAvailable);
            return;
        }

        if (action.actionTypeId() == boblightPowerActionTypeId) {
            bobClient->setPower(thing->paramValue(boblightThingChannelParamTypeId).toInt(), action.param(boblightPowerActionPowerParamTypeId).value().toBool());
            info->finish(Thing::ThingErrorNoError);
            return;
        }
        if (action.actionTypeId() == boblightColorActionTypeId) {
            bobClient->setColor(thing->paramValue(boblightThingChannelParamTypeId).toInt(), action.param(boblightColorActionColorParamTypeId).value().value<QColor>());
            info->finish(Thing::ThingErrorNoError);
            return;
        }
        if (action.actionTypeId() == boblightBrightnessActionTypeId) {
            bobClient->setBrightness(thing->paramValue(boblightThingChannelParamTypeId).toInt(), action.param(boblightBrightnessActionBrightnessParamTypeId).value().toInt());
            info->finish(Thing::ThingErrorNoError);
            return;
        }
        if (action.actionTypeId() == boblightColorTemperatureActionTypeId) {
            bobClient->setColor(thing->paramValue(boblightThingChannelParamTypeId).toInt(), tempToRgb(action.param(boblightColorTemperatureActionColorTemperatureParamTypeId).value().toInt()));
            info->finish(Thing::ThingErrorNoError);
            return;
        }
        info->finish(Thing::ThingErrorActionTypeNotFound);
        return;
    }
    info->finish(Thing::ThingErrorThingClassNotFound);
}

void IntegrationPluginBoblight::onConnectionChanged()
{
    BobClient *bobClient = static_cast<BobClient *>(sender());
    qCDebug(dcBoblight()) << "Connection changed. BobClient:" << bobClient << bobClient->connected() << m_bobClients.keys(bobClient);
    foreach (const ThingId &thingId, m_bobClients.keys(bobClient)) {
        foreach (Thing *thing, myThings()) {
            if (thing->id() == thingId || thing->parentId() == thingId) {
                thing->setStateValue(boblightConnectedStateTypeId, bobClient->connected());
                break;
            }
        }
    }
}

