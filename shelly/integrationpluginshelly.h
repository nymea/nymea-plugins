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

#ifndef INTEGRATIONPLUGINSHELLY_H
#define INTEGRATIONPLUGINSHELLY_H

#include <integrations/integrationplugin.h>
#include <coap/coap.h>

#include <QHostAddress>
#include <QNetworkRequest>
#include <QUrlQuery>

#include "extern-plugininfo.h"

class ZeroConfServiceBrowser;
class PluginTimer;

class ShellyJsonRpcClient;

class IntegrationPluginShelly: public IntegrationPlugin
{
    Q_OBJECT

    Q_PLUGIN_METADATA(IID "io.nymea.IntegrationPlugin" FILE "integrationpluginshelly.json")
    Q_INTERFACES(IntegrationPlugin)

public:
    explicit IntegrationPluginShelly();
    ~IntegrationPluginShelly() override;

    void init() override;
    void discoverThings(ThingDiscoveryInfo *info) override;
    void startPairing(ThingPairingInfo *info) override;
    void confirmPairing(ThingPairingInfo *info, const QString &username, const QString &password) override;
    void setupThing(ThingSetupInfo *info) override;
    void postSetupThing(Thing *thing) override;
    void thingRemoved(Thing *thing) override;
    void executeAction(ThingActionInfo *info) override;

private slots:
    void joinMulticastGroup();
    void onMulticastMessageReceived(const QHostAddress &source, const CoapPdu &pdu);

    void updateStatus();
    void fetchStatusGen1(Thing *thing);
    void fetchStatusGen2Plus(Thing *thing);

private:
    void setupGen1(ThingSetupInfo *info);
    void setupGen2Plus(ThingSetupInfo *info);
    void setupShellyChild(ThingSetupInfo *info);

    QHostAddress getIP(Thing *thing) const;
    bool isGen2Plus(const QString &shellyId) const;

    void handleInputEvent(Thing *thing, const QString &buttonName, const QString &inputEventString, int inputEventCount);

    QNetworkRequest createHttpRequest(Thing *thing, const QString &path, const QUrlQuery &urlQuery = QUrlQuery());
    QVariantMap createRpcRequest(const QString &method);

private:
    ZeroConfServiceBrowser *m_zeroconfBrowser = nullptr;
    PluginTimer *m_statusUpdateTimer = nullptr;
    PluginTimer *m_reconfigureTimer = nullptr;

    Coap *m_coap = nullptr;
    uint m_multicastWarningPrintCount = 0;

    QHash<Thing*, ShellyJsonRpcClient*> m_rpcClients;
};

#endif // INTEGRATIONPLUGINSHELLY_H
