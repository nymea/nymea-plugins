/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *                                                                         *
 *  Copyright (C) 2016 Simon Stürz <simon.stuerz@guh.io>                   *
 *  Copyright (C) 2016 Michael Zanetti <michael_zanetti@gmx.net>           *
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

#ifndef DEVICEPLUGINNETWORKDETECTOR_H
#define DEVICEPLUGINNETWORKDETECTOR_H

#include "devices/deviceplugin.h"
#include "host.h"
#include "discovery.h"
#include "plugintimer.h"
#include "devicemonitor.h"
#include "broadcastping.h"

#include <QProcess>
#include <QXmlStreamReader>
#include <QHostInfo>

class DevicePluginNetworkDetector : public DevicePlugin
{
    Q_OBJECT

    Q_PLUGIN_METADATA(IID "io.nymea.DevicePlugin" FILE "devicepluginnetworkdetector.json")
    Q_INTERFACES(DevicePlugin)

public:
    explicit DevicePluginNetworkDetector();
    ~DevicePluginNetworkDetector();

    void init() override;

    void setupDevice(DeviceSetupInfo *info) override;
    void discoverDevices(DeviceDiscoveryInfo *info) override;
    void deviceRemoved(Device *device) override;


private slots:
    void deviceReachableChanged(bool reachable);
    void deviceAddressChanged(const QString &address);
    void deviceSeen();

    void broadcastPingFinished();

private:
    PluginTimer *m_pluginTimer = nullptr;
    BroadcastPing *m_broadcastPing = nullptr;
    QHash<DeviceMonitor*, Device*> m_monitors;
};

#endif // DEVICEPLUGINNETWORKDETECTOR_H
