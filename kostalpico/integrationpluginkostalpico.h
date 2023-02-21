/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *                                                                         *
 *  Copyright (C) 2020 Consolinno Energy GmbH <f.stoecker@consolinno.de>                 *
 *                                                                         *
 *  This library is free software; you can redistribute it and/or          *
 *  modify it under the terms of the GNU Lesser General Public             *
 *  License as published by the Free Software Foundation;                  *
 *  version 3 of the License.                                              *
 *                                                                         *
 *  This library is distributed in the hope that it will be useful,        *
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of         *
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU      *
 *  Lesser General Public License for more details.                        *
 *                                                                         *
 *  You should have received a copy of the GNU Lesser General Public       *
 *  License along with this library; If not, see                           *
 *  <http://www.gnu.org/licenses/>.                                        *
 *                                                                         *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

#ifndef INTEGRATIONPLUGINKOSTALPICO_H
#define INTEGRATIONPLUGINKOSTALPICO_H

#include <integrations/integrationplugin.h>

#include "kostalpicoconnection.h"

#include <QObject>
#include <QHash>
#include <QNetworkReply>
#include <QTimer>
#include <QUuid>

class PluginTimer;

class IntegrationPluginKostal: public IntegrationPlugin
{
    Q_OBJECT

    Q_PLUGIN_METADATA(IID "io.nymea.IntegrationPlugin" FILE "integrationpluginkostalpico.json")
    Q_INTERFACES(IntegrationPlugin)


public:
    explicit IntegrationPluginKostal();

    //void init() override;

    void discoverThings(ThingDiscoveryInfo *info) override;

    void setupThing(ThingSetupInfo *info) override;

    void postSetupThing(Thing *info) override;

    void executeAction(ThingActionInfo *info) override;

    void thingRemoved(Thing *thing) override;

private:

    PluginTimer *m_connectionRefreshTimer = nullptr;

    KostalPicoConnection *m_kostalConnection;
    Thing *m_connectionThing = nullptr;

    void refreshConnection();
    //Consumption
    void updateCurrentPower(KostalPicoConnection *connection);
    //Production
    void updateTotalEnergyProduced(KostalPicoConnection *connection);
    bool m_toggle;
};

#endif // INTEGRATIONPLUGINKOSTALPICO_H
