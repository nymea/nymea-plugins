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

#ifndef INTEGRATIONPLUGINTADO_H
#define INTEGRATIONPLUGINTADO_H

#include "tado.h"
#include "extern-plugininfo.h"

#include <plugintimer.h>
#include <integrations/integrationplugin.h>
#include <network/oauth2.h>

#include <QHash>

#include <QTimer>

class IntegrationPluginTado : public IntegrationPlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "io.nymea.IntegrationPlugin" FILE "integrationplugintado.json")
    Q_INTERFACES(IntegrationPlugin)

public:
    explicit IntegrationPluginTado();
    void startPairing(ThingPairingInfo *info) override;
    void confirmPairing(ThingPairingInfo *info, const QString &username, const QString &secret) override;
    void setupThing(ThingSetupInfo *info) override;
    void thingRemoved(Thing *thing) override;
    void postSetupThing(Thing *thing) override;
    void executeAction(ThingActionInfo *info) override;

private:
    PluginTimer *m_pluginTimer = nullptr;
    QHash<ThingId, Tado*> m_unfinishedTadoAccounts;

    QHash<ThingId, Tado*> m_tadoAccounts;
    QHash<QUuid, ThingActionInfo *> m_asyncActions;

private slots:
    void onPluginTimer();

    void onConnectionChanged(bool connected);
    void onAuthenticationStatusChanged(bool authenticated);
    void onRequestExecuted(QUuid requestId, bool success);
    void onHomesReceived(QList<Tado::Home> homes);
    void onZonesReceived(const QString &homeId, QList<Tado::Zone> zones);
    void onZoneStateReceived(const QString &homeId,const QString &zoneId, Tado::ZoneState sate);
    void onOverlayReceived(const QString &homeId, const QString &zoneId, const Tado::Overlay &overlay);
};

#endif // INTEGRATIONPLUGINTADO_H
