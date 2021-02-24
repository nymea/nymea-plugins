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

#include "integrationpluginsma.h"
#include "plugininfo.h"

IntegrationPluginSma::IntegrationPluginSma()
{

}

void IntegrationPluginSma::discoverThings(ThingDiscoveryInfo *info)
{
    if (info->thingClassId() == sunnyWebBoxThingClassId) {

        Discovery *discovery = new Discovery(this);
        discovery->discoverHosts(25);

        // clean up discovery object when this discovery info is deleted
        connect(info, &ThingDiscoveryInfo::destroyed, discovery, &Discovery::deleteLater);
        connect(discovery, &Discovery::finished, info, [this, info](const QList<Host> &hosts) {
            qCDebug(dcSma()) << "Discovery finished. Found" << hosts.count() << "devices";
            foreach (const Host &host, hosts) {
                if (host.hostName().contains("SMA")){
                    ThingDescriptor descriptor(info->thingClassId(), host.hostName(), host.address() + " (" + host.macAddress() + ")");

                    foreach (Thing *existingThing, myThings()) {
                        if (existingThing->paramValue(sunnyWebBoxThingMacAddressParamTypeId).toString() == host.macAddress()) {
                            descriptor.setThingId(existingThing->id());
                            break;
                        }
                    }
                    ParamList params;
                    params << Param(sunnyWebBoxThingMacAddressParamTypeId, host.macAddress());
                    params << Param(sunnyWebBoxThingHostParamTypeId, host.address());
                    descriptor.setParams(params);

                    info->addThingDescriptor(descriptor);
                }
            }
            info->finish(Thing::ThingErrorNoError);
        });
    }
}

void IntegrationPluginSma::setupThing(ThingSetupInfo *info)
{
    Thing *thing = info->thing();
    qCDebug(dcSma()) << "Setup thing" << thing->name();

    if (!m_refreshTimer) {
        qCDebug(dcSma()) << "Starting refresh timer";
        m_refreshTimer = hardwareManager()->pluginTimerManager()->registerTimer(30);
        connect(m_refreshTimer, &PluginTimer::timeout, this, &IntegrationPluginSma::onRefreshTimer);
    }

    if (thing->thingClassId() == sunnyWebBoxThingClassId) {
        //check if a Sunny WebBox is already added with this IPv4Address
        foreach(SunnyWebBox *sunnyWebBox, m_sunnyWebBoxes.values()) {
            if(sunnyWebBox->hostAddress().toString() == thing->paramValue(sunnyWebBoxThingHostParamTypeId).toString()){
                //this logger at this IPv4 address is already added
                qCWarning(dcSma()) << "thing at " << thing->paramValue(sunnyWebBoxThingHostParamTypeId).toString() << " already added!";
                info->finish(Thing::ThingErrorThingInUse);
                return;
            }
        }
        if (m_sunnyWebBoxes.contains(thing)) {
            qCDebug(dcSma()) << "Setup after reconfiguration, cleaning up...";
            m_sunnyWebBoxes.take(thing)->deleteLater();
        }
        SunnyWebBox *sunnyWebBox = new SunnyWebBox(hardwareManager()->networkManager(), QHostAddress(thing->paramValue(sunnyWebBoxThingHostParamTypeId).toString()), this);
        connect(info, &ThingSetupInfo::aborted, sunnyWebBox, &SunnyWebBox::deleteLater);
        connect(sunnyWebBox, &SunnyWebBox::destroyed, this, [thing, this] { m_sunnyWebBoxes.remove(thing);});
        QString requestId = sunnyWebBox->getPlantOverview();
        connect(sunnyWebBox, &SunnyWebBox::plantOverviewReceived, info, [sunnyWebBox, info, this] {
            qCDebug(dcSma()) << "Received plant overview, finishing setup";
            info->finish(Thing::ThingErrorNoError);
            connect(sunnyWebBox, &SunnyWebBox::plantOverviewReceived, this, &IntegrationPluginSma::onPlantOverviewReceived);
            connect(sunnyWebBox, &SunnyWebBox::devicesReceived, this, &IntegrationPluginSma::onDevicesReceived);
            connect(sunnyWebBox, &SunnyWebBox::processDataReceived, this, &IntegrationPluginSma::onProcessDataReceived);
            connect(sunnyWebBox, &SunnyWebBox::parameterChannelsReceived, this, &IntegrationPluginSma::onParameterChannelsReceived);
            m_sunnyWebBoxes.insert(info->thing(), sunnyWebBox);
        });

    } else if (thing->thingClassId() == inverterThingClassId) {
        Thing *parentThing = myThings().findById(thing->parentId());
        if (!parentThing) {
            qCWarning(dcSma()) << "Could not find parentThing for thing " << thing->name();
            return info->finish(Thing::ThingErrorHardwareNotAvailable, "Please try again");
        }
        if (!parentThing->setupComplete()) {
            //wait for the parent to finish the setup process
            connect(parentThing, &Thing::setupStatusChanged, info, [this, info, parentThing] {

                if (parentThing->setupComplete())
                    setupChild(info, parentThing);
            });
            return;
        }
        setupChild(info, parentThing);
    } else {
        Q_ASSERT_X(false, "setupThing", QString("Unhandled thingClassId: %1").arg(thing->thingClassId().toString()).toUtf8());
    }
}

void IntegrationPluginSma::postSetupThing(Thing *thing)
{
    qCDebug(dcSma()) << "Post setup thing" << thing->name();
    if (thing->thingClassId() == sunnyWebBoxThingClassId) {
        SunnyWebBox *sunnyWebBox = m_sunnyWebBoxes.value(thing);
        if (!sunnyWebBox)
            return;
        sunnyWebBox->getDevices();
        thing->setStateValue(sunnyWebBoxConnectedStateTypeId, true);
    } else if (thing->thingClassId() == inverterThingClassId) {

    }
}

void IntegrationPluginSma::executeAction(ThingActionInfo *info)
{
    Thing *thing = info->thing();
    Action action = info->action();

    if (thing->thingClassId() == sunnyWebBoxThingClassId) {
        SunnyWebBox *sunnyWebBox = m_sunnyWebBoxes.value(thing);
        if (!sunnyWebBox)
            return;
        if (action.actionTypeId() == sunnyWebBoxSearchDevicesActionTypeId) {
            QString requestId = sunnyWebBox->getDevices();
            if (requestId.isEmpty()) {
                return info->finish(Thing::ThingErrorHardwareNotAvailable);
            }
            m_asyncActions.insert(requestId, info);
            connect(info, &ThingActionInfo::aborted, info, [requestId, this] {m_asyncActions.remove(requestId);});
        } else {
            Q_ASSERT_X(false, "executeAction", QString("Unhandled actionTypeId: %1").arg(action.actionTypeId().toString()).toUtf8());
        }
    } else {
        Q_ASSERT_X(false, "executeAction", QString("Unhandled thingClassId: %1").arg(thing->thingClassId().toString()).toUtf8());
    }
}

void IntegrationPluginSma::thingRemoved(Thing *thing)
{
    if (thing->thingClassId() == sunnyWebBoxThingClassId) {
        m_sunnyWebBoxes.take(thing)->deleteLater();
    }

    if (myThings().isEmpty()) {
        qCDebug(dcSma()) << "Stopping timer";
        hardwareManager()->pluginTimerManager()->unregisterTimer(m_refreshTimer);
        m_refreshTimer = nullptr;
    }
}

void IntegrationPluginSma::onRefreshTimer()
{
    Q_FOREACH(SunnyWebBox *sunnyWebBox, m_sunnyWebBoxes) {
        sunnyWebBox->getPlantOverview();
    }
}

void IntegrationPluginSma::onPlantOverviewReceived(const QString &messageId, SunnyWebBox::Overview overview)
{
    qCDebug(dcSma()) << "Plant overview received" << overview.status;
    if (m_asyncSetup.contains(messageId)) {
        ThingSetupInfo *info = m_asyncSetup.value(messageId);
        info->finish(Thing::ThingErrorNoError);
    }

    Thing *thing = m_sunnyWebBoxes.key(static_cast<SunnyWebBox *>(sender()));
    if (!thing)
        return;

    thing->setStateValue(sunnyWebBoxConnectedStateTypeId, true);
    thing->setStateValue(sunnyWebBoxCurrentPowerStateTypeId, overview.power);
    thing->setStateValue(sunnyWebBoxDayEnergyStateTypeId, overview.dailyYield);
    thing->setStateValue(sunnyWebBoxTotalEnergyStateTypeId, overview.totalYield);
    thing->setStateValue(sunnyWebBoxModeStateTypeId, overview.status);
    if (!overview.error.isEmpty()){
        qCDebug(dcSma()) << "Received error" << overview.error;
        thing->setStateValue(sunnyWebBoxErrorStateTypeId, overview.error);
    }
}

void IntegrationPluginSma::onDevicesReceived(const QString &messageId, QList<SunnyWebBox::Device> devices)
{
    qCDebug(dcSma()) << "Devices received, count:" << devices.count();
    if (m_asyncActions.contains(messageId)) {
        ThingActionInfo *info = m_asyncActions.value(messageId);
        info->finish(Thing::ThingErrorNoError);
    }

    Thing *thing = m_sunnyWebBoxes.key(static_cast<SunnyWebBox *>(sender()));
    if (!thing)
        return;

    ThingDescriptors descriptors;
    Q_FOREACH(SunnyWebBox::Device device, devices){
        qCDebug(dcSma()) << "   - Device received" << device.name << device.key;
        ThingDescriptor descriptor(inverterThingClassId, device.name, device.key ,thing->id());
        descriptors.append(descriptor);
    }
    emit autoThingsAppeared(descriptors);
}

void IntegrationPluginSma::onProcessDataReceived(const QString &messageId, const QString &deviceKey, const QHash<QString, QVariant> &channels)
{
    Q_UNUSED(messageId)
    Thing *thing = m_sunnyWebBoxes.key(static_cast<SunnyWebBox *>(sender()));
    if (!thing)
        return;

    qCDebug(dcSma()) << "Process data received" << deviceKey;

    Q_FOREACH(Thing *childThing, myThings().filterByParentId(thing->id())) {
        if (childThing->paramValue(inverterThingIdParamTypeId).toString() == deviceKey) {
            if (channels.contains("E-Total")) {
                //TODO set total energy
            }
            break;
        }
    }
}

void IntegrationPluginSma::onParameterChannelsReceived(const QString &messageId, const QString &deviceKey, QStringList parameterChannels)
{
    Q_UNUSED(messageId)

    Thing *thing = m_sunnyWebBoxes.key(static_cast<SunnyWebBox *>(sender()));
    if (!thing)
        return;

    qCDebug(dcSma()) << "Parameter channels received" << deviceKey << parameterChannels;
}

void IntegrationPluginSma::setupChild(ThingSetupInfo *info, Thing *parentThing)
{
    Q_UNUSED(info)
    Q_UNUSED(parentThing)
}

void IntegrationPluginSma::getData(Thing *thing)
{
    Q_UNUSED(thing)
}
