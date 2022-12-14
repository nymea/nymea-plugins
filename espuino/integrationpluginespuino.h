/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*
* Copyright 2013 - 2022, nymea GmbH
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

#ifndef INTEGRATIONPLUGINESPUINO_H
#define INTEGRATIONPLUGINESPUINO_H

#include "integrations/integrationplugin.h"
#include "plugintimer.h"

#include <QHash>

#include "extern-plugininfo.h"

class MqttChannel;
class ZeroConfServiceBrowser;

class IntegrationPluginEspuino : public IntegrationPlugin
{
    Q_OBJECT

    Q_PLUGIN_METADATA(IID "io.nymea.IntegrationPlugin" FILE "integrationpluginespuino.json")
    Q_INTERFACES(IntegrationPlugin)

public:
    void init() override;
    void discoverThings(ThingDiscoveryInfo *info) override;
    void setupThing(ThingSetupInfo *info) override;
    void thingRemoved(Thing *thing) override;
    void executeAction(ThingActionInfo *info) override;
    void browseThing(BrowseResult *result) override;
    void browseThing(BrowseResult *result, const QString &path);
    void browserItem(BrowserItemResult *result) override;
    BrowserItem browserItemFromQuery(const QUrlQuery &query);
    void executeBrowserItem(BrowserActionInfo *info) override;
    void executeBrowserItemAction(BrowserItemActionInfo *info) override;

private slots:
    void onClientConnected(MqttChannel *channel);
    void onClientDisconnected(MqttChannel *channel);
    void onPublishReceived(MqttChannel* channel, const QString &topic, const QByteArray &payload);

private:
    QString getHost(Thing *thing) const;

    ZeroConfServiceBrowser *m_zeroConfBrowser;
    QHash<Thing*, MqttChannel*> m_mqttChannels;
    QHash<ThingClassId, ParamTypeId> m_ipAddressParamTypeMap;
    QMap<QString, QPointer<ThingActionInfo>> m_pendingActions;
};

#endif // INTEGRATIONPLUGINESPUINO_H
