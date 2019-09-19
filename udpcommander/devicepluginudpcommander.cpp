/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *                                                                         *
 *  Copyright (C) 2015-2018 Simon Stürz <simon.stuerz@guh.io>              *
 *  Copyright (C) 2017 Bernhard Trinnes <bernhard.trinnes@guh.io>          *
 *                                                                         *
 *  This file is part of nymea.                                            *
 *                                                                         *
 *  This library is free software; you can redistribute it and/or          *
 *  modify it under the terms of the GNU Lesser General Public             *
 *  License as published by the Free Software Foundation; either           *
 *  version 2.1 of the License, or (at your option) any later version.     *
 *                                                                         *
 *  This library is distributed in the hope that it will be useful,        *
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of         *
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU      *
 *  Lesser General Public License for more details.                        *
 *                                                                         *
 *  You should have received a copy of the GNU Lesser General Public       *
 *  License along with this library; If not, see                           *
 *  <http://www.gnu.org/licenses/>.                                        *
 *                                                                         *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

#include "devicepluginudpcommander.h"
#include "devices/device.h"
#include "plugininfo.h"

DevicePluginUdpCommander::DevicePluginUdpCommander()
{

}

void DevicePluginUdpCommander::setupDevice(DeviceSetupInfo *info)
{
    Device *device = info->device();

    qCDebug(dcUdpCommander()) << "Setup device" << device->name() << device->params();

    if (device->deviceClassId() == udpReceiverDeviceClassId) {
        QUdpSocket *udpSocket = new QUdpSocket(this);
        int port = device->paramValue(udpReceiverDevicePortParamTypeId).toInt();
        if (!udpSocket->bind(QHostAddress::Any, port, QUdpSocket::ShareAddress)) {
            qCWarning(dcUdpCommander()) << device->name() << "cannot bind to port" << port;
            delete udpSocket;
            return info->finish(Device::DeviceErrorHardwareNotAvailable, QT_TR_NOOP("Error opening UDP port."));
        }
        qCDebug(dcUdpCommander()) << "Listening on port" << port;

        connect(udpSocket, SIGNAL(readyRead()), this, SLOT(readPendingDatagrams()));
        m_receiverList.insert(udpSocket, device);

        return info->finish(Device::DeviceErrorNoError);
    } else if (device->deviceClassId() == udpCommanderDeviceClassId) {
        QUdpSocket *udpSocket = new QUdpSocket(this);
        m_commanderList.insert(udpSocket, device);
        return info->finish(Device::DeviceErrorNoError);
    }
}


void DevicePluginUdpCommander::executeAction(DeviceActionInfo *info)
{
    Device *device = info->device();
    Action action = info->action();

    Q_ASSERT_X(action.actionTypeId() == udpCommanderTriggerActionTypeId, "UdpCommander", "Unhandled action type in UDP commander.");

    QUdpSocket *udpSocket = m_commanderList.key(device);
    int port = device->paramValue(udpCommanderDevicePortParamTypeId).toInt();
    QHostAddress address = QHostAddress(device->paramValue(udpCommanderDeviceAddressParamTypeId).toString());
    QByteArray data = action.param(udpCommanderTriggerActionDataParamTypeId).value().toByteArray();
    qDebug(dcUdpCommander()) << "Send UDP datagram:" << data << "address:" << address.toIPv4Address() << "port:" << port;
    udpSocket->writeDatagram(data, address, port);

    return info->finish(Device::DeviceErrorNoError);
}

void DevicePluginUdpCommander::deviceRemoved(Device *device)
{
    if (device->deviceClassId() == udpReceiverDeviceClassId) {
        QUdpSocket *socket = m_receiverList.key(device);
        m_receiverList.remove(socket);
        socket->close();
        socket->deleteLater();

    } else if (device->deviceClassId() == udpCommanderDeviceClassId) {
        QUdpSocket *socket = m_commanderList.key(device);
        m_commanderList.remove(socket);
        socket->close();
        socket->deleteLater();
    }
}

void DevicePluginUdpCommander::readPendingDatagrams()
{
    qCDebug(dcUdpCommander()) << "UDP datagram received";
    QUdpSocket *socket= static_cast<QUdpSocket *>(sender());
    Device *device = m_receiverList.value(socket);

    if (!device) {
        qCWarning(dcUdpCommander()) << "Received a datagram from a socket we don't know";
        return;
    }

    QByteArray datagram;
    QHostAddress sender;
    quint16 senderPort = 0;

    while (socket->hasPendingDatagrams()) {
        datagram.resize(socket->pendingDatagramSize());
        socket->readDatagram(datagram.data(), datagram.size(), &sender, &senderPort);
    }

    qCDebug(dcUdpCommander()) << device->name() << "got command from" << sender.toString() << senderPort;
    Event ev = Event(udpReceiverTriggeredEventTypeId, device->id());
    ParamList params;
    params.append(Param(udpReceiverTriggeredEventDataParamTypeId, datagram));
    ev.setParams(params);
    emit emitEvent(ev);

    // Send response for verification
    socket->writeDatagram("OK\n", sender, senderPort);
}
