/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *                                                                         *
 *  Copyright (C) 2017 Bernhard Trinnes <bernhard.trinnes@guh.io>          *
 *                                                                         *
 *  This file is part of nymea.                                            *
 *                                                                         *
 *  nymea is free software: you can redistribute it and/or modify          *
 *  it under the terms of the GNU General Public License as published by   *
 *  the Free Software Foundation, version 2 of the License.                *
 *                                                                         *
 *  nymea is distributed in the hope that it will be useful,               *
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of         *
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the           *
 *  GNU General Public License for more details.                           *
 *                                                                         *
 *  You should have received a copy of the GNU General Public License      *
 *  along with nymea. If not, see <http://www.gnu.org/licenses/>.          *
 *                                                                         *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

#ifndef DEVICEPLUGINDEVTCPCOMMANDER_H
#define DEVICEPLUGINDEVTCPCOMMANDER_H

#include "devices/deviceplugin.h"
#include "tcpserver.h"

class DevicePluginTcpCommander : public DevicePlugin
{
    Q_OBJECT

    Q_PLUGIN_METADATA(IID "io.nymea.DevicePlugin" FILE "deviceplugintcpcommander.json")
    Q_INTERFACES(DevicePlugin)

public:
    explicit DevicePluginTcpCommander();

    Device::DeviceSetupStatus setupDevice(Device *device) override;

    void deviceRemoved(Device *device) override;

    Device::DeviceError executeAction(Device *device, const Action &action) override;

private:
    QHash<QTcpSocket *, Device *> m_tcpSockets;
    QHash<TcpServer *, Device *> m_tcpServer;

private slots:
    void onTcpSocketConnected();
    void onTcpSocketDisconnected();
    void onTcpSocketBytesWritten();

    void onTcpServerConnected();
    void onTcpServerDisconnected();
    void onTcpServerTextMessageReceived(QByteArray message);

};

#endif // DEVICEPLUGINTCPCOMMANDER_H
