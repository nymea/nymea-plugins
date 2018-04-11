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

/*!
    \page udpcommander.html
    \title UDP Commander
    \brief Plugin for catching UDP commands from the network.

    \ingroup plugins
    \ingroup nymea-plugins-maker

    This plugin allows to receive UDP packages over a certain UDP port and generates an \l{Event} if the message content matches
    the \l{Param} command.

    \note This plugin is ment to be combined with a \l{nymeaserver::Rule}.

    \section3 Example

    If you create an UDP Commander on port 2323 and with the command \c{"Light 1 ON"}, following command will trigger an \l{Event} in nymea
    and allows you to connect this \l{Event} with a \l{nymeaserver::Rule}.

    \note In this example nymea is running on \c localhost

    \code
    $ echo "Light 1 ON" | nc -u localhost 2323
    OK
    \endcode

    This allows you to execute \l{Action}{Actions} in your home automation system when a certain UDP message will be sent to nymea.

    If the command will be recognized from nymea, the sender will receive as answere a \c{"OK"} string.

    \chapter Plugin properties
    Following JSON file contains the definition and the description of all available \l{DeviceClass}{DeviceClasses}
    and \l{Vendor}{Vendors} of this \l{DevicePlugin}.

    For more details how to read this JSON file please check out the documentation for \l{The plugin JSON File}.

    \quotefile plugins/deviceplugins/udpcommander/devicepluginudpcommander.json
*/

#include "devicepluginudpcommander.h"
#include "plugin/device.h"
#include "plugininfo.h"

DevicePluginUdpCommander::DevicePluginUdpCommander()
{

}

DeviceManager::DeviceSetupStatus DevicePluginUdpCommander::setupDevice(Device *device)
{
    qCDebug(dcUdpCommander()) << "Setup device" << device->name() << device->params();

    if (device->deviceClassId() == udpReceiverDeviceClassId) {
        QUdpSocket *udpSocket = new QUdpSocket(this);
        int port = device->paramValue(udpReceiverPortParamTypeId).toInt();
        if (!udpSocket->bind(QHostAddress::Any, port, QUdpSocket::ShareAddress)) {
            qCWarning(dcUdpCommander()) << device->name() << "cannot bind to port" << port;
            delete udpSocket;
            return DeviceManager::DeviceSetupStatusFailure;
        }
        qCDebug(dcUdpCommander()) << "Listening on port" << port;

        connect(udpSocket, SIGNAL(readyRead()), this, SLOT(readPendingDatagrams()));
        m_receiverList.insert(udpSocket, device);

        return DeviceManager::DeviceSetupStatusSuccess;
    } else if (device->deviceClassId() == udpCommanderDeviceClassId) {
        QUdpSocket *udpSocket = new QUdpSocket(this);
        m_commanderList.insert(udpSocket, device);
        return DeviceManager::DeviceSetupStatusSuccess;
    }

    return DeviceManager::DeviceSetupStatusFailure;
}


DeviceManager::DeviceError DevicePluginUdpCommander::executeAction(Device *device, const Action &action) {

    if (device->deviceClassId() == udpCommanderDeviceClassId) {
        if (action.actionTypeId() == udpCommanderTriggerActionTypeId) {
            QUdpSocket *udpSocket = m_commanderList.key(device);
            int port = device->paramValue(udpCommanderPortParamTypeId).toInt();
            QHostAddress address = QHostAddress(device->paramValue(udpCommanderAddressParamTypeId).toString());
            QByteArray data = action.param(udpCommanderDataParamTypeId).value().toByteArray();
            qDebug(dcUdpCommander()) << "Send UDP datagram:" << data << "address:" << address.toIPv4Address() << "port:" << port;
            udpSocket->writeDatagram(data, address, port);

            return DeviceManager::DeviceErrorNoError;
        }
        return DeviceManager::DeviceErrorActionTypeNotFound;
    }
    return DeviceManager::DeviceErrorDeviceClassNotFound;
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
    quint16 senderPort;

    while (socket->hasPendingDatagrams()) {
        datagram.resize(socket->pendingDatagramSize());
        socket->readDatagram(datagram.data(), datagram.size(), &sender, &senderPort);
    }

    qCDebug(dcUdpCommander()) << device->name() << "got command from" << sender.toString() << senderPort;
    Event ev = Event(udpReceiverTriggeredEventTypeId, device->id());
    ParamList params;
    params.append(Param(udpReceiverDataParamTypeId, datagram));
    ev.setParams(params);
    emit emitEvent(ev);

    // Send response for verification
    socket->writeDatagram("OK\n", sender, senderPort);
}
