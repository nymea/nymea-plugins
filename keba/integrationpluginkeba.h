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

#ifndef INTEGRATIONPLUGINKEBA_H
#define INTEGRATIONPLUGINKEBA_H

#include <plugintimer.h>
#include <integrations/integrationplugin.h>
#include <network/networkdevicediscovery.h>

#include "kecontact.h"
#include "kebadiscovery.h"
#include "kecontactdatalayer.h"

#include <QHash>
#include <QUdpSocket>
#include <QDateTime>
#include <QUdpSocket>

#include "extern-plugininfo.h"

class IntegrationPluginKeba : public IntegrationPlugin
{
    Q_OBJECT

    Q_PLUGIN_METADATA(IID "io.nymea.IntegrationPlugin" FILE "integrationpluginkeba.json")
    Q_INTERFACES(IntegrationPlugin)

public:
    explicit IntegrationPluginKeba();

    void init() override;

    void discoverThings(ThingDiscoveryInfo *info) override;
    void setupThing(ThingSetupInfo *info) override;

    void postSetupThing(Thing* thing) override;
    void thingRemoved(Thing* thing) override;

    void executeAction(ThingActionInfo *info) override;

private:
    PluginTimer *m_updateTimer = nullptr;
    KeContactDataLayer *m_kebaDataLayer = nullptr;

    QHash<ThingId, KeContact *> m_kebaDevices;
    QHash<Thing *, NetworkDeviceMonitor *> m_monitors;
    QHash<ThingId, int> m_lastSessionId;
    QHash<QUuid, ThingActionInfo *> m_asyncActions;
    KebaDiscovery *m_runningDiscovery = nullptr;

    QHash<ThingClassId, ParamTypeId> m_macAddressParamTypeIds;
    QHash<ThingClassId, ParamTypeId> m_hostNameParamTypeIds;
    QHash<ThingClassId, ParamTypeId> m_addressParamTypeIds;
    QHash<ThingClassId, ParamTypeId> m_modelParamTypeIds;
    QHash<ThingClassId, ParamTypeId> m_serialNumberParamTypeIds;

    void setupKeba(ThingSetupInfo *info, const QHostAddress &address);

    void setDeviceState(Thing *device, KeContact::State state);
    void setDevicePlugState(Thing *device, KeContact::PlugState plugState);

    void refresh(Thing *thing, KeContact *keba);

private slots:
    void onCommandExecuted(QUuid requestId, bool success);
    void onReportTwoReceived(const KeContact::ReportTwo &reportTwo);
    void onReportThreeReceived(const KeContact::ReportThree &reportThree);
    void onReport1XXReceived(int reportNumber, const KeContact::Report1XX &report);
    void onBroadcastReceived(KeContact::BroadcastType type, const QVariant &content);
};

#endif // INTEGRATIONPLUGINKEBA_H
