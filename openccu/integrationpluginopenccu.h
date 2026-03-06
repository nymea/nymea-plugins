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

#ifndef INTEGRATIONPLUGINOPENCCU_H
#define INTEGRATIONPLUGINOPENCCU_H

#include <integrations/integrationplugin.h>
#include <plugintimer.h>

#include <QSslError>
#include <QUrlQuery>
#include <QXmlStreamReader>

#include "extern-plugininfo.h"

class IntegrationPluginOpenCCU : public IntegrationPlugin
{
    Q_OBJECT

    Q_PLUGIN_METADATA(IID "io.nymea.IntegrationPlugin" FILE "integrationpluginopenccu.json")
    Q_INTERFACES(IntegrationPlugin)

public:
    explicit IntegrationPluginOpenCCU();

    void init() override;
    void discoverThings(ThingDiscoveryInfo *info) override;
    void setupThing(ThingSetupInfo *info) override;
    void postSetupThing(Thing *thing) override;
    void thingRemoved(Thing *thing) override;
    void executeAction(ThingActionInfo *info) override;

private:
    struct DeviceInfo {
        int iseId = -1;
        QString name;
        QString interface;
        QString deviceType;
        QString serialNumber;
    };

    struct ThermostatChannel {
        int index = 1;
        int iseId = -1;
        int targetTemperatureId = -1;
        int modeId = -1;
        int controlModeId = -1;
    };

    struct Thermostat {
        QHash<int, ThermostatChannel> channels;
    };

    PluginTimer *m_pluginTimer = nullptr;
    QHash<Thing *, Thermostat> m_thermostats;
    QHash<Thing *, bool> m_usingSsl;

    void getDevices(Thing *thing);
    void getDeviceTypeList(Thing *thing);
    void getState(Thing *thing);
    void getStateList(Thing *thing);
    void getChannels(Thing *floorHeatinController);

    void processThingStateList(QXmlStreamReader *xml, Thing *thing);

    int getSignalStrenthFromRssi(int rssi);

    QUrl buildUrl(Thing *thing, const QString &method, const QUrlQuery &query = QUrlQuery());

private slots:
    void onSslError(const QList<QSslError> &errors);

};

#endif // INTEGRATIONPLUGINOPENCCU_H
