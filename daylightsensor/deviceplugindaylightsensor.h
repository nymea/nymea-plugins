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

#ifndef DEVICEPLUGINDAYLIGHTSENSOR_H
#define DEVICEPLUGINDAYLIGHTSENSOR_H


#include "devices/deviceplugin.h"
#include "plugintimer.h"

class DevicePluginDaylightSensor: public DevicePlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "io.nymea.DevicePlugin" FILE "deviceplugindaylightsensor.json")
    Q_INTERFACES(DevicePlugin)

public:
    explicit DevicePluginDaylightSensor();
    ~DevicePluginDaylightSensor() override;

    void discoverDevices(DeviceDiscoveryInfo *info) override;
    void setupDevice(DeviceSetupInfo *info) override;
    void deviceRemoved(Device *device) override;

private slots:
    void pluginTimerEvent();

    void updateDevice(Device *device);
private:
    QHash<Device*, PluginTimer*> m_timers;
    QPair<QDateTime, QDateTime> calculateSunriseSunset(qreal lat, qreal lon, const QDateTime &dateTime);

};

#endif // DEVICEPLUGINDAYLIGHTSENSOR_H
