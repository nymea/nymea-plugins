/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *                                                                         *
 *  Copyright (C) 2019 Michael Zanetti <michael.zanetti@nymea.io>          *
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

#ifndef DEVICEPLUGINTEXASINSTRUMENTS_H
#define DEVICEPLUGINTEXASINSTRUMENTS_H

#include <QObject>

#include "devices/deviceplugin.h"

class SensorTag;

class PluginTimer;

class DevicePluginTexasInstruments : public DevicePlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "io.nymea.DevicePlugin" FILE "deviceplugintexasinstruments.json")
    Q_INTERFACES(DevicePlugin)

public:
    explicit DevicePluginTexasInstruments(QObject *parent = nullptr);
    ~DevicePluginTexasInstruments() override;

    void discoverDevices(DeviceDiscoveryInfo *info) override;
    void setupDevice(DeviceSetupInfo *info) override;
    void postSetupDevice(Device *device) override;
    void deviceRemoved(Device *device) override;
    void executeAction(DeviceActionInfo *info) override;

private:
    QHash<Device*, SensorTag*> m_sensorTags;

    PluginTimer *m_reconnectTimer = nullptr;
};

#endif // DEVICEPLUGINTEXASINSTRUMENTS_H
