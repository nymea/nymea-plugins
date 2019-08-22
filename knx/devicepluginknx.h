/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *                                                                         *
 *  Copyright (C) 2019 Simon Stürz <simon.stuerz@guh.io>                   *
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

#ifndef DEVICEPLUGINKNX_H
#define DEVICEPLUGINKNX_H

#include "plugintimer.h"
#include "devicemanager.h"
#include "plugin/deviceplugin.h"

#include "knxtunnel.h"
#include "knxserverdiscovery.h"


class DevicePluginKnx: public DevicePlugin
{
    Q_OBJECT

    Q_PLUGIN_METADATA(IID "io.nymea.DevicePlugin" FILE "devicepluginknx.json")
    Q_INTERFACES(DevicePlugin)

public:
    explicit DevicePluginKnx();

    void init() override;
    void startMonitoringAutoDevices() override;

    DeviceManager::DeviceError discoverDevices(const DeviceClassId &deviceClassId, const ParamList &params) override;
    DeviceManager::DeviceSetupStatus setupDevice(Device *device) override;

    void postSetupDevice(Device *device) override;
    void deviceRemoved(Device *device) override;

    DeviceManager::DeviceError executeAction(Device *device, const Action &action) override;

private:
    PluginTimer *m_pluginTimer = nullptr;

    KnxServerDiscovery *m_discovery = nullptr;
    QHash<KnxTunnel *, Device *> m_tunnels;

    KnxTunnel *getTunnelForDevice(Device *device);

    void autoCreateKnownDevices(Device *parentDevice);

    void createGenericDevices(Device *parentDevice);
    void destroyGenericDevices(Device *parentDevice);

private slots:
    void onPluginTimerTimeout();
    void onPluginConfigurationChanged(const ParamTypeId &paramTypeId, const QVariant &value);

    void onDiscoveryFinished();
    void onTunnelConnectedChanged();
    void onTunnelFrameReceived(const QKnxLinkLayerFrame &frame);

};

#endif // DEVICEPLUGINKNX_H
