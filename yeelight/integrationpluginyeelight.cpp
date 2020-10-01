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

#include "integrationpluginyeelight.h"

#include "types/param.h"
#include "plugininfo.h"
#include "ssdp.h"

#include <QDebug>
#include <QColor>
#include <QRgb>

IntegrationPluginYeelight::IntegrationPluginYeelight()
{

}

void IntegrationPluginYeelight::init()
{
    m_connectedStateTypeIds.insert(colorBulbThingClassId, colorBulbConnectedStateTypeId);
    m_connectedStateTypeIds.insert(dimmableBulbThingClassId, dimmableBulbConnectedStateTypeId);

    m_powerStateTypeIds.insert(colorBulbThingClassId, colorBulbPowerStateTypeId);
    m_powerStateTypeIds.insert(dimmableBulbThingClassId, dimmableBulbPowerStateTypeId);

    m_brightnessStateTypeIds.insert(colorBulbThingClassId, colorBulbBrightnessStateTypeId);
    m_brightnessStateTypeIds.insert(dimmableBulbThingClassId, dimmableBulbBrightnessStateTypeId);

    m_ssdp = new Ssdp(this);
    m_ssdp->enable();
    m_ssdp->discover();
}

void IntegrationPluginYeelight::discoverThings(ThingDiscoveryInfo *info)
{
    if (info->thingClassId() == colorBulbThingClassId) {


        qCDebug(dcYeelight()) << "Starting UPnP discovery...";
        /*
        connect(ssdp, &Ssdp::discovered, this,[info, this](const QString &address, int port, int id, const QString &model) {

            ThingDescriptor descriptor(colorBulbThingClassId, "Yeelight"+ model + "Bulb", address);
            ParamList params;
            foreach (Thing *existingThing, myThings()) {
                if (existingThing->paramValue(colorBulbThingIdParamTypeId).toString() == id) {
                    descriptor.setThingId(existingThing->id());
                    break;
                }
            }
            params.append(Param(colorBulbThingHostParamTypeId, address));
            params.append(Param(colorBulbThingIdParamTypeId, id));
            params.append(Param(colorBulbThingPortParamTypeId, port));
            descriptor.setParams(params);
            qCDebug(dcYeelight()) << "UPnP: Found Yeelight thing:" << id;
            info->finish(Thing::ThingErrorNoError);
        });*/
    }
}

void IntegrationPluginYeelight::setupThing(ThingSetupInfo *info)
{
    Thing    *thing = info->thing();

    if (thing->thingClassId() == colorBulbThingClassId) {
        QHostAddress address = QHostAddress(thing->paramValue(colorBulbThingHostParamTypeId).toString());
        quint16 port = thing->paramValue(colorBulbThingPortParamTypeId).toUInt();
        Yeelight *yeelight = new Yeelight(hardwareManager()->networkManager(), address, port, this);
        m_yeelightConnections.insert(thing->id(), yeelight);
        connect(yeelight, &Yeelight::connectionChanged, this, &IntegrationPluginYeelight::onConnectionChanged);
        connect(yeelight, &Yeelight::requestExecuted, this, &IntegrationPluginYeelight::onRequestExecuted);
        connect(yeelight, &Yeelight::colorModeNotificationReceived, this, &IntegrationPluginYeelight::onColorModeNotificationReceived);
        connect(yeelight, &Yeelight::powerNotificationReceived, this, &IntegrationPluginYeelight::onPowerNotificationReceived);
        connect(yeelight, &Yeelight::brightnessNotificationReceived, this, &IntegrationPluginYeelight::onBrightnessNotificationReceived);
        connect(yeelight, &Yeelight::colorTemperatureNotificationReceived, this, &IntegrationPluginYeelight::onColorTemperatureNotificationReceived);
        connect(yeelight, &Yeelight::rgbNotificationReceived, this, &IntegrationPluginYeelight::onRgbNotificationReceived);
        connect(yeelight, &Yeelight::nameNotificationReceived, this, &IntegrationPluginYeelight::onNameNotificationReceived);
        info->finish(Thing::ThingErrorNoError);
    }
}

void IntegrationPluginYeelight::postSetupThing(Thing *thing)
{
    connect(thing, &Thing::nameChanged, this, &IntegrationPluginYeelight::onDeviceNameChanged);

    if (!m_pluginTimer) {
        m_pluginTimer = hardwareManager()->pluginTimerManager()->registerTimer(60);
        connect(m_pluginTimer, &PluginTimer::timeout, this, [this]() {
            foreach (ThingId id, m_yeelightConnections.keys()) {
                Thing *thing = myThings().findById(id);
                Yeelight *yeelight = m_yeelightConnections.value(id);
                if (yeelight->isConnected()) {
                    QList<Yeelight::YeelightProperty> properties;
                    if (thing->thingClassId() == colorBulbThingClassId) {
                        properties << Yeelight::YeelightProperty::Ct;
                        properties << Yeelight::YeelightProperty::Rgb;
                        properties << Yeelight::YeelightProperty::Power;
                        properties << Yeelight::YeelightProperty::Bright;
                        properties << Yeelight::YeelightProperty::ColorMode;
                    } else if (thing->thingClassId() == colorBulbThingClassId) {
                        properties << Yeelight::YeelightProperty::Power;
                        properties << Yeelight::YeelightProperty::Bright;
                    }
                    yeelight->getParam(properties);
                }
            }
        });
    }
    m_pluginTimer->timeout();
}

void IntegrationPluginYeelight::executeAction(ThingActionInfo *info)
{
    Thing *thing = info->thing();
    Action action = info->action();
    Yeelight *yeelight = m_yeelightConnections.value(thing->id());
    if (!yeelight)
        return info->finish(Thing::ThingErrorHardwareFailure);

    if (thing->thingClassId() == colorBulbThingClassId) {

        if (action.actionTypeId() == colorBulbPowerActionTypeId) {
            bool power = action.param(colorBulbPowerActionPowerParamTypeId).value().toBool();
            int requestId = yeelight->setPower(power);
            connect(info, &ThingActionInfo::aborted, this, [requestId, this] {m_asyncActions.remove(requestId);});
            m_asyncActions.insert(requestId, info);
        } else if (action.actionTypeId() == colorBulbBrightnessActionTypeId) {

            if (!thing->stateValue(colorBulbPowerStateTypeId).toBool())
                yeelight->setPower(true);

            if (m_pendingBrightnessAction.contains(thing->id())) {
                //Scrap old info and insert new one
                m_pendingBrightnessAction.insert(thing->id(), info);
            } else {
                m_pendingBrightnessAction.insert(thing->id(), info);
                    QTimer::singleShot(500, this, [yeelight, info, this]{
                    int brightness = info->action().param(colorBulbBrightnessActionBrightnessParamTypeId).value().toInt();
                    m_pendingBrightnessAction.remove(info->thing()->id());
                    int requestId = yeelight->setBrightness(brightness);
                    connect(info, &ThingActionInfo::aborted, this, [requestId, this] {m_asyncActions.remove(requestId);});
                    m_asyncActions.insert(requestId, info);
                });
            }
        } else if (action.actionTypeId() == colorBulbColorActionColorParamTypeId) {
            QRgb color = QColor(action.param(colorBulbColorActionColorParamTypeId).value().toString()).rgba();
            if (!thing->stateValue(colorBulbPowerStateTypeId).toBool())
                yeelight->setPower(true);
            int requestId = yeelight->setRgb(color);
            connect(info, &ThingActionInfo::aborted, this, [requestId, this] {m_asyncActions.remove(requestId);});
            m_asyncActions.insert(requestId, info);
        } else if (action.actionTypeId() == colorBulbColorTemperatureActionTypeId) {
            int colorTemperature = 6500 - (action.param(colorBulbColorTemperatureActionColorTemperatureParamTypeId).value().toUInt() * -11.12);
            if (!thing->stateValue(colorBulbPowerStateTypeId).toBool())
                yeelight->setPower(true);
            int requestId = yeelight->setColorTemperature(colorTemperature);
            connect(info, &ThingActionInfo::aborted, this, [requestId, this] {m_asyncActions.remove(requestId);});
            m_asyncActions.insert(requestId, info);
        } else if (action.actionTypeId() == colorBulbEffectActionTypeId) {
            QString effect = action.param(colorBulbEffectActionEffectParamTypeId).value().toString();
            if (!thing->stateValue(colorBulbPowerStateTypeId).toBool())
                yeelight->setPower(true);
            if (effect.contains("non")) {
                int requestId = yeelight->stopColorFlow();
                connect(info, &ThingActionInfo::aborted, this, [requestId, this] {m_asyncActions.remove(requestId);});
                m_asyncActions.insert(requestId, info);
            } else {
                int requestId = yeelight->startColorFlow();
                connect(info, &ThingActionInfo::aborted, this, [requestId, this] {m_asyncActions.remove(requestId);});
                m_asyncActions.insert(requestId, info);
            }
        } else if (action.actionTypeId() == colorBulbAlertActionTypeId) {
            QString alertType = action.param(colorBulbAlertActionAlertParamTypeId).value().toString();
            if (!thing->stateValue(colorBulbPowerStateTypeId).toBool())
                yeelight->setPower(true);
            if (alertType.contains("15")) { //Flash 15 sec
                int requestId = yeelight->flash15s();
                connect(info, &ThingActionInfo::aborted, this, [requestId, this] {m_asyncActions.remove(requestId);});
                m_asyncActions.insert(requestId, info);
            } else {
                int requestId = yeelight->flash();
                connect(info, &ThingActionInfo::aborted, this, [requestId, this] {m_asyncActions.remove(requestId);});
                m_asyncActions.insert(requestId, info);
            }
        } else if (action.actionTypeId() == colorBulbDefaultActionTypeId) {
            int requestId = yeelight->setDefault();
            connect(info, &ThingActionInfo::aborted, this, [requestId, this] {m_asyncActions.remove(requestId);});
            m_asyncActions.insert(requestId, info);
        } else {
            qCWarning(dcYeelight()) << "Unhandled action";
        }
    } else  if (thing->thingClassId() == dimmableBulbThingClassId) {
        if (action.actionTypeId() == dimmableBulbPowerActionTypeId) {
            bool power = action.param(dimmableBulbPowerActionPowerParamTypeId).value().toBool();
            int requestId = yeelight->setPower(power);
            connect(info, &ThingActionInfo::aborted, this, [requestId, this] {m_asyncActions.remove(requestId);});
        } else if (action.actionTypeId() == dimmableBulbBrightnessActionTypeId) {
            int brightness = action.param(dimmableBulbBrightnessActionBrightnessParamTypeId).value().toInt();
            if (!thing->stateValue(dimmableBulbPowerStateTypeId).toBool())
                yeelight->setPower(true);
            int requestId = yeelight->setBrightness(brightness);
            connect(info, &ThingActionInfo::aborted, this, [requestId, this] {m_asyncActions.remove(requestId);});
            m_asyncActions.insert(requestId, info);
        } else if (action.actionTypeId() == dimmableBulbAlertActionTypeId) {
            QString alertType = action.param(dimmableBulbAlertActionAlertParamTypeId).value().toString();
            if (!thing->stateValue(dimmableBulbPowerStateTypeId).toBool())
                yeelight->setPower(true);
            if (alertType.contains("15")) { //Flash 15 sec
                int requestId = yeelight->flash15s();
                connect(info, &ThingActionInfo::aborted, this, [requestId, this] {m_asyncActions.remove(requestId);});
                m_asyncActions.insert(requestId, info);
            } else {
                int requestId = yeelight->flash();
                connect(info, &ThingActionInfo::aborted, this, [requestId, this] {m_asyncActions.remove(requestId);});
                m_asyncActions.insert(requestId, info);
            }
        } else if (action.actionTypeId() == dimmableBulbDefaultActionTypeId) {
            int requestId = yeelight->setDefault();
            connect(info, &ThingActionInfo::aborted, this, [requestId, this] {m_asyncActions.remove(requestId);});
            m_asyncActions.insert(requestId, info);
        } else {
            qCWarning(dcYeelight()) << "Unhandled action";
        }
    } else {
        qCWarning(dcYeelight()) << "Unhandled thing class";
    }
}

void IntegrationPluginYeelight::thingRemoved(Thing *thing)
{
    if ((thing->thingClassId() == colorBulbThingClassId) ||
            (thing->thingClassId() == dimmableBulbThingClassId)){
        Yeelight *yeelight = m_yeelightConnections.take(thing->id());
        yeelight->deleteLater();
    }
}

void IntegrationPluginYeelight::onDeviceNameChanged()
{
    Thing *thing = static_cast<Thing*>(sender());
    Yeelight *yeelight = m_yeelightConnections.value(thing->id());
    yeelight->setName(thing->name());
}

void IntegrationPluginYeelight::onConnectionChanged(bool connected)
{
    Yeelight *yeelight = static_cast<Yeelight *>(sender());
    Thing * thing = myThings().findById(m_yeelightConnections.key(yeelight));
    if(!thing)
        return;

    thing->setStateValue(m_connectedStateTypeIds.value(thing->thingClassId()), connected);
}

void IntegrationPluginYeelight::onRequestExecuted(int requestId, bool success)
{
    if (m_asyncActions.contains(requestId)) {
        ThingActionInfo *info = m_asyncActions.take(requestId);
        if (success) {
            info->finish(Thing::ThingErrorNoError);
        } else {
            info->finish(Thing::ThingErrorHardwareFailure);
        }
    }
}

void IntegrationPluginYeelight::onPowerNotificationReceived(bool status)
{
    Yeelight *yeelight = static_cast<Yeelight *>(sender());
    Thing * thing = myThings().findById(m_yeelightConnections.key(yeelight));
    if(!thing)
        return;

    thing->setStateValue(m_powerStateTypeIds.value(thing->thingClassId()), status);
}

void IntegrationPluginYeelight::onBrightnessNotificationReceived(int percentage)
{
    Yeelight *yeelight = static_cast<Yeelight *>(sender());
    Thing * thing = myThings().findById(m_yeelightConnections.key(yeelight));
    if(!thing)
        return;

    thing->setStateValue(m_brightnessStateTypeIds.value(thing->thingClassId()), percentage);
}

void IntegrationPluginYeelight::onColorTemperatureNotificationReceived(int kelvin)
{
    Yeelight *yeelight = static_cast<Yeelight *>(sender());
    Thing * thing = myThings().findById(m_yeelightConnections.key(yeelight));
    if(!thing)
        return;

    thing->setStateValue(colorBulbColorTemperatureStateTypeId, kelvin/11.12); //TODO needs proper conversion
}

void IntegrationPluginYeelight::onRgbNotificationReceived(QRgb rgbColor)
{
    Yeelight *yeelight = static_cast<Yeelight *>(sender());
    Thing * thing = myThings().findById(m_yeelightConnections.key(yeelight));
    if(!thing)
        return;

    thing->setStateValue(colorBulbColorStateTypeId, rgbColor);
}

void IntegrationPluginYeelight::onNameNotificationReceived(const QString &name)
{
    Yeelight *yeelight = static_cast<Yeelight *>(sender());
    Thing * thing = myThings().findById(m_yeelightConnections.key(yeelight));
    if(!thing)
        return;
    thing->setName(name);
}

void IntegrationPluginYeelight::onColorModeNotificationReceived(Yeelight::YeelightColorMode colorMode)
{
    Yeelight *yeelight = static_cast<Yeelight *>(sender());
    Thing * thing = myThings().findById(m_yeelightConnections.key(yeelight));
    if(!thing)
        return;

    switch (colorMode) {
    case Yeelight::RGB:
        thing->setStateValue(colorBulbColorModeStateTypeId, "RGB");
        break;
    case Yeelight::HSV:
        thing->setStateValue(colorBulbColorModeStateTypeId, "HSV");
        break;
    case Yeelight::ColorTemperature:
        thing->setStateValue(colorBulbColorModeStateTypeId, "Color temperature");
        break;
    }
}
