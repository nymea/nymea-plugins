/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *                                                                         *
 *  Copyright (C) 2015 Simon Stürz <simon.stuerz@guh.io>                   *
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

#include "devicepluginwakeonlan.h"

#include "devices/device.h"
#include "plugininfo.h"

#include <QDebug>
#include <QStringList>
#include <QUdpSocket>

DevicePluginWakeOnLan::DevicePluginWakeOnLan()
{
}

void DevicePluginWakeOnLan::executeAction(DeviceActionInfo *info)
{
    qCDebug(dcWakeOnLan) << "Wake up" << info->device()->name();
    wakeup(info->device()->paramValue(wolDeviceMacParamTypeId).toString());
    return info->finish(Device::DeviceErrorNoError);
}

void DevicePluginWakeOnLan::wakeup(QString mac)
{
    const char header[] = {char(0xff), char(0xff), char(0xff), char(0xff), char(0xff), char(0xff)};
    QByteArray packet = QByteArray::fromRawData(header, sizeof(header));
    for(int i = 0; i < 16; ++i) {
        packet.append(QByteArray::fromHex(mac.remove(':').toLocal8Bit()));
    }
    qCDebug(dcWakeOnLan) << "Created magic packet:" << packet.toHex();
    QUdpSocket udpSocket;
    udpSocket.writeDatagram(packet.data(), packet.size(), QHostAddress::Broadcast, 9);
}

