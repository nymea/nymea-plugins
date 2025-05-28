/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *                                                                         *
 *  Copyright (C) 2025 devendragajjar <devendragajjar@gmail.com>                 *
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

#ifndef INTEGRATIONPLUGINEV_CHARGER_H
#define INTEGRATIONPLUGINEV_CHARGER_H

#include "integrations/integrationplugin.h"
#include "extern-plugininfo.h"
#include "plugintimer.h"
#include "network/networkaccessmanager.h"

#include <QHash>
#include <QDebug>
#include <QTimer>
#include <QPointer>
#include <QNetworkReply>


class V2xeAmberElectric: public IntegrationPlugin
{
    Q_OBJECT

    Q_PLUGIN_METADATA(IID "io.nymea.IntegrationPlugin" FILE "integrationpluginv2xeamberelectric.json")
    Q_INTERFACES(IntegrationPlugin)


public:
    explicit V2xeAmberElectric();
    ~V2xeAmberElectric();

    void init() override;

    void setupThing(ThingSetupInfo *info) override;

    void executeAction(ThingActionInfo *info) override;

    void thingRemoved(Thing *thing) override;

private slots:
    void onPluginTimer();
    void requestSitePriceData(Thing* thing, ThingSetupInfo* setup = nullptr);
    void requestSiteData(Thing* thing, ThingSetupInfo* setup = nullptr);

private:
    PluginTimer *mPluginTimer = nullptr;
    QString mConsumerKey;
    QString mCurrentSite;
    QHash<ThingClassId, QString> mServerUrls;

    void onSiteDataReceived(Thing* thing, ThingSetupInfo* setup = nullptr, QNetworkReply* reply = nullptr);
    void onSitePriceDataReceived(Thing* thing, ThingSetupInfo* setup = nullptr, QNetworkReply* reply = nullptr);
};

#endif // INTEGRATIONPLUGINEV_CHARGER_H
