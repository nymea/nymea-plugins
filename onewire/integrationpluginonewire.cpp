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
            deviceClassId == singleChannelSwitchThingClassId ||
            deviceClassId == dualChannelSwitchThingClassId   ||
            deviceClassId == eightChannelSwitchThingClassId) {

        if (myThings().filterByThingClassId(oneWireInterfaceThingClassId).isEmpty()) {
            //No one wire interface intitialized
            //: Error discovering one wire devices
            return info->finish(Thing::ThingErrorHardwareNotAvailable, QT_TR_NOOP("No one wire interface initialized. Please set up a one wire interface first."));
        }

        foreach(Thing *parentDevice, myThings().filterByThingClassId(oneWireInterfaceThingClassId)) {
            if (parentDevice->stateValue(oneWireInterfaceAutoAddStateTypeId).toBool()) {
                //devices cannot be discovered since auto mode is enabled
                return info->finish(Thing::ThingErrorNoError);
            } else {
                m_runningDiscoveries.insert(parentDevice, info);
                connect(info, &ThingDiscoveryInfo::destroyed, this, [this, parentDevice](){
                    m_runningDiscoveries.remove(parentDevice);
                });

                if (m_oneWireInterface)
                    m_oneWireInterface->discoverDevices();
            }
        }

        return;
    }

    qCWarning(dcOneWire()) << "Discovery called for a deviceclass which does not support discovery? Device class ID:" << info->thingClassId().toString();
    info->finish(Thing::ThingErrorThingClassNotFound);
}


void IntegrationPluginOneWire::setupThing(ThingSetupInfo *info)
{
    Thing *thing = info->thing();
    if (thing->thingClassId() == oneWireInterfaceThingClassId) {
        qCDebug(dcOneWire) << "Setup one wire interface";

        if (m_oneWireInterface) {
            qCWarning(dcOneWire) << "One wire interface already set up";
            //: Error setting up thing
            return info->finish(Thing::ThingErrorThingInUse, QT_TR_NOOP("There can only be one one wire interface per system."));
        }
        m_oneWireInterface = new OneWire(this);
        QByteArray initArguments = thing->paramValue(oneWireInterfaceThingInitArgsParamTypeId).toByteArray();

        if (!m_oneWireInterface->init(initArguments)){
            m_oneWireInterface->deleteLater();
            m_oneWireInterface = nullptr;
            //: Error setting up thing
            return info->finish(Thing::ThingErrorHardwareFailure, QT_TR_NOOP("Error initializing one wire interface."));
        }
        connect(m_oneWireInterface, &OneWire::devicesDiscovered, this, &IntegrationPluginOneWire::onOneWireDevicesDiscovered);
        return info->finish(Thing::ThingErrorNoError);
    }

    if (thing->thingClassId() == temperatureSensorThingClassId) {

        qCDebug(dcOneWire) << "Setup one wire temperature sensor" << thing->params();
        if (!m_oneWireInterface) { //in case the child was setup before the interface
            double temperature = m_oneWireInterface->getTemperature(thing->paramValue(temperatureSensorThingAddressParamTypeId).toByteArray());
            thing->setStateValue(temperatureSensorTemperatureStateTypeId, temperature);
        }
        return info->finish(Thing::ThingErrorNoError);
    }

    if (thing->thingClassId() == singleChannelSwitchThingClassId) {
        qCDebug(dcOneWire) << "Setup one wire switch" << thing->params();
        if (!m_oneWireInterface) {
            QByteArray address = thing->paramValue(singleChannelSwitchThingAddressParamTypeId).toByteArray();
            thing->setStateValue(singleChannelSwitchDigitalOutputStateTypeId, m_oneWireInterface->getSwitchOutput(address, OneWire::SwitchChannel::PIO_A));
        }
        return info->finish(Thing::ThingErrorNoError);
    }

    if (thing->thingClassId() == dualChannelSwitchThingClassId) {
        qCDebug(dcOneWire) << "Setup one wire dual switch" << thing->params();
        if (!m_oneWireInterface) {
            QByteArray address = thing->paramValue(dualChannelSwitchThingAddressParamTypeId).toByteArray();
            thing->setStateValue(dualChannelSwitchDigitalOutput1StateTypeId, m_oneWireInterface->getSwitchOutput(address, OneWire::SwitchChannel::PIO_A));
            thing->setStateValue(dualChannelSwitchDigitalOutput2StateTypeId, m_oneWireInterface->getSwitchOutput(address, OneWire::SwitchChannel::PIO_B));
        }
        return info->finish(Thing::ThingErrorNoError);
    }

    if (thing->thingClassId() == eightChannelSwitchThingClassId) {
        qCDebug(dcOneWire) << "Setup one wire eight channel switch" << thing->params();
        if (!m_oneWireInterface) {
            QByteArray address = thing->paramValue(eightChannelSwitchThingAddressParamTypeId).toByteArray();
            thing->setStateValue(eightChannelSwitchDigitalOutput1StateTypeId, m_oneWireInterface->getSwitchOutput(address, OneWire::SwitchChannel::PIO_A));
            thing->setStateValue(eightChannelSwitchDigitalOutput2StateTypeId, m_oneWireInterface->getSwitchOutput(address, OneWire::SwitchChannel::PIO_B));
            thing->setStateValue(eightChannelSwitchDigitalOutput3StateTypeId, m_oneWireInterface->getSwitchOutput(address, OneWire::SwitchChannel::PIO_C));
            thing->setStateValue(eightChannelSwitchDigitalOutput4StateTypeId, m_oneWireInterface->getSwitchOutput(address, OneWire::SwitchChannel::PIO_D));
            thing->setStateValue(eightChannelSwitchDigitalOutput5StateTypeId, m_oneWireInterface->getSwitchOutput(address, OneWire::SwitchChannel::PIO_E));
            thing->setStateValue(eightChannelSwitchDigitalOutput6StateTypeId, m_oneWireInterface->getSwitchOutput(address, OneWire::SwitchChannel::PIO_F));
            thing->setStateValue(eightChannelSwitchDigitalOutput7StateTypeId, m_oneWireInterface->getSwitchOutput(address, OneWire::SwitchChannel::PIO_G));
            thing->setStateValue(eightChannelSwitchDigitalOutput8StateTypeId, m_oneWireInterface->getSwitchOutput(address, OneWire::SwitchChannel::PIO_H));
        }
        return info->finish(Thing::ThingErrorNoError);
    }
    return info->finish(Thing::ThingErrorThingNotFound);
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

    if (thing->thingClassId() == oneWireInterfaceThingClassId) {
        if (action.actionTypeId() == oneWireInterfaceAutoAddActionTypeId){
            thing->setStateValue(oneWireInterfaceAutoAddStateTypeId, action.param(oneWireInterfaceAutoAddActionAutoAddParamTypeId).value());
            return info->finish(Thing::ThingErrorNoError);
        }
        return info->finish(Thing::ThingErrorActionTypeNotFound);
    }

    if (thing->thingClassId() == singleChannelSwitchThingClassId) {
        if (action.actionTypeId() == singleChannelSwitchDigitalOutputActionTypeId){
            m_oneWireInterface->setSwitchOutput(thing->paramValue(singleChannelSwitchThingAddressParamTypeId).toByteArray(), OneWire::SwitchChannel::PIO_A, action.param(singleChannelSwitchDigitalOutputActionDigitalOutputParamTypeId).value().toBool());

            return info->finish(Thing::ThingErrorNoError);
        }
        return info->finish(Thing::ThingErrorActionTypeNotFound);
    }

    if (thing->thingClassId() == dualChannelSwitchThingClassId) {
        if (action.actionTypeId() == dualChannelSwitchDigitalOutput1ActionTypeId){
            m_oneWireInterface->setSwitchOutput(thing->paramValue(dualChannelSwitchThingAddressParamTypeId).toByteArray(), OneWire::SwitchChannel::PIO_A, action.param(dualChannelSwitchDigitalOutput1ActionDigitalOutput1ParamTypeId).value().toBool());
            return info->finish(Thing::ThingErrorNoError);
        }
        if (action.actionTypeId() == dualChannelSwitchDigitalOutput2ActionTypeId){
            m_oneWireInterface->setSwitchOutput(thing->paramValue(dualChannelSwitchThingAddressParamTypeId).toByteArray(), OneWire::SwitchChannel::PIO_B, action.param(dualChannelSwitchDigitalOutput2ActionDigitalOutput2ParamTypeId).value().toBool());
            return info->finish(Thing::ThingErrorNoError);
        }
        return info->finish(Thing::ThingErrorActionTypeNotFound);
    }

    if (thing->thingClassId() == eightChannelSwitchThingClassId) {
        if (action.actionTypeId() == eightChannelSwitchDigitalOutput1ActionTypeId){
            m_oneWireInterface->setSwitchOutput(thing->paramValue(eightChannelSwitchThingAddressParamTypeId).toByteArray(), OneWire::SwitchChannel::PIO_A, action.param(eightChannelSwitchDigitalOutput1ActionDigitalOutput1ParamTypeId).value().toBool());
            return info->finish(Thing::ThingErrorNoError);
        }
        if (action.actionTypeId() == eightChannelSwitchDigitalOutput2ActionTypeId){
            m_oneWireInterface->setSwitchOutput(thing->paramValue(eightChannelSwitchThingAddressParamTypeId).toByteArray(), OneWire::SwitchChannel::PIO_B, action.param(eightChannelSwitchDigitalOutput2ActionDigitalOutput2ParamTypeId).value().toBool());
            return info->finish(Thing::ThingErrorNoError);
        }
        if (action.actionTypeId() == eightChannelSwitchDigitalOutput3ActionTypeId){
            m_oneWireInterface->setSwitchOutput(thing->paramValue(eightChannelSwitchThingAddressParamTypeId).toByteArray(), OneWire::SwitchChannel::PIO_C, action.param(eightChannelSwitchDigitalOutput3ActionDigitalOutput3ParamTypeId).value().toBool());
            return info->finish(Thing::ThingErrorNoError);
        }
        if (action.actionTypeId() == eightChannelSwitchDigitalOutput4ActionTypeId){
            m_oneWireInterface->setSwitchOutput(thing->paramValue(eightChannelSwitchThingAddressParamTypeId).toByteArray(), OneWire::SwitchChannel::PIO_D, action.param(eightChannelSwitchDigitalOutput4ActionDigitalOutput4ParamTypeId).value().toBool());
            return info->finish(Thing::ThingErrorNoError);
        }
        if (action.actionTypeId() == eightChannelSwitchDigitalOutput5ActionTypeId){
            m_oneWireInterface->setSwitchOutput(thing->paramValue(eightChannelSwitchThingAddressParamTypeId).toByteArray(), OneWire::SwitchChannel::PIO_E, action.param(eightChannelSwitchDigitalOutput5ActionDigitalOutput5ParamTypeId).value().toBool());
            return info->finish(Thing::ThingErrorNoError);
        }
        if (action.actionTypeId() == eightChannelSwitchDigitalOutput6ActionTypeId){
            m_oneWireInterface->setSwitchOutput(thing->paramValue(eightChannelSwitchThingAddressParamTypeId).toByteArray(), OneWire::SwitchChannel::PIO_F, action.param(eightChannelSwitchDigitalOutput6ActionDigitalOutput6ParamTypeId).value().toBool());
            return info->finish(Thing::ThingErrorNoError);
        }
        if (action.actionTypeId() == eightChannelSwitchDigitalOutput7ActionTypeId){
            m_oneWireInterface->setSwitchOutput(thing->paramValue(eightChannelSwitchThingAddressParamTypeId).toByteArray(), OneWire::SwitchChannel::PIO_G, action.param(eightChannelSwitchDigitalOutput7ActionDigitalOutput7ParamTypeId).value().toBool());
            return info->finish(Thing::ThingErrorNoError);
        }
        if (action.actionTypeId() == eightChannelSwitchDigitalOutput8ActionTypeId){
            m_oneWireInterface->setSwitchOutput(thing->paramValue(eightChannelSwitchThingAddressParamTypeId).toByteArray(), OneWire::SwitchChannel::PIO_H, action.param(eightChannelSwitchDigitalOutput8ActionDigitalOutput8ParamTypeId).value().toBool());
            return info->finish(Thing::ThingErrorNoError);
        }
        return info->finish(Thing::ThingErrorActionTypeNotFound);
    }
    return info->finish(Thing::ThingErrorNoError);
}


void IntegrationPluginOneWire::thingRemoved(Thing *thing)
{
    if (thing->thingClassId() == oneWireInterfaceThingClassId) {
        m_oneWireInterface->deleteLater();
        m_oneWireInterface = nullptr;
        return;
    }

    if (myThings().empty()) {
        hardwareManager()->pluginTimerManager()->unregisterTimer(m_pluginTimer);
        m_pluginTimer = nullptr;
    }
}


void IntegrationPluginOneWire::onPluginTimer()
{
    foreach (Thing *thing, myThings()) {
        if (thing->thingClassId() == oneWireInterfaceThingClassId) {
            thing->setStateValue(oneWireInterfaceConnectedStateTypeId, m_oneWireInterface->interfaceIsAvailable());

            if (thing->stateValue(oneWireInterfaceAutoAddStateTypeId).toBool()) {
                m_oneWireInterface->discoverDevices();
            }
        }

        if (thing->thingClassId() == temperatureSensorThingClassId) {
            QByteArray address = thing->paramValue(temperatureSensorThingAddressParamTypeId).toByteArray();

            double temperature = m_oneWireInterface->getTemperature(address);
            thing->setStateValue(temperatureSensorTemperatureStateTypeId, temperature);
        }

        if (thing->thingClassId() == singleChannelSwitchThingClassId) {
            QByteArray address = thing->paramValue(singleChannelSwitchThingAddressParamTypeId).toByteArray();
            thing->setStateValue(singleChannelSwitchDigitalOutputStateTypeId, m_oneWireInterface->getSwitchOutput(address, OneWire::SwitchChannel::PIO_A));
        }

        if (thing->thingClassId() == dualChannelSwitchThingClassId) {
            QByteArray address = thing->paramValue(dualChannelSwitchThingAddressParamTypeId).toByteArray();
            thing->setStateValue(dualChannelSwitchDigitalOutput1StateTypeId, m_oneWireInterface->getSwitchOutput(address, OneWire::SwitchChannel::PIO_A));
            thing->setStateValue(dualChannelSwitchDigitalOutput2StateTypeId, m_oneWireInterface->getSwitchOutput(address, OneWire::SwitchChannel::PIO_B));
        }

        if (thing->thingClassId() == eightChannelSwitchThingClassId) {
            QByteArray address = thing->paramValue(eightChannelSwitchThingAddressParamTypeId).toByteArray();
            thing->setStateValue(eightChannelSwitchDigitalOutput1StateTypeId, m_oneWireInterface->getSwitchOutput(address, OneWire::SwitchChannel::PIO_A));
            thing->setStateValue(eightChannelSwitchDigitalOutput2StateTypeId, m_oneWireInterface->getSwitchOutput(address, OneWire::SwitchChannel::PIO_B));
            thing->setStateValue(eightChannelSwitchDigitalOutput3StateTypeId, m_oneWireInterface->getSwitchOutput(address, OneWire::SwitchChannel::PIO_C));
            thing->setStateValue(eightChannelSwitchDigitalOutput4StateTypeId, m_oneWireInterface->getSwitchOutput(address, OneWire::SwitchChannel::PIO_D));
            thing->setStateValue(eightChannelSwitchDigitalOutput5StateTypeId, m_oneWireInterface->getSwitchOutput(address, OneWire::SwitchChannel::PIO_E));
            thing->setStateValue(eightChannelSwitchDigitalOutput6StateTypeId, m_oneWireInterface->getSwitchOutput(address, OneWire::SwitchChannel::PIO_F));
            thing->setStateValue(eightChannelSwitchDigitalOutput7StateTypeId, m_oneWireInterface->getSwitchOutput(address, OneWire::SwitchChannel::PIO_G));
            thing->setStateValue(eightChannelSwitchDigitalOutput8StateTypeId, m_oneWireInterface->getSwitchOutput(address, OneWire::SwitchChannel::PIO_H));
        }
    }
}

void IntegrationPluginOneWire::onOneWireDevicesDiscovered(QList<OneWire::OneWireDevice> oneWireDevices)
{
    foreach(Thing *parentDevice, myThings().filterByThingClassId(oneWireInterfaceThingClassId)) {

        bool autoDiscoverEnabled = parentDevice->stateValue(oneWireInterfaceAutoAddStateTypeId).toBool();
        ThingDescriptors descriptors;
        foreach (OneWire::OneWireDevice oneWireDevice, oneWireDevices){
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
        if (autoDiscoverEnabled) {
            emit autoThingsAppeared(descriptors);
        } else {
            ThingDiscoveryInfo *info = m_runningDiscoveries.take(parentDevice);
            if (info && m_runningDiscoveries.isEmpty()) {
                info->addThingDescriptors(descriptors);
                info->finish(Thing::ThingErrorNoError);
            }
        }
        break;
    }
}
