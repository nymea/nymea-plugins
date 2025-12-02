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

#ifndef INTEGRATIONPLUGINAWATTAR_H
#define INTEGRATIONPLUGINAWATTAR_H

#include <integrations/integrationplugin.h>
#include <plugintimer.h>

#include <QHash>
#include <QDebug>
#include <QTimer>
#include <QPointer>
#include <QNetworkReply>

class IntegrationPluginAwattar : public IntegrationPlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "io.nymea.IntegrationPlugin" FILE "integrationpluginawattar.json")
    Q_INTERFACES(IntegrationPlugin)

public:
    explicit IntegrationPluginAwattar();
    ~IntegrationPluginAwattar();

    void setupThing(ThingSetupInfo *info) override;
    void thingRemoved(Thing *thing) override;

private slots:
    void onPluginTimer();
    void requestPriceData(Thing* thing, ThingSetupInfo *setup = nullptr);
    void processPriceData(Thing *thing, const QVariantMap &data);

private:
    PluginTimer *m_pluginTimer = nullptr;

    QHash<ThingClassId, QString> m_serverUrls;
    QHash<ThingClassId, StateTypeId> m_connectedStateTypeIds;
    QHash<ThingClassId, StateTypeId> m_currentMarketPriceStateTypeIds;
    QHash<ThingClassId, StateTypeId> m_validUntilStateTypeIds;
    QHash<ThingClassId, StateTypeId> m_averagePriceStateTypeIds;
    QHash<ThingClassId, StateTypeId> m_lowestPriceStateTypeIds;
    QHash<ThingClassId, StateTypeId> m_highestPriceStateTypeIds;
    QHash<ThingClassId, StateTypeId> m_averageDeviationStateTypeIds;
};

#endif // INTEGRATIONPLUGINAWATTAR_H
