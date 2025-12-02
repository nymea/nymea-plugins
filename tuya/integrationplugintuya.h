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

#ifndef INTEGRATIONPLUGINTUYA_H
#define INTEGRATIONPLUGINTUYA_H

#include <QTimer>

#include <integrations/integrationplugin.h>

class PluginTimer;

class IntegrationPluginTuya: public IntegrationPlugin
{
    Q_OBJECT

    Q_PLUGIN_METADATA(IID "io.nymea.IntegrationPlugin" FILE "integrationplugintuya.json")
    Q_INTERFACES(IntegrationPlugin)


public:
    explicit IntegrationPluginTuya(QObject *parent = nullptr);
    ~IntegrationPluginTuya() override;

    void setupThing(ThingSetupInfo *info) override;
    void postSetupThing(Thing *thing) override;
    void thingRemoved(Thing *thing) override;
    void startPairing(ThingPairingInfo *info) override;
    void confirmPairing(ThingPairingInfo *info, const QString &username, const QString &secret) override;
    void executeAction(ThingActionInfo *info) override;

signals:
    void tokenRefreshed(Thing *thing, bool success);

private:
    void refreshAccessToken(Thing *thing);
    void updateChildDevices(Thing *thing);
    void queryDevice(Thing *thing);

    void controlTuyaSwitch(const QString &devId, const QString &command, const QVariant &value, ThingActionInfo *info);

    QHash<ThingId, QTimer *> m_tokenExpiryTimers;
    PluginTimer *m_pluginTimerQuery = nullptr;
    PluginTimer *m_pluginTimerDiscovery = nullptr;

    QHash<Thing *, QList<Thing *>> m_pollQueue;
};

#endif // INTEGRATIONPLUGINTUYA_H
