/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*
* Copyright 2013 - 2020, nymea GmbH
* Contact: contact@nymea.io
*
* This file is part of nymea.
* This project including source code and documentation is protected by
* copyright law, and remains the property of nymea GmbH. All rights, including
* reproduction, publication, editing and translation, are reserved. The use of
* this project is subject to the terms of a license agreement to be concluded
* with nymea GmbH in accordance with the terms of use of nymea GmbH, available
* under https://nymea.io/license
*
* GNU Lesser General Public License Usage
* Alternatively, this project may be redistributed and/or modified under the
* terms of the GNU Lesser General Public License as published by the Free
* Software Foundation; version 3. This project is distributed in the hope that
* it will be useful, but WITHOUT ANY WARRANTY; without even the implied
* warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
* Lesser General Public License for more details.
*
* You should have received a copy of the GNU Lesser General Public License
* along with this project. If not, see <https://www.gnu.org/licenses/>.
*
* For any further details and any questions please contact us under
* contact@nymea.io or see our FAQ/Licensing Information on
* https://nymea.io/license/faq
*
* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

#ifndef INTEGRATIONPLUGINSYSTEMMONITOR_H
#define INTEGRATIONPLUGINSYSTEMMONITOR_H

#include "integrations/integrationplugin.h"
#include "plugintimer.h"

#include <QDebug>
#include <QProcess>
#include <QUrlQuery>

#include "extern-plugininfo.h"

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

    QHash<Thing*, qulonglong> m_oldTotalJiffies;
    QHash<Thing*, qulonglong> m_oldWorkJiffies;
    QHash<Thing*, qulonglong> m_oldProcessWorkJiffies;

};

#endif // INTEGRATIONPLUGINSYSTEMMONITOR_H
