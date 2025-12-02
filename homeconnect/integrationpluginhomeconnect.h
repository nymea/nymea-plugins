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

#ifndef INTEGRATIONPLUGINHOMECONNECT_H
#define INTEGRATIONPLUGINHOMECONNECT_H

#include <integrations/integrationplugin.h>
#include <plugintimer.h>

#include <QHash>
#include <QDebug>

#include "homeconnect.h"

class IntegrationPluginHomeConnect : public IntegrationPlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "io.nymea.IntegrationPlugin" FILE "integrationpluginhomeconnect.json")
    Q_INTERFACES(IntegrationPlugin)

public:
    explicit IntegrationPluginHomeConnect();

    void startPairing(ThingPairingInfo *info) override;
    void confirmPairing(ThingPairingInfo *info, const QString &username, const QString &secret) override;

    void setupThing(ThingSetupInfo *info) override;
    void postSetupThing(Thing *thing) override;
    void executeAction(ThingActionInfo *info) override;
    void thingRemoved(Thing *thing) override;

    void browseThing(BrowseResult *result) override;
    void browserItem(BrowserItemResult *result) override;
    void executeBrowserItem(BrowserActionInfo *info) override;

private:
    PluginTimer *m_pluginTimer15min = nullptr;

    QHash<HomeConnect *, ThingSetupInfo *> m_asyncSetup;

    QHash<ThingId, HomeConnect *> m_setupHomeConnectConnections;
    QHash<Thing *, HomeConnect *> m_homeConnectConnections;

    QHash<QUuid, ThingActionInfo *> m_pendingActions;
    QHash<Thing *, QString> m_selectedProgram;

    QHash<ThingClassId, ParamTypeId> m_idParamTypeIds;

    QHash<ThingClassId, StateTypeId> m_connectedStateTypeIds;
    QHash<ThingClassId, StateTypeId> m_doorStateStateTypeIds;
    QHash<ThingClassId, StateTypeId> m_localControlStateTypeIds;
    QHash<ThingClassId, StateTypeId> m_remoteControlActivationStateTypeIds;
    QHash<ThingClassId, StateTypeId> m_remoteStartAllowanceStateTypeIds;
    QHash<ThingClassId, StateTypeId> m_operationStateTypeIds;
    QHash<ThingClassId, StateTypeId> m_doorStateTypeIds;
    QHash<ThingClassId, StateTypeId> m_selectedProgramStateTypeIds;
    QHash<ThingClassId, StateTypeId> m_progressStateTypeIds;
    QHash<ThingClassId, StateTypeId> m_endTimerStateTypeIds;

    QHash<ThingClassId, ActionTypeId> m_startActionTypeIds;
    QHash<ThingClassId, ActionTypeId> m_stopActionTypeIds;

    QHash<ThingClassId, EventTypeId> m_programFinishedEventTypeIds;

    QHash<QString, QString> m_coffeeStrengthTypes;

    void parseKey(Thing *thing, const QString &key, const QVariant &value);
    void parseSettingKey(Thing *thing, const QString &key, const QVariant &value);
    bool checkIfActionIsPossible(ThingActionInfo *info);

private slots:
    void onConnectionChanged(bool connected);
    void onAuthenticationStatusChanged(bool authenticated);
    void onRequestExecuted(QUuid requestId, bool success);
    void onReceivedHomeAppliances(const QList<HomeConnect::HomeAppliance> &appliances);
    void onReceivedStatusList(const QString &haId, const QHash<QString, QVariant> &statusList);
    void onReceivedEvents(HomeConnect::EventType eventType, const QString &haId, const QList<HomeConnect::Event> &events);
    void onReceivedSelectedProgram(const QString &haId, const QString &key, const QHash<QString, QVariant> &options);
    void onReceivedSettings(const QString &haId, const QHash<QString, QVariant> &settings);
};

#endif // INTEGRATIONPLUGINHOMECONNECT_H
