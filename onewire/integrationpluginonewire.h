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

#ifndef INTEGRATIONPLUGINONEWIRE_H
#define INTEGRATIONPLUGINONEWIRE_H

#include "plugintimer.h"
#include "integrations/integrationplugin.h"
#include "owfs.h"
#include "w1.h"

#include "extern-plugininfo.h"

#include <QHash>

class IntegrationPluginOneWire : public IntegrationPlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "io.nymea.IntegrationPlugin" FILE "integrationpluginonewire.json")
    Q_INTERFACES(IntegrationPlugin)

public:
    explicit IntegrationPluginOneWire();

    void discoverThings(ThingDiscoveryInfo *info) override;
    void setupThing(ThingSetupInfo *info) override;
    void postSetupThing(Thing *thing) override;
    void executeAction(ThingActionInfo *info) override;
    void thingRemoved(Thing *thing) override;

private:
    PluginTimer *m_pluginTimer = nullptr;
    Owfs *m_owfsInterface = nullptr;
    W1 *m_w1Interface = nullptr;

    QHash<Thing*, ThingDiscoveryInfo*> m_runningDiscoveries;

    void setupOwfsTemperatureSensor(ThingSetupInfo *info);
    void setupOwfsTemperatureHumiditySensor(ThingSetupInfo *info);

private slots:
    void onPluginTimer();
    void onOneWireDevicesDiscovered(QList<Owfs::OwfsDevice> devices);
};

#endif // INTEGRATIONPLUGINONEWIRE_H
