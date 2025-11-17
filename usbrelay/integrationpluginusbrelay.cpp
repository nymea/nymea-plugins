// SPDX-License-Identifier: GPL-3.0-or-later

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*
* Copyright (C) 2013 - 2024, nymea GmbH
* Copyright (C) 2024 - 2025, chargebyte austria GmbH
*
* This file is part of nymea-plugins.
*
* nymea-plugins is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* (at your option) any later version.
*
* nymea-plugins is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
* General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with nymea-plugins. If not, see <https://www.gnu.org/licenses/>.
*
* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

#include "integrationpluginusbrelay.h"
#include "plugininfo.h"
#include "usbrelay.h"

#include <QTimer>
#include <hidapi/hidapi.h>

IntegrationPluginUsbRelay::IntegrationPluginUsbRelay()
{

}

void IntegrationPluginUsbRelay::init()
{
    // Initialize/create objects
}

void IntegrationPluginUsbRelay::startMonitoringAutoThings()
{
    // Start seaching for devices which can be discovered and added automatically
}

void IntegrationPluginUsbRelay::postSetupThing(Thing *thing)
{
    qCDebug(dcUsbRelay()) << "Post setup thing" << thing;

    if (thing->thingClassId() == usbRelayConnectorThingClassId) {

        // Initialize the states
        UsbRelay *relay = m_relayDevices.key(thing);
        if (!relay) {
            qCWarning(dcUsbRelay()) << "Could not find relay in post setup.";
            return;
        }

        thing->setStateValue(usbRelayConnectorConnectedStateTypeId, relay->connected());

        // Check if we have to create child devices (relays)
        if (myThings().filterByParentId(thing->id()).isEmpty()) {

            ThingDescriptors descriptors;
            for (int i = 0; i < relay->relayCount(); i++) {
                int relayNumber = i + 1;
                ThingDescriptor descriptor(usbRelayThingClassId, QString("Relay %1").arg(relayNumber), QString(), thing->id());
                ParamList params;
                params.append(Param(usbRelayThingRelayNumberParamTypeId, relayNumber));
                descriptor.setParams(params);
                descriptors.append(descriptor);
            }

            emit autoThingsAppeared(descriptors);
        }
    } else if (thing->thingClassId() == usbRelayThingClassId) {

        UsbRelay *relay = getRelayForDevice(thing);
        if (!relay) return;

        // Set the current states
        int relayNumber = thing->paramValue(usbRelayThingRelayNumberParamTypeId).toInt();
        thing->setStateValue(usbRelayConnectedStateTypeId, relay->connected());
        thing->setStateValue(usbRelayPowerStateTypeId, relay->relayPower(relayNumber));
    }
}

void IntegrationPluginUsbRelay::thingRemoved(Thing *thing)
{
    qCDebug(dcUsbRelay()) << "Remove thing" << thing;
    if (thing->thingClassId() == usbRelayConnectorThingClassId) {
        UsbRelay *relay = m_relayDevices.key(thing);
        if (!relay) return;
        m_relayDevices.remove(relay);
        delete relay;
    }
}

void IntegrationPluginUsbRelay::discoverThings(ThingDiscoveryInfo *info)
{
    // Init
    if (hid_init() < 0) {
        qCWarning(dcUsbRelay()) << "Could not initialize HID.";
        info->finish(Thing::ThingErrorHardwareFailure, QT_TR_NOOP("Cannot discover USB devices. HID initialisation failed on this system."));
        return;
    }

    // Enumerate hid devices
    struct hid_device_info *devices = nullptr;
    struct hid_device_info *currentDevice = nullptr;
    devices = hid_enumerate(0x16c0, 0x05df);
    int relayCount = 0;
    currentDevice = devices;

    for (relayCount = 0; currentDevice != nullptr; relayCount++) {
        currentDevice = currentDevice->next;
    }

    qCDebug(dcUsbRelay()) << "Found" << relayCount << "usb relay devices";
    currentDevice = devices;
    for (int i = 0; i < relayCount; i++) {
        QString path = QString::fromLatin1(currentDevice->path);
        QString manufacturer = QString::fromWCharArray(currentDevice->manufacturer_string);
        QString product = QString::fromWCharArray(currentDevice->product_string);
        QString serialnumber = QString::fromWCharArray(currentDevice->serial_number);
        quint16 releaseNumber = static_cast<quint16>(currentDevice->release_number);
        quint16 productId = static_cast<quint16>(currentDevice->product_id);
        quint16 vendorId = static_cast<quint16>(currentDevice->vendor_id);
        int relayCount = QString(product.at(product.length() -1)).toInt();

        qCDebug(dcUsbRelay()) << path << manufacturer << product << serialnumber << "Relay count" << relayCount << QString("%1:%2").arg(QString::number(vendorId, 16), QString::number(productId, 16)) << releaseNumber;

        // Open it to get more details
        hid_device *hidDevice = nullptr;
        hidDevice = hid_open_path(currentDevice->path);
        if (!hidDevice) {
            qCWarning(dcUsbRelay()) << "Could not create HID thing for" << path;
            continue;
        }

        unsigned char buf[9];
        buf[0] = 0x01;
        int ret = hid_get_feature_report(hidDevice, buf, sizeof(buf));
        if (ret < 0) {
            qCWarning(dcUsbRelay()) << "Could not create HID thing hidDevice for" << path;
            continue;
        }

        quint8 relayStatus = static_cast<quint8>(buf[7]);
        for (int i = 0; i < relayCount; i++) {
            int relayNumber = i + 1;
            bool power = static_cast<bool>(relayStatus & 1 << i);
            qCDebug(dcUsbRelay()) << "--> Relay" << relayNumber << ":" << power;
        }

        hid_close(hidDevice);

        ThingDescriptor descriptor(usbRelayConnectorThingClassId, "USB Relay Connector", QString("%1 (%2)").arg(product, path));
        ParamList params;
        params.append(Param(usbRelayConnectorThingPathParamTypeId, path));
        params.append(Param(usbRelayConnectorThingRelayCountParamTypeId, relayCount));
        descriptor.setParams(params);

        // Set the current thing id if we already have a thing on this path
        foreach (Thing *existingThing, myThings()) {
            if (existingThing->paramValue(usbRelayConnectorThingPathParamTypeId).toString() == path &&
                    existingThing->paramValue(usbRelayConnectorThingRelayCountParamTypeId).toInt() == relayCount) {
                descriptor.setThingId(existingThing->id());
                break;
            }
        }
        info->addThingDescriptor(descriptor);
    }

    hid_free_enumeration(devices);
    hid_exit();

    info->finish(Thing::ThingErrorNoError);
}

void IntegrationPluginUsbRelay::setupThing(ThingSetupInfo *info)
{
    qCDebug(dcUsbRelay()) << "Setup thing" << info->thing();

    // Relay connector
    if (info->thing()->thingClassId() == usbRelayConnectorThingClassId) {
        Thing *thing = info->thing();
        QString path = thing->paramValue(usbRelayConnectorThingPathParamTypeId).toString();
        int relayCount = thing->paramValue(usbRelayConnectorThingRelayCountParamTypeId).toInt();

        UsbRelay *relay = new UsbRelay(path, relayCount, this);
        m_relayDevices.insert(relay, thing);

        connect(relay, &UsbRelay::connectedChanged, this, [this, thing, relay](bool connected) {
            qCDebug(dcUsbRelay()) << "Device" << thing->name() << (connected ? "connected" : "disconnected");
            thing->setStateValue(usbRelayConnectorConnectedStateTypeId, connected);

            // Set the connected state of all child devices
            foreach (Thing *childDevice, myThings().filterByParentId(thing->id())) {
                if (childDevice->thingClassId() == usbRelayThingClassId && childDevice->setupComplete()) {
                    childDevice->setStateValue(usbRelayConnectedStateTypeId, connected);
                    if (connected) {
                        childDevice->setStateValue(usbRelayPowerStateTypeId, relay->relayPower(childDevice->paramValue(usbRelayThingRelayNumberParamTypeId).toInt()));
                    }
                }
            }
        });

        connect(relay, &UsbRelay::relayPowerChanged, this, [this, thing](int relayNumber, bool power) {
            Thing *relayDevice = getRelayDevice(thing, relayNumber);
            if (!relayDevice) {
                // Note: probably not set up yet
                qCWarning(dcUsbRelay()) << "Could not find USB relay child thing for" << thing << relayNumber;
                return;
            }

            relayDevice->setStateValue(usbRelayPowerStateTypeId, power);
        });

        info->finish(Thing::ThingErrorNoError);
        return;
    }

    // Relay
    if (info->thing()->thingClassId() == usbRelayThingClassId) {
        info->finish(Thing::ThingErrorNoError);
        return;
    }

    info->finish(Thing::ThingErrorSetupFailed);
}

void IntegrationPluginUsbRelay::executeAction(ThingActionInfo *info)
{
    qCDebug(dcUsbRelay()) << "Executing action for thing" << info->thing() << info->action().actionTypeId().toString() << info->action().params();

    if (info->thing()->thingClassId() == usbRelayThingClassId) {

        Thing *thing = info->thing();
        UsbRelay *relay = getRelayForDevice(thing);

        if (!relay) {
            qCWarning(dcUsbRelay()) << "Could execute action because could not find USB relay for" << thing;
            info->finish(Thing::ThingErrorHardwareNotAvailable);
            return;
        }

        if (!relay->connected()) {
            qCWarning(dcUsbRelay()) << "Relay is not connected";
            info->finish(Thing::ThingErrorHardwareNotAvailable);
            return;
        }

        int relayNumber = thing->paramValue(usbRelayThingRelayNumberParamTypeId).toInt();

        if (info->action().actionTypeId() == usbRelayPowerActionTypeId) {
            bool power = info->action().param(usbRelayPowerActionPowerParamTypeId).value().toBool();
            if (!relay->setRelayPower(relayNumber, power)) {
                info->finish(Thing::ThingErrorHardwareFailure);
                return;
            }
            info->finish(Thing::ThingErrorNoError);
            return;
        }

        info->finish(Thing::ThingErrorActionTypeNotFound);
    }

    info->finish(Thing::ThingErrorThingClassNotFound);
}

Thing *IntegrationPluginUsbRelay::getRelayDevice(Thing *parentDevice, int relayNumber)
{
    foreach (Thing *childDevice, myThings().filterByParentId(parentDevice->id())) {
        if (childDevice->thingClassId() == usbRelayThingClassId) {
            if (childDevice->paramValue(usbRelayThingRelayNumberParamTypeId).toInt() == relayNumber) {
                return childDevice;
            }
        }
    }

    return nullptr;
}

UsbRelay *IntegrationPluginUsbRelay::getRelayForDevice(Thing *relayDevice)
{
    Thing *parentDevice = myThings().findById(relayDevice->parentId());
    if (!parentDevice) {
        qCWarning(dcUsbRelay()) << "Could not find the parent thing for" << relayDevice;
        return nullptr;
    }

    UsbRelay *relay = m_relayDevices.key(parentDevice);
    if (!relay) {
        qCWarning(dcUsbRelay()) << "Could not find USB relay for" << relayDevice;
        return nullptr;
    }

    return relay;
}
