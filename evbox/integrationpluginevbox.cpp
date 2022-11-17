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
    // Create the list of available serial interfaces

    foreach(QSerialPortInfo port, QSerialPortInfo::availablePorts()) {

        qCDebug(dcEVBox()) << "Found serial port:" << port.portName();
        QString description = port.portName() + " " + port.manufacturer() + " " + port.description();
        ThingDescriptor thingDescriptor(info->thingClassId(), "EVBox Elvi", description);
        ParamList parameters;
        foreach (Thing *existingThing, myThings()) {
            if (existingThing->paramValue(evboxThingSerialPortParamTypeId).toString() == port.portName()) {
                thingDescriptor.setThingId(existingThing->id());
                break;
            }
        }
        parameters.append(Param(evboxThingSerialPortParamTypeId, port.portName()));
        thingDescriptor.setParams(parameters);
        info->addThingDescriptor(thingDescriptor);
    }

    info->finish(Thing::ThingErrorNoError);
}

void IntegrationPluginEVBox::setupThing(ThingSetupInfo *info)
{
    Thing *thing = info->thing();
    QString interface = thing->paramValue(evboxThingSerialPortParamTypeId).toString();
    QSerialPort *serialPort = new QSerialPort(interface, info->thing());

    serialPort->setBaudRate(QSerialPort::Baud38400);
    serialPort->setDataBits(QSerialPort::Data8);
    serialPort->setStopBits(QSerialPort::OneStop);
    serialPort->setParity(QSerialPort::NoParity);

    connect(serialPort, &QSerialPort::readyRead, thing, [=]() {
        thing->setStateValue(evboxConnectedStateTypeId, true);
        QByteArray data = serialPort->readAll();
//        qCDebug(dcEVBox()) << "Data received from serial port:" << data;
        m_inputBuffers[thing].append(data);
        processInputBuffer(thing);
    });

    connect(serialPort, static_cast<void(QSerialPort::*)(QSerialPort::SerialPortError)>(&QSerialPort::error), thing, [=](){
        qCWarning(dcEVBox()) << "Serial Port error" << serialPort->error() << serialPort->errorString();
        if (serialPort->error() != QSerialPort::NoError) {
            if (serialPort->isOpen()) {
                serialPort->close();
            }
            thing->setStateValue(evboxConnectedStateTypeId, false);
            QTimer::singleShot(1000, this, [=](){
                serialPort->open(QSerialPort::ReadWrite);
            });
        }
    });

    if (!serialPort->open(QSerialPort::ReadWrite)) {
        qCWarning(dcEVBox()) << "Unable to open serial port";
        info->finish(Thing::ThingErrorHardwareFailure, QT_TR_NOOP("Unable to open the RS485 port. Please make sure the RS485 adapter is connected properly."));
        return;
    }

    m_serialPorts.insert(thing, serialPort);

    m_pendingSetups.insert(thing, info);
    connect(info, &ThingSetupInfo::finished, this, [=](){
        m_pendingSetups.remove(thing);
    });
    QTimer::singleShot(2000, info, [=](){
        qCDebug(dcEVBox()) << "Timeout during setup";
        delete m_serialPorts.take(info->thing());
        info->finish(Thing::ThingErrorHardwareNotAvailable, QT_TR_NOOP("The EVBox is not responding."));
    });


    sendCommand(thing, Command69, 1);
}

void IntegrationPluginEVBox::thingRemoved(Thing *thing)
{
    m_timers.remove(thing);
    delete m_serialPorts.take(thing);
}

void IntegrationPluginEVBox::executeAction(ThingActionInfo *info)
{
    Thing *thing = info->thing();

    if (info->action().actionTypeId() == evboxPowerActionTypeId) {
        bool power = info->action().paramValue(evboxPowerActionPowerParamTypeId).toBool();
        sendCommand(info->thing(), Command69, power ? info->thing()->stateValue(evboxMaxChargingCurrentStateTypeId).toUInt() : 0);
    } else if (info->action().actionTypeId() == evboxMaxChargingCurrentActionTypeId) {
        int maxChargingCurrent = info->action().paramValue(evboxMaxChargingCurrentActionMaxChargingCurrentParamTypeId).toInt();
        sendCommand(info->thing(), Command69, maxChargingCurrent);
    }

    m_pendingActions[thing].append(info);
    connect(info, &ThingActionInfo::finished, this, [=](){
        m_pendingActions[thing].removeAll(info);
    });

}

bool IntegrationPluginEVBox::sendCommand(Thing *thing, Command command, quint16 maxChargingCurrent)
{
    QByteArray commandData;

    commandData += "80"; // Dst addr
    commandData += "A0"; // Sender address
    commandData += QString::number(command);
    commandData += QString("%1").arg(maxChargingCurrent * 10, 4, 10, QChar('0'));
    commandData += QString("%1").arg(maxChargingCurrent * 10, 4, 10, QChar('0'));
    commandData += QString("%1").arg(maxChargingCurrent * 10, 4, 10, QChar('0'));
    commandData += "003C"; // Timeout (60 sec)
    // If we fail to refresh the wallbox after the timeout, it shall turn off, which is what we'll use as default
    // when we don't know what its set to (as we can't read it).
    // Hence we do *not* cache the power and maxChargingCurrent states for this one
    commandData += QString("%1").arg(0, 4, 10, QChar('0'));
    commandData += QString("%1").arg(0, 4, 10, QChar('0'));
    commandData += QString("%1").arg(0, 4, 10, QChar('0'));

    commandData += createChecksum(commandData);

    QByteArray data;
    QDataStream stream(&data, QIODevice::WriteOnly);
    stream << static_cast<quint8>(STX);
    stream.writeRawData(commandData.data(), commandData.length());
    stream << static_cast<quint8>(ETX);

    qCDebug(dcEVBox()) << "Writing data:" << data << "->" << data.toHex();
    QSerialPort *serialPort = m_serialPorts.value(thing);
    qint64 count = serialPort->write(data);
    if (count == data.length()) {
        m_waitingForResponses[thing] = true;
    }
    return count == data.length();
}

QByteArray IntegrationPluginEVBox::createChecksum(const QByteArray &data) const
{
    QDataStream checksumStream(data);
    quint8 sum = 0;
    quint8 xOr = 0;
    while (!checksumStream.atEnd()) {
        quint8 byte;
        checksumStream >> byte;
        sum += byte;
        xOr ^= byte;
    }
    return QString("%1%2").arg(sum,2,16, QChar('0')).arg(xOr,2,16, QChar('0')).toUpper().toLocal8Bit();
}

void IntegrationPluginEVBox::processInputBuffer(Thing *thing)
{
    QByteArray packet;
    QDataStream inputStream(m_inputBuffers.value(thing));
    QDataStream outputStream(&packet, QIODevice::WriteOnly);
    bool startFound = false, endFound = false;

    while (!inputStream.atEnd()) {
        quint8 byte;
        inputStream >> byte;
        if (!startFound) {
            if (byte == STX) {
                startFound = true;
                continue;
            } else {
                qCWarning(dcEVBox()) << "Discarding byte not matching start of frame 0x" + QString::number(byte, 16);
                continue;
            }
        }

        if (byte == ETX) {
            endFound = true;
            break;
        }

        outputStream << byte;
    }

    if (startFound && endFound) {
        m_inputBuffers[thing].remove(0, packet.length() + 2);
    } else {
//        qCDebug(dcEVBox()) << "Data is incomplete... Waiting for more...";
        return;
    }

    if (packet.length() < 2) { // In practice it'll be longer, but let's make sure we won't crash checking the checksum on erraneous data
        qCWarning(dcEVBox()) << "Packet is too short. Discarding packet...";
        return;
    }

    qCDebug(dcEVBox()) << "Packet received:" << packet;

    QByteArray checksum = createChecksum(packet.left(packet.length() - 4));
    if (checksum != packet.right(4)) {
        qCWarning(dcEVBox()) << "Checksum mismatch for incoming packet:" << packet << "Given checksum:" << packet.right(4) << "Expected:" << checksum;
        return;
    }

    // We received something valid... Assuming the last command we've sent is OK.
    // There's no way to properly match a response to a command, so...
    if (m_pendingSetups.contains(thing)) {
        qCDebug(dcEVBox()) << "Finishing setup";

        // Can't use a pluginTimer because it may collide with data on the wire, so we're
        // manually re-starting the timer whenever we receive something.
        QTimer *timer = new QTimer(thing);
        m_timers.insert(thing, timer);
        timer->setInterval(1000);

        connect(timer, &QTimer::timeout, thing, [=](){
            thing->setStateValue(evboxConnectedStateTypeId, !m_waitingForResponses[thing]);

            if (thing->stateValue(evboxPowerStateTypeId).toBool()) {
                sendCommand(thing, Command69, thing->stateValue(evboxMaxChargingCurrentStateTypeId).toDouble());
            } else {
                sendCommand(thing, Command69, 0);
            }
        });

        m_pendingSetups.take(thing)->finish(Thing::ThingErrorNoError);
    }
    if (!m_pendingActions.value(thing).isEmpty()) {
        ThingActionInfo *info = m_pendingActions.value(thing).first();
        if (info->action().actionTypeId() == evboxPowerActionTypeId) {
            thing->setStateValue(evboxPowerStateTypeId, info->action().paramValue(evboxPowerActionPowerParamTypeId));
        } else if (info->action().actionTypeId() == evboxMaxChargingCurrentActionTypeId) {
            thing->setStateValue(evboxMaxChargingCurrentStateTypeId, info->action().paramValue(evboxMaxChargingCurrentActionMaxChargingCurrentParamTypeId));
        }
        info->finish(Thing::ThingErrorNoError);
    }

    processDataPacket(thing, packet);
}

void IntegrationPluginEVBox::processDataPacket(Thing *thing, const QByteArray &packet)
{

    // The data is a mess of hex and dec values... So do not wonder about the weird from/to hex mess in here...
    QDataStream stream(QByteArray::fromHex(packet));

    quint8 from, to, commandId, wallboxCount;
    quint16 minPollInterval, maxChargingCurrent;
    stream >> from >> to >> commandId >> minPollInterval >> maxChargingCurrent >> wallboxCount;

    commandId = QString::number(commandId, 16).toInt();

    qCDebug(dcEVBox()) << QString("From: %1, To: %2, CMD: %3, MinPollInterval: %4, maxChargingCurrent: %5, Wallbox data count: %6")
                          .arg(from).arg(to).arg(commandId).arg(minPollInterval).arg(maxChargingCurrent).arg(wallboxCount);

    if (commandId != Command69) {
        qCWarning(dcEVBox()) << "Only command 69 is implemented! Adjust response parsing if sending other commands.";
        return;
    }

    m_waitingForResponses[thing] = false;

    // Command 69 would give a list of wallboxes (they can be chained apparently) but we only support a single one for now
//    for (int i = 0; i < wallboxCount; i++) {

    if (wallboxCount > 0) {
        quint16 minChargingCurrent, chargingCurrentL1, chargingCurrentL2, chargingCurrentL3, cosinePhiL1, cosinePhiL2, cosinePhiL3, totalEnergyConsumed;
        stream >> minChargingCurrent >> chargingCurrentL1 >> chargingCurrentL2 >> chargingCurrentL3 >> cosinePhiL1 >> cosinePhiL2 >> cosinePhiL3 >> totalEnergyConsumed;

        qCDebug(dcEVBox()) << QString("Min current: %1, actual current L1: %2, L2: %3, L3: %4, Total energy: %5")
                           .arg(minChargingCurrent).arg(chargingCurrentL1).arg(chargingCurrentL2).arg(chargingCurrentL3).arg(totalEnergyConsumed);

        thing->setStateMinMaxValues(evboxMaxChargingCurrentStateTypeId, minChargingCurrent / 10, maxChargingCurrent / 10);

        double currentPower = (chargingCurrentL1 + chargingCurrentL2 + chargingCurrentL3) * 23;
        thing->setStateValue(evboxCurrentPowerStateTypeId, currentPower);

        thing->setStateValue(evboxTotalEnergyConsumedStateTypeId, totalEnergyConsumed / 1000.0);

        thing->setStateValue(evboxChargingStateTypeId, currentPower > 0);

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
        // If all phases are on 0, we aren't charging and don't know how may phases are used...
        // so only updating the count if we actually do know that at least one is charging.
        if (phaseCount > 0) {
            thing->setStateValue(evboxPhaseCountStateTypeId, phaseCount);
        }
    }

    m_timers.value(thing)->start();
}

