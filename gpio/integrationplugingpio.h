// SPDX-License-Identifier: GPL-3.0-or-later

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*
* Copyright (C) 2013 - 2024, nymea GmbH
* Copyright (C) 2024 - 2025, chargebyte austria GmbH
*
* This file is part of nymea-plugins.
*
* nymea-plugins is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* (at your option) any later version.
*
* nymea-plugins is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
* General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with nymea-plugins. If not, see <https://www.gnu.org/licenses/>.
*
* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

#ifndef INTEGRATIONPLUGINGPIO_H
#define INTEGRATIONPLUGINGPIO_H

#include <integrations/integrationplugin.h>
#include <plugintimer.h>

#include "gpiodescriptor.h"

// libnymea-gpio
#include <gpio.h>
#include <gpiomonitor.h>
#include <gpiobutton.h>

class IntegrationPluginGpio : public IntegrationPlugin
{
    Q_OBJECT

    Q_PLUGIN_METADATA(IID "io.nymea.IntegrationPlugin" FILE "integrationplugingpio.json")
    Q_INTERFACES(IntegrationPlugin)

public:
    explicit IntegrationPluginGpio();

    void init() override;
    void discoverThings(ThingDiscoveryInfo *info) override;
    void setupThing(ThingSetupInfo *info) override;
    void postSetupThing(Thing *thing) override;
    void thingRemoved(Thing *thing) override;
    void executeAction(ThingActionInfo *info) override;

private:
    QHash<ThingClassId, ParamTypeId> m_gpioParamTypeIds;
    QHash<ThingClassId, ParamTypeId> m_activeLowParamTypeIds;

    QHash<Gpio *, Thing *> m_gpioDevices;
    QHash<GpioMonitor *, Thing *> m_monitorDevices;
    QHash<GpioButton *, Thing *> m_buttonDevices;

    QHash<int, Gpio *> m_raspberryPiGpios;
    QHash<int, GpioMonitor *> m_raspberryPiGpioMoniors;
    QHash<int, GpioButton *> m_raspberryPiGpioButtons;

    QHash<int, Gpio *> m_beagleboneBlackGpios;
    QHash<int, GpioMonitor *> m_beagleboneBlackGpioMoniors;
    QHash<int, GpioButton *> m_beagleboneBlackGpioButtons;

    QList<GpioDescriptor> raspberryPiGpioDescriptors();
    QList<GpioDescriptor> beagleboneBlackGpioDescriptors();
    PluginTimer *m_counterTimer = nullptr;
    QHash<ThingId, int> m_counterValues;

};

#endif // INTEGRATIONPLUGINGPIO_H
