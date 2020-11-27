/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*
* Copyright 2013 - 2020, nymea GmbH
* Contact: contact@nymea.io
*
* This file is part of nymea.
* This project including source code and documentation is protected by
* copyright law, and remains the property of nymea GmbH. All rights, including
* reproduction, publication, editing and translation, are reserved. The use of
* this project is subject to the terms of a license agreement to be concluded
* with nymea GmbH in accordance with the terms of use of nymea GmbH, available
* under https://nymea.io/license
*
* GNU Lesser General Public License Usage
* Alternatively, this project may be redistributed and/or modified under the
* terms of the GNU Lesser General Public License as published by the Free
* Software Foundation; version 3. This project is distributed in the hope that
* it will be useful, but WITHOUT ANY WARRANTY; without even the implied
* warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
* Lesser General Public License for more details.
*
* You should have received a copy of the GNU Lesser General Public License
* along with this project. If not, see <https://www.gnu.org/licenses/>.
*
* For any further details and any questions please contact us under
* contact@nymea.io or see our FAQ/Licensing Information on
* https://nymea.io/license/faq
*
* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

#ifndef INTEGRATIONPLUGINMIELE_H
#define INTEGRATIONPLUGINMIELE_H

#include "integrations/integrationplugin.h"
#include "plugintimer.h"
#include "miele.h"

#include <QHash>
#include <QDebug>

class IntegrationPluginMiele : public IntegrationPlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "io.nymea.IntegrationPlugin" FILE "integrationpluginmiele.json")
    Q_INTERFACES(IntegrationPlugin)

public:
    explicit IntegrationPluginMiele();

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

    QHash<Miele *, ThingSetupInfo *> m_asyncSetup;

    QHash<ThingId, Miele *> m_setupMieleConnections;
    QHash<Thing *, Miele *> m_mieleConnections;

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
};

#endif // INTEGRATIONPLUGINMIELE_H
