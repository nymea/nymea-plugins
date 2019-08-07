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

#ifndef DEVICEPLUGINSENIC_H
#define DEVICEPLUGINSENIC_H

#include "plugintimer.h"
#include "devices/deviceplugin.h"
#include "hardware/bluetoothlowenergy/bluetoothlowenergydevice.h"

#include "nuimo.h"

class DevicePluginSenic : public DevicePlugin
{
    Q_OBJECT

    Q_PLUGIN_METADATA(IID "io.nymea.DevicePlugin" FILE "devicepluginsenic.json")
    Q_INTERFACES(DevicePlugin)

public:
    explicit DevicePluginSenic();

    void init() override;
    Device::DeviceError discoverDevices(const DeviceClassId &deviceClassId, const ParamList &params) override;
    Device::DeviceSetupStatus setupDevice(Device *device) override;
    Device::DeviceError executeAction(Device *device, const Action &action) override;

    void deviceRemoved(Device *device) override;

private:
    QHash<Nuimo *, Device *> m_nuimos;
    PluginTimer *m_reconnectTimer = nullptr;
    bool m_autoSymbolMode = true;

    bool verifyExistingDevices(const QBluetoothDeviceInfo &deviceInfo);

private slots:
    void onPluginConfigurationChanged(const ParamTypeId &paramTypeId, const QVariant &value);
    void onReconnectTimeout();
    void onBluetoothDiscoveryFinished();

    void onButtonPressed();
    void onButtonReleased();
    void onSwipeDetected(const Nuimo::SwipeDirection &direction);
    void onRotationValueChanged(const uint &value);

};

#endif // DEVICEPLUGINSENIC_H
