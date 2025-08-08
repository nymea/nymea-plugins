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
