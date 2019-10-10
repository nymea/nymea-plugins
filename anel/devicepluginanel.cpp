/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *                                                                         *
 *  Copyright (C) 2018 Michael Zanetti <michael.zanetti@nymea.io>          *
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

/*
Example reply for HOME and PRO:

  HOME: GET /daten.cfg: Length 13
    0: NET-CONTROL    ;
    1: Mo, 07.01.19 02:24:54;
    2: 18;
    3: ;
    4: 12;
    5: 1;
    6: Login: <b>user7</b> 10.10.10.128;
    7: 4.5 DE;
    8: 7648;
    9: 0;
    10: H;
    11: end;
    12: NET - Power Control

  ADV: GET /daten.cfg: Length 11
    0: 1546827738;
    1: 64;
    2: 0;
    3: 37;
    4: 1;
    5: 19789;
    6: 5;
    7: 24.7;
    8: 9;
    9: 2;
    10: end

  HUT: GET /daten.cfg: Length 18
    0: 1546830796;
    1: 93;
    2: 41;
    3: 194;
    4: 1;
    5: 12587;
    6: 14;
    7: 24.7;
    8: 9;
    9: 2;
    10: end;
    11: s;
    12: 1;
    13: 22.90;
    14: 44.4;
    15: 851;
    16: 1;
    17: 0.00;



  HOME/PRO GET strg.cfg: Length 60
    0: NET-PWRCTRL_04.5;     // device name
    1: NET-CONTROL    ;      // hostname
    2: 10.10.10.132;         // IP
    3: 255.255.255.0;        // Netmask
    4: 10.10.10.1;           // Gateway
    5: 00:04:A3:0B:0C:3A;    // MAC
    6: 80;                   // Webcontrol port
    7: ;                     // Temp
    8: H;                    // Type
    9: ;                     // ?? (Skipped by upstream code)
    Following fields are repeated 8 times each, one for each socket
    10: Nr. 1;                // Name 1
    11: 1;                    // Stand
    12: 0;                    // Dis
    13: Anfangsstatus;        // Info
    14: ;                     // TK

    end;
    NET - Power Control"

  ADV: GET /strg.cfg: Length 58
    0: Nr.1;0;0;28;K;
    5: Nr.2;0;0;28;;
    10: Nr.3;1;0;27;;
    15: Nr.4;0;0;28;;
    20: Nr.5;0;0;28;;
    25: Nr.6;0;0;28;;
    30: Nr.7;0;0;0;;
    35: Nr.8;0;0;28;;
    40: end;
    41: 0;
    42: 0;
    43: 0;
    44: 1;
    45: 0;
    46: 0;
    47: 0;
    48: 1;
    49: 10122;
    50: 10123;
    51: 10124;
    52: 8657;
    53: 10126;
    54: 10127;
    55: 10128;
    56: 8659;

*/

#include "devicepluginanel.h"
#include "plugininfo.h"
#include "plugintimer.h"

#include <network/networkaccessmanager.h>
#include <QNetworkReply>
#include <QAuthenticator>
#include <QUrlQuery>

DevicePluginAnel::DevicePluginAnel()
{
    m_connectedStateTypeIdMap.insert(netPwrCtlHomeDeviceClassId, netPwrCtlHomeConnectedStateTypeId);
    m_connectedStateTypeIdMap.insert(netPwrCtlProDeviceClassId, netPwrCtlProConnectedStateTypeId);
    m_connectedStateTypeIdMap.insert(netPwrCtlAdvDeviceClassId, netPwrCtlAdvConnectedStateTypeId);
    m_connectedStateTypeIdMap.insert(netPwrCtlHutDeviceClassId, netPwrCtlHutConnectedStateTypeId);
    m_connectedStateTypeIdMap.insert(socketDeviceClassId, socketConnectedStateTypeId);

    m_ipAddressParamTypeIdMap.insert(netPwrCtlHomeDeviceClassId, netPwrCtlHomeDeviceIpAddressParamTypeId);
    m_ipAddressParamTypeIdMap.insert(netPwrCtlProDeviceClassId, netPwrCtlProDeviceIpAddressParamTypeId);
    m_ipAddressParamTypeIdMap.insert(netPwrCtlAdvDeviceClassId, netPwrCtlAdvDeviceIpAddressParamTypeId);
    m_ipAddressParamTypeIdMap.insert(netPwrCtlHutDeviceClassId, netPwrCtlHutDeviceIpAddressParamTypeId);

    m_portParamTypeIdMap.insert(netPwrCtlHomeDeviceClassId, netPwrCtlHomeDevicePortParamTypeId);
    m_portParamTypeIdMap.insert(netPwrCtlProDeviceClassId, netPwrCtlProDevicePortParamTypeId);
    m_portParamTypeIdMap.insert(netPwrCtlAdvDeviceClassId, netPwrCtlAdvDevicePortParamTypeId);
    m_portParamTypeIdMap.insert(netPwrCtlHutDeviceClassId, netPwrCtlHutDevicePortParamTypeId);
}

DevicePluginAnel::~DevicePluginAnel()
{
}

void DevicePluginAnel::init()
{
}

void DevicePluginAnel::discoverDevices(DeviceDiscoveryInfo *info)
{
    QUdpSocket *searchSocket = new QUdpSocket(this);

    // Note: This will fail, and it's not a problem, but it is required to force the socket to stick to IPv4...
    searchSocket->bind(QHostAddress::AnyIPv4, 30303);

    QString discoveryString = "Durchsuchen: Wer ist da?";
    qint64 len = searchSocket->writeDatagram(discoveryString.toUtf8(), QHostAddress("255.255.255.255"), 30303);
    if (len != discoveryString.length()) {
        searchSocket->deleteLater();
        qCWarning(dcAnelElektronik()) << "Error sending discovery";
        info->finish(Device::DeviceErrorHardwareFailure, QT_TR_NOOP("Error sending data to the network."));
        return;
    }

    QTimer::singleShot(2000, info, [this, searchSocket, info](){
        while(searchSocket->hasPendingDatagrams()) {
            char buffer[1024];
            QHostAddress senderAddress;
            int len = searchSocket->readDatagram(buffer, 1024, &senderAddress);
            QByteArray data = QByteArray::fromRawData(buffer, len);
            qCDebug(dcAnelElektronik()) << "Have datagram:" << data;
            if (!data.startsWith("NET-CONTROL")) {
                qCDebug(dcAnelElektronik()) << "Failed to parse discovery datagram from" << senderAddress << data;
                continue;
            }
            QStringList parts = QString(data).split("\r\n");
            if (parts.count() != 4) {
                qCDebug(dcAnelElektronik()) << "Failed to parse discovery datagram from" << senderAddress << data;
                continue;
            }
            qCDebug(dcAnelElektronik()) << "Found NET-CONTROL:" << senderAddress << parts.at(2) << parts.at(3) << senderAddress.protocol();

            ParamTypeId ipAddressParamTypeId = m_ipAddressParamTypeIdMap.value(info->deviceClassId());
            ParamTypeId portParamTypeId = m_portParamTypeIdMap.value(info->deviceClassId());

            bool existing = false;
            foreach (Device *existingDev, myDevices()) {
                if (existingDev->deviceClassId() == info->deviceClassId() && existingDev->paramValue(ipAddressParamTypeId).toString() == senderAddress.toString()) {
                    existing = true;
                }
            }
            if (existing) {
                qCDebug(dcAnelElektronik()) << "Already have device" << senderAddress << "in configured devices. Skipping...";
                continue;
            }
            DeviceDescriptor d(info->deviceClassId(), parts.at(2), senderAddress.toString());
            ParamList params;
            params << Param(ipAddressParamTypeId, senderAddress.toString());
            params << Param(portParamTypeId, parts.at(3).toInt());
            d.setParams(params);
            info->addDeviceDescriptor(d);
        }
        info->finish(Device::DeviceErrorNoError);
        searchSocket->deleteLater();
    });
}

void DevicePluginAnel::startPairing(DevicePairingInfo *info)
{
    info->finish(Device::DeviceErrorNoError, QT_TR_NOOP("Please enter the login credentials for your NET-PWRCTRL device."));
}

void DevicePluginAnel::confirmPairing(DevicePairingInfo *info, const QString &username, const QString &password)
{
    QString ipAddress = info->params().paramValue(m_ipAddressParamTypeIdMap.value(info->deviceClassId())).toString();
    int port = info->params().paramValue(m_portParamTypeIdMap.value(info->deviceClassId())).toInt();

    QNetworkRequest request;
    request.setUrl(QUrl(QString("http://%1:%2/strg.cfg").arg(ipAddress).arg(port)));
    request.setRawHeader("Authorization", "Basic " + QString("%1:%2").arg(username).arg(password).toUtf8().toBase64());
    qCDebug(dcAnelElektronik()) << "SetupDevice fetching:" << request.url() << request.rawHeader("Authorization") << username << password;
    QNetworkReply *reply = hardwareManager()->networkManager()->get(request);
    connect(reply, &QNetworkReply::finished, reply, &QNetworkReply::deleteLater);
    connect(reply, &QNetworkReply::finished, info, [this, info, reply, username, password](){
        if (reply->error() == QNetworkReply::NoError) {
            pluginStorage()->beginGroup(info->deviceId().toString());
            pluginStorage()->setValue("username", username);
            pluginStorage()->setValue("password", password);
            pluginStorage()->endGroup();
            info->finish(Device::DeviceErrorNoError);
        } else {
            info->finish(Device::DeviceErrorAuthenticationFailure, QT_TR_NOOP("Wrong username or password."));
        }
    });
}

void DevicePluginAnel::setupDevice(DeviceSetupInfo *info)
{
    Device *device = info->device();
    if (device->deviceClassId() == netPwrCtlHomeDeviceClassId
            || device->deviceClassId() == netPwrCtlProDeviceClassId) {
        setupHomeProDevice(info);
        return;
    }
    if (device->deviceClassId() == netPwrCtlAdvDeviceClassId
            || device->deviceClassId() == netPwrCtlHutDeviceClassId) {
        setupAdvDevice(info);
        return;
    }

    if (device->deviceClassId() == socketDeviceClassId) {
        qCDebug(dcAnelElektronik()) << "Setting up" << device->name();
        if (!m_pollTimer) {
            m_pollTimer = hardwareManager()->pluginTimerManager()->registerTimer(2);
            connect(m_pollTimer, &PluginTimer::timeout, this, &DevicePluginAnel::refreshStates);
        }
        info->finish(Device::DeviceErrorNoError);
        return;
    }

    qCWarning(dcAnelElektronik) << "Unhandled DeviceClass in setupDevice" << device->deviceClassId();
    info->finish(Device::DeviceErrorDeviceClassNotFound);
}

void DevicePluginAnel::deviceRemoved(Device *device)
{
    qCDebug(dcAnelElektronik) << "Device removed" << device->name();
    if (myDevices().isEmpty()) {
        hardwareManager()->pluginTimerManager()->unregisterTimer(m_pollTimer);
        m_pollTimer = nullptr;
    }
}

void DevicePluginAnel::executeAction(DeviceActionInfo *info)
{
    Device *device = info->device();
    Action action = info->action();

    if (device->deviceClassId() == socketDeviceClassId) {
        if (action.actionTypeId() == socketPowerActionTypeId) {

            Device *parentDevice = myDevices().findById(device->parentId());

            QString ipAddress = parentDevice->paramValue(m_ipAddressParamTypeIdMap.value(parentDevice->deviceClassId())).toString();
            int port = parentDevice->paramValue(m_portParamTypeIdMap.value(parentDevice->deviceClassId())).toInt();

            pluginStorage()->beginGroup(device->id().toString());
            QString username = pluginStorage()->value("username").toString();
            QString password = pluginStorage()->value("password").toString();
            pluginStorage()->endGroup();

            QUrl url(QString("http://%1:%2/ctrl.htm").arg(ipAddress).arg(port));
            QNetworkRequest request(url);
            request.setRawHeader("Authorization", "Basic " + QString("%1:%2").arg(username, password).toUtf8().toBase64());
            request.setHeader(QNetworkRequest::ContentTypeHeader, "text/plain");
            QByteArray data = QString("F%1=%2").arg(device->paramValue(socketDeviceNumberParamTypeId).toString(), action.param(socketPowerActionPowerParamTypeId).value().toBool() == true ? "1" : "0").toUtf8();
            QNetworkReply *reply = hardwareManager()->networkManager()->post(request, data);
            connect(reply, &QNetworkReply::finished, reply, &QNetworkReply::deleteLater);
            connect(reply, &QNetworkReply::finished, info, [reply, info](){
                if (reply->error() != QNetworkReply::NoError) {
                    qCWarning(dcAnelElektronik()) << "Execute action failed:" << reply->error() << reply->errorString();
                    info->finish(Device::DeviceErrorHardwareNotAvailable);
                    return;
                }
                qCDebug(dcAnelElektronik()) << "Execute action done.";
                info->finish(Device::DeviceErrorNoError);
            });
            return;
        }
    }
    info->finish(Device::DeviceErrorDeviceClassNotFound);
}

void DevicePluginAnel::refreshStates()
{
    foreach (Device *device, myDevices()) {
        if (device->deviceClassId() == netPwrCtlHomeDeviceClassId
                || device->deviceClassId() == netPwrCtlProDeviceClassId) {
            refreshHomePro(device);
        }
        if (device->deviceClassId() == netPwrCtlAdvDeviceClassId
                || device->deviceClassId() == netPwrCtlHutDeviceClassId) {
            refreshAdv(device);
        }
    }
}

void DevicePluginAnel::setConnectedState(Device *device, bool connected)
{
    device->setStateValue(m_connectedStateTypeIdMap.value(device->deviceClassId()), connected);
    foreach (Device *child, myDevices()) {
        if (child->parentId() == device->id()) {
            child->setStateValue(m_connectedStateTypeIdMap.value(child->deviceClassId()), connected);
        }
    }
}

void DevicePluginAnel::setupHomeProDevice(DeviceSetupInfo *info)
{
    Device *device = info->device();

    QString ipAddress = device->paramValue(m_ipAddressParamTypeIdMap.value(device->deviceClassId())).toString();
    int port = device->paramValue(m_portParamTypeIdMap.value(device->deviceClassId())).toInt();

    pluginStorage()->beginGroup(device->id().toString());
    QString username = pluginStorage()->value("username").toString();
    QString password = pluginStorage()->value("password").toString();
    pluginStorage()->endGroup();

    QNetworkRequest request;
    request.setUrl(QUrl(QString("http://%1:%2/strg.cfg").arg(ipAddress).arg(port)));
    request.setRawHeader("Authorization", "Basic " + QString("%1:%2").arg(username).arg(password).toUtf8().toBase64());
    qCDebug(dcAnelElektronik()) << "SetupDevice fetching:" << request.url() << request.rawHeader("Authorization") << username << password;
    QNetworkReply *reply = hardwareManager()->networkManager()->get(request);
    connect(reply, &QNetworkReply::finished, reply, &QNetworkReply::deleteLater);
    connect(reply, &QNetworkReply::finished, info, [this, info, reply](){
        StateTypeId connectedStateTypeId = m_connectedStateTypeIdMap.value(info->device()->deviceClassId());
        if (reply->error() != QNetworkReply::NoError) {
            qCWarning(dcAnelElektronik()) << "Error fetching state for" << info->device()->name() << reply->error() << reply->errorString();
            info->device()->setStateValue(connectedStateTypeId, false);
            info->finish(Device::DeviceErrorHardwareFailure, QT_TR_NOOP("The device rejected our connection. Please check the configured network ports."));
            return;
        }
        info->device()->setStateValue(connectedStateTypeId, true);

        QByteArray data = reply->readAll();

        QStringList parts = QString(data).split(';');

        int startIndex = parts.indexOf("end") - 58;
        if (startIndex < 0 || !parts.at(startIndex).startsWith("NET-PWRCTRL") || parts.length() < 60) {
            qCWarning(dcAnelElektronik()) << "Bad data from panel:" << data << "Length:" << parts.length();
            info->finish(Device::DeviceErrorHardwareFailure, QT_TR_NOOP("Unexpected data received from NET-PWRCTL device. Perhaps it's running an old firmware?"));
            return;
        }

        // At this point we're done with gathering information about the panel. Setup defintely succeeded for the gateway device
        info->finish(Device::DeviceErrorNoError);

        // If we haven't set up childs for this gateway yet, let's do it now
        foreach (Device *child, myDevices()) {
            if (child->parentId() == info->device()->id()) {
                // Already have childs for this panel. We're done here
                return;
            }
        }

        // Lets add the child devices now
        int childs = -1;
        QString type = parts.at(startIndex + 8);
        if (type == "H") {
            childs = 3;
        } else {
            childs = 8;
        }

        QList<DeviceDescriptor> descriptorList;
        for (int i = 0; i < childs; i++) {
            QString deviceName = parts.at(startIndex + 10 + i);
            DeviceDescriptor d(socketDeviceClassId, deviceName, info->device()->name(), info->device()->id());
            d.setParams(ParamList() << Param(socketDeviceNumberParamTypeId, i));
            descriptorList << d;
        }
        emit autoDevicesAppeared(descriptorList);
    });
}

void DevicePluginAnel::setupAdvDevice(DeviceSetupInfo *info)
{
    Device *device = info->device();

    QString ipAddress = device->paramValue(m_ipAddressParamTypeIdMap.value(device->deviceClassId())).toString();
    int port = device->paramValue(m_portParamTypeIdMap.value(device->deviceClassId())).toInt();

    pluginStorage()->beginGroup(device->id().toString());
    QString username = pluginStorage()->value("username").toString();
    QString password = pluginStorage()->value("password").toString();
    pluginStorage()->endGroup();

    QNetworkRequest request;
    request.setUrl(QUrl(QString("http://%1:%2/strg.cfg").arg(ipAddress).arg(port)));
    request.setRawHeader("Authorization", "Basic " + QString("%1:%2").arg(username).arg(password).toUtf8().toBase64());
    qCDebug(dcAnelElektronik()) << "SetupDevice fetching:" << request.url() << request.rawHeader("Authorization") << username << password;
    QNetworkReply *reply = hardwareManager()->networkManager()->get(request);
    connect(reply, &QNetworkReply::finished, reply, &QNetworkReply::deleteLater);
    connect(reply, &QNetworkReply::finished, info, [this, info, reply](){
        StateTypeId connectedStateTypeId = m_connectedStateTypeIdMap.value(info->device()->deviceClassId());
        if (reply->error() != QNetworkReply::NoError) {
            qCWarning(dcAnelElektronik()) << "Error fetching state for" << info->device()->name() << reply->error() << reply->errorString();
            info->device()->setStateValue(connectedStateTypeId, false);
            info->finish(Device::DeviceErrorHardwareFailure, QT_TR_NOOP("The device rejected our connection. Please check the configured network ports."));
            return;
        }
        info->device()->setStateValue(connectedStateTypeId, true);

        QByteArray data = reply->readAll();

        QStringList parts = QString(data).split(';');

        int startIndex = parts.indexOf("end") - 40;
        if (startIndex < 0 || parts.length() < 58) {
            qCWarning(dcAnelElektronik()) << "Bad data from panel:" << data << "Length:" << parts.length();
            info->finish(Device::DeviceErrorHardwareFailure, QT_TR_NOOP("Unexpected data received from NET-PWRCTL device. Perhaps it's running an old firmware?"));
            return;
        }

        // At this point we're done with gathering information about the panel. Setup defintely succeeded for the gateway device
        info->finish(Device::DeviceErrorNoError);

        // If we haven't set up childs for this gateway yet, let's do it now
        foreach (Device *child, myDevices()) {
            if (child->parentId() == info->device()->id()) {
                // Already have childs for this panel. We're done here
                return;
            }
        }

        QList<DeviceDescriptor> descriptorList;
        for (int i = 0; i < 8; i++) {
            QString deviceName = parts.at(startIndex + (i * 5));
            DeviceDescriptor d(socketDeviceClassId, deviceName, info->device()->name(), info->device()->id());
            d.setParams(ParamList() << Param(socketDeviceNumberParamTypeId, i));
            descriptorList << d;
        }
        emit autoDevicesAppeared(descriptorList);
    });
}

void DevicePluginAnel::refreshHomePro(Device *device)
{
    QString ipAddress = device->paramValue(m_ipAddressParamTypeIdMap.value(device->deviceClassId())).toString();
    int port = device->paramValue(m_portParamTypeIdMap.value(device->deviceClassId())).toInt();

    pluginStorage()->beginGroup(device->id().toString());
    QString username = pluginStorage()->value("username").toString();
    QString password = pluginStorage()->value("password").toString();
    pluginStorage()->endGroup();

    QUrl url(QString("http://%1:%2/strg.cfg").arg(ipAddress).arg(port));

    QNetworkRequest request;
    request.setUrl(url);
    request.setRawHeader("Authorization", "Basic " + QString("%1:%2").arg(username, password).toUtf8().toBase64());
    QNetworkReply *reply = hardwareManager()->networkManager()->get(request);
    connect(reply, &QNetworkReply::finished, reply, &QNetworkReply::deleteLater);
    connect(reply, &QNetworkReply::finished, device, [this, device, reply](){
        if (reply->error() != QNetworkReply::NoError) {
            qCWarning(dcAnelElektronik()) << "Error fetching state for" << device->name();
            setConnectedState(device, false);
            return;
        }
        QByteArray data = reply->readAll();
//        qCDebug(dcAnelElektronik()) << "States reply:" << data;

        QStringList parts = QString(data).split(';');
        int startIndex = parts.indexOf("end") - 58;
        if (startIndex < 0 || !parts.at(startIndex).startsWith("NET-PWRCTRL")) {
            qCWarning(dcAnelElektronik()) << "Bad data from Panel" << device->name() << data;
            // This happens sometimes when multiple clients are talking to the panel... Just ignore it...
            return;
        }
        setConnectedState(device, true);

        // The temp sensor seems to have a bit of jitter. Reduce sample rate to avoid spamming the log
        quint32 samples = device->property("tempSamples").toUInt();
        if (samples % 15 == 0 && device->deviceClassId() == netPwrCtlProDeviceClassId) {
            bool ok;
            double tempValue = parts.at(startIndex + 7).toDouble(&ok);
            if (ok) {
                device->setStateValue(netPwrCtlProTemperatureStateTypeId, tempValue);
            } else {
                qCWarning(dcAnelElektronik()) << "Error reading temperature value from data:" << parts.at(startIndex + 7);
            }
        }
        device->setProperty("tempSamples", ++samples);

        foreach (Device *child, myDevices()) {
            if (child->parentId() == device->id()) {
                int number = child->paramValue(socketDeviceNumberParamTypeId).toInt();
                child->setStateValue(socketPowerStateTypeId, parts.value(startIndex + 20 + number).toInt() == 1);
            }
        }
    });
}

void DevicePluginAnel::refreshAdv(Device *device)
{
    QString ipAddress = device->paramValue(m_ipAddressParamTypeIdMap.value(device->deviceClassId())).toString();
    int port = device->paramValue(m_portParamTypeIdMap.value(device->deviceClassId())).toInt();

    pluginStorage()->beginGroup(device->id().toString());
    QString username = pluginStorage()->value("username").toString();
    QString password = pluginStorage()->value("password").toString();
    pluginStorage()->endGroup();

    QUrl url(QString("http://%1:%2/strg.cfg").arg(ipAddress).arg(port));

    QNetworkRequest request;
    request.setUrl(url);
    request.setRawHeader("Authorization", "Basic " + QString("%1:%2").arg(username, password).toUtf8().toBase64());
    QNetworkReply *reply = hardwareManager()->networkManager()->get(request);
    connect(reply, &QNetworkReply::finished, reply, &QNetworkReply::deleteLater);
    connect(reply, &QNetworkReply::finished, device, [this, device, reply](){
        if (reply->error() != QNetworkReply::NoError) {
            qCWarning(dcAnelElektronik()) << "Error fetching state for" << device->name();
            setConnectedState(device, false);
            return;
        }
        QByteArray data = reply->readAll();
//        qCDebug(dcAnelElektronik()) << "States reply:" << data;

        QStringList parts = QString(data).split(';');
        int startIndex = parts.indexOf("end") - 40;
        if (startIndex < 0 || parts.count() < 58) {
            qCWarning(dcAnelElektronik()) << "Bad data from Panel" << device->name() << data << "Length:" << parts.length();
            // This happens sometimes when multiple clients are talking to the panel... Just ignore it...
            return;
        }
        setConnectedState(device, true);

        foreach (Device *child, myDevices()) {
            if (child->parentId() == device->id()) {
                int number = child->paramValue(socketDeviceNumberParamTypeId).toInt();
                child->setStateValue(socketPowerStateTypeId, parts.value(startIndex + (number * 5) + 1).toInt() == 1);
            }
        }

        // The temp sensor seems to have a bit of jitter. Reduce sample rate to avoid spamming the log
        quint32 samples = device->property("tempSamples").toUInt();
        if (samples % 15 == 0) {
            refreshAdvTemp(device);
        }
        device->setProperty("tempSamples", ++samples);

    });
}

void DevicePluginAnel::refreshAdvTemp(Device *device)
{
    QString ipAddress = device->paramValue(m_ipAddressParamTypeIdMap.value(device->deviceClassId())).toString();
    int port = device->paramValue(m_portParamTypeIdMap.value(device->deviceClassId())).toInt();

    pluginStorage()->beginGroup(device->id().toString());
    QString username = pluginStorage()->value("username").toString();
    QString password = pluginStorage()->value("password").toString();
    pluginStorage()->endGroup();

    QUrl url(QString("http://%1:%2/daten.cfg").arg(ipAddress).arg(port));

    QNetworkRequest request;
    request.setUrl(url);
    request.setRawHeader("Authorization", "Basic " + QString("%1:%2").arg(username, password).toUtf8().toBase64());
    QNetworkReply *reply = hardwareManager()->networkManager()->get(request);
    connect(reply, &QNetworkReply::finished, reply, &QNetworkReply::deleteLater);
    connect(reply, &QNetworkReply::finished, device, [this, device, reply](){
        if (reply->error() != QNetworkReply::NoError) {
            qCWarning(dcAnelElektronik()) << "Error fetching temp for" << device->name();
            setConnectedState(device, false);
            return;
        }
        QByteArray data = reply->readAll();
        qCDebug(dcAnelElektronik()) << "Temp reply:" << data;

        QStringList parts = QString(data).split(';');
        int startIndex = parts.indexOf("end") - 10;
        if (startIndex < 0) {
            qCWarning(dcAnelElektronik()) << "Bad data from Panel" << device->name() << data << "Length:" << parts.length();
            // This happens sometimes when multiple clients are talking to the panel... Just ignore it...
            return;
        }

        if (device->deviceClassId() == netPwrCtlAdvDeviceClassId) {
            bool ok;
            double temp = parts.at(7).toDouble(&ok);
            if (ok) {
                device->setStateValue(netPwrCtlAdvTemperatureStateTypeId, temp);
            } else {
                qCWarning(dcAnelElektronik()) << "Error fetching temperature value:" << data;
            }
        } else { // HUT
            if (parts.length() < 18) {
                qCWarning(dcAnelElektronik()) << "Data too short for HUT device" << data;
                return;
            }
            bool ok;
            double temp = parts.at(13).toDouble(&ok);
            if (ok) {
                device->setStateValue(netPwrCtlHutTemperatureStateTypeId, temp);
            } else {
                qCWarning(dcAnelElektronik()) << "Error fetching temperature value:" << data;
            }
            double humidity = parts.at(14).toDouble(&ok);
            if (ok) {
                device->setStateValue(netPwrCtlHutHumidityStateTypeId, humidity);
            } else {
                qCWarning(dcAnelElektronik()) << "Error fetching humidity value:" << data;
            }
            int lux = parts.at(15).toInt(&ok);
            if (ok) {
                device->setStateValue(netPwrCtlHutLightIntensityStateTypeId, lux);
            } else {
                qCWarning(dcAnelElektronik()) << "Error fetching light intensity value:" << data;
            }
        }

    });
}
