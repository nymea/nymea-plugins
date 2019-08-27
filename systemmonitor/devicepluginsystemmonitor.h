/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *                                                                         *
 *  Copyright (C) 2018 Michael Zanetti <michael.zanetti@guh.io             *
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

#ifndef DEVICEPLUGINSYSTEMMONITOR_H
#define DEVICEPLUGINSYSTEMMONITOR_H

#include "devices/devicemanager.h"
#include "devices/deviceplugin.h"
#include "plugintimer.h"

#include <QDebug>
#include <QProcess>
#include <QUrlQuery>


class DevicePluginSystemMonitor: public DevicePlugin {
	Q_OBJECT
    Q_PLUGIN_METADATA(IID "io.nymea.DevicePlugin" FILE "devicepluginsystemmonitor.json")
	Q_INTERFACES(DevicePlugin)

public:
    explicit DevicePluginSystemMonitor();
    ~DevicePluginSystemMonitor() override;

    Device::DeviceSetupStatus setupDevice(Device *device) override;
    void deviceRemoved(Device *device) override;

private slots:
    void onRefreshTimer();
    void onProcessFinished(int exitCode, QProcess::ExitStatus exitStatus);

private:
    PluginTimer *m_refreshTimer = nullptr;

};

#endif // DEVICEPLUGINSYSTEMMONITOR_H
