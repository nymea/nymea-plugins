/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *                                                                         *
 *  Copyright (C) 2015 Simon Stuerz <simon.stuerz@guh.guru>                *
 *                                                                         *
 *  This file is part of guh.                                              *
 *                                                                         *
 *  Guh is free software: you can redistribute it and/or modify            *
 *  it under the terms of the GNU General Public License as published by   *
 *  the Free Software Foundation, version 2 of the License.                *
 *                                                                         *
 *  Guh is distributed in the hope that it will be useful,                 *
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of         *
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the          *
 *  GNU General Public License for more details.                           *
 *                                                                         *
 *  You should have received a copy of the GNU General Public License      *
 *  along with guh. If not, see <http://www.gnu.org/licenses/>.            *
 *                                                                         *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

#ifndef DEVICEPLUGINBOBLIGHT_H
#define DEVICEPLUGINBOBLIGHT_H

#include "plugin/deviceplugin.h"
#include "bobclient.h"

class BobClient;
class PluginTimer;

class DevicePluginBoblight : public DevicePlugin
{
    Q_OBJECT

    Q_PLUGIN_METADATA(IID "io.nymea.DevicePlugin" FILE "devicepluginboblight.json")
    Q_INTERFACES(DevicePlugin)

public:
    explicit DevicePluginBoblight();

    void init() override;

    DeviceManager::DeviceSetupStatus setupDevice(Device *device) override;
    void postSetupDevice(Device *device) override;
    void deviceRemoved(Device *device) override;

    void startMonitoringAutoDevices() override;

    DeviceManager::DeviceError executeAction(Device *device, const Action &action) override;

private slots:
    void onConnectionChanged();
    void guhTimer();

    void onPowerChanged(int channel, bool power);
    void onBrightnessChanged(int channel, int brightness);
    void onColorChanged(int channel, const QColor &color);
    void onPriorityChanged(int priority);

private:
    QColor tempToRgb(int temp);
private:
    PluginTimer *m_pluginTimer = nullptr;

    QHash<DeviceId, BobClient*> m_bobClients;
    bool m_canCreateAutoDevices = false;
};

#endif // DEVICEPLUGINBOBLIGHT_H
