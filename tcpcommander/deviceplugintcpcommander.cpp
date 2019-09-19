/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *                                                                         *
 *  Copyright (C) 2019 Bernhard Trinnes <bernhard.trinnes@nymea.io>        *
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

        quint16 port = device->paramValue(tcpOutputDevicePortParamTypeId).toUInt();
        QHostAddress address= QHostAddress(device->paramValue(tcpOutputDeviceIpv4addressParamTypeId).toString());
        TcpSocket *tcpSocket = new TcpSocket(address, port, this);
        m_tcpSockets.insert(tcpSocket, device);
        connect(tcpSocket, &TcpSocket::connectionChanged, this, &DevicePluginTcpCommander::onTcpSocketConnectionChanged);
        connect(tcpSocket, &TcpSocket::commandSent, this, &DevicePluginTcpCommander::onTcpSocketCommandSent);
        connect(tcpSocket, &TcpSocket::connectionTestFinished, this, [this, device] (bool status) {

            if (status) {
               emit deviceSetupFinished(device, Device::DeviceSetupStatusSuccess);
            } else {
               emit deviceSetupFinished(device, Device::DeviceSetupStatusFailure);
            }
        });
        tcpSocket->connectionTest();
        // Test the socket, if a socket can be established the setup process was successfull
        return Device::DeviceSetupStatusAsync;
    }

    if (device->deviceClassId() == tcpInputDeviceClassId) {
        int port = device->paramValue(tcpInputDevicePortParamTypeId).toInt();
        TcpServer *tcpServer = new TcpServer(port, this);

        if (tcpServer->isValid()) {
            m_tcpServer.insert(tcpServer, device);
            connect(tcpServer, &TcpServer::connectionChanged, this, &DevicePluginTcpCommander::onTcpServerConnectionChanged);
            connect(tcpServer, &TcpServer::commandReceived, this, &DevicePluginTcpCommander::onTcpServerCommandReceived);
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
            TcpSocket *tcpSocket = m_tcpSockets.key(device);
            QByteArray data = action.param(tcpOutputTriggerActionOutputDataAreaParamTypeId).value().toByteArray();
            tcpSocket->sendCommand(data);
            m_pendingActions.insert(action.id(), device->id());
            return Device::DeviceErrorAsync;
        }
        return Device::DeviceErrorActionTypeNotFound;
    }
    return Device::DeviceErrorDeviceClassNotFound;
}


void DevicePluginTcpCommander::deviceRemoved(Device *device)
{
    if(device->deviceClassId() == tcpOutputDeviceClassId){

        TcpSocket *tcpSocket = m_tcpSockets.key(device);
        m_tcpSockets.remove(tcpSocket);
        tcpSocket->deleteLater();

    } else if(device->deviceClassId() == tcpInputDeviceClassId){

        TcpServer *tcpServer = m_tcpServer.key(device);
        m_tcpServer.remove(tcpServer);
        tcpServer->deleteLater();
    }
}


void DevicePluginTcpCommander::onTcpSocketConnectionChanged(bool connected)
{
    TcpSocket *tcpSocket = static_cast<TcpSocket *>(sender());
    Device *device = m_tcpSockets.value(tcpSocket);
    if (device->deviceClassId() == tcpOutputDeviceClassId) {
        device->setStateValue(tcpOutputConnectedStateTypeId, connected);
    }
}

void DevicePluginTcpCommander::onTcpSocketCommandSent(bool successfull)
{
    TcpSocket *tcpSocket = static_cast<TcpSocket *>(sender());
    Device *device = m_tcpSockets.value(tcpSocket);

    ActionId action = m_pendingActions.key(device->id());
    m_pendingActions.remove(action);
    if (successfull) {
        emit actionExecutionFinished(action, Device::DeviceErrorNoError);
    } else {
        emit actionExecutionFinished(action, Device::DeviceErrorHardwareNotAvailable);
    }
}


void DevicePluginTcpCommander::onTcpServerConnectionChanged(bool connected)
{
    TcpServer *tcpServer = static_cast<TcpServer *>(sender());
    Device *device = m_tcpServer.value(tcpServer);
    qDebug(dcTCPCommander()) << device->name() << "Tcp Server Client connected" ;
    if (device->deviceClassId() == tcpInputDeviceClassId) {
        device->setStateValue(tcpInputConnectedStateTypeId, connected);
    }
}

void DevicePluginTcpCommander::onTcpServerCommandReceived(QByteArray data)
{
    TcpServer *tcpServer = static_cast<TcpServer *>(sender());
    Device *device = m_tcpServer.value(tcpServer);
    qDebug(dcTCPCommander()) << device->name() << "Message received" << data;

    Event event = Event(tcpInputTriggeredEventTypeId, device->id());
    ParamList params;
    params.append(Param(tcpInputTriggeredEventDataParamTypeId, data));
    event.setParams(params);
    emitEvent(event);
}
