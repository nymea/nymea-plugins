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

#ifndef INTEGRATIONPLUGINSOLARLOG_H
#define INTEGRATIONPLUGINSOLARLOG_H

#include <integrations/integrationplugin.h>

#include <QDebug>
#include <QHostAddress>
#include <QUrlQuery>

class PluginTimer;

class IntegrationPluginSolarLog: public IntegrationPlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "io.nymea.IntegrationPlugin" FILE "integrationpluginsolarlog.json")
    Q_INTERFACES(IntegrationPlugin)

public:
    enum JsonObjectNumbers{
        LastUpdateTime = 100,
        Pac,
        Pdc,
        Uac,
        DCVoltage,
        YieldDay,
        YieldYesterday,
        YieldMonth,
        YieldYear,
        YieldTotal,
        ConsPac,
        ConsYieldDay,
        ConsYieldYesterday,
        ConsYieldMonth,
        ConsYieldYear,
        ConsYieldTotal,
        TotalPower
    };
    explicit IntegrationPluginSolarLog();

    void setupThing(ThingSetupInfo *info) override;
    void postSetupThing(Thing *thing) override;
    void thingRemoved(Thing *thing) override;

private slots:
    void onRefreshTimer();

private:
    PluginTimer *m_refreshTimer = nullptr;
    QHash<Thing *, ThingSetupInfo *> m_asyncSetup;

    void getData(Thing *thing);
    void markThingDisconnected(Thing *thing);
};

#endif // INTEGRATIONPLUGINSOLARLOG_H
