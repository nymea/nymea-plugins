/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *                                                                         *
 *  Copyright (C) 2017 Bernhard Trinnes <bernhard.trinnes@guh.io>          *
 *                                                                         *
 *  This file is part of nymea.                                            *
 *                                                                         *
 *  nymea is free software: you can redistribute it and/or modify          *
 *  it under the terms of the GNU General Public License as published by   *
 *  the Free Software Foundation, version 2 of the License.                *
 *                                                                         *
 *  nymea is distributed in the hope that it will be useful,               *
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of         *
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the           *
 *  GNU General Public License for more details.                           *
 *                                                                         *
 *  You should have received a copy of the GNU General Public License      *
 *  along with nymea. If not, see <http://www.gnu.org/licenses/>.          *
 *                                                                         *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

#include "deviceplugintcpcommander.h"
#include "plugininfo.h"


DevicePluginTcpCommander::DevicePluginTcpCommander()
{
}

Device::DeviceSetupStatus DevicePluginTcpCommander::setupDevice(Device *device)
{
    if (device->deviceClassId() == tcpOutputDeviceClassId) {
        QTcpSocket *tcpSocket = new QTcpSocket(this);
        m_tcpSockets.insert(tcpSocket, device);
        connect(tcpSocket, &QTcpSocket::connected, this, &DevicePluginTcpCommander::onTcpSocketConnected);
        connect(tcpSocket, &QTcpSocket::disconnected, this, &DevicePluginTcpCommander::onTcpSocketDisconnected);
        connect(tcpSocket, &QTcpSocket::bytesWritten, this, &DevicePluginTcpCommander::onTcpSocketBytesWritten);
        return Device::DeviceSetupStatusAsync;
    }

    if (device->deviceClassId() == tcpInputDeviceClassId) {
        int port = device->paramValue(tcpInputDevicePortParamTypeId).toInt();
        TcpServer *tcpServer = new TcpServer(port, this);

        if (tcpServer->isValid()) {
            m_tcpServer.insert(tcpServer, device);
            connect(tcpServer, &TcpServer::connected, this, &DevicePluginTcpCommander::onTcpServerConnected);
            connect(tcpServer, &TcpServer::disconnected, this, &DevicePluginTcpCommander::onTcpServerDisconnected);
            return Device::DeviceSetupStatusSuccess;
        } else {
            tcpServer->deleteLater();
            qDebug(dcTCPCommander()) << "Could not open TCP Server";
        }
    }
    return Device::DeviceSetupStatusFailure;
}


Device::DeviceError DevicePluginTcpCommander::executeAction(Device *device, const Action &action)
{
    if (device->deviceClassId() == tcpOutputDeviceClassId) {

        if (action.actionTypeId() == tcpOutputTriggerActionTypeId) {
            int port = device->paramValue(tcpOutputDevicePortParamTypeId).toInt();
            QHostAddress address= QHostAddress(device->paramValue(tcpOutputDeviceIpv4addressParamTypeId).toString());
            QTcpSocket *tcpSocket = m_tcpSockets.key(device);
            tcpSocket->connectToHost(address, port);
            return Device::DeviceErrorNoError;
        }
        return Device::DeviceErrorActionTypeNotFound;
    }
    return Device::DeviceErrorDeviceClassNotFound;
}


void DevicePluginTcpCommander::deviceRemoved(Device *device)
{
    if(device->deviceClassId() == tcpOutputDeviceClassId){

        QTcpSocket *tcpSocket = m_tcpSockets.key(device);
        m_tcpSockets.remove(tcpSocket);
        tcpSocket->deleteLater();

    }else if(device->deviceClassId() == tcpInputDeviceClassId){

        TcpServer *tcpServer = m_tcpServer.key(device);
        m_tcpServer.remove(tcpServer);
        tcpServer->deleteLater();
    }
}


void DevicePluginTcpCommander::onTcpSocketConnected()
{
    QTcpSocket *tcpSocket = static_cast<QTcpSocket *>(sender());
    Device *device = m_tcpSockets.value(tcpSocket);
    if (device->deviceClassId() == tcpOutputDeviceClassId) {
        if (!device->setupComplete()) {
            qDebug(dcTCPCommander()) << device->name() << "Setup finished" ;
            emit deviceSetupFinished(device, Device::DeviceSetupStatusSuccess);
        } else {
            QByteArray data = device->paramValue(tcpOutputTriggerActionOutputDataAreaParamTypeId).toByteArray();
            tcpSocket->write(data);
        }
        device->setStateValue(tcpOutputConnectedStateTypeId, true);
    }
    if (device->deviceClassId() == tcpInputDeviceClassId) {
        if (!device->setupComplete()) {
            qDebug(dcTCPCommander()) << device->name() << "Setup finished" ;
            emit deviceSetupFinished(device, Device::DeviceSetupStatusSuccess);
        }
        device->setStateValue(tcpInputConnectedStateTypeId, true);
    }
}


void DevicePluginTcpCommander::onTcpSocketDisconnected()
{
    QTcpSocket *tcpSocket = static_cast<QTcpSocket *>(sender());
    Device *device = m_tcpSockets.value(tcpSocket);
    if (device->deviceClassId() == tcpInputDeviceClassId) {
        device->setStateValue(tcpInputConnectedStateTypeId, false);
    } else if (device->deviceClassId() == tcpOutputDeviceClassId) {
        device->setStateValue(tcpOutputConnectedStateTypeId, false);
    }
}


void DevicePluginTcpCommander::onTcpSocketBytesWritten()
{
    QTcpSocket *tcpSocket = static_cast<QTcpSocket *>(sender());
    tcpSocket->close();
}

void DevicePluginTcpCommander::onTcpServerConnected()
{
    TcpServer *tcpServer = static_cast<TcpServer *>(sender());
    Device *device = m_tcpServer.value(tcpServer);
    qDebug(dcTCPCommander()) << device->name() << "Tcp Server Client connected" ;
    if (device->deviceClassId() == tcpInputDeviceClassId) {
        device->setStateValue(tcpInputConnectedStateTypeId, true);
    } else if (device->deviceClassId() == tcpOutputDeviceClassId) {
        device->setStateValue(tcpOutputConnectedStateTypeId, true);
    }

    connect(tcpServer, &TcpServer::textMessageReceived, this, &DevicePluginTcpCommander::onTcpServerTextMessageReceived);
    //send signal device Setup was successful
}


void DevicePluginTcpCommander::onTcpServerDisconnected()
{
    TcpServer *tcpServer = static_cast<TcpServer *>(sender());
    Device *device = m_tcpServer.value(tcpServer);
    qDebug(dcTCPCommander()) << device->name() << "Tcp Server Client disconnected" ;
    if (device->deviceClassId() == tcpInputDeviceClassId) {
        device->setStateValue(tcpInputConnectedStateTypeId, false);
    } else if (device->deviceClassId() == tcpOutputDeviceClassId) {
        device->setStateValue(tcpOutputConnectedStateTypeId, false);
    }
}

void DevicePluginTcpCommander::onTcpServerTextMessageReceived(QByteArray data)
{
    TcpServer *tcpServer = static_cast<TcpServer *>(sender());
    Device *device = m_tcpServer.value(tcpServer);
    qDebug(dcTCPCommander()) << device->name() << "Message received" << data;
    device->setStateValue(tcpInputDataReceivedStateTypeId, data);

    if (device->paramValue(tcpInputDeviceComparisionParamTypeId).toString() == "Is exactly") {
        qDebug(dcTCPCommander()) << "is exactly";
        if (data == device->paramValue(tcpInputDeviceInputDataParamTypeId)) {
            qDebug(dcTCPCommander()) << "comparison successful";
            emitEvent(Event(tcpInputTriggeredEventTypeId, device->id()));
        }

    } else if (device->paramValue(tcpInputDeviceComparisionParamTypeId).toString() == "Contains") {
        if (data.contains(device->paramValue(tcpInputDeviceInputDataParamTypeId).toByteArray())) {
            emitEvent(Event(tcpInputTriggeredEventTypeId, device->id()));
        }

    } else if (device->paramValue(tcpInputDeviceComparisionParamTypeId) == "Contains not") {
        if (!data.contains(device->paramValue(tcpInputDeviceInputDataParamTypeId).toByteArray()))
            emitEvent(Event(tcpInputTriggeredEventTypeId, device->id()));

    } else if (device->paramValue(tcpInputDeviceComparisionParamTypeId) == "Starts with") {
        if (data.startsWith(device->paramValue(tcpInputDeviceInputDataParamTypeId).toByteArray()))
            emitEvent(Event(tcpInputTriggeredEventTypeId, device->id()));

    } else if (device->paramValue(tcpInputDeviceComparisionParamTypeId) == "Ends with") {
        if (data.endsWith(device->paramValue(tcpInputDeviceInputDataParamTypeId).toByteArray()))
            emitEvent(Event(tcpInputTriggeredEventTypeId, device->id()));
    }
}
