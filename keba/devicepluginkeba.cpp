/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *                                                                         *
 *  Copyright (C) 2019 Bernhard Trinnes <bernhard.trinnes@nymea.io         *
 *  Copyright (C) 2016-2018 Simon Stuerz <simon.stuerz@guh.io>             *
 *  Copyright (C) 2016 Christian Stachowitz                                *
 *                                                                         *
 *  This file is part of nymea.                                            *
 *                                                                         *
 *  Nymea is free software: you can redistribute it and/or modify          *
 *  it under the terms of the GNU General Public License as published by   *
 *  the Free Software Foundation, version 2 of the License.                *
 *                                                                         *
 *  Nymea is distributed in the hope that it will be useful,               *
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of         *
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the           *
 *  GNU General Public License for more details.                           *
 *                                                                         *
 *  You should have received a copy of the GNU General Public License      *
 *  along with nymea. If not, see <http://www.gnu.org/licenses/>.          *
 *                                                                         *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

#include "devicepluginkeba.h"

#include <QUrl>
#include <QUrlQuery>
#include <QJsonDocument>
#include <QPointer>
#include "plugininfo.h"
#include <QUdpSocket>

DevicePluginKeba::DevicePluginKeba()
{
    m_discovery = new Discovery(this);
    connect(m_discovery, &Discovery::finished, this, &DevicePluginKeba::discoveryFinished);
}

DevicePluginKeba::~DevicePluginKeba()
{

}

DeviceManager::DeviceError DevicePluginKeba::discoverDevices(const DeviceClassId &deviceClassId, const ParamList &params)
{
    Q_UNUSED(params);
    Q_UNUSED(deviceClassId);

    if (m_discovery->isRunning()) {
        qCWarning(dcKebaKeContact()) << "Network discovery already running";
        return DeviceManager::DeviceErrorDeviceInUse;
    }

    m_discovery->discoverHosts(25);
    return DeviceManager::DeviceErrorAsync;
}

DeviceManager::DeviceSetupStatus DevicePluginKeba::setupDevice(Device *device)
{
    if(!m_updateTimer) {
        m_updateTimer = hardwareManager()->pluginTimerManager()->registerTimer(60);
        connect(m_updateTimer, &PluginTimer::timeout, this, &DevicePluginKeba::updateData);
    }

    if(!m_kebaSocket){
        m_kebaSocket = new QUdpSocket(this);
        if (!m_kebaSocket->bind(QHostAddress::AnyIPv4, 7090)) {
            qCWarning(dcKebaKeContact()) << "Cannot bind to port" << 7090;
            delete m_kebaSocket;
            return DeviceManager::DeviceSetupStatusFailure;
        }
        connect(m_kebaSocket, &QUdpSocket::readyRead, this, &DevicePluginKeba::readPendingDatagrams);
    }

    qCDebug(dcKebaKeContact()) << "Setting up a new device:" << device->name() << device->paramValue(wallboxDeviceIpAddressParamTypeId).toString();
    QHostAddress address = QHostAddress(device->paramValue(wallboxDeviceIpAddressParamTypeId).toString());

    //Check if the IP is empty
    if (address.isNull()) {
        qCWarning(dcKebaKeContact()) << "Address is empty";
        return DeviceManager::DeviceSetupStatusFailure;
    }

    KebaConnection *kebaConnection = new KebaConnection(address, this);
    m_kebaConnections.insert(device, kebaConnection);
    connect(kebaConnection, &KebaConnection::sendData, this, &DevicePluginKeba::onSendData);
    connect(kebaConnection, &KebaConnection::connectionChanged, this, &DevicePluginKeba::onConnectionChanged);
    return DeviceManager::DeviceSetupStatusSuccess;
}

void DevicePluginKeba::postSetupDevice(Device *device)
{
    qCDebug(dcKebaKeContact()) << "Post setup" << device->name();
    KebaConnection *kebaConnection = m_kebaConnections.value(device);
    if (!kebaConnection) {
        qCWarning(dcKebaKeContact()) << "No Keba Connection";
        return;
    }
    kebaConnection->displayMessage("nymea");
    kebaConnection->getReport2();
    kebaConnection->getReport3();
}

void DevicePluginKeba::deviceRemoved(Device *device)
{
    // Remove devices
    KebaConnection *kebaConnection = m_kebaConnections.take(device);
    kebaConnection->deleteLater();

    if(m_kebaConnections.isEmpty()){
        m_kebaSocket->close();
        m_kebaSocket->deleteLater();
        qCDebug(dcKebaKeContact()) << "clear socket";
    }

    if (myDevices().isEmpty()) {
        hardwareManager()->pluginTimerManager()->unregisterTimer(m_updateTimer);
        m_updateTimer = nullptr;
        qCDebug(dcKebaKeContact()) << "device list empty, stopping timer";
    }
}

void DevicePluginKeba::updateData()
{
    foreach (KebaConnection *kebaConnection, m_kebaConnections.values()) {
        kebaConnection->getReport2();
        kebaConnection->getReport3();
    }
}

DeviceManager::DeviceError DevicePluginKeba::executeAction(Device *device, const Action &action)
{
    if (device->deviceClassId() == wallboxDeviceClassId) {
        KebaConnection *kebaConnection = m_kebaConnections.value(device);
        if (!kebaConnection) {
            qCWarning(dcKebaKeContact()) << "No Keba Connection";
            return DeviceManager::DeviceErrorHardwareNotAvailable;
        }

        if(action.actionTypeId() == wallboxMaxChargingCurrentActionTypeId){

            int current = action.param(wallboxMaxChargingCurrentActionMaxChargingCurrentParamTypeId).value().toInt();
            kebaConnection->setMaxAmpere(current);
            return DeviceManager::DeviceErrorNoError;
        }

        if(action.actionTypeId() == wallboxPowerActionTypeId) {
            bool state = action.param(wallboxPowerActionPowerParamTypeId).value().toBool();
            kebaConnection->enableOutput(state);
            return DeviceManager::DeviceErrorNoError;
        }

        if(action.actionTypeId() == wallboxDisplayActionTypeId) {
            QByteArray message = action.param(wallboxDisplayActionMessageParamTypeId).value().toByteArray();
            kebaConnection->displayMessage(message);
            return DeviceManager::DeviceErrorNoError;
        }
        return DeviceManager::DeviceErrorActionTypeNotFound;
    }
    return DeviceManager::DeviceErrorDeviceClassNotFound;
}

void DevicePluginKeba::readPendingDatagrams()
{
    QUdpSocket *socket= qobject_cast<QUdpSocket*>(sender());

    QByteArray datagram;
    QHostAddress sender;
    quint16 senderPort;
    bool senderUnkown = true;

    while (socket->hasPendingDatagrams()) {
        datagram.resize(socket->pendingDatagramSize());
        socket->readDatagram(datagram.data(), datagram.size(), &sender, &senderPort);

        foreach (Device *device, myDevices()) {
            if (device->paramValue(wallboxDeviceIpAddressParamTypeId).toString() == sender.toString())
            {
                senderUnkown = false;
                KebaConnection *kebaConnection = m_kebaConnections.value(device);
                if (!kebaConnection) {
                    qCWarning(dcKebaKeContact()) << "No Keba Connection";
                    continue;
                }
                kebaConnection->onAnswerReceived();

                if(datagram.contains("TCH-OK")){
                    qCDebug(dcKebaKeContact()) << "Command ACK:" << datagram;
                    continue;
                }

                // Convert the rawdata to a json document
                QJsonParseError error;
                QJsonDocument jsonDoc = QJsonDocument::fromJson(datagram, &error);
                if (error.error != QJsonParseError::NoError) {
                    qCWarning(dcKebaKeContact()) << "Failed to parse JSON data" << datagram << ":" << error.errorString();

                }
                QVariantMap data = jsonDoc.toVariant().toMap();
                if(data.contains("Firmware")){
                    qCDebug(dcKebaKeContact()) << "Firmware information reveiced";
                }

                if(data.contains("ID")){
                    if (data.value("ID").toString() == "1") {
                        qCDebug(dcKebaKeContact()) << "Report 1 reveiced";

                    } else if(data.value("ID").toString() == "2"){
                        qCDebug(dcKebaKeContact()) << "Report 2 reveiced";
                        device->setStateValue(wallboxConnectedStateTypeId,true);
                        //activity state
                        if(data.value("State").toString() == "0"){
                            device->setStateValue(wallboxActivityStateTypeId,"Starting");
                        }
                        else if(data.value("State").toString() == "1"){
                            device->setStateValue(wallboxActivityStateTypeId,"Not ready for charging");
                        }
                        else if(data.value("State").toString() == "2"){
                            device->setStateValue(wallboxActivityStateTypeId,"Ready for charging");
                        }
                        else if(data.value("State").toString() == "3"){
                            device->setStateValue(wallboxActivityStateTypeId,"Charging");
                        }
                        else if(data.value("State").toString() == "4"){
                            device->setStateValue(wallboxActivityStateTypeId,"Error");
                        }
                        else if(data.value("State").toString() == "5"){
                            device->setStateValue(wallboxActivityStateTypeId,"Authorization rejected");
                        }
                        // plug state
                        if(data.value("Plug").toString() == "0"){
                            device->setStateValue(wallboxPlugStateStateTypeId,"Unplugged");
                        }
                        else if(data.value("Plug").toString() == "1"){
                            device->setStateValue(wallboxPlugStateStateTypeId,"Plugged on charging station");
                        }
                        else if(data.value("Plug").toString() == "3"){
                            device->setStateValue(wallboxPlugStateStateTypeId,"Locked plug on charging station");
                        }
                        else if(data.value("Plug").toString() == "5"){
                            device->setStateValue(wallboxPlugStateStateTypeId,"Plugged on charging station and vehicle");
                        }
                        else if(data.value("Plug").toString() == "7"){
                            device->setStateValue(wallboxPlugStateStateTypeId,"Locked plug on charging station and vehicle");
                        }
                        //maximum current setting
                        device->setStateValue(wallboxMaxChargingCurrentStateTypeId,data.value("Curr user").toInt());
                        device->setStateValue(wallboxMaxChargingCurrentPercentStateTypeId,data.value("Max curr %").toInt()/10);
                        device->setStateValue(wallboxPowerStateTypeId,data.value("Enable sys").toBool());
                    }
                    else if(data.value("ID").toString() == "3"){

                        qCDebug(dcKebaKeContact()) << "Report 3 reveiced";
                        device->setStateValue(wallboxI1StateTypeId,data.value("I1").toInt());
                        device->setStateValue(wallboxI2StateTypeId,data.value("I2").toInt());
                        device->setStateValue(wallboxI3StateTypeId,data.value("I3").toInt());
                        device->setStateValue(wallboxU1StateTypeId,data.value("U1").toInt());
                        device->setStateValue(wallboxU2StateTypeId,data.value("U2").toInt());
                        device->setStateValue(wallboxU3StateTypeId,data.value("U3").toInt());

                        device->setStateValue(wallboxPStateTypeId,data.value("P").toInt());
                        device->setStateValue(wallboxEPStateTypeId,data.value("E pres").toInt()/10000.00);
                        device->setStateValue(wallboxTotalEnergyConsumedStateTypeId,data.value("E total").toInt()/10000.00);
                    }
                }
            }
        }

        // If the sender was unknown check the device if the IP address might have changed
        if (senderUnkown){
            qCDebug(dcKebaKeContact()) << "Unknown sender:" << sender.toString();

        }
    }
}

void DevicePluginKeba::discoveryFinished(const QList<Host> &hosts)
{
    qCDebug(dcKebaKeContact()) << "Discovery finished. Found" << hosts.count() << "devices";
    QList<DeviceDescriptor> discoveredDevices;
    foreach (const Host &host, hosts) {

        // check if IP is already added to another keba device
        bool deviceAddressUsed = false;
        foreach (Device *checkDevice, myDevices()) {
            if (checkDevice->paramValue(wallboxDeviceIpAddressParamTypeId).toString() == host.address()) {
                qCWarning(dcKebaKeContact()) << "Address already taken";
                deviceAddressUsed = true;
            }
        }
        if (host.hostName().contains("Keba") && !deviceAddressUsed) {
            DeviceDescriptor descriptor(wallboxDeviceClassId, ("KeContact (" + host.address() + ")"), host.macAddress());

            ParamList paramList;
            Param macAddress(wallboxDeviceMacAddressParamTypeId, host.macAddress());
            Param address(wallboxDeviceIpAddressParamTypeId, host.address());
            paramList.append(macAddress);
            paramList.append(address);
            descriptor.setParams(paramList);
            discoveredDevices.append(descriptor);
        }
    }
    emit devicesDiscovered(wallboxDeviceClassId, discoveredDevices);
}

void DevicePluginKeba::onConnectionChanged(bool status)
{
    KebaConnection *kebaConnection = qobject_cast<KebaConnection*>(sender());
    Device *device = m_kebaConnections.key(kebaConnection);
    device->setStateValue(wallboxConnectedStateTypeId, status);
}

void DevicePluginKeba::onSendData(const QByteArray &data)
{
    KebaConnection *kebaConnection = qobject_cast<KebaConnection*>(sender());
    qCDebug(dcKebaKeContact()) << "Sending data to:" << kebaConnection->getAddress() << data;
    m_kebaSocket->writeDatagram(data, kebaConnection->getAddress(), 7090);
}

void DevicePluginKeba::rediscoverDevice(Device *device)
{
    Q_UNUSED(device);
    //TODO rediscover device if IP address has changed
}
