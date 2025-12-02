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

#ifndef INTEGRATIONPLUGINLIFX_H
#define INTEGRATIONPLUGINLIFX_H

#include <integrations/integrationplugin.h>

#include <plugintimer.h>
#include <network/networkaccessmanager.h>
#include <network/zeroconf/zeroconfservicebrowser.h>
#include <network/zeroconf/zeroconfserviceentry.h>

#include <QTimer>

#include "lifxlan.h"
#include "lifxcloud.h"

class IntegrationPluginLifx : public IntegrationPlugin
{
    Q_OBJECT

    Q_PLUGIN_METADATA(IID "io.nymea.IntegrationPlugin" FILE "integrationpluginlifx.json")
    Q_INTERFACES(IntegrationPlugin)

public:
    explicit IntegrationPluginLifx();

    void init() override;
    void startPairing(ThingPairingInfo *info) override;
    void confirmPairing(ThingPairingInfo *info, const QString &username, const QString &secret) override;

    void discoverThings(ThingDiscoveryInfo *info) override;
    void setupThing(ThingSetupInfo *info) override;
    void postSetupThing(Thing *thing) override;
    void executeAction(ThingActionInfo *info) override;
    void thingRemoved(Thing *thing) override;

    void browseThing(BrowseResult *result) override;
    void browserItem(BrowserItemResult *result) override;
    void executeBrowserItem(BrowserActionInfo *info) override;

private:
    NetworkAccessManager *m_networkManager = nullptr;
    PluginTimer *m_pluginTimer = nullptr;
    QHash<LifxCloud *, ThingSetupInfo *> m_asyncCloudSetups;
    QHash<int, ThingActionInfo *> m_asyncActions;
    QHash<Thing *, LifxLan *> m_lifxLanConnections;
    QHash<Thing *, LifxCloud *> m_lifxCloudConnections;
    QHash<LifxCloud *, BrowseResult *> m_asyncBrowseResults;
    QHash<int, BrowserActionInfo *> m_asyncBrowserItem;

    ZeroConfServiceBrowser *m_serviceBrowser = nullptr;

    QHash<ThingClassId, StateTypeId> m_connectedStateTypeIds;
    QHash<ThingClassId, StateTypeId> m_powerStateTypeIds;
    QHash<ThingClassId, StateTypeId> m_brightnessStateTypeIds;
    QHash<ThingClassId, StateTypeId> m_colorTemperatureStateTypeIds;
    QHash<ThingClassId, ParamTypeId> m_hostAddressParamTypeIds;
    QHash<ThingClassId, ParamTypeId> m_portParamTypeIds;
    QHash<ThingClassId, ParamTypeId> m_idParamTypeIds;

    QHash<ThingId, ThingActionInfo *> m_pendingBrightnessAction;
    QHash<int, LifxLan::LifxProduct> m_lifxProducts;

private slots:
    void onLifxLanConnectionChanged(bool connected);
    void onLifxLanRequestExecuted(int requestId, bool success);

    void onLifxCloudConnectionChanged(bool connected);
    void onLifxCloudAuthenticationChanged(bool authenticated);
    void onLifxCloudRequestExecuted(int requestId, bool success);
    void onLifxCloudLightsListReceived(const QList<LifxCloud::Light> &lights);
    void onLifxCloudScenesListReceived(const QList<LifxCloud::Scene> &scenes);
};

#endif // INTEGRATIONPLUGIN_LIFX_H
