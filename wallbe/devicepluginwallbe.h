/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *                                                                         *
 *  Copyright (C) 2015 Simon Stuerz <simon.stuerz@nymea.io>                *
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

#ifndef DEVICEPLUGINWALLBE_H
#define DEVICEPLUGINWALLBE_H

#include "devices/deviceplugin.h"
#include "wallbe.h"
#include "host.h"
#include "discover.h"
#include "plugintimer.h"

#include <QObject>
#include <QHostAddress>

class DevicePluginWallbe : public DevicePlugin
{
    Q_OBJECT

    Q_PLUGIN_METADATA(IID "io.nymea.DevicePlugin" FILE "devicepluginwallbe.json")
    Q_INTERFACES(DevicePlugin)

public:
    explicit DevicePluginWallbe();

    void init() override;
    void discoverDevices(DeviceDiscoveryInfo *info) override;
    void setupDevice(DeviceSetupInfo *info) override;
    void postSetupDevice(Device *device) override;
    void executeAction(DeviceActionInfo *info) override;
    void deviceRemoved(Device *device) override;

private slots:
    void processFinished(int exitCode, QProcess::ExitStatus exitStatus);

private:
    QHash<Device *, WallBe *> m_connections;
    QList<QHostAddress> m_address;
    PluginTimer *m_pluginTimer = nullptr;

    void update(Device *device);
};

#endif // DEVICEPLUGINWALLBE_H
