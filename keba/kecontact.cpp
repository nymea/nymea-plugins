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

#include "kecontact.h"
#include "extern-plugininfo.h"

#include <QJsonDocument>


KeContact::KeContact(QHostAddress address, QObject *parent) :
    QObject(parent),
    m_address(address)
{
    m_requestTimeoutTimer = new QTimer(this);
    m_requestTimeoutTimer->setSingleShot(true);
    connect(m_requestTimeoutTimer, &QTimer::timeout, this, [this] {
        //This timer will be started when a request is sent and stopped or resetted when a response has been received
        emit reachableChanged(false);
        //Try to send the next command
        handleNextCommandInQueue();
        m_deviceBlocked = false;
    });
}

KeContact::~KeContact() {
    qCDebug(dcKebaKeContact()) << "Deleting KeContact connection for address" << m_address;
}

bool KeContact::init(){

    if(!m_udpSocket){
        m_udpSocket = new QUdpSocket(this);
        if (!m_udpSocket->bind(QHostAddress::AnyIPv4, 7090, QAbstractSocket::ShareAddress)) {
            qCWarning(dcKebaKeContact()) << "Cannot bind to port" << 7090;
            delete m_udpSocket;
            return false;
        }
        connect(m_udpSocket, &QUdpSocket::readyRead, this, &KeContact::readPendingDatagrams);
    }
    return true;
}

QHostAddress KeContact::address()
{
    return m_address;
}

QUuid KeContact::start(const QByteArray &rfidToken, const QByteArray &rfidClassifier)
{
    QUuid requestId = QUuid::createUuid();
    m_pendingRequests.append(requestId);
    QByteArray datagram = "start "+rfidToken + " " + rfidClassifier;
    qCDebug(dcKebaKeContact()) << "Datagram : " << datagram;
    sendCommand(datagram, requestId);;
    return requestId;
}

QUuid KeContact::stop(const QByteArray &rfidToken)
{
    QUuid requestId = QUuid::createUuid();
    m_pendingRequests.append(requestId);
    QByteArray datagram = "stop "+rfidToken;
    qCDebug(dcKebaKeContact()) << "Datagram : " << datagram;
    sendCommand(datagram, requestId);
    return requestId;
}

void KeContact::setAddress(const QHostAddress &address)
{
    m_address = address;
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
    if (!m_udpSocket) {
        qCWarning(dcKebaKeContact()) << "UDP socket not initialized";
        emit reachableChanged(false);
        return;
    }

    if(m_deviceBlocked) {
        //add command to queue
        m_commandList.append(command);
    } else {
        //send command
        m_udpSocket->writeDatagram(command, m_address, 7090);
        m_requestTimeoutTimer->start(5000);
        m_deviceBlocked = true;
    }
}

void KeContact::handleNextCommandInQueue()
{
    if (!m_udpSocket) {
        qCWarning(dcKebaKeContact()) << "UDP socket not initialized";
        if (m_reachable == true) {
            m_reachable = false;
            emit reachableChanged(false);
        }
        return;
    }
    qCDebug(dcKebaKeContact()) << "Handle Command Queue- Pending commands" << m_commandList.length() << "Pending requestIds" << m_pendingRequests.length();
    if (!m_commandList.isEmpty()) {
        QByteArray command = m_commandList.takeFirst();
        m_udpSocket->writeDatagram(command, m_address, 7090);
        m_requestTimeoutTimer->start(5000);
    } else {
        //nothing to do
    }
}


QUuid KeContact::enableOutput(bool state)
{
    QUuid requestId = QUuid::createUuid();
    m_pendingRequests.append(requestId);
    // Print information that we are executing now the update action;
    QByteArray datagram;
    if(state){
        datagram.append("ena 1");
    } else{
        datagram.append("ena 0");
    }
    qCDebug(dcKebaKeContact()) << "Datagram : " << datagram;
    sendCommand(datagram, requestId);
    return requestId;
}

QUuid KeContact::setMaxAmpere(int milliAmpere)
{
    QUuid requestId = QUuid::createUuid();
    m_pendingRequests.append(requestId);
    // Print information that we are executing now the update action
    qCDebug(dcKebaKeContact()) << "Update max current to : " << milliAmpere;
    QByteArray data;
    data.append("curr " + QVariant(milliAmpere).toByteArray());
    qCDebug(dcKebaKeContact()) << "sSnd command: " << data;
    sendCommand(data, requestId);
    return requestId;
}

QUuid KeContact::displayMessage(const QByteArray &message)
{
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
    qCDebug(dcKebaKeContact()) << "Send command: " << data;
    sendCommand(data, requestId);
    return requestId;
}

QUuid KeContact::chargeWithEnergyLimit(double energy)
{
    QUuid requestId = QUuid::createUuid();
    m_pendingRequests.append(requestId);

    QByteArray data;
    data.append("setenergy " + QVariant(static_cast<int>(energy*10000)).toByteArray());
    qCDebug(dcKebaKeContact()) << "Send command: " << data;
    sendCommand(data, requestId);
    return requestId;
}

QUuid KeContact::setFailsafe(int timeout, int current, bool save)
{
    QUuid requestId = QUuid::createUuid();
    m_pendingRequests.append(requestId);

    QByteArray data;
    data.append("failsave");
    data.append(" "+QVariant(timeout).toByteArray());
    data.append(" "+QVariant(current).toByteArray());
    data.append((save ? " 1":" 0"));
    qCDebug(dcKebaKeContact()) << "Send command: " << data;
    sendCommand(data, requestId);
    return requestId;
}


void KeContact::getDeviceInformation()
{
    QByteArray data;
    data.append("i");
    qCDebug(dcKebaKeContact()) << "send command: " << data;
    sendCommand(data);
}

void KeContact::getReport1()
{
    QByteArray data;
    data.append("report 1");
    qCDebug(dcKebaKeContact()) << "send command : " << data;
    sendCommand(data);
}

void KeContact::getReport2()
{
    QByteArray data;
    data.append("report 2");
    qCDebug(dcKebaKeContact()) << "send command: " << data;
    sendCommand(data);
}

void KeContact::getReport3()
{
    QByteArray data;
    data.append("report 3");
    qCDebug(dcKebaKeContact()) << "data: " << data;
    sendCommand(data);
}

QUuid KeContact::unlockCharger()
{
    QUuid requestId = QUuid::createUuid();
    m_pendingRequests.append(requestId);
    QByteArray data;
    data.append("unlock");
    qCDebug(dcKebaKeContact()) << "send command: " << data;
    sendCommand(data);
    return requestId;
}

void KeContact::readPendingDatagrams()
{
    QUdpSocket *socket= qobject_cast<QUdpSocket*>(sender());

    QByteArray datagram;
    QHostAddress sender;
    quint16 senderPort;

    while (socket->hasPendingDatagrams()) {
        datagram.resize(socket->pendingDatagramSize());
        socket->readDatagram(datagram.data(), datagram.size(), &sender, &senderPort);
        if (sender != m_address) {
            //Only process data from the target device
            continue;
        }
        if (m_reachable != true) {
            m_reachable = true;
            emit reachableChanged(true);
        }

        qCDebug(dcKebaKeContact()) << "Data received" << datagram;
        if(datagram.contains("TCH-OK")){

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
        } else if(datagram.left(8).contains("Firmware")){

            //Command response has been received, now send the next command
            m_deviceBlocked = false;
            m_requestTimeoutTimer->stop();
            handleNextCommandInQueue();

            qCDebug(dcKebaKeContact()) << "Firmware information reveiced";
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
            }

            QVariantMap data = jsonDoc.toVariant().toMap();

            if(data.contains("ID")) {
                int id = data.value("ID").toInt();
                if (id == 1) {
                    ReportOne reportOne;
                    qCDebug(dcKebaKeContact()) << "Report 1 received";
                    reportOne.product      = data.value("Product").toString();
                    reportOne.firmware     = data.value("Firmware").toString();
                    reportOne.serialNumber = data.value("Serial").toString();
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

                    ReportTwo reportTwo;
                    qCDebug(dcKebaKeContact()) << "Report 2 received";
                    int state = data.value("State").toInt();
                    reportTwo.state = State(state);
                    reportTwo.error1 = data.value("Error1").toInt();
                    reportTwo.error2 = data.value("Error2").toInt();
                    reportTwo.plugState = PlugState(data.value("Plug").toInt());
                    reportTwo.enableUser  = data.value("Enable user").toBool();
                    reportTwo.enableSys   = data.value("Enable sys").toBool();
                    reportTwo.maxCurrent  = data.value("Max curr").toInt()/1000;
                    reportTwo.maxCurrentPercentage = data.value("Max curr %").toInt()/10;
                    reportTwo.currentHardwareLimitation = data.value("Curr HW").toInt()/1000;
                    reportTwo.currentUser = data.value("Curr user").toInt();
                    reportTwo.currentFailsafe = data.value("Curr FS").toInt();
                    reportTwo.timeoutFailsafe = data.value("Tmo FS").toInt();
                    reportTwo.output = data.value("Output").toInt();
                    reportTwo.input= data.value("Input").toInt();
                    reportTwo.serialNumber = data.value("Serial").toString();
                    reportTwo.seconds = data.value("Sec").toInt();
                    emit reportTwoReceived(reportTwo);

                } else if (id == 3) {

                    ReportThree reportThree;
                    qCDebug(dcKebaKeContact()) << "Report 3 reveiced";
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
                    Report1XX report;
                    report.sessionId = data.value("Session ID").toInt();
                    report.currHW = data.value("Curr HW").toInt();
                    //report. = data.value("Curr HW").toInt(); TODO
                    report.currHW = data.value("Curr HW").toInt();
                    report.currHW = data.value("Curr HW").toInt();
                    report.currHW = data.value("Curr HW").toInt();
                    report.currHW = data.value("Curr HW").toInt();
                    emit report1XXReceived(id, report);
                }
            } else {
                if (data.contains("State")) {
                    emit broadcastReceived(BroadcastType::BroadcastTypeState, data.value("State"));
                }
                if (data.contains("Plug")) {
                    emit broadcastReceived(BroadcastType::BroadcastTypePlug, data.value("Plug"));
                }
                if (data.contains("Input")) {
                    emit broadcastReceived(BroadcastType::BroadcastTypeInput, data.value("Input"));
                }
                if (data.contains("Enable sys")) {
                    emit broadcastReceived(BroadcastType::BroadcastTypeEnableSys, data.value("Enable sys"));
                }
                if (data.contains("Max curr")) {
                    emit broadcastReceived(BroadcastType::BroadcastTypeMaxCurr, data.value("Max curr"));
                }
                if (data.contains("E pres")) {
                    emit broadcastReceived(BroadcastType::BroadcastTypeEPres, data.value("E pres"));
                }
            }
        }
    }
}
