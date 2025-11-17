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

#ifndef INTEGRATIONPLUGINLGSMARTTV_H
#define INTEGRATIONPLUGINLGSMARTTV_H

#include <integrations/integrationplugin.h>

#include <network/upnp/upnpdevicedescriptor.h>
#include <plugintimer.h>

#include <QNetworkReply>

class TvDevice;

class IntegrationPluginLgSmartTv : public IntegrationPlugin
{
    Q_OBJECT

    Q_PLUGIN_METADATA(IID "io.nymea.IntegrationPlugin" FILE "integrationpluginlgsmarttv.json")
    Q_INTERFACES(IntegrationPlugin)

public:
    explicit IntegrationPluginLgSmartTv();
    ~IntegrationPluginLgSmartTv();

    void init() override;

    void discoverThings(ThingDiscoveryInfo *info) override;

    void startPairing(ThingPairingInfo *info) override;
    void confirmPairing(ThingPairingInfo *info, const QString &username, const QString &secret) override;

    void setupThing(ThingSetupInfo *info) override;
    void postSetupThing(Thing *thing) override;
    void thingRemoved(Thing *thing) override;

    void executeAction(ThingActionInfo *info) override;

private:
    PluginTimer *m_pluginTimer = nullptr;
    QHash<TvDevice *, Thing *> m_tvList;
    QHash<QString, QString> m_tvKeys;

    // update requests
    QHash<QNetworkReply *, Thing *> m_volumeInfoRequests;
    QHash<QNetworkReply *, Thing *> m_channelInfoRequests;

    void pairTvDevice(Thing *thing);
    void unpairTvDevice(Thing *thing);
    void refreshTv(Thing *thing);

private slots:
    void onPluginTimer();
    void onNetworkManagerReplyFinished();
    void stateChanged();
};

#endif // INTEGRATIONPLUGINLGSMARTTV_H
