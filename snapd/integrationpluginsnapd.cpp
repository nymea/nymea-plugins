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

#include "integrationpluginsnapd.h"
#include "integrations/thing.h"
#include "plugininfo.h"
#include "network/networkaccessmanager.h"

IntegrationPluginSnapd::IntegrationPluginSnapd()
{

}

IntegrationPluginSnapd::~IntegrationPluginSnapd()
{
    hardwareManager()->pluginTimerManager()->unregisterTimer(m_refreshTimer);
    hardwareManager()->pluginTimerManager()->unregisterTimer(m_updateTimer);
}

void IntegrationPluginSnapd::init()
{
    // Initialize plugin configurations
    m_advancedMode = configValue(snapdPluginAdvancedModeParamTypeId).toBool();
    m_refreshTime = configValue(snapdPluginRefreshScheduleParamTypeId).toInt();
    connect(this, &IntegrationPluginSnapd::configValueChanged, this, &IntegrationPluginSnapd::onPluginConfigurationChanged);

    // Refresh timer for snapd checks
    m_refreshTimer = hardwareManager()->pluginTimerManager()->registerTimer(2);
    connect(m_refreshTimer, &PluginTimer::timeout, this, &IntegrationPluginSnapd::onRefreshTimer);

    // Check all 4 hours if there is an update available
    m_updateTimer = hardwareManager()->pluginTimerManager()->registerTimer(14400);
    connect(m_updateTimer, &PluginTimer::timeout, this, &IntegrationPluginSnapd::onUpdateTimer);
}

void IntegrationPluginSnapd::startMonitoringAutoThings()
{
    // Check if we already have a controller and if snapd is available
    if (m_snapdControl && !m_snapdControl->available()) {
        return;
    }

    // Check if we already have a thing for the snapd control
    bool deviceAlreadyExists = false;
    foreach (Thing *thing, myThings()) {
        if (thing->thingClassId() == snapdControlThingClassId) {
            deviceAlreadyExists = true;
        }
    }

    // Add thing if there isn't already one
    if (!deviceAlreadyExists) {
        ThingDescriptor descriptor(snapdControlThingClassId, "Update manager");
        emit autoThingsAppeared({descriptor});
    }
}

void IntegrationPluginSnapd::postSetupThing(Thing *thing)
{
    if (m_snapdControl && m_snapdControl->thing() == thing) {
        m_snapdControl->update();
    }
}

void IntegrationPluginSnapd::thingRemoved(Thing *thing)
{
    if (thing->thingClassId() == snapdControlThingClassId) {
        qCWarning(dcSnapd()) << "User deleted snapd control.";

        // Clean up old thing
        if (m_snapdControl) {
            delete m_snapdControl;
            m_snapdControl = nullptr;
        }

        // Respawn thing immediately
        startMonitoringAutoThings();

    } else if (thing->thingClassId() == snapThingClassId) {
        if (m_snapDevices.values().contains(thing)) {
            QString snapId = m_snapDevices.key(thing);
            m_snapDevices.remove(snapId);
        }
    }
}

void IntegrationPluginSnapd::setupThing(ThingSetupInfo *info)
{
    Thing *thing = info->thing();
    qCDebug(dcSnapd()) << "Setup" << thing->name() << thing->params();

    if (thing->thingClassId() == snapdControlThingClassId) {
        if (m_snapdControl) {
            delete m_snapdControl;
            m_snapdControl = nullptr;
        }

        m_snapdControl = new SnapdControl(thing, this);
        m_snapdControl->setPreferredRefreshTime(configValue(snapdPluginRefreshScheduleParamTypeId).toInt());
        connect(m_snapdControl, &SnapdControl::snapListUpdated, this, &IntegrationPluginSnapd::onSnapListUpdated);

    } else if (thing->thingClassId() == snapThingClassId) {
        thing->setName(QString("%1").arg(thing->paramValue(snapThingNameParamTypeId).toString()));
        m_snapDevices.insert(thing->paramValue(snapThingIdParamTypeId).toString(), thing);
    }

    info->finish(Thing::ThingErrorNoError);
}

void IntegrationPluginSnapd::executeAction(ThingActionInfo *info)
{
    Thing *thing = info->thing();
    Action action = info->action();

    if (thing->thingClassId() == snapdControlThingClassId) {

        if (!m_snapdControl) {
            qCDebug(dcSnapd()) << "There is currently no snapd controller.";
            return info->finish(Thing::ThingErrorHardwareFailure);
        }

        if (!m_snapdControl->connected()) {
            qCDebug(dcSnapd()) << "Snapd controller not connected to to backend.";
            return info->finish(Thing::ThingErrorHardwareFailure);
        }

        if (action.actionTypeId() == snapdControlStartUpdateActionTypeId) {
            m_snapdControl->snapRefresh();
            return info->finish(Thing::ThingErrorNoError);
        } else if (action.actionTypeId() == snapdControlCheckUpdatesActionTypeId) {
            m_snapdControl->checkForUpdates();
            return info->finish(Thing::ThingErrorNoError);
        }

        return info->finish(Thing::ThingErrorActionTypeNotFound);

    } else if (thing->thingClassId() == snapThingClassId) {

        if (!m_snapdControl) {
            qCDebug(dcSnapd()) << "There is currently no snapd controller.";
            return info->finish(Thing::ThingErrorHardwareFailure);
        }

        if (!m_snapdControl->connected()) {
            qCDebug(dcSnapd()) << "Snapd controller not connected to the backend.";
            return info->finish(Thing::ThingErrorHardwareFailure);
        }

        if (action.actionTypeId() == snapChannelActionTypeId) {
            QString snapName = thing->paramValue(snapThingNameParamTypeId).toString();
            m_snapdControl->changeSnapChannel(snapName, action.param(snapChannelActionChannelParamTypeId).value().toString());
            return info->finish(Thing::ThingErrorNoError);
        } else if (action.actionTypeId() == snapRevertActionTypeId) {
            QString snapName = thing->paramValue(snapThingNameParamTypeId).toString();
            m_snapdControl->snapRevert(snapName);
            return info->finish(Thing::ThingErrorNoError);
        }

        return info->finish(Thing::ThingErrorActionTypeNotFound);
    }

    return info->finish(Thing::ThingErrorThingClassNotFound);
}

void IntegrationPluginSnapd::onPluginConfigurationChanged(const ParamTypeId &paramTypeId, const QVariant &value)
{
    qCDebug(dcSnapd()) << "Plugin configuration changed";

    // Check advanced mode
    if (paramTypeId == snapdPluginAdvancedModeParamTypeId) {
        qCDebug(dcSnapd()) << "Advanced mode" << (value.toBool() ? "enabled." : "disabled.");
        m_advancedMode = value.toBool();

        // If advanced mode disabled, clean up all snap devices
        if (!m_advancedMode) {
            foreach (const QString deviceSnapId, m_snapDevices.keys()) {
                Thing *thing = m_snapDevices.take(deviceSnapId);
                qCDebug(dcSnapd()) << "Remove thing for snap" << thing->paramValue(snapThingNameParamTypeId).toString();
                emit autoThingDisappeared(thing->id());
            }
        } else {
            if (!m_snapdControl)
                return;

            m_snapdControl->update();
        }
    }

    // Check refresh schedule
    if (paramTypeId == snapdPluginRefreshScheduleParamTypeId) {
        if (!m_snapdControl)
            return;

        m_refreshTime = value.toInt();
        qCDebug(dcSnapd()) << "Refresh schedule start time" << QTime(m_refreshTime, 0, 0).toString("hh:mm");
        m_snapdControl->setPreferredRefreshTime(m_refreshTime);
    }
}

void IntegrationPluginSnapd::onRefreshTimer()
{
    if (!m_snapdControl) {
        startMonitoringAutoThings();
        return;
    }

    m_snapdControl->update();
}

void IntegrationPluginSnapd::onUpdateTimer()
{
    if (!m_snapdControl)
        return;

    m_snapdControl->checkForUpdates();
}

void IntegrationPluginSnapd::onSnapListUpdated(const QVariantList &snapList)
{
    // Check for new snap devices only in advanced mode
    if (!m_advancedMode)
        return;

    // Collect list of snap id's from snapList for remove checking
    QStringList snapIdList;

    // Check if we have to create a new thing
    foreach (const QVariant &snapVariant, snapList) {
        QVariantMap snapMap = snapVariant.toMap();
        snapIdList.append(snapMap.value("id").toString());
        // If there is no thing for this snap yet
        if (!m_snapDevices.contains(snapMap.value("id").toString())) {
            ThingDescriptor descriptor(snapThingClassId, QString("Snap %1").arg(snapMap.value("name").toString()));
            ParamList params;
            params.append(Param(snapThingNameParamTypeId, snapMap.value("name")));
            params.append(Param(snapThingIdParamTypeId, snapMap.value("id")));
            params.append(Param(snapThingSummaryParamTypeId, snapMap.value("summary")));
            params.append(Param(snapThingDescriptionParamTypeId, snapMap.value("description")));
            params.append(Param(snapThingDeveloperParamTypeId, snapMap.value("developer")));
            descriptor.setParams(params);

            emit autoThingsAppeared({descriptor});
        } else {
            // Update the states
            Thing *thing = m_snapDevices.value(snapMap.value("id").toString(), nullptr);
            if (!thing) {
                qCWarning(dcSnapd()) << "Holding invalid snap thing. This should never happen. Please report a bug if you see this message.";
                continue;
            }

            thing->setStateValue(snapChannelStateTypeId, snapMap.value("channel").toString());
            thing->setStateValue(snapVersionStateTypeId, snapMap.value("version").toString());
            thing->setStateValue(snapRevisionStateTypeId, snapMap.value("revision").toString());
        }
    }

    // Check if we have to remove a thing
    foreach (const QString deviceSnapId, m_snapDevices.keys()) {
        if (!snapIdList.contains(deviceSnapId)) {
            Thing *thing = m_snapDevices.take(deviceSnapId);
            qCDebug(dcSnapd()) << "The snap" << thing->paramValue(snapThingNameParamTypeId).toString() << "is not installed any more.";
            emit autoThingDisappeared(thing->id());
        }
    }
}
