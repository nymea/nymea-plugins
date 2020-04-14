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

#include "integrationplugintcpcommander.h"
#include "plugininfo.h"


IntegrationPluginTcpCommander::IntegrationPluginTcpCommander()
{
}


void IntegrationPluginTcpCommander::setupThing(ThingSetupInfo *info)
{
    Thing *thing = info->thing();

    if (thing->thingClassId() == tcpOutputThingClassId) {

        quint16 port = thing->paramValue(tcpOutputThingPortParamTypeId).toUInt();
        QHostAddress address= QHostAddress(thing->paramValue(tcpOutputThingIpv4addressParamTypeId).toString());
        TcpSocket *tcpSocket = new TcpSocket(address, port, this);
        m_tcpSockets.insert(tcpSocket, thing);
        connect(tcpSocket, &TcpSocket::connectionChanged, this, &IntegrationPluginTcpCommander::onTcpSocketConnectionChanged);

        connect(tcpSocket, &TcpSocket::connectionTestFinished, info, [info] (bool status) {
            if (status) {
               info->finish(Thing::ThingErrorNoError);
            } else {
                info->finish(Thing::ThingErrorSetupFailed, QT_TR_NOOP("Error connecting to remote server."));
            }
        });

        // Test the socket, if a socket can be established the setup process was successfull
        tcpSocket->connectionTest();
        return;
    }

    if (thing->thingClassId() == tcpInputThingClassId) {
        int port = thing->paramValue(tcpInputThingPortParamTypeId).toInt();
        TcpServer *tcpServer = new TcpServer(port, this);

        if (tcpServer->isValid()) {
            m_tcpServer.insert(tcpServer, thing);
            connect(tcpServer, &TcpServer::connectionCountChanged, this, &IntegrationPluginTcpCommander::onTcpServerConnectionCountChanged);
            connect(tcpServer, &TcpServer::commandReceived, this, &IntegrationPluginTcpCommander::onTcpServerCommandReceived);
            return info->finish(Thing::ThingErrorNoError);
        } else {
            tcpServer->deleteLater();
            qDebug(dcTCPCommander()) << "Could not open TCP Server";
            return info->finish(Thing::ThingErrorSetupFailed, QT_TR_NOOP("Error opening TCP port."));
        }
    }
}


void IntegrationPluginTcpCommander::executeAction(ThingActionInfo *info)
{
    Thing *thing = info->thing();
    Action action = info->action();

    Q_ASSERT_X(action.actionTypeId() == tcpOutputTriggerActionTypeId, "TcpCommander", "Invalid action type in executeAction");

    TcpSocket *tcpSocket = m_tcpSockets.key(thing);
    QByteArray data = action.param(tcpOutputTriggerActionOutputDataAreaParamTypeId).value().toByteArray();
    tcpSocket->sendCommand(data);

    connect(tcpSocket, &TcpSocket::commandSent, info, [info](bool success){
        if (success) {
            info->finish(Thing::ThingErrorNoError);
        } else {
            info->finish(Thing::ThingErrorHardwareNotAvailable);
        }
    });
}


void IntegrationPluginTcpCommander::thingRemoved(Thing *thing)
{
    if(thing->thingClassId() == tcpOutputThingClassId){

        TcpSocket *tcpSocket = m_tcpSockets.key(thing);
        m_tcpSockets.remove(tcpSocket);
        tcpSocket->deleteLater();

    } else if(thing->thingClassId() == tcpInputThingClassId){

        TcpServer *tcpServer = m_tcpServer.key(thing);
        m_tcpServer.remove(tcpServer);
        tcpServer->deleteLater();
    }
}


void IntegrationPluginTcpCommander::onTcpSocketConnectionChanged(bool connected)
{
    TcpSocket *tcpSocket = static_cast<TcpSocket *>(sender());
    Thing *thing = m_tcpSockets.value(tcpSocket);
    if (thing->thingClassId() == tcpOutputThingClassId) {
        thing->setStateValue(tcpOutputConnectedStateTypeId, connected);
    }
}


void IntegrationPluginTcpCommander::onTcpServerConnectionCountChanged(int connections)
{
    TcpServer *tcpServer = static_cast<TcpServer *>(sender());
    Thing *thing = m_tcpServer.value(tcpServer);
    if (!thing)
        return;
    qDebug(dcTCPCommander()) << thing->name() << "Tcp Server Client connected";
    if (thing->thingClassId() == tcpInputThingClassId) {
        if (connections > 0) {
            thing->setStateValue(tcpInputConnectedStateTypeId, true);
        } else {
            thing->setStateValue(tcpInputConnectedStateTypeId, false);
        }
        thing->setStateValue(tcpInputConnectionCountStateTypeId, connections);
    }
}

void IntegrationPluginTcpCommander::onTcpServerCommandReceived(QByteArray data)
{
    TcpServer *tcpServer = static_cast<TcpServer *>(sender());
    Thing *thing = m_tcpServer.value(tcpServer);
    qDebug(dcTCPCommander()) << thing->name() << "Message received" << data;

    Event event = Event(tcpInputTriggeredEventTypeId, thing->id());
    ParamList params;
    params.append(Param(tcpInputTriggeredEventDataParamTypeId, data));
    event.setParams(params);
    emitEvent(event);
}
