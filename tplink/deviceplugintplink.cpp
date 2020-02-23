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

#include "deviceplugintplink.h"
#include "plugininfo.h"

#include <network/networkaccessmanager.h>
#include <plugintimer.h>

#include <QNetworkReply>
#include <QJsonDocument>
#include <QTimer>
#include <QDataStream>

// https://github.com/softScheck/tplink-smartplug/blob/master/tplink-smarthome-commands.txt

DevicePluginTPLink::DevicePluginTPLink()
{
}

DevicePluginTPLink::~DevicePluginTPLink()
{
}

void DevicePluginTPLink::init()
{
    m_broadcastSocket = new QUdpSocket(this);

}

void DevicePluginTPLink::discoverDevices(DeviceDiscoveryInfo *info)
{
    QVariantMap map;
    QVariantMap getSysInfo;
    getSysInfo.insert("get_sysinfo", QVariant());
    map.insert("system", getSysInfo);
    QByteArray payload = QJsonDocument::fromVariant(map).toJson(QJsonDocument::Compact);
    QByteArray datagram = encryptPayload(payload);

    qint64 len = m_broadcastSocket->writeDatagram(datagram, QHostAddress::Broadcast, 9999);
    if (len != datagram.length()) {
        info->finish(Device::DeviceErrorHardwareFailure, QT_TR_NOOP("An error happened sending the discovery to the network."));
        return;
    }

    QTimer::singleShot(2000, info, [this, info](){
        while(m_broadcastSocket->hasPendingDatagrams()) {
            char buffer[1024];
            QHostAddress senderAddress;
            qint64 len = m_broadcastSocket->readDatagram(buffer, 1024, &senderAddress);
            QByteArray data = decryptPayload(QByteArray::fromRawData(buffer, len));
            QJsonParseError error;
            QJsonDocument jsonDoc = QJsonDocument::fromJson(data, &error);
            if (error.error != QJsonParseError::NoError) {
                qCWarning(dcTplink()) << "Error parsing JSON from device:" << data;
                continue;
            }
            QVariantMap properties = jsonDoc.toVariant().toMap();
            QVariantMap sysInfo = properties.value("system").toMap().value("get_sysinfo").toMap();

            qCWarning(dcTplink()) << qUtf8Printable(jsonDoc.toJson(QJsonDocument::Indented));

            QString deviceType = sysInfo.contains("type") ? sysInfo.value("type").toString() : sysInfo.value("mic_type").toString();
            if (sysInfo.value("type").toString() == "IOT.SMARTPLUGSWITCH") {

                DeviceDescriptor descriptor(kasaPlugDeviceClassId, sysInfo.value("alias").toString(), sysInfo.value("dev_name").toString());
                Param idParam = Param(kasaPlugDeviceIdParamTypeId, sysInfo.value("deviceId").toString());
                descriptor.setParams(ParamList() << idParam);
                Device *existingDevice = myDevices().findByParams(ParamList() << idParam);
                if (existingDevice) {
                    descriptor.setDeviceId(existingDevice->id());
                }
                info->addDeviceDescriptor(descriptor);

            } else {
                qCWarning(dcTplink()) << "Unhandled device type:\n" << qUtf8Printable(jsonDoc.toJson(QJsonDocument::Indented));
            }

        }
        info->finish(Device::DeviceErrorNoError);
    });
}

void DevicePluginTPLink::setupDevice(DeviceSetupInfo *info)
{
    QVariantMap map;
    QVariantMap getSysInfo;
    getSysInfo.insert("get_sysinfo", QVariant());
    map.insert("system", getSysInfo);
    QVariantMap getRealTime;
    getRealTime.insert("get_realtime", QVariant());
    map.insert("emeter", getRealTime);
    QByteArray payload = QJsonDocument::fromVariant(map).toJson(QJsonDocument::Compact);
    QByteArray datagram = encryptPayload(payload);

    qint64 len = m_broadcastSocket->writeDatagram(datagram, QHostAddress::Broadcast, 9999);
    if (len != datagram.length()) {
        info->finish(Device::DeviceErrorHardwareFailure, QT_TR_NOOP("An error happened finding the device in the network."));
        return;
    }

    QTimer::singleShot(2000, info, [this, info](){

        while(m_broadcastSocket->hasPendingDatagrams()) {
            char buffer[1024];
            QHostAddress senderAddress;
            qint64 len = m_broadcastSocket->readDatagram(buffer, 1024, &senderAddress);
            QByteArray data = decryptPayload(QByteArray::fromRawData(buffer, len));
            QJsonParseError error;
            QJsonDocument jsonDoc = QJsonDocument::fromJson(data, &error);
            if (error.error != QJsonParseError::NoError) {
                qCWarning(dcTplink()) << "Error parsing JSON from device:" << data;
                continue;
            }
            QVariantMap properties = jsonDoc.toVariant().toMap();
            QVariantMap sysInfo = properties.value("system").toMap().value("get_sysinfo").toMap();
            if (info->device()->paramValue(kasaPlugDeviceIdParamTypeId).toString() == sysInfo.value("deviceId").toString()) {
                qCDebug(dcTplink()) << "Found device at" << senderAddress;

                connectToDevice(info->device(), senderAddress);

                info->finish(Device::DeviceErrorNoError);
                m_setupRetries.remove(info);
                return;
            }
        }

        if (!m_setupRetries.contains(info) || m_setupRetries.value(info) < 5) {
            qCDebug(dcTplink()) << "Device not found in network. Retrying... (" << m_setupRetries[info] << ")";
            m_setupRetries[info]++;
            setupDevice(info);
            return;
        }
        m_setupRetries.remove(info);
        info->finish(Device::DeviceErrorDeviceNotFound, QT_TR_NOOP("The device could not be found on the network."));
    });
}

void DevicePluginTPLink::postSetupDevice(Device *device)
{
    connect(device, &Device::nameChanged, this, [this, device](){
        QVariantMap map;
        QVariantMap systemMap;
        QVariantMap aliasMap;
        aliasMap.insert("alias", device->name());
        systemMap.insert("set_dev_alias", aliasMap);
        map.insert("system", systemMap);
        QByteArray payload = QJsonDocument::fromVariant(map).toJson(QJsonDocument::Compact);
        qCDebug(dcTplink) << "Setting device name:" << payload;
        payload = encryptPayload(payload);
        QByteArray data;
        QDataStream stream(&data, QIODevice::ReadWrite);
        stream << static_cast<quint32>(payload.length());
        data.append(payload);

        Job job;
        job.id = m_jobIdx++;
        job.data = data;
        m_jobQueue[device].append(job);
        processQueue(device);
    });

    if (!m_timer) {
        m_timer = hardwareManager()->pluginTimerManager()->registerTimer(1);
        connect(m_timer, &PluginTimer::timeout, this, [this](){
            foreach (Device *d, myDevices()) {
                if (!m_pendingJobs.contains(d) && m_jobQueue[d].isEmpty()) {
                    fetchState(d);
                }
            }
        });
    }
}

void DevicePluginTPLink::deviceRemoved(Device *device)
{
    qCDebug(dcTplink()) << "Device removed" << device->name();
    m_sockets.remove(device);
    m_pendingJobs.remove(device);
    m_jobQueue.remove(device);

    if (myDevices().isEmpty() && m_timer) {
        hardwareManager()->pluginTimerManager()->unregisterTimer(m_timer);
        m_timer = nullptr;
    }
}

void DevicePluginTPLink::executeAction(DeviceActionInfo *info)
{
    QVariantMap map;
    QVariantMap systemMap;
    QVariantMap powerMap;
    powerMap.insert("state", info->action().param(kasaPlugPowerActionPowerParamTypeId).value().toBool() ? 1 : 0);
    systemMap.insert("set_relay_state", powerMap);
    map.insert("system", systemMap);

    qCDebug(dcTplink()) << "Executing action" << qUtf8Printable(QJsonDocument::fromVariant(map).toJson(QJsonDocument::Compact));

    QByteArray payload = encryptPayload(QJsonDocument::fromVariant(map).toJson(QJsonDocument::Compact));
    QByteArray data;
    QDataStream stream(&data, QIODevice::ReadWrite);
    stream << static_cast<quint32>(payload.length());
    data.append(payload);

    Job job;
    job.id = m_jobIdx++;
    job.data = data;
    job.actionInfo = info;
    m_jobQueue[info->device()].append(job);
    connect(info, &DeviceActionInfo::aborted, this, [=](){
        m_jobQueue[info->device()].removeAll(job);
    });

    // Directly queue up fetchState
    fetchState(info->device(), info);

    processQueue(info->device());
}

QByteArray DevicePluginTPLink::encryptPayload(const QByteArray &payload)
{
    QByteArray result;
    int k = 171;
    for (int i = 0; i < payload.length(); i++){
        char t = payload.at(i) xor k;
        k = t;
        result.append(t);
    }
    return result;
}

QByteArray DevicePluginTPLink::decryptPayload(const QByteArray &payload)
{
    QByteArray result;
    int k = 171;
    for (int i = 0; i < payload.length(); i++){
        char t = payload.at(i);
        result.append(t xor k);
        k = t;
    }
    return result;
}

void DevicePluginTPLink::connectToDevice(Device *device, const QHostAddress &address)
{
    if (m_sockets.contains(device)) {
        qCWarning(dcTplink) << "Already have a connection to this device";
        return;
    }
    qCDebug(dcTplink()) << "Connecting to" << address;

    QTcpSocket *socket = new QTcpSocket(this);
    m_sockets.insert(device, socket);

    connect(socket, &QTcpSocket::connected, device, [this, device, address] () {
        qCDebug(dcTplink()) << "Connected to device" << address;
        device->setStateValue(kasaPlugConnectedStateTypeId, true);
        fetchState(device);
    });

    typedef void (QTcpSocket:: *errorSignal)(QAbstractSocket::SocketError);
    connect(socket, static_cast<errorSignal>(&QTcpSocket::error), device, [](QAbstractSocket::SocketError error) {
        qCWarning(dcTplink()) << "Error in device connection:" << error;
    });

    connect(socket, &QTcpSocket::readyRead, device, [this, socket, device](){
        m_inputBuffers[device].append(socket->readAll());
        while (m_inputBuffers[device].length() > 4) {
            QByteArray data = m_inputBuffers[device];
            QDataStream stream(data);
            qint32 len;
            stream >> len;
            data.remove(0, 4);
            if (data.length() < len) {
                // Buffer not complete... wait for more...
                return;
            }
            QByteArray payload = data.left(len);
            data.remove(0, len);
            m_inputBuffers[device] = data;

            if (!m_pendingJobs.contains(device)) {
                qCWarning(dcTplink()) << "Received packet from device but don't have a job waiting for it. Did it time out?";
                processQueue(device);
                return;
            }
            Job job = m_pendingJobs.take(device);

            QJsonParseError error;
            QJsonDocument jsonDoc = QJsonDocument::fromJson(decryptPayload(payload), &error);
            if (error.error != QJsonParseError::NoError) {
                qCWarning(dcTplink()) << "Cannot parse json from device:" << decryptPayload(payload);
                m_jobQueue[device].prepend(job);
                socket->disconnectFromHost();
                return;
            }
            qCDebug(dcTplink()) << "Socket data received" << qUtf8Printable(jsonDoc.toJson());

            QVariantMap map = jsonDoc.toVariant().toMap();
            if (map.contains("system")) {
                QVariantMap systemMap = map.value("system").toMap();
                if (systemMap.contains("set_relay_state")) {
                    int err_code = systemMap.value("set_relay_state").toMap().value("err_code").toInt();
                    if (err_code != 0) {
                        qCWarning(dcTplink()) << "Set relay state failed:" << qUtf8Printable(jsonDoc.toJson());
                        if (job.actionInfo) {
                            job.actionInfo->finish(Device::DeviceErrorHardwareFailure);
                        }
                    }
                }
                if (systemMap.contains("get_sysinfo")) {
                    int relayState = systemMap.value("get_sysinfo").toMap().value("relay_state").toInt();
                    device->setStateValue(kasaPlugPowerStateTypeId, relayState == 1 ? true : false);

                    QString alias = systemMap.value("get_sysinfo").toMap().value("alias").toString();
                    if (device->name() != alias) {
                        device->setName(alias);
                    }

                    if (job.actionInfo) {
                        job.actionInfo->finish(Device::DeviceErrorNoError);
                    }
                }
            }
            if (map.contains("emeter")) {
                QVariantMap emeterMap = map.value("emeter").toMap();
                if (emeterMap.contains("get_realtime")) {
                    // This has quite a bit of jitter... Let's smoothen it while within +/- 0.1W to produce less events in the system
                    double oldValue = device->stateValue(kasaPlugCurrentPowerStateTypeId).toDouble();
                    double newValue = emeterMap.value("get_realtime").toMap().value("power_mw").toDouble() / 1000;
                    if (qAbs(oldValue - newValue) > 0.1) {
                        device->setStateValue(kasaPlugCurrentPowerStateTypeId, newValue);
                    }
                    device->setStateValue(kasaPlugTotalEnergyConsumedStateTypeId, emeterMap.value("get_realtime").toMap().value("total_wh").toDouble() / 1000);
                }
            }

            processQueue(device);
        }
    });

    connect(socket, &QTcpSocket::disconnected, device, [this, device, address](){
        qCDebug(dcTplink()) << "Device disconnected";
        m_sockets.take(device)->deleteLater();
        if (m_pendingJobs.contains(device)) {
            // Putting active job back to queue
            m_jobQueue[device].prepend(m_pendingJobs.take(device));
        }
        device->setStateValue(kasaPlugConnectedStateTypeId, false);
        QTimer::singleShot(500, device, [this, device, address]() {connectToDevice(device, address);});
    });

    socket->connectToHost(address.toString(), 9999, QIODevice::ReadWrite);
}

void DevicePluginTPLink::fetchState(Device *device, DeviceActionInfo *info)
{
    QTcpSocket *socket = m_sockets.value(device);
    if (!socket || !socket->isOpen()) {
        qCWarning(dcTplink()) << "Cannot fetch state";
    }

    QVariantMap map;
    QVariantMap getSysInfo;
    getSysInfo.insert("get_sysinfo", QVariant());
    map.insert("system", getSysInfo);
    QVariantMap getRealTime;
    getRealTime.insert("get_realtime", QVariant());
    map.insert("emeter", getRealTime);
    QByteArray plaintext = QJsonDocument::fromVariant(map).toJson(QJsonDocument::Compact);
    qCDebug(dcTplink()) << "Fetching device state";
    QByteArray payload = encryptPayload(plaintext);
    QByteArray data;
    QDataStream stream(&data, QIODevice::ReadWrite);
    stream << static_cast<quint32>(payload.length());
    data.append(payload);

    Job job;
    job.id = m_jobIdx++;
    job.data = data;
    job.actionInfo = info;
    m_jobQueue[device].append(job);

    processQueue(device);

}

void DevicePluginTPLink::processQueue(Device *device)
{
    if (m_pendingJobs.contains(device)) {
        // Busy
        return;
    }
    if (m_jobQueue[device].isEmpty()) {
        // No jobs queued for this device
        return;
    }

    QTcpSocket *socket = m_sockets.value(device);
    if (!socket) {
        qCWarning(dcTplink()) << "Cannot process queue. Device not connected.";
        return;
    }
    Job job = m_jobQueue[device].takeFirst();

    m_pendingJobs[device] = job;

    qint64 len = socket->write(job.data);
    if (len != job.data.length()) {
        qCWarning(dcTplink()) << "Error writing data to network.";
        if (job.actionInfo) {
            job.actionInfo->finish(Device::DeviceErrorHardwareFailure, QT_TR_NOOP("Error sending command to the network."));
        }
        socket->disconnectFromHost();
        return;
    }
}

