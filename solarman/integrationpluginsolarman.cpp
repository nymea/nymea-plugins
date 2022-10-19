/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*
* Copyright 2013 - 2022, nymea GmbH
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


#include "integrationpluginsolarman.h"
#include "plugininfo.h"

#include <plugintimer.h>
#include <network/networkdevicediscovery.h>
#include <network/networkaccessmanager.h>
#include <network/networkdevicediscoveryreply.h>

#include <QNetworkReply>
#include <QAuthenticator>
#include <QUrlQuery>
#include <QJsonDocument>
#include <QMetaEnum>
#include <QUdpSocket>
#include <QFile>
#include <QDir>

#include "solarmandiscovery.h"
#include "solarmanmodbus.h"
#include "solarmanmodbusreply.h"

IntegrationPluginSolarman::IntegrationPluginSolarman()
{
    QFile registerMappings(":/registermappings.json");
    registerMappings.open(QFile::ReadOnly);
    QJsonDocument jsonDoc = QJsonDocument::fromJson(registerMappings.readAll());
    m_registerMappings = jsonDoc.toVariant().toMap();
}

IntegrationPluginSolarman::~IntegrationPluginSolarman()
{
}

void IntegrationPluginSolarman::discoverThings(ThingDiscoveryInfo *info)
{
    SolarmanDiscovery *discovery = new SolarmanDiscovery(info);
    connect(discovery, &SolarmanDiscovery::discoveryResults, info, [this, info](const QList<SolarmanDiscovery::DiscoveryResult> &results) {
        foreach (const SolarmanDiscovery::DiscoveryResult &result, results) {
            ThingDescriptor descriptor(deyeStringThingClassId, "Deye String Inverter", "Serial: " + result.serial + ", IP: " + result.ip.toString());
            descriptor.setParams({{deyeStringThingSerialParamTypeId, result.serial}});

            if (myThings().findByParams(descriptor.params())) {
                descriptor.setThingId(myThings().findByParams(descriptor.params())->id());
            }
            info->addThingDescriptor(descriptor);
        }
        info->finish(Thing::ThingErrorNoError);
    });
    discovery->discover();
}

void IntegrationPluginSolarman::setupThing(ThingSetupInfo *info)
{
    Thing *thing = info->thing();

    SolarmanDiscovery *monitor = m_monitors.value(thing);
    if (!monitor) {
        monitor = new SolarmanDiscovery(thing);
        m_monitors.insert(thing, monitor);
    }

    if (!monitor->monitor(thing->paramValue(deyeStringThingSerialParamTypeId).toString())) {
        info->finish(Thing::ThingErrorHardwareFailure, QT_TR_NOOP("Unalbe to open network connection."));
        m_monitors.take(thing)->deleteLater();
        return;
    }

    SolarmanModbus *modbus = new SolarmanModbus(thing);
    m_connections.insert(thing, modbus);

    // For testing
//    modbus->connectToHost(monitor->monitorResult().ip, 8899, monitor->monitorResult().serial);
//    pollDevice(thing);

    connect(monitor, &SolarmanDiscovery::monitorStateChanged, thing, [modbus, monitor](const SolarmanDiscovery::DiscoveryResult &result) {
        if (result.online && !modbus->isConnected()) {
            modbus->connectToHost(monitor->monitorResult().ip, 8899, monitor->monitorResult().serial);
        }
    });

    connect(modbus, &SolarmanModbus::connectedChanged, thing, [this, thing, monitor, modbus](bool connected){
        thing->setStateValue(deyeStringConnectedStateTypeId, connected);
        if (connected) {
            pollDevice(thing);
            m_timers.value(thing)->reset();
            m_timers.value(thing)->start();
        } else {
            m_timers.value(thing)->stop();
            if (monitor->monitorResult().online) {
                qCDebug(dcSolarman()) << "Reconnecting to solarman inverter";
                modbus->connectToHost(monitor->monitorResult().ip, 8899, monitor->monitorResult().serial);
            }
        }
    });

    PluginTimer *timer = m_timers.take(thing);
    if (!timer) {
        // the inverter updates its internal values once per minute...
        timer = hardwareManager()->pluginTimerManager()->registerTimer(21);
        m_timers.insert(thing, timer);

        connect(timer, &PluginTimer::timeout, thing, [this, thing](){
            pollDevice(thing);
        });
    }

    info->finish(Thing::ThingErrorNoError);
}

void IntegrationPluginSolarman::thingRemoved(Thing *thing)
{
    m_monitors.remove(thing);
    m_connections.remove(thing);
    hardwareManager()->pluginTimerManager()->unregisterTimer(m_timers.take(thing));
}

void IntegrationPluginSolarman::pollDevice(Thing *thing)
{
    SolarmanModbus *modbus = m_connections.value(thing);
    if (!modbus->isConnected()) {
        qCDebug(dcSolarman()) << "Inverter offline. Not polling.";
        return;
    }

    QVariantMap mapping = m_registerMappings.value(thing->thingClass().name()).toMap();
    quint8 slaveId = mapping.value("slaveId").toUInt();
    quint16 startRegister = mapping.value("startRegister").toUInt();
    quint16 endRegister = mapping.value("endRegister").toUInt();
    QMetaEnum fcEnum = QMetaEnum::fromType<SolarmanModbus::FunctionCode>();
    SolarmanModbus::FunctionCode functionCode = static_cast<SolarmanModbus::FunctionCode>(fcEnum.keyToValue(mapping.value("functionCode").toByteArray()));
    QVariantList registers = mapping.value("registers").toList();


    SolarmanModbusReply *reply = modbus->readRegisters(slaveId, startRegister, endRegister, functionCode);
    qCDebug(dcSolarman()) << "Polling inverter" << thing->name() << "slaveID:" << slaveId << "Start:" << startRegister << "End:" << endRegister << "Request ID" << reply->requestId();
    connect(reply, &SolarmanModbusReply::finished, thing, [modbus, thing, reply, registers](bool success){
        if (!success) {
            qCWarning(dcSolarman()) << "Polling failed..." << reply->requestId();
            modbus->disconnectFromHost();
            return;
        }
        qCDebug(dcSolarman()) << "modbus reply for" << thing->name() << reply->requestId();

        foreach (const QVariant &registerVariant, registers) {
            QVariantMap definition = registerVariant.toMap();
            quint16 reg = definition.value("index").toUInt();
            quint8 length = definition.value("length").toUInt();
            QString type = definition.value("type").toString();
            QString stateName = definition.value("state").toString();
            QString registerName = definition.value("name").toString();
            int offset = definition.value("offset", 0).toInt();
            double scale = definition.value("scale", 1).toDouble();


            QVariant value;
            if (type == "uint16") {
                double val = reply->readRegister16(reg);
                val += offset;
                val *= scale;
                value = val;

            } else if (type == "uint32") {
                double val = reply->readRegister32(reg);
                val += offset;
                val *= scale;
                value = val;
            } else if (type == "string") {
                value = reply->readRegisterString(reg, length);
            }
            qCDebug(dcSolarman()) << "Register" << reg << registerName << ":" << value;
            if (!stateName.isEmpty()) {
                thing->setStateValue(stateName, value);
            }
        }


//        qCDebug(dcSolarman()) << "Today's production:" << reply->readRegister16(0x003C);
//        qCDebug(dcSolarman()) << "Inverter status:" << reply->readRegister16(0x003B);
//        qCDebug(dcSolarman()) << "Total production:" << reply->readRegister32(0x003F);
    });

}
