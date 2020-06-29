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

#include "integrationpluginlifx.h"

#include "integrations/integrationplugin.h"
#include "types/param.h"
#include "plugininfo.h"

#include <QDebug>
#include <QColor>
#include <QRgb>

IntegrationPluginLifx::IntegrationPluginLifx()
{

}

void IntegrationPluginLifx::init()
{
    m_connectedStateTypeIds.insert(colorBulbThingClassId, colorBulbConnectedStateTypeId);
    m_connectedStateTypeIds.insert(dimmableBulbThingClassId, dimmableBulbConnectedStateTypeId);

    m_powerStateTypeIds.insert(colorBulbThingClassId, colorBulbPowerStateTypeId);
    m_powerStateTypeIds.insert(dimmableBulbThingClassId, dimmableBulbPowerStateTypeId);

    m_brightnessStateTypeIds.insert(colorBulbThingClassId, colorBulbBrightnessStateTypeId);
    m_brightnessStateTypeIds.insert(dimmableBulbThingClassId, dimmableBulbBrightnessStateTypeId);
}

void IntegrationPluginLifx::discoverThings(ThingDiscoveryInfo *info)
{
    if (!m_lifxConnection) {
        m_lifxConnection = new Lifx(this);
        m_lifxConnection->enable();
    }

    if (info->thingClassId() == colorBulbThingClassId) {
    } else if () {

    } else {

    }
}

void IntegrationPluginLifx::setupThing(ThingSetupInfo *info)
{
    Thing *thing = info->thing();

    if (!m_lifxConnection) {
        m_lifxConnection = new Lifx(this);
        m_lifxConnection->enable();
    }

    if (thing->thingClassId() == colorBulbThingClassId) {

        info->finish(Thing::ThingErrorNoError);
    }
}

void IntegrationPluginLifx::postSetupThing(Thing *thing)
{
    connect(thing, &Thing::nameChanged, this, &IntegrationPluginLifx::onDeviceNameChanged);

    if (!m_pluginTimer) {
        m_pluginTimer = hardwareManager()->pluginTimerManager()->registerTimer(60);
        connect(m_pluginTimer, &PluginTimer::timeout, this, [this]() {

        });
    }
    m_pluginTimer->timeout();
}

void IntegrationPluginLifx::executeAction(ThingActionInfo *info)
{
    Thing *thing = info->thing();
    Action action = info->action();

    if (!m_lifxConnection)
        return info->finish(Thing::ThingErrorHardwareFailure);

    if (thing->thingClassId() == colorBulbThingClassId) {

        if (action.actionTypeId() == colorBulbPowerActionTypeId) {
            bool power = action.param(colorBulbPowerActionPowerParamTypeId).value().toBool();
            int requestId = m_lifxConnection->setPower(power);
            connect(info, &ThingActionInfo::aborted, this, [requestId, this] {m_asyncActions.remove(requestId);});
            m_asyncActions.insert(requestId, info);
        } else if (action.actionTypeId() == colorBulbBrightnessActionTypeId) {

            if (!thing->stateValue(colorBulbPowerStateTypeId).toBool())
                m_lifxConnection->setPower(true);

            if (m_pendingBrightnessAction.contains(thing->id())) {
                //Scrap old info and insert new one
                m_pendingBrightnessAction.insert(thing->id(), info);
            } else {
                m_pendingBrightnessAction.insert(thing->id(), info);
                    QTimer::singleShot(500, this, [info, this]{
                    int brightness = info->action().param(colorBulbBrightnessActionBrightnessParamTypeId).value().toInt();
                    m_pendingBrightnessAction.remove(info->thing()->id());
                    int requestId = m_lifxConnection->setBrightness(brightness);
                    connect(info, &ThingActionInfo::aborted, this, [requestId, this] {m_asyncActions.remove(requestId);});
                    m_asyncActions.insert(requestId, info);
                });
            }
        } else if (action.actionTypeId() == colorBulbColorActionColorParamTypeId) {
            QRgb color = QColor(action.param(colorBulbColorActionColorParamTypeId).value().toString()).rgba();
            if (!thing->stateValue(colorBulbPowerStateTypeId).toBool())
                m_lifxConnection->setPower(true);
            int requestId = m_lifxConnection->setColor(color);
            connect(info, &ThingActionInfo::aborted, this, [requestId, this] {m_asyncActions.remove(requestId);});
            m_asyncActions.insert(requestId, info);
        } else if (action.actionTypeId() == colorBulbColorTemperatureActionTypeId) {
            int colorTemperature = 6500 - (action.param(colorBulbColorTemperatureActionColorTemperatureParamTypeId).value().toUInt() * -11.12);
            if (!thing->stateValue(colorBulbPowerStateTypeId).toBool())
                m_lifxConnection->setPower(true);
            int requestId = m_lifxConnection->setColorTemperature(colorTemperature);
            connect(info, &ThingActionInfo::aborted, this, [requestId, this] {m_asyncActions.remove(requestId);});
            m_asyncActions.insert(requestId, info);
        } else {
            qCWarning(dcLifx()) << "Unhandled action";
        }
    } else  if (thing->thingClassId() == dimmableBulbThingClassId) {
        if (action.actionTypeId() == dimmableBulbPowerActionTypeId) {
            bool power = action.param(dimmableBulbPowerActionPowerParamTypeId).value().toBool();
            int requestId = m_lifxConnection->setPower(power);
            connect(info, &ThingActionInfo::aborted, this, [requestId, this] {m_asyncActions.remove(requestId);});
        } else if (action.actionTypeId() == dimmableBulbBrightnessActionTypeId) {
            int brightness = action.param(dimmableBulbBrightnessActionBrightnessParamTypeId).value().toInt();
            if (!thing->stateValue(dimmableBulbPowerStateTypeId).toBool())
                m_lifxConnection->setPower(true);
            int requestId = m_lifxConnection->setBrightness(brightness);
            connect(info, &ThingActionInfo::aborted, this, [requestId, this] {m_asyncActions.remove(requestId);});
            m_asyncActions.insert(requestId, info);
        } else {
            qCWarning(dcLifx()) << "Unhandled action";
        }
    } else {
        qCWarning(dcLifx()) << "Unhandled device class";
    }
}

void IntegrationPluginLifx::thingRemoved(Thing *thing)
{
    Q_UNUSED(thing)

    if (myThings().isEmpty()) {
       m_lifxConnection->deleteLater();
       m_lifxConnection = nullptr;
    }
}

void IntegrationPluginLifx::onDeviceNameChanged()
{
    Thing *thing = static_cast<Thing *>(sender());
    Q_UNUSED(thing)
}

void IntegrationPluginLifx::onConnectionChanged(bool connected)
{
  Q_UNUSED(connected)
//    thing->setStateValue(m_connectedStateTypeIds.value(thing->ThingClassId()), connected);
}

void IntegrationPluginLifx::onRequestExecuted(int requestId, bool success)
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

/*void IntegrationPluginLifx::onPowerNotificationReceived(bool status)
{
    Q_UNUSED(status)
    Lifx *Lifx = static_cast<Lifx *>(sender());
    Device * device = myThings().findById(m_LifxConnections.key(Lifx));
    if(!device)
        return;

    thing->setStateValue(m_powerStateTypeIds.value(thing->ThingClassId()), status);

}

void IntegrationPluginLifx::onBrightnessNotificationReceived(int percentage)
{
    Q_UNUSED(percentage)
    Lifx *lifx = static_cast<Lifx *>(sender());
    Device * device = myThings().findById(m_lifxConnections.key(lifx));
    if(!device)
        return;

    thing->setStateValue(m_brightnessStateTypeIds.value(thing->ThingClassId()), percentage);
}*/
