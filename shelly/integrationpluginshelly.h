/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*
* Copyright 2013 - 2020, nymea GmbH
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

#ifndef INTEGRATIONPLUGINSHELLY_H
#define INTEGRATIONPLUGINSHELLY_H

#include "integrations/integrationplugin.h"

#include "extern-plugininfo.h"

#include <coap/coap.h>
#include <QHostAddress>

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
    void setupThing(ThingSetupInfo *info) override;
    void postSetupThing(Thing *thing) override;
    void thingRemoved(Thing *thing) override;
    void executeAction(ThingActionInfo *info) override;

private slots:
    void joinMulticastGroup();
    void onMulticastMessageReceived(const QHostAddress &source, const CoapPdu &pdu);

    void updateStatus();
    void fetchStatusGen1(Thing *thing);
    void fetchStatusGen2(Thing *thing);

private:
    void setupGen1(ThingSetupInfo *info);
    void setupGen2(ThingSetupInfo *info);
    void setupShellyChild(ThingSetupInfo *info);

    QHostAddress getIP(Thing *thing) const;

    void handleInputEvent(Thing *thing, const QString &buttonName, const QString &inputEventString, int inputEventCount);

    QVariantMap createRpcRequest(const QString &method);

private:
    ZeroConfServiceBrowser *m_zeroconfBrowser = nullptr;
    PluginTimer *m_statusUpdateTimer = nullptr;
    PluginTimer *m_reconfigureTimer = nullptr;

    Coap *m_coap = nullptr;

    QHash<Thing*, ShellyJsonRpcClient*> m_rpcClients;
};

#endif // INTEGRATIONPLUGINSHELLY_H
