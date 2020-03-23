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

#include "integrationpluginatag.h"
#include "plugininfo.h"
#include "network/networkaccessmanager.h"

#include <QDebug>
#include <QUrlQuery>
#include <QJsonDocument>
#include <QNetworkInterface>


IntegrationPluginAtag::IntegrationPluginAtag()
{

}

void IntegrationPluginAtag::init()
{
    m_udpSocket = new QUdpSocket(this);

    if(m_udpSocket->bind(11000))
        qDebug() << "Bind on port" << m_udpSocket->localPort();
    else
        qDebug() << "Bind failed";

    connect(m_udpSocket, &QUdpSocket::readyRead, this, [this] (){
        QByteArray data;
        QHostAddress address;
        qint64 lenght = m_udpSocket->readDatagram(data.data(), 37, &address);
        if (data.startsWith("ONE") && (lenght == 37)) {
            //QByteArray serialnumber = data.right(33);
            AtagDiscoveryInfo atagInfo;
            atagInfo.port = 10000;
            atagInfo.address = address;
            atagInfo.lastSeen = QDateTime::currentDateTimeUtc();
            m_discoveredThings.insert(data, atagInfo);
        }
    });
}

void IntegrationPluginAtag::discoverThings(ThingDiscoveryInfo *info)
{
    foreach (QByteArray serialnumber, m_discoveredThings.keys()) {
        AtagDiscoveryInfo atagInfo = m_discoveredThings.value(serialnumber);
        ThingDescriptor descriptor(atagOneThingClassId, serialnumber, atagInfo.address.toString());
        ParamList params;

        foreach (Thing *existingThing, myThings().filterByThingClassId(info->thingClassId())) {
            if (existingThing->paramValue(atagOneThingSerialnumberParamTypeId).toString() == serialnumber) {
                descriptor.setThingId(existingThing->id());
            }
        }
        params << Param(atagOneThingSerialnumberParamTypeId, serialnumber);
        params << Param(atagOneThingIpAddressParamTypeId, atagInfo.address.toIPv4Address());
        params << Param(atagOneThingPortParamTypeId, atagInfo.port);
        descriptor.setParams(params);
        info->addThingDescriptor(descriptor);
    }
    info->finish(Thing::ThingErrorNoError);
}

void IntegrationPluginAtag::startPairing(ThingPairingInfo *info)
{
    info->finish(Thing::ThingErrorNoError, QT_TR_NOOP("Please follow the instructions displayed on the Atag One screen."));
}

void IntegrationPluginAtag::confirmPairing(ThingPairingInfo *info, const QString &username, const QString &password)
{
    Q_UNUSED(username)
    Q_UNUSED(password)

    QString macAddress;
    foreach (QNetworkInterface interface, QNetworkInterface::allInterfaces()){
        if (interface.flags() & QNetworkInterface::IsUp) {
            macAddress = interface.hardwareAddress();
            break;
        }
    }

    ParamList params = info->params();
    QHostAddress address(params.paramValue(atagOneThingIpAddressParamTypeId).toString());
    int port = params.paramValue(atagOneThingPortParamTypeId).toInt();
    Atag *atag = new Atag(hardwareManager()->networkManager(), address, port, macAddress, this);
    connect(atag, &Atag::pairMessageReceived, this, &IntegrationPluginAtag::onPairMessageReceived);
    m_unfinishedAtagConnections.insert(info->thingId(), atag);
    m_unfinishedThingPairings.insert(info->thingId(), info);
    atag->requestAuthorization();
}

void IntegrationPluginAtag::setupThing(ThingSetupInfo *info)
{
    Thing *thing = info->thing();

    if (thing->thingClassId() == atagOneThingClassId) {

        qCDebug(dcAtag()) << "Setup atag one connection" << thing->name() << thing->params();
        pluginStorage()->beginGroup(thing->id().toString());
        QString username = pluginStorage()->value("token").toString();
        pluginStorage()->endGroup();

        Atag *atag;
        if (m_unfinishedAtagConnections.contains(thing->id())) {
            atag = m_unfinishedAtagConnections.take(thing->id());
            m_atagConnections.insert(thing->id(), atag);
            return info->finish(Thing::ThingErrorNoError);
        } else {
            QString macAddress;
            foreach (QNetworkInterface interface, QNetworkInterface::allInterfaces()){
                if (interface.flags() & QNetworkInterface::IsUp) {
                    macAddress = interface.hardwareAddress();
                    break;
                }
            }
            QHostAddress address(thing->paramValue(atagOneThingIpAddressParamTypeId).toString());
            int port = thing->paramValue(atagOneThingPortParamTypeId).toInt();
            atag = new Atag(hardwareManager()->networkManager(), address, port, macAddress, this);
            connect(atag, &Atag::pairMessageReceived, this, &IntegrationPluginAtag::onPairMessageReceived);
            m_atagConnections.insert(thing->id(), atag);
            m_asyncThingSetup.insert(atag, info);
            return;
        }

    } else {
        qCWarning(dcAtag()) << "Unhandled device class in setupDevice" << thing->thingClassId();
        info->finish(Thing::ThingErrorSetupFailed);
    }
}

void IntegrationPluginAtag::thingRemoved(Thing *thing)
{
    if (thing->thingClassId() == atagOneThingClassId) {
        Atag *atag = m_atagConnections.take(thing->id());
        atag->deleteLater();
    }

    if (myThings().isEmpty()) {
        if (m_pluginTimer) {
            m_pluginTimer->deleteLater();
            m_pluginTimer = nullptr;
        }

        if (m_udpSocket) {
            m_udpSocket->deleteLater();
            m_udpSocket = nullptr;
        }
    }
}

void IntegrationPluginAtag::postSetupThing(Thing *thing)
{
    if (!m_pluginTimer) {
        m_pluginTimer = hardwareManager()->pluginTimerManager()->registerTimer(10);
        connect(m_pluginTimer, &PluginTimer::timeout, this, &IntegrationPluginAtag::onPluginTimer);
    }

    if (thing->thingClassId() == atagOneThingClassId) {
        Atag *atag = m_atagConnections.value(thing->id());
        atag->getInfo(Atag::InfoCategory::Report);
    }
}

void IntegrationPluginAtag::executeAction(ThingActionInfo *info)
{
    Thing *thing = info->thing();
    Action action = info->action();

    if (thing->thingClassId() == atagOneThingClassId) {
        Atag *atag = m_atagConnections.value(thing->parentId());
        if (!atag)
            return;

    }
}

void IntegrationPluginAtag::onPluginTimer()
{
    foreach (Thing *thing, myThings().filterByThingClassId(atagOneThingClassId)) {
        Atag *atag = m_atagConnections.value(thing->parentId());
        if (!atag)
            continue;

        atag->getInfo(Atag::InfoCategory::Status);
    }
}

void IntegrationPluginAtag::onPairMessageReceived(Atag::AuthResult result)
{
    Q_UNUSED(result)
}
