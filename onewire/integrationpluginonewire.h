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

#ifndef INTEGRATIONPLUGINONEWIRE_H
#define INTEGRATIONPLUGINONEWIRE_H

#include <integrations/integrationplugin.h>
#include <plugintimer.h>

#include <owfs.h>
#include <w1.h>

#include "extern-plugininfo.h"

#include <QHash>

class IntegrationPluginOneWire : public IntegrationPlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "io.nymea.IntegrationPlugin" FILE "integrationpluginonewire.json")
    Q_INTERFACES(IntegrationPlugin)

public:
    explicit IntegrationPluginOneWire();

    void discoverThings(ThingDiscoveryInfo *info) override;
    void setupThing(ThingSetupInfo *info) override;
    void postSetupThing(Thing *thing) override;
    void executeAction(ThingActionInfo *info) override;
    void thingRemoved(Thing *thing) override;

private:
    PluginTimer *m_pluginTimer = nullptr;
    Owfs *m_owfsInterface = nullptr;
    W1 *m_w1Interface = nullptr;

    QHash<Thing*, ThingDiscoveryInfo*> m_runningDiscoveries;

    void setupOwfsTemperatureSensor(ThingSetupInfo *info);
    void setupOwfsTemperatureHumiditySensor(ThingSetupInfo *info);

private slots:
    void onPluginTimer();
    void onOneWireDevicesDiscovered(QList<Owfs::OwfsDevice> devices);
};

#endif // INTEGRATIONPLUGINONEWIRE_H
