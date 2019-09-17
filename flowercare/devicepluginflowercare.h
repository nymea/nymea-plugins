/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *                                                                         *
 *  Copyright (C) 2018 Michael Zanetti <michael.zanetti@guh.io>            *
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

#ifndef DEVICEPLUGINFLOWERCARE_H
#define DEVICEPLUGINFLOWERCARE_H


#include <QPointer>
#include <QHash>
#include "devices/deviceplugin.h"
#include "plugintimer.h"
#include "hardware/bluetoothlowenergy/bluetoothlowenergydevice.h"

class FlowerCare;

class DevicePluginFlowercare : public DevicePlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "io.nymea.DevicePlugin" FILE "devicepluginflowercare.json")
    Q_INTERFACES(DevicePlugin)

public:
    explicit DevicePluginFlowercare();
    ~DevicePluginFlowercare();

    void init() override;
    void discoverDevices(DeviceDiscoveryInfo *info) override;
    void setupDevice(DeviceSetupInfo *info) override;
    void postSetupDevice(Device *device) override;
    void deviceRemoved(Device *device) override;

private:
    PluginTimer *m_reconnectTimer = nullptr;
    QHash<Device*, FlowerCare*> m_list;
    QHash<FlowerCare*, int> m_refreshMinutes;

    bool verifyExistingDevices(const QBluetoothDeviceInfo &deviceInfo);

private slots:
    void onPluginTimer();
    void onSensorDataReceived(quint8 batteryLevel, double degreeCelsius, double lux, double moisture, double fertility);

};

#endif // DEVICEPLUGINFLOWERCARE_H
