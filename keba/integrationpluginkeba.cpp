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
        qCDebug(dcKeba()) << "Discovering Keba Wallbox...";
        NetworkDeviceDiscoveryReply *discoveryReply = hardwareManager()->networkDeviceDiscovery()->discover();
        connect(discoveryReply, &NetworkDeviceDiscoveryReply::finished, this, [=](){
            ThingDescriptors descriptors;
            qCDebug(dcKeba()) << "Discovery finished. Found" << discoveryReply->networkDeviceInfos().count() << "devices";
            foreach (const NetworkDeviceInfo &networkDeviceInfo, discoveryReply->networkDeviceInfos()) {
                if (!networkDeviceInfo.macAddressManufacturer().contains("keba", Qt::CaseSensitivity::CaseInsensitive))
                    continue;

                qCDebug(dcKeba()) << "     - Keba Wallbox" << networkDeviceInfo;
                QString title = "Keba Wallbox ";
                if (networkDeviceInfo.hostName().isEmpty()) {
                    title += "(" + networkDeviceInfo.address().toString() + ")";
                } else {
                    title += networkDeviceInfo.hostName() + " (" + networkDeviceInfo.address().toString() + ")";
                }

                QString description;
                if (networkDeviceInfo.macAddressManufacturer().isEmpty()) {
                    description = networkDeviceInfo.macAddress();
                } else {
                    description = networkDeviceInfo.macAddress() + " (" + networkDeviceInfo.macAddressManufacturer() + ")";
                }

                ThingDescriptor descriptor(wallboxThingClassId, title, description);

                // Check if we already have set up this device
                Things existingThings = myThings().filterByParam(wallboxThingMacAddressParamTypeId, networkDeviceInfo.macAddress());
                if (existingThings.count() == 1) {
                    qCDebug(dcKeba()) << "This wallbox already exists in the system!" << networkDeviceInfo;
                    descriptor.setThingId(existingThings.first()->id());
                }

                ParamList params;
                params << Param(wallboxThingMacAddressParamTypeId, networkDeviceInfo.macAddress());
                params << Param(wallboxThingIpAddressParamTypeId, networkDeviceInfo.address().toString());
                descriptor.setParams(params);
                info->addThingDescriptor(descriptor);
            }

            info->finish(Thing::ThingErrorNoError);
        });
    } else {
        qCWarning(dcKeba()) << "Could not discover things because of unhandled thing class id" << info->thingClassId().toString();
        info->finish(Thing::ThingErrorThingClassNotFound);
    }
}

void IntegrationPluginKeba::setupThing(ThingSetupInfo *info)
{
    Thing *thing = info->thing();
    if (thing->thingClassId() == wallboxThingClassId) {

        // Handle reconfigure
        if (myThings().contains(thing)) {
            KeContact *keba = m_kebaDevices.take(thing->id());
            if (keba) {
                qCDebug(dcKeba()) << "Reconfigure" << thing->name() << thing->params();
                delete keba;
                // Now continue with the normal setup
            }
        }

        qCDebug(dcKeba()) << "Setting up" << thing->name() << thing->params();

        if (!m_kebaDataLayer){
            qCDebug(dcKeba()) << "Creating new Keba data layer...";
            m_kebaDataLayer= new KeContactDataLayer(this);
            if (!m_kebaDataLayer->init()) {
                m_kebaDataLayer->deleteLater();
                m_kebaDataLayer = nullptr;
                connect(info, &ThingSetupInfo::aborted, m_kebaDataLayer, &KeContactDataLayer::deleteLater); // Clean up if the setup fails
                return info->finish(Thing::ThingErrorHardwareNotAvailable, QT_TR_NOOP("Error opening network port."));
            }
        }

        QHostAddress address = QHostAddress(thing->paramValue(wallboxThingIpAddressParamTypeId).toString());

        // Check if we have a keba with this ip, if reconfigure the object would already been removed from the hash
        foreach (KeContact *kebaConnect, m_kebaDevices.values()) {
            if (kebaConnect->address() == address) {
                qCWarning(dcKeba()) << "Failed to set up keba for host address" << address.toString() << "because there has already been configured a keba for this ip.";
                return info->finish(Thing::ThingErrorThingInUse, QT_TR_NOOP("Already configured for this IP address."));
            }
        }

        KeContact *keba = new KeContact(address, m_kebaDataLayer, this);
        connect(keba, &KeContact::reachableChanged, this, &IntegrationPluginKeba::onConnectionChanged);
        connect(keba, &KeContact::commandExecuted, this, &IntegrationPluginKeba::onCommandExecuted);
        connect(keba, &KeContact::reportTwoReceived, this, &IntegrationPluginKeba::onReportTwoReceived);
        connect(keba, &KeContact::reportThreeReceived, this, &IntegrationPluginKeba::onReportThreeReceived);
        connect(keba, &KeContact::report1XXReceived, this, &IntegrationPluginKeba::onReport1XXReceived);
        connect(keba, &KeContact::broadcastReceived, this, &IntegrationPluginKeba::onBroadcastReceived);

        connect(keba, &KeContact::reportOneReceived, info, [info, this, keba] (const KeContact::ReportOne &report) {
            Thing *thing = info->thing();

            qCDebug(dcKeba()) << "Report one received for" << thing->name();
            qCDebug(dcKeba()) << "     - Firmware" << report.firmware;
            qCDebug(dcKeba()) << "     - Serial" << report.serialNumber;
            qCDebug(dcKeba()) << "     - Product" << report.product;
            qCDebug(dcKeba()) << "     - Uptime" << report.seconds/60 << "[min]";
            qCDebug(dcKeba()) << "     - Com Module" << report.comModule;

            thing->setStateValue(wallboxConnectedStateTypeId, true);
            thing->setStateValue(wallboxFirmwareStateTypeId, report.firmware);
            thing->setStateValue(wallboxSerialnumberStateTypeId, report.serialNumber);
            thing->setStateValue(wallboxModelStateTypeId, report.product);
            thing->setStateValue(wallboxUptimeStateTypeId, report.seconds/60);

            m_kebaDevices.insert(thing->id(), keba);
            info->finish(Thing::ThingErrorNoError);
        });

        keba->getReport1();

        connect(info, &ThingSetupInfo::aborted, keba, &KeContact::deleteLater); // Clean up if the setup fails
        connect(keba, &KeContact::destroyed, this, [thing, this]{
            m_kebaDevices.remove(thing->id());
        });

    } else {
        qCWarning(dcKeba()) << "Could not setup thing: unhandled device class" << thing->thingClass();
        info->finish(Thing::ThingErrorThingClassNotFound);
    }
}

void IntegrationPluginKeba::postSetupThing(Thing *thing)
{
    qCDebug(dcKeba()) << "Post setup" << thing->name();
    if (thing->thingClassId() != wallboxThingClassId) {
        qCWarning(dcKeba()) << "Thing class id not supported" << thing->thingClassId();
        return;
    }

    KeContact *keba = m_kebaDevices.value(thing->id());
    if (!keba) {
        qCWarning(dcKeba()) << "No Keba connection found for this thing";
        return;
    } else {
        keba->getReport2();
        keba->getReport3();
    }

    // Try to find the mac address in case the user added the ip manually
    if (thing->paramValue(wallboxThingMacAddressParamTypeId).toString().isEmpty() || thing->paramValue(wallboxThingMacAddressParamTypeId).toString() == "00:00:00:00:00:00") {
        searchNetworkDevices();
    }

    if (!m_updateTimer) {
        m_updateTimer = hardwareManager()->pluginTimerManager()->registerTimer(60);
        connect(m_updateTimer, &PluginTimer::timeout, this, [this]() {
            foreach (Thing *thing, myThings().filterByThingClassId(wallboxThingClassId)) {
                KeContact *keba = m_kebaDevices.value(thing->id());
                if (!keba) {
                    qCWarning(dcKeba()) << "No Keba connection found for" << thing->name();
                    return;
                }
                keba->getReport2();
                keba->getReport3();
                if (thing->stateValue(wallboxActivityStateTypeId).toString() == "Charging") {
                    keba->getReport1XX(100);
                }
            }
        });

        m_updateTimer->start();
    }

    if (!m_reconnectTimer) {
        m_reconnectTimer = hardwareManager()->pluginTimerManager()->registerTimer(60*5);
        connect(m_reconnectTimer, &PluginTimer::timeout, this, [this] {
            // Only search for new network devices if there is one keba which is not connected
            foreach (Thing *thing, myThings().filterByThingClassId(wallboxThingClassId)) {
                KeContact *keba = m_kebaDevices.value(thing->id());
                if (!keba) {
                    qCWarning(dcKeba()) << "No Keba connection found for" << thing->name();
                    continue;
                }

                if (!keba->reachable()) {
                    searchNetworkDevices();
                    return;
                }
            }
        });

        m_reconnectTimer->start();
    }
}

void IntegrationPluginKeba::thingRemoved(Thing *thing)
{
    qCDebug(dcKeba()) << "Deleting" << thing->name();
    if (thing->thingClassId() == wallboxThingClassId) {
        KeContact *keba = m_kebaDevices.take(thing->id());
        keba->deleteLater();
    }

    if (myThings().empty()) {
        qCDebug(dcKeba()) << "Closing UDP Ports";
        m_kebaDataLayer->deleteLater();
        m_kebaDataLayer= nullptr;

        qCDebug(dcKeba()) << "Stopping plugin timers ...";
        if (m_reconnectTimer) {
            hardwareManager()->pluginTimerManager()->unregisterTimer(m_reconnectTimer);
            m_reconnectTimer = nullptr;
        }

        if (m_updateTimer) {
            hardwareManager()->pluginTimerManager()->unregisterTimer(m_updateTimer);
            m_updateTimer = nullptr;
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

    thing->setStateValue(wallboxChargingStateTypeId, state == KeContact::StateCharging);
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

    if (plugState >= 5) {
        thing->setStateValue(wallboxPluggedInStateTypeId, true);
    } else {
        thing->setStateValue(wallboxPluggedInStateTypeId, false);
    }
}

void IntegrationPluginKeba::searchNetworkDevices()
{
    qCDebug(dcKeba()) << "Start searching for things...";
    NetworkDeviceDiscoveryReply *discoveryReply = hardwareManager()->networkDeviceDiscovery()->discover();
    connect(discoveryReply, &NetworkDeviceDiscoveryReply::finished, this, [=](){
        ThingDescriptors descriptors;
        qCDebug(dcKeba()) << "Discovery finished. Found" << discoveryReply->networkDeviceInfos().count() << "devices";
        foreach (const NetworkDeviceInfo &networkDeviceInfo, discoveryReply->networkDeviceInfos()) {
            if (!networkDeviceInfo.hostName().contains("keba", Qt::CaseSensitivity::CaseInsensitive))
                continue;

            foreach (Thing *existingThing, myThings().filterByThingClassId(wallboxThingClassId)) {
                if (existingThing->paramValue(wallboxThingMacAddressParamTypeId).toString().isEmpty()) {
                    //This device got probably manually setup, to enable auto rediscovery the MAC address needs to setup
                    if (existingThing->paramValue(wallboxThingIpAddressParamTypeId).toString() == networkDeviceInfo.address().toString()) {
                        qCDebug(dcKeba()) << "Keba Wallbox MAC Address has been discovered" << existingThing->name() << networkDeviceInfo.macAddress();
                        existingThing->setParamValue(wallboxThingMacAddressParamTypeId, networkDeviceInfo.macAddress());
                    }
                } else if (existingThing->paramValue(wallboxThingMacAddressParamTypeId).toString() == networkDeviceInfo.macAddress())  {
                    // We found the existing keba thing, lets check if the ip has changed
                    if (existingThing->paramValue(wallboxThingIpAddressParamTypeId).toString() != networkDeviceInfo.address().toString()) {
                        qCDebug(dcKeba()) << "Keba Wallbox IP Address has changed, from"  << existingThing->paramValue(wallboxThingIpAddressParamTypeId).toString() << "to" << networkDeviceInfo.address().toString();
                        existingThing->setParamValue(wallboxThingIpAddressParamTypeId, networkDeviceInfo.address().toString());
                        KeContact *keba = m_kebaDevices.value(existingThing->id());
                        if (keba) {
                            keba->setAddress(QHostAddress(networkDeviceInfo.address()));
                        } else {
                            qCWarning(dcKeba()) << "Could not update IP address, for" << existingThing;
                        }
                    } else {
                        qCDebug(dcKeba()) << "Keba Wallbox" << existingThing->name() << "IP address has not changed" << networkDeviceInfo.address().toString();
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
        qCDebug(dcKeba()) << "Received connected changed but the thing seems not to be setup yet.";
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
            qCWarning(dcKeba()) << "On command executed: missing device object";
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

    qCDebug(dcKeba()) << "Report 2 received for" << thing->name() << "Serial number:" << thing->stateValue(wallboxSerialnumberStateTypeId).toString();
    qCDebug(dcKeba()) << "     - State:" << reportTwo.state;
    qCDebug(dcKeba()) << "     - Error 1:" << reportTwo.error1;
    qCDebug(dcKeba()) << "     - Error 2:" << reportTwo.error2;
    qCDebug(dcKeba()) << "     - Plug:" << reportTwo.plugState;
    qCDebug(dcKeba()) << "     - Enable sys:" << reportTwo.enableSys;
    qCDebug(dcKeba()) << "     - Enable user:" << reportTwo.enableUser;
    qCDebug(dcKeba()) << "     - Max curr:" << reportTwo.maxCurrent;
    qCDebug(dcKeba()) << "     - Max curr %:" << reportTwo.maxCurrentPercentage;
    qCDebug(dcKeba()) << "     - Curr HW:" << reportTwo.currentHardwareLimitation;
    qCDebug(dcKeba()) << "     - Curr User:" << reportTwo.currentUser;
    qCDebug(dcKeba()) << "     - Curr FS:" << reportTwo.currentFailsafe;
    qCDebug(dcKeba()) << "     - Tmo FS:" << reportTwo.timeoutFailsafe;
    qCDebug(dcKeba()) << "     - Curr timer:" << reportTwo.currTimer;
    qCDebug(dcKeba()) << "     - Timeout CT:" << reportTwo.timeoutCt;
    qCDebug(dcKeba()) << "     - Output:" << reportTwo.output;
    qCDebug(dcKeba()) << "     - Input:" << reportTwo.input;
    qCDebug(dcKeba()) << "     - Serial number:" << reportTwo.serialNumber;
    qCDebug(dcKeba()) << "     - Uptime:" << reportTwo.seconds/60 << "[min]";

    if (reportTwo.serialNumber == thing->stateValue(wallboxSerialnumberStateTypeId).toString()) {
        setDeviceState(thing, reportTwo.state);
        setDevicePlugState(thing, reportTwo.plugState);

        thing->setStateValue(wallboxPowerStateTypeId, reportTwo.enableUser);
        thing->setStateValue(wallboxError1StateTypeId, reportTwo.error1);
        thing->setStateValue(wallboxError2StateTypeId, reportTwo.error2);
        thing->setStateValue(wallboxSystemEnabledStateTypeId, reportTwo.enableSys);

        thing->setStateValue(wallboxMaxChargingCurrentStateTypeId, qRound(reportTwo.currentUser));
        thing->setStateValue(wallboxMaxChargingCurrentPercentStateTypeId, reportTwo.maxCurrentPercentage);

        // Set the state limits according to the hardware limits
        thing->setStateMaxValue(wallboxMaxChargingCurrentStateTypeId, reportTwo.currentHardwareLimitation);

        thing->setStateValue(wallboxOutputX2StateTypeId, reportTwo.output);
        thing->setStateValue(wallboxInputStateTypeId, reportTwo.input);

        thing->setStateValue(wallboxUptimeStateTypeId, reportTwo.seconds / 60);
    } else {
        qCWarning(dcKeba()) << "Received report but the serial number didn't match";
    }
}

void IntegrationPluginKeba::onReportThreeReceived(const KeContact::ReportThree &reportThree)
{
    KeContact *keba = static_cast<KeContact *>(sender());
    Thing *thing = myThings().findById(m_kebaDevices.key(keba));
    if (!thing)
        return;

    qCDebug(dcKeba()) << "Report 3 received for" << thing->name() << "Serial number:" << thing->stateValue(wallboxSerialnumberStateTypeId).toString();
    qCDebug(dcKeba()) << "     - Current phase 1:" << reportThree.currentPhase1 << "[A]";
    qCDebug(dcKeba()) << "     - Current phase 2:" << reportThree.currentPhase2 << "[A]";
    qCDebug(dcKeba()) << "     - Current phase 3:" << reportThree.currentPhase3 << "[A]";
    qCDebug(dcKeba()) << "     - Voltage phase 1:" << reportThree.voltagePhase1 << "[V]";
    qCDebug(dcKeba()) << "     - Voltage phase 2:" << reportThree.voltagePhase2 << "[V]";
    qCDebug(dcKeba()) << "     - Voltage phase 3:" << reportThree.voltagePhase3 << "[V]";
    qCDebug(dcKeba()) << "     - Power consumption:" << reportThree.power << "[kW]";
    qCDebug(dcKeba()) << "     - Energy session" << reportThree.energySession << "[kWh]";
    qCDebug(dcKeba()) << "     - Energy total" << reportThree.energyTotal << "[kWh]";
    qCDebug(dcKeba()) << "     - Serial number" << reportThree.serialNumber;
    qCDebug(dcKeba()) << "     - Uptime" << reportThree.seconds / 60 << "[min]";

    if (reportThree.serialNumber == thing->stateValue(wallboxSerialnumberStateTypeId).toString()) {
        thing->setStateValue(wallboxCurrentPhaseAEventTypeId, reportThree.currentPhase1);
        thing->setStateValue(wallboxCurrentPhaseBEventTypeId, reportThree.currentPhase2);
        thing->setStateValue(wallboxCurrentPhaseCEventTypeId, reportThree.currentPhase3);
        thing->setStateValue(wallboxVoltagePhaseAEventTypeId, reportThree.voltagePhase1);
        thing->setStateValue(wallboxVoltagePhaseBEventTypeId, reportThree.voltagePhase2);
        thing->setStateValue(wallboxVoltagePhaseCEventTypeId, reportThree.voltagePhase3);
        thing->setStateValue(wallboxCurrentPowerStateTypeId, reportThree.power);
        thing->setStateValue(wallboxSessionEnergyStateTypeId, reportThree.energySession);
        thing->setStateValue(wallboxPowerFactorStateTypeId, reportThree.powerFactor);
        thing->setStateValue(wallboxTotalEnergyConsumedStateTypeId, reportThree.energyTotal);

        // Check how many phases are actually charging, and update the phase count only if something happens on the phases (current or power)
        if (!(reportThree.currentPhase1 == 0 && reportThree.currentPhase2 == 0 && reportThree.currentPhase3 == 0)) {
            uint phaseCount = 0;
            if (reportThree.currentPhase1 != 0)
                phaseCount += 1;

            if (reportThree.currentPhase2 != 0)
                phaseCount += 1;

            if (reportThree.currentPhase3 != 0)
                phaseCount += 1;

            thing->setStateValue(wallboxPhaseCountStateTypeId, phaseCount);
        }
    } else {
        qCWarning(dcKeba()) << "Received report but the serial number didn't match";
    }
}

void IntegrationPluginKeba::onReport1XXReceived(int reportNumber, const KeContact::Report1XX &report)
{
    KeContact *keba = static_cast<KeContact *>(sender());
    Thing *thing = myThings().findById(m_kebaDevices.key(keba));
    if (!thing)
        return;

    qCDebug(dcKeba()) << "Report" << reportNumber << "received for" << thing->name() << "Serial number:" << thing->stateValue(wallboxSerialnumberStateTypeId).toString();
    qCDebug(dcKeba()) << "     - Session Id" << report.sessionId;
    qCDebug(dcKeba()) << "     - Curr HW" << report.currHW;
    qCDebug(dcKeba()) << "     - Energy start" << report.startEnergy;
    qCDebug(dcKeba()) << "     - Energy present" << report.presentEnergy;
    qCDebug(dcKeba()) << "     - Start time" << report.startTime << QDateTime::fromMSecsSinceEpoch(report.startTime * 1000).toString();
    qCDebug(dcKeba()) << "     - End time" << report.endTime;
    qCDebug(dcKeba()) << "     - Stop reason" << report.stopReason;
    qCDebug(dcKeba()) << "     - RFID Tag" << report.rfidTag;
    qCDebug(dcKeba()) << "     - RFID Class" << report.rfidClass;
    qCDebug(dcKeba()) << "     - Serial number" << report.serialNumber;
    qCDebug(dcKeba()) << "     - Uptime" << report.seconds;

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
                    qCDebug(dcKeba()) << "New session id receivd";
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
            qCWarning(dcKeba()) << "Received report but the serial number didn't match";
        }
    } else {
        qCWarning(dcKeba()) << "Received unhandled report" << reportNumber;
    }
}

void IntegrationPluginKeba::onBroadcastReceived(KeContact::BroadcastType type, const QVariant &content)
{
    KeContact *keba = static_cast<KeContact *>(sender());
    Thing *thing = myThings().findById(m_kebaDevices.key(keba));
    if (!thing)
        return;

    qCDebug(dcKeba()) << "Broadcast received" << type << "value" << content;

    switch (type) {
    case KeContact::BroadcastTypePlug:
        setDevicePlugState(thing, KeContact::PlugState(content.toInt()));
        break;
    case KeContact::BroadcastTypeInput:
        thing->setStateValue(wallboxInputStateTypeId, (content.toInt() == 1));
        break;
    case KeContact::BroadcastTypeEPres:
        thing->setStateValue(wallboxSessionEnergyStateTypeId, content.toInt() / 10000.00);
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
            qCWarning(dcKeba()) << "Device not properly initialized, Keba object missing";
            return info->finish(Thing::ThingErrorHardwareNotAvailable);
        }

        QUuid requestId;
        if (action.actionTypeId() == wallboxMaxChargingCurrentActionTypeId) {
            int milliAmpere = action.param(wallboxMaxChargingCurrentActionMaxChargingCurrentParamTypeId).value().toUInt() * 1000;
            requestId = keba->setMaxAmpereGeneral(milliAmpere);
        } else if(action.actionTypeId() == wallboxPowerActionTypeId) {
            requestId = keba->enableOutput(action.param(wallboxPowerActionTypeId).value().toBool());
        } else if(action.actionTypeId() == wallboxDisplayActionTypeId) {
            requestId = keba->displayMessage(action.param(wallboxDisplayActionMessageParamTypeId).value().toByteArray());
        } else if(action.actionTypeId() == wallboxOutputX2ActionTypeId) {
            requestId = keba->setOutputX2(action.param(wallboxOutputX2ActionOutputX2ParamTypeId).value().toBool());
        } else if(action.actionTypeId() == wallboxFailsafeModeActionTypeId) {
            int timeout = 0;
            if (action.param(wallboxFailsafeModeActionFailsafeModeParamTypeId).value().toBool()) {
                timeout = 60;
            }
            requestId = keba->setFailsafe(timeout, 0, false);
        } else {
            qCWarning(dcKeba()) << "Unhandled ActionTypeId:" << action.actionTypeId();
            return info->finish(Thing::ThingErrorActionTypeNotFound);
        }

        // If the keba returns an invalid uuid, something went wrong
        if (requestId.isNull()) {
            info->finish(Thing::ThingErrorHardwareFailure);
            return;
        }

        m_asyncActions.insert(requestId, info);
        connect(info, &ThingActionInfo::aborted, this, [requestId, this]{ m_asyncActions.remove(requestId); });
    } else {
        qCWarning(dcKeba()) << "Execute action, unhandled device class" << thing->thingClass();
        info->finish(Thing::ThingErrorThingClassNotFound);
    }
}
