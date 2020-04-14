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

#ifndef INTEGRATIONPLUGINUNIFI_H
#define INTEGRATIONPLUGINUNIFI_H

#include <QObject>

#include "integrations/integrationplugin.h"

#include <QNetworkRequest>

class PluginTimer;

class IntegrationPluginUnifi : public IntegrationPlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "io.nymea.IntegrationPlugin" FILE "integrationpluginunifi.json")
    Q_INTERFACES(IntegrationPlugin)

public:
    explicit IntegrationPluginUnifi(QObject *parent = nullptr);
    ~IntegrationPluginUnifi() override;

    void init() override;
    void discoverThings(ThingDiscoveryInfo *info) override;
    void startPairing(ThingPairingInfo *info) override;
    void confirmPairing(ThingPairingInfo *info, const QString &username, const QString &secret) override;
    void setupThing(ThingSetupInfo *info) override;
    void postSetupThing(Thing *thing) override;
    void thingRemoved(Thing *thing) override;
//    void executeAction(ThingActionInfo *info) override;

private:
    QNetworkRequest createRequest(const QString &address, const QString &path);
    QNetworkRequest createRequest(Thing *thing, const QString &path);

    void markOffline(Thing *thing);
private:
    QHash<ThingDiscoveryInfo*, Things> m_pendingDiscoveries;
    QHash<Thing*, QStringList> m_pendingSiteDiscoveries;

    PluginTimer *m_loginTimer = nullptr;
    PluginTimer *m_pollTimer = nullptr;
};

#endif // INTEGRATIONPLUGINUNIFI_H
