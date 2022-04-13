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

#include <QTimer>

IntegrationPluginTcpCommander::IntegrationPluginTcpCommander()
{
}


void IntegrationPluginTcpCommander::setupThing(ThingSetupInfo *info)
{
    Thing *thing = info->thing();

    if (thing->thingClassId() == tcpClientThingClassId) {

        quint16 port = thing->paramValue(tcpClientThingPortParamTypeId).toUInt();
        QHostAddress address= QHostAddress(thing->paramValue(tcpClientThingIpv4addressParamTypeId).toString());
        QTcpSocket *tcpSocket = m_tcpSockets.value(thing);
        if (!tcpSocket) {
            tcpSocket = new QTcpSocket(this);
            m_tcpSockets.insert(thing, tcpSocket);
        } else {
            // In case of a reconfigure, make sure we reconnect
            tcpSocket->disconnectFromHost();
        }
        connect(tcpSocket, &QTcpSocket::stateChanged, thing, [=](QAbstractSocket::SocketState state){
            thing->setStateValue(tcpClientConnectedStateTypeId, state == QAbstractSocket::ConnectedState);

            if (state == QAbstractSocket::UnconnectedState) {
                QTimer::singleShot(10000, tcpSocket, [=](){
                    qCDebug(dcTCPCommander()) << "Reconnecting to server" << address << port;
                    tcpSocket->connectToHost(address, port);
                });
            }
        });
        connect(tcpSocket, &QTcpSocket::readyRead, thing, [=](){
            QByteArray data = tcpSocket->readAll();
            ParamList params;
            params << Param(tcpClientTriggeredEventDataParamTypeId, data);
            Event event(tcpClientTriggeredEventTypeId, thing->id(), params);
            emitEvent(event);
        });

        tcpSocket->connectToHost(address, port);
        info->finish(Thing::ThingErrorNoError);
        return;
    }

    if (thing->thingClassId() == tcpServerThingClassId) {
        int port = thing->paramValue(tcpServerThingPortParamTypeId).toInt();

        TcpServer *tcpServer = m_tcpServers.value(thing);
        if (tcpServer) {
            // In case of reconfigure, make sure to re-setup the server
            delete tcpServer;
        }
        tcpServer = new TcpServer(port, this);

        if (tcpServer->isValid()) {
            m_tcpServers.insert(thing, tcpServer);
            connect(tcpServer, &TcpServer::connectionCountChanged, this, &IntegrationPluginTcpCommander::onTcpServerConnectionCountChanged);
            connect(tcpServer, &TcpServer::commandReceived, this, &IntegrationPluginTcpCommander::onTcpServerCommandReceived);
            info->finish(Thing::ThingErrorNoError);
            thing->setStateValue("connected", true);
            return;
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

    if (action.actionTypeId() == tcpClientTriggerActionTypeId) {
        QTcpSocket *tcpSocket = m_tcpSockets.value(thing);
        QByteArray data = action.param(tcpClientTriggerActionDataParamTypeId).value().toByteArray();
        qint64 len = tcpSocket->write(data);
        if (len == data.length()) {
            info->finish(Thing::ThingErrorNoError);
        } else {
            info->finish(Thing::ThingErrorHardwareNotAvailable);
        }
    }

    else if (action.actionTypeId() == tcpServerTriggerActionTypeId){
        TcpServer *server = m_tcpServers.value(thing);
        QByteArray data = action.param(tcpServerTriggerActionDataParamTypeId).value().toByteArray();
        QString clientIp = action.param(tcpServerTriggerActionClientIpParamTypeId).value().toString();
        bool success = server->sendCommand(clientIp, data);
        if (success) {
            info->finish(Thing::ThingErrorNoError);
        } else {
            info->finish(Thing::ThingErrorHardwareFailure, QT_TR_NOOP("Failed to send the command to the specified client(s)."));
        }
    }
}


void IntegrationPluginTcpCommander::thingRemoved(Thing *thing)
{
    if(thing->thingClassId() == tcpClientThingClassId){
        QTcpSocket *tcpSocket = m_tcpSockets.take(thing);
        tcpSocket->deleteLater();

    } else if(thing->thingClassId() == tcpServerThingClassId){
        TcpServer *tcpServer = m_tcpServers.take(thing);
        tcpServer->deleteLater();
    }
}


void IntegrationPluginTcpCommander::onTcpSocketConnectionChanged(bool connected)
{
    QTcpSocket *tcpSocket = static_cast<QTcpSocket *>(sender());
    Thing *thing = m_tcpSockets.key(tcpSocket);
    if (thing->thingClassId() == tcpClientThingClassId) {
        thing->setStateValue(tcpClientConnectedStateTypeId, connected);
    }
}


void IntegrationPluginTcpCommander::onTcpServerConnectionCountChanged(int connections)
{
    TcpServer *tcpServer = static_cast<TcpServer *>(sender());
    Thing *thing = m_tcpServers.key(tcpServer);
    if (!thing)
        return;
    qDebug(dcTCPCommander()) << thing->name() << "Tcp Server Client connected";
    if (thing->thingClassId() == tcpServerThingClassId) {
        if (connections > 0) {
            thing->setStateValue(tcpServerConnectedStateTypeId, true);
        } else {
            thing->setStateValue(tcpServerConnectedStateTypeId, false);
        }
        thing->setStateValue(tcpServerConnectionCountStateTypeId, connections);
    }
}

void IntegrationPluginTcpCommander::onTcpServerCommandReceived(const QString &clientIp, const QByteArray &data)
{
    TcpServer *tcpServer = static_cast<TcpServer *>(sender());
    Thing *thing = m_tcpServers.key(tcpServer);
    qDebug(dcTCPCommander()) << thing->name() << "Message received" << data;

    Event event = Event(tcpServerTriggeredEventTypeId, thing->id());
    ParamList params;
    params.append(Param(tcpServerTriggeredEventDataParamTypeId, data));
    params.append(Param(tcpServerTriggeredEventClientIpParamTypeId, clientIp));
    event.setParams(params);
    emitEvent(event);
}
