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
        info->finish(Thing::ThingErrorHardwareNotAvailable, QT_TR_NOOP("The discovery is not available."));
        return;
    }

    qCDebug(dcWakeOnLan()) << "Start discovering network devices...";
    NetworkDeviceDiscoveryReply *discoveryReply = hardwareManager()->networkDeviceDiscovery()->discover();
    connect(discoveryReply, &NetworkDeviceDiscoveryReply::finished, this, [=](){
        ThingDescriptors descriptors;
        qCDebug(dcWakeOnLan()) << "Discovery finished. Found" << discoveryReply->networkDevices().count() << "devices";
        foreach (const NetworkDevice &networkDevice, discoveryReply->networkDevices()) {
            // We need the mac address...
            if (networkDevice.macAddress().isEmpty())
                continue;

            // Filter out already added network devices, rediscovery is in this case no option
            if (myThings().filterByParam(wolThingMacParamTypeId, networkDevice.macAddress()).count() != 0)
                continue;

            QString title;
            if (networkDevice.hostName().isEmpty()) {
                title = networkDevice.address().toString();
            } else {
                title = networkDevice.address().toString() + " (" + networkDevice.hostName() + ")";
            }
            QString description;
            if (networkDevice.macAddressManufacturer().isEmpty()) {
                description = networkDevice.macAddress();
            } else {
                description = networkDevice.macAddress() + " (" + networkDevice.macAddressManufacturer() + ")";
            }
            ThingDescriptor descriptor(wolThingClassId, title, description);
            ParamList params;
            params.append(Param(wolThingMacParamTypeId, networkDevice.macAddress()));
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

