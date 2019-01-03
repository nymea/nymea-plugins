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

/*!
    \page tasmota.html
    \title ANEL Elektronik devices
    \brief Plugin for ANEL Elektronik NET-PwrCtrl network controlled power sockets.

    \ingroup plugins
    \ingroup nymea-plugins-maker

    This plugin allows to make use of ANEL Elektronik NET-PwrCtrl controlled powet sockets.

    See https://anel-elektronik.de/ for a detailed description of the devices.

    \chapter Plugin properties
    When adding a device it will detect the type of the panel and create a gateway device and a powersocket
    device for each of the available sockets on the panel.

    \quotefile plugins/deviceplugins/tasmota/devicepluginanel.json
*/

#include "devicepluginanel.h"
#include "plugininfo.h"
#include "plugintimer.h"

#include <QNetworkDatagram>
#include <network/networkaccessmanager.h>
#include <QNetworkReply>
#include <QAuthenticator>

DevicePluginAnel::DevicePluginAnel()
{
    m_connectedStateTypeIdMap.insert(netPwrCtlDeviceClassId, netPwrCtlConnectedStateTypeId);
    m_connectedStateTypeIdMap.insert(socketDeviceClassId, socketConnectedStateTypeId);
}

DevicePluginAnel::~DevicePluginAnel()
{
}

void DevicePluginAnel::init()
{
}

DeviceManager::DeviceError DevicePluginAnel::discoverDevices(const DeviceClassId &deviceClassId, const ParamList &params)
{
    Q_UNUSED(deviceClassId)
    Q_UNUSED(params)

    QUdpSocket *searchSocket = new QUdpSocket(this);

    // Note: This will fail, and it's not a problem, but it is required to force the socket to stick to IPv4...
    searchSocket->bind(QHostAddress::AnyIPv4, 30303);

    QString discoveryString = "Durchsuchen: Wer ist da?";
    qint64 len = searchSocket->writeDatagram(discoveryString.toUtf8(), QHostAddress("255.255.255.255"), 30303);
    if (len != discoveryString.length()) {
        searchSocket->deleteLater();
        qCWarning(dcAnelElektronik()) << "Error sending discovery";
        return DeviceManager::DeviceErrorHardwareFailure;
    }

    QTimer::singleShot(2000, this, [this, searchSocket](){
        QList<DeviceDescriptor> descriptorList;
        while(searchSocket->hasPendingDatagrams()) {
            QNetworkDatagram datagram = searchSocket->receiveDatagram();
            qCDebug(dcAnelElektronik()) << "Have datagram:" << datagram.data();
            if (!datagram.data().startsWith("NET-CONTROL")) {
                qCDebug(dcAnelElektronik()) << "Failed to parse discovery datagram from" << datagram.senderAddress() << datagram.data();
                continue;
            }
            QStringList parts = QString(datagram.data()).split("\r\n");
            if (parts.count() != 4) {
                qCDebug(dcAnelElektronik()) << "Failed to parse discovery datagram from" << datagram.senderAddress() << datagram.data();
                continue;
            }
            qCDebug(dcAnelElektronik()) << "Found NET-CONTROL:" << datagram.senderAddress() << parts.at(2) << parts.at(3) << datagram.senderAddress().protocol();
            DeviceDescriptor d(netPwrCtlDeviceClassId, parts.at(2), datagram.senderAddress().toString());
            ParamList params;
            params << Param(netPwrCtlDeviceIpAddressParamTypeId, datagram.senderAddress().toString());
            params << Param(netPwrCtlDevicePortParamTypeId, parts.at(3).toInt());
            params << Param(netPwrCtlDeviceUsernameParamTypeId, "user7");
            params << Param(netPwrCtlDevicePasswordParamTypeId, "anel");
            d.setParams(params);
            descriptorList << d;
        }
        emit devicesDiscovered(netPwrCtlDeviceClassId, descriptorList);
        searchSocket->deleteLater();
    });
    return DeviceManager::DeviceErrorAsync;
}

DeviceManager::DeviceSetupStatus DevicePluginAnel::setupDevice(Device *device)
{
    if (device->deviceClassId() == netPwrCtlDeviceClassId) {
//        int sendPort = device->paramValue(netPwrCtlHomeDeviceSendPortParamTypeId).toInt();
//        int receivePort = device->paramValue(netPwrCtlHomeDeviceReceivePortParamTypeId).toInt();


        QNetworkRequest request;
        request.setUrl(QUrl("http://" + device->paramValue(netPwrCtlDeviceIpAddressParamTypeId).toString() + ":" + device->paramValue(netPwrCtlDevicePortParamTypeId).toString() + "/strg.cfg"));
        QString username = device->paramValue(netPwrCtlDeviceUsernameParamTypeId).toString();
        QString password = device->paramValue(netPwrCtlDevicePasswordParamTypeId).toString();
        request.setRawHeader("Authorization", "Basic " + QString("%1:%2").arg(username, password).toUtf8().toBase64());
        qCDebug(dcAnelElektronik()) << "SetupDevice fetching:" << request.url() << request.rawHeader("Authorization") << username << password;
        QNetworkReply *reply = hardwareManager()->networkManager()->get(request);
        connect(reply, &QNetworkReply::finished, reply, &QNetworkReply::deleteLater);
        connect(reply, &QNetworkReply::finished, device, [this, device, reply](){
            if (reply->error() != QNetworkReply::NoError) {
                qCWarning(dcAnelElektronik()) << "Error fetching state for" << device->name() << reply->error() << reply->errorString();
                device->setStateValue(netPwrCtlConnectedStateTypeId, false);
                emit deviceSetupFinished(device, DeviceManager::DeviceSetupStatusFailure);
                return;
            }
            device->setStateValue(netPwrCtlConnectedStateTypeId, true);

            QByteArray data = reply->readAll();

            QStringList parts = QString(data).split(';');

            int startIndex = parts.indexOf("end") - 58;
            if (startIndex < 0 || !parts.at(startIndex + 1).startsWith("NET-CONTROL")) {
                qCWarning(dcAnelElektronik()) << "Bad data from panel:" << data << "Length:" << parts.length();
                emit deviceSetupFinished(device, DeviceManager::DeviceSetupStatusFailure);
                return;
            }

            // At this point we're done with gathering information about the panel. Setup defintely succeeded for the gateway device
            emit deviceSetupFinished(device, DeviceManager::DeviceSetupStatusSuccess);

            // If we haven't set up childs for this gateway yet, let's do it now
            foreach (Device *child, myDevices()) {
                if (child->parentId() == device->id()) {
                    // Already have childs for this panel. We're done here
                    return;
                }
            }

            // Example reply:

            // NET-PWRCTRL_04.5;     // device name
            // NET-CONTROL    ;      // hostname
            // 10.10.10.132;         // IP
            // 255.255.255.0;        // Netmask
            // 10.10.10.1;           // Gateway
            // 00:04:A3:0B:0C:3A;    // MAC
            // 80;                   // Webcontrol port
            // ;                     // Temp
            // H;                    // Type
            // ;                     // ?? (Skipped by upstream code)

            // Following fields are repeated 1 times each, one for each socket

            // Nr. 1;                // Name 1
            // 1;                    // Stand
            // 0;                    // Dis
            // Anfangsstatus;        // Info
            // ;                     // TK

            // end;
            // NET - Power Control"


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
                DeviceDescriptor d(socketDeviceClassId, deviceName, device->name(), device->id());
                d.setParams(ParamList() << Param(socketDeviceNumberParamTypeId, i));
                descriptorList << d;
            }
            emit autoDevicesAppeared(socketDeviceClassId, descriptorList);
        });

        return DeviceManager::DeviceSetupStatusAsync;
    }

    if (device->deviceClassId() == socketDeviceClassId) {
        qCDebug(dcAnelElektronik()) << "Setting up" << device->name();
        if (!m_pollTimer) {
            m_pollTimer = hardwareManager()->pluginTimerManager()->registerTimer(2);
            connect(m_pollTimer, &PluginTimer::timeout, this, &DevicePluginAnel::refreshStates);
        }
        return DeviceManager::DeviceSetupStatusSuccess;
    }

    qCWarning(dcAnelElektronik) << "Unhandled DeviceClass in setupDevice" << device->deviceClassId();
    return DeviceManager::DeviceSetupStatusFailure;
}

void DevicePluginAnel::deviceRemoved(Device *device)
{
    qCDebug(dcAnelElektronik) << "Device removed" << device->name();
    if (myDevices().isEmpty()) {
        hardwareManager()->pluginTimerManager()->unregisterTimer(m_pollTimer);
        m_pollTimer = nullptr;
    }
}

DeviceManager::DeviceError DevicePluginAnel::executeAction(Device *device, const Action &action)
{
    if (device->deviceClassId() == socketDeviceClassId) {

        Device *parentDevice = myDevices().findById(device->parentId());

        if (action.actionTypeId() == socketPowerActionTypeId) {
            QUrl url("http://" + parentDevice->paramValue(netPwrCtlDeviceIpAddressParamTypeId).toString() + ":" + parentDevice->paramValue(netPwrCtlDevicePortParamTypeId).toString() + "/ctrl.htm");
            QNetworkRequest request(url);
            QString username = parentDevice->paramValue(netPwrCtlDeviceUsernameParamTypeId).toString();
            QString password = parentDevice->paramValue(netPwrCtlDevicePasswordParamTypeId).toString();
            request.setRawHeader("Authorization", "Basic " + QString("%1:%2").arg(username, password).toUtf8().toBase64());
            request.setHeader(QNetworkRequest::ContentTypeHeader, "text/plain");
            QByteArray data = QString("F%1=%2").arg(device->paramValue(socketDeviceNumberParamTypeId).toString(), action.param(socketPowerActionPowerParamTypeId).value().toBool() == true ? "1" : "0").toUtf8();
            QNetworkReply *reply = hardwareManager()->networkManager()->post(request, data);
            connect(reply, &QNetworkReply::finished, reply, &QNetworkReply::deleteLater);
            connect(reply, &QNetworkReply::finished, device, [this, reply, action](){
                if (reply->error() != QNetworkReply::NoError) {
                    qCWarning(dcAnelElektronik()) << "Execute action failed:" << reply->error() << reply->errorString();
                    emit actionExecutionFinished(action.id(), DeviceManager::DeviceErrorHardwareNotAvailable);
                }
                qCDebug(dcAnelElektronik()) << "Execute action done.";
                emit actionExecutionFinished(action.id(), DeviceManager::DeviceErrorNoError);
            });
            return DeviceManager::DeviceErrorAsync;
        }
    }
    return DeviceManager::DeviceErrorDeviceClassNotFound;
}

void DevicePluginAnel::refreshStates()
{
    foreach (Device *device, myDevices()) {
        if (device->deviceClassId() != netPwrCtlDeviceClassId) {
            continue;
        }

        QUrl url(QUrl("http://" + device->paramValue(netPwrCtlDeviceIpAddressParamTypeId).toString() + ":" + device->paramValue(netPwrCtlDevicePortParamTypeId).toString() + "/strg.cfg"));
//        qCDebug(dcAnelElektronik()) << "Fetching state from:" << url.toString();

        QNetworkRequest request;
        request.setUrl(url);
        QString username = device->paramValue(netPwrCtlDeviceUsernameParamTypeId).toString();
        QString password = device->paramValue(netPwrCtlDevicePasswordParamTypeId).toString();
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
//            qCDebug(dcAnelElektronik()) << "States reply:" << data;

            QStringList parts = QString(data).split(';');
            int startIndex = parts.indexOf("end") - 58;
            if (startIndex < 0 || !parts.at(startIndex + 1).startsWith("NET-CONTROL")) {
                qCWarning(dcAnelElektronik()) << "Bad data from Panel" << device->name() << data;
                // This happens sometimes as the panel replies with packets we didn't request... Just ignore it...
                return;
            }
            setConnectedState(device, true);

            foreach (Device *child, myDevices()) {
                if (child->parentId() == device->id()) {
                    int number = child->paramValue(socketDeviceNumberParamTypeId).toInt();
                    child->setStateValue(socketPowerStateTypeId, parts.value(startIndex + 20 + number).toInt() == 1);
                }
            }
        });
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
