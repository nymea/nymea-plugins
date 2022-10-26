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

#include "integrationpluginwakeonlan.h"

#include "integrations/thing.h"
#include "plugininfo.h"

#include "network/networkdevicediscovery.h"

#include <QDebug>
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
        ThingDescriptors descriptors;
        qCDebug(dcWakeOnLan()) << "Discovery finished. Found" << discoveryReply->networkDeviceInfos().count() << "devices";
        foreach (const NetworkDeviceInfo &networkDeviceInfo, discoveryReply->networkDeviceInfos()) {
            // We need the mac address...
            if (networkDeviceInfo.macAddress().isEmpty())
                continue;

            // Filter out already added network devices, rediscovery is in this case no option
            if (myThings().filterByParam(wolThingMacParamTypeId, networkDeviceInfo.macAddress()).count() != 0)
                continue;

            QString title;
            if (networkDeviceInfo.hostName().isEmpty()) {
                title = networkDeviceInfo.address().toString();
            } else {
                title = networkDeviceInfo.hostName() + " (" + networkDeviceInfo.address().toString() + ")";
            }
            QString description;
            if (networkDeviceInfo.macAddressManufacturer().isEmpty()) {
                description = networkDeviceInfo.macAddress();
            } else {
                description = networkDeviceInfo.macAddress() + " (" + networkDeviceInfo.macAddressManufacturer() + ")";
            }
            ThingDescriptor descriptor(wolThingClassId, title, description);
            ParamList params;
            params.append(Param(wolThingMacParamTypeId, networkDeviceInfo.macAddress()));
            descriptor.setParams(params);
            descriptors.append(descriptor);
        }
        info->addThingDescriptors(descriptors);
        info->finish(Thing::ThingErrorNoError);
    });
}

void IntegrationPluginWakeOnLan::executeAction(ThingActionInfo *info)
{
    qCDebug(dcWakeOnLan) << "Wake up" << info->thing()->name();
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

