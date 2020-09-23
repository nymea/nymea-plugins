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

#ifndef INTEGRATIONPLUGINLIFX_H
#define INTEGRATIONPLUGINLIFX_H

#include "integrations/integrationplugin.h"
#include "plugintimer.h"
#include "lifxlan.h"
#include "lifxcloud.h"

#include "network/networkaccessmanager.h"
#include "network/zeroconf/zeroconfservicebrowser.h"
#include "network/zeroconf/zeroconfserviceentry.h"

#include <QTimer>

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
