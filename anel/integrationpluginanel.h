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

#ifndef INTEGRATIONPLUGINANEL_H
#define INTEGRATIONPLUGINANEL_H

#include "integrations/integrationplugin.h"

#include <QUdpSocket>

#include <QNetworkAccessManager>

class PluginTimer;
class Discovery;

class IntegrationPluginAnel: public IntegrationPlugin
{
    Q_OBJECT

    Q_PLUGIN_METADATA(IID "io.nymea.IntegrationPlugin" FILE "integrationpluginanel.json")
    Q_INTERFACES(IntegrationPlugin)

public:
    explicit IntegrationPluginAnel();
    ~IntegrationPluginAnel();

    void init() override;
    void discoverThings(ThingDiscoveryInfo *info) override;
    void startPairing(ThingPairingInfo *info) override;
    void confirmPairing(ThingPairingInfo *info, const QString &username, const QString &secret) override;
    void setupThing(ThingSetupInfo *info) override;
    void postSetupThing(Thing *thing) override;
    void thingRemoved(Thing *thing) override;
    void executeAction(ThingActionInfo *info) override;

private slots:
    void refreshStates();



private:
    void setConnectedState(Thing *thing, bool connected);

    void setupHomeProDevice(ThingSetupInfo *info);
    void setupAdvDevice(ThingSetupInfo *info);

    void refreshHomePro(Thing *thing);
    void refreshAdv(Thing *thing);
    void refreshAdvTemp(Thing *thing);

private:
    Discovery *m_discovery = nullptr;
    PluginTimer *m_pollTimer = nullptr;
    PluginTimer *m_discoverTimer = nullptr;

    QHash<QString, QHostAddress> m_ipCache;
};

#endif // INTEGRATIONPLUGINANEL_H
