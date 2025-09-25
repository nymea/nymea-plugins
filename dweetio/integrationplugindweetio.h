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
