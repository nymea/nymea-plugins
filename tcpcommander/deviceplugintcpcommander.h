/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *                                                                         *
 *  Copyright (C) 2019 Bernhard Trinnes <bernhard.trinnes@nymea.io>        *
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
#include "tcpsocket.h"

class DevicePluginTcpCommander : public DevicePlugin
{
    Q_OBJECT

    Q_PLUGIN_METADATA(IID "io.nymea.DevicePlugin" FILE "deviceplugintcpcommander.json")
    Q_INTERFACES(DevicePlugin)

public:
    explicit DevicePluginTcpCommander();

    void setupDevice(DeviceSetupInfo *info) override;

    void deviceRemoved(Device *device) override;

    void executeAction(DeviceActionInfo *info) override;

private:
    QHash<TcpSocket *, Device *> m_tcpSockets;
    QHash<TcpServer *, Device *> m_tcpServer;

private slots:
    void onTcpSocketConnectionChanged(bool connected);

    void onTcpServerConnectionChanged(bool connected);
    void onTcpServerCommandReceived(QByteArray message);
};

#endif // DEVICEPLUGINTCPCOMMANDER_H
