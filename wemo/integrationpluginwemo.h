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

#ifndef INTEGRATIONPLUGINWEMO_H
#define INTEGRATIONPLUGINWEMO_H

#include <integrations/integrationplugin.h>

class QNetworkReply;
class PluginTimer;

class IntegrationPluginWemo : public IntegrationPlugin
{
    Q_OBJECT

    Q_PLUGIN_METADATA(IID "io.nymea.IntegrationPlugin" FILE "integrationpluginwemo.json")
    Q_INTERFACES(IntegrationPlugin)

public:
    explicit IntegrationPluginWemo();
    ~IntegrationPluginWemo();

    void init() override;
    void discoverThings(ThingDiscoveryInfo *info) override;
    void setupThing(ThingSetupInfo *info) override;
    void executeAction(ThingActionInfo *info) override;
    void thingRemoved(Thing *thing) override;

private:
    PluginTimer *m_pluginTimer = nullptr;
    QHash<QNetworkReply *, Thing *> m_refreshReplies;

    void refresh(Thing* thing);

    void processRefreshData(const QByteArray &data, Thing *thing);

private slots:
    void onNetworkReplyFinished();
    void onPluginTimer();
    void onUpnpDiscoveryFinished();
    void onUpnpNotifyReceived(const QByteArray &notification);

};

#endif // INTEGRATIONPLUGINWEMO_H
