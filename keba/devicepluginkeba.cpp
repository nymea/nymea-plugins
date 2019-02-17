/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *                                                                         *
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
    if(!m_pluginTimer) {
        m_pluginTimer = hardwareManager()->pluginTimerManager()->registerTimer(60);
        connect(m_pluginTimer, &PluginTimer::timeout, this, &DevicePluginKeba::updateData);
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

    // check if IP is already added to another keba device
    if(m_kebaDevices.keys().contains(address)){
        qCWarning(dcKebaKeContact()) << "Address already taken";
        return DeviceManager::DeviceSetupStatusFailure;
    }

    m_kebaDevices.insert(address, device);
    return DeviceManager::DeviceSetupStatusSuccess;
}

void DevicePluginKeba::postSetupDevice(Device *device)
{
    qCDebug(dcKebaKeContact()) << "Post setup" << device->name();
    displayMessage("nymea", device);
    getReport2(device);
    getReport3(device);
}

void DevicePluginKeba::deviceRemoved(Device *device)
{
    // Remove devices
    QHostAddress address = m_kebaDevices.key(device);
    m_kebaDevices.remove(address);

    if(m_kebaDevices.isEmpty()){
        m_kebaSocket->close();
        m_kebaSocket->deleteLater();
        qCDebug(dcKebaKeContact()) << "clear socket";
    }

    if (myDevices().isEmpty()) {
        hardwareManager()->pluginTimerManager()->unregisterTimer(m_pluginTimer);
        qCDebug(dcKebaKeContact()) << "device list empty, stopping timer";
    }
}

void DevicePluginKeba::updateData()
{
    foreach (Device *device, m_kebaDevices.values()) {
        getReport2(device);
        getReport3(device);
    }
}

DeviceManager::DeviceError DevicePluginKeba::executeAction(Device *device, const Action &action)
{
    qCDebug(dcKebaKeContact()) << "Execute action" << device->name() << action.actionTypeId().toString();

    if (device->deviceClassId() == wallboxDeviceClassId) {

        if(action.actionTypeId() == wallboxMaxChargingCurrentActionTypeId){

            int current = action.param(wallboxMaxChargingCurrentActionMaxChargingCurrentParamTypeId).value().toInt();
            setMaxAmpere(current, device);
            return DeviceManager::DeviceErrorNoError;
        }

        if(action.actionTypeId() == wallboxPowerActionTypeId) {
            bool state = action.param(wallboxPowerActionPowerParamTypeId).value().toBool();
            enableOutput(state, device);
            return DeviceManager::DeviceErrorNoError;
        }


        if(action.actionTypeId() == wallboxDisplayActionTypeId) {
            QByteArray message = action.param(wallboxDisplayActionMessageParamTypeId).value().toByteArray();
            displayMessage(message, device);
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

    while (socket->hasPendingDatagrams()) {
        datagram.resize(socket->pendingDatagramSize());
        socket->readDatagram(datagram.data(), datagram.size(), &sender, &senderPort);
        qCDebug(dcKebaKeContact()) << "Got message from" << sender.toString() << datagram;

        if(!m_kebaDevices.keys().contains(sender)){
            qCDebug(dcKebaKeContact()) << "Unknown sender:" << sender.toString();
            continue;
        }

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
                m_kebaDevices.value(sender)->setStateValue(wallboxConnectedStateTypeId,true);
                //activity state
                if(data.value("State").toString() == "0"){
                    m_kebaDevices.value(sender)->setStateValue(wallboxActivityStateTypeId,"Starting");
                }
                else if(data.value("State").toString() == "1"){
                    m_kebaDevices.value(sender)->setStateValue(wallboxActivityStateTypeId,"Not ready for charging");
                }
                else if(data.value("State").toString() == "2"){
                    m_kebaDevices.value(sender)->setStateValue(wallboxActivityStateTypeId,"Ready for charging");
                }
                else if(data.value("State").toString() == "3"){
                    m_kebaDevices.value(sender)->setStateValue(wallboxActivityStateTypeId,"Charging");
                }
                else if(data.value("State").toString() == "4"){
                    m_kebaDevices.value(sender)->setStateValue(wallboxActivityStateTypeId,"Error");
                }
                else if(data.value("State").toString() == "5"){
                    m_kebaDevices.value(sender)->setStateValue(wallboxActivityStateTypeId,"Authorization rejected");
                }
                // plug state
                if(data.value("Plug").toString() == "0"){
                    m_kebaDevices.value(sender)->setStateValue(wallboxPlugStateStateTypeId,"Unplugged");
                }
                else if(data.value("Plug").toString() == "1"){
                    m_kebaDevices.value(sender)->setStateValue(wallboxPlugStateStateTypeId,"Plugged on charging station");
                }
                else if(data.value("Plug").toString() == "3"){
                    m_kebaDevices.value(sender)->setStateValue(wallboxPlugStateStateTypeId,"Locked plug on charging station");
                }
                else if(data.value("Plug").toString() == "5"){
                    m_kebaDevices.value(sender)->setStateValue(wallboxPlugStateStateTypeId,"Plugged on charging station and vehicle");
                }
                else if(data.value("Plug").toString() == "7"){
                    m_kebaDevices.value(sender)->setStateValue(wallboxPlugStateStateTypeId,"Locked plug on charging station and vehicle");
                }
                //maximum current setting
                m_kebaDevices.value(sender)->setStateValue(wallboxMaxChargingCurrentStateTypeId,data.value("Curr user").toInt());
                m_kebaDevices.value(sender)->setStateValue(wallboxMaxChargingCurrentPercentStateTypeId,data.value("Max curr %").toInt()/10);
                m_kebaDevices.value(sender)->setStateValue(wallboxPowerStateTypeId,data.value("Enable sys").toBool());
            }
            else if(data.value("ID").toString() == "3"){

                qCDebug(dcKebaKeContact()) << "Report 3 reveiced";
                m_kebaDevices.value(sender)->setStateValue(wallboxI1StateTypeId,data.value("I1").toInt());
                m_kebaDevices.value(sender)->setStateValue(wallboxI2StateTypeId,data.value("I2").toInt());
                m_kebaDevices.value(sender)->setStateValue(wallboxI3StateTypeId,data.value("I3").toInt());
                m_kebaDevices.value(sender)->setStateValue(wallboxU1StateTypeId,data.value("U1").toInt());
                m_kebaDevices.value(sender)->setStateValue(wallboxU2StateTypeId,data.value("U2").toInt());
                m_kebaDevices.value(sender)->setStateValue(wallboxU3StateTypeId,data.value("U3").toInt());

                m_kebaDevices.value(sender)->setStateValue(wallboxPStateTypeId,data.value("P").toInt());
                m_kebaDevices.value(sender)->setStateValue(wallboxEPStateTypeId,data.value("E pres").toInt()/10000.00);
                m_kebaDevices.value(sender)->setStateValue(wallboxTotalEnergyConsumedStateTypeId,data.value("E total").toInt()/10000.00);
            }
        }
    }
}

void DevicePluginKeba::enableOutput(bool state, Device *device)
{
    // Print information that we are executing now the update action;
    QByteArray datagram;
    if(state){
        datagram.append("ena 1");
    }
    else{
        datagram.append("ena 0");
    }
    qCDebug(dcKebaKeContact()) << "Datagram : " << datagram;
    m_kebaSocket->writeDatagram(datagram.data(),datagram.size(), QHostAddress(device->paramValue(wallboxDeviceIpAddressParamTypeId).toString()) , 7090);
}

void  DevicePluginKeba::setMaxAmpere(int milliAmpere, Device *device)
{
    // Print information that we are executing now the update action
    qCDebug(dcKebaKeContact()) << "Update max current to : " << milliAmpere;
    QByteArray data;
    data.append("curr " + QVariant(milliAmpere).toByteArray());
    qCDebug(dcKebaKeContact()) << "send command: " << data;
    sendData(data, QHostAddress(device->paramValue(wallboxDeviceIpAddressParamTypeId).toString()));
}

void  DevicePluginKeba::displayMessage(const QByteArray &message, Device * device)
{
    /* Text shown on the display. Maximum 23 ASCII characters can be used. 0 .. 23 characters
    ~ == Σ
    $ == blank
    , == comma
    */

    qCDebug(dcKebaKeContact()) << "Set display message: " << message;
    QByteArray data;
    QByteArray modifiedMessage = message;
    modifiedMessage.replace(" ", "$");
    if (modifiedMessage.size() > 23) {
        modifiedMessage.resize(23);
    }
    data.append("display 0 0 0 0 " + modifiedMessage);
    qCDebug(dcKebaKeContact()) << "send command: " << data;
    sendData(data, QHostAddress(device->paramValue(wallboxDeviceIpAddressParamTypeId).toString()));
}


void  DevicePluginKeba::getDeviceInformation(Device *device)
{
    QByteArray data;
    data.append("report 2");
    qCDebug(dcKebaKeContact()) << "send command: " << data;
    sendData(data, QHostAddress(device->paramValue(wallboxDeviceIpAddressParamTypeId).toString()));
}

void  DevicePluginKeba::getReport1(Device *device)
{
    QByteArray data;
    data.append("report 2");
    qCDebug(dcKebaKeContact()) << "send command : " << data;
    sendData(data, QHostAddress(device->paramValue(wallboxDeviceIpAddressParamTypeId).toString()));
}

void  DevicePluginKeba::getReport2(Device *device)
{
    QByteArray data;
    data.append("report 2");
    qCDebug(dcKebaKeContact()) << "send command: " << data;
    sendData(data, QHostAddress(device->paramValue(wallboxDeviceIpAddressParamTypeId).toString()));
}

void  DevicePluginKeba::getReport3(Device *device)
{
    QByteArray data;
    data.append("report 3");
    qCDebug(dcKebaKeContact()) << "data: " << data;
    sendData(data, QHostAddress(device->paramValue(wallboxDeviceIpAddressParamTypeId).toString()));
}

void DevicePluginKeba::sendData(const QByteArray &data,const QHostAddress &address){
    //TODO add timeout counter

    m_kebaSocket->writeDatagram(data, address , 7090);
}

void DevicePluginKeba::discoveryFinished(const QList<Host> &hosts)
{
    qCDebug(dcKebaKeContact()) << "Discovery finished. Found" << hosts.count() << "devices";
    QList<DeviceDescriptor> discoveredDevices;
    foreach (const Host &host, hosts) {

        if (host.hostName().contains("Keba") && !m_kebaDevices.contains(QHostAddress(host.address()))) {
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

void DevicePluginKeba::rediscoverDevice(Device *device)
{
    Q_UNUSED(device);
    //TODO rediscover device if IP address has changed
}
