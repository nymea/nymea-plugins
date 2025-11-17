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

#ifndef INTEGRATIONPLUGINOSDOMOTICS_H
#define INTEGRATIONPLUGINOSDOMOTICS_H

#include <integrations/integrationplugin.h>
#include <coap/coap.h>
#include <plugintimer.h>

#include <QHash>
#include <QDebug>
#include <QNetworkReply>

class IntegrationPluginOsdomotics : public IntegrationPlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "io.nymea.IntegrationPlugin" FILE "integrationpluginosdomotics.json")
    Q_INTERFACES(IntegrationPlugin)

public:
    explicit IntegrationPluginOsdomotics();
    ~IntegrationPluginOsdomotics();

    void init() override;
    void setupThing(ThingSetupInfo *info) override;
    void thingRemoved(Thing *thing) override;
    void postSetupThing(Thing *thing) override;
    void executeAction(ThingActionInfo *info) override;

private:
    PluginTimer *m_pluginTimer = nullptr;
    Coap *m_coap = nullptr;
    QHash<QNetworkReply *, Thing *> m_asyncSetup;
    QHash<QNetworkReply *, Thing *> m_asyncNodeRescans;

    QHash<CoapReply *, Thing *> m_discoveryRequests;
    QHash<CoapReply *, Thing *> m_updateRequests;
    QHash<CoapReply *, Action> m_toggleLightRequests;

    void scanNodes(Thing *thing);
    void parseNodes(Thing *thing, const QByteArray &data);
    void updateNode(Thing *thing);

    Thing *findDevice(const QHostAddress &address);

private slots:
    void onPluginTimer();
    void onNetworkReplyFinished();
    void coapReplyFinished(CoapReply *reply);

};

#endif // INTEGRATIONPLUGINOSDOMOTICS_H
