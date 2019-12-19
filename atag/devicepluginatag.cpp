/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *                                                                         *
 *  Copyright (C) 2019 Bernhard Trinnes <bernhard.trinnes@nymea.io>        *
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

#include "devicepluginatag.h"
#include "devices/device.h"
#include "plugininfo.h"
#include "network/networkaccessmanager.h"

#include <QDebug>
#include <QUrlQuery>
#include <QJsonDocument>
#include <QNetworkInterface>


DevicePluginAtag::DevicePluginAtag()
{

}

DevicePluginAtag::~DevicePluginAtag()
{
    if (m_udpSocket)
        m_udpSocket->deleteLater();
}

void DevicePluginAtag::init()
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
            m_discoveredDevices.insert(data, atagInfo);
        }
    });
}

void DevicePluginAtag::discoverDevices(DeviceDiscoveryInfo *info)
{
    foreach (QByteArray serialnumber, m_discoveredDevices.keys()) {
        AtagDiscoveryInfo atagInfo = m_discoveredDevices.value(serialnumber);
        DeviceDescriptor descriptor(atagOneDeviceClassId, serialnumber, atagInfo.address.toString());
        ParamList params;
        //TDOO check if devices is already added
        //TODO check last seen timestamp
        params << Param(atagOneDeviceSerialnumberParamTypeId, serialnumber);
        params << Param(atagOneDeviceIpAddressParamTypeId, atagInfo.address.toIPv4Address());
        params << Param(atagOneDevicePortParamTypeId, atagInfo.port);
        descriptor.setParams(params);
        info->addDeviceDescriptor(descriptor);
    }
    info->finish(Device::DeviceErrorNoError);
}

void DevicePluginAtag::startPairing(DevicePairingInfo *info)
{
    info->finish(Device::DeviceErrorNoError, QT_TR_NOOP("Please follow the instructions displayed on the Atag One screen."));
}

void DevicePluginAtag::confirmPairing(DevicePairingInfo *info, const QString &username, const QString &password)
{
    Q_UNUSED(username)
    Q_UNUSED(password)

    QString macAddress;
    foreach (QNetworkInterface interface, QNetworkInterface::allInterfaces()){
        if ((interface.flags() & QNetworkInterface::IsUp) && (interface.type()) & (QNetworkInterface::Ethernet | QNetworkInterface::Wifi)) {
            macAddress = interface.hardwareAddress();
            break;
        }
    }

    ParamList params = info->params();
    QHostAddress address(params.paramValue(atagOneDeviceIpAddressParamTypeId).toString());
    int port = params.paramValue(atagOneDevicePortParamTypeId).toInt();
    Atag *atag = new Atag(hardwareManager()->networkManager(), address, port, macAddress, this);
    connect(atag, &Atag::pairMessageReceived, this, &DevicePluginAtag::onPairMessageReceived);
    m_unfinishedAtagConnections.insert(info->deviceId(), atag);
    m_unfinishedDevicePairings.insert(info->deviceId(), info);
    atag->requestAuthorization();
}

void DevicePluginAtag::setupDevice(DeviceSetupInfo *info)
{
    Device *device = info->device();

    if (device->deviceClassId() == atagOneDeviceClassId) {

        qCDebug(dcAtag()) << "Setup atag one connection" << device->name() << device->params();
        pluginStorage()->beginGroup(device->id().toString());
        QString username = pluginStorage()->value("token").toString();
        pluginStorage()->endGroup();

        Atag *atag;
        if (m_unfinishedAtagConnections.contains(device->id())) {
            atag = m_unfinishedAtagConnections.take(device->id());
            m_atagConnections.insert(device->id(), atag);
            return info->finish(Device::DeviceErrorNoError);
        } else {
            QString macAddress;
            foreach (QNetworkInterface interface, QNetworkInterface::allInterfaces()){
                if ((interface.flags() & QNetworkInterface::IsUp) && (interface.type()) & (QNetworkInterface::Ethernet | QNetworkInterface::Wifi)) {
                    macAddress = interface.hardwareAddress();
                    break;
                }
            }
            QHostAddress address(device->paramValue(atagOneDeviceIpAddressParamTypeId).toString());
            int port = device->paramValue(atagOneDevicePortParamTypeId).toInt();
            atag = new Atag(hardwareManager()->networkManager(), address, port, macAddress, this);
            connect(atag, &Atag::pairMessageReceived, this, &DevicePluginAtag::onPairMessageReceived);
            m_atagConnections.insert(device->id(), atag);
            m_asyncDeviceSetup.insert(atag, info);
            return;
        }

    }
    qCWarning(dcAtag()) << "Unhandled device class in setupDevice";
}

void DevicePluginAtag::deviceRemoved(Device *device)
{
    if (device->deviceClassId() == atagOneDeviceClassId) {
        Atag *atag = m_atagConnections.take(device->id());
        atag->deleteLater();
    }

    if (myDevices().isEmpty() && m_pluginTimer) {
        m_pluginTimer->deleteLater();
        m_pluginTimer = nullptr;
    }
}

void DevicePluginAtag::postSetupDevice(Device *device)
{
    if (!m_pluginTimer) {
        m_pluginTimer = hardwareManager()->pluginTimerManager()->registerTimer(10);
        connect(m_pluginTimer, &PluginTimer::timeout, this, &DevicePluginAtag::onPluginTimer);
    }

    if (device->deviceClassId() == atagOneDeviceClassId) {
        Atag *atag = m_atagConnections.value(device->id());
        atag->getInfo(Atag::InfoCategory::Report);
    }
}

void DevicePluginAtag::executeAction(DeviceActionInfo *info)
{
    Device *device = info->device();
    Action action = info->action();

    if (device->deviceClassId() == atagOneDeviceClassId) {
        Atag *atag = m_atagConnections.value(device->parentId());
        if (!atag)
            return;

    }
}

void DevicePluginAtag::onPluginTimer()
{
    foreach (Device *device, myDevices().filterByDeviceClassId(atagOneDeviceClassId)) {
        Atag *atag = m_atagConnections.value(device->parentId());
        if (!atag)
            continue;

        atag->getInfo(Atag::InfoCategory::Status);
    }
}

void DevicePluginAtag::onPairMessageReceived(Atag::AuthResult result)
{
    Q_UNUSED(result)
}
