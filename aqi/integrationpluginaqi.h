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

#ifndef INTEGRATIONPLUGINAQI_H
#define INTEGRATIONPLUGINAQI_H

#include <plugintimer.h>
#include <integrations/integrationplugin.h>
#include <network/networkaccessmanager.h>

#include "airqualityindex.h"
#include "extern-plugininfo.h"

#include <QTimer>
#include <QUrl>
#include <QHostAddress>

class IntegrationPluginAqi : public IntegrationPlugin
{
    Q_OBJECT

    Q_PLUGIN_METADATA(IID "io.nymea.IntegrationPlugin" FILE "integrationpluginaqi.json")
    Q_INTERFACES(IntegrationPlugin)

public:
    explicit IntegrationPluginAqi();

    void discoverThings(ThingDiscoveryInfo *info) override;
    void setupThing(ThingSetupInfo *info) override;
    void thingRemoved(Thing *thing) override;
    void postSetupThing(Thing *thing) override;

private:
    PluginTimer *m_pluginTimer = nullptr;
    AirQualityIndex *m_aqiConnection = nullptr;

    QHash<QUuid, ThingDiscoveryInfo *> m_asyncDiscovery;
    QHash<QUuid, ThingSetupInfo *> m_asyncSetups;
    QHash<QUuid, ThingId> m_asyncRequests;

    QString getApiKey();
    bool createAqiConnection();

    double convertFromAQI(int aqi, const QList<QPair<int, double> > &map) const;

private slots:
    void onPluginTimer();
    void onRequestExecuted(QUuid requestId, bool success);
    void onAirQualityDataReceived(QUuid requestId, AirQualityIndex::AirQualityData data);
    void onAirQualityStationsReceived(QUuid requestId, QList<AirQualityIndex::Station> stations);

};

#endif // INTEGRATIONPLUGINAQI_H
