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

#include "deviceplugintcpcommander.h"
#include "plugininfo.h"


DevicePluginTcpCommander::DevicePluginTcpCommander()
{
}


void DevicePluginTcpCommander::setupDevice(DeviceSetupInfo *info)
{
    Device *device = info->device();

    if (device->deviceClassId() == tcpOutputDeviceClassId) {

        quint16 port = device->paramValue(tcpOutputDevicePortParamTypeId).toUInt();
        QHostAddress address= QHostAddress(device->paramValue(tcpOutputDeviceIpv4addressParamTypeId).toString());
        TcpSocket *tcpSocket = new TcpSocket(address, port, this);
        m_tcpSockets.insert(tcpSocket, device);
        connect(tcpSocket, &TcpSocket::connectionChanged, this, &DevicePluginTcpCommander::onTcpSocketConnectionChanged);

        connect(tcpSocket, &TcpSocket::connectionTestFinished, info, [info] (bool status) {
            if (status) {
               info->finish(Device::DeviceErrorNoError);
            } else {
                info->finish(Device::DeviceErrorSetupFailed, QT_TR_NOOP("Error connecting to remote server."));
            }
        });

        // Test the socket, if a socket can be established the setup process was successfull
        tcpSocket->connectionTest();
        return;
    }

    if (device->deviceClassId() == tcpInputDeviceClassId) {
        int port = device->paramValue(tcpInputDevicePortParamTypeId).toInt();
        TcpServer *tcpServer = new TcpServer(port, this);

        if (tcpServer->isValid()) {
            m_tcpServer.insert(tcpServer, device);
            connect(tcpServer, &TcpServer::connectionChanged, this, &DevicePluginTcpCommander::onTcpServerConnectionChanged);
            connect(tcpServer, &TcpServer::commandReceived, this, &DevicePluginTcpCommander::onTcpServerCommandReceived);
            return info->finish(Device::DeviceErrorNoError);
        } else {
            tcpServer->deleteLater();
            qDebug(dcTCPCommander()) << "Could not open TCP Server";
            return info->finish(Device::DeviceErrorSetupFailed, QT_TR_NOOP("Error opening TCP port."));
        }
    }
}


void DevicePluginTcpCommander::executeAction(DeviceActionInfo *info)
{
    Device *device = info->device();
    Action action = info->action();

    Q_ASSERT_X(action.actionTypeId() == tcpOutputTriggerActionTypeId, "TcpCommander", "Invalid action type in executeAction");

    TcpSocket *tcpSocket = m_tcpSockets.key(device);
    QByteArray data = action.param(tcpOutputTriggerActionOutputDataAreaParamTypeId).value().toByteArray();
    tcpSocket->sendCommand(data);

    connect(tcpSocket, &TcpSocket::commandSent, info, [info](bool success){
        if (success) {
            info->finish(Device::DeviceErrorNoError);
        } else {
            info->finish(Device::DeviceErrorHardwareNotAvailable);
        }
    });
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
