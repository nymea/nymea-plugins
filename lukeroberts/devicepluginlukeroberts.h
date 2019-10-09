/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *                                                                         *
 *  Copyright (C) 2019 Bernhard Trinnes <bernhard.trinnes@nymea.io         *
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

#ifndef DEVICEPLUGINLUKEROBERTS_H
#define DEVICEPLUGINLUKEROBERTS_H

#include "plugintimer.h"
#include "devices/deviceplugin.h"
#include "hardware/bluetoothlowenergy/bluetoothlowenergydevice.h"

#include "lukeroberts.h"

class DevicePluginLukeRoberts : public DevicePlugin
{
    Q_OBJECT

    Q_PLUGIN_METADATA(IID "io.nymea.DevicePlugin" FILE "devicepluginlukeroberts.json")
    Q_INTERFACES(DevicePlugin)

public:
    explicit DevicePluginLukeRoberts();

    void init() override;
    void discoverDevices(DeviceDiscoveryInfo *info) override;
    void setupDevice(DeviceSetupInfo *info) override;
    void postSetupDevice(Device *device) override;
    void executeAction(DeviceActionInfo *info) override;

    void deviceRemoved(Device *device) override;

private:
    QHash<Nuimo *, Device *> m_nuimos;
    PluginTimer *m_reconnectTimer = nullptr;
    bool m_autoSymbolMode = true;

private slots:
    void onPluginConfigurationChanged(const ParamTypeId &paramTypeId, const QVariant &value);
    void onReconnectTimeout();

    void onConnectedChanged(bool connected);
    void onStatusCodeReceived(LukeRoberts::StatusCodes statusCode);
    void onDeviceInformationChanged(const QString &firmwareRevision, const QString &hardwareRevision, const QString &softwareRevision);
};

#endif // DEVICEPLUGINLUKEROBERTS_H
