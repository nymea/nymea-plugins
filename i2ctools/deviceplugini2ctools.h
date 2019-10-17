/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *                                                                         *
 *  Copyright (C) 2019 Bernhard Trinnes <bernhard.trinnes@nymea.io>        *
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

#ifndef DEVICEPLUGINI2CTOOLS_H
#define DEVICEPLUGINI2CTOOLS_H

#include "plugintimer.h"
#include "devices/deviceplugin.h"

#include <QHash>

extern "C" {
#include "i2cbusses.h"
}

class DevicePluginI2cTools : public DevicePlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "io.nymea.DevicePlugin" FILE "deviceplugini2ctools.json")
    Q_INTERFACES(DevicePlugin)

public:
    enum ScanMode {
        ScanModeAuto  = 0,
        ScanModeQuick = 1,
        ScanModeRead  = 2,
        ScanModeFunc  =	3
    };

    explicit DevicePluginI2cTools();

    void discoverDevices(DeviceDiscoveryInfo *info) override;
    void setupDevice(DeviceSetupInfo *info) override;
    void postSetupDevice(Device *device) override;
    void deviceRemoved(Device *device) override;

private:
    PluginTimer *m_pluginTimer = nullptr;

    QHash<DeviceId, int> m_i2cDeviceFiles;
    QHash<int, unsigned long> m_fileFuncs;
    QList<DeviceDiscoveryInfo*> m_runningDiscoveries;

    QList<int> scanI2cBus(int file, ScanMode mode, unsigned long funcs, int first, int last);
    QList<i2c_adap> getI2cBusses();

private slots:
    void onPluginTimer();
};

#endif // DEVICEPLUGINI2CTOOLS_H
