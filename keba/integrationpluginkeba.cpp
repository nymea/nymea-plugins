/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*
* Copyright 2013 - 2022, nymea GmbH
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
#include "kebaproductinfo.h"
#include "integrationpluginkeba.h"

#include <QJsonDocument>
#include <QUdpSocket>
#include <QTimeZone>

IntegrationPluginKeba::IntegrationPluginKeba()
{

}

void IntegrationPluginKeba::init()
{
    m_macAddressParamTypeIds.insert(kebaThingClassId, kebaThingMacAddressParamTypeId);
    m_macAddressParamTypeIds.insert(kebaSimpleThingClassId, kebaSimpleThingMacAddressParamTypeId);

    m_ipAddressParamTypeIds.insert(kebaThingClassId, kebaThingIpAddressParamTypeId);
    m_ipAddressParamTypeIds.insert(kebaSimpleThingClassId, kebaSimpleThingIpAddressParamTypeId);

    m_modelParamTypeIds.insert(kebaThingClassId, kebaThingModelParamTypeId);
    m_modelParamTypeIds.insert(kebaSimpleThingClassId, kebaSimpleThingModelParamTypeId);

    m_serialNumberParamTypeIds.insert(kebaThingClassId, kebaThingSerialNumberParamTypeId);
    m_serialNumberParamTypeIds.insert(kebaSimpleThingClassId, kebaSimpleThingSerialNumberParamTypeId);
}

void IntegrationPluginKeba::discoverThings(ThingDiscoveryInfo *info)
{
    // Init data layer if not already created
    if (!m_kebaDataLayer){
        qCDebug(dcKeba()) << "Creating new Keba data layer...";
        m_kebaDataLayer= new KeContactDataLayer(this);
        if (!m_kebaDataLayer->init()) {
            m_kebaDataLayer->deleteLater();
            m_kebaDataLayer = nullptr;
            qCWarning(dcKeba()) << "Failed to create Keba data layer...";
            info->finish(Thing::ThingErrorHardwareFailure, QT_TR_NOOP("The communication could not be established."));
            return;
        }
    }

    if (!hardwareManager()->networkDeviceDiscovery()->available()) {
        qCWarning(dcKeba()) << "The network discovery does not seem to be available.";
        info->finish(Thing::ThingErrorHardwareNotAvailable, QT_TR_NOOP("The network discovery is not available. Please enter the IP address manually."));
        return;
    }

    // Create a discovery with the info as parent for auto deleting the object once the discovery info is done
    KebaDiscovery *discovery = new KebaDiscovery(m_kebaDataLayer, hardwareManager()->networkDeviceDiscovery(), info);
    connect(discovery, &KebaDiscovery::discoveryFinished, info, [=](){
        foreach (const KebaDiscovery::KebaDiscoveryResult &result, discovery->discoveryResults()) {

            KebaProductInfo productInformation(result.product);
            if (!productInformation.isValid()) {
                qCWarning(dcKeba()) << "Discovered keba with invalid product information" << result.product;
                continue;
            }

            ThingClassId discoveredThingClassId = kebaThingClassId;
            // Check if this is a keba without meter (aka simple)
            if (productInformation.meter() == KebaProductInfo::NoMeter) {
                discoveredThingClassId = kebaSimpleThingClassId;
            }

            // Make sure we show only the result we searched for to prevent cross adding between normal and simple
            if (discoveredThingClassId != info->thingClassId())
                continue;

            ThingDescriptor descriptor(discoveredThingClassId, "Keba " + result.product, "Serial: " + result.serialNumber + " - " + result.networkDeviceInfo.address().toString());
            qCDebug(dcKeba()) << "Discovered:" << descriptor.title() << descriptor.description();

            // Check if we already have set up this device
            Things existingThings = myThings().filterByParam(m_macAddressParamTypeIds.value(discoveredThingClassId), result.networkDeviceInfo.macAddress());
            if (existingThings.count() == 1) {
                qCDebug(dcKeba()) << "This keba already exists in the system!" << result.networkDeviceInfo;
                descriptor.setThingId(existingThings.first()->id());
            }

            ParamList params;
            params << Param(m_macAddressParamTypeIds.value(discoveredThingClassId), result.networkDeviceInfo.macAddress());
            params << Param(m_ipAddressParamTypeIds.value(discoveredThingClassId), result.networkDeviceInfo.address().toString());
            params << Param(m_modelParamTypeIds.value(discoveredThingClassId), result.product);
            params << Param(m_serialNumberParamTypeIds.value(discoveredThingClassId), result.serialNumber);
            descriptor.setParams(params);
            info->addThingDescriptor(descriptor);
        }

        info->finish(Thing::ThingErrorNoError);
    });

    // Start the discovery process
    discovery->startDiscovery();
}

void IntegrationPluginKeba::setupThing(ThingSetupInfo *info)
{
    Thing *thing = info->thing();

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
            info->finish(Thing::ThingErrorHardwareNotAvailable, QT_TR_NOOP("Error opening network port."));
            return;
        }
    }

    QHostAddress address = QHostAddress(thing->paramValue(m_ipAddressParamTypeIds.value(thing->thingClassId())).toString());
    // Check if we have a keba with this ip, if reconfigure the object would already been removed from the hash
    foreach (KeContact *kebaConnect, m_kebaDevices.values()) {
        if (kebaConnect->address() == address) {
            qCWarning(dcKeba()) << "Failed to set up keba for host address" << address.toString() << "because there has already been configured a keba for this IP.";
            info->finish(Thing::ThingErrorThingInUse, QT_TR_NOOP("Already configured for this IP address."));
            return;
        }
    }

    KeContact *keba = new KeContact(address, m_kebaDataLayer, this);
    connect(keba, &KeContact::reachableChanged, this, &IntegrationPluginKeba::onConnectionChanged);
    connect(keba, &KeContact::commandExecuted, this, &IntegrationPluginKeba::onCommandExecuted);
    connect(keba, &KeContact::reportTwoReceived, this, &IntegrationPluginKeba::onReportTwoReceived);
    connect(keba, &KeContact::reportThreeReceived, this, &IntegrationPluginKeba::onReportThreeReceived);
    connect(keba, &KeContact::report1XXReceived, this, &IntegrationPluginKeba::onReport1XXReceived);
    connect(keba, &KeContact::broadcastReceived, this, &IntegrationPluginKeba::onBroadcastReceived);

    connect(info, &ThingSetupInfo::aborted, keba, &KeContact::deleteLater); // Clean up if the setup fails

    // Make sure we receive data from the keba and the DIP switches are configured correctly
    connect(keba, &KeContact::reportOneReceived, info, [=] (const KeContact::ReportOne &report) {
        Thing *thing = info->thing();
        qCDebug(dcKeba()) << "Report one received for" << thing->name();
        qCDebug(dcKeba()) << "     - Firmware" << report.firmware;
        qCDebug(dcKeba()) << "     - Serial" << report.serialNumber;
        qCDebug(dcKeba()) << "     - Product" << report.product;
        qCDebug(dcKeba()) << "     - Uptime" << report.seconds / 60 << "[min]";
        qCDebug(dcKeba()) << "     - Com Module" << report.comModule;
        qCDebug(dcKeba()) << "     - DIP switch 1" << report.dipSw1;
        qCDebug(dcKeba()) << "     - DIP switch 2" << report.dipSw2;

        KebaProductInfo productInformation(report.product);

        if (thing->paramValue(m_serialNumberParamTypeIds.value(thing->thingClassId())).toString().isEmpty()) {
            qCDebug(dcKeba()) << "Update serial number parameter for" << thing << "to" << report.serialNumber;
            thing->setParamValue(m_serialNumberParamTypeIds.value(thing->thingClassId()), report.serialNumber);
        }

        if (thing->paramValue(m_modelParamTypeIds.value(thing->thingClassId())).toString().isEmpty()) {
            qCDebug(dcKeba()) << "Update model parameter for" << thing << "to" << report.product;
            thing->setParamValue(m_modelParamTypeIds.value(thing->thingClassId()), report.product);
        }

        // Verify the DIP switches and warn the user in case if wrong configuration
        // For having UPD controll on the keba we need DIP Switch 1.3 enabled
        KeContact::DipSwitchOneFlag dipSwOne(report.dipSw1);
        qCDebug(dcKeba()) << dipSwOne;
        if (!dipSwOne.testFlag(KeContact::DipSwitchOneSmartHomeInterface)) {
            qCWarning(dcKeba()) << "Connected successfully to Keba but the DIP Switch for controlling it is not enabled.";
            info->finish(Thing::ThingErrorHardwareFailure, QT_TR_NOOP("The required communication interface is not enabled on this keba. Please make sure the DIP switch 1.3 is switched on and try again."));
            return;
        }

        // Parse the product code and check if the model actually supports the UDP/Modbus communication
        // Supported are:
        // - The A series (german edition), no meter DE440 (green edition)
        // - The B series (german edition), no meter DE440
        // - All C series
        // - All X series

        if (productInformation.isValid()) {

            bool supported = false;

            qCDebug(dcKeba()) << "Product information are valid. Evaluating if model supports UDP/Modbus communication...";

            switch (productInformation.series()) {
            case KebaProductInfo::SeriesA:
                if (productInformation.model() == "P30" && productInformation.germanEdition()) {
                    qCDebug(dcKeba()) << "The P30 A series german edition is supported (DE440 GREEN EDITION)";
                    supported = true;
                }
                break;
            case KebaProductInfo::SeriesB:
                if (productInformation.model() == "P30" && productInformation.germanEdition()) {
                    qCDebug(dcKeba()) << "The P30 B series german edition is supported (DE440)";
                    supported = true;
                }
                break;
            case KebaProductInfo::SeriesC:
            case KebaProductInfo::SeriesXWlan:
            case KebaProductInfo::SeriesXWlan3G:
            case KebaProductInfo::SeriesXWlan4G:
            case KebaProductInfo::SeriesX3G:
            case KebaProductInfo::SeriesX4G:
                qCDebug(dcKeba()) << "The keba" << productInformation.series() << "is capable of communicating using UDP";
                supported = true;
                break;
            default:
                break;
            }

            if (!supported) {
                qCWarning(dcKeba()) << "Connected successfully to Keba but this model" << productInformation.series() << "has no communication module.";
                info->finish(Thing::ThingErrorHardwareFailure, QT_TR_NOOP("This model does not support communication with smart devices."));
                return;
            }
        } else {
            qCWarning(dcKeba()) << "Product information are not valid. Cannot determin if this model supports UDP/Modbus communication, assuming yes so let's try to init...";
        }

        m_kebaDevices.insert(thing->id(), keba);
        info->finish(Thing::ThingErrorNoError);
        qCDebug(dcKeba()) << "Setup finsihed successfully for" << thing << thing->params();

        thing->setStateValue("connected", true);
        thing->setStateValue("firmware", report.firmware);
        thing->setStateValue("uptime", report.seconds / 60);
    });

    keba->getReport1();

    connect(keba, &KeContact::destroyed, this, [this, thing]{
        m_kebaDevices.remove(thing->id());
        // Setup failed, lets search the network, maybe the IP has changed...
        searchNetworkDevices();
    });
}

void IntegrationPluginKeba::postSetupThing(Thing *thing)
{
    qCDebug(dcKeba()) << "Post setup" << thing->name();

    KeContact *keba = m_kebaDevices.value(thing->id());
    if (!keba) {
        qCWarning(dcKeba()) << "No Keba connection found for this thing while doing post setup.";
        return;
    } else {
        keba->getReport2();
        // No valid information if no meter
        if (thing->thingClassId() != kebaSimpleThingClassId)
            keba->getReport3();
    }

    // Try to find the mac address in case the user added the ip manually
    if (thing->paramValue(m_macAddressParamTypeIds.value(thing->thingClassId())).toString().isEmpty()
            || thing->paramValue(m_macAddressParamTypeIds.value(thing->thingClassId())).toString() == "00:00:00:00:00:00") {
        searchNetworkDevices();
    }

    if (!m_updateTimer) {
        m_updateTimer = hardwareManager()->pluginTimerManager()->registerTimer(10);
        connect(m_updateTimer, &PluginTimer::timeout, this, [this]() {
            foreach (const ThingId &thingId, m_kebaDevices.keys()) {
                KeContact *keba = m_kebaDevices.value(thingId);
                Thing *thing = myThings().findById(thingId);
                if (!keba) {
                    qCWarning(dcKeba()) << "No Keba connection found for" << thing->name();
                    return;
                }

                keba->getReport2();
                keba->getReport3();
                if (thing->stateValue("activity").toString() == "Charging") {
                    keba->getReport1XX(100);
                }
            }
        });

        m_updateTimer->start();
    }

    if (!m_reconnectTimer) {
        m_reconnectTimer = hardwareManager()->pluginTimerManager()->registerTimer(60 * 5);
        connect(m_reconnectTimer, &PluginTimer::timeout, this, [this] {
            bool startDiscoveryRequired = false;
            // Only search for new network devices if there is one keba which is not connected
            foreach (const ThingId &thingId, m_kebaDevices.keys()) {
                KeContact *keba = m_kebaDevices.value(thingId);
                Thing *thing = myThings().findById(thingId);
                if (!keba) {
                    qCWarning(dcKeba()) << "No Keba connection found for" << thing->name();
                    startDiscoveryRequired = true;
                    continue;
                }

                if (!keba->reachable()) {
                    startDiscoveryRequired = true;
                    return;
                }
            }

            if (startDiscoveryRequired)
                searchNetworkDevices();
        });

        m_reconnectTimer->start();
    }
}

void IntegrationPluginKeba::thingRemoved(Thing *thing)
{
    qCDebug(dcKeba()) << "Deleting" << thing->name();
    if (m_kebaDevices.contains(thing->id())) {
        KeContact *keba = m_kebaDevices.take(thing->id());
        keba->deleteLater();
    }

    m_lastSessionId.remove(thing->id());

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

void IntegrationPluginKeba::executeAction(ThingActionInfo *info)
{
    Thing *thing = info->thing();
    Action action = info->action();

    KeContact *keba = m_kebaDevices.value(thing->id());
    if (!keba) {
        qCWarning(dcKeba()) << "Device not properly initialized, Keba object missing";
        return info->finish(Thing::ThingErrorHardwareNotAvailable);
    }

    // Make sure keba is reachable
    if (!keba->reachable()) {
        qCWarning(dcKeba()) << "Failed to execute action. The keba seems not to be reachable" << thing;
        info->finish(Thing::ThingErrorHardwareNotAvailable);
        return;
    }

    QUuid requestId;
    if (thing->thingClassId() == kebaThingClassId) {
        if (action.actionTypeId() == kebaMaxChargingCurrentActionTypeId) {
            int milliAmpere = action.paramValue(kebaMaxChargingCurrentActionMaxChargingCurrentParamTypeId).toUInt() * 1000;
            requestId = keba->setMaxAmpereGeneral(milliAmpere);
        } else if (action.actionTypeId() == kebaPowerActionTypeId) {
            requestId = keba->enableOutput(action.param(kebaPowerActionTypeId).value().toBool());
        } else if (action.actionTypeId() == kebaDisplayActionTypeId) {
            requestId = keba->displayMessage(action.param(kebaDisplayActionMessageParamTypeId).value().toByteArray());
        } else if (action.actionTypeId() == kebaOutputX2ActionTypeId) {
            requestId = keba->setOutputX2(action.param(kebaOutputX2ActionOutputX2ParamTypeId).value().toBool());
        } else if (action.actionTypeId() == kebaFailsafeModeActionTypeId) {
            int timeout = 0;
            if (action.param(kebaFailsafeModeActionFailsafeModeParamTypeId).value().toBool()) {
                timeout = 60;
            }
            requestId = keba->setFailsafe(timeout, 0, false);
        } else {
            qCWarning(dcKeba()) << "Unhandled ActionTypeId:" << action.actionTypeId();
            return info->finish(Thing::ThingErrorActionTypeNotFound);
        }

    } else if (thing->thingClassId() == kebaSimpleThingClassId) {
        if (action.actionTypeId() == kebaSimpleMaxChargingCurrentActionTypeId) {
            int milliAmpere = action.paramValue(kebaSimpleMaxChargingCurrentActionMaxChargingCurrentParamTypeId).toUInt() * 1000;
            requestId = keba->setMaxAmpereGeneral(milliAmpere);
        } else if (action.actionTypeId() == kebaSimplePowerActionTypeId) {
            requestId = keba->enableOutput(action.param(kebaSimplePowerActionTypeId).value().toBool());
        } else if (action.actionTypeId() == kebaSimpleDisplayActionTypeId) {
            requestId = keba->displayMessage(action.param(kebaSimpleDisplayActionMessageParamTypeId).value().toByteArray());
        } else if (action.actionTypeId() == kebaSimpleOutputX2ActionTypeId) {
            requestId = keba->setOutputX2(action.param(kebaSimpleOutputX2ActionOutputX2ParamTypeId).value().toBool());
        } else if (action.actionTypeId() == kebaSimpleFailsafeModeActionTypeId) {
            int timeout = 0;
            if (action.param(kebaSimpleFailsafeModeActionFailsafeModeParamTypeId).value().toBool()) {
                timeout = 60;
            }
            requestId = keba->setFailsafe(timeout, 0, false);
        } else {
            qCWarning(dcKeba()) << "Unhandled ActionTypeId:" << action.actionTypeId();
            return info->finish(Thing::ThingErrorActionTypeNotFound);
        }
    }


    // If the keba returns an invalid uuid, something went wrong
    if (requestId.isNull()) {
        info->finish(Thing::ThingErrorHardwareFailure);
        return;
    }

    m_asyncActions.insert(requestId, info);
    connect(info, &ThingActionInfo::aborted, this, [requestId, this]{ m_asyncActions.remove(requestId); });
}

void IntegrationPluginKeba::onCommandExecuted(QUuid requestId, bool success)
{
    if (m_asyncActions.contains(requestId)) {
        KeContact *keba = static_cast<KeContact *>(sender());
        Thing *thing = myThings().findById(m_kebaDevices.key(keba));
        if (!thing) {
            qCWarning(dcKeba()) << "On command executed: missing device object";
            return;
        }

        ThingActionInfo *info = m_asyncActions.take(requestId);
        if (success) {
            qCDebug(dcKeba()) << "Action execution finished successfully. Request ID:" << requestId.toString();
            info->finish(Thing::ThingErrorNoError);

            if (thing->thingClassId() == kebaThingClassId) {
                // Set the value to the state so we don't have to wait for the report 2 response
                if (info->action().actionTypeId() == kebaMaxChargingCurrentActionTypeId) {
                    uint value = info->action().paramValue(kebaMaxChargingCurrentActionMaxChargingCurrentParamTypeId).toUInt();
                    info->thing()->setStateValue("maxChargingCurrent", value);
                } else if (info->action().actionTypeId() == kebaPowerActionTypeId) {
                    info->thing()->setStateValue("power", info->action().paramValue(kebaPowerActionTypeId).toBool());
                }
            } else if (thing->thingClassId() == kebaSimpleThingClassId) {
                // Set the value to the state so we don't have to wait for the report 2 response
                if (info->action().actionTypeId() == kebaSimpleMaxChargingCurrentActionTypeId) {
                    uint value = info->action().paramValue(kebaSimpleMaxChargingCurrentActionMaxChargingCurrentParamTypeId).toUInt();
                    info->thing()->setStateValue("maxChargingCurrent", value);
                } else if (info->action().actionTypeId() == kebaPowerActionTypeId) {
                    info->thing()->setStateValue("power", info->action().paramValue(kebaSimplePowerActionTypeId).toBool());
                }
            }
        } else {
            qCWarning(dcKeba()) << "Action execution finished with error. Request ID:" << requestId.toString();
            info->finish(Thing::ThingErrorHardwareFailure);
        }
    }
}

void IntegrationPluginKeba::setDeviceState(Thing *thing, KeContact::State state)
{
    switch (state) {
    case KeContact::StateStarting:
        thing->setStateValue("activity", "Starting");
        break;
    case KeContact::StateNotReady:
        thing->setStateValue("activity", "Not ready for charging");
        break;
    case KeContact::StateReady:
        thing->setStateValue("activity", "Ready for charging");
        break;
    case KeContact::StateCharging:
        thing->setStateValue("activity", "Charging");
        break;
    case KeContact::StateError:
        thing->setStateValue("activity", "Error");
        break;
    case KeContact::StateAuthorizationRejected:
        thing->setStateValue("activity", "Authorization rejected");
        break;
    }

    thing->setStateValue("charging", state == KeContact::StateCharging);
}

void IntegrationPluginKeba::setDevicePlugState(Thing *thing, KeContact::PlugState plugState)
{
    switch (plugState) {
    case KeContact::PlugStateUnplugged:
        thing->setStateValue("plugState", "Unplugged");
        break;
    case KeContact::PlugStatePluggedOnChargingStation:
        thing->setStateValue("plugState", "Plugged in charging station");
        break;
    case KeContact::PlugStatePluggedOnChargingStationAndPluggedOnEV:
        thing->setStateValue("plugState", "Plugged in on EV");
        break;
    case KeContact::PlugStatePluggedOnChargingStationAndPlugLocked:
        thing->setStateValue("plugState", "Plugged in and locked");
        break;
    case KeContact::PlugStatePluggedOnChargingStationAndPlugLockedAndPluggedOnEV:
        thing->setStateValue("plugState", "Plugged in on EV and locked");
        break;
    }

    if (plugState >= 5) {
        thing->setStateValue("pluggedIn", true);
    } else {
        thing->setStateValue("pluggedIn", false);
    }
}

void IntegrationPluginKeba::searchNetworkDevices()
{
    if (m_runningDiscovery) {
        qCDebug(dcKeba()) << "Keba discovery already running.";
        return;
    }

    if (!m_kebaDataLayer) {
        qCDebug(dcKeba()) << "Could not search keba wallboxes in the network. The data layer seems not to be available";
        return;
    }

    qCDebug(dcKeba()) << "Start searching keba wallboxes in the network...";
    m_runningDiscovery = new KebaDiscovery(m_kebaDataLayer, hardwareManager()->networkDeviceDiscovery(), this);
    connect(m_runningDiscovery, &KebaDiscovery::discoveryFinished, this, [=](){
        foreach (const KebaDiscovery::KebaDiscoveryResult &result, m_runningDiscovery->discoveryResults()) {
            foreach (const ThingId &thingId, m_kebaDevices.keys()) {
                Thing *existingThing = myThings().findById(thingId);
                if (!existingThing)
                    continue;

                if (existingThing->paramValue(m_macAddressParamTypeIds.value(existingThing->thingClassId())).toString().isEmpty()) {
                    //This device got probably manually setup, to enable auto rediscovery the MAC address needs to setup
                    if (existingThing->paramValue(m_ipAddressParamTypeIds.value(existingThing->thingClassId())).toString() == result.networkDeviceInfo.address().toString()) {
                        qCDebug(dcKeba()) << "Wallbox MAC address has been discovered" << existingThing->name() << result.networkDeviceInfo.macAddress();
                        existingThing->setParamValue(m_macAddressParamTypeIds.value(existingThing->thingClassId()), result.networkDeviceInfo.macAddress());
                    }
                } else if (existingThing->paramValue(m_macAddressParamTypeIds.value(existingThing->thingClassId())).toString() == result.networkDeviceInfo.macAddress())  {
                    // We found the existing keba thing, lets check if the ip has changed
                    if (existingThing->paramValue(m_ipAddressParamTypeIds.value(existingThing->thingClassId())).toString() != result.networkDeviceInfo.address().toString()) {
                        // Update the ip address of the thing.
                        // FIXME: as of now the thing manager does not store the changed param
                        qCDebug(dcKeba()) << "Wallbox IP Address has changed, from" << existingThing->paramValue(m_ipAddressParamTypeIds.value(existingThing->thingClassId())).toString()
                                          << "to" << result.networkDeviceInfo.address().toString();
                        existingThing->setParamValue(m_ipAddressParamTypeIds.value(existingThing->thingClassId()), result.networkDeviceInfo.address().toString());

                        // Make sure the setup has already run for this thing, if not, the thingmanager will retry with the new ip every 15 seconds
                        KeContact *keba = m_kebaDevices.value(existingThing->id());
                        if (keba) {
                            keba->setAddress(QHostAddress(result.networkDeviceInfo.address()));

                            // Refresh
                            keba->getReport2();
                            keba->getReport3();
                        } else {
                            qCWarning(dcKeba()) << "Could not update IP address since the keba connection has not been set up yet for" << existingThing;
                        }
                    } else {
                        qCDebug(dcKeba()) << "Wallbox" << existingThing->name() << "IP address has not changed" << result.networkDeviceInfo.address().toString();
                    }
                    break;
                }
            }
        }

        // Clean up
        m_runningDiscovery->deleteLater();
        m_runningDiscovery = nullptr;
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

    thing->setStateValue("connected", status);
    if (!status) {
        searchNetworkDevices();
    }
}

void IntegrationPluginKeba::onReportTwoReceived(const KeContact::ReportTwo &reportTwo)
{
    KeContact *keba = static_cast<KeContact *>(sender());
    Thing *thing = myThings().findById(m_kebaDevices.key(keba));
    if (!thing)
        return;

    qCDebug(dcKeba()) << "Report 2 received for" << thing->name() << "Serial number:" << thing->paramValue(m_serialNumberParamTypeIds.value(thing->thingClassId())).toString();
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

    if (reportTwo.serialNumber == thing->paramValue(m_serialNumberParamTypeIds.value(thing->thingClassId())).toString()) {
        setDeviceState(thing, reportTwo.state);
        setDevicePlugState(thing, reportTwo.plugState);

        thing->setStateValue("power", reportTwo.enableUser);
        thing->setStateValue("error1", reportTwo.error1);
        thing->setStateValue("error2", reportTwo.error2);
        thing->setStateValue("systemEnabled", reportTwo.enableSys);

        thing->setStateValue("maxChargingCurrent", qRound(reportTwo.currentUser));
        thing->setStateValue("maxChargingCurrentPercent", reportTwo.maxCurrentPercentage);
        thing->setStateValue("maxChargingCurrentHardware", reportTwo.currentHardwareLimitation);

        // Set the state limits according to the hardware limits
        if (reportTwo.currentHardwareLimitation > 0) {
            thing->setStateMaxValue("maxChargingCurrent", reportTwo.currentHardwareLimitation);
        } else {
            // If we have no limit given, reset to the statetype limit
            thing->setStateMaxValue("maxChargingCurrent", thing->thingClass().stateTypes().findByName("maxChargingCurrent").maxValue());
        }
        thing->setStateValue("outputX2", reportTwo.output);
        thing->setStateValue("input", reportTwo.input);

        thing->setStateValue("uptime", reportTwo.seconds / 60);
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

    qCDebug(dcKeba()) << "Report 3 received for" << thing->name() << "Serial number:" << thing->paramValue(m_serialNumberParamTypeIds.value(thing->thingClassId())).toString();
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

    // Note: all these infos are from the meter...
    if (thing->thingClassId() == kebaSimpleThingClassId) {
        qCDebug(dcKeba()) << "Received report 3 from keba but this model has no meter. Ignoring report data...";
        return;
    }

    if (reportThree.serialNumber == thing->paramValue(m_serialNumberParamTypeIds.value(thing->thingClassId())).toString()) {
        thing->setStateValue("currentPhaseA", reportThree.currentPhase1);
        thing->setStateValue("currentPhaseB", reportThree.currentPhase2);
        thing->setStateValue("currentPhaseC", reportThree.currentPhase3);
        thing->setStateValue("voltagePhaseA", reportThree.voltagePhase1);
        thing->setStateValue("voltagePhaseB", reportThree.voltagePhase2);
        thing->setStateValue("voltagePhaseC", reportThree.voltagePhase3);
        thing->setStateValue("currentPower", reportThree.power);
        thing->setStateValue("sessionEnergy", reportThree.energySession);
        thing->setStateValue("powerFactor", reportThree.powerFactor);
        thing->setStateValue("totalEnergyConsumed", reportThree.energyTotal);

        // Check how many phases are actually charging, and update the phase count only if something happens on the phases (current or power)
        if (!(reportThree.currentPhase1 == 0 && reportThree.currentPhase2 == 0 && reportThree.currentPhase3 == 0)) {
            uint phaseCount = 0;
            if (reportThree.currentPhase1 != 0)
                phaseCount += 1;

            if (reportThree.currentPhase2 != 0)
                phaseCount += 1;

            if (reportThree.currentPhase3 != 0)
                phaseCount += 1;

            thing->setStateValue("phaseCount", phaseCount);
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

    qCDebug(dcKeba()) << "Report" << reportNumber << "received for" << thing->name() << "Serial number:" << thing->paramValue(m_serialNumberParamTypeIds.value(thing->thingClassId())).toString();
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

    // Note: all these infos are from the meter...
    if (thing->thingClassId() == kebaSimpleThingClassId) {
        qCDebug(dcKeba()) << "Received" << reportNumber << "from keba but this model has no meter. Ignoring report data...";
        return;
    }

    if (reportNumber == 100) {
        // Report 100 is the current charging session
        if (report.endTime == 0) {
            // if the charing session is finished the end time will be set
            double duration = (report.seconds - report.startTime) / 60.00;
            thing->setStateValue("sessionTime", duration);
        } else {
            // Charging session is finished and copied to Report 101
        }

    } else if (reportNumber == 101) {
        // Report 101 is the lastest finished session
        if (report.serialNumber == thing->paramValue(m_serialNumberParamTypeIds.value(thing->thingClassId())).toString()) {
            if (!m_lastSessionId.contains(thing->id())) {
                // This happens after reboot
                m_lastSessionId.insert(thing->id(), report.sessionId);
            } else {
                if (m_lastSessionId.value(thing->id()) != report.sessionId) {
                    qCDebug(dcKeba()) << "New session id receivd";
                    Event event;
                    event.setEventTypeId(kebaChargingSessionFinishedEventTypeId);
                    event.setThingId(thing->id());
                    ParamList params;
                    params << Param(kebaChargingSessionFinishedEventEnergyParamTypeId, report.presentEnergy);
                    params << Param(kebaChargingSessionFinishedEventDurationParamTypeId, report.endTime);
                    params << Param(kebaChargingSessionFinishedEventIdParamTypeId);
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
        thing->setStateValue("input", (content.toInt() == 1));
        break;
    case KeContact::BroadcastTypeEPres:
        thing->setStateValue("sessionEnergy", content.toInt() / 10000.00);
        break;
    case KeContact::BroadcastTypeState:
        setDeviceState(thing, KeContact::State(content.toInt()));
        break;
    case KeContact::BroadcastTypeMaxCurr:
        //Current preset value via Control pilot in milliampere
        break;
    case KeContact::BroadcastTypeEnableSys:
        thing->setStateValue("systemEnabled", (content.toInt() != 0));
        break;
    }
}
