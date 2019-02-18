#include "kebaconnection.h"
#include "extern-plugininfo.h"


KebaConnection::KebaConnection(QHostAddress address, QObject *parent) :
    QObject(parent),
    m_address(address)
{
    m_timeoutTimer = new QTimer(this);
    m_timeoutTimer->setSingleShot(true);
    connect(m_timeoutTimer, &QTimer::timeout, this, &KebaConnection::onTimeout);
}

QHostAddress KebaConnection::getAddress()
{
    return m_address;
}

void KebaConnection::setAddress(QHostAddress address)
{
    m_address = address;
}

void KebaConnection::sendCommand(const QByteArray &data)
{
    if(m_deviceBlocked) {
        //Set device blocked, will be unclocken when answer got received
        m_commandList.append(data);
    } else {
        //send command
        emit sendData(data);
        m_deviceBlocked = true;
        m_timeoutTimer->start(5000);
    }
}

void KebaConnection::onAnswerReceived()
{
    m_deviceBlocked = false;
    m_connected = true;
    emit connectionChanged(true);
    m_timeoutTimer->stop();

    //Send the next command
    if (!m_commandList.isEmpty()) {
        QByteArray command = m_commandList.takeFirst();
        emit sendData(command);
        m_deviceBlocked = true;
        m_timeoutTimer->start(5000);
    }
}

void KebaConnection::onTimeout()
{
    m_deviceBlocked = false;
    m_connected = false;
    emit connectionChanged(false);

    // Try to send the next command
    if (!m_commandList.isEmpty()) {
        QByteArray command = m_commandList.takeFirst();
        emit sendData(command);
        m_deviceBlocked = true;
        m_timeoutTimer->start(5000);
    }
}


void KebaConnection::enableOutput(bool state)
{
    // Print information that we are executing now the update action;
    QByteArray datagram;
    if(state){
        datagram.append("ena 1");
    }
    else{
        datagram.append("ena 0");
    }
    qCDebug(dcKebaKeContact()) << "Datagram : " << datagram;
    sendCommand(datagram);
}

void  KebaConnection::setMaxAmpere(int milliAmpere)
{
    // Print information that we are executing now the update action
    qCDebug(dcKebaKeContact()) << "Update max current to : " << milliAmpere;
    QByteArray data;
    data.append("curr " + QVariant(milliAmpere).toByteArray());
    qCDebug(dcKebaKeContact()) << "send command: " << data;
    sendCommand(data);
}

void  KebaConnection::displayMessage(const QByteArray &message)
{
    /* Text shown on the display. Maximum 23 ASCII characters can be used. 0 .. 23 characters
    ~ == Σ
    $ == blank
    , == comma
    */

    qCDebug(dcKebaKeContact()) << "Set display message: " << message;
    QByteArray data;
    QByteArray modifiedMessage = message;
    modifiedMessage.replace(" ", "$");
    if (modifiedMessage.size() > 23) {
        modifiedMessage.resize(23);
    }
    data.append("display 0 0 0 0 " + modifiedMessage);
    qCDebug(dcKebaKeContact()) << "send command: " << data;
    sendCommand(data);
}


void  KebaConnection::getDeviceInformation()
{
    QByteArray data;
    data.append("report 2");
    qCDebug(dcKebaKeContact()) << "send command: " << data;
    sendCommand(data);
}

void  KebaConnection::getReport1()
{
    QByteArray data;
    data.append("report 2");
    qCDebug(dcKebaKeContact()) << "send command : " << data;
    sendCommand(data);
}

void  KebaConnection::getReport2()
{
    QByteArray data;
    data.append("report 2");
    qCDebug(dcKebaKeContact()) << "send command: " << data;
    sendCommand(data);
}

void  KebaConnection::getReport3()
{
    QByteArray data;
    data.append("report 3");
    qCDebug(dcKebaKeContact()) << "data: " << data;
    sendCommand(data);
}


