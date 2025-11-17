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

#ifndef INTEGRATIONPLUGINDWEETIO_H
#define INTEGRATIONPLUGINDWEETIO_H

#include <integrations/integrationplugin.h>

#include <QHash>
#include <QObject>
#include <QPointer>
#include <QHostAddress>
#include <QNetworkReply>

#include "plugintimer.h"

class IntegrationPluginDweetio : public IntegrationPlugin
{
    Q_OBJECT

    Q_PLUGIN_METADATA(IID "io.nymea.IntegrationPlugin" FILE "integrationplugindweetio.json")
    Q_INTERFACES(IntegrationPlugin)

public:
    explicit IntegrationPluginDweetio();
    ~IntegrationPluginDweetio();

    void init() override;
    void setupThing(ThingSetupInfo *info) override;
    void thingRemoved(Thing *thing) override;
    void executeAction(ThingActionInfo *info) override;
    void postSetupThing(Thing *thing) override;

private:
    PluginTimer *m_pluginTimer = nullptr;
    QHash<QNetworkReply *, Thing *> m_postReplies;
    QHash<QNetworkReply *, Thing *> m_getReplies;

    QHash<QNetworkReply *, ThingActionInfo*> m_asyncActions;

    void postContent(const QString &content, Thing *thing, ThingActionInfo *info);
    void processPostReply(const QVariantMap &data, Thing *thing);
    void processGetReply(const QVariantMap &data, Thing *thing);
    void setConnectionStatus(bool status, Thing *thing);

    void getRequest(Thing *thing);
private slots:
    void onNetworkReplyFinished();
    void onPluginTimer();
};

#endif // INTEGRATIONPLUGINDWEETIO_H
