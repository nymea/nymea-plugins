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

#ifndef INTEGRATIONPLUGINESPUINO_H
#define INTEGRATIONPLUGINESPUINO_H

#include <integrations/integrationplugin.h>
#include <plugintimer.h>

#include <QHash>

#include "extern-plugininfo.h"

class MqttChannel;
class ZeroConfServiceBrowser;

class IntegrationPluginEspuino : public IntegrationPlugin
{
    Q_OBJECT

    Q_PLUGIN_METADATA(IID "io.nymea.IntegrationPlugin" FILE "integrationpluginespuino.json")
    Q_INTERFACES(IntegrationPlugin)

public:
    void init() override;
    void discoverThings(ThingDiscoveryInfo *info) override;
    void setupThing(ThingSetupInfo *info) override;
    void thingRemoved(Thing *thing) override;
    void executeAction(ThingActionInfo *info) override;
    void browseThing(BrowseResult *result) override;
    void browseThing(BrowseResult *result, const QString &path);
    void browserItem(BrowserItemResult *result) override;
    BrowserItem browserItemFromQuery(const QUrlQuery &query);
    void executeBrowserItem(BrowserActionInfo *info) override;
    void executeBrowserItemAction(BrowserItemActionInfo *info) override;

private slots:
    void onClientConnected(MqttChannel *channel);
    void onClientDisconnected(MqttChannel *channel);
    void onPublishReceived(MqttChannel* channel, const QString &topic, const QByteArray &payload);

private:
    QString getHost(Thing *thing) const;

    ZeroConfServiceBrowser *m_zeroConfBrowser = nullptr;
    QHash<Thing *, MqttChannel *> m_mqttChannels;
    QHash<ThingClassId, ParamTypeId> m_ipAddressParamTypeMap;
    QMap<QString, QPointer<ThingActionInfo>> m_pendingActions;
};

#endif // INTEGRATIONPLUGINESPUINO_H
