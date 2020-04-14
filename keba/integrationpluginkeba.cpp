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

#include "integrationpluginkeba.h"
#include "plugininfo.h"

#include <QUrl>
#include <QUrlQuery>
#include <QJsonDocument>
#include <QUdpSocket>
#include <QTimeZone>

IntegrationPluginKeba::IntegrationPluginKeba()
{

}

void IntegrationPluginKeba::discoverThings(ThingDiscoveryInfo *info)
{
    if (info->thingClassId() == wallboxThingClassId) {
        Discovery *discovery = new Discovery(info);
        discovery->discoverHosts(25);

        connect(discovery, &Discovery::finished, info, [this, info](const QList<Host> &hosts) {
            qCDebug(dcKebaKeContact()) << "Discovery finished. Found" << hosts.count() << "devices";
            foreach (const Host &host, hosts) {
                if (!host.hostName().contains("keba", Qt::CaseSensitivity::CaseInsensitive))
                    continue;

                ThingDescriptor descriptor(wallboxThingClassId, "Wallbox", host.address() + " (" + host.macAddress() + ")");

                foreach (Thing *existingThing, myThings()) {
                    if (existingThing->paramValue(wallboxThingMacAddressParamTypeId).toString() == host.macAddress()) {
                        descriptor.setThingId(existingThing->id());
                        break;
                    }
                }
                ParamList params;
                params << Param(wallboxThingMacAddressParamTypeId, host.macAddress());
                params << Param(wallboxThingIpAddressParamTypeId, host.address());
                descriptor.setParams(params);
                info->addThingDescriptor(descriptor);
            }
            info->finish(Thing::ThingErrorNoError);
        });
    } else {
        qCWarning(dcKebaKeContact()) << "Discover device, unhandled device class" << info->thingClassId();
        info->finish(Thing::ThingErrorThingClassNotFound);
    }
}

void IntegrationPluginKeba::setupThing(ThingSetupInfo *info)
{
    Thing *thing = info->thing();

    qCDebug(dcKebaKeContact()) << "Setting up a new thing:" << thing->name() << thing->params();

    if (thing->thingClassId() == wallboxThingClassId) {

        QHostAddress address = QHostAddress(thing->paramValue(wallboxThingIpAddressParamTypeId).toString());
        KeContact *keba = new KeContact(address, this);
        connect(keba, &KeContact::connectionChanged, this, &IntegrationPluginKeba::onConnectionChanged);
        connect(keba, &KeContact::commandExecuted, this, &IntegrationPluginKeba::onCommandExecuted);
        connect(keba, &KeContact::reportOneReceived, this, &IntegrationPluginKeba::onReportOneReceived);
        connect(keba, &KeContact::reportTwoReceived, this, &IntegrationPluginKeba::onReportTwoReceived);
        connect(keba, &KeContact::reportThreeReceived, this, &IntegrationPluginKeba::onReportThreeReceived);
        connect(keba, &KeContact::broadcastReceived, this, &IntegrationPluginKeba::onBroadcastReceived);
        if (!keba->init()){
            qCWarning(dcKebaKeContact()) << "Cannot bind to port" << 7090;
            keba->deleteLater();
            return info->finish(Thing::ThingErrorHardwareNotAvailable, QT_TR_NOOP("Error opening network port."));
        }

        ThingId id = thing->id();
        m_kebaDevices.insert(id, keba);
        m_asyncSetup.insert(keba, info);
        keba->getReport1();
        connect(info, &ThingSetupInfo::aborted, this, [id, keba, this]{
            m_asyncSetup.remove(keba);
            m_kebaDevices.remove(id);
            keba->deleteLater();
        });
    } else {
        qCWarning(dcKebaKeContact()) << "setupDevice, unhandled device class" << thing->thingClass();
        info->finish(Thing::ThingErrorThingClassNotFound);
    }
}

void IntegrationPluginKeba::postSetupThing(Thing *thing)
{
    qCDebug(dcKebaKeContact()) << "Post setup" << thing->name();
    KeContact *keba = m_kebaDevices.value(thing->id());
    if (!keba) {
        return;
    }
    keba->getReport2();
    keba->getReport3();

    if (!m_pluginTimer) {
        m_pluginTimer = hardwareManager()->pluginTimerManager()->registerTimer(60);
        connect(m_pluginTimer, &PluginTimer::timeout, this, &IntegrationPluginKeba::updateData);
    }
}

void IntegrationPluginKeba::thingRemoved(Thing *thing)
{   
    if (thing->thingClassId() == wallboxThingClassId) {
        KeContact *keba = m_kebaDevices.take(thing->id());
        keba->deleteLater();
    }

    if (myThings().empty()) {
        // last device has been removed the plug in timer can be stopped again
        hardwareManager()->pluginTimerManager()->unregisterTimer(m_pluginTimer);
        m_pluginTimer = nullptr;
    }
}

void IntegrationPluginKeba::updateData()
{
    foreach (KeContact *keba, m_kebaDevices) {
        keba->getReport2();
        keba->getReport3();
    }

    foreach (Thing *thing, myThings().filterByThingClassId(wallboxThingClassId)) {
        if (m_chargingSessionStartTime.contains(thing->id())) {
            QDateTime startTime = m_chargingSessionStartTime.value(thing->id());

            QTimeZone tz = QTimeZone(QTimeZone::systemTimeZoneId());
            QDateTime currentTime = QDateTime::currentDateTime().toTimeZone(tz);

            int minutes = (currentTime.toMSecsSinceEpoch() - startTime.toMSecsSinceEpoch())/60000;
            thing->setStateValue(wallboxSessionTimeStateTypeId, minutes);
        } else {
            thing->setStateValue(wallboxSessionTimeStateTypeId, 0);
        }
    }
}

void IntegrationPluginKeba::setDeviceState(Thing *thing, KeContact::State state)
{
    switch (state) {
    case KeContact::StateStarting:
        thing->setStateValue(wallboxActivityStateTypeId, "Starting");
        break;
    case KeContact::StateNotReady:
        thing->setStateValue(wallboxActivityStateTypeId, "Not ready for charging");
        break;
    case KeContact::StateReady:
        thing->setStateValue(wallboxActivityStateTypeId, "Ready for charging");
        break;
    case KeContact::StateCharging:
        thing->setStateValue(wallboxActivityStateTypeId, "Charging");
        break;
    case KeContact::StateError:
        thing->setStateValue(wallboxActivityStateTypeId, "Error");
        break;
    case KeContact::StateAuthorizationRejected:
        thing->setStateValue(wallboxActivityStateTypeId, "Authorization rejected");
        break;
    }

    if (state == KeContact::StateCharging) {
        //Set charging session
        QTimeZone tz = QTimeZone(QTimeZone::systemTimeZoneId());
        QDateTime startedChargingSession = QDateTime::currentDateTime().toTimeZone(tz);
        m_chargingSessionStartTime.insert(thing->id(), startedChargingSession);
    } else {
        m_chargingSessionStartTime.remove(thing->id());
        thing->setStateValue(wallboxSessionTimeStateTypeId, 0);
    }
}

void IntegrationPluginKeba::setDevicePlugState(Thing *thing, KeContact::PlugState plugState)
{
    switch (plugState) {
    case KeContact::PlugStateUnplugged:
        thing->setStateValue(wallboxPlugStateStateTypeId, "Unplugged");
        break;
    case KeContact::PlugStatePluggedOnChargingStation:
        thing->setStateValue(wallboxPlugStateStateTypeId, "Plugged in charging station");
        break;
    case KeContact::PlugStatePluggedOnChargingStationAndPluggedOnEV:
        thing->setStateValue(wallboxPlugStateStateTypeId, "Plugged in on EV");
        break;
    case KeContact::PlugStatePluggedOnChargingStationAndPlugLocked:
        thing->setStateValue(wallboxPlugStateStateTypeId, "Plugged in and locked");
        break;
    case KeContact::PlugStatePluggedOnChargingStationAndPlugLockedAndPluggedOnEV:
        thing->setStateValue(wallboxPlugStateStateTypeId, "Plugged in on EV and locked");
        break;
    }
}

void IntegrationPluginKeba::onConnectionChanged(bool status)
{
    KeContact *keba = static_cast<KeContact *>(sender());
    Thing *thing = myThings().findById(m_kebaDevices.key(keba));
    if (!thing) {
        qCWarning(dcKebaKeContact()) << "On connection changed: missing device object";
        return;
    }
    thing->setStateValue(wallboxConnectedStateTypeId, status);
    if (!status) {
        //TODO start rediscovery
    }
}

void IntegrationPluginKeba::onCommandExecuted(QUuid requestId, bool success)
{
    updateData();
    if (m_asyncActions.contains(requestId)) {
        KeContact *keba = static_cast<KeContact *>(sender());
        Thing *thing = myThings().findById(m_kebaDevices.key(keba));
        if (!thing) {
            qCWarning(dcKebaKeContact()) << "On command executed: missing device object";
            return;
        }
        ThingActionInfo *info = m_asyncActions.take(requestId);
        if (success) {
            info->finish(Thing::ThingErrorNoError);
        } else {
            info->finish(Thing::ThingErrorHardwareFailure);
        }
    }
}

void IntegrationPluginKeba::onReportOneReceived(const KeContact::ReportOne &reportOne)
{
    Q_UNUSED(reportOne);
    KeContact *keba = static_cast<KeContact *>(sender());
    if (m_asyncSetup.contains(keba)) {
        ThingSetupInfo *info = m_asyncSetup.value(keba);
        info->finish(Thing::ThingErrorNoError);
    } else {
        qCDebug(dcKebaKeContact()) << "Report one received without an associated async setup";
    }
}

void IntegrationPluginKeba::onReportTwoReceived(const KeContact::ReportTwo &reportTwo)
{
    KeContact *keba = static_cast<KeContact *>(sender());
    Thing *thing = myThings().findById(m_kebaDevices.key(keba));
    if (!thing)
        return;

    thing->setStateValue(wallboxPowerStateTypeId, reportTwo.enableUser);
    thing->setStateValue(wallboxMaxChargingCurrentPercentStateTypeId, reportTwo.MaxCurrentPercentage);

    setDeviceState(thing, reportTwo.state);
    setDevicePlugState(thing, reportTwo.plugState);
}

void IntegrationPluginKeba::onReportThreeReceived(const KeContact::ReportThree &reportThree)
{
    KeContact *keba = static_cast<KeContact *>(sender());
    Thing *thing = myThings().findById(m_kebaDevices.key(keba));
    if (!thing)
        return;

    thing->setStateValue(wallboxI1EventTypeId, reportThree.CurrentPhase1);
    thing->setStateValue(wallboxI2EventTypeId, reportThree.CurrentPhase2);
    thing->setStateValue(wallboxI3EventTypeId, reportThree.CurrentPhase3);
    thing->setStateValue(wallboxU1EventTypeId, reportThree.VoltagePhase1);
    thing->setStateValue(wallboxU2EventTypeId, reportThree.VoltagePhase2);
    thing->setStateValue(wallboxU3EventTypeId, reportThree.VoltagePhase3);
    thing->setStateValue(wallboxPStateTypeId,  reportThree.Power);
    thing->setStateValue(wallboxEPStateTypeId, reportThree.EnergySession);
    thing->setStateValue(wallboxTotalEnergyConsumedStateTypeId, reportThree.EnergyTotal);
}

void IntegrationPluginKeba::onBroadcastReceived(KeContact::BroadcastType type, const QVariant &content)
{
    KeContact *keba = static_cast<KeContact *>(sender());
    Thing *thing = myThings().findById(m_kebaDevices.key(keba));
    if (!thing)
        return;

    switch (type) {
    case KeContact::BroadcastTypePlug:
        setDevicePlugState(thing, KeContact::PlugState(content.toInt()));
        break;
    case KeContact::BroadcastTypeInput:
        break;
    case KeContact::BroadcastTypeEPres:
        thing->setStateValue(wallboxEPStateTypeId, content.toInt());
        break;
    case KeContact::BroadcastTypeState:
        setDeviceState(thing, KeContact::State(content.toInt()));
        break;
    case KeContact::BroadcastTypeMaxCurr:
        thing->setStateValue(wallboxMaxChargingCurrentStateTypeId, content.toInt());
        break;
    case KeContact::BroadcastTypeEnableSys:
        break;
    }
}

void IntegrationPluginKeba::executeAction(ThingActionInfo *info)
{
    Thing *thing = info->thing();
    Action action = info->action();

    if (thing->thingClassId() == wallboxThingClassId) {
        KeContact *keba = m_kebaDevices.value(thing->id());
        if (!keba) {
            qCWarning(dcKebaKeContact()) << "Device not properly initialized, Keba object missing";
            return info->finish(Thing::ThingErrorHardwareNotAvailable);
        }

        if(action.actionTypeId() == wallboxMaxChargingCurrentActionTypeId){
            int milliAmpere = action.param(wallboxMaxChargingCurrentActionMaxChargingCurrentParamTypeId).value().toInt();
            QUuid requestId = keba->setMaxAmpere(milliAmpere);
            m_asyncActions.insert(requestId, info);
            connect(info, &ThingActionInfo::aborted, this, [requestId, this]{m_asyncActions.remove(requestId);});

        } else if(action.actionTypeId() == wallboxPowerActionTypeId){
            QUuid requestId = keba->enableOutput(action.param(wallboxPowerActionTypeId).value().toBool());
            m_asyncActions.insert(requestId, info);
            connect(info, &ThingActionInfo::aborted, this, [requestId, this]{m_asyncActions.remove(requestId);});

        } else if(action.actionTypeId() == wallboxDisplayActionTypeId){
            QUuid requestId = keba->displayMessage(action.param(wallboxDisplayActionMessageParamTypeId).value().toByteArray());
            m_asyncActions.insert(requestId, info);
            connect(info, &ThingActionInfo::aborted, this, [requestId, this]{m_asyncActions.remove(requestId);});

        } else {
            qCWarning(dcKebaKeContact()) << "Unhandled ActionTypeId:" << action.actionTypeId();
            info->finish(Thing::ThingErrorActionTypeNotFound);
        }
    } else {
        qCWarning(dcKebaKeContact()) << "Execute action, unhandled device class" << thing->thingClass();
        info->finish(Thing::ThingErrorThingClassNotFound);
    }
}
