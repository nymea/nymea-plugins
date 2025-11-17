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

#ifndef INTEGRATIONPLUGINTASMOTA_H
#define INTEGRATIONPLUGINTASMOTA_H

#include <integrations/integrationplugin.h>

#include "extern-plugininfo.h"

class MqttChannel;

class IntegrationPluginTasmota: public IntegrationPlugin
{
    Q_OBJECT

    Q_PLUGIN_METADATA(IID "io.nymea.IntegrationPlugin" FILE "integrationplugintasmota.json")
    Q_INTERFACES(IntegrationPlugin)

public:
    explicit IntegrationPluginTasmota();
    ~IntegrationPluginTasmota();

    void init() override;
    void setupThing(ThingSetupInfo *info) override;
    void thingRemoved(Thing *thing) override;
    void executeAction(ThingActionInfo *info) override;

private slots:
    void onClientConnected(MqttChannel *channel);
    void onClientDisconnected(MqttChannel *channel);
    void onPublishReceived(MqttChannel *channel, const QString &topic, const QByteArray &payload);

private:
    QHash<Thing *, MqttChannel *> m_mqttChannels;

    // Helpers for parent devices (the ones starting with sonoff)
    QHash<ThingClassId, ParamTypeId> m_ipAddressParamTypeMap;
    QHash<ThingClassId, QList<ParamTypeId> > m_attachedDeviceParamTypeIdMap;

    // Helpers for child devices (virtual ones, starting with tasmota)
    QHash<ThingClassId, ParamTypeId> m_channelParamTypeMap;
    QHash<ThingClassId, ParamTypeId> m_openingChannelParamTypeMap;
    QHash<ThingClassId, ParamTypeId> m_closingChannelParamTypeMap;

    QHash<ThingClassId, ActionTypeId> m_closableOpenActionTypeMap;
    QHash<ThingClassId, ActionTypeId> m_closableCloseActionTypeMap;
    QHash<ThingClassId, ActionTypeId> m_closableStopActionTypeMap;
};

#endif // INTEGRATIONPLUGINTASMOTA_H
