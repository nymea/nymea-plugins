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

#ifndef INTEGRATIONPLUGINPOWERFOX_H
#define INTEGRATIONPLUGINPOWERFOX_H

#include <integrations/integrationplugin.h>

#include "extern-plugininfo.h"

#include <QUrlQuery>

class PluginTimer;
class QNetworkReply;

class IntegrationPluginPowerfox: public IntegrationPlugin
{
    Q_OBJECT

    Q_PLUGIN_METADATA(IID "io.nymea.IntegrationPlugin" FILE "integrationpluginpowerfox.json")
    Q_INTERFACES(IntegrationPlugin)

public:
    enum Division {
        DivisionUnknown = -1,
        DivisionPowerMeter = 0,
        DivisionColdWaterMeter = 1,
        DivisionWarmWaterMeter = 2,
        DivisionHeatMeter = 3,
        DivisionGasMeter = 4,
        DivisionColdAndWarmWaterMeter = 5
    };
    Q_ENUM(Division)

    explicit IntegrationPluginPowerfox();
    ~IntegrationPluginPowerfox();

    void startPairing(ThingPairingInfo *info) override;
    void confirmPairing(ThingPairingInfo *info, const QString &username, const QString &secret) override;
    void setupThing(ThingSetupInfo *info) override;
    void postSetupThing(Thing *thing) override;
    void thingRemoved(Thing *thing) override;

private:
    QNetworkReply *request(Thing *thing, const QString &path, const QUrlQuery &query = QUrlQuery());
    void markAsDisconnected(Thing *thing);

private:
    PluginTimer *m_pollTimer = nullptr;
};

#endif // INTEGRATIONPLUGINPOWERFOX_H
