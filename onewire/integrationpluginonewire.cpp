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

#include "integrationpluginonewire.h"
#include "integrations/thing.h"
#include "plugininfo.h"

#include <QDebug>
#include <QDir>

IntegrationPluginOneWire::IntegrationPluginOneWire()
{
}

void IntegrationPluginOneWire::discoverThings(ThingDiscoveryInfo *info)
{
    ThingClassId deviceClassId = info->thingClassId();

    if (deviceClassId == temperatureSensorThingClassId       ||
            deviceClassId == temperatureHumiditySensorThingClassId ||
            deviceClassId == singleChannelSwitchThingClassId ||
            deviceClassId == dualChannelSwitchThingClassId   ||
            deviceClassId == eightChannelSwitchThingClassId) {

        if (myThings().filterByThingClassId(oneWireInterfaceThingClassId).isEmpty()) {
            if (!m_w1Interface) {
                m_w1Interface = new W1(this);
            }

            if (!m_w1Interface->interfaceIsAvailable()) {
                return info->finish(Thing::ThingErrorHardwareNotAvailable, QT_TR_NOOP("No one wire interface initialized. Please set up a one wire interface first."));
            }
            QStringList deviceList = m_w1Interface->discoverDevices();

            Q_FOREACH(QString device, deviceList) {
                if (device.startsWith("10") ||
                        device.startsWith("22") ||
                        device.startsWith("28") ||
                        device.startsWith("3B", Qt::CaseInsensitive)) {

                    QString type = "Unkown";
                    if (device.startsWith("10")) { //
                        type = "DS18S20";
                    } else if (device.startsWith("22")) { //
                        type = "DS1822";
                    } else if (device.startsWith("28")) { //
                        type = "DS18B20";
                    } else if (device.startsWith("3B", Qt::CaseInsensitive)) { //DS1825
                        type = "DS1825";
                    }
                    ThingDescriptor descriptor(temperatureSensorThingClassId, type, "One wire temperature sensor");
                    ParamList params;
                    params.append(Param(temperatureSensorThingAddressParamTypeId, device));
                    params.append(Param(temperatureSensorThingTypeParamTypeId, type));
                    foreach (Thing *existingThing, myThings().filterByThingClassId(temperatureSensorThingClassId)){
                        if (existingThing->paramValue(temperatureSensorThingAddressParamTypeId).toString() == device) {
                            descriptor.setThingId(existingThing->id());
                            break;
                        }
                    }
                    descriptor.setParams(params);
                    info->addThingDescriptor(descriptor);
                }
            }
            return info->finish(Thing::ThingErrorNoError);
        } else {
            Thing *parentDevice = myThings().filterByThingClassId(oneWireInterfaceThingClassId).first();
            m_runningDiscoveries.insert(parentDevice, info);
            connect(info, &ThingDiscoveryInfo::destroyed, this, [this, parentDevice](){
                m_runningDiscoveries.remove(parentDevice);
            });

            if (m_owfsInterface)
                m_owfsInterface->discoverDevices();
        }
        return;
    } else {
        qCWarning(dcOneWire()) << "Discovery called for a device class which does not support discovery? Device class ID:" << info->thingClassId().toString();
        info->finish(Thing::ThingErrorThingClassNotFound);
    }
}


void IntegrationPluginOneWire::setupThing(ThingSetupInfo *info)
{
    Thing *thing = info->thing();
    if (thing->thingClassId() == oneWireInterfaceThingClassId) {
        qCDebug(dcOneWire) << "Setup one wire interface";

        if (m_owfsInterface) {
            qCWarning(dcOneWire) << "One wire interface already set up";
            //: Error setting up thing
            return info->finish(Thing::ThingErrorThingInUse, QT_TR_NOOP("There can only be one one wire interface per system."));
        }
        m_owfsInterface = new Owfs(this);
        QByteArray initArguments = thing->paramValue(oneWireInterfaceThingInitArgsParamTypeId).toByteArray();

        if (!m_owfsInterface->init(initArguments)){
            m_owfsInterface->deleteLater();
            m_owfsInterface = nullptr;
            //: Error setting up thing
            return info->finish(Thing::ThingErrorHardwareFailure, QT_TR_NOOP("Error initializing one wire interface."));
        }
        connect(m_owfsInterface, &Owfs::devicesDiscovered, this, &IntegrationPluginOneWire::onOneWireDevicesDiscovered);
        return info->finish(Thing::ThingErrorNoError);

    } else if (thing->thingClassId() == temperatureSensorThingClassId) {

        qCDebug(dcOneWire) << "Setup one wire temperature sensor" << thing->params();
        if (!thing->parentId().isNull()) {
            // If there is a parent then it is from an OWFS interface
            Thing *parentThing = myThings().findById(thing->parentId());
            if (!parentThing) {
                qCWarning(dcOneWire()) << "Could not find parent thing for" << thing->name();
            }
            if (parentThing->setupComplete()) {
                setupOwfsTemperatureSensor(info);
            } else {
                connect(parentThing, &Thing::setupStatusChanged, info, [info, parentThing, this] {
                    if (parentThing->setupComplete()) {
                        setupOwfsTemperatureSensor(info);
                    }
                });
            }
        } else {
            if (!m_w1Interface) {
                m_w1Interface = new W1(this);
            }
            if (m_w1Interface->interfaceIsAvailable()) {
                QString address = thing->paramValue(temperatureSensorThingAddressParamTypeId).toString();
                thing->setStateValue(temperatureSensorConnectedStateTypeId,  m_w1Interface->deviceAvailable(address));
                thing->setStateValue(temperatureSensorTemperatureStateTypeId, m_w1Interface->getTemperature(address));
                return info->finish(Thing::ThingErrorNoError);
            } else {
                qCWarning(dcOneWire()) << "W1 interface is not available";
                return info->finish(Thing::ThingErrorHardwareNotAvailable);
            }
        }
    } else if (thing->thingClassId() == temperatureHumiditySensorThingClassId) {

        qCDebug(dcOneWire) << "Setup one wire temperature and humidity sensor" << thing->params();
        Thing *parentThing = myThings().findById(thing->parentId());
        if (!parentThing) {
            qCWarning(dcOneWire()) << "Could not find parent thing for" << thing->name();
        }
        if (parentThing->setupComplete()) {
            setupOwfsTemperatureHumiditySensor(info);
        } else {
            connect(parentThing, &Thing::setupStatusChanged, info, [info, parentThing, this] {
                if (parentThing->setupComplete()) {
                    setupOwfsTemperatureHumiditySensor(info);
                }
            });
        }
        return info->finish(Thing::ThingErrorNoError);

    } else if (thing->thingClassId() == singleChannelSwitchThingClassId) {
        qCDebug(dcOneWire) << "Setup one wire switch" << thing->params();
        if (m_owfsInterface) {
            QByteArray address = thing->paramValue(singleChannelSwitchThingAddressParamTypeId).toByteArray();
            thing->setStateValue(singleChannelSwitchConnectedStateTypeId, m_owfsInterface->isConnected(address));
            bool ok;
            bool output = m_owfsInterface->getSwitchOutput(address, Owfs::SwitchChannel::PIO_A, &ok);
            if (ok) {
                thing->setStateValue(singleChannelSwitchDigitalOutputStateTypeId, output);
            }
        }
        return info->finish(Thing::ThingErrorNoError);

    } else if (thing->thingClassId() == dualChannelSwitchThingClassId) {
        qCDebug(dcOneWire) << "Setup one wire dual switch" << thing->params();
        if (m_owfsInterface) {
            QByteArray address = thing->paramValue(dualChannelSwitchThingAddressParamTypeId).toByteArray();
            thing->setStateValue(dualChannelSwitchConnectedStateTypeId, m_owfsInterface->isConnected(address));
            bool ok;
            bool output = m_owfsInterface->getSwitchOutput(address, Owfs::SwitchChannel::PIO_A, &ok);
            if (ok) {
                thing->setStateValue(dualChannelSwitchDigitalOutput1StateTypeId, output);
            }
            output = m_owfsInterface->getSwitchOutput(address, Owfs::SwitchChannel::PIO_B, &ok);
            if (ok) {
                thing->setStateValue(dualChannelSwitchDigitalOutput2StateTypeId, output);
            }
        }
        return info->finish(Thing::ThingErrorNoError);

    } else if (thing->thingClassId() == eightChannelSwitchThingClassId) {
        qCDebug(dcOneWire) << "Setup one wire eight channel switch" << thing->params();
        if (m_owfsInterface) {
            QByteArray address = thing->paramValue(eightChannelSwitchThingAddressParamTypeId).toByteArray();
            thing->setStateValue(eightChannelSwitchConnectedStateTypeId, m_owfsInterface->isConnected(address));
            bool ok;
            bool output =  m_owfsInterface->getSwitchOutput(address, Owfs::SwitchChannel::PIO_A, &ok);
            if (ok) {
                thing->setStateValue(eightChannelSwitchDigitalOutput1StateTypeId, output);
            }
            output =  m_owfsInterface->getSwitchOutput(address, Owfs::SwitchChannel::PIO_B, &ok);
            if (ok) {
                thing->setStateValue(eightChannelSwitchDigitalOutput2StateTypeId, output);
            }
            output =  m_owfsInterface->getSwitchOutput(address, Owfs::SwitchChannel::PIO_C, &ok);
            if (ok) {
                thing->setStateValue(eightChannelSwitchDigitalOutput3StateTypeId, output);
            }
            output =  m_owfsInterface->getSwitchOutput(address, Owfs::SwitchChannel::PIO_D, &ok);
            if (ok) {
                thing->setStateValue(eightChannelSwitchDigitalOutput4StateTypeId, output);
            }
            output =  m_owfsInterface->getSwitchOutput(address, Owfs::SwitchChannel::PIO_E, &ok);
            if (ok) {
                thing->setStateValue(eightChannelSwitchDigitalOutput5StateTypeId, output);
            }
            output =  m_owfsInterface->getSwitchOutput(address, Owfs::SwitchChannel::PIO_F, &ok);
            if (ok) {
                thing->setStateValue(eightChannelSwitchDigitalOutput6StateTypeId, output);
            }
            output =  m_owfsInterface->getSwitchOutput(address, Owfs::SwitchChannel::PIO_G, &ok);
            if (ok) {
                thing->setStateValue(eightChannelSwitchDigitalOutput7StateTypeId, output);
            }
            output =  m_owfsInterface->getSwitchOutput(address, Owfs::SwitchChannel::PIO_H, &ok);
            if (ok) {
                thing->setStateValue(eightChannelSwitchDigitalOutput8StateTypeId, output);
            }
        }
        return info->finish(Thing::ThingErrorNoError);
    } else {
        return info->finish(Thing::ThingErrorThingNotFound);
    }
}

void IntegrationPluginOneWire::postSetupThing(Thing *thing)
{
    Q_UNUSED(thing);

    if(!m_pluginTimer) {
        m_pluginTimer = hardwareManager()->pluginTimerManager()->registerTimer(10);
        connect(m_pluginTimer, &PluginTimer::timeout, this, &IntegrationPluginOneWire::onPluginTimer);
    }
}

void IntegrationPluginOneWire::executeAction(ThingActionInfo *info)
{
    Thing *thing = info->thing();
    Action action = info->action();

    if (!m_owfsInterface) {
        //All current things with actions require an OWFS interface
        info->finish(Thing::ThingErrorHardwareNotAvailable, QT_TR_NOOP("OWFS interface is not available."));
    }

    if (thing->thingClassId() == oneWireInterfaceThingClassId) {
        return info->finish(Thing::ThingErrorActionTypeNotFound);

    } else if (thing->thingClassId() == singleChannelSwitchThingClassId) {
        if (action.actionTypeId() == singleChannelSwitchDigitalOutputActionTypeId){
            m_owfsInterface->setSwitchOutput(thing->paramValue(singleChannelSwitchThingAddressParamTypeId).toByteArray(), Owfs::SwitchChannel::PIO_A, action.param(singleChannelSwitchDigitalOutputActionDigitalOutputParamTypeId).value().toBool());

            return info->finish(Thing::ThingErrorNoError);
        } else {
            return info->finish(Thing::ThingErrorActionTypeNotFound);
        }
    } else if (thing->thingClassId() == dualChannelSwitchThingClassId) {
        if (action.actionTypeId() == dualChannelSwitchDigitalOutput1ActionTypeId){
            m_owfsInterface->setSwitchOutput(thing->paramValue(dualChannelSwitchThingAddressParamTypeId).toByteArray(), Owfs::SwitchChannel::PIO_A, action.param(dualChannelSwitchDigitalOutput1ActionDigitalOutput1ParamTypeId).value().toBool());
            return info->finish(Thing::ThingErrorNoError);
        } else if (action.actionTypeId() == dualChannelSwitchDigitalOutput2ActionTypeId){
            m_owfsInterface->setSwitchOutput(thing->paramValue(dualChannelSwitchThingAddressParamTypeId).toByteArray(), Owfs::SwitchChannel::PIO_B, action.param(dualChannelSwitchDigitalOutput2ActionDigitalOutput2ParamTypeId).value().toBool());
            return info->finish(Thing::ThingErrorNoError);
        } else {
            return info->finish(Thing::ThingErrorActionTypeNotFound);
        }
    } else if (thing->thingClassId() == eightChannelSwitchThingClassId) {
        if (action.actionTypeId() == eightChannelSwitchDigitalOutput1ActionTypeId){
            m_owfsInterface->setSwitchOutput(thing->paramValue(eightChannelSwitchThingAddressParamTypeId).toByteArray(), Owfs::SwitchChannel::PIO_A, action.param(eightChannelSwitchDigitalOutput1ActionDigitalOutput1ParamTypeId).value().toBool());
            return info->finish(Thing::ThingErrorNoError);
        } else if (action.actionTypeId() == eightChannelSwitchDigitalOutput2ActionTypeId){
            m_owfsInterface->setSwitchOutput(thing->paramValue(eightChannelSwitchThingAddressParamTypeId).toByteArray(), Owfs::SwitchChannel::PIO_B, action.param(eightChannelSwitchDigitalOutput2ActionDigitalOutput2ParamTypeId).value().toBool());
            return info->finish(Thing::ThingErrorNoError);
        } else if (action.actionTypeId() == eightChannelSwitchDigitalOutput3ActionTypeId){
            m_owfsInterface->setSwitchOutput(thing->paramValue(eightChannelSwitchThingAddressParamTypeId).toByteArray(), Owfs::SwitchChannel::PIO_C, action.param(eightChannelSwitchDigitalOutput3ActionDigitalOutput3ParamTypeId).value().toBool());
            return info->finish(Thing::ThingErrorNoError);
        } else if (action.actionTypeId() == eightChannelSwitchDigitalOutput4ActionTypeId){
            m_owfsInterface->setSwitchOutput(thing->paramValue(eightChannelSwitchThingAddressParamTypeId).toByteArray(), Owfs::SwitchChannel::PIO_D, action.param(eightChannelSwitchDigitalOutput4ActionDigitalOutput4ParamTypeId).value().toBool());
            return info->finish(Thing::ThingErrorNoError);
        } else if (action.actionTypeId() == eightChannelSwitchDigitalOutput5ActionTypeId){
            m_owfsInterface->setSwitchOutput(thing->paramValue(eightChannelSwitchThingAddressParamTypeId).toByteArray(), Owfs::SwitchChannel::PIO_E, action.param(eightChannelSwitchDigitalOutput5ActionDigitalOutput5ParamTypeId).value().toBool());
            return info->finish(Thing::ThingErrorNoError);
        } else if (action.actionTypeId() == eightChannelSwitchDigitalOutput6ActionTypeId){
            m_owfsInterface->setSwitchOutput(thing->paramValue(eightChannelSwitchThingAddressParamTypeId).toByteArray(), Owfs::SwitchChannel::PIO_F, action.param(eightChannelSwitchDigitalOutput6ActionDigitalOutput6ParamTypeId).value().toBool());
            return info->finish(Thing::ThingErrorNoError);
        } else if (action.actionTypeId() == eightChannelSwitchDigitalOutput7ActionTypeId){
            m_owfsInterface->setSwitchOutput(thing->paramValue(eightChannelSwitchThingAddressParamTypeId).toByteArray(), Owfs::SwitchChannel::PIO_G, action.param(eightChannelSwitchDigitalOutput7ActionDigitalOutput7ParamTypeId).value().toBool());
            return info->finish(Thing::ThingErrorNoError);
        } else if (action.actionTypeId() == eightChannelSwitchDigitalOutput8ActionTypeId){
            m_owfsInterface->setSwitchOutput(thing->paramValue(eightChannelSwitchThingAddressParamTypeId).toByteArray(), Owfs::SwitchChannel::PIO_H, action.param(eightChannelSwitchDigitalOutput8ActionDigitalOutput8ParamTypeId).value().toBool());
            return info->finish(Thing::ThingErrorNoError);
        } else {
            return info->finish(Thing::ThingErrorActionTypeNotFound);
        }
    } else {
        return info->finish(Thing::ThingErrorNoError);
    }
}


void IntegrationPluginOneWire::thingRemoved(Thing *thing)
{
    if (thing->thingClassId() == oneWireInterfaceThingClassId) {
        if (m_owfsInterface) {
            m_owfsInterface->deleteLater();
            m_owfsInterface = nullptr;
        }
    }

    if (myThings().filterByThingClassId(temperatureSensorThingClassId).isEmpty()) {
        if(m_w1Interface) {
            m_w1Interface->deleteLater();
            m_w1Interface = nullptr;
        }
    }

    if (myThings().empty()) {
        hardwareManager()->pluginTimerManager()->unregisterTimer(m_pluginTimer);
        m_pluginTimer = nullptr;
    }
}

void IntegrationPluginOneWire::setupOwfsTemperatureSensor(ThingSetupInfo *info)
{
    Thing *thing = info->thing();
    QByteArray address = thing->paramValue(temperatureSensorThingAddressParamTypeId).toByteArray();
    if (m_owfsInterface) {
        thing->setStateValue(temperatureSensorConnectedStateTypeId,  m_owfsInterface->isConnected(address));
        bool ok;
        double temp = m_owfsInterface->getTemperature(address, &ok);
        if (ok) {
            thing->setStateValue(temperatureSensorTemperatureStateTypeId, temp);
        }
        return info->finish(Thing::ThingErrorNoError);
    } else {
        qCWarning(dcOneWire()) << "OWFS interface is not available";
        return info->finish(Thing::ThingErrorHardwareNotAvailable);
    }
}

void IntegrationPluginOneWire::setupOwfsTemperatureHumiditySensor(ThingSetupInfo *info)
{
    Thing *thing = info->thing();
    QByteArray address = thing->paramValue(temperatureHumiditySensorThingAddressParamTypeId).toByteArray();
    if (m_owfsInterface) {
        thing->setStateValue(temperatureHumiditySensorConnectedStateTypeId,  m_owfsInterface->isConnected(address));
        bool ok;
        double temp = m_owfsInterface->getTemperature(address, &ok);
        if (ok) {
            thing->setStateValue(temperatureHumiditySensorTemperatureStateTypeId, temp);
        }
        double humidity = m_owfsInterface->getHumidity(address, &ok);
        if (ok) {
            thing->setStateValue(temperatureHumiditySensorHumidityStateTypeId, humidity);
        }
        return info->finish(Thing::ThingErrorNoError);
    } else {
        qCWarning(dcOneWire()) << "OWFS interface is not available";
        return info->finish(Thing::ThingErrorHardwareNotAvailable);
    }
}

void IntegrationPluginOneWire::onPluginTimer()
{
    foreach (Thing *thing, myThings()) {
        if (thing->thingClassId() == oneWireInterfaceThingClassId) {
            thing->setStateValue(oneWireInterfaceConnectedStateTypeId, m_owfsInterface->interfaceIsAvailable());

        } else if (thing->thingClassId() == temperatureSensorThingClassId) {
            QByteArray address = thing->paramValue(temperatureSensorThingAddressParamTypeId).toByteArray();
            bool ok = true;
            double temperature = 0;
            bool connected = false;

            if (!thing->parentId().isNull()) {
                if (m_owfsInterface) {
                    connected = m_owfsInterface->isConnected(address);
                    bool ok;
                    temperature = m_owfsInterface->getTemperature(address, &ok);
                } else {
                    qCWarning(dcOneWire()) << "onPlugInTimer: OWFS interface not setup for thing" << thing->name();
                }

            } else {
                temperature = m_w1Interface->getTemperature(address);
                connected = m_w1Interface->deviceAvailable(address);
            }

            if (ok) {
                thing->setStateValue(temperatureSensorTemperatureStateTypeId, temperature);
            }
            thing->setStateValue(temperatureSensorConnectedStateTypeId, connected);
        } else if (thing->thingClassId() == temperatureHumiditySensorThingClassId)  {
            if (!m_owfsInterface)
                continue;
            QByteArray address = thing->paramValue(temperatureHumiditySensorThingAddressParamTypeId).toByteArray();
            bool ok;
            double temp = m_owfsInterface->getTemperature(address, &ok);
            if (ok) {
                thing->setStateValue(temperatureHumiditySensorTemperatureStateTypeId, temp);
            }
            double humidity = m_owfsInterface->getHumidity(address, &ok);
            if (ok) {
                thing->setStateValue(temperatureHumiditySensorHumidityStateTypeId, humidity);
            }
            thing->setStateValue(temperatureHumiditySensorConnectedStateTypeId, m_owfsInterface->isConnected(address));
        } else if (thing->thingClassId() == singleChannelSwitchThingClassId) {
            if (!m_owfsInterface)
                continue;
            QByteArray address = thing->paramValue(singleChannelSwitchThingAddressParamTypeId).toByteArray();
            bool ok;
            bool output = m_owfsInterface->getSwitchOutput(address, Owfs::SwitchChannel::PIO_A, &ok);
            if (ok) {
                thing->setStateValue(singleChannelSwitchDigitalOutputStateTypeId, output);
            }
            thing->setStateValue(singleChannelSwitchConnectedStateTypeId, m_owfsInterface->isConnected(address));
        } else if (thing->thingClassId() == dualChannelSwitchThingClassId) {
            if (!m_owfsInterface)
                continue;
            QByteArray address = thing->paramValue(dualChannelSwitchThingAddressParamTypeId).toByteArray();
            bool ok;
            bool output = m_owfsInterface->getSwitchOutput(address, Owfs::SwitchChannel::PIO_A, &ok);
            if (ok) {
                thing->setStateValue(dualChannelSwitchDigitalOutput1StateTypeId, output);
            }
            output = m_owfsInterface->getSwitchOutput(address, Owfs::SwitchChannel::PIO_B, &ok);
            if (ok) {
                thing->setStateValue(dualChannelSwitchDigitalOutput2StateTypeId, output);
            }
            thing->setStateValue(dualChannelSwitchConnectedStateTypeId, m_owfsInterface->isConnected(address));
        } else if (thing->thingClassId() == eightChannelSwitchThingClassId) {
            if (!m_owfsInterface)
                continue;
            QByteArray address = thing->paramValue(eightChannelSwitchThingAddressParamTypeId).toByteArray();
            bool ok;
            bool output = m_owfsInterface->getSwitchOutput(address, Owfs::SwitchChannel::PIO_A, &ok);
            if (ok) {
                thing->setStateValue(eightChannelSwitchDigitalOutput1StateTypeId, output);
            }
            output = m_owfsInterface->getSwitchOutput(address, Owfs::SwitchChannel::PIO_B, &ok);
            if (ok) {
                thing->setStateValue(eightChannelSwitchDigitalOutput2StateTypeId, output);
            }
            output = m_owfsInterface->getSwitchOutput(address, Owfs::SwitchChannel::PIO_C, &ok);
            if (ok) {
                thing->setStateValue(eightChannelSwitchDigitalOutput3StateTypeId, output);
            }
            output = m_owfsInterface->getSwitchOutput(address, Owfs::SwitchChannel::PIO_D, &ok);
            if (ok) {
                thing->setStateValue(eightChannelSwitchDigitalOutput4StateTypeId, output);
            }
            output = m_owfsInterface->getSwitchOutput(address, Owfs::SwitchChannel::PIO_E, &ok);
            if (ok) {
                thing->setStateValue(eightChannelSwitchDigitalOutput5StateTypeId, output);
            }
            output = m_owfsInterface->getSwitchOutput(address, Owfs::SwitchChannel::PIO_F, &ok);
            if (ok) {
                thing->setStateValue(eightChannelSwitchDigitalOutput6StateTypeId, output);
            }
            output = m_owfsInterface->getSwitchOutput(address, Owfs::SwitchChannel::PIO_G, &ok);
            if (ok) {
                thing->setStateValue(eightChannelSwitchDigitalOutput7StateTypeId, output);
            }
            output = m_owfsInterface->getSwitchOutput(address, Owfs::SwitchChannel::PIO_H, &ok);
            if (ok) {
                thing->setStateValue(eightChannelSwitchDigitalOutput8StateTypeId, output);
            }
            thing->setStateValue(eightChannelSwitchConnectedStateTypeId, m_owfsInterface->isConnected(address));
        }
    }
}

void IntegrationPluginOneWire::onOneWireDevicesDiscovered(QList<Owfs::OwfsDevice> oneWireDevices)
{
    Thing *parentDevice =  myThings().filterByThingClassId(oneWireInterfaceThingClassId).first();

    ThingDescriptors descriptors;
    foreach (Owfs::OwfsDevice oneWireDevice, oneWireDevices){
        switch (oneWireDevice.family) {
        //https://github.com/owfs/owfs-doc/wiki/1Wire-Device-List
        case 0x10:  //DS18S20
        case 0x22:  //DS1822
        case 0x28:  //DS18B20
        case 0x3b:  {//DS1825, MAX31826, MAX31850
            ThingDescriptor descriptor(temperatureSensorThingClassId, oneWireDevice.type, "One wire temperature sensor", parentDevice->id());
            ParamList params;
            params.append(Param(temperatureSensorThingAddressParamTypeId, oneWireDevice.address));
            params.append(Param(temperatureSensorThingTypeParamTypeId, oneWireDevice.type));
            foreach (Thing *existingThing, myThings().filterByThingClassId(temperatureSensorThingClassId)){
                if (existingThing->paramValue(temperatureSensorThingAddressParamTypeId).toString() == oneWireDevice.address) {
                    descriptor.setThingId(existingThing->id());
                    break;
                }
            }
            descriptor.setParams(params);
            descriptors.append(descriptor);
            break;
        }
        case 0x26: {//DS2834
            ThingDescriptor descriptor(temperatureHumiditySensorThingClassId, oneWireDevice.type, "One wire temperature and humidity sensor", parentDevice->id());
            ParamList params;
            params.append(Param(temperatureHumiditySensorThingAddressParamTypeId, oneWireDevice.address));
            params.append(Param(temperatureHumiditySensorThingTypeParamTypeId, oneWireDevice.type));
            foreach (Thing *existingThing, myThings().filterByThingClassId(temperatureHumiditySensorThingClassId)){
                if (existingThing->paramValue(temperatureHumiditySensorThingAddressParamTypeId).toString() == oneWireDevice.address) {
                    descriptor.setThingId(existingThing->id());
                    break;
                }
            }
            descriptor.setParams(params);
            descriptors.append(descriptor);
            break;
        }
        case 0x05: { //single channel switch
            ThingDescriptor descriptor(singleChannelSwitchThingClassId, oneWireDevice.type, "One wire single channel switch", parentDevice->id());
            ParamList params;
            params.append(Param(singleChannelSwitchThingAddressParamTypeId, oneWireDevice.address));
            params.append(Param(singleChannelSwitchThingTypeParamTypeId, oneWireDevice.type));
            foreach (Thing *existingThing, myThings().filterByThingClassId(singleChannelSwitchThingClassId)){
                if (existingThing->paramValue(singleChannelSwitchThingAddressParamTypeId).toString() == oneWireDevice.address) {
                    descriptor.setThingId(existingThing->id());
                    break;
                }
            }
            descriptor.setParams(params);
            descriptors.append(descriptor);
            break;
        }
        case 0x12:
        case 0x3a: {//dual channel switch
            ThingDescriptor descriptor(dualChannelSwitchThingClassId, oneWireDevice.type, "One wire dual channel switch", parentDevice->id());
            ParamList params;
            params.append(Param(dualChannelSwitchThingAddressParamTypeId, oneWireDevice.address));
            params.append(Param(dualChannelSwitchThingTypeParamTypeId, oneWireDevice.type));
            foreach (Thing *existingThing, myThings().filterByThingClassId(dualChannelSwitchThingClassId)){
                if (existingThing->paramValue(dualChannelSwitchThingAddressParamTypeId).toString() == oneWireDevice.address) {
                    descriptor.setThingId(existingThing->id());
                    break;
                }
            }
            descriptor.setParams(params);
            descriptors.append(descriptor);
            break;
        }
        case 0x29: { //eight channel switch
            ThingDescriptor descriptor(eightChannelSwitchThingClassId, oneWireDevice.type, "One wire eight channel switch", parentDevice->id());
            ParamList params;
            params.append(Param(eightChannelSwitchThingAddressParamTypeId, oneWireDevice.address));
            params.append(Param(eightChannelSwitchThingTypeParamTypeId, oneWireDevice.type));
            foreach (Thing *existingThing, myThings().filterByThingClassId(eightChannelSwitchThingClassId)){
                if (existingThing->paramValue(eightChannelSwitchThingAddressParamTypeId).toString() == oneWireDevice.address) {
                    descriptor.setThingId(existingThing->id());
                    break;
                }
            }
            descriptor.setParams(params);
            descriptors.append(descriptor);
            break;
        }
        default:
            qDebug(dcOneWire()) << "Unknown Device discovered" << oneWireDevice.type << oneWireDevice.address;
            break;

        }
    }
    ThingDiscoveryInfo *info = m_runningDiscoveries.take(parentDevice);
    if (info && m_runningDiscoveries.isEmpty()) {
        info->addThingDescriptors(descriptors);
        info->finish(Thing::ThingErrorNoError);
    }
}
