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

#ifndef INTEGRATIONPLUGINNETATMO_H
#define INTEGRATIONPLUGINNETATMO_H

#include <integrations/integrationplugin.h>

#include "extern-plugininfo.h"

#include <QHash>
#include <QTimer>

class PluginTimer;
class NetatmoConnection;

class IntegrationPluginNetatmo : public IntegrationPlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "io.nymea.IntegrationPlugin" FILE "integrationpluginnetatmo.json")
    Q_INTERFACES(IntegrationPlugin)

public:
    explicit IntegrationPluginNetatmo();

    void init() override;

    void startPairing(ThingPairingInfo *info) override;
    void confirmPairing(ThingPairingInfo *info, const QString &username, const QString &secret) override;

    void setupThing(ThingSetupInfo *info) override;
    void postSetupThing(Thing *thing) override;
    void thingRemoved(Thing *thing) override;

private:
    QByteArray m_clientId;
    QByteArray m_clientSecret;

    PluginTimer *m_pluginTimer = nullptr;

    QHash<Thing *, NetatmoConnection *> m_connections;
    QHash<ThingId, NetatmoConnection *> m_pendingSetups;

    QHash<QString, QVariantMap> m_temporaryInitData;

    void setupConnection(ThingSetupInfo *info);
    void refreshConnection(Thing *thing);
    void processRefreshData(Thing *connectionThing, const QVariantList &devices);

    Thing *findIndoorStationThing(const QString &macAddress);
    Thing *findOutdoorModuleThing(const QString &macAddress);
    Thing *findIndoorModuleThing(const QString &macAddress);
    Thing *findWindModuleThing(const QString &macAddress);
    Thing *findRainGaugeModuleThing(const QString &macAddress);

    void updateModuleStates(Thing *thing, const QVariantMap &data);

    bool doingLoginMigration(ThingSetupInfo *info);

    bool loadClientCredentials();
    QString censorDebugOutput(const QString &text);

};

#endif // INTEGRATIONPLUGINNETATMO_H
