/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*
* Copyright 2013 - 2021, nymea GmbH
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

#include "kecontact.h"
#include "extern-plugininfo.h"

#include <QJsonDocument>

KeContact::KeContact(const QHostAddress &address, KeContactDataLayer *dataLayer, QObject *parent) :
    QObject(parent),
    m_dataLayer(dataLayer),
    m_address(address)
{
    qCDebug(dcKebaKeContact()) << "Creating KeContact connection for address" << m_address;
    m_requestTimeoutTimer = new QTimer(this);
    m_requestTimeoutTimer->setSingleShot(true);
    connect(m_requestTimeoutTimer, &QTimer::timeout, this, [this] {
        // This timer will be started when a request is sent and stopped or resetted when a response has been received
        setReachable(false);

        //Try to send the next command
        handleNextCommandInQueue();
        m_deviceBlocked = false;
    });

    connect(m_dataLayer, &KeContactDataLayer::datagramReceived, this, &KeContact::onReceivedDatagram);
}

KeContact::~KeContact()
{
    qCDebug(dcKebaKeContact()) << "Deleting KeContact connection for address" << m_address.toString();
}

QHostAddress KeContact::address() const
{
    return m_address;
}

QUuid KeContact::start(const QByteArray &rfidToken, const QByteArray &rfidClassifier)
{
    if (!m_dataLayer) {
        qCWarning(dcKebaKeContact()) << "UDP socket not initialized";
        setReachable(false);
        return QUuid();
    }

    QUuid requestId = QUuid::createUuid();
    m_pendingRequests.append(requestId);
    QByteArray datagram = "start " + rfidToken + " " + rfidClassifier;
    qCDebug(dcKebaKeContact()) << "Datagram : " << datagram;
    sendCommand(datagram, requestId);;
    return requestId;
}

QUuid KeContact::stop(const QByteArray &rfidToken)
{
    if (!m_dataLayer) {
        qCWarning(dcKebaKeContact()) << "UDP socket not initialized";
        setReachable(false);
        return QUuid();
    }

    QUuid requestId = QUuid::createUuid();
    m_pendingRequests.append(requestId);
    QByteArray datagram = "stop " + rfidToken;
    qCDebug(dcKebaKeContact()) << "Datagram : " << datagram;
    sendCommand(datagram, requestId);
    return requestId;
}

void KeContact::setAddress(const QHostAddress &address)
{
    if (m_address == address)
        return;

    qCDebug(dcKebaKeContact()) << "Updating Keba connection address from" << m_address.toString() << "to" << address.toString();
    m_address = address;
}

bool KeContact::reachable() const
{
    return m_reachable;
}

void KeContact::sendCommand(const QByteArray &command, const QUuid &requestId)
{
    QTimer::singleShot(5000, this, [requestId, this] {
        if (m_pendingRequests.contains(requestId)) {
            m_pendingRequests.removeOne(requestId);
            emit commandExecuted(requestId, false);
        }
    });

    sendCommand(command);
}

void KeContact::sendCommand(const QByteArray &command)
{
    if (!m_dataLayer) {
        qCWarning(dcKebaKeContact()) << "UDP socket not initialized";
        setReachable(false);
        return;
    }

    if (m_deviceBlocked) {
        // Add command to queue
        m_commandList.append(command);
    } else {
        qCDebug(dcKebaKeContact()) << "Writing datagram to" << m_address.toString() << command;
        m_dataLayer->write(m_address, command);
        m_requestTimeoutTimer->start(5000);
        m_deviceBlocked = true;
    }
}

void KeContact::handleNextCommandInQueue()
{
    if (!m_dataLayer) {
        qCWarning(dcKebaKeContact()) << "Data layer not initialized";
        setReachable(false);
        return;
    }

    qCDebug(dcKebaKeContact()) << "Handle Command Queue - Pending commands" << m_commandList.length() << "Pending requestIds" << m_pendingRequests.length();
    if (!m_commandList.isEmpty()) {
        QByteArray command = m_commandList.takeFirst();
        qCDebug(dcKebaKeContact()) << "Writing datagram to" << m_address.toString() << command;
        m_dataLayer->write( m_address, command);
        m_requestTimeoutTimer->start(5000);
    }
}

void KeContact::setReachable(bool reachable)
{
    if (m_reachable == reachable)
        return;

    if (reachable) {
        qCDebug(dcKebaKeContact()) << "The keba wallbox on" << m_address.toString() << "is now reachable again.";
    } else {
        qCWarning(dcKebaKeContact()) << "The keba wallbox on" << m_address.toString() << "is not reachable any more.";
    }

    m_reachable = reachable;
    emit reachableChanged(m_reachable);
}

QUuid KeContact::enableOutput(bool state)
{
    if (!m_dataLayer) {
        qCWarning(dcKebaKeContact()) << "UDP socket not initialized";
        setReachable(false);
        return QUuid();
    }

    QUuid requestId = QUuid::createUuid();
    m_pendingRequests.append(requestId);
    // Print information that we are executing now the update action;
    QByteArray datagram;
    if (state){
        datagram.append("ena 1");
    } else{
        datagram.append("ena 0");
    }

    qCDebug(dcKebaKeContact()) << "Enable output, command:" << datagram;
    sendCommand(datagram, requestId);
    return requestId;
}

QUuid KeContact::setMaxAmpere(int milliAmpere)
{
    if (!m_dataLayer) {
        qCWarning(dcKebaKeContact()) << "UDP socket not initialized";
        setReachable(false);
        return QUuid();
    }

    if (milliAmpere < 6000 || milliAmpere > 63000) {
        qCWarning(dcKebaKeContact()) << "KeContact: Set max ampere, mA out of range [6000, 63000]" << milliAmpere;
        return QUuid();
    }

    QUuid requestId = QUuid::createUuid();
    m_pendingRequests.append(requestId);
    // Print information that we are executing now the update action
    qCDebug(dcKebaKeContact()) << "Update max current to : " << milliAmpere;
    QString commandLine = QString("currtime %1 %2").arg(milliAmpere, 1);
    QByteArray data = commandLine.toUtf8();
    qCDebug(dcKebaKeContact()) << "Set max. ampere, command: " << data;
    sendCommand(data, requestId);
    return requestId;
}

QUuid KeContact::displayMessage(const QByteArray &message)
{
    if (!m_dataLayer) {
        qCWarning(dcKebaKeContact()) << "UDP socket not initialized";
        setReachable(false);
        return QUuid();
    }

    /* Text shown on the display. Maximum 23 ASCII characters can be used. 0 .. 23 characters
    ~ == Σ
    $ == blank
    , == comma
    */
    QUuid requestId = QUuid::createUuid();
    m_pendingRequests.append(requestId);
    qCDebug(dcKebaKeContact()) << "Set display message: " << message;
    QByteArray data;
    QByteArray modifiedMessage = message;
    modifiedMessage.replace(" ", "$");
    if (modifiedMessage.size() > 23) {
        modifiedMessage.resize(23);
    }
    data.append("display 0 0 0 0 " + modifiedMessage);
    qCDebug(dcKebaKeContact()) << "Display message, command: " << data;
    sendCommand(data, requestId);
    return requestId;
}

QUuid KeContact::chargeWithEnergyLimit(double energy)
{
    if (!m_dataLayer) {
        qCWarning(dcKebaKeContact()) << "UDP socket not initialized";
        setReachable(false);
        return QUuid();
    }

    QUuid requestId = QUuid::createUuid();
    m_pendingRequests.append(requestId);

    QByteArray data;
    data.append("setenergy " + QVariant(static_cast<int>(energy*10000)).toByteArray());
    qCDebug(dcKebaKeContact()) << "Charge with energy limit, command: " << data;
    sendCommand(data, requestId);
    return requestId;
}

QUuid KeContact::setFailsafe(int timeout, int current, bool save)
{
    if (!m_dataLayer) {
        qCWarning(dcKebaKeContact()) << "UDP socket not initialized";
        setReachable(false);
        return QUuid();
    }

    QUuid requestId = QUuid::createUuid();
    m_pendingRequests.append(requestId);

    QByteArray data;
    data.append("failsave");
    data.append(" "+QVariant(timeout).toByteArray());
    data.append(" "+QVariant(current).toByteArray());
    data.append((save ? " 1":" 0"));
    qCDebug(dcKebaKeContact()) << "Set failsafe mode, command: " << data;
    sendCommand(data, requestId);
    return requestId;
}

void KeContact::getDeviceInformation()
{
    QByteArray data;
    data.append("i");
    qCDebug(dcKebaKeContact()) << "Get device information, command: " << data;
    sendCommand(data);
}

void KeContact::getReport1()
{
    getReport(1);
}

void KeContact::getReport2()
{
    getReport(2);
}

void KeContact::getReport3()
{
    getReport(3);
}

void KeContact::getReport1XX(int reportNumber)
{
    getReport(reportNumber);
}

QUuid KeContact::setOutputX2(bool state)
{
    if (!m_dataLayer) {
        qCWarning(dcKebaKeContact()) << "UDP socket not initialized";
        setReachable(false);
        return QUuid();
    }

    QUuid requestId = QUuid::createUuid();
    m_pendingRequests.append(requestId);
    QByteArray data;
    data.append("output "+QVariant((state ? 1 : 0)).toByteArray());
    qCDebug(dcKebaKeContact()) << "Set Output X2, state:" << state << "Command:" << data;
    sendCommand(data, requestId);
    return requestId;
}

void KeContact::getReport(int reportNumber)
{
    QByteArray data;
    data.append("report "+QVariant(reportNumber).toByteArray());
    qCDebug(dcKebaKeContact()) << "Get report" << reportNumber << "Command:" << data;
    sendCommand(data);
}

QUuid KeContact::unlockCharger()
{
    if (!m_dataLayer) {
        qCWarning(dcKebaKeContact()) << "UDP socket not initialized";
        setReachable(false);
        return QUuid();
    }

    QUuid requestId = QUuid::createUuid();
    m_pendingRequests.append(requestId);
    QByteArray data;
    data.append("unlock");
    qCDebug(dcKebaKeContact()) << "Unlock charger, command: " << data;
    sendCommand(data);
    return requestId;
}

void KeContact::onReceivedDatagram(const QHostAddress &address, const QByteArray &datagram)
{
    // Make sure the datagram is for this keba
    if (address != m_address) {
        return;
    }

    if (datagram.contains("TCH-OK")){
        // We received valid data from the address over the data link, so the wallbox must be reachable
        setReachable(true);

        //Command response has been received, now send the next command
        m_deviceBlocked = false;
        m_requestTimeoutTimer->stop();
        handleNextCommandInQueue();

        if (!m_pendingRequests.isEmpty()) {
            QUuid requestId = m_pendingRequests.takeFirst();
            if (datagram.contains("done")) {
                emit commandExecuted(requestId, true);
            } else {
                emit commandExecuted(requestId, false);
            }
        } else {
            //Probably the response has taken too long and the requestId has been already removed
        }

    } else if (datagram.left(8).contains("Firmware")){
        // We received valid data from the address over the data link, so the wallbox must be reachable
        setReachable(true);

        //Command response has been received, now send the next command
        m_deviceBlocked = false;
        m_requestTimeoutTimer->stop();
        handleNextCommandInQueue();

        qCDebug(dcKebaKeContact()) << "Firmware information received";
        QByteArrayList firmware = datagram.split(':');
        if (firmware.length() >= 2) {
            emit deviceInformationReceived(firmware[1]);
        }
    } else {

        //Command response has been received, now send the next command
        m_deviceBlocked = false;
        m_requestTimeoutTimer->stop();
        handleNextCommandInQueue();

        // Convert the rawdata to a json document
        QJsonParseError error;
        QJsonDocument jsonDoc = QJsonDocument::fromJson(datagram, &error);
        if (error.error != QJsonParseError::NoError) {
            qCWarning(dcKebaKeContact()) << "Failed to parse JSON data" << datagram << ":" << error.errorString();
            return;
        }

        QVariantMap data = jsonDoc.toVariant().toMap();

        if (data.contains("ID")) {
            int id = data.value("ID").toInt();
            if (id == 1) {
                // We received valid data from the address over the data link, so the wallbox must be reachable
                setReachable(true);

                ReportOne reportOne;
                qCDebug(dcKebaKeContact()) << "Report 1 received";
                reportOne.product      = data.value("Product").toString();
                reportOne.firmware     = data.value("Firmware").toString();
                reportOne.serialNumber = data.value("Serial").toString();
                //"Backend:"
                //"timeQ": 3
                //"DIP-Sw1": "0x22"
                //"DIP-Sw2":
                if (data.contains("COM-module")) {
                    reportOne.comModule = (data.value("COM-module").toInt() == 1);
                } else {
                    reportOne.comModule = false;
                }
                if (data.contains("Sec")) {
                    reportOne.comModule = data.value("Sec").toInt();
                } else {
                    reportOne.comModule = 0;
                }
                emit reportOneReceived(reportOne);

            } else if (id == 2) {
                // We received valid data from the address over the data link, so the wallbox must be reachable
                setReachable(true);

                ReportTwo reportTwo;
                qCDebug(dcKebaKeContact()) << "Report 2 received";
                int state = data.value("State").toInt();
                reportTwo.state = State(state);
                reportTwo.error1 = data.value("Error1").toInt();
                reportTwo.error2 = data.value("Error2").toInt();
                reportTwo.plugState = PlugState(data.value("Plug").toInt());
                reportTwo.enableUser  = data.value("Enable user").toBool();
                reportTwo.enableSys   = data.value("Enable sys").toBool();
                reportTwo.maxCurrent  = data.value("Max curr").toInt()/1000.00;
                reportTwo.maxCurrentPercentage = data.value("Max curr %").toInt()/10.00;
                reportTwo.currentHardwareLimitation = data.value("Curr HW").toInt()/1000.00;
                reportTwo.currentUser = data.value("Curr user").toInt()/1000.00;
                reportTwo.currentFailsafe = data.value("Curr FS").toInt()/1000.00;
                reportTwo.timeoutFailsafe = data.value("Tmo FS").toInt();
                reportTwo.setEnergy = data.value("Setenergy").toInt()/10000.00;
                reportTwo.output = data.value("Output").toInt();
                reportTwo.input= data.value("Input").toInt();
                reportTwo.serialNumber = data.value("Serial").toString();
                reportTwo.seconds = data.value("Sec").toInt();
                // Not documented:
                //"AuthON": 0
                //"Authreq": 0
                emit reportTwoReceived(reportTwo);

            } else if (id == 3) {
                // We received valid data from the address over the data link, so the wallbox must be reachable
                setReachable(true);

                ReportThree reportThree;
                qCDebug(dcKebaKeContact()) << "Report 3 received";
                reportThree.currentPhase1 = data.value("I1").toInt()/1000.00;
                reportThree.currentPhase2 = data.value("I2").toInt()/1000.00;
                reportThree.currentPhase3 = data.value("I3").toInt()/1000.00;
                reportThree.voltagePhase1 = data.value("U1").toInt();
                reportThree.voltagePhase2 = data.value("U2").toInt();
                reportThree.voltagePhase3 = data.value("U3").toInt();
                reportThree.power         = data.value("P").toInt()/1000.00;
                reportThree.powerFactor   = data.value("PF").toInt()/10.00;
                reportThree.energySession = data.value("E pres").toInt()/10000.00;
                reportThree.energyTotal   = data.value("E total").toInt()/10000.00;
                reportThree.serialNumber  = data.value("Serial").toString();
                reportThree.seconds  = data.value("Sec").toInt();
                emit reportThreeReceived(reportThree);
            } else if (id >= 100) {
                // We received valid data from the address over the data link, so the wallbox must be reachable
                setReachable(true);

                Report1XX report;
                qCDebug(dcKebaKeContact()) << "Report" << id << "received";
                report.sessionId = data.value("Session ID").toInt();
                report.currHW = data.value("Curr HW").toInt();
                report.startTime = data.value("E Start   ").toInt()/10000.00;
                report.presentEnergy = data.value("E Pres    ").toInt()/10000.00;
                report.startTime = data.value("started[s]").toInt();
                report.endTime = data.value("ended[s]  ").toInt();
                report.stopReason = data.value("reason ").toInt();
                report.rfidTag = data.value("RFID tag").toByteArray();
                report.rfidClass = data.value("RFID class").toByteArray();
                report.serialNumber = data.value("Serial").toString();
                report.seconds = data.value("Sec").toInt();
                emit report1XXReceived(id, report);
            }
        } else {
            if (data.contains("State")) {
                // We received valid data from the address over the data link, so the wallbox must be reachable
                setReachable(true);
                emit broadcastReceived(BroadcastType::BroadcastTypeState, data.value("State"));
            }
            if (data.contains("Plug")) {
                // We received valid data from the address over the data link, so the wallbox must be reachable
                setReachable(true);
                emit broadcastReceived(BroadcastType::BroadcastTypePlug, data.value("Plug"));
            }
            if (data.contains("Input")) {
                // We received valid data from the address over the data link, so the wallbox must be reachable
                setReachable(true);
                emit broadcastReceived(BroadcastType::BroadcastTypeInput, data.value("Input"));
            }
            if (data.contains("Enable sys")) {
                // We received valid data from the address over the data link, so the wallbox must be reachable
                setReachable(true);
                emit broadcastReceived(BroadcastType::BroadcastTypeEnableSys, data.value("Enable sys"));
            }
            if (data.contains("Max curr")) {
                // We received valid data from the address over the data link, so the wallbox must be reachable
                setReachable(true);
                emit broadcastReceived(BroadcastType::BroadcastTypeMaxCurr, data.value("Max curr"));
            }
            if (data.contains("E pres")) {
                // We received valid data from the address over the data link, so the wallbox must be reachable
                setReachable(true);
                emit broadcastReceived(BroadcastType::BroadcastTypeEPres, data.value("E pres"));
            }
        }
    }
}
