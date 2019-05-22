/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *                                                                         *
 *  Copyright (C) 2019 Simon Stürz <simon.stuerz@nymea.io>                 *
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

#ifndef DEVICEPLUGINHONEYWELLSCANNER_H
#define DEVICEPLUGINHONEYWELLSCANNER_H

#include "devicemanager.h"
#include "plugin/deviceplugin.h"

#include "honeywellscanner.h"

class DevicePluginHoneywellScanner: public DevicePlugin
{
    Q_OBJECT

    Q_PLUGIN_METADATA(IID "io.nymea.DevicePlugin" FILE "devicepluginhoneywellscanner.json")
    Q_INTERFACES(DevicePlugin)


public:
    explicit DevicePluginHoneywellScanner();

    void init() override;
    void startMonitoringAutoDevices() override;
    void postSetupDevice(Device *device) override;
    void deviceRemoved(Device *device) override;

    DeviceManager::DeviceError discoverDevices(const DeviceClassId &deviceClassId, const ParamList &params) override;
    DeviceManager::DeviceSetupStatus setupDevice(Device *device) override;
    DeviceManager::DeviceError executeAction(Device *device, const Action &action) override;

private:
    QHash<Device *, HoneywellScanner *> m_scanners;

};

#endif // DEVICEPLUGINHONEYWELLSCANNER_H
