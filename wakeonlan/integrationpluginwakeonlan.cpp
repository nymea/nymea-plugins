// SPDX-License-Identifier: GPL-3.0-or-later

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*
* Copyright (C) 2013 - 2024, nymea GmbH
* Copyright (C) 2024 - 2025, chargebyte austria GmbH
*
* This file is part of nymea-plugins.
*
* nymea-plugins is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* (at your option) any later version.
*
* nymea-plugins is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
* General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with nymea-plugins. If not, see <https://www.gnu.org/licenses/>.
*
* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

#include "integrationpluginwakeonlan.h"
#include "plugininfo.h"

#include <integrations/thing.h>
#include <network/networkdevicediscovery.h>

#include <QDebug>
#include <QProcess>
#include <QStringList>
#include <QUdpSocket>

IntegrationPluginWakeOnLan::IntegrationPluginWakeOnLan()
{
}

void IntegrationPluginWakeOnLan::discoverThings(ThingDiscoveryInfo *info)
{
    if (!hardwareManager()->networkDeviceDiscovery()->available()) {
        qCWarning(dcWakeOnLan()) << "Failed to discover network devices. The network device discovery is not available.";
        info->finish(Thing::ThingErrorHardwareNotAvailable, QT_TR_NOOP("Unable to discovery devices in your network."));
        return;
    }

    qCDebug(dcWakeOnLan()) << "Starting network discovery...";
    NetworkDeviceDiscoveryReply *discoveryReply = hardwareManager()->networkDeviceDiscovery()->discover();
    connect(discoveryReply, &NetworkDeviceDiscoveryReply::finished, discoveryReply, &NetworkDeviceDiscoveryReply::deleteLater);
    connect(discoveryReply, &NetworkDeviceDiscoveryReply::finished, this, [=](){
        qCDebug(dcWakeOnLan()) << "Discovery finished. Found" << discoveryReply->networkDeviceInfos().count() << "devices";
        foreach (const NetworkDeviceInfo &networkDeviceInfo, discoveryReply->networkDeviceInfos()) {

            // We need a unique mac
            if (networkDeviceInfo.monitorMode() != NetworkDeviceInfo::MonitorModeMac)
                continue;

            MacAddressInfo macInfo = networkDeviceInfo.macAddressInfos().constFirst();
            // Filter out already added network devices, rediscovery is in this case no option
            if (myThings().filterByParam(wolThingMacParamTypeId, macInfo.macAddress().toString()).count() != 0)
                continue;

            QString title;
            if (networkDeviceInfo.hostName().isEmpty()) {
                title = networkDeviceInfo.address().toString();
            } else {
                title = networkDeviceInfo.hostName() + " (" + networkDeviceInfo.address().toString() + ")";
            }
            QString description;
            if (macInfo.vendorName().isEmpty()) {
                description = macInfo.macAddress().toString();
            } else {
                description = macInfo.macAddress().toString() + " (" + macInfo.vendorName() + ")";
            }
            ThingDescriptor descriptor(wolThingClassId, title, description);
            descriptor.setParams({Param(wolThingMacParamTypeId, macInfo.macAddress().toString())});
            info->addThingDescriptor(descriptor);
        }
        info->finish(Thing::ThingErrorNoError);
    });
}

void IntegrationPluginWakeOnLan::executeAction(ThingActionInfo *info)
{
    qCInfo(dcWakeOnLan) << "Wake up" << info->thing()->name();
    wakeup(info->thing()->paramValue(wolThingMacParamTypeId).toString());
    return info->finish(Thing::ThingErrorNoError);
}

void IntegrationPluginWakeOnLan::wakeup(const QString &macAddress)
{
    const char header[] = {char(0xff), char(0xff), char(0xff), char(0xff), char(0xff), char(0xff)};
    QByteArray packet = QByteArray::fromRawData(header, sizeof(header));
    for(int i = 0; i < 16; ++i) {
        packet.append(QByteArray::fromHex(QString(macAddress).remove(':').toLocal8Bit()));
    }
    qCDebug(dcWakeOnLan) << "Created magic packet:" << packet.toHex();
    QUdpSocket udpSocket;
    udpSocket.writeDatagram(packet.data(), packet.size(), QHostAddress::Broadcast, 9);
}

