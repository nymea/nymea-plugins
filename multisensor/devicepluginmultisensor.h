/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *                                                                         *
 *  Copyright (C) 2015-2018 Simon Stuerz <simon.stuerz@guh.io>             *
 *  Copyright (C) 2016 nicc                                                *
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

#ifndef DEVICEPLUGINMULTISENSOR_H
#define DEVICEPLUGINMULTISENSOR_H


#include <QPointer>
#include <QHash>
#include "plugin/deviceplugin.h"
#include "devicemanager.h"
#include "plugintimer.h"
#include "hardware/bluetoothlowenergy/bluetoothlowenergydevice.h"
#include "sensortag.h"

class DevicePluginMultiSensor : public DevicePlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "io.nymea.DevicePlugin" FILE "devicepluginmultisensor.json")
    Q_INTERFACES(DevicePlugin)

public:
    explicit DevicePluginMultiSensor();
    ~DevicePluginMultiSensor();

    void init() override;
    DeviceManager::DeviceError discoverDevices(const DeviceClassId &deviceClassId, const ParamList &params) override;
    DeviceManager::DeviceSetupStatus setupDevice(Device *device) override;
    void postSetupDevice(Device *device) override;
    void deviceRemoved(Device *device) override;

private:
    PluginTimer *m_reconnectTimer = nullptr;
    QHash<Device *, SensorTag *> m_sensors;

    bool verifyExistingDevices(const QBluetoothDeviceInfo &deviceInfo);

private slots:
    void onPluginTimer();
    void onBluetoothDiscoveryFinished();
    void onPluginConfigurationChanged(const ParamTypeId &paramTypeId, const QVariant &value);
};

#endif // DEVICEPLUGINMULTISENSOR_H
