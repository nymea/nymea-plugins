/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *                                                                         *
 *  Copyright (C) 2020 devendragajjar <devendragajjar@gmail.com>                 *
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

#define ENABLE_RENEWEBLE_API 0

class v2xe_amber_electric: public IntegrationPlugin
{
    Q_OBJECT

    Q_PLUGIN_METADATA(IID "io.nymea.IntegrationPlugin" FILE "integrationpluginv2xe_amber_electric.json")
    Q_INTERFACES(IntegrationPlugin)


public:
    explicit v2xe_amber_electric();
    ~v2xe_amber_electric();

    void init() override;

    void setupThing(ThingSetupInfo *info) override;

    void executeAction(ThingActionInfo *info) override;

    void thingRemoved(Thing *thing) override;

private slots:
    void onPluginTimer();
    void requestSitePriceData(Thing* thing, ThingSetupInfo* setup = nullptr);
    void requestSiteData(Thing* thing, ThingSetupInfo* setup = nullptr);
#if ENABLE_RENEWEBLE_API
    void requestRenewablesData(Thing* thing, ThingSetupInfo* setup = nullptr);
#endif

private:
    PluginTimer *m_pluginTimer = nullptr;
    QString m_consumerKey;
    QString m_current_site;
    QHash<ThingClassId, QString> m_serverUrls;
    QHash<ThingClassId, StateTypeId> m_connectedStateTypeIds;
    QHash<ThingClassId, StateTypeId> m_forecastPriceStateTypeIds;
    QHash<ThingClassId, StateTypeId> m_currentPriceStateTypeIds;
    QHash<ThingClassId, StateTypeId> m_currentSiteTypeIds;
    QHash<ThingClassId, StateTypeId> m_currentfeddinStateTypeIds;
    QHash<ThingClassId, StateTypeId> m_fururefeedinStateTypeIds;

    void onSiteDataReceived(Thing* thing, ThingSetupInfo* setup = nullptr, QNetworkReply* reply = nullptr);
    void onSitePriceDataReceived(Thing* thing, ThingSetupInfo* setup = nullptr, QNetworkReply* reply = nullptr);
#if ENABLE_RENEWEBLE_API
    void onRenewablesDataReceived(Thing* thing, ThingSetupInfo* setup = nullptr, QNetworkReply* reply = nullptr);
#endif
};

#endif // INTEGRATIONPLUGINEV_CHARGER_H
