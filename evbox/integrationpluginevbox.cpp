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


#include "integrationpluginevbox.h"
#include "plugininfo.h"
#include "plugintimer.h"
#include "evboxport.h"

#include <QSerialPortInfo>
#include <QSerialPort>
#include <QDataStream>

#define STX 0x02
#define ETX 0x03

IntegrationPluginEVBox::IntegrationPluginEVBox()
{

}

IntegrationPluginEVBox::~IntegrationPluginEVBox()
{
}

void IntegrationPluginEVBox::discoverThings(ThingDiscoveryInfo *info)
{
    if (QSerialPortInfo::availablePorts().isEmpty()) {
        info->finish(Thing::ThingErrorHardwareNotAvailable, QT_TR_NOOP("No serial ports are available on this system. Please connect a RS485 adapter first."));
        return;
    }

    int discoveryCount = 0;

    foreach(const QSerialPortInfo &portInfo, QSerialPortInfo::availablePorts()) {
        // Reusing existing ports as multiple boxes could be connected to a single adapter
        EVBoxPort *port = m_ports.value(portInfo.portName());
        if (!port) {
            // But if we don't have one yet, create it just for the discovery and delete it when the discovery info ends
            port = new EVBoxPort(portInfo.portName(), info);
            if (!port->open()) {
                qCWarning(dcEVBox()) << "Unable to open serial port" << portInfo.portName() << "for discovery.";
                delete port;
                continue;
            }
            qCInfo(dcEVBox()) << "Serial port" << portInfo.portName() << "opened for discovery.";
        } else {
            qCDebug(dcEVBox()) << "Discovering on already open serial port:" << portInfo.portName();
        }

        discoveryCount++;
        port->sendCommand(EVBoxPort::Command68, 10, 1);

        connect(port, &EVBoxPort::responseReceived, info, [=](EVBoxPort::Command command, const QString &serial){
            if (command != EVBoxPort::Command68) {
                return;
            }

            ThingDescriptor thingDescriptor(info->thingClassId(), thingClass(info->thingClassId()).displayName(), serial);
            ParamTypeId serialPortParamTypeId = thingClass(info->thingClassId()).paramTypes().findByName("serialPort").id();
            ParamTypeId serialNumberParamTypeId = thingClass(info->thingClassId()).paramTypes().findByName("serialNumber").id();
            ParamList params{
                {serialPortParamTypeId, portInfo.portName()},
                {serialNumberParamTypeId, serial}
            };
            thingDescriptor.setParams(params);
            Thing *existingThing = myThings().findByParams(params);
            if (existingThing) {
                thingDescriptor.setThingId(existingThing->id());
            }

            if (!info->property("foundSerials").toStringList().contains(serial)) {
                qCInfo(dcEVBox()) << "Adding descriptor for port" << portInfo.portName() << "Serial:" << serial << "Existing:" << (existingThing != nullptr ? "yes" : "no");
                info->addThingDescriptor(thingDescriptor);
                info->setProperty("foundSerials", QStringList{serial} + info->property("foundSerials").toStringList());
            } else {
                qCInfo(dcEVBox()) << "Discarding duplicate descriptor for port" << portInfo.portName() << "Serial:" << serial;
            }
        });
    }

    if (discoveryCount == 0) {
        info->finish(Thing::ThingErrorHardwareFailure, QT_TR_NOOP("Unable to open the RS485 port. Please make sure the RS485 adapter is connected properly and not in use by anything else."));
        return;
    }

    QTimer::singleShot(3000, info, [info](){
        info->finish(Thing::ThingErrorNoError);
    });
}

void IntegrationPluginEVBox::setupThing(ThingSetupInfo *info)
{
    Thing *thing = info->thing();
    QString portName = thing->paramValue("serialPort").toString();
    QString serialNumber = thing->paramValue("serialNumber").toString();

    // Opening the port, sharing with others if already opened.
    EVBoxPort *port = m_ports.value(portName);
    if (!port) {
        qCInfo(dcEVBox()) << "Port" << portName << "not open yet. Opening.";
        port = new EVBoxPort(portName, this);
        if (!port->open()) {
            qCWarning(dcEVBox()) << "Unable to open port" << portName << "for EVBox" << serialNumber;
            delete port;
            info->finish(Thing::ThingErrorHardwareFailure, QT_TR_NOOP("Unable to open the RS485 port. Please make sure the RS485 adapter is connected properly."));
            return;
        }
        m_ports.insert(portName, port);
    }



    // Setup routine: Try to set the max charging current to 6A and see if we get a valid answer
    port->sendCommand(EVBoxPort::Command68, 60, 6, serialNumber);
    connect(port, &EVBoxPort::closed, info, [info](){
        info->finish(Thing::ThingErrorHardwareFailure, QT_TR_NOOP("The EVBox has closed the connection."));
    });
    connect(port, &EVBoxPort::responseReceived, info, [info, serialNumber](EVBoxPort::Command /*command*/, const QString &serial){
        if (serial == serialNumber) {
            info->finish(Thing::ThingErrorNoError);
        }
    });
    QTimer::singleShot(5000, info, [info](){
        info->finish(Thing::ThingErrorTimeout, QT_TR_NOOP("The EVBox is not responding."));
    });


    // And connect the signals to the thing for when the setup went well
    connect(port, &EVBoxPort::closed, thing, [thing, portName](){
        qCInfo(dcEVBox()) << "Port" << portName << "closed. Marking thing as offline:" << thing->name();
        thing->setStateValue("connected", false);
    });
    connect(port, &EVBoxPort::opened, thing, [portName](){
        qCInfo(dcEVBox()) << "Port" << portName << "opened.";
    });

    connect(port, &EVBoxPort::shortResponseReceived, thing, [this, thing, serialNumber](EVBoxPort::Command /*command*/, const QString &serial){
        if (serial != serialNumber) {
            return;
        }
        thing->setStateValue("connected", true);
        finishPendingAction(thing);
        m_waitingForResponses[thing] = false;
    });

    connect(port, &EVBoxPort::responseReceived, thing, [this, thing, serialNumber](EVBoxPort::Command /*command*/, const QString &serial, quint16 minChargingCurrent, quint16 maxChargingCurrent, quint16 chargingCurrentL1, quint16 chargingCurrentL2, quint16 chargingCurrentL3, quint32 totalEnergyConsumed){
        if (serial != serialNumber) {
            return;
        }
        thing->setStateValue("connected", true);
        finishPendingAction(thing);
        m_waitingForResponses[thing] = false;

        thing->setStateMinMaxValues("maxChargingCurrent", minChargingCurrent / 10, maxChargingCurrent / 10);

        if (thing->thingClassId() == elviMidThingClassId) {
            double currentPower = (chargingCurrentL1 + chargingCurrentL2 + chargingCurrentL3) * 23;
            thing->setStateValue("currentPower", currentPower);
            thing->setStateValue("totalEnergyConsumed", totalEnergyConsumed / 1000.0);
            thing->setStateValue("charging", currentPower > 0);
            int phaseCount = 0;
            if (chargingCurrentL1 > 0) {
                phaseCount++;
            }
            if (chargingCurrentL2 > 0) {
                phaseCount++;
            }
            if (chargingCurrentL3 > 0) {
                phaseCount++;
            }
            // If all phases are on 0, we aren't charging and don't know how many phases are available...
            // Only updating the count if we actually do know that at least one is charging.
            if (phaseCount > 0) {
                thing->setStateValue("phaseCount", phaseCount);
            }
        } else {
            thing->setStateValue("charging", thing->stateValue("maxChargingCurrent").toUInt() > 0 && thing->stateValue("power").toBool());
            thing->setStateValue("phaseCount", thing->setting("phaseCount").toUInt());
        }

    });
}

void IntegrationPluginEVBox::postSetupThing(Thing */*thing*/)
{
    if (!m_timer) {
        m_timer = hardwareManager()->pluginTimerManager()->registerTimer(5);
        connect(m_timer, &PluginTimer::timeout, this, [this](){
            foreach (Thing *t, myThings()) {
                QString portName = t->paramValue("serialPort").toString();
                QString serial = t->paramValue("serialNumber").toString();

                if (m_waitingForResponses.value(t)) {
                    qCInfo(dcEVBox()) << "Wallbox" << t->name() << "did not respond to last command. Marking offline.";
                    t->setStateValue("connected", false);
                }

                EVBoxPort *port = m_ports.value(portName);
                if (port->isOpen()) {
                    quint16 maxChargingCurrent = 0;
                    if (t->stateValue("power").toBool()) {
                        maxChargingCurrent = t->stateValue("maxChargingCurrent").toUInt();
                    }
                    port->sendCommand(EVBoxPort::Command68, 60, maxChargingCurrent, serial);
                    m_waitingForResponses[t] = true;
                }
            }
        });
    }
}

void IntegrationPluginEVBox::thingRemoved(Thing *thing)
{
    QString portName = thing->paramValue("serialPort").toString();
    ParamTypeId serialPortParamTypeId = thing->thingClass().paramTypes().findByName("serialPort").id();
    if (myThings().filterByParam(serialPortParamTypeId, portName).isEmpty()) {
        qCInfo(dcEVBox()).nospace() << "No more EVBox devices using port " << portName << ". Destroying port.";
        delete m_ports.take(portName);
    }

    if (myThings().isEmpty()) {
        hardwareManager()->pluginTimerManager()->unregisterTimer(m_timer);
        m_timer = nullptr;
    }
}

void IntegrationPluginEVBox::executeAction(ThingActionInfo *info)
{
    Thing *thing = info->thing();

    QString portName = thing->paramValue("serialPort").toString();
    QString serial = thing->paramValue("serialNumber").toString();
    EVBoxPort *port = m_ports.value(portName);

    qCDebug(dcEVBox()) << "Executing action" << info->action().actionTypeId().toString();
    ActionType actionType = thing->thingClass().actionTypes().findById(info->action().actionTypeId());
    if (actionType.name() == "power") {
        bool power = info->action().paramValue(actionType.id()).toBool();
        quint16 maxChargingCurrent = thing->stateValue("maxChargingCurrent").toUInt();
        port->sendCommand(EVBoxPort::Command68, 60, power ? maxChargingCurrent : 0, serial);
    } else if (actionType.name() == "maxChargingCurrent") {
        int maxChargingCurrent = info->action().paramValue(actionType.id()).toInt();
        port->sendCommand(EVBoxPort::Command68, 60, maxChargingCurrent, serial);
    }

    m_pendingActions[thing].append(info);
    connect(info, &ThingActionInfo::aborted, this, [=](){
        m_pendingActions[thing].removeAll(info);
    });

}

void IntegrationPluginEVBox::finishPendingAction(Thing *thing)
{
    if (!m_pendingActions.value(thing).isEmpty()) {
        ThingActionInfo *info = m_pendingActions[thing].takeFirst();
        qCDebug(dcEVBox()) << "Finishing action:" << info->action().actionTypeId().toString();
        ActionType actionType = thing->thingClass().actionTypes().findById(info->action().actionTypeId());
        if (actionType.name() == "power") {
            thing->setStateValue("power", info->action().paramValue(actionType.id()));
        } else if (actionType.name() == "maxChargingCurrent") {
            thing->setStateValue("maxChargingCurrent", info->action().paramValue(actionType.id()));
        }
        info->finish(Thing::ThingErrorNoError);
    }
}

