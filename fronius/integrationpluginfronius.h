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

#ifndef INTEGRATIONPLUGINFRONIUS_H
#define INTEGRATIONPLUGINFRONIUS_H

#include "integrations/integrationplugin.h"
#include "froniuslogger.h"

#include <QHash>
#include <QNetworkReply>
#include <QTimer>
#include <QUuid>

class PluginTimer;

class IntegrationPluginFronius : public IntegrationPlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "io.nymea.IntegrationPlugin" FILE "integrationpluginfronius.json")
    Q_INTERFACES(IntegrationPlugin)

public:
    explicit IntegrationPluginFronius(QObject *parent = nullptr);

    void setupThing(ThingSetupInfo *thing) override;
    void postSetupThing(Thing* thing) override;
    void executeAction(ThingActionInfo *info) override;
    void thingRemoved(Thing* thing) override;

private:
    PluginTimer *m_pluginTimer = nullptr;

    QHash<FroniusLogger *, Thing *> m_froniusLoggers;
    QHash<FroniusInverter *, Thing *> m_froniusInverters;
    QHash<FroniusMeter *, Thing *> m_froniusMeters;
    QHash<FroniusStorage *, Thing *> m_froniusStorages;

    QHash<QUuid, ThingActionInfo *> m_asyncActions;

    void updateThingStates(Thing *thing);

    void searchNewThings(FroniusLogger *logger);
    bool thingExists(ParamTypeId thingParamId, QString thingId);

    void setupChild(ThingSetupInfo *info, Thing *parentThing);
};

#endif // INTEGRATIONPLUGINFRONIUS_H
