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

#ifndef INTEGRATIONPLUGINDOORBIRD_H
#define INTEGRATIONPLUGINDOORBIRD_H

#include <QImage>

#include <integrations/integrationplugin.h>
#include <network/zeroconf/zeroconfservicebrowser.h>

#include "doorbird.h"

class QNetworkAccessManager;
class QNetworkReply;

class IntegrationPluginDoorbird: public IntegrationPlugin
{
    Q_OBJECT

    Q_PLUGIN_METADATA(IID "io.nymea.IntegrationPlugin" FILE "integrationplugindoorbird.json")
    Q_INTERFACES(IntegrationPlugin)

public:
    explicit IntegrationPluginDoorbird();

    void init() override;
    void discoverThings(ThingDiscoveryInfo *info) override;
    void setupThing(ThingSetupInfo *info) override;
    void postSetupThing(Thing *thing) override;
    void executeAction(ThingActionInfo *info) override;

    void startPairing(ThingPairingInfo *info) override;
    void confirmPairing(ThingPairingInfo *info, const QString &username, const QString &secret) override;

    void thingRemoved(Thing *thing)override;

private:

    ZeroConfServiceBrowser *m_serviceBrowser = nullptr;

    QHash<ThingId, Doorbird *> m_pairingConnections;
    QHash<ThingId, Doorbird *> m_doorbirdConnections;

    QHash<QUuid, ThingActionInfo *> m_asyncActions;

private slots:
    void onDoorBirdConnected(bool status);
    void onDoorBirdEvent(Doorbird::EventType eventType, bool status);
    void onDoorBirdRequestSent(QUuid requestId, bool success);
};

#endif // INTEGRATIONPLUGINDOORBIRD_H
