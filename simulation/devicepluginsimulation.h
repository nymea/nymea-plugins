/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *                                                                         *
 *  Copyright (C) 2017 Bernhard Trinnes <bernhard.trinnes@guh.io>          *
 *  Copyright (C) 2018 Simon Stürz <simon.stuerz@guh.io>                   *
 *                                                                         *
 *  This file is part of nymea.                                            *
 *                                                                         *
 *  nymea is free software: you can redistribute it and/or modify          *
 *  it under the terms of the GNU General Public License as published by   *
 *  the Free Software Foundation, version 2 of the License.                *
 *                                                                         *
 *  nymea is distributed in the hope that it will be useful,               *
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of         *
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the           *
 *  GNU General Public License for more details.                           *
 *                                                                         *
 *  You should have received a copy of the GNU General Public License      *
 *  along with nymea. If not, see <http://www.gnu.org/licenses/>.          *
 *                                                                         *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

#ifndef DEVICEPLUGINSIMMULATION_H
#define DEVICEPLUGINSIMMULATION_H

#include "plugin/deviceplugin.h"
#include "devicemanager.h"
#include "plugintimer.h"

class DevicePluginSimulation : public DevicePlugin
{
    Q_OBJECT

    Q_PLUGIN_METADATA(IID "io.nymea.DevicePlugin" FILE "devicepluginsimulation.json")
    Q_INTERFACES(DevicePlugin)


public:
    explicit DevicePluginSimulation();
    ~DevicePluginSimulation();

    void init() override;
    DeviceManager::DeviceSetupStatus setupDevice(Device *device) override;
    DeviceManager::DeviceError executeAction(Device *device, const Action &action) override;

private:
    PluginTimer *m_pluginTimer20Seconds = nullptr;
    PluginTimer *m_pluginTimer5Min = nullptr;

    int generateRandomIntValue(int min, int max);
    double generateRandomDoubleValue(double min, double max);
    bool generateRandomBoolValue();

private slots:
    void onPluginTimer20Seconds();
    void onPluginTimer5Minutes();

};

#endif // DEVICEPLUGINSIMMULATION_H
