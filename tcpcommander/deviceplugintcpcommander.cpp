/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *                                                                         *
 *  Copyright (C) 2017 Bernhard Trinnes <bernhard.trinnes@guh.io>          *
 *                                                                         *
 *  This file is part of guh.                                              *
 *                                                                         *
 *  Guh is free software: you can redistribute it and/or modify            *
 *  it under the terms of the GNU General Public License as published by   *
 *  the Free Software Foundation, version 2 of the License.                *
 *                                                                         *
 *  Guh is distributed in the hope that it will be useful,                 *
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of         *
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the           *
 *  GNU General Public License for more details.                           *
 *                                                                         *
 *  You should have received a copy of the GNU General Public License      *
 *  along with guh. If not, see <http://www.gnu.org/licenses/>.            *
 *                                                                         *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

#include "deviceplugintcpcommander.h"
#include "plugininfo.h"


DevicePluginTcpCommander::DevicePluginTcpCommander()
{
}

DeviceManager::DeviceSetupStatus DevicePluginTcpCommander::setupDevice(Device *device)
{
    if (device->deviceClassId() == tcpOutputDeviceClassId) {
        QTcpSocket *tcpSocket = new QTcpSocket(this);
        m_tcpSockets.insert(tcpSocket, device);
        connect(tcpSocket, &QTcpSocket::connected, this, &DevicePluginTcpCommander::onTcpSocketConnected);
        connect(tcpSocket, &QTcpSocket::disconnected, this, &DevicePluginTcpCommander::onTcpSocketDisconnected);
        connect(tcpSocket, &QTcpSocket::bytesWritten, this, &DevicePluginTcpCommander::onTcpSocketBytesWritten);
        return DeviceManager::DeviceSetupStatusAsync;
    }

    if (device->deviceClassId() == tcpInputDeviceClassId) {
        int port = device->paramValue(tcpInputPortParamTypeId).toInt();
        TcpServer *tcpServer = new TcpServer(port, this);

        if (tcpServer->isValid()) {
            m_tcpServer.insert(tcpServer, device);
            connect(tcpServer, &TcpServer::connected, this, &DevicePluginTcpCommander::onTcpServerConnected);
            connect(tcpServer, &TcpServer::disconnected, this, &DevicePluginTcpCommander::onTcpServerDisconnected);
            return DeviceManager::DeviceSetupStatusSuccess;
        } else {
            tcpServer->deleteLater();
            qDebug(dcTCPCommander()) << "Could not open TCP Server";
        }
    }
    return DeviceManager::DeviceSetupStatusFailure;
}


DeviceManager::DeviceError DevicePluginTcpCommander::executeAction(Device *device, const Action &action)
{
    if (device->deviceClassId() == tcpOutputDeviceClassId) {

        if (action.actionTypeId() == tcpOutputOutputDataActionTypeId) {
            int port = device->paramValue(tcpOutputPortParamTypeId).toInt();
            QHostAddress address= QHostAddress(device->paramValue(tcpOutputIpv4addressParamTypeId).toString());
            QTcpSocket *tcpSocket = m_tcpSockets.key(device);
            tcpSocket->connectToHost(address, port);
            return DeviceManager::DeviceErrorNoError;
        }
        return DeviceManager::DeviceErrorActionTypeNotFound;
    }
    return DeviceManager::DeviceErrorDeviceClassNotFound;
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
            emit deviceSetupFinished(device, DeviceManager::DeviceSetupStatusSuccess);
        } else {
            QByteArray data = device->paramValue(tcpOutputOutputDataAreaParamTypeId).toByteArray();
            tcpSocket->write(data);
        }
        device->setStateValue(tcpOutputConnectedStateTypeId, true);
    }
    if (device->deviceClassId() == tcpInputDeviceClassId) {
        if (!device->setupComplete()) {
            qDebug(dcTCPCommander()) << device->name() << "Setup finished" ;
            emit deviceSetupFinished(device, DeviceManager::DeviceSetupStatusSuccess);
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
    //send signal device Setup was successfull
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

    if (device->paramValue(tcpInputComparisionParamTypeId).toString() == "Is exactly") {
        qDebug(dcTCPCommander()) << "is exacly";
        if (data == device->paramValue(tcpInputInputDataParamTypeId)) {
            qDebug(dcTCPCommander()) << "comparison successful";
            emitEvent(Event(tcpInputCommandReceivedEventTypeId, device->id()));
        }

    } else if (device->paramValue(tcpInputComparisionParamTypeId).toString() == "Contains") {
        if (data.contains(device->paramValue(tcpInputInputDataParamTypeId).toByteArray())) {
            emitEvent(Event(tcpInputCommandReceivedEventTypeId, device->id()));
        }

    } else if (device->paramValue(tcpInputComparisionParamTypeId) == "Contains not") {
        if (!data.contains(device->paramValue(tcpInputInputDataParamTypeId).toByteArray()))
            emitEvent(Event(tcpInputCommandReceivedEventTypeId, device->id()));

    } else if (device->paramValue(tcpInputComparisionParamTypeId) == "Starts with") {
        if (data.startsWith(device->paramValue(tcpInputInputDataParamTypeId).toByteArray()))
            emitEvent(Event(tcpInputCommandReceivedEventTypeId, device->id()));

    } else if (device->paramValue(tcpInputComparisionParamTypeId) == "Ends with") {
        if (data.endsWith(device->paramValue(tcpInputInputDataParamTypeId).toByteArray()))
            emitEvent(Event(tcpInputCommandReceivedEventTypeId, device->id()));
    }
}
