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

#ifndef INTEGRATIONPLUGINMEROSS_H
#define INTEGRATIONPLUGINMEROSS_H

#include <integrations/integrationplugin.h>

#include "extern-plugininfo.h"

class PluginTimer;
class NetworkDeviceMonitor;
class QNetworkReply;

class IntegrationPluginMeross: public IntegrationPlugin
{
    Q_OBJECT

    Q_PLUGIN_METADATA(IID "io.nymea.IntegrationPlugin" FILE "integrationpluginmeross.json")
    Q_INTERFACES(IntegrationPlugin)

public:
    enum Method {
        GET,
        SET
    };
    Q_ENUM(Method)

    explicit IntegrationPluginMeross();
    ~IntegrationPluginMeross();

    void discoverThings(ThingDiscoveryInfo *info) override;
    void startPairing(ThingPairingInfo *info) override;
    void confirmPairing(ThingPairingInfo *info, const QString &username, const QString &secret) override;

    void setupThing(ThingSetupInfo *info) override;
    void postSetupThing(Thing *thing) override;
    void thingRemoved(Thing *thing) override;

    void executeAction(ThingActionInfo *info) override;

private slots:
    void pollDevice5s(Thing *thing);
    void pollDevice60s(Thing *thing);

private:
    void retrieveKey();

    QNetworkReply *request(Thing *thing, const QString &nameSpace, Method method = GET, const QVariantMap &payload = QVariantMap());

    QHash<Thing *, QByteArray> m_keys;
    QHash<Thing *, NetworkDeviceMonitor *> m_monitors;
    PluginTimer *m_timer5s = nullptr;
    PluginTimer *m_timer60s = nullptr;
};

#endif // INTEGRATIONPLUGINMEROSS_H
