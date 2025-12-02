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

#ifndef INTEGRATIONPLUGINNANOLEAF_H
#define INTEGRATIONPLUGINNANOLEAF_H

#include <integrations/integrationplugin.h>
#include <plugintimer.h>
#include <network/networkaccessmanager.h>
#include <network/zeroconf/zeroconfservicebrowser.h>

#include "nanoleaf.h"

#include <QHostAddress>

class IntegrationPluginNanoleaf: public IntegrationPlugin
{
    Q_OBJECT

    Q_PLUGIN_METADATA(IID "io.nymea.IntegrationPlugin" FILE "integrationpluginnanoleaf.json")
    Q_INTERFACES(IntegrationPlugin)

public:
    explicit IntegrationPluginNanoleaf();

    void init() override;
    void discoverThings(ThingDiscoveryInfo *info) override;
    void startPairing(ThingPairingInfo *info) override;
    void confirmPairing(ThingPairingInfo *info, const QString &username, const QString &secret) override;
    void setupThing(ThingSetupInfo *info) override;
    void postSetupThing(Thing *thing) override;
    void thingRemoved(Thing *thing) override;
    void executeAction(ThingActionInfo *info) override;

    void browseThing(BrowseResult *result) override;
    void browserItem(BrowserItemResult *result) override;
    void executeBrowserItem(BrowserActionInfo *info) override;

private:
    ZeroConfServiceBrowser *m_zeroconfBrowser = nullptr;
    PluginTimer *m_pluginTimer = nullptr;
    QHash<ThingId, Nanoleaf *> m_nanoleafConnections;
    QHash<ThingId, Nanoleaf *> m_unfinishedNanoleafConnections;
    QHash<QUuid, ThingActionInfo *> m_asyncActions;
    QHash<Nanoleaf *, ThingPairingInfo *> m_unfinishedPairing;
    QHash<Nanoleaf *, ThingSetupInfo *> m_asyncDeviceSetup;

    QHash<Nanoleaf *, BrowseResult *> m_asyncBrowseResults;
    QHash<QUuid, BrowserActionInfo *> m_asyncBrowserItem;

    Nanoleaf *createNanoleafConnection(const QHostAddress &address, int port);
    QHostAddress getHostAddress(const QString &serialNumber);
    uint getPort(const QString &serialNumber);

public slots:
    void onAuthTokenReceived(const QString &token);
    void onAuthenticationStatusChanged(bool authenticated);
    void onRequestExecuted(QUuid requestId, bool success);
    void onConnectionChanged(bool connected);

    void onControllerInfoReceived(const Nanoleaf::ControllerInfo &controllerInfo);
    void onPowerReceived(bool power);
    void onBrightnessReceived(int percentage);
    void onColorReceived(QColor color);
    void onColorModeReceived(Nanoleaf::ColorMode colorMode);
    void onHueReceived(int hue);
    void onSaturationReceived(int percentage);
    void onEffectListReceived(const QStringList &effects);
    void onColorTemperatureReceived(int kelvin);
    void onSelectedEffectReceived(const QString &effect);
};

#endif // INTEGRATIONPLUGINNANOLEAF_H
