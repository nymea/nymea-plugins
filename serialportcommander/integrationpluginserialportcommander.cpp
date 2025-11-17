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

#include "integrationpluginserialportcommander.h"
#include "plugininfo.h"

IntegrationPluginSerialPortCommander::IntegrationPluginSerialPortCommander()
{
}


void IntegrationPluginSerialPortCommander::discoverThings(ThingDiscoveryInfo *info)
{
    // Create the list of available serial interfaces

    foreach(QSerialPortInfo port, QSerialPortInfo::availablePorts()) {

        qCDebug(dcSerialPortCommander()) << "Found serial port:" << port.portName();
        QString description = port.manufacturer() + " " + port.description();
        ThingDescriptor thingDescriptor(info->thingClassId(), port.portName(), description);
        ParamList parameters;
        foreach (Thing *existingThing, myThings()) {
            if (existingThing->paramValue(serialPortCommanderThingSerialPortParamTypeId).toString() == port.portName()) {
                thingDescriptor.setThingId(existingThing->id());
                break;
            }
        }
        parameters.append(Param(serialPortCommanderThingSerialPortParamTypeId, port.portName()));
        thingDescriptor.setParams(parameters);
        info->addThingDescriptor(thingDescriptor);
    }

    info->finish(Thing::ThingErrorNoError);
}


void IntegrationPluginSerialPortCommander::setupThing(ThingSetupInfo *info)
{
    Thing *thing = info->thing();

    if(!m_reconnectTimer) {
        m_reconnectTimer = new QTimer(this);
        m_reconnectTimer->setSingleShot(true);
        m_reconnectTimer->setInterval(5000);

        connect(m_reconnectTimer, &QTimer::timeout, this, &IntegrationPluginSerialPortCommander::onReconnectTimer);
    }

    if (thing->thingClassId() == serialPortCommanderThingClassId) {
        QString interface = thing->paramValue(serialPortCommanderThingSerialPortParamTypeId).toString();
        QSerialPort *serialPort = new QSerialPort(interface, this);

        serialPort->setBaudRate(thing->paramValue(serialPortCommanderThingBaudRateParamTypeId).toInt());
        serialPort->setDataBits(QSerialPort::DataBits(thing->paramValue(serialPortCommanderThingDataBitsParamTypeId).toInt()));

        if (thing->paramValue(serialPortCommanderThingParityParamTypeId).toString().contains("Even")) {
            serialPort->setParity(QSerialPort::Parity::EvenParity );
        } else if (thing->paramValue(serialPortCommanderThingParityParamTypeId).toString().contains("Odd")) {
            serialPort->setParity(QSerialPort::Parity::OddParity );
        } else if (thing->paramValue(serialPortCommanderThingParityParamTypeId).toString().contains("Space")) {
            serialPort->setParity(QSerialPort::Parity::SpaceParity );
        } else if (thing->paramValue(serialPortCommanderThingParityParamTypeId).toString().contains("Mark")) {
            serialPort->setParity(QSerialPort::Parity::MarkParity );
        } else {
            serialPort->setParity(QSerialPort::Parity::NoParity);
        }

        serialPort->setStopBits(QSerialPort::StopBits(thing->paramValue(serialPortCommanderThingStopBitsParamTypeId).toInt()));

        if (thing->paramValue(serialPortCommanderThingFlowControlParamTypeId).toString().contains("Hardware")) {
            serialPort->setFlowControl(QSerialPort::FlowControl::HardwareControl);
        } else if (thing->paramValue(serialPortCommanderThingFlowControlParamTypeId).toString().contains("Software")) {
            serialPort->setFlowControl(QSerialPort::FlowControl::SoftwareControl);
        } else {
            serialPort->setFlowControl(QSerialPort::FlowControl::NoFlowControl);
        }

        if (!serialPort->open(QIODevice::ReadWrite)) {
            qCWarning(dcSerialPortCommander()) << "Could not open serial port" << interface << serialPort->errorString();
            serialPort->deleteLater();
            //: Error setting up thing
            return info->finish(Thing::ThingErrorHardwareFailure, QT_TR_NOOP("Could not open serial port."));
        }

#if QT_VERSION >= QT_VERSION_CHECK(5, 8, 0)
        connect(serialPort, &QSerialPort::errorOccurred, this, &IntegrationPluginSerialPortCommander::onSerialError);
#else
        connect(serialPort, SIGNAL(error(QSerialPort::SerialPortError)), this, SLOT(onSerialError(QSerialPort::SerialPortError)));
#endif
        connect(serialPort, &QSerialPort::readyRead, this, &IntegrationPluginSerialPortCommander::onReadyRead);
        connect(serialPort, &QSerialPort::baudRateChanged, this, &IntegrationPluginSerialPortCommander::onBaudRateChanged);
        connect(serialPort, &QSerialPort::parityChanged, this, &IntegrationPluginSerialPortCommander::onParityChanged);
        connect(serialPort, &QSerialPort::dataBitsChanged, this, &IntegrationPluginSerialPortCommander::onDataBitsChanged);
        connect(serialPort, &QSerialPort::stopBitsChanged, this, &IntegrationPluginSerialPortCommander::onStopBitsChanged);
        connect(serialPort, &QSerialPort::flowControlChanged, this, &IntegrationPluginSerialPortCommander::onFlowControlChanged);


        m_serialPorts.insert(thing, serialPort);
        thing->setStateValue(serialPortCommanderConnectedStateTypeId, true);
    }
    return info->finish(Thing::ThingErrorNoError);
}


void IntegrationPluginSerialPortCommander::executeAction(ThingActionInfo *info)
{
    Thing *thing = info->thing();
    Action action = info->action();


    if (action.actionTypeId() == serialPortCommanderTriggerActionTypeId) {

        QSerialPort *serialPort = m_serialPorts.value(thing);
        qint64 size = serialPort->write(action.param(serialPortCommanderTriggerActionOutputDataParamTypeId).value().toByteArray());
        if(size != action.param(serialPortCommanderTriggerActionOutputDataParamTypeId).value().toByteArray().length()) {
            return info->finish(Thing::ThingErrorHardwareFailure, QT_TR_NOOP("Error writing to serial port."));
        }
        return info->finish(Thing::ThingErrorNoError);
    }
    info->finish(Thing::ThingErrorActionTypeNotFound);
}


void IntegrationPluginSerialPortCommander::thingRemoved(Thing *thing)
{
    if (thing->thingClassId() == serialPortCommanderThingClassId) {

        QSerialPort *serialPort = m_serialPorts.take(thing);
        if (serialPort) {
            if (serialPort->isOpen()){
                serialPort->flush();
                serialPort->close();
            }
            serialPort->deleteLater();
        }
    }

    if (myThings().empty()) {
        m_reconnectTimer->stop();
        m_reconnectTimer->deleteLater();
    }
}

void IntegrationPluginSerialPortCommander::onReadyRead()
{
    QSerialPort *serialPort =  static_cast<QSerialPort*>(sender());
    Thing *thing = m_serialPorts.key(serialPort);

    QByteArray data;
    while (!serialPort->atEnd()) {
        data.append(serialPort->read(100));
    }
    qCDebug(dcSerialPortCommander()) << "Message received" << data;

    Event event(serialPortCommanderTriggeredEventTypeId, thing->id());
    ParamList parameters;
    parameters.append(Param(serialPortCommanderTriggeredEventInputDataParamTypeId, data));
    event.setParams(parameters);
    emit emitEvent(event);
}

void IntegrationPluginSerialPortCommander::onSerialError(QSerialPort::SerialPortError error)
{
    QSerialPort *serialPort =  static_cast<QSerialPort*>(sender());
    Thing *thing = m_serialPorts.key(serialPort);

    if (error != QSerialPort::NoError && serialPort->isOpen()) {
        qCCritical(dcSerialPortCommander()) << "Serial port error:" << error << serialPort->errorString();
        m_reconnectTimer->start();
        serialPort->close();
        thing->setStateValue(serialPortCommanderConnectedStateTypeId, false);
    }
}

void IntegrationPluginSerialPortCommander::onBaudRateChanged(qint32 baudRate, QSerialPort::Directions direction)
{
    Q_UNUSED(direction)
    QSerialPort *serialPort =  static_cast<QSerialPort*>(sender());
    Thing *thing = m_serialPorts.key(serialPort);
    thing->setParamValue(serialPortCommanderThingBaudRateParamTypeId, baudRate);
}

void IntegrationPluginSerialPortCommander::onParityChanged(QSerialPort::Parity parity)
{
    QSerialPort *serialPort =  static_cast<QSerialPort*>(sender());
    Thing *thing = m_serialPorts.key(serialPort);
    thing->setParamValue(serialPortCommanderThingParityParamTypeId, parity);
}

void IntegrationPluginSerialPortCommander::onDataBitsChanged(QSerialPort::DataBits dataBits)
{
    QSerialPort *serialPort =  static_cast<QSerialPort*>(sender());
    Thing *thing = m_serialPorts.key(serialPort);
    thing->setParamValue(serialPortCommanderThingDataBitsParamTypeId, dataBits);
}

void IntegrationPluginSerialPortCommander::onStopBitsChanged(QSerialPort::StopBits stopBits)
{
    QSerialPort *serialPort =  static_cast<QSerialPort*>(sender());
    Thing *thing = m_serialPorts.key(serialPort);
    thing->setParamValue(serialPortCommanderThingStopBitsParamTypeId, stopBits);
}

void IntegrationPluginSerialPortCommander::onFlowControlChanged(QSerialPort::FlowControl flowControl)
{
    QSerialPort *serialPort =  static_cast<QSerialPort*>(sender());
    Thing *thing = m_serialPorts.key(serialPort);
    thing->setParamValue(serialPortCommanderThingFlowControlParamTypeId, flowControl);
}

void IntegrationPluginSerialPortCommander::onReconnectTimer()
{
    foreach(Thing *thing, myThings()) {
        if (!thing->stateValue(serialPortCommanderConnectedStateTypeId).toBool()) {
            QSerialPort *serialPort =  m_serialPorts.value(thing);
            if (serialPort) {
                if (serialPort->open(QSerialPort::ReadWrite)) {
                    thing->setStateValue(serialPortCommanderConnectedStateTypeId, true);
                } else {
                    thing->setStateValue(serialPortCommanderConnectedStateTypeId, false);
                    m_reconnectTimer->start();
                }
            }
        }
    }
}

