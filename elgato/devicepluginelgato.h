/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *                                                                         *
 *  Copyright (C) 2016-2018 Simon Stürz <simon.stuerz@guh.io>              *
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

#ifndef DEVICEPLUGINELGATO_H
#define DEVICEPLUGINELGATO_H

#include "aveabulb.h"
#include "plugintimer.h"
#include "devices/deviceplugin.h"
#include "hardware/bluetoothlowenergy/bluetoothlowenergydevice.h"

class DevicePluginElgato : public DevicePlugin
{
    Q_OBJECT

    Q_PLUGIN_METADATA(IID "io.nymea.DevicePlugin" FILE "devicepluginelgato.json")
    Q_INTERFACES(DevicePlugin)

public:
    explicit DevicePluginElgato();
    ~DevicePluginElgato();

    void init() override;
    void discoverDevices(DeviceDiscoveryInfo *info) override;
    void setupDevice(DeviceSetupInfo *info) override;
    void postSetupDevice(Device *device) override;
    void executeAction(DeviceActionInfo *info) override;
    void deviceRemoved(Device *device) override;

private:
    PluginTimer *m_pluginTimer = nullptr;
    QHash<Device *, AveaBulb *> m_bulbs;

    bool verifyExistingDevices(const QBluetoothDeviceInfo &deviceInfo);

private slots:
    void onPluginTimer();

};

#endif // DEVICEPLUGINELGATO_H
