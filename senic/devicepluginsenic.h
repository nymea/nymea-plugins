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
    void onBatteryValueChanged(const uint &percentage);
    void onButtonPressed();
    void onButtonLongPressed();
    void onSwipeDetected(const Nuimo::SwipeDirection &direction);
    void onRotationValueChanged(const uint &value);
    void onDeviceInformationChanged(const QString &firmwareRevision, const QString &hardwareRevision, const QString &softwareRevision);
};

#endif // DEVICEPLUGINSENIC_H
