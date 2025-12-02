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

#ifndef INTEGRATIONPLUGINMECMETER_H
#define INTEGRATIONPLUGINMECMETER_H

#include <integrations/integrationplugin.h>

class PluginTimer;
class ZeroConfServiceBrowser;

#include <QNetworkRequest>

class IntegrationPluginMecMeter: public IntegrationPlugin
{
    Q_OBJECT

    Q_PLUGIN_METADATA(IID "io.nymea.IntegrationPlugin" FILE "integrationpluginmecelectronics.json")
    Q_INTERFACES(IntegrationPlugin)

public:
    explicit IntegrationPluginMecMeter();
    ~IntegrationPluginMecMeter();

    void init() override;
    void discoverThings(ThingDiscoveryInfo *info) override;
    void startPairing(ThingPairingInfo *info) override;
    void confirmPairing(ThingPairingInfo *info, const QString &username, const QString &secret) override;
    void setupThing(ThingSetupInfo *info) override;
    void postSetupThing(Thing *thing) override;
    void thingRemoved(Thing *thing) override;

private slots:
    void refresh(Thing *thing);

private:
    QNetworkRequest composeRequest(const QString &meterId, const QString &userId, const QString &password);

    ZeroConfServiceBrowser *m_zeroConf = nullptr;
    PluginTimer *m_timer = nullptr;
};

#endif // INTEGRATIONPLUGINMECMETER_H
