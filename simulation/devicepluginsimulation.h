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

#include "devices/deviceplugin.h"
#include "plugintimer.h"

#include <QDateTime>

class DevicePluginSimulation : public DevicePlugin
{
    Q_OBJECT

    Q_PLUGIN_METADATA(IID "io.nymea.DevicePlugin" FILE "devicepluginsimulation.json")
    Q_INTERFACES(DevicePlugin)


public:
    explicit DevicePluginSimulation();
    ~DevicePluginSimulation();

    void init() override;
    void setupDevice(DeviceSetupInfo *info) override;
    void deviceRemoved(Device *device) override;
    void executeAction(DeviceActionInfo *info) override;

private:
    PluginTimer *m_pluginTimer20Seconds = nullptr;
    PluginTimer *m_pluginTimer5Min = nullptr;

    int generateRandomIntValue(int min, int max);
    double generateRandomDoubleValue(double min, double max);
    bool generateRandomBoolValue();

    // Generates values in a sin curve from min to max, moving the start by hourOffset from midnight
    qreal generateSinValue(int min, int max, int hourOffset, int decimals = 2);
    qreal generateBatteryValue(int chargeStartHour, int chargeDurationInMinutes);
    qreal generateNoisyRectangle(int min, int max, int noise, int stablePeriodInMinutes, int &lastValue, QDateTime &lastChangeTimestamp);

    QHash<Device*, QTimer*> m_simulationTimers;
private slots:
    void onPluginTimer20Seconds();
    void onPluginTimer5Minutes();
    void simulationTimerTimeout();

};

#endif // DEVICEPLUGINSIMMULATION_H
