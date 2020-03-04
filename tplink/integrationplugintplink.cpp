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

#include "integrationplugintplink.h"
#include "plugininfo.h"

#include <network/networkaccessmanager.h>
#include <plugintimer.h>

#include <QNetworkReply>
#include <QJsonDocument>
#include <QTimer>
#include <QDataStream>

// https://github.com/softScheck/tplink-smartplug/blob/master/tplink-smarthome-commands.txt

IntegrationPluginTPLink::IntegrationPluginTPLink()
{
    m_idParamTypesMap[kasaPlug100ThingClassId] = kasaPlug100ThingIdParamTypeId;
    m_idParamTypesMap[kasaPlug110ThingClassId] = kasaPlug110ThingIdParamTypeId;
    m_idParamTypesMap[kasaSwitch200ThingClassId] = kasaSwitch200ThingIdParamTypeId;

    m_connectedStateTypesMap[kasaPlug100ThingClassId] = kasaPlug100ConnectedStateTypeId;
    m_connectedStateTypesMap[kasaPlug110ThingClassId] = kasaPlug110ConnectedStateTypeId;
    m_connectedStateTypesMap[kasaSwitch200ThingClassId] = kasaSwitch200ConnectedStateTypeId;

    m_powerStatetTypesMap[kasaPlug100ThingClassId] = kasaPlug100PowerStateTypeId;
    m_powerStatetTypesMap[kasaPlug110ThingClassId] = kasaPlug110PowerStateTypeId;
    m_powerStatetTypesMap[kasaSwitch200ThingClassId] = kasaSwitch200PowerStateTypeId;

    m_currentPowerStatetTypesMap[kasaPlug110ThingClassId] = kasaPlug110CurrentPowerStateTypeId;

    m_totalEnergyConsumedStatetTypesMap[kasaPlug110ThingClassId] = kasaPlug110TotalEnergyConsumedStateTypeId;

    m_powerActionParamTypesMap[kasaPlug100ThingClassId] = kasaPlug100PowerActionPowerParamTypeId;
    m_powerActionParamTypesMap[kasaPlug110ThingClassId] = kasaPlug110PowerActionPowerParamTypeId;
    m_powerActionParamTypesMap[kasaSwitch200ThingClassId] = kasaSwitch200PowerActionPowerParamTypeId;
}

IntegrationPluginTPLink::~IntegrationPluginTPLink()
{
}

void IntegrationPluginTPLink::init()
{
    m_broadcastSocket = new QUdpSocket(this);

}

void IntegrationPluginTPLink::discoverThings(ThingDiscoveryInfo *info)
{
    QVariantMap map;
    QVariantMap getSysInfo;
    getSysInfo.insert("get_sysinfo", QVariant());
    map.insert("system", getSysInfo);
    QByteArray payload = QJsonDocument::fromVariant(map).toJson(QJsonDocument::Compact);
    QByteArray datagram = encryptPayload(payload);

    qint64 len = m_broadcastSocket->writeDatagram(datagram, QHostAddress::Broadcast, 9999);
    if (len != datagram.length()) {
        info->finish(Thing::ThingErrorHardwareFailure, QT_TR_NOOP("An error happened sending the discovery to the network."));
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
                qCWarning(dcTplink()) << "Error parsing JSON from thing:" << data;
                continue;
            }
            QVariantMap properties = jsonDoc.toVariant().toMap();
            QVariantMap sysInfo = properties.value("system").toMap().value("get_sysinfo").toMap();

            qCWarning(dcTplink()) << qUtf8Printable(jsonDoc.toJson(QJsonDocument::Indented));

            QRegExp modelFilter;
            if (info->thingClassId() == kasaPlug100ThingClassId) {
                modelFilter = QRegExp("(HS100|HS103|HS105|KP100).*");
            } else if (info->thingClassId() == kasaPlug110ThingClassId) {
                modelFilter = QRegExp("HS110.*");
            } else if (info->thingClassId() == kasaSwitch200ThingClassId) {
                modelFilter = QRegExp("HS200.*");
            }
            QString model = sysInfo.value("model").toString();

            if (modelFilter.exactMatch(model)) {
                ThingDescriptor descriptor(info->thingClassId(), sysInfo.value("alias").toString(), sysInfo.value("dev_name").toString());
                Param idParam = Param(m_idParamTypesMap.value(info->thingClassId()), sysInfo.value("deviceId").toString());
                descriptor.setParams(ParamList() << idParam);
                Thing *existingThing = myThings().findByParams(ParamList() << idParam);
                if (existingThing) {
                    descriptor.setThingId(existingThing->id());
                }
                info->addThingDescriptor(descriptor);
            } else {
                qCWarning(dcTplink()) << "Ignoring not matching device type:\n" << qUtf8Printable(jsonDoc.toJson(QJsonDocument::Indented));
            }

        }
        info->finish(Thing::ThingErrorNoError);
    });
}

void IntegrationPluginTPLink::setupThing(ThingSetupInfo *info)
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
        info->finish(Thing::ThingErrorHardwareFailure, QT_TR_NOOP("An error happened finding the device in the network."));
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
                qCWarning(dcTplink()) << "Error parsing JSON from thing:" << data;
                continue;
            }
            QVariantMap properties = jsonDoc.toVariant().toMap();
            QVariantMap sysInfo = properties.value("system").toMap().value("get_sysinfo").toMap();
            if (info->thing()->paramValue(m_idParamTypesMap.value(info->thing()->thingClassId())).toString() == sysInfo.value("deviceId").toString()) {
                qCDebug(dcTplink()) << "Found thing at" << senderAddress;

                connectToDevice(info->thing(), senderAddress);

                info->finish(Thing::ThingErrorNoError);
                m_setupRetries.remove(info);
                return;
            }
        }

        if (!m_setupRetries.contains(info) || m_setupRetries.value(info) < 5) {
            qCDebug(dcTplink()) << "Device not found in network. Retrying... (" << m_setupRetries[info] << ")";
            m_setupRetries[info]++;
            setupThing(info);
            return;
        }
        m_setupRetries.remove(info);
        info->finish(Thing::ThingErrorThingNotFound, QT_TR_NOOP("The device could not be found on the network."));
    });
}

void IntegrationPluginTPLink::postSetupThing(Thing *thing)
{
    connect(thing, &Thing::nameChanged, this, [this, thing](){
        QVariantMap map;
        QVariantMap systemMap;
        QVariantMap aliasMap;
        aliasMap.insert("alias", thing->name());
        systemMap.insert("set_dev_alias", aliasMap);
        map.insert("system", systemMap);
        QByteArray payload = QJsonDocument::fromVariant(map).toJson(QJsonDocument::Compact);
        qCDebug(dcTplink) << "Setting thing name:" << payload;
        payload = encryptPayload(payload);
        QByteArray data;
        QDataStream stream(&data, QIODevice::ReadWrite);
        stream << static_cast<quint32>(payload.length());
        data.append(payload);

        Job job;
        job.id = m_jobIdx++;
        job.data = data;
        m_jobQueue[thing].append(job);
        processQueue(thing);
    });

    if (!m_timer) {
        m_timer = hardwareManager()->pluginTimerManager()->registerTimer(1);
        connect(m_timer, &PluginTimer::timeout, this, [this](){
            foreach (Thing *d, myThings()) {
                if (!m_pendingJobs.contains(d) && m_jobQueue[d].isEmpty()) {
                    fetchState(d);
                }
            }
        });
    }
}

void IntegrationPluginTPLink::thingRemoved(Thing *thing)
{
    qCDebug(dcTplink()) << "Device removed" << thing->name();
    m_sockets.remove(thing);
    m_pendingJobs.remove(thing);
    m_jobQueue.remove(thing);

    if (myThings().isEmpty() && m_timer) {
        hardwareManager()->pluginTimerManager()->unregisterTimer(m_timer);
        m_timer = nullptr;
    }
}

void IntegrationPluginTPLink::executeAction(ThingActionInfo *info)
{
    QVariantMap map;
    QVariantMap systemMap;
    QVariantMap powerMap;
    ParamTypeId powerActionParamTypeId = m_powerActionParamTypesMap.value(info->thing()->thingClassId());
    powerMap.insert("state", info->action().param(powerActionParamTypeId).value().toBool() ? 1 : 0);
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
    m_jobQueue[info->thing()].append(job);
    connect(info, &ThingActionInfo::aborted, this, [=](){
        m_jobQueue[info->thing()].removeAll(job);
    });

    // Directly queue up fetchState
    fetchState(info->thing(), info);

    processQueue(info->thing());
}

QByteArray IntegrationPluginTPLink::encryptPayload(const QByteArray &payload)
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

QByteArray IntegrationPluginTPLink::decryptPayload(const QByteArray &payload)
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

void IntegrationPluginTPLink::connectToDevice(Thing *thing, const QHostAddress &address)
{
    if (m_sockets.contains(thing)) {
        qCWarning(dcTplink) << "Already have a connection to this device";
        return;
    }
    qCDebug(dcTplink()) << "Connecting to" << address;

    QTcpSocket *socket = new QTcpSocket(this);
    m_sockets.insert(thing, socket);

    connect(socket, &QTcpSocket::connected, thing, [this, thing, address] () {
        qCDebug(dcTplink()) << "Connected to device" << address;
        StateTypeId connectedStateTypeId = m_connectedStateTypesMap.value(thing->thingClassId());
        thing->setStateValue(connectedStateTypeId, true);
        fetchState(thing);
    });

    typedef void (QTcpSocket:: *errorSignal)(QAbstractSocket::SocketError);
    connect(socket, static_cast<errorSignal>(&QTcpSocket::error), thing, [](QAbstractSocket::SocketError error) {
        qCWarning(dcTplink()) << "Error in device connection:" << error;
    });

    connect(socket, &QTcpSocket::readyRead, thing, [this, socket, thing](){
        m_inputBuffers[thing].append(socket->readAll());
        while (m_inputBuffers[thing].length() > 4) {
            QByteArray data = m_inputBuffers[thing];
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
            m_inputBuffers[thing] = data;

            if (!m_pendingJobs.contains(thing)) {
                qCWarning(dcTplink()) << "Received packet from thing but don't have a job waiting for it. Did it time out?";
                processQueue(thing);
                return;
            }
            Job job = m_pendingJobs.take(thing);

            QJsonParseError error;
            QJsonDocument jsonDoc = QJsonDocument::fromJson(decryptPayload(payload), &error);
            if (error.error != QJsonParseError::NoError) {
                qCWarning(dcTplink()) << "Cannot parse json from device:" << decryptPayload(payload);
                m_jobQueue[thing].prepend(job);
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
                            job.actionInfo->finish(Thing::ThingErrorHardwareFailure);
                        }
                    }
                }
                if (systemMap.contains("get_sysinfo")) {
                    int relayState = systemMap.value("get_sysinfo").toMap().value("relay_state").toInt();
                    StateTypeId powerStateTypeId = m_powerStatetTypesMap.value(thing->thingClassId());
                    thing->setStateValue(powerStateTypeId, relayState == 1 ? true : false);

                    QString alias = systemMap.value("get_sysinfo").toMap().value("alias").toString();
                    if (thing->name() != alias) {
                        thing->setName(alias);
                    }

                    if (job.actionInfo) {
                        job.actionInfo->finish(Thing::ThingErrorNoError);
                    }
                }
            }
            if (map.contains("emeter")) {
                QVariantMap emeterMap = map.value("emeter").toMap();
                if (emeterMap.contains("get_realtime")) {
                    // This has quite a bit of jitter... Let's smoothen it while within +/- 0.1W to produce less events in the system
                    StateTypeId currentPowerStateTypeId = m_currentPowerStatetTypesMap.value(thing->thingClassId());
                    double oldValue = thing->stateValue(currentPowerStateTypeId).toDouble();
                    double newValue = emeterMap.value("get_realtime").toMap().value("power_mw").toDouble() / 1000;
                    if (qAbs(oldValue - newValue) > 0.1) {
                        thing->setStateValue(currentPowerStateTypeId, newValue);
                    }
                    StateTypeId totalEnergyConsumedStateTypeId = m_totalEnergyConsumedStatetTypesMap.value(thing->thingClassId());
                    thing->setStateValue(totalEnergyConsumedStateTypeId, emeterMap.value("get_realtime").toMap().value("total_wh").toDouble() / 1000);
                }
            }

            processQueue(thing);
        }
    });

    connect(socket, &QTcpSocket::disconnected, thing, [this, thing, address](){
        qCDebug(dcTplink()) << "Device disconnected";
        m_sockets.take(thing)->deleteLater();
        if (m_pendingJobs.contains(thing)) {
            // Putting active job back to queue
            m_jobQueue[thing].prepend(m_pendingJobs.take(thing));
        }
        StateTypeId connectedStateTypeId = m_connectedStateTypesMap.value(thing->thingClassId());
        thing->setStateValue(connectedStateTypeId, false);
        QTimer::singleShot(500, thing, [this, thing, address]() {connectToDevice(thing, address);});
    });

    socket->connectToHost(address.toString(), 9999, QIODevice::ReadWrite);
}

void IntegrationPluginTPLink::fetchState(Thing *thing, ThingActionInfo *info)
{
    QTcpSocket *socket = m_sockets.value(thing);
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
    m_jobQueue[thing].append(job);

    processQueue(thing);

}

void IntegrationPluginTPLink::processQueue(Thing *thing)
{
    if (m_pendingJobs.contains(thing)) {
        // Busy
        return;
    }
    if (m_jobQueue[thing].isEmpty()) {
        // No jobs queued for this thing
        return;
    }

    QTcpSocket *socket = m_sockets.value(thing);
    if (!socket) {
        qCWarning(dcTplink()) << "Cannot process queue. Device not connected.";
        return;
    }
    Job job = m_jobQueue[thing].takeFirst();

    m_pendingJobs[thing] = job;

    qint64 len = socket->write(job.data);
    if (len != job.data.length()) {
        qCWarning(dcTplink()) << "Error writing data to network.";
        if (job.actionInfo) {
            job.actionInfo->finish(Thing::ThingErrorHardwareFailure, QT_TR_NOOP("Error sending command to the network."));
        }
        socket->disconnectFromHost();
        return;
    }
}

