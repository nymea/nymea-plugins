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

#ifndef INTEGRATIONPLUGINPII2CDEVICES_H
#define INTEGRATIONPLUGINPII2CDEVICES_H

#include <integrations/integrationplugin.h>

#include "extern-plugininfo.h"

class I2CDevice;

class IntegrationPluginI2CDevices: public IntegrationPlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "io.nymea.IntegrationPlugin" FILE "integrationplugini2cdevices.json")
    Q_INTERFACES(IntegrationPlugin)

public:
    IntegrationPluginI2CDevices();

    void init() override;
    void discoverThings(ThingDiscoveryInfo *info) override;

    void setupThing(ThingSetupInfo *info) override;
    void thingRemoved(Thing *thing) override;

private:
    QHash<I2CDevice*, Thing*> m_i2cDevices;

    QHash<int, StateTypeId> m_ads1115ChannelMap;
    QHash<int, StateTypeId> m_ads1115OvervoltageMap;

    QHash<int, StateTypeId> m_pi16adcChannelMap;
    QHash<int, StateTypeId> m_pi16adcOvervoltageMap;
};

#endif
