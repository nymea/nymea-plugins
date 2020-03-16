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

#ifndef INTEGRATIONPLUGINBLUOS_H
#define INTEGRATIONPLUGINBLUOS_H

#include "bluos.h"

#include "integrations/integrationplugin.h"
#include "platform/platformzeroconfcontroller.h"
#include "network/zeroconf/zeroconfservicebrowser.h"

#include <QUdpSocket>
#include <QNetworkAccessManager>

class PluginTimer;

class IntegrationPluginBluOS: public IntegrationPlugin
{
    Q_OBJECT

    Q_PLUGIN_METADATA(IID "io.nymea.IntegrationPlugin" FILE "integrationpluginbluos.json")
    Q_INTERFACES(IntegrationPlugin)

public:
    explicit IntegrationPluginBluOS();
    ~IntegrationPluginBluOS();

    void init() override;
    void discoverThings(ThingDiscoveryInfo *info) override;
    void setupThing(ThingSetupInfo *info) override;
    void postSetupThing(Thing *thing) override;
    void thingRemoved(Thing *thing) override;
    void executeAction(ThingActionInfo *info) override;

    void browseThing(BrowseResult *result) override;
    void browserItem(BrowserItemResult *result) override;
    void executeBrowserItem(BrowserActionInfo *info) override;

private:
    PluginTimer *m_pluginTimer = nullptr;
    ZeroConfServiceBrowser *m_serviceBrowser = nullptr;

    QHash<ThingId, BluOS *> m_bluos;
    QHash<BluOS *, ThingSetupInfo *> m_asyncSetup;
    QHash<QUuid, ThingActionInfo *> m_asyncActions;
    QHash<QUuid, BrowseResult *> m_asyncBrowseResults;
    QHash<QUuid, BrowserActionInfo *> m_asyncExecuteBrowseItems;
    QHash<QUuid, BrowserItemResult *> m_asyncBrowseItemResults;

private slots:
    void onPluginTimer();

    void onConnectionChanged(bool connected);
    void onStatusResponseReceived(const BluOS::StatusResponse &status);
    void onActionExecuted(QUuid actionId, bool success);
    void onVolumeReceived(int volume, bool mute);
};

#endif // INTEGRATIONPLUGINBLUOS_H
