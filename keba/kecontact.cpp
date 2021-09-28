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
    qCDebug(dcKeba()) << "Creating KeContact connection for address" << m_address;
    m_requestTimeoutTimer = new QTimer(this);
    m_requestTimeoutTimer->setSingleShot(true);
    connect(m_requestTimeoutTimer, &QTimer::timeout, this, [this]() {
        // This timer will be started when a request is sent and stopped or resetted when a response has been received
        setReachable(false);

        if (m_currentRequest.isValid()) {
            // Schedule pause timer to send next request
            qCWarning(dcKeba()) << "Command timeouted" << m_currentRequest.command();
            emit commandExecuted(m_currentRequest.requestId(), false);
        }

        // Timeout...send the next request right the way since at least 5 seconds passed since tha last command
        m_currentRequest = KeContactRequest();
        sendNextCommand();
    });

    m_pauseTimer = new QTimer(this);
    m_pauseTimer->setSingleShot(true);
    connect(m_pauseTimer, &QTimer::timeout, this, [this](){
        sendNextCommand();
    });

    connect(m_dataLayer, &KeContactDataLayer::datagramReceived, this, &KeContact::onReceivedDatagram);
}

KeContact::~KeContact()
{
    qCDebug(dcKeba()) << "Deleting KeContact connection for address" << m_address.toString();
}

QHostAddress KeContact::address() const
{
    return m_address;
}

QUuid KeContact::start(const QByteArray &rfidToken, const QByteArray &rfidClassifier)
{
    if (!m_dataLayer) {
        qCWarning(dcKeba()) << "UDP socket not initialized";
        setReachable(false);
        return QUuid();
    }

    QByteArray datagram = "start " + rfidToken + " " + rfidClassifier;
    KeContactRequest request(QUuid::createUuid(), datagram);
    qCDebug(dcKeba()) << "Start: Datagram:" << datagram;
    m_requestQueue.enqueue(request);
    sendNextCommand();
    return request.requestId();
}

QUuid KeContact::stop(const QByteArray &rfidToken)
{
    if (!m_dataLayer) {
        qCWarning(dcKeba()) << "UDP socket not initialized";
        setReachable(false);
        return QUuid();
    }

    QByteArray datagram = "stop " + rfidToken;
    KeContactRequest request(QUuid::createUuid(), datagram);
    qCDebug(dcKeba()) << "Stop: Datagram:" << datagram;
    m_requestQueue.enqueue(request);
    sendNextCommand();
    return request.requestId();
}

void KeContact::setAddress(const QHostAddress &address)
{
    if (m_address == address)
        return;

    qCDebug(dcKeba()) << "Updating Keba connection address from" << m_address.toString() << "to" << address.toString();
    m_address = address;
}

bool KeContact::reachable() const
{
    return m_reachable;
}

void KeContact::sendCommand(const QByteArray &command)
{
    if (!m_dataLayer) {
        qCWarning(dcKeba()) << "UDP socket not initialized";
        setReachable(false);
        return;
    }

    qCDebug(dcKeba()) << "--> Writing datagram to" << m_address.toString() << command;
    m_dataLayer->write(m_address, command);
    m_requestTimeoutTimer->start(5000);
}

void KeContact::sendNextCommand()
{
    // No message left, we are done
    if (m_requestQueue.isEmpty())
        return;

    // Still a request pending
    if (m_currentRequest.isValid())
        return;

    m_currentRequest = m_requestQueue.dequeue();
    sendCommand(m_currentRequest.command());
}

void KeContact::setReachable(bool reachable)
{
    if (m_reachable == reachable)
        return;

    if (reachable) {
        qCDebug(dcKeba()) << "The keba wallbox on" << m_address.toString() << "is now reachable again.";
    } else {
        qCWarning(dcKeba()) << "The keba wallbox on" << m_address.toString() << "is not reachable any more.";
    }

    m_reachable = reachable;
    emit reachableChanged(m_reachable);
}

QUuid KeContact::enableOutput(bool state)
{
    if (!m_dataLayer) {
        qCWarning(dcKeba()) << "UDP socket not initialized";
        setReachable(false);
        return QUuid();
    }

    // Print information that we are executing now the update action;
    QByteArray datagram;
    if (state){
        datagram.append("ena 1");
    } else{
        datagram.append("ena 0");
    }

    KeContactRequest request(QUuid::createUuid(), datagram);
    request.setDelayUntilNextCommand(2000);
    qCDebug(dcKeba()) << "Enable output: Datagram:" << datagram;
    m_requestQueue.enqueue(request);
    sendNextCommand();
    return request.requestId();
}

QUuid KeContact::setMaxAmpere(int milliAmpere)
{
    if (!m_dataLayer) {
        qCWarning(dcKeba()) << "UDP socket not initialized";
        setReachable(false);
        return QUuid();
    }

    if (milliAmpere < 6000 || milliAmpere > 63000) {
        qCWarning(dcKeba()) << "KeContact: Set max ampere, currtime mA out of range [6000, 63000]" << milliAmpere;
        return QUuid();
    }

    // Print information that we are executing now the update action
    qCDebug(dcKeba()) << "Update max current to : " << milliAmpere;
    QString commandLine = QString("currtime %1 0").arg(milliAmpere);
    QByteArray datagram = commandLine.toUtf8();
    KeContactRequest request(QUuid::createUuid(), datagram);
    request.setDelayUntilNextCommand(1200);
    qCDebug(dcKeba()) << "Set max charging amps: Datagram:" << datagram;
    m_requestQueue.enqueue(request);
    sendNextCommand();
    return request.requestId();
}

QUuid KeContact::setMaxAmpereGeneral(int milliAmpere)
{
    if (!m_dataLayer) {
        qCWarning(dcKeba()) << "UDP socket not initialized";
        setReachable(false);
        return QUuid();
    }

    if (milliAmpere < 6000 || milliAmpere > 63000) {
        qCWarning(dcKeba()) << "KeContact: Set max ampere curr, mA out of range [6000, 63000]" << milliAmpere;
        return QUuid();
    }

    // Print information that we are executing now the update action
    qCDebug(dcKeba()) << "Update general max current to: " << milliAmpere;
    QString commandLine = QString("curr %1").arg(milliAmpere);
    QByteArray datagram = commandLine.toUtf8();
    KeContactRequest request(QUuid::createUuid(), datagram);
    request.setDelayUntilNextCommand(1200);
    qCDebug(dcKeba()) << "Set max  general charging amps: Datagram:" << datagram;
    m_requestQueue.enqueue(request);
    sendNextCommand();
    return request.requestId();
}

QUuid KeContact::displayMessage(const QByteArray &message)
{
    if (!m_dataLayer) {
        qCWarning(dcKeba()) << "UDP socket not initialized";
        setReachable(false);
        return QUuid();
    }

    /* Text shown on the display. Maximum 23 ASCII characters can be used. 0 .. 23 characters
    ~ == Σ
    $ == blank
    , == comma
    */

    qCDebug(dcKeba()) << "Set display message: " << message;
    QByteArray datagram;
    QByteArray modifiedMessage = message;
    modifiedMessage.replace(" ", "$");
    if (modifiedMessage.size() > 23) {
        modifiedMessage.resize(23);
    }
    datagram.append("display 0 0 0 0 " + modifiedMessage);
    KeContactRequest request(QUuid::createUuid(), datagram);
    qCDebug(dcKeba()) << "Display message: Datagram:" << datagram;
    m_requestQueue.enqueue(request);
    sendNextCommand();
    return request.requestId();
}

QUuid KeContact::chargeWithEnergyLimit(double energy)
{
    if (!m_dataLayer) {
        qCWarning(dcKeba()) << "UDP socket not initialized";
        setReachable(false);
        return QUuid();
    }

    QByteArray datagram;
    datagram.append("setenergy " + QVariant(static_cast<int>(energy*10000)).toByteArray());
    KeContactRequest request(QUuid::createUuid(), datagram);
    qCDebug(dcKeba()) << "Charge with energy limit: Datagram: " << datagram;
    m_requestQueue.enqueue(request);
    sendNextCommand();
    return request.requestId();
}

QUuid KeContact::setFailsafe(int timeout, int current, bool save)
{
    if (!m_dataLayer) {
        qCWarning(dcKeba()) << "UDP socket not initialized";
        setReachable(false);
        return QUuid();
    }

    QByteArray data;
    data.append("failsave");
    data.append(" "+QVariant(timeout).toByteArray());
    data.append(" "+QVariant(current).toByteArray());
    data.append((save ? " 1":" 0"));
    KeContactRequest request(QUuid::createUuid(), data);
    qCDebug(dcKeba()) << "Set failsafe mode: Datagram: " << data;
    m_requestQueue.enqueue(request);
    sendNextCommand();
    return request.requestId();
}

void KeContact::getDeviceInformation()
{
    QByteArray data;
    data.append("i");
    KeContactRequest request(QUuid::createUuid(), data);
    qCDebug(dcKeba()) << "Get device information: Datagram: " << data;
    m_requestQueue.enqueue(request);
    sendNextCommand();
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
        qCWarning(dcKeba()) << "UDP socket not initialized";
        setReachable(false);
        return QUuid();
    }


    QByteArray datagram;
    datagram.append("output " + QVariant((state ? 1 : 0)).toByteArray());

    KeContactRequest request(QUuid::createUuid(), datagram);
    qCDebug(dcKeba()) << "Set Output X2, state:" << state << "Datagram:" << datagram;
    m_requestQueue.enqueue(request);
    sendNextCommand();
    return request.requestId();
}

void KeContact::getReport(int reportNumber)
{
    if (!m_dataLayer) {
        qCWarning(dcKeba()) << "UDP socket not initialized";
        setReachable(false);
        return;
    }

    QByteArray datagram;
    datagram.append("report " + QVariant(reportNumber).toByteArray());

    KeContactRequest request(QUuid::createUuid(), datagram);
    qCDebug(dcKeba()) << "Get report" << reportNumber << "Datagram:" << datagram;
    m_requestQueue.enqueue(request);
    sendNextCommand();
}

QUuid KeContact::unlockCharger()
{
    if (!m_dataLayer) {
        qCWarning(dcKeba()) << "UDP socket not initialized";
        setReachable(false);
        return QUuid();
    }

    QByteArray datagram;
    datagram.append("unlock");

    KeContactRequest request(QUuid::createUuid(), datagram);
    qCDebug(dcKeba()) << "Unlock charger: Datagram:" << datagram;
    m_requestQueue.enqueue(request);
    sendNextCommand();
    return request.requestId();
}

void KeContact::onReceivedDatagram(const QHostAddress &address, const QByteArray &datagram)
{
    // Make sure the datagram is for this keba
    if (address != m_address) {
        return;
    }

    qCDebug(dcKeba()) << "<--" << qUtf8Printable(datagram);

    if (datagram.contains("TCH-OK")){
        // We received valid data from the address over the data link, so the wallbox must be reachable
        setReachable(true);

        //Command response has been received, now send the next command
        m_requestTimeoutTimer->stop();

        if (m_currentRequest.isValid()) {
            if (datagram.contains("done")) {
                qCDebug(dcKeba()) << "Command" << m_currentRequest.command() << "finished successfully";
                emit commandExecuted(m_currentRequest.requestId(), true);
            } else {
                qCWarning(dcKeba()) << "Command" << m_currentRequest.command() << "finished with error" << datagram;
                emit commandExecuted(m_currentRequest.requestId(), false);
            }

            // Schedule pause timer to send next request
            m_pauseTimer->start(m_currentRequest.delayUntilNextCommand());
            m_currentRequest = KeContactRequest();
        } else {
            //Probably the response has taken too long and the requestId has been already removed
            qCWarning(dcKeba()) << "Received command OK response without pending request." << datagram;
        }
    } else if (datagram.left(8).contains("Firmware")){
        // We received valid data from the address over the data link, so the wallbox must be reachable
        setReachable(true);

        // Command response has been received, now send the next command
        m_requestTimeoutTimer->stop();
        if (m_currentRequest.isValid()) {
            // Schedule pause timer to send next request
            m_pauseTimer->start(m_currentRequest.delayUntilNextCommand());
            m_currentRequest = KeContactRequest();
        }

        qCDebug(dcKeba()) << "Firmware information received";
        QByteArrayList firmware = datagram.split(':');
        if (firmware.length() >= 2) {
            emit deviceInformationReceived(firmware[1]);
        }
    } else {
        //Command response has been received, now send the next command
        m_requestTimeoutTimer->stop();
        if (m_currentRequest.isValid()) {
            // Schedule pause timer to send next request
            m_pauseTimer->start(m_currentRequest.delayUntilNextCommand());
            m_currentRequest = KeContactRequest();
        }

        // Convert the rawdata to a json document
        QJsonParseError error;
        QJsonDocument jsonDoc = QJsonDocument::fromJson(datagram, &error);
        if (error.error != QJsonParseError::NoError) {
            qCWarning(dcKeba()) << "Failed to parse JSON data" << datagram << ":" << error.errorString();
            return;
        }

        QVariantMap data = jsonDoc.toVariant().toMap();

        if (data.contains("ID")) {
            int id = data.value("ID").toInt();
            if (id == 1) {
                // We received valid data from the address over the data link, so the wallbox must be reachable
                setReachable(true);

                ReportOne reportOne;
                qCDebug(dcKeba()) << "Report 1 received";
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
                qCDebug(dcKeba()) << "Report 2 received";
                int state = data.value("State").toInt();
                reportTwo.state = State(state);
                reportTwo.error1 = data.value("Error1").toInt();
                reportTwo.error2 = data.value("Error2").toInt();
                reportTwo.plugState = PlugState(data.value("Plug").toInt());
                reportTwo.enableUser = data.value("Enable user").toBool();
                reportTwo.enableSys = data.value("Enable sys").toBool();
                reportTwo.maxCurrent = data.value("Max curr").toInt() / 1000.00;
                reportTwo.maxCurrentPercentage = data.value("Max curr %").toInt() / 10.00;
                reportTwo.currentHardwareLimitation = data.value("Curr HW").toInt() / 1000.00;
                reportTwo.currentUser = data.value("Curr user").toInt() / 1000.00;
                reportTwo.currTimer = data.value("Curr timer").toInt() / 1000.00;
                reportTwo.timeoutCt = data.value("Tmo CT").toInt();
                reportTwo.currentFailsafe = data.value("Curr FS").toInt() / 1000.00;
                reportTwo.timeoutFailsafe = data.value("Tmo FS").toInt();
                reportTwo.setEnergy = data.value("Setenergy").toInt() / 10000.00;
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
                qCDebug(dcKeba()) << "Report 3 received";
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
                qCDebug(dcKeba()) << "Report" << id << "received";
                report.sessionId = data.value("Session ID").toInt();
                report.currHW = data.value("Curr HW").toInt();
                report.startEnergy = data.value("E start").toInt() / 10000.00;
                report.presentEnergy = data.value("E pres").toInt() / 10000.00;
                report.startTime = data.value("started[s]").toInt();
                report.endTime = data.value("ended[s]").toInt();
                report.stopReason = data.value("reason").toInt();
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
