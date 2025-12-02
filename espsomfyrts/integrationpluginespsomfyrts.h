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

#ifndef INTEGRATIONPLUGINESPSOMFYRTS_H
#define INTEGRATIONPLUGINESPSOMFYRTS_H

#include <QUuid>

#include <plugintimer.h>
#include <network/networkaccessmanager.h>
#include <network/networkdevicemonitor.h>
#include <integrations/integrationplugin.h>

#include "extern-plugininfo.h"

#include "espsomfyrts.h"

class IntegrationPluginEspSomfyRts: public IntegrationPlugin
{
    Q_OBJECT

    Q_PLUGIN_METADATA(IID "io.nymea.IntegrationPlugin" FILE "integrationpluginespsomfyrts.json")
    Q_INTERFACES(IntegrationPlugin)

public:
    explicit IntegrationPluginEspSomfyRts();

    void init() override;
    void discoverThings(ThingDiscoveryInfo *info) override;
    void setupThing(ThingSetupInfo *info) override;
    void postSetupThing(Thing *thing) override;
    void thingRemoved(Thing *thing) override;

    void executeAction(ThingActionInfo *info) override;

private:
    PluginTimer *m_refreshTimer = nullptr;
    QHash<Thing *, EspSomfyRts *> m_somfys;
    QHash<uint, Thing *> m_shadeThings;

    int calculateAngleFromPercentage(int minAngle, int maxAngle, int percentage);
    int calculatePercentageFromAngle(int minAngle, int maxAngle, int angle);

    void createThingForShade(const QVariantMap &shadeMap, const ThingId &parentThingId);
    void processShadeState(Thing *thing, const QVariantMap &shadeState);
    void synchronizeShades(Thing *thing);

    ThingClassId getThingClassForShadeType(const QVariantMap &shadeMap);

private slots:
    void onEspSomfyConnectedChanged(Thing *thing, bool connected);

};

#endif // INTEGRATIONPLUGINESPSOMFYRTS_H

