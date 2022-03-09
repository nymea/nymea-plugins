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
#include "integrationpluginusbrly82.h"

IntegrationPluginUsbRly82::IntegrationPluginUsbRly82()
{

}

void IntegrationPluginUsbRly82::init()
{
    m_monitor = new SerialPortMonitor(this);
}

void IntegrationPluginUsbRly82::discoverThings(ThingDiscoveryInfo *info)
{
    info->finish(Thing::ThingErrorNoError);
}

void IntegrationPluginUsbRly82::startMonitoringAutoThings()
{
    // Start seaching for devices which can be discovered and added automatically
    connect(m_monitor, &SerialPortMonitor::serialPortAdded, this, &IntegrationPluginUsbRly82::onSerialPortAdded);
    connect(m_monitor, &SerialPortMonitor::serialPortRemoved, this, &IntegrationPluginUsbRly82::onSerialPortRemoved);

    // Check the initial list of
    foreach (const SerialPortMonitor::SerialPortInfo &serialPortInfo, m_monitor->serialPortInfos()) {
        onSerialPortAdded(serialPortInfo);
    }
}

void IntegrationPluginUsbRly82::setupThing(ThingSetupInfo *info)
{
    Thing *thing = info->thing();
    qCDebug(dcUsbRly82()) << "Setup thing" << thing;

    // Relay
    if (info->thing()->thingClassId() == usbRelayThingClassId) {

        // Search for the serial port with the given serialnumber
        foreach (const SerialPortMonitor::SerialPortInfo &serialPortInfo, m_monitor->serialPortInfos()) {
            if (serialPortInfo.serialNumber == thing->paramValue(usbRelayThingSerialNumberParamTypeId).toString()) {
                qCDebug(dcUsbRly82()) << "Found serial port for" << thing << serialPortInfo;

                UsbRly82 *relay = new UsbRly82(this);
                connect(relay, &UsbRly82::availableChanged, thing, [=](bool available){
                    qCDebug(dcUsbRly82()) << thing << "available changed" << available;
                    thing->setStateValue("connected", available);
                });

                connect(relay, &UsbRly82::powerRelay1Changed, thing, [=](bool power){
                    qCDebug(dcUsbRly82()) << thing << "relay 1 power changed" << power;
                    thing->setStateValue(usbRelayPowerRelay1StateTypeId, power);
                });

                connect(relay, &UsbRly82::powerRelay2Changed, thing, [=](bool power){
                    qCDebug(dcUsbRly82()) << thing << "relay 2 power changed" << power;
                    thing->setStateValue(usbRelayPowerRelay1StateTypeId, power);
                });


                connect(relay, &UsbRly82::digitalInputsChanged, thing, [=](){
                    thing->setStateValue(usbRelayPowerRelay1StateTypeId, UsbRly82::checkBit());
                });


                if (!relay->connectRelay(serialPortInfo.systemLocation)) {
                    qCWarning(dcUsbRly82()) << "Setup failed. Could not connect to relay" << thing;
                    info->finish(Thing::ThingErrorHardwareFailure);
                    relay->deleteLater();
                    return;
                }

                m_relays.insert(thing, relay);
                info->finish(Thing::ThingErrorNoError);
                return;
            }
        }

        // Does not seem to be available
        info->finish(Thing::ThingErrorHardwareNotAvailable);
        return;
    }

    info->finish(Thing::ThingErrorSetupFailed);
}


void IntegrationPluginUsbRly82::postSetupThing(Thing *thing)
{
    qCDebug(dcUsbRly82()) << "Post setup thing" << thing;

    //    if (thing->thingClassId() == usbRelayConnectorThingClassId) {

    //        // Initialize the states
    //        UsbRelay *relay = m_relays.key(thing);
    //        if (!relay) {
    //            qCWarning(dcUsbRly82()) << "Could not find relay in post setup.";
    //            return;
    //        }

    //        thing->setStateValue(usbRelayConnectorConnectedStateTypeId, relay->connected());

    //        // Check if we have to create child devices (relays)
    //        if (myThings().filterByParentId(thing->id()).isEmpty()) {

    //            ThingDescriptors descriptors;
    //            for (int i = 0; i < relay->relayCount(); i++) {
    //                int relayNumber = i + 1;
    //                ThingDescriptor descriptor(usbRelayThingClassId, QString("Relay %1").arg(relayNumber), QString(), thing->id());
    //                ParamList params;
    //                params.append(Param(usbRelayThingRelayNumberParamTypeId, relayNumber));
    //                descriptor.setParams(params);
    //                descriptors.append(descriptor);
    //            }

    //            emit autoThingsAppeared(descriptors);
    //        }
    //    } else if (thing->thingClassId() == usbRelayThingClassId) {

    //        UsbRelay *relay = getRelayForDevice(thing);
    //        if (!relay) return;

    //        // Set the current states
    //        int relayNumber = thing->paramValue(usbRelayThingRelayNumberParamTypeId).toInt();
    //        thing->setStateValue(usbRelayConnectedStateTypeId, relay->connected());
    //        thing->setStateValue(usbRelayPowerStateTypeId, relay->relayPower(relayNumber));
    //    }
}

void IntegrationPluginUsbRly82::thingRemoved(Thing *thing)
{
    qCDebug(dcUsbRly82()) << "Remove thing" << thing;
    if (thing->thingClassId() == usbRelayThingClassId) {
        UsbRly82 *relay = m_relays.take(thing);
        if (!relay) return;
        delete relay;
    }
}


void IntegrationPluginUsbRly82::executeAction(ThingActionInfo *info)
{
    qCDebug(dcUsbRly82()) << "Executing action for thing" << info->thing() << info->action().actionTypeId().toString() << info->action().params();

    if (info->thing()->thingClassId() == usbRelayThingClassId) {

            Thing *thing = info->thing();
            UsbRly82 *relay = m_relays.value(thing);

            if (!relay) {
                qCWarning(dcUsbRly82()) << "Could execute action because could not find USB relay for" << thing;
                info->finish(Thing::ThingErrorHardwareNotAvailable);
                return;
            }

            if (!relay->available()) {
                qCWarning(dcUsbRly82()) << "Relay is not connected";
                info->finish(Thing::ThingErrorHardwareNotAvailable);
                return;
            }

            if (info->action().actionTypeId() == usbRelayPowerRelay1ActionTypeId) {
                bool power = info->action().paramValue(usbRelayPowerRelay1ActionPowerRelay1ParamTypeId).toBool();
                UsbRly82Reply *reply = relay->setRelay1Power(power);
                connect(reply, &UsbRly82Reply::finished, info, [=](){
                    if (reply->error() != UsbRly82Reply::ErrorNoError) {
                        info->finish(Thing::ThingErrorHardwareFailure);
                        return;
                    }

                    info->finish(Thing::ThingErrorNoError);
                });
                return;
            } else if (info->action().actionTypeId() == usbRelayPowerRelay2ActionTypeId) {
                bool power = info->action().paramValue(usbRelayPowerRelay2ActionPowerRelay2ParamTypeId).toBool();
                UsbRly82Reply *reply = relay->setRelay2Power(power);
                connect(reply, &UsbRly82Reply::finished, info, [=](){
                    if (reply->error() != UsbRly82Reply::ErrorNoError) {
                        info->finish(Thing::ThingErrorHardwareFailure);
                        return;
                    }

                    info->finish(Thing::ThingErrorNoError);
                });
                return;
            }

            info->finish(Thing::ThingErrorActionTypeNotFound);
        }

    info->finish(Thing::ThingErrorThingClassNotFound);
}

void IntegrationPluginUsbRly82::onSerialPortAdded(const SerialPortMonitor::SerialPortInfo &serialPortInfo)
{
    if (serialPortInfo.vendorId == 0x04d8 && serialPortInfo.productId == 0xffee) {
        qCDebug(dcUsbRly82()) << "[+] Added" << serialPortInfo;
        Things alreadyAddedThings = myThings().filterByThingClassId(usbRelayThingClassId).filterByParam(usbRelayThingSerialNumberParamTypeId, serialPortInfo.serialNumber);
        if (alreadyAddedThings.isEmpty()) {
            qCDebug(dcUsbRly82()) << "New" << serialPortInfo.product << serialPortInfo.serialNumber << "showed up. Setting up a new thing for this.";
            ThingDescriptor descriptor(usbRelayThingClassId, "USB-RLY82");
            ParamList params;
            params.append(Param(usbRelayThingSerialNumberParamTypeId, serialPortInfo.serialNumber));
            descriptor.setParams(params);
            emit autoThingsAppeared(ThingDescriptors() << descriptor);
        } else {
            Thing *relayThing = alreadyAddedThings.first();
            if (relayThing) {
                qCDebug(dcUsbRly82()) << "Thing already set up for this controller" << relayThing;
                UsbRly82 *relay = m_relays.value(relayThing);
                if (relay) {
                    relay->connectRelay(serialPortInfo.systemLocation);
                }
            }
        }
    }
}

void IntegrationPluginUsbRly82::onSerialPortRemoved(const SerialPortMonitor::SerialPortInfo &serialPortInfo)
{
    if (serialPortInfo.vendorId == 0x04d8 && serialPortInfo.productId == 0xffee) {
        qCDebug(dcUsbRly82()) << "[-] Removed" << serialPortInfo;
    }
}
