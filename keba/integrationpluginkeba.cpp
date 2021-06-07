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

#include "plugininfo.h"
#include "integrationpluginkeba.h"
#include "network/networkdevicediscovery.h"

#include <QJsonDocument>
#include <QUdpSocket>
#include <QTimeZone>

IntegrationPluginKeba::IntegrationPluginKeba()
{

}

void IntegrationPluginKeba::init()
{

}

void IntegrationPluginKeba::discoverThings(ThingDiscoveryInfo *info)
{
    if (info->thingClassId() == wallboxThingClassId) {
        qCDebug(dcKebaKeContact()) << "Discovering Keba Wallbox";
        NetworkDeviceDiscoveryReply *discoveryReply = hardwareManager()->networkDeviceDiscovery()->discover();
        connect(discoveryReply, &NetworkDeviceDiscoveryReply::finished, this, [=](){
            ThingDescriptors descriptors;
            qCDebug(dcKebaKeContact()) << "Discovery finished. Found" << discoveryReply->networkDevices().count() << "devices";
            foreach (const NetworkDevice &networkDevice, discoveryReply->networkDevices()) {
                if (!networkDevice.hostName().contains("keba", Qt::CaseSensitivity::CaseInsensitive))
                    continue;

                qCDebug(dcKebaKeContact()) << "     - Keba Wallbox" << networkDevice;
                QString title = "Wallbox ";
                if (networkDevice.hostName().isEmpty()) {
                    title += networkDevice.address().toString();
                } else {
                    title += networkDevice.hostName() + " (" + networkDevice.address().toString() + ")";
                }

                QString description;
                if (networkDevice.macAddressManufacturer().isEmpty()) {
                    description = networkDevice.macAddress();
                } else {
                    description = networkDevice.macAddress() + " (" + networkDevice.macAddressManufacturer() + ")";
                }

                ThingDescriptor descriptor(wallboxThingClassId, title, description);

                // Check if we already have set up this device
                Things existingThings = myThings().filterByParam(wallboxThingMacAddressParamTypeId, networkDevice.macAddress());
                if (existingThings.count() == 1) {
                    qCDebug(dcKebaKeContact()) << "This wallbox already exists in the system!" << networkDevice;
                    descriptor.setThingId(existingThings.first()->id());
                }

                ParamList params;
                params << Param(wallboxThingMacAddressParamTypeId, networkDevice.macAddress());
                params << Param(wallboxThingIpAddressParamTypeId, networkDevice.address().toString());
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

        if(!m_kebaData){
            qCDebug(dcKebaKeContact()) << "Creating new Keba data layer";
            m_kebaData = new KeContactDataLayer(this);
            if (!m_kebaData->init()) {
                m_kebaData->deleteLater();
                m_kebaData = nullptr;
                connect(info, &ThingSetupInfo::aborted, m_kebaData, &KeContactDataLayer::deleteLater); // Clean up if the setup fails
                return info->finish(Thing::ThingErrorHardwareNotAvailable, QT_TR_NOOP("Error opening network port."));
            }
        }

        QHostAddress address = QHostAddress(thing->paramValue(wallboxThingIpAddressParamTypeId).toString());
        KeContact *keba = new KeContact(address, m_kebaData, m_kebaData);
        connect(keba, &KeContact::reachableChanged, this, &IntegrationPluginKeba::onConnectionChanged);
        connect(keba, &KeContact::commandExecuted, this, &IntegrationPluginKeba::onCommandExecuted);
        connect(keba, &KeContact::reportTwoReceived, this, &IntegrationPluginKeba::onReportTwoReceived);
        connect(keba, &KeContact::reportThreeReceived, this, &IntegrationPluginKeba::onReportThreeReceived);
        connect(keba, &KeContact::report1XXReceived, this, &IntegrationPluginKeba::onReport1XXReceived);
        connect(keba, &KeContact::broadcastReceived, this, &IntegrationPluginKeba::onBroadcastReceived);

        keba->getReport1();
        connect(keba, &KeContact::reportOneReceived, info, [info, this, keba] (const KeContact::ReportOne &report) {
            Thing *thing = info->thing();

            qCDebug(dcKebaKeContact()) << "Report one received for" << thing->name();
            qCDebug(dcKebaKeContact()) << "     - Firmware" << report.firmware;
            qCDebug(dcKebaKeContact()) << "     - Serial" << report.serialNumber;
            qCDebug(dcKebaKeContact()) << "     - Product" << report.product;
            qCDebug(dcKebaKeContact()) << "     - Uptime" << report.seconds/60 << "[min]";
            qCDebug(dcKebaKeContact()) << "     - Com Module" << report.comModule;

            thing->setStateValue(wallboxConnectedStateTypeId, true);
            thing->setStateValue(wallboxFirmwareStateTypeId, report.firmware);
            thing->setStateValue(wallboxSerialnumberStateTypeId, report.serialNumber);
            thing->setStateValue(wallboxModelStateTypeId, report.product);
            thing->setStateValue(wallboxUptimeStateTypeId, report.seconds/60);

            m_kebaDevices.insert(thing->id(), keba);
            info->finish(Thing::ThingErrorNoError);
        });
        connect(info, &ThingSetupInfo::aborted, keba, &KeContact::deleteLater); // Clean up if the setup fails
        connect(keba, &KeContact::destroyed, this, [thing, this]{
            m_kebaDevices.remove(thing->id());
        });
    } else {
        qCWarning(dcKebaKeContact()) << "setupDevice, unhandled device class" << thing->thingClass();
        info->finish(Thing::ThingErrorThingClassNotFound);
    }
}

void IntegrationPluginKeba::postSetupThing(Thing *thing)
{
    qCDebug(dcKebaKeContact()) << "Post setup" << thing->name();
    if (thing->thingClassId() != wallboxThingClassId) {
        qCWarning(dcKebaKeContact()) << "Thing class id not supported" << thing->thingClassId();
        return;
    }
    thing->setStateValue(wallboxConnectedStateTypeId, true);
    KeContact *keba = m_kebaDevices.value(thing->id());
    if (!keba) {
        qCWarning(dcKebaKeContact()) << "No Keba connection found for this thing";
        return;
    } else {
        keba->getReport2();
        keba->getReport3();
    }

    if (thing->paramValue(wallboxThingMacAddressParamTypeId).toString().isEmpty()) {
        searchNetworkDevices();
    }

    if (!m_updateTimer) {
        m_updateTimer = hardwareManager()->pluginTimerManager()->registerTimer(60);
        connect(m_updateTimer, &PluginTimer::timeout, this, [this] {

            foreach (Thing *thing, myThings().filterByThingClassId(wallboxThingClassId)) {
                KeContact *keba = m_kebaDevices.value(thing->id());
                if (!keba) {
                    qCWarning(dcKebaKeContact()) << "No Keba connection found for" << thing->name();
                    return;
                }
                keba->getReport2();
                keba->getReport3();
                if (thing->stateValue(wallboxActivityStateTypeId).toString() == "Charging") {
                    keba->getReport1XX(100);
                }
            }
        });
    }

    if (!m_reconnectTimer) {
        m_reconnectTimer = hardwareManager()->pluginTimerManager()->registerTimer(60*5);
        connect(m_reconnectTimer, &PluginTimer::timeout, this, [this] {
            searchNetworkDevices();
        });
    }
}

void IntegrationPluginKeba::thingRemoved(Thing *thing)
{
    qCDebug(dcKebaKeContact()) << "Deleting" << thing->name();
    if (thing->thingClassId() == wallboxThingClassId) {
        KeContact *keba = m_kebaDevices.take(thing->id());
        keba->deleteLater();
    }

    if (myThings().empty()) {
        qCDebug(dcKebaKeContact()) << "Closing UDP Ports";
        m_kebaData->deleteLater();
        m_kebaData = nullptr;

        qCDebug(dcKebaKeContact()) << "Stopping plugin timers";
        hardwareManager()->pluginTimerManager()->unregisterTimer(m_reconnectTimer);
        m_reconnectTimer = nullptr;
        hardwareManager()->pluginTimerManager()->unregisterTimer(m_updateTimer);
        m_updateTimer = nullptr;
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

void IntegrationPluginKeba::searchNetworkDevices()
{
    qCDebug(dcKebaKeContact()) << "Start searching for things...";
    NetworkDeviceDiscoveryReply *discoveryReply = hardwareManager()->networkDeviceDiscovery()->discover();
    connect(discoveryReply, &NetworkDeviceDiscoveryReply::finished, this, [=](){
        ThingDescriptors descriptors;
        qCDebug(dcKebaKeContact()) << "Discovery finished. Found" << discoveryReply->networkDevices().count() << "devices";
        foreach (const NetworkDevice &networkDevice, discoveryReply->networkDevices()) {
            if (!networkDevice.hostName().contains("keba", Qt::CaseSensitivity::CaseInsensitive))
                continue;

            foreach (Thing *existingThing, myThings().filterByThingClassId(wallboxThingClassId)) {
                if (existingThing->paramValue(wallboxThingMacAddressParamTypeId).toString().isEmpty()) {
                    //This device got probably manually setup, to enable auto rediscovery the MAC address needs to setup
                    if (existingThing->paramValue(wallboxThingIpAddressParamTypeId).toString() == networkDevice.address().toString()) {
                        qCDebug(dcKebaKeContact()) << "Keba Wallbox MAC Address has been discovered" << existingThing->name() << networkDevice.macAddress();
                        existingThing->setParamValue(wallboxThingMacAddressParamTypeId, networkDevice.macAddress());
                    }
                } else if (existingThing->paramValue(wallboxThingMacAddressParamTypeId).toString() == networkDevice.macAddress())  {
                    if (existingThing->paramValue(wallboxThingIpAddressParamTypeId).toString() != networkDevice.address().toString()) {
                        qCDebug(dcKebaKeContact()) << "Keba Wallbox IP Address has changed, from"  << existingThing->paramValue(wallboxThingIpAddressParamTypeId).toString() << "to" << networkDevice.address().toString();
                        existingThing->setParamValue(wallboxThingIpAddressParamTypeId, networkDevice.address().toString());
                    } else {
                        qCDebug(dcKebaKeContact()) << "Keba Wallbox" << existingThing->name() << "IP address has not changed" << networkDevice.address().toString();
                    }
                    break;
                }
            }
        }
    });
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
        searchNetworkDevices();
    }
}

void IntegrationPluginKeba::onCommandExecuted(QUuid requestId, bool success)
{
    if (m_asyncActions.contains(requestId)) {
        KeContact *keba = static_cast<KeContact *>(sender());

        keba->getReport2(); //Check if the state was actually set

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

void IntegrationPluginKeba::onReportTwoReceived(const KeContact::ReportTwo &reportTwo)
{
    KeContact *keba = static_cast<KeContact *>(sender());
    Thing *thing = myThings().findById(m_kebaDevices.key(keba));
    if (!thing)
        return;

    qCDebug(dcKebaKeContact()) << "Report 2 received for" << thing->name() << "Serial number:" << thing->stateValue(wallboxSerialnumberStateTypeId).toString();
    qCDebug(dcKebaKeContact()) << "     - State:" << reportTwo.state;
    qCDebug(dcKebaKeContact()) << "     - Error 1:" << reportTwo.error1;
    qCDebug(dcKebaKeContact()) << "     - Error 2:" << reportTwo.error2;
    qCDebug(dcKebaKeContact()) << "     - Plug:" << reportTwo.plugState;
    qCDebug(dcKebaKeContact()) << "     - Enable sys:" << reportTwo.enableSys;
    qCDebug(dcKebaKeContact()) << "     - Enable user:" << reportTwo.enableUser;
    qCDebug(dcKebaKeContact()) << "     - Max curr:" << reportTwo.maxCurrent;
    qCDebug(dcKebaKeContact()) << "     - Max curr %:" << reportTwo.maxCurrentPercentage;
    qCDebug(dcKebaKeContact()) << "     - Curr HW:" << reportTwo.currentHardwareLimitation;
    qCDebug(dcKebaKeContact()) << "     - Curr User:" << reportTwo.currentUser;
    qCDebug(dcKebaKeContact()) << "     - Curr FS:" << reportTwo.currentFailsafe;
    qCDebug(dcKebaKeContact()) << "     - Tmo FS:" << reportTwo.timeoutFailsafe;
    qCDebug(dcKebaKeContact()) << "     - Curr timer:" << reportTwo.currTimer;
    qCDebug(dcKebaKeContact()) << "     - Timeout CT:" << reportTwo.timeoutCt;
    qCDebug(dcKebaKeContact()) << "     - Output:" << reportTwo.output;
    qCDebug(dcKebaKeContact()) << "     - Input:" << reportTwo.input;
    qCDebug(dcKebaKeContact()) << "     - Serial number:" << reportTwo.serialNumber;
    qCDebug(dcKebaKeContact()) << "     - Uptime:" << reportTwo.seconds/60 << "[min]";

    if (reportTwo.serialNumber == thing->stateValue(wallboxSerialnumberStateTypeId).toString()) {
        setDeviceState(thing, reportTwo.state);
        setDevicePlugState(thing, reportTwo.plugState);

        thing->setStateValue(wallboxPowerStateTypeId, reportTwo.enableUser);
        thing->setStateValue(wallboxError1StateTypeId, reportTwo.error1);
        thing->setStateValue(wallboxError2StateTypeId, reportTwo.error2);
        thing->setStateValue(wallboxSystemEnabledStateTypeId, reportTwo.enableSys);

        thing->setStateValue(wallboxMaxChargingCurrentStateTypeId, reportTwo.currentUser);
        thing->setStateValue(wallboxMaxChargingCurrentPercentStateTypeId, reportTwo.maxCurrentPercentage);
        thing->setStateValue(wallboxMaxPossibleChargingCurrentStateTypeId, reportTwo.currentHardwareLimitation);

        thing->setStateValue(wallboxOutputX2StateTypeId, reportTwo.output);
        thing->setStateValue(wallboxInputStateTypeId, reportTwo.input);

        thing->setStateValue(wallboxUptimeStateTypeId, reportTwo.seconds/60);
    } else {
        qCWarning(dcKebaKeContact()) << "Received report but the serial number didn't match";
    }
}

void IntegrationPluginKeba::onReportThreeReceived(const KeContact::ReportThree &reportThree)
{
    KeContact *keba = static_cast<KeContact *>(sender());
    Thing *thing = myThings().findById(m_kebaDevices.key(keba));
    if (!thing)
        return;

    qCDebug(dcKebaKeContact()) << "Report 3 received for" << thing->name() << "Serial number:" << thing->stateValue(wallboxSerialnumberStateTypeId).toString();
    qCDebug(dcKebaKeContact()) << "     - Current phase 1:" << reportThree.currentPhase1 << "[A]";
    qCDebug(dcKebaKeContact()) << "     - Current phase 2:" << reportThree.currentPhase2 << "[A]";
    qCDebug(dcKebaKeContact()) << "     - Current phase 3:" << reportThree.currentPhase3 << "[A]";
    qCDebug(dcKebaKeContact()) << "     - Voltage phase 1:" << reportThree.voltagePhase1 << "[V]";
    qCDebug(dcKebaKeContact()) << "     - Voltage phase 2:" << reportThree.voltagePhase2 << "[V]";
    qCDebug(dcKebaKeContact()) << "     - Voltage phase 3:" << reportThree.voltagePhase3 << "[V]";
    qCDebug(dcKebaKeContact()) << "     - Power consumption:" << reportThree.power << "[kW]";
    qCDebug(dcKebaKeContact()) << "     - Energy session" << reportThree.energySession << "[kWh]";
    qCDebug(dcKebaKeContact()) << "     - Energy total" << reportThree.energyTotal << "[kWh]";
    qCDebug(dcKebaKeContact()) << "     - Serial number" << reportThree.serialNumber;
    qCDebug(dcKebaKeContact()) << "     - Uptime" << reportThree.seconds/60 << "[min]";

    if (reportThree.serialNumber == thing->stateValue(wallboxSerialnumberStateTypeId).toString()) {
        thing->setStateValue(wallboxCurrentPhase1EventTypeId, reportThree.currentPhase1);
        thing->setStateValue(wallboxCurrentPhase2EventTypeId, reportThree.currentPhase2);
        thing->setStateValue(wallboxCurrentPhase3EventTypeId, reportThree.currentPhase3);
        thing->setStateValue(wallboxVoltagePhase1EventTypeId, reportThree.voltagePhase1);
        thing->setStateValue(wallboxVoltagePhase2EventTypeId, reportThree.voltagePhase2);
        thing->setStateValue(wallboxVoltagePhase3EventTypeId, reportThree.voltagePhase3);
        thing->setStateValue(wallboxPowerConsumptionStateTypeId, reportThree.power);
        thing->setStateValue(wallboxSessionEnergyStateTypeId, reportThree.energySession);
        thing->setStateValue(wallboxPowerFactorStateTypeId, reportThree.powerFactor);
        thing->setStateValue(wallboxTotalEnergyConsumedStateTypeId, reportThree.energyTotal);
    } else {
        qCWarning(dcKebaKeContact()) << "Received report but the serial number didn't match";
    }
}

void IntegrationPluginKeba::onReport1XXReceived(int reportNumber, const KeContact::Report1XX &report)
{
    KeContact *keba = static_cast<KeContact *>(sender());
    Thing *thing = myThings().findById(m_kebaDevices.key(keba));
    if (!thing)
        return;

    qCDebug(dcKebaKeContact()) << "Report" << reportNumber << "received for" << thing->name() << "Serial number:" << thing->stateValue(wallboxSerialnumberStateTypeId).toString();
    qCDebug(dcKebaKeContact()) << "     - Session Id" << report.sessionId;
    qCDebug(dcKebaKeContact()) << "     - Curr HW" << report.currHW;
    qCDebug(dcKebaKeContact()) << "     - Energy start" << report.startEnergy;
    qCDebug(dcKebaKeContact()) << "     - Energy present" << report.presentEnergy;
    qCDebug(dcKebaKeContact()) << "     - Start time" << report.startTime;
    qCDebug(dcKebaKeContact()) << "     - End time" << report.endTime;
    qCDebug(dcKebaKeContact()) << "     - Stop reason" << report.stopReason;
    qCDebug(dcKebaKeContact()) << "     - RFID Tag" << report.rfidTag;
    qCDebug(dcKebaKeContact()) << "     - RFID Class" << report.rfidClass;
    qCDebug(dcKebaKeContact()) << "     - Serial number" << report.serialNumber;
    qCDebug(dcKebaKeContact()) << "     - Uptime" << report.seconds;

    if (reportNumber == 100) {
        // Report 100 is the current charging session
        if (report.endTime == 0) {
            // if the charing session is finished the end time will be set
            double duration = (report.seconds - report.startTime) / 60.00;
            thing->setStateValue(wallboxSessionTimeStateTypeId, duration);
        } else {
            // Charging session is finished and copied to Report 101
        }

    } else if (reportNumber == 101) {
        // Report 101 is the lastest finished session
        if (report.serialNumber == thing->stateValue(wallboxSerialnumberStateTypeId).toString()) {
            if (!m_lastSessionId.contains(thing->id())) {
                // This happens after reboot
                m_lastSessionId.insert(thing->id(), report.sessionId);
            } else {
                if (m_lastSessionId.value(thing->id()) != report.sessionId) {
                    qCDebug(dcKebaKeContact()) << "New session id receivd";
                    Event event;
                    event.setEventTypeId(wallboxChargingSessionFinishedEventTypeId);
                    event.setThingId(thing->id());
                    ParamList params;
                    params << Param(wallboxChargingSessionFinishedEventEnergyParamTypeId, report.presentEnergy);
                    params << Param(wallboxChargingSessionFinishedEventDurationParamTypeId, report.endTime);
                    params << Param(wallboxChargingSessionFinishedEventIdParamTypeId);
                    event.setParams(params);
                    emit emitEvent(event);
                }
            }
        } else {
            qCWarning(dcKebaKeContact()) << "Received report but the serial number didn't match";
        }
    } else {
        qCWarning(dcKebaKeContact()) << "Received unhandled report" << reportNumber;
    }
}

void IntegrationPluginKeba::onBroadcastReceived(KeContact::BroadcastType type, const QVariant &content)
{
    KeContact *keba = static_cast<KeContact *>(sender());
    Thing *thing = myThings().findById(m_kebaDevices.key(keba));
    if (!thing)
        return;

    qCDebug(dcKebaKeContact()) << "Broadcast received" << type << "value" << content;

    switch (type) {
    case KeContact::BroadcastTypePlug:
        setDevicePlugState(thing, KeContact::PlugState(content.toInt()));
        break;
    case KeContact::BroadcastTypeInput:
        thing->setStateValue(wallboxInputStateTypeId, (content.toInt() == 1));
        break;
    case KeContact::BroadcastTypeEPres:
        thing->setStateValue(wallboxSessionEnergyStateTypeId, content.toInt()/10000.00);
        break;
    case KeContact::BroadcastTypeState:
        setDeviceState(thing, KeContact::State(content.toInt()));
        break;
    case KeContact::BroadcastTypeMaxCurr:
        //Current preset value via Control pilot in milliampere
        break;
    case KeContact::BroadcastTypeEnableSys:
        thing->setStateValue(wallboxSystemEnabledStateTypeId, (content.toInt() != 0));
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

        QUuid requestId;
        if(action.actionTypeId() == wallboxMaxChargingCurrentActionTypeId){
            int milliAmpere = action.param(wallboxMaxChargingCurrentActionMaxChargingCurrentParamTypeId).value().toDouble()*1000;
            requestId = keba->setMaxAmpere(milliAmpere);

        } else if(action.actionTypeId() == wallboxPowerActionTypeId){
            requestId = keba->enableOutput(action.param(wallboxPowerActionTypeId).value().toBool());

        } else if(action.actionTypeId() == wallboxDisplayActionTypeId){
            requestId = keba->displayMessage(action.param(wallboxDisplayActionMessageParamTypeId).value().toByteArray());

        } else if(action.actionTypeId() == wallboxOutputX2ActionTypeId) {
            requestId = keba->setOutputX2(action.param(wallboxOutputX2ActionOutputX2ParamTypeId).value().toBool());

        } else if(action.actionTypeId() == wallboxFailsafeModeActionTypeId){
            int timeout = 0;
            if (action.param(wallboxFailsafeModeActionFailsafeModeParamTypeId).value().toBool()) {
                timeout = 60;
            }
            requestId = keba->setFailsafe(timeout, 0, false);
        } else {
            qCWarning(dcKebaKeContact()) << "Unhandled ActionTypeId:" << action.actionTypeId();
            return info->finish(Thing::ThingErrorActionTypeNotFound);
        }
        m_asyncActions.insert(requestId, info);
        connect(info, &ThingActionInfo::aborted, this, [requestId, this]{m_asyncActions.remove(requestId);});
    } else {
        qCWarning(dcKebaKeContact()) << "Execute action, unhandled device class" << thing->thingClass();
        info->finish(Thing::ThingErrorThingClassNotFound);
    }
}
