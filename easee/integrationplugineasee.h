/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *                                                                         *
 *  Copyright (C) 2022 Tim Stöcker <t.stoecker@consolinno.de>                 *
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

#ifndef INTEGRATIONPLUGINEASEE_H
#define INTEGRATIONPLUGINEASEE_H

#include "integrations/integrationplugin.h"
#include <QString>
class PluginTimer;
#include <QNetworkRequest>

class IntegrationPluginEasee: public IntegrationPlugin
{
    Q_OBJECT

    Q_PLUGIN_METADATA(IID "io.nymea.IntegrationPlugin" FILE "integrationplugineasee.json")
    Q_INTERFACES(IntegrationPlugin)


public:
    explicit IntegrationPluginEasee();

    void init() override;

    void startPairing(ThingPairingInfo *info) override;

    void confirmPairing(ThingPairingInfo *info, const QString &username, const QString &secret) override;

    void setupThing(ThingSetupInfo *info) override;

    void postSetupThing(Thing *thing) override;

    void executeAction(ThingActionInfo *info) override;

    void thingRemoved(Thing *thing) override;


private slots:
    void refreshAccessToken(Thing *thing);
    void getSiteAndCircuit(Thing *thing);
    void getCurrentPower(Thing *thing);
    void writeCurrentLimit(Thing *thing);


private:
    QNetworkRequest composeApiKeyRequest();
    QNetworkRequest composeSiteAndCircuitRequest(const QString &chargerId);
    QNetworkRequest composeCurrentPowerRequest(const QString &chargerId);
    QNetworkRequest composeCurrentLimitRequest();
    QString accessKey;
    PluginTimer *m_timer = nullptr;
    PluginTimer *access_timer = nullptr;
    double siteId = 0;
    double circuitId = 0;
};

#endif // INTEGRATIONPLUGINEASEE_H
