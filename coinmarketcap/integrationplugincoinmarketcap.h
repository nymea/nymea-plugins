/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*
* Copyright 2013 - 2025, nymea GmbH
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

#ifndef INTEGRATIONPLUGINCOINMARKETCAP_H
#define INTEGRATIONPLUGINCOINMARKETCAP_H

#include <integrations/integrationplugin.h>
#include <plugintimer.h>

#include <QNetworkReply>
#include <QHostInfo>

class IntegrationPluginCoinMarketCap : public IntegrationPlugin
{
    Q_OBJECT

    Q_PLUGIN_METADATA(IID "io.nymea.IntegrationPlugin" FILE "integrationplugincoinmarketcap.json")
    Q_INTERFACES(IntegrationPlugin)

public:
    explicit IntegrationPluginCoinMarketCap();

    void startPairing(ThingPairingInfo *info) override;
    void confirmPairing(ThingPairingInfo *info, const QString &username, const QString &secret) override;
    void setupThing(ThingSetupInfo *info) override;
    void thingRemoved(Thing *thing) override;
    void postSetupThing(Thing *thing) override;

private:
    PluginTimer *m_pluginTimer = nullptr;
    QHash<ThingId, QByteArray> m_apiKeys;

    QHash<QUrl, Thing *> m_priceRequests;
    QHash<QNetworkReply *, Thing *>  m_httpRequests;

    void getPriceCall(Thing *thing);

private slots:
    void onPluginTimer();
    void onPriceCallFinished();
};

#endif // INTEGRATIONPLUGINCOINMARKETCAP_H
