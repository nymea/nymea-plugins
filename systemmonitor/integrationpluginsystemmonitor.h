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

#ifndef INTEGRATIONPLUGINSYSTEMMONITOR_H
#define INTEGRATIONPLUGINSYSTEMMONITOR_H

#include <integrations/integrationplugin.h>

#include <QDebug>
#include <QProcess>
#include <QUrlQuery>

#include "extern-plugininfo.h"

class PluginTimer;

class IntegrationPluginSystemMonitor: public IntegrationPlugin {
	Q_OBJECT
    Q_PLUGIN_METADATA(IID "io.nymea.IntegrationPlugin" FILE "integrationpluginsystemmonitor.json")
	Q_INTERFACES(IntegrationPlugin)

public:
    explicit IntegrationPluginSystemMonitor();
    ~IntegrationPluginSystemMonitor() override;

    void setupThing(ThingSetupInfo *info) override;
    void thingRemoved(Thing *thing) override;

private:
    void updateSystemMonitor(Thing *thing);
    void updateProcessMonitor(Thing *thing);

    double readTotalCpuUsage(Thing *thing);
    double readTotalMemoryUsage();
    bool readProcessMemoryUsage(qint32 pid, quint32 &total, quint32 &rss, quint32 &shared, double &percentage);
    double readProcessCpuUsage(qint32 pid, Thing *thing);

    qint32 getPidByName(const QString &processName);

private:
    PluginTimer *m_refreshTimer = nullptr;

    QHash<Thing *, qulonglong> m_oldTotalJiffies;
    QHash<Thing *, qulonglong> m_oldWorkJiffies;
    QHash<Thing *, qulonglong> m_oldProcessWorkJiffies;

};

#endif // INTEGRATIONPLUGINSYSTEMMONITOR_H
