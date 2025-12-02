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

#ifndef INTEGRATIONPLUGINSOMFYTAHOMA_H
#define INTEGRATIONPLUGINSOMFYTAHOMA_H

#include <integrations/integrationplugin.h>

#include "extern-plugininfo.h"

class PluginTimer;
class ZeroConfServiceBrowser;
class SomfyTahomaRequest;

class IntegrationPluginSomfyTahoma : public IntegrationPlugin
{
    Q_OBJECT

    Q_PLUGIN_METADATA(IID "io.nymea.IntegrationPlugin" FILE "integrationpluginsomfytahoma.json")
    Q_INTERFACES(IntegrationPlugin)

public:
    void init() override;
    void discoverThings(ThingDiscoveryInfo *info) override;

    void startPairing(ThingPairingInfo *info) override;
    void confirmPairing(ThingPairingInfo *info, const QString &username, const QString &password) override;

    void setupThing(ThingSetupInfo *info) override;
    void postSetupThing(Thing *thing) override;
    void thingRemoved(Thing *thing) override;

    void executeAction(ThingActionInfo *info) override;

private:
    void refreshGateway(Thing *thing);
    void handleEvents(const QVariantList &events);
    void updateThingStates(const QString &deviceUrl, const QVariantList &stateList);
    void markDisconnected(Thing *thing);
    void restoreChildConnectedState(Thing *thing);
    QString getHost(Thing *thing) const;
    QString getToken(Thing *thing) const;

private:
    ZeroConfServiceBrowser *m_zeroConfBrowser = nullptr;
    QMap<Thing *, PluginTimer *> m_eventPollTimer;
    QMap<QString, QPointer<ThingActionInfo>> m_pendingActions;
    QMap<QString, QList<Thing *>> m_currentExecutions;
};

#endif // INTEGRATIONPLUGINSOMFYTAHOMA_H
