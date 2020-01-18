/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *                                                                         *
 *  Copyright (C) 2020 Bernhard Trinnes <bernhard.trinnes@nymea.io>        *
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

#ifndef DEVICEPLUGINOPENUV_H
#define DEVICEPLUGINOPENUV_H

#include "plugintimer.h"
#include "devices/deviceplugin.h"
#include "network/networkaccessmanager.h"

#include <QTimer>
#include <QHostAddress>

class DevicePluginOpenUv : public DevicePlugin
{
    Q_OBJECT

    Q_PLUGIN_METADATA(IID "io.nymea.DevicePlugin" FILE "devicepluginopenuv.json")
    Q_INTERFACES(DevicePlugin)

public:
    explicit DevicePluginOpenUv();

    void init() override;
    void discoverDevices(DeviceDiscoveryInfo *info) override;
    void setupDevice(DeviceSetupInfo *info) override;
    void postSetupDevice(Device *device)override;
    void deviceRemoved(Device *device) override;

    void getUvIndex(Device *device);
private:
    PluginTimer *m_pluginTimer = nullptr;
    QByteArray m_apiKey;

    void searchGeoLocation(double lat, double lon, const QString &country, DeviceDiscoveryInfo *info);

private slots:
    void onPluginTimer();
};

#endif // DEVICEPLUGINOPENUV_H
