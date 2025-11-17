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

#ifndef INTEGRATIONPLUGINSENIC_H
#define INTEGRATIONPLUGINSENIC_H

#include <integrations/integrationplugin.h>
#include <plugintimer.h>

#include "nuimo.h"

class IntegrationPluginSenic : public IntegrationPlugin
{
    Q_OBJECT

    Q_PLUGIN_METADATA(IID "io.nymea.IntegrationPlugin" FILE "integrationpluginsenic.json")
    Q_INTERFACES(IntegrationPlugin)

public:
    explicit IntegrationPluginSenic();

    void init() override;
    void discoverThings(ThingDiscoveryInfo *info) override;
    void setupThing(ThingSetupInfo *info) override;
    void postSetupThing(Thing *thing) override;
    void executeAction(ThingActionInfo *info) override;

    void thingRemoved(Thing *thing) override;

private:
    QHash<Nuimo *, Thing *> m_nuimos;
    PluginTimer *m_reconnectTimer = nullptr;
    bool m_autoSymbolMode = true;

private slots:
    void onPluginConfigurationChanged(const ParamTypeId &paramTypeId, const QVariant &value);
    void onReconnectTimeout();

    void onConnectedChanged(bool connected);
    void onBatteryValueChanged(uint percentage);
    void onButtonPressed();
    void onButtonLongPressed();
    void onSwipeDetected(Nuimo::SwipeDirection direction);
    void onRotationValueChanged(uint value);
    void onDeviceInformationChanged(const QString &firmwareRevision, const QString &hardwareRevision, const QString &softwareRevision);
};

#endif // INTEGRATIONPLUGINSENIC_H
